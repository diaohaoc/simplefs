
#include "simplefs.h"



unsigned char *blockaddr[BLOCK_NUM];
block0 init_block;
fat fat0[BLOCK_NUM], fat1[BLOCK_NUM];

int main() {
    int fd;
    char commend[80];
    startsys();

    printf("%s", openfile_list[curid].dir);
    while (~scanf("%s", commend)) {
        if (!strcmp(commend, "ls")) {
            my_ls();
        } else if (!strcmp(commend, "exit")) {
            break;
        } else if (!strcmp(commend, "cd")) {
            scanf("%s", commend);
            my_cd(commend);
        } else if (!strcmp(commend, "mkdir")) {
            scanf("%s", commend);
            my_mkdir(commend);
        } else if (!strcmp(commend, "rmdir")) {
            scanf("%s", commend);
            my_rmdir(commend);
        } else if (!strcmp(commend, "rm")) {
            scanf("%s", commend);
            my_rm(commend);
        }
        my_reload(curid);
        printf("%s", openfile_list[curid].dir);
    }
    my_exitsys();
    return 0;
}

void set_fcb(fcb *f, const char *filename, unsigned char attr, unsigned short first){
    strcpy(f->filename, filename);
    f->attribute = attr;
    f->first = first;
    f->free = 0;
    if (attr) {
        f->length = 0;
    } else {
        f->length = 2 * sizeof(fcb);
    }
}

void useropen_init(useropen *user, int dirno, int diroff, const char *dir) {
    user->dirno = dirno;
    user->diroff = diroff;
    strcpy(user->dir, dir);
    user->fcbstate = 0;
    user->topenfile = 1;
    user->count = user->open_fcb.length;
}

void startsys(){
    fs_head = (unsigned char *)malloc(DISK_SIZE);
    for (int i = 0; i < BLOCK_NUM; i++) {
        blockaddr[i] = i * BLOCK_SIZE + fs_head;
    }
    for (int i = 0; i < MAXOPENFILE; i++) {
        openfile_list[i].topenfile = 0;
    }

    FILE *fp = fopen(SYS_PATH, "rb");
    char need_format = 0;
    if (fp != NULL) {
        unsigned char *buf = (unsigned char *)malloc(DISK_SIZE);
        fread(buf, 1, DISK_SIZE, fp);
        memcpy(fs_head, buf, DISK_SIZE);
        memcpy(&init_block, blockaddr[0], sizeof(block0));
        if (!strcmp(init_block.information, INFO)) {
            need_format = 1;
        }
        free(buf);
        fclose(fp);
    } else {
        need_format = 1;
    }

    if (!need_format) {
        memcpy(fat0, blockaddr[1], sizeof(fat));
        memcpy(fat1, blockaddr[3], sizeof(fat));
    } else {
        printf("System is not initialized, now create system file\n");
        format();
        printf("Init success!\n");
    }

    curid = 0;
    memcpy(&openfile_list[curid].open_fcb, blockaddr[5], sizeof(fcb));
    useropen_init(&openfile_list[curid], 5, 0, "~/");
}

void format(){
    strcpy(init_block.information, INFO);
    for (int i = 0; i < BLOCK_NUM; i++) {
        if (i < 5) {
            fat0[i].id = END;
            fat1[i].id = END;
        } else {
            fat0[i].id = FREE;
            fat1[i].id = FREE;
        }
    }
    fat0[5].id = END;
    fcb root;
    set_fcb(&root, ".", 0, 5);
    memcpy(blockaddr[5], &root, sizeof(fcb));
    strcpy(root.filename, "..");
    memcpy(blockaddr[5] + sizeof(fcb), &root, sizeof(fcb));
}

void fatFree(int id) {
    if (id == END) {
        return;
    }
    if (fat0[id].id != END) {
        fatFree(fat0[id].id);
    }
    fat0[id].id = FREE;
}

int getFreeFatId() {
    for (int i = 5; i < BLOCK_NUM; i++) {
        if (fat0[i].id == FREE) {
            return i;
        }
    }
    return END;
}

int getFreeOpenList() {
    for (int i = 0; i < MAXOPENFILE; i++) {
        if (!openfile_list[i].topenfile) {
            return i;
        }
    }
    return -1;
}

int getNextFatId(int id) {
    if (fat0[id].id == END) {
        fat0[id].id = getFreeFatId();
    }
    return fat0[id].id;
}

