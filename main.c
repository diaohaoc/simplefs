
#include "simplefs.h"


int main() {
    char commend[80];
    startsys();
    printf("[%s]:", openfile_list[curid].dir);
    while (~scanf("%s", commend)) {
        if (!strcmp(commend, "ls")) {
            my_ls();
        } else if (!strcmp(commend, "mkdir")) {
            scanf("%s", commend);
            my_mkdir(commend);
        } else if (!strcmp(commend, "rmdir")) {
            scanf("%s", commend);
            my_rmdir(commend);
        } else if (!strcmp(commend, "cd")) {
            scanf("%s", commend);
            my_cd(commend);
        } else if (!strcmp(commend, "create")) {
            scanf("%s", commend);
            my_create(commend);
        } else if (!strcmp(commend, "rm")) {
            scanf("%s", commend);
            my_rm(commend);
        } else if (!strcmp(commend, "open")) {
            scanf("%s", commend);
            my_open(commend);
        } else if (!strcmp(commend, "close")) {
            scanf("%s", commend);
            my_close(commend);
        } else if (!strcmp(commend, "write")) {
            scanf("%s", commend);
            my_write(commend);
        } else if (!strcmp(commend, "read")) {
            scanf("%s", commend);
            my_read(commend);
        } else if (!strcmp(commend, "exit")) {
            my_exit_sys();
            return 0;
        }
        printf("[%s]:", openfile_list[curid].dir);
    }
    return 0;
}


void my_exit_sys() {
    FILE *fp;
    for (int i = 0; i < MAXOPENFILE; i++) {
        do_close(i);
    }

    fp = fopen(SYS_PATH, "w");
    fwrite(fs_head, DISK_SIZE, 1, fp);
    free(fs_head);
    fclose(fp);
}

void startsys(){
    fs_head = (unsigned char *)malloc(DISK_SIZE);
    memset(fs_head, 0, DISK_SIZE);
    FILE *fp;
    if ((fp = fopen(SYS_PATH, "r")) != NULL) {
        fread(fs_head, DISK_SIZE, 1, fp);
        fclose(fp);
    } else {
        printf("System is not initialized, now create system file\n");
        format();
        printf("Init success!\n");
    }
    //用户打开文件表中根目录文件的打开项初始化
    fcb_cpy(&openfile_list[0].open_fcb, ((fcb *)(fs_head + 5 * BLOCK_SIZE)));
    strcpy(openfile_list[0].dir, ROOT);
    openfile_list[0].count = 0;
    openfile_list[0].fcbstate = 0;
    openfile_list[0].topenfile = 1;
    curid = 0;

    //初始化其他文件打开表的目录项
    fcb *empty = (fcb *)malloc(sizeof(fcb));
    set_fcb(empty, "/0", "/0", 0, 0, 0, 0);
    for (int i = 1; i < MAXOPENFILE; i++) {
        fcb_cpy(&openfile_list[i].open_fcb, empty);
        strcpy(openfile_list[i].dir, "\0");
        openfile_list[i].count = 0;
        openfile_list[i].fcbstate = 0;
        openfile_list[i].topenfile = 0;
    }

    //初始化全局变量
    strcpy(currentdir, openfile_list[curid].dir);
    start = ((block0 *)fs_head)->start_block;
    free(empty);
}

int set_fcb(fcb *f, const char *filename, const char *exname, unsigned char attribute,
            unsigned short first, unsigned long length, char ffree) {
    memset(f->filename, '\0', 8);
    memset(f->exname, '\0', 3);
    strncpy(f->filename, filename, 7);
    strncpy(f->exname, exname, 2);
    f->attribute = attribute;
    f->first = first;
    f->length = length;
    f->free = ffree;

    return 0;
}

fcb *fcb_cpy(fcb *dest, fcb *src) {
    memset(dest->filename, '\0', 8);
    memset(dest->exname, '\0', 3);

    strcpy(dest->filename, src->filename);
    strcpy(dest->exname, src->exname);
    dest->attribute = src->attribute;
    dest->time = src->time;
    dest->date = src->date;
    dest->first = src->first;
    dest->length = src->length;
    dest->free = src->free;
}

