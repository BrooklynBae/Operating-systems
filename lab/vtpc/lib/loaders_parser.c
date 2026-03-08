#include "vtpc.h"
#include <ctype.h>
#include <stdio.h>
#include <string.h>

int parse_args(char *str) {
    char *args[MAX_ARGS] = {0};
    char *ptr = str;

    int idx = 0;

    if (*str == 0) {
        return 0;
    }

    while (*ptr) {
        while (isspace(*ptr)) { 
            ptr++;
        }

        if (*ptr == '\0') {    
            break;
        }

        args[idx++] = ptr;       


        while (*ptr && !isspace(*ptr)) { 
            ptr++;
        }

        if (*ptr) {
            *ptr = '\0';
            ptr++;
        }
    }
    args[idx] = NULL;

    fill_struct(args);
    return 0;
}