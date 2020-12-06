//
// Created by 啦啦啦 on 2020/12/6.
//

#ifndef SIMPLEFS_SIMPLEFS_H
#define SIMPLEFS_SIMPLEFS_H
#define BLOCKSIZE 1024


typedef struct FCB{
    char filename[8];
    char exname[3];
    unsigned char attribute;
    unsigned char reverse[10];
    unsigned short create_time;
    unsigned short create_data;
    unsigned short first_block_num;
    unsigned long file_length;
}fcb;

typedef struct FAT{
        unsigned short id;
}fat;

#endif //SIMPLEFS_SIMPLEFS_H