void my_ls() {
    unsigned char *buf = (unsigned char *)malloc(DISK_SIZE);
    int read_size = read_ls(curid, buf, openfile_list[curid].open_fcb.length);
    if (read_size == -1) {
        free(buf);
        printf("my_ls: error\n");
        return;
    }
    fcb dirfcb;
    for (int i = 0; i < read_size; i += sizeof(fcb)) {
        memcpy(&dirfcb, buf + i, sizeof(fcb));
        if (dirfcb.free){
            continue;
        }
        if (dirfcb.attribute) {
            printf(" %s\n", dirfcb.filename);
        } else {
            printf("%s\n", dirfcb.filename);
        }
    }
    free(buf);
}

int read_ls(int fd, unsigned char *text, int len) {
    int tcount = openfile_list[fd].count;
    openfile_list[fd].count = 0;
    int ret = do_read(fd, text, len);
    openfile_list[fd].count = tcount;
    return ret;
}

int do_read(int fd, unsigned char *text, int len) {
    int blockorder = openfile_list[fd].count >> 10;
    int blockoffset = openfile_list[fd].count % 1024;
    int id = openfile_list[fd].open_fcb.first;
    while (blockorder) {
        --blockorder;
        id = fat0[id].id;
    }
    int ret = fat_read(id, text, blockoffset, len);
    return ret;
}
int fat_read(int id, unsigned char *text, int blockoffset, int len) {
    int ret = 0;
    unsigned char *buf = (unsigned char *)malloc(BLOCK_SIZE);

    int count = 0;
    while (len) {
        memcpy(buf, blockaddr[id], BLOCK_SIZE);
        count = min(len, 1024 - blockoffset);
        memcpy(text + ret, buf + blockoffset, count);
        len -= count;
        ret += count;
        blockoffset = 0;
        id = fat0[id].id;
    }
    free(buf);
    return ret;
}
int do_write(int fd, unsigned char *text, int len) {
    fcb *fcbp = &openfile_list[fd].open_fcb;
    int blockorder = openfile_list[fd].count >> 10;
    int blockoffset = openfile_list[fd].count % 1024;
    unsigned short id = openfile_list[fd].open_fcb.first;
    while (blockorder) {
        --blockorder;
        id = fat0[id].id;
    }
    int ret = fat_write(id, text, blockoffset, len);
    fcbp->length += ret;
    openfile_list[fd].fcbstate = 1;

    if (!fcbp->attribute) {
        fcb tmp;
        memcpy(&tmp, fcbp, sizeof(fcb));
        strcmp(tmp.filename, ".");
        memcpy(blockaddr[fcbp->first], &tmp, sizeof(fcb));

        strcpy(tmp.filename, "..");
        if (fcbp->first == 5) {
            memcpy(blockaddr[fcbp->first] + sizeof(fcb), &tmp, sizeof(fcb));
        }

        unsigned char *buf = (unsigned char *)malloc(DISK_SIZE);
        int read_size = read_ls(fd, buf, fcbp->length);
        if (read_size == -1) {
            printf("do_write: read_ls error!\n");
            return 0;
        }
        fcb dirfcb;
        for (int i = 2 * sizeof(fcb); i < read_size; i += sizeof(fcb)) {
            memcpy(&dirfcb, buf + i, sizeof(fcb));
            if (dirfcb.free || dirfcb.attribute) {
                continue;
            }
            memcpy(blockaddr[dirfcb.first] + sizeof(fcb), &tmp, sizeof(fcb));
        }
    }

    return ret;
}
int fat_write(int id, unsigned char *text, int blockoffset, int len) {
    int ret = 0;
    unsigned char *buf = (unsigned char *)malloc(BLOCK_SIZE);
    if (buf == NULL) {
        printf("fat_write: malloc error!");
        return -1;
    }

    int tlen = len;
    int toffset = blockoffset;
    while (tlen) {
        if (tlen < 1024 - toffset) {
            break;
        }
        tlen -= (1024 - toffset);
        toffset = 0;
        id = getNextFatId(id);
        if (id == END) {
            printf("fat_write: no next fat\n");
            return -1;
        }
    }

    int count = 0;
    while (len) {
        memcpy(buf, blockaddr[id], BLOCK_SIZE);
        count = min(len, 1024 - blockoffset);
        memcpy(buf + blockoffset, text + ret, count);
        memcpy(blockaddr[id], buf, BLOCK_SIZE);
        len -= count;
        ret += count;
        blockoffset = 0;
        id = fat0[id].id;
    }

    free(buf);
    return ret;
}