void format(){
    unsigned char *ptr = fs_head;
    FILE *fp;

    //初始化引导块 block0
    block0 *init_block = (block0 *)ptr;
    strcpy(init_block->information, INFO);
    init_block->root = 5;
    init_block->start_block =(unsigned char *)(init_block + 7 * BLOCK_SIZE);
    ptr += BLOCK_SIZE;

    //格式化 fat
    set_free(0, 0, 2);

    //在fat中 分配5个块给block0 和 两个 fat
    set_free(get_free(1), 1, 0);
    set_free(get_free(2), 2, 0);
    set_free(get_free(2), 2, 0);

    ptr += BLOCK_SIZE * 4;

    //初始化根目录
    fcb *root = (fcb *) ptr;
    int first = get_free(ROOT_BLOCK_NUM);
    set_free(first, ROOT_BLOCK_NUM, 0);
    set_fcb(root, ".", "di", 0, first, 2 * BLOCK_SIZE, 1);
    root++;
    set_fcb(root, "..", "di", 0, first, 2 * BLOCK_SIZE, 1);
    root++;

    for (int i = 2; i < BLOCK_SIZE * 2 / sizeof(fcb); i++, root++) {
        root->free = 0;
    }

    fp = fopen(SYS_PATH, "w");
    fwrite(fs_head, DISK_SIZE, 1, fp);
    fclose(fp);
}

int get_free(int count) {
    unsigned char *ptr = fs_head;
    fat *fat0 = (fat *)(ptr + BLOCK_SIZE);
    int flag = 0;
    int j, i;
    int fat[BLOCK_NUM];

    for (int i = 0; i < BLOCK_NUM; i++, fat0++) {
        fat[i] = fat0->id;
    }

    for (i = 0; i < BLOCK_NUM - count; i++) {
        for (j = i; j < count + i; j++) {
            if (fat[j] > 0) {
                flag = 1;
                break;
            }
        }
        if (flag) {
            flag = 0;
            i = j;
        } else {
            return i;
        }
    }
    return -1;
}

int set_free(unsigned short first, unsigned short length, int mode) {
    fat *flag = (fat *)(fs_head + BLOCK_SIZE);
    fat *fat0 = (fat *)(fs_head + BLOCK_SIZE);
    fat *fat1 = (fat *)(fs_head + 3 * BLOCK_SIZE);
    int i;
    int offset;
    for (i = 0; i < first; i++, fat0++, fat1++);

    if (mode == 1) {
        while (fat0->id != END) {
            offset = fat0->id - (fat0 - flag) / sizeof(fat);
            fat0->id = FREE;
            fat1->id = FREE;
            fat0 += offset;
            fat1 += offset;
        }
        fat0->id = FREE;
        fat1->id = FREE;
    } else if (mode == 2) {
        //格式化 fat
        for (int i = 0; i < BLOCK_NUM; i++, fat0++, fat1++) {
            fat0->id = FREE;
            fat1->id = FREE;
        }
    } else {
        for (; i < first + length - 1; i++, fat0++, fat1++) {
            fat0->id = first + 1;
            fat1->id = first + 1;
        }
        fat0->id = END;
        fat1->id = END;
    }
}

void my_ls() {
    int first = openfile_list[curid].open_fcb.first;
    int length = BLOCK_SIZE;
    char fullname[NAMELENGTH];
    fcb *root = (fcb *)(fs_head + BLOCK_SIZE * first);
    block0 *init_block = (block0 *) fs_head;
    if (first == init_block->root) {
        length = BLOCK_SIZE * ROOT_BLOCK_NUM;
    }

    for (int i = 0; i < length / sizeof(fcb); i++, root++) {
        if (root->free == 0) {
            continue;
        }
        get_fullname(fullname, root);
        if (root->attribute == 0) {
            printf("%s\t", fullname);
        } else {
            printf("%s\t", fullname);
        }
        printf("%d\t%6d\t%6ld\t\n", root->attribute, root->first, root->length);
    }
}

void get_fullname(char *fullname, fcb *fcb1) {
    memset(fullname, '\0', NAMELENGTH);
    strcat(fullname, fcb1->filename);
    if (fcb1->attribute == 1) {
        strncat(fullname, ".", 2);
        strncat(fullname, fcb1->exname, 3);
    }
}

