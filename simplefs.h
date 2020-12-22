//
// Created by 啦啦啦 on 2020/12/6.
//

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>

#ifndef SIMPLEFS_SIMPLEFS_H
#define SIMPLEFS_SIMPLEFS_H
#define BLOCK_SIZE         1024
#define BLOCK_NUM          1024
#define ROOT_BLOCK_NUM     2
#define MAXOPENFILE        10
#define NAMELENGTH         32
#define DISK_SIZE          1048576
#define DIRLEN             80
#define SYS_PATH           "./file"
#define ROOT               "/"
#define GRN                "dir:"
#define INFO               "Disk Size = 1MB, Block Size = 1KB, Block0 in 0, FAT0/1 in 1/3, Root Directory in 5"
#define FREE               0x0000
#define END                0xffff
#define min(X, Y) (((X) < (Y)) ? (X) : (Y))


typedef struct BLOCK0{
    char information[200];
    unsigned short root;
    unsigned char *start_block;
}block0;


typedef struct FCB{
    char filename[8];
    char exname[3];
    unsigned char attribute; // 0:directory   1:file
    unsigned short create_time;
    unsigned short create_data;
    unsigned short first; //first block num of file
    unsigned long length;
    char free; // is the directory empty
}fcb;

typedef struct FAT{
        unsigned short id;
}fat;

typedef struct USEROPEN{

    fcb open_fcb;
    //file current state
    char dir[80];
    int count;
    int dirno;
    int diroff;
    char fcbstate;
    char topenfile;
}useropen;


unsigned char *fs_head;
useropen openfile_list[MAXOPENFILE];
int curid;
char currentdir[80];
unsigned char *startp;


void startsys();

unsigned short get_time(struct tm *timeinfo);
unsigned short get_date(struct tm *timeinfo);

void format();

void set_fcb(fcb *f, const char *filename, unsigned char attr, unsigned short first);
void useropen_init(useropen *user, int dirno, int diroff, const char *dir);
int getFreeFatId();
void fatFree(int id);
int getFreeOpenList();
int getNextFatId();

void my_ls();
void my_save(int fd);
void my_close(int fd);
void my_exitsys();
void my_reload(int fd);
void my_cd(char *filename);
void my_rm(char *filename);
void my_rmdir(char *filename);
void my_mkdir(char *filename);
int my_create(char *filename);
int my_open(char *filename);
int my_touch(char *filename, int attribute, int *rpafd);
int getOpenlist(int fd, const char *orgdir);
int getFcb(fcb *fcbp, int *dirno, int *diroff, int fd, const char *dir);
void getPos(int *id, int *offset, unsigned short first, int length);
int read_ls(int fd, unsigned char *text, int len);
int do_read(int fd, unsigned char *text, int len);
int do_write(int fd, unsigned char *text, int len);
int fat_read(int id, unsigned char *text, int blockoffset, int len);
int fat_write(int id, unsigned char *text, int blockoffset, int len);


int splitDir(char dirs[DIRLEN][DIRLEN], char *filename);
void splitLastDir(char *dir, char new_dir[2][DIRLEN]);
#endif //SIMPLEFS_SIMPLEFS_H
