#include "simplefs.h"


void startsys()
{
    fs_head = (unsigned char *) malloc(DISK_SIZE);
    memset(fs_head, 0, DISK_SIZE);
    FILE* fp;

    if ((fp = fopen(SYS_PATH, "r")) != NULL)
    {
        fread(fs_head, DISK_SIZE, 1, fp);
        fclose(fp);
    }
    else
    {
        printf("System is not initialized, now create system file\n");
        format();
        printf("Initialized success!\n");
    }
    //init the first openfile
    fcb_copy(&openfile_list[0].open_fcb, ((fcb *)(fs_head + 5 * DISK_SIZE)));
    strcpy(openfile_list[0].dir, ROOT);
    openfile_list[0].count = 0;
    openfile_list[0].fcbstate = 0;
    openfile_list[0].topenfile = 1;
    curdir = 0;

    //init the other openfile
    fcb *empty = (fcb *)malloc(sizeof(fcb));
    set_fcb(empty, '\0', '\0', 0, 0, 0, 0);
    for (int i = 1; i < MAXOPENFILE; i++)
    {
        fcb_copy(&openfile_list[i].open_fcb, empty);
        strcpy(openfile_list[i].dir, '\0');
        openfile_list[i].count = 0;
        openfile_list[i].fcbstate = 0;
        openfile_list[i].topenfile = 0;
    }

    strcpy(currentdir, openfile_list[curdir].dir);
    startp = ((block0 *)fs_head)->start_block;
    free(empty);
}


void format()
{
    unsigned char *ptr = fs_head;
    FILE * fp;
    int first;
    //block0
    block0 *init_block = (block0 *)ptr;
    strcpy(init_block->information, "Disk Size = 1MB, Block Size = 1KB, Block0 in 0, FAT0/1 in 1/3, Root Directory in 5");
    init_block->root = 5;
    init_block->start_block = (unsigned char *)(init_block + BLOCK_SIZE * 7);
    ptr = ptr + BLOCK_SIZE;


    //allocate 5 blocks : block0 1  FAT0  2  FAT1  2
    set_free_block(get_free_block(1), 1, 0);
    set_free_block(get_free_block(2), 2, 0);
    set_free_block(get_free_block(2), 2, 0);

    ptr = ptr + BLOCK_SIZE * 4;

    //root directory 2 blocks
    fcb *root = (fcb *)ptr;
    first = get_free_block(ROOT_BLOCK_NUM);
    set_free_block(first, ROOT_BLOCK_NUM, 0);
    set_fcb(root, ".", "di", 0, first, BLOCK_SIZE * 2, 0);
    root++;
    set_fcb(root, ".", "di", 0, first, BLOCK_SIZE * 2, 0);
    root++;

    for (int i = 2; i < BLOCK_SIZE * 2 / (sizeof(fcb)); i++, root++)
    {
        root->free = 0;
    }

    memset(fs_head + BLOCK_SIZE * 7, 'a', 15);
    fp = fopen(SYS_PATH, "w");
    fwrite(fs_head, DISK_SIZE, 1, fp);
    fclose(fp);

}

int my_ls(char **args)
{
    int first = openfile_list[curdir].open_fcb.first_block_num;
    char fullname[NAMELENGTH], data[16], time[16];
    int length = BLOCK_SIZE;
    fcb *root = (fcb *)(fs_head + BLOCK_SIZE * first);
    block0 *init_block = (block0 *)fs_head;
    if (first == init_block->root)
    {
        length = BLOCK_SIZE * ROOT_BLOCK_NUM;
    }

    for (int i = 0; i < length / sizeof(fcb); i++, root++)
    {
        if (root->free == 0)
        {
            continue;
        }

        trans_date(data, root->create_data);
        trans_time(time, root->create_time);
        get_fullname(fullname, root);
        if (root->attribute == 0)
        {
            printf("%s", FOLDER_COLOR);
            printf("%s\n", fullname);
            printf("%s", DEFAULT_COLOR);
        } else {
            printf("%s\n", fullname);
        }
        printf("%d\t%6d\t%6ld\t%s\t%s\t\n", root->attribute, root->first_block_num, root->file_length, data, time);
    }
}