int my_mkdir(char *foldername) {
    char abspath[PATHLENGTH];
    char parpath[PATHLENGTH];
    char dirname[NAMELENGTH];
    char *end;

    get_abspath(abspath, foldername);
    end = strrchr(abspath, '/');
    strcpy(dirname, end + 1);
    if (end == abspath) {
        strcpy(parpath, "/");
    } else {
        strncpy(parpath, abspath, strlen(abspath) - strlen(end));
    }

    if (find_fcb(abspath) != NULL) {
        printf("mkdir: cannot create, %s already exists\n", dirname);
        return -1;
    }

    do_mkdir(parpath, dirname);

    return 1;
}

int do_mkdir(const char *parpath, const char *dirname) {
    int second = get_free(1);
    int first = find_fcb(parpath)->first;
    fcb *dir =(fcb *)(fs_head + first * BLOCK_SIZE);
    int flag = 0;
    for (int i = 0; i < BLOCK_SIZE / sizeof(fcb); i++, dir++) {
        if (dir->free == 0) {
            flag = 1;
            break;
        }
    }

    if (!flag) {
        printf("mkdir: cannot create more file\n");
        return -1;
    }

    if (second == -1) {
        printf("mkdir: no more free space\n");
        return -1;
    }

    set_free(second, 1, 0);

    set_fcb(dir, dirname, "di", 0, second, BLOCK_SIZE, 1);
    init_folder(first, second);
    return 0;
}

char *get_abspath(char *abspath, const char *relpath) {
    if (!strcmp(relpath, DELIM) || relpath[0] == '/') {
        strcpy(abspath, relpath);
        return 0;
    }

    char str[PATHLENGTH];
    char *token, *end;
    memset(abspath, '\0', PATHLENGTH);
    strcpy(abspath, currentdir);
    strcpy(str, relpath);
    token = strtok(str, DELIM);
    do {
        if (!strcmp(token, ".")) {
            continue;
        }
        if (!strcmp(token, "..")) {
            end = strrchr(abspath, '/');
            if (end == abspath){
                strcpy(abspath, ROOT);
                continue;
            }
            char ppath[NAMELENGTH];
            strncpy(ppath, abspath, strlen(abspath) - strlen(end));
            strcpy(abspath, ppath);
            continue;
        }
        int len = strlen(abspath);
        if (strcmp(abspath + len - 1, "/")) {
            strcat(abspath, "/");
        }
        strcat(abspath, token);
    } while ((token = strtok(NULL, DELIM)) != NULL);

    return abspath;
}

fcb *find_fcb(const char *path) {
    char abspath[PATHLENGTH];
    get_abspath(abspath, path);
    char *token = strtok(abspath, DELIM);
    if (token == NULL) {
        return (fcb *)(fs_head + BLOCK_SIZE * 5);
    }
    return find_fcb_r(token, 5);
}

fcb *find_fcb_r(char *token, int first) {
    int length = BLOCK_SIZE;
    char fullname[NAMELENGTH] = "\0";
    fcb *root = (fcb *)(fs_head + first * BLOCK_SIZE);
    fcb *dir;
    int i;
    block0 *init_block = (block0 *)fs_head;
    if (init_block->root == first) {
        length = BLOCK_SIZE * ROOT_BLOCK_NUM;
    }

    for (i = 0, dir = root; i < length / sizeof(fcb); i++, dir++) {
        if (dir->free == 0) {
            continue;
        }
        get_fullname(fullname, dir);
        if (!strcmp(token, fullname)) {
            token = strtok(NULL, DELIM);
            if (token == NULL) {
                return dir;
            }
            return find_fcb_r(token, dir->first);
        }
    }
    return NULL;
}

void init_folder(int first, int second) {
    fcb *par = (fcb *)(fs_head + BLOCK_SIZE * first);
    fcb *cur = (fcb *)(fs_head + BLOCK_SIZE * second);

    set_fcb(cur, ".", "di", 0, second, BLOCK_SIZE, 1);
    cur++;
    set_fcb(cur, "..", "di", 0, first, par->length, 1);
    cur++;

    for (int i = 2; i < BLOCK_SIZE / sizeof(fcb); i++, cur++) {
        cur->free = 0;
    }
}