void my_save(int fd) {
    if (fd < 0 || fd >= MAXOPENFILE) {
        printf("my_save: fd invalid\n");
        return;
    }
    useropen *file = &openfile_list[fd];
    if (file->fcbstate) {
        fat_write(file->dirno, (unsigned char *)&file->open_fcb, file->diroff, sizeof(fcb));
    }
    file->fcbstate = 0;
}

void my_close(int fd) {
    if (fd < 0 || fd >= MAXOPENFILE) {
        printf("my_close: fd invalid\n");
        return;
    }
    if (openfile_list[fd].topenfile == 0) {
        return;
    }
    if (openfile_list[fd].fcbstate) {
        my_save(fd);
    }

    openfile_list[fd].topenfile = 0;
    return;
}

void my_exitsys() {
    for (int i = 0; i < MAXOPENFILE; i++) {
        my_close(i);
    }
    memcpy(blockaddr[0], &init_block, BLOCK_SIZE);
    memcpy(blockaddr[1], fat0, sizeof(fat0));
    memcpy(blockaddr[3], fat0, sizeof(fat0));
    FILE *fp = fopen(SYS_PATH, "wb");
    free(fs_head);
    fclose(fp);
}
void my_reload(int fd) {
    if (fd < 0 || fd >= MAXOPENFILE) {
        printf("my_close: fd invalid\n");
        return;
    }
    fat_read(openfile_list[fd].dirno, (unsigned char *)&openfile_list[fd].open_fcb, openfile_list[fd].diroff, sizeof(fcb));
    return;
}

void my_cd(char *filename) {
    int fd = my_open(filename);
    if (fd < 1 || fd >= MAXOPENFILE) {
        return;
    }
    if (openfile_list[fd].open_fcb.attribute) {
        my_close(fd);
        printf("%s is a file! ", openfile_list[fd].dir);
        return;
    }

    my_close(curid);
    curid = fd;
}

int my_open(char *filename) {
    char dirs[DIRLEN][DIRLEN];
    char newdir[DIRLEN];
    strcpy(newdir, openfile_list[curid].dir);
    strcat(newdir, filename);
    int count = splitDir(dirs, newdir);

    char realDir[DIRLEN][DIRLEN];
    int tot = 0;
    for (int i = 1; i < count; i++) {
        if (!strcmp(dirs[i], ".")) {
            continue;
        }
        if (!strcmp(dirs[i], "..")) {
            if (tot) --tot;
            continue;
        }
        strcpy(realDir[tot++], dirs[i]);
    }

    int fd = getOpenlist(-1, "");

    int flag = 0;
    for (int i = 0; i < tot; i++) {
        int newfd = getOpenlist(fd, realDir[i]);
        if (newfd == -1) {
            flag = 1;
            break;
        }
        my_close(fd);
        fd = newfd;
    }

    if (flag) {
        printf("%s no such file or directory\n", filename);
        openfile_list[fd].topenfile = 0;
        return -1;
    }
    if (openfile_list[fd].open_fcb.attribute) {
        openfile_list[fd].count = 0;
    } else {
        openfile_list[fd].count = openfile_list[fd].open_fcb.length;
    }
    return fd;
}

int getOpenlist(int fd, const char *orgdir) {
    char dir[DIRLEN];
    if (fd == -1) {
        strcpy(dir, "~/");
    } else {
        strcpy(dir, openfile_list[fd].dir);
        strcat(dir, orgdir);
    }

    for (int i = 0; i < MAXOPENFILE; i++) {
        if (i != fd) {
            if (openfile_list[i].topenfile && !strcmp(openfile_list[i].dir, dir)) {
                my_save(i);
            }
        }
    }

    int fileid = getFreeOpenList();
    if (fileid == -1) {
        printf("getOpenList: openfile_list is full!\n");
        return -1;
    }

    useropen *file = &openfile_list[fileid];
    int ret;
    if (fd == -1) {
        ret = getFcb(&file->open_fcb, &file->dirno, &file->diroff, -1, ".");
    } else {
        ret = getFcb(&file->open_fcb, &file->dirno, &file->diroff, fd, orgdir);
    }
    strcpy(file->dir, dir);
    file->fcbstate = 0;
    file->topenfile = 1;

    if (!file->open_fcb.attribute) {
        int len = strlen(file->dir);
        if (file->dir[len - 1] != '/') {
            strcat(file->dir, "/");
        }
    }
    if (ret == -1) {
        file->topenfile = 0;
        return -1;
    }

    return fileid;
}

