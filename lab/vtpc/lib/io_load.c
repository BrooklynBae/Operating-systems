#define _GNU_SOURCE

#include "vtpc.h"
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>

#define MAX_INPUT 1024
#define DECIMAL_BASE 10
#define ALIGNMENT 4096
#define PATTERN 0xAB
#define MODE 0644

#define NANOSECONDS_IN_MILLISECOND 1000000
#define MILLISECONDS_IN_SECOND 1000

/*gcc -o io_load io_load.c ../lib/loaders_parser.c -I../lib*/

/*cmake -S . -B build
cmake --build build*/


long parse_ll(char *str) {
    char *end = 0;
    long res = strtol(str, &end, DECIMAL_BASE);
    return res;
}

Range parse_range(char *str) {
    Range range;
    char *start = 0;
    char *end = 0;
    char *dash = 0;

    dash = strchr(str, '-');
    *dash = '\0';
    start = str;
    end = dash + 1;

    range.start = (int) parse_ll(start);
    range.end = (int) parse_ll(end);

    return range;
}

Args fill_struct(char *args[]) {
    Args command = {};

    int idx = 0;
    for (idx = 0; args[idx] != NULL; idx++) {
        if (strcmp(args[idx], "--rw") == 0) {
            if (strcmp(args[idx+1], "write") == 0) {
                command.is_write = 1;
            }
            idx++;
        } 
        if (strcmp(args[idx], "--block-size") == 0) {
            command.block_size = parse_ll(args[idx+1]);
            idx++;
        }
        if (strcmp(args[idx], "--block-count") == 0) {
            command.block_count = parse_ll(args[idx+1]);
            idx++;
        }
        if (strcmp(args[idx], "--file") == 0) {
            command.path = args[idx+1];
            idx++;
        }
        if (strcmp(args[idx], "--range") == 0) {
            command.range = parse_range(args[idx+1]);
            idx++;
        }
        if (strcmp(args[idx], "--direct") == 0) {
            if (strcmp(args[idx+1], "on") == 0) {
                command.direct = 1;
                command.custom = 0;
            } else if (strcmp(args[idx+1], "off") == 0) {
                command.direct = 0;
                command.custom = 0;
            } else if (strcmp(args[idx+1], "custom") == 0) {
                command.direct = 0;
                command.custom = 1;
            }   
            idx++;
        }
        if (strcmp(args[idx], "--type") == 0) {
            if (strcmp(args[idx+1], "random") == 0) {
                command.rand_access = 1;
            }
        }
    }
    file_work(&command);
    return command;
}

void do_io_loop(Args *command, void *buf, int file_d) {
    long offset = 0;
    int idx = 0;
    int blocks_in_range = (*command).range.end - (*command).range.start + 1;
    if (blocks_in_range == 0) {
        blocks_in_range = 1;
    } 

    for (idx = 0; idx < (*command).block_count; idx++) {

        if ((*command).rand_access == 0) {
            offset = ((*command).range.start + (idx % blocks_in_range)) * (*command).block_size;
        } else {
            offset = ((*command).range.start + (rand() % blocks_in_range)) * (*command).block_size;
        }
        init_pages();
        if (command->custom) {
            vtpc_lseek(file_d, offset, SEEK_SET);
            if (command->is_write) {
                vtpc_write(file_d, buf, command->block_size);
            } else {
                vtpc_read(file_d, buf, command->block_size);
            }
        } else {
            if (command->is_write) {
                pwrite(file_d, buf, command->block_size, offset);
            } else {
                pread(file_d, buf, command->block_size, offset);
            }
        }
    }
}

void file_work(Args *command) {
    unsigned int flags = 0;
    int file_d = 0;
    void *buf = NULL;

    off_t size_before_trunc = 0;
    if ((*command).is_write == 1 && (*command).range.start == 0 && (*command).range.end == 0) {

        struct stat st_pre;
        if (stat((*command).path, &st_pre) == 0) {
            size_before_trunc = st_pre.st_size;
        }
    }

    if ((*command).is_write == 1) {
        flags = (O_RDWR | O_CREAT | O_TRUNC);
    } else {
        flags = O_RDWR;
    }

    if (command->direct || command->custom) {
        if (command->direct && command->block_size % ALIGNMENT != 0) {
            printf("O_DIRECT requires block_size multiple of 4096\n");
            return;
        }
        flags |= O_DIRECT;
    }
    if (command->custom) {
        file_d = vtpc_open(command->path, (int)flags, MODE);
    } else {
        file_d = open((*command).path, (int)flags, MODE);
    }

    if (file_d == -1) {
        perror("open");
        return;
    }

    struct stat stat;
    if (fstat(file_d, &stat) == -1) {
        perror("fstat");
        if (command->custom) {
            vtpc_close(file_d);
        } else {
            close(file_d);
        }
        return;
    }

    if ((*command).range.start == 0 && (*command).range.end == 0) {
        off_t file_size = stat.st_size;

        if ((*command).is_write == 1) {
            file_size = size_before_trunc;
            if (file_size == 0) {
                file_size = (*command).block_count * (*command).block_size;
            }
        }

        long total_blocks = (file_size + (*command).block_size - 1) / (*command).block_size;

        (*command).range.start = 0;
        (*command).range.end = (int) total_blocks - 1;
    }

    if (posix_memalign(&buf, ALIGNMENT, (*command).block_size) != 0) {
        perror("posix_memalign");
        _exit(EXIT_FAILURE);
    }

    if ((*command).is_write == 1) {
        memset(buf, PATTERN, (*command).block_size);
    }

    struct timeval start_tv;
    struct timeval end_tv;
    long long start = 0;
    long long end = 0;
    double final_time = 0;

    gettimeofday(&start_tv, NULL);

    do_io_loop(command, buf, file_d);

    gettimeofday(&end_tv, NULL);

    start = start_tv.tv_sec * NANOSECONDS_IN_MILLISECOND + start_tv.tv_usec;
    end = end_tv.tv_sec * NANOSECONDS_IN_MILLISECOND + end_tv.tv_usec;
    final_time = (double)(end - start) / MILLISECONDS_IN_SECOND;

    printf("Время выполнения: %.3f мс\n", final_time);

    free(buf);
    if (command->custom) {
        vtpc_close(file_d);
    } else {
        close(file_d);
    }
}
