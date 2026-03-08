#include <linux/limits.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/time.h>
#include "vtsh.h"

#define DECIMAL_BASE 10
#define VALUE_SIZE 9
#define MAX_LINE 64

#define NANOSECONDS_IN_MILLISECOND 1000000
#define MILLISECONDS_IN_SECOND 1000

/*gcc -o ema_join ema_join.c ../lib/loaders_parser.c -I../lib*/
/*--left-file left.txt --right-file right.txt --output-file out.txt --repetitions 1*/

typedef struct {
    FILE *fd_left;
    FILE *fd_right;
    FILE *fd_output;
} Files;

typedef struct {
    long idt;
    char value[VALUE_SIZE];
} Row;

/*int main() {
    char input[MAX_INPUT];
    size_t size = 0;

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

long parse_ll(char *str) {
    char *end = 0;
    long res = strtol(str, &end, DECIMAL_BASE);
    return res;
}
*/
void fill_struct_ema(char *args[]) {
    JoinConf command;

    int idx = 0;
    for (idx = 0; args[idx] != NULL; idx++) {
        if (strcmp(args[idx], "--left-file") == 0) {
            command.left = args[idx+1];
            idx++;
        } 
        if (strcmp(args[idx], "--right-file") == 0) {
            command.right = args[idx+1];
            idx++;
        } 
        if (strcmp(args[idx], "--output-file") == 0) {
            command.output = args[idx+1];
            idx++;
        } 
        if (strcmp(args[idx], "--repetitions") == 0) {
            command.repetitions = parse_ll(args[idx+1]);
        }
    }

    nested_loop_join(&command);
}

long read_rows_count(FILE *file) {
    char buf[MAX_LINE];
    if (!fgets(buf, MAX_LINE, file)) {
        return 0;    
    }
    return strtol(buf, NULL, DECIMAL_BASE);
}

int read_row(FILE *file, Row *row) {
    char buf[MAX_LINE];
    char *ptr = NULL;
    char *end = NULL;

    if (!fgets(buf, sizeof(buf), file)) {
        return 0;
    }

    ptr = buf;
    row->idt = strtol(ptr, &end, DECIMAL_BASE);

    if (end == ptr) {
        return 0;
    }

    while (*end == ' ' || *end == '\t') {
        end++;
    }

    strncpy(row->value, end, VALUE_SIZE - 1);
    row->value[VALUE_SIZE - 1] = '\0';

    return 1;
}


void nested_loop_join(JoinConf *command) {
    struct timeval start_tv;
    struct timeval end_tv;
    long long start = 0;
    long long end = 0;
    double final_time = 0;

    gettimeofday(&start_tv, NULL);

    unsigned int rep = 0;
    for (rep = 0; rep < command->repetitions; rep++) {
        FILE *fleft = fopen(command->left, "r");
        FILE *fout = fopen(command->output, "w");

        if (!fleft || !fout) {
            perror("file open");
            return;
        }

        long left_count = read_rows_count(fleft);

        Row left;
        Row right;
        long idx = 0;

        for (idx = 0; idx < left_count; idx++) {
            if (!read_row(fleft, &left)) {
                break;
            }

            FILE *fright = fopen(command->right, "r");
            if (!fright) {
                perror("right open");
                break;
            }

            long right_count = read_rows_count(fright);

            long jdx = 0;
            for (jdx = 0; jdx < right_count; jdx++) {
                if (!read_row(fright, &right)) {
                    break;
                }

                if (left.idt == right.idt) {
                    (void) fprintf(fout, "%ld %s %s\n", left.idt, left.value, right.value);
                }
            }

            (void) fclose(fright);
        }
        (void) fclose(fleft);
        (void) fclose(fout);   
    }

    gettimeofday(&end_tv, NULL);

    start = start_tv.tv_sec * NANOSECONDS_IN_MILLISECOND + start_tv.tv_usec;
    end = end_tv.tv_sec * NANOSECONDS_IN_MILLISECOND + end_tv.tv_usec;
    final_time = (double)(end - start) / MILLISECONDS_IN_SECOND;

    printf("Время выполнения: %.3f мс\n", final_time);
}
