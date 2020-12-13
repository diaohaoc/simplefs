//
// Created by 啦啦啦 on 2020/12/6.
//

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include <string.h>

#ifndef SIMPLEFS_SIMPLEFS_H
#define SIMPLEFS_SIMPLEFS_H
#define BLOCK_SIZE         1024
#define BLOCK_NUM          1024
#define ROOT_BLOCK_NUM     2
#define MAXOPENFILE        10
#define NAMELENGTH         32
#define DISK_SIZE          1048576
#define SYS_PATH           "./file"
#define ROOT               "/"
#define FOLDER_COLOR       "\e[1;32m"
#define DEFAULT_COLOR      "\e[0m"
#define FREE               0x0000
#define END                0xffff

typedef struct BLOCK0{
    char information[200];
    unsigned short root;
    unsigned char *start_block;
}block0;


typedef struct FCB{
    char filename[8];
    char exname[3];
    unsigned char attribute; // 0:directory   1:file
    unsigned char reverse[10];
    unsigned short create_time;
    unsigned short create_data;
    unsigned short first_block_num; //first block num of file
    unsigned long file_length;
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
int curdir;
char currentdir[80];
unsigned char *startp;


void startsys();

unsigned short get_time(struct tm *timeinfo);
unsigned short get_date(struct tm *timeinfo);

void format();

int set_fcb(fcb *f, const char *filename, const char *exname, unsigned char attr, unsigned short first, unsigned long length, char ffree);
fcb *fcb_copy(fcb *dest, fcb *src);

int get_free_block(int count);
int set_free_block(unsigned short first, unsigned short length, int mode);

int my_ls(char **args);

void get_fullname(char *fullname, fcb *fcb1);
char *trans_time(char *stime, unsigned short time);
char *trans_date(char *sdate, unsigned short date);

#endif //SIMPLEFS_SIMPLEFS_H