int get_free_block(int count)
{
    unsigned char *ptr = fs_head;
    fat *fat0 = (fat *)(ptr + BLOCK_SIZE);
    int fat[BLOCK_NUM];
    int flag = 0;
    for (int i = 0; i < BLOCK_NUM; i++)
    {
        fat[i] = fat0->id;
    }
    int j = 0;
    for (int i = 0; i < BLOCK_NUM - count; i++)
    {
        for (j = i; j < i + count; j++)
        {
            if (fat[i] > 0)
            {
                flag = 1;
                break;
            }
        }
        if (flag)
        {
            flag = 0;
            i = j;
        }
        else
        {
            return i;
        }
    }
    return 0;
}


int set_free_block(unsigned short first, unsigned short length, int mode)
{
    fat *flag = (fat *)(fs_head + BLOCK_SIZE);
    fat *fat0 = (fat *)(fs_head + BLOCK_SIZE);
    fat *fat1 = (fat *)(fs_head + BLOCK_SIZE * 3);

    int i;
    for (i = 0; i < first; i++, fat0++, fat1++);

    if (mode == 1) {
        //free space
        int offset;
        while (fat0->id != END)
        {
            offset = fat0->id - (fat0 -flag) / sizeof(fat);
            fat0->id = FREE;
            fat1->id = FREE;
            fat0 += offset;
            fat1 += offset;
        }
        fat0->id = FREE;
        fat1->id = FREE;
    }
    else if (mode == 2)
    {
        for (i = 0; i < BLOCK_NUM; i++, fat0++, fat1++)
        {
            fat0->id = FREE;
            fat1->id = FREE;
        }
    }
    else
    {
        for (; i < first + length - 1; i++, fat0++, fat1++)
        {
            fat0->id = first + 1;
            fat1->id = first + 1;
        }
        fat0->id = END;
        fat1->id = END;
    }
    return 0;
}

void get_fullname(char *fullname, fcb *fcb1)
{
    memset(fullname, '\0', NAMELENGTH);
    strcat(fullname, fcb1->filename);
    if (fcb1->attribute == 1)
    {
        strncat(fullname, ".", 2);
        strncat(fullname, fcb1->exname, 3);
    }
}

unsigned short get_time(struct tm *timeinfo)
{
    int hour, min, sec;
    unsigned short result;

    hour = timeinfo->tm_hour;
    min = timeinfo->tm_min;
    sec = timeinfo->tm_sec;
    result = (hour << 11) + (min << 5) + (sec >> 1);

    return result;
}

char *trans_time(char *stime, unsigned short time) {
    int hour, min, sec;
    memset(stime, '\0', 16);

    hour = time & 0xfc1f;
    min = time & 0x03e0;
    sec = time & 0x001f;
    sprintf(stime, "%02d:%02d:%02d", hour >> 11, min >> 5, sec << 1);
    return stime;
}


unsigned short get_date(struct tm *timeinfo)
{
    int year, month, day;
    unsigned short result;

    year = timeinfo->tm_year;
    month = timeinfo->tm_mon;
    day = timeinfo->tm_mday;

    result = (year << 9) + (month << 5) + day;

    return result;
}

char *trans_date(char *sdate, unsigned short date) {
    int year, month, day;
    memset(sdate, '\0', 16);

    year = date & 0xfe00;
    month = date & 0x01e0;
    day = date & 0x001f;
    sprintf(sdate, "%04d-%02d-%02d", (year >> 9) + 1900, (month >> 5) + 1, day);
    return sdate;
}

int set_fcb(fcb *f, const char *filename, const char *exname, unsigned char attr, unsigned short first, unsigned long length, char ffree)
{
    time_t *now = (time_t *)malloc(sizeof(time_t));
    struct tm *timeinfo;
    time(now);
    timeinfo = localtime(now);

    memset(f->filename, 0, 8);
    memset(f->exname, 0, 3);
    strncpy(f->filename, filename, 8);
    strncpy(f->exname, exname, 3);
    f->attribute = attr;
    f->create_time = get_time(timeinfo);
    f->create_data = get_date(timeinfo);
    f->first_block_num = first;
    f->file_length = length;
    f->free = ffree;

    free(now);
    return 0;
}

fcb *fcb_copy(fcb *dest, fcb *src)
{
    memset(dest->filename, '\0', 8);
    memset(dest->exname, '\0', 3);

    strcpy(dest->filename, src->filename);
    strcpy(dest->exname, src->exname);

    dest->attribute = src->attribute;
    dest->create_data = src->create_data;
    dest->create_time = src->create_time;
    dest->file_length = src->file_length;
    dest->first_block_num = dest->first_block_num;
    dest->free = src->free;

    return dest;
}