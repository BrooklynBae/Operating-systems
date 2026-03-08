#include "vtsh.h"
#include <ctype.h>
#include <stdio.h>
#include <string.h>

int parse_str(char *str, int is_interractive) {
    char *ptr = str;
    char *args[MAX_ARGS] = {0};
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
    int result = execute_command(args, is_interractive);
    return result;
}

void prepare_commands(char *str, int is_interractive) {
    char *ptr = str;
    char *command[MAX_ARGS];
    int idx = 0;

    while (*ptr) {
        command[idx++] = ptr;

        while (*ptr && !(ptr[0] == '&' && ptr[1] == '&')) {
            ptr++;
        }

        if (*ptr) {
            *ptr = '\0';
            ptr += 2;
        }
    }

    command[idx] = NULL;

    int jdx = 0;

    for (jdx = 0; command[jdx]; jdx++) {
        int status = parse_str(command[jdx], is_interractive);
        if (status != 0) {
            break;
        }
    }
}
