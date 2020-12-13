
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "simplefs.h"

char *builtin_str[] = {
        "ls",
};

int (*builtin_func[])(char **) = {
        &my_ls,
};

int buildins_num()
{
    return sizeof(builtin_str) / sizeof(char *);
}

int main() {
    startsys();
    return 0;
}
