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
#define PATHLENGTH         80
#define WRITESIZE          2 * BLOCK_SIZE
#define SYS_PATH           "./file"
#define ROOT               "/"
#define DELIM              "/"
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
    unsigned short time;
    unsigned short date;
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
    char fcbstate;
    char topenfile;
}useropen;


unsigned char *fs_head;
useropen openfile_list[MAXOPENFILE];
int curid;
char currentdir[80];
unsigned char *start;


void startsys();
void my_exit_sys();
fcb *fcb_cpy(fcb *dest, fcb *src);
int set_fcb(fcb *f, const char *filename, const char *exname, unsigned char attribute,
            unsigned short first, unsigned long length, char ffree);
int set_free(unsigned short first, unsigned short length, int mode);
int get_free(int count);
void my_ls();
void get_fullname(char *fullname, fcb *fcb1);
void format();
int my_mkdir(char *foldername);
int do_mkdir(const char *parpath, const char *dirname);
int my_rmdir(char *foldername);
char *get_abspath(char *abspath, const char *relpath);
fcb *find_fcb(const char *path);
fcb *find_fcb_r(char *token, int first);
void init_folder(int first, int second);
int my_cd(char *foldername);
void do_cd(int fd);
int my_close(char *filename);
void do_close(int fd);
int my_open(char *filename);
int do_open(char *path);
int get_useropen();
int my_create(char *filename);
int do_create(const char *parpath, const char *filename);
int my_rm(char *filename);
int my_read(char *filename);
int do_read(int fd, int length, char *text);
int my_write(char *filename);
int do_write(int fd, int len, char *content, char mode);

#endif //SIMPLEFS_SIMPLEFS_H
