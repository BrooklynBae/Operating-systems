#define _GNU_SOURCE
#include "vtpc.h"
#include <stdio.h>
#include <string.h>

#define MAX_INPUT 1024

int main() {
    char input[MAX_INPUT];

    while (1) {
        if (!fgets(input, MAX_INPUT, stdin)) {
            break;
        }
        input[strcspn(input, "\n")] = 0;
        if (strcmp(input, "exit") == 0) {
            break;
        }
        parse_args(input);
    }
    return 0;
}