int my_rmdir(char *foldername) {
    fcb *dir;
    if (!strcmp(foldername, ".") || !strcmp(foldername, "..")) {
        printf("rmdir: cannot remove! '.' or '..' is read only\n");
        return -1;
    }

    if (!strcmp(foldername, "/")) {
        printf("rmdir: cannot remove '/'\n");
        return -1;
    }

    dir = find_fcb(foldername);
    if (dir == NULL) {
        printf("rmdir: cannot remove %s, no such folder\n", foldername);
        return -1;
    }

    if (dir->attribute == 1) {
        printf("rmdir: cannot remove %s, Is a data file\n", foldername);
        return -1;
    }

    for (int i = 0; i < MAXOPENFILE; i++) {
        if (openfile_list[i].topenfile == 0) {
            continue;
        }

        if (!strcmp(dir->filename, openfile_list[i].open_fcb.filename) && dir->first == openfile_list[i].open_fcb.first) {
            printf("rmdir: cannot remove %s, folder is open\n", foldername);
            return -1;
        }
    }

    int first = dir->first;
    dir->free = 0;
    dir = (fcb *)(fs_head + first * BLOCK_SIZE);
    dir->free = 0;
    dir++;
    dir->free = 0;
    set_free(first, 1, 1);
    return 1;
}

int my_cd(char *foldername) {
    char abspath[PATHLENGTH];
    fcb *dir;
    int fd;
    memset(abspath, '\0', PATHLENGTH);
    get_abspath(abspath, foldername);
    dir = find_fcb(abspath);
    if (dir == NULL || dir->attribute == 1) {
        printf("cd: No such file or is a data file\n");
        return -1;
    }


    for (int i = 0; i < MAXOPENFILE; i++) {
        if (openfile_list[i].topenfile == 0) {
            continue;
        }

        if (!strcmp(dir->filename, openfile_list[i].open_fcb.filename) && dir->first == openfile_list[i].open_fcb.first) {
            do_cd(i);
            return 1;
        }
    }

    if ((fd = do_open(abspath)) > 0) {
        do_cd(fd);
    }
    return 1;

}

void do_cd(int fd) {
    curid = fd;
    memset(currentdir, '\0', sizeof(currentdir));
    strcpy(currentdir, openfile_list[fd].dir);
}

int my_close(char *filename) {
    fcb *file = find_fcb(filename);
    int flag = 0;
    if (file == NULL) {
        printf("close: cannot close %s, no such file or folder\n", filename);
        return -1;
    }

    for (int i = 0; i < MAXOPENFILE; i++) {
        if (openfile_list[i].topenfile == 0) {
            continue;
        }
        if (!strcmp(openfile_list[i].open_fcb.filename, file->filename) && file->first == openfile_list[i].open_fcb.first) {
            flag = 1;
            do_close(i);
        }
    }

    if (flag == 1) {
        printf("close: %s has closed\n", filename);
    } else {
        printf("close: %s not open\n", filename);
    }

    return 1;
}

void do_close(int fd) {
    if (openfile_list[fd].fcbstate == 1) {
        fcb_cpy(find_fcb(openfile_list[fd].dir), &openfile_list[fd].open_fcb);
    }
    openfile_list[fd].topenfile = 0;
}

int get_useropen() {
    for (int i = 0; i < MAXOPENFILE; i++) {
        if (openfile_list[i].topenfile == 0) {
            return i;
        }
    }
    return -1;
}

int my_open(char *filename) {
    fcb *file = find_fcb(filename);
    int fd;
    char abspath[PATHLENGTH];
    if (file == NULL) {
        printf("open: cannot open %s, no such file or folder\n", filename);
        return -1;
    }

    for (int i = 0; i < MAXOPENFILE; i++) {
        if (openfile_list[i].topenfile == 0) {
            continue;
        }
        if (!strcmp(openfile_list[i].open_fcb.filename, file->filename) && file->first == openfile_list[i].open_fcb.first) {
            printf("open: cannot open %s, file or folder is open\n", filename);
            return -1;
        }
    }

    fd = do_open(get_abspath(abspath, filename));
    return fd;
}