int getFcb(fcb *fcbp, int *dirno, int *diroff, int fd, const char *dir) {
    if (fd == -1) {
        memcpy(fcbp, blockaddr[5], sizeof(fcb));
        *dirno = 5;
        *diroff = 0;
        return 1;
    }

    useropen *file = &openfile_list[fd];

    unsigned char *buf = (unsigned char *)malloc(DISK_SIZE);
    int read_size = read_ls(fd, buf, file->open_fcb.length);
    if (read_size == -1) {
        printf("getFcb: read_ls error!\n");
        return -1;
    }

    fcb dirfcb;
    int flag = -1;
    for (int i = 0; i < read_size; i += sizeof(fcb)) {
        memcpy(&dirfcb, buf + i, sizeof(fcb));
        if (dirfcb.free) {
            continue;
        }
        if (!strcmp(dirfcb.filename, dir)) {
            flag = i;
            break;
        }
    }

    free(buf);
    if (flag == -1) {
        return -1;
    }
    getPos(dirno, diroff, file->open_fcb.first, flag);
    memcpy(fcbp, &dirfcb, sizeof(fcb));

    return 1;
}

void getPos(int *id, int *offset, unsigned short first, int length) {
    int blockorder = length >> 10;
    *offset = length % 1024;
    *id = first;
    while (blockorder) {
        --blockorder;
        *id = fat0[*id].id;
    }
}

int splitDir(char dirs[DIRLEN][DIRLEN], char *filename) {
    int bg = 0;
    int ed = strlen(filename);
    if (filename[0] == '/') {
        ++bg;
    }
    if (filename[ed - 1] == '/') {
        --ed;
    }

    int row = 0;
    int col = 0;
    for (int i = bg; i < ed; i++) {
        if (filename[i] == '/') {
            dirs[row][col] = '\0';
            col = 0;
            ++row;
        } else {
            dirs[row][col++] = filename[i];
        }
    }
    dirs[row][col] = '\0';
    return row + 1;
}

void splitLastDir(char *dir, char new_dir[2][DIRLEN]) {
    int len = strlen(dir);
    int flag = -1;
    for (int i = 0; i < len; i++) {
        if (dir[i] == '/') {
            flag = i;
        }
    }

    if (flag == -1) {
        printf("splitLastsDir: can not split dir!\n");
        return;
    }
    int tlen = 0;
    for (int i = 0; i < flag; i++) {
        new_dir[0][tlen++] = dir[i];
    }
    new_dir[0][tlen++] = '\0';
    tlen = 0;
    for (int i = flag + 1; i < len; i++) {
        new_dir[1][tlen++] = dir[i];
    }
    new_dir[1][tlen++] = '\0';
}

void my_rm(char *filename) {
    int fd = my_open(filename);
    if (fd >= 0 && fd < MAXOPENFILE) {
        if (openfile_list[fd].open_fcb.attribute == 0) {
            printf("%s is a directory, please use rmdir!\n", openfile_list[fd].open_fcb.filename);
            my_close(fd);
            return;
        }
    }
    openfile_list[fd].open_fcb.free = 1;
    fatFree(openfile_list[fd].open_fcb.first);
    openfile_list[fd].fcbstate = 1;
    my_close(fd);
}

void my_rmdir(char *filename) {
    int fd = my_open(filename);
    if (fd >= 0 && fd < MAXOPENFILE) {
        if (openfile_list[fd].open_fcb.attribute) {
            printf("%s is a file, please use rm!\n", openfile_list[fd].open_fcb.filename);
            my_close(fd);
            return;
        }
    }
    if (!strcmp(openfile_list[fd].dir, openfile_list[curid].dir)) {
        printf("my_dir: can not remove current directory!\n");
        my_close(fd);
        return;
    }

    unsigned char *buf = (unsigned char *)malloc(DISK_SIZE);
    int read_size = read_ls(fd, buf, openfile_list[fd].open_fcb.length);
    if (read_size == -1) {
        my_close(fd);
        free(buf);
        printf("my_rmdir: read_ls error!\n");
        return;
    }

    fcb fcbdir;
    int cnt = 0;
    for (int i = 0; i < read_size; i += sizeof(fcb)) {
        memcpy(&fcbdir, buf + i, sizeof(fcb));
        if (fcbdir.free) {
            continue;
        }
        cnt++;
    }

    if (cnt > 2) {
        my_close(fd);
        printf("my_rmdir: %s is not empty!\n", filename);
        return;
    }

    openfile_list[fd].open_fcb.free = 1;
    fatFree(openfile_list[fd].open_fcb.first);
    openfile_list[fd].fcbstate = 1;
    my_close(fd);
}

