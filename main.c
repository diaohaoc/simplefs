
#include "simplefs.h"



unsigned char *blockaddr[BLOCK_NUM];
block0 init_block;
fat fat0[BLOCK_NUM], fat1[BLOCK_NUM];

int main() {
    int fd;
    char commend[80];
    startsys();

    printf("%s \n", openfile_list[curid].dir);
    while (~scanf("%s", commend)) {
        if (!strcmp(commend, "ls")) {
            my_ls();
        } else if (!strcmp(commend, "exit")) {
            break;
        }
        my_reload(curid);
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
        memcpy(buf + count, text + ret, count);
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
    return;
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
    if (fd < -1 || fd >= MAXOPENFILE) {
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
            if (tot) tot--;
            continue;
        }
        strcpy(realDir[tot++], dirs[i]);
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