int do_open(char *path) {
    int fd = get_useropen();
    fcb *file = find_fcb(path);
    if (fd == -1) {
        printf("open: cannot open file, no more useropen entry\n");
        return -1;
    }

    fcb_cpy(&openfile_list[fd].open_fcb, file);
    openfile_list[fd].topenfile = 1;
    openfile_list[fd].count = 0;
    memset(openfile_list[fd].dir, '\0', PATHLENGTH);
    strcpy(openfile_list[fd].dir, path);

    return fd;
}

int my_create(char *name) {
    char abspath[PATHLENGTH];
    char parpath[PATHLENGTH];
    char filename[NAMELENGTH];
    char *end;

    get_abspath(abspath, name);
    end = strrchr(abspath, '/');
    strcpy(filename, end + 1);
    if (end == abspath) {
        strcpy(parpath, "/");
    } else {
        strncpy(parpath, abspath, strlen(abspath) - strlen(end));
    }

    if (find_fcb(abspath) != NULL) {
        printf("mkdir: cannot create, %s already exists\n", filename);
        return -1;
    }

    do_create(parpath, filename);

    return 1;
}

int do_create(const char *parpath, const char *filename) {
    char fullname[NAMELENGTH];
    char fname[16], ename[8];
    char *token;
    int first = get_free(1);
    int flag = 0;
    fcb *dir = (fcb *)(fs_head + BLOCK_SIZE * find_fcb(parpath)->first);

    for (int i = 0; i < BLOCK_SIZE / sizeof(fcb); i++, dir++) {
        if (dir->free == 0) {
            flag = 1;
            break;
        }
    }

    if (!flag) {
        printf("create: cannot create more file in %s\n", parpath);
        return -1;
    }

    if (first == -1) {
        printf("get_free: no more space\n");
        return -1;
    }

    memset(fullname, '\0', NAMELENGTH);
    memset(fname, '\0', 8);
    memset(ename, '\0', 3);
    strcpy(fullname, filename);
    token = strtok(fullname, ".");
    strncpy(fname, token, 8);
    token = strtok(NULL, ".");
    if (token == NULL) {
        strncpy(ename, "d", 2);
    } else {
        strncpy(ename, token, 3);
    }

    set_fcb(dir, fname, ename, 1, first, 0, 1);
    set_free(first, 1, 0);
    return 1;
}

int my_rm(char *filename) {
    fcb *file = find_fcb(filename);
    if (file == NULL) {
        printf("rm: cannot remove %s, no such file\n", filename);
        return -1;
    }

    if (file->attribute == 0) {
        printf("rm: cannot remove %s, Is a folder\n", filename);
        return -1;
    }

    for (int i = 0; i < MAXOPENFILE; i++) {
        if (openfile_list[i].topenfile == 0) {
            continue;
        }

        if (!strcmp(file->filename, openfile_list[i].open_fcb.filename) && file->first == openfile_list[i].open_fcb.first) {
            printf("rm: cannot remove %s, file is open\n", filename);
            return -1;
        }
    }

    int first = file->first;
    file->free = 0;
    set_free(first, 0, 1);

    return 1;

}

int my_read(char *filename) {
    fcb *file = find_fcb(filename);
    char str[WRITESIZE];
    int length;
    int fd = -1;
    if (file == NULL) {
        printf("read: file not exists\n");
        return -1;
    }

    if (file->attribute == 0) {
        printf("read: cannot access a folder\n");
        return -1;
    }

    memset(str, '\0', WRITESIZE);
    for (int i = 0; i < MAXOPENFILE; i++) {
        if (openfile_list[i].topenfile == 0) {
            continue;
        }

        if (!strcmp(file->filename, openfile_list[i].open_fcb.filename) && file->first == openfile_list[i].open_fcb.first) {
            fd = i;
        }
    }
    if (fd == -1) {
        fd = my_open(filename);
        openfile_list[fd].count = 0;
        length = file->length;
    }
    do_read(fd, length, str);
    fputs(str, stdout);
    printf("\n");
    do_close(fd);
    return -1;

}