int my_touch(char *filename, int attribute, int *rpafd) {
    int len = strlen(filename);
    if (filename[len - 1] == '/') {
        filename[len - 1] = '\0';
    }
    char newDir[DIRLEN] = "./";
    char split_Dir[2][DIRLEN];
    strcat(newDir, filename);
    splitLastDir(newDir, split_Dir);

    int pafd = my_open(split_Dir[0]);
    if (pafd < 0 || pafd >= MAXOPENFILE) {
        printf("my_touch: my_open error!\n");
        return -1;
    }

    unsigned char *buf = (unsigned char *)malloc(DISK_SIZE);
    int read_size = read_ls(pafd, buf, openfile_list[pafd].open_fcb.length);
    if (read_size == -1) {
        printf("my_touch: read_ls error\n");
    }

    fcb dirfcb;
    for (int i = 0; i < read_size; i += sizeof(fcb)) {
        memcpy(&dirfcb, buf + i, sizeof(fcb));
        if (dirfcb.free || dirfcb.attribute) {
            continue;
        }
        if (!strcmp(dirfcb.filename, filename)) {
            printf("%s is already exit\n", filename);
            return -1;
        }
    }

    int fatid = getFreeFatId();
    if (fatid == -1) {
        printf("my_touch: no free fat!\n");
        return -1;
    }
    fat0[fatid].id = END;
    set_fcb(&dirfcb, filename, attribute, fatid);

    memcpy(buf, &dirfcb, sizeof(fcb));
    int write_size = do_write(pafd, buf, sizeof(fcb));
    if (write_size == -1) {
        printf("my_touch: do_write error\n");
        return -1;
    }
    openfile_list[pafd].count += write_size;

    int fd = getFreeOpenList();
    if (fd == -1) {
        printf("my_touch: no free open list\n");
        return -1;
    }

    getPos(&openfile_list[fd].dirno, &openfile_list[fd].diroff, openfile_list[pafd].open_fcb.first, openfile_list[pafd].count - write_size);
    memcpy(&openfile_list[fd].open_fcb, &dirfcb, sizeof(fcb));
    if (attribute) {
        openfile_list[fd].count = 0;
    } else {
        openfile_list[fd].count = openfile_list[fd].open_fcb.length;
    }
    openfile_list[fd].fcbstate = 1;
    openfile_list[fd].topenfile = 1;
    strcpy(openfile_list[fd].dir, openfile_list[pafd].dir);
    strcat(openfile_list[fd].dir, split_Dir[1]);

    free(buf);
    *rpafd = pafd;
    return fd;
}

void my_mkdir(char *filename) {
    int pafd;
    int fd = my_touch(filename, 0, &pafd);
    if (fd == -1) {
        printf("my_mkdir: my_touch error\n");
        return;
    }

    unsigned char *buf = (unsigned char *)malloc(DISK_SIZE);

    fcb dirfcb;
    memcpy(&dirfcb, &openfile_list[fd].open_fcb, sizeof(fcb));
    int fatid = dirfcb.first;
    strcpy(dirfcb.filename, ".");
    memcpy(blockaddr[fatid], &dirfcb, sizeof(fcb));
    memcpy(&dirfcb, &openfile_list[pafd].open_fcb, sizeof(fcb));
    strcpy(dirfcb.filename, "..");
    memcpy(blockaddr[fatid] + sizeof(fcb), &dirfcb, sizeof(fcb));

    my_close(pafd);
    my_close(fd);
    free(buf);
}

int my_create(char *filename) {
    int pafd;
    int fd = my_touch(filename, 1, &pafd);
    if (fd == -1) {
        printf("my_create: my_touch error\n");
        return -1;
    }

    my_close(pafd);
    return fd;
}