int do_read(int fd, int length, char *text) {
    memset(text, '\0', WRITESIZE);
    fat *fat0 = (fat *)(fs_head + BLOCK_SIZE);
    int location = 0;
    int len = length;
    int count = openfile_list[fd].count;
    while (len) {
        char *buf = (char *)malloc(BLOCK_SIZE);
        int logic_block_num = count / BLOCK_SIZE;
        int offset = count % BLOCK_SIZE;
        int physics_block_num = openfile_list[fd].open_fcb.first;
        for (int i = 0; i < logic_block_num; i++) {
            physics_block_num = (fat0 + physics_block_num)->id;
        }
        unsigned char *p = fs_head + BLOCK_SIZE * physics_block_num;
        memcpy(buf, p, BLOCK_SIZE);
        if ((offset + len) <= BLOCK_SIZE) {
            memcpy(&text[location], &buf[offset], len);
            openfile_list[fd].count = openfile_list[fd].count + len;
            location += len;
            len = 0;
        } else {
            memcpy(&text[location], &buf[offset], BLOCK_SIZE - offset);
            openfile_list[fd].count = openfile_list[fd].count + BLOCK_SIZE - offset;
            location += BLOCK_SIZE - offset;
            len = len - (BLOCK_SIZE - offset);
        }
    }
    return location;
}

int my_write(char *filename) {
    fcb *file = find_fcb(filename);
    char str[WRITESIZE];
    char mode;
    char c;
    int j = 0;
    int fd = -1;
    if (file == NULL) {
        printf("write: file not exists\n");
        return -1;
    }

    if (file->attribute == 0) {
        printf("write: cannot write a folder\n");
        return -1;
    }

    for (int i = 0; i < MAXOPENFILE; i++) {
        if (openfile_list[i].topenfile == 0) {
            continue;
        }

        if (!strcmp(file->filename, openfile_list[i].open_fcb.filename) && file->first == openfile_list[i].open_fcb.first) {
            fd = i;
        }
    }

    if (fd == -1) {
        fd = my_open(filename);
    }

    printf("a: append, o: override, t: truncated \n");
    printf("choose write mode: ");
    scanf("%s", &mode);
    getchar();
    while (1) {
        for (; (str[j] = getchar()) != '\n'; j++);
        j++;
        if ((c = getchar()) == '\n') {
            break;
        } else {
            str[j] = c;
            j++;
        }

    }
    int len = do_write(fd, j - 1, str, mode);
    printf("write: write %d bit\n", len);
    do_close(fd);
    return -1;
}

int do_write(int fd, int len, char *content, char mode) {
    fat *fat0 = (fat *)(fs_head + BLOCK_SIZE);
    fat *fat1 = (fat *)(fs_head+ BLOCK_SIZE * 3);

    int count = openfile_list[fd].count;
    char text[WRITESIZE];
    openfile_list[fd].count = 0;
    do_read(fd, openfile_list[fd].open_fcb.length, text);
    openfile_list[fd].count = count;
    int first = openfile_list[fd].open_fcb.first;
    char input[WRITESIZE] = {0};
    strncpy(input, content, len);
    if (mode == 'a') {
        memcpy(text + count, input, len);
    } else {
        memset(text, '\0', openfile_list[fd].open_fcb.length);
        memcpy(text, input, len);
    }

    int length = strlen(text);
    int offset = 0;
    while (length) {
        unsigned char *p = (unsigned char *)(fs_head + first * BLOCK_SIZE);
        if (length <= BLOCK_SIZE) {
            memcpy(p, &text[offset], length);
            offset += length;
            length = 0;
        } else {
            memcpy(p, text, BLOCK_SIZE);
            length -= BLOCK_SIZE;
            fat *fat_cur = fat0 + first;
            if (fat_cur->id == END) {
                int next = get_free(1);
                fat_cur->id = next;
                fat_cur = fat0 + next;
                fat_cur->id = END;
            }
            first = (fat0 + first)->id;
        }
    }

    memcpy(fat1, fat0, 2 * BLOCK_SIZE);
    openfile_list[fd].open_fcb.length = strlen(text);
    openfile_list[fd].fcbstate = 1;
    return strlen(input);
}