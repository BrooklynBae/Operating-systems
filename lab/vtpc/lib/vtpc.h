#pragma once

#include <sys/types.h>

#define MAX_ARGS 20

typedef struct {
    int start;
    int end;
} Range;

typedef struct {
    int is_write;
    long block_size;
    long block_count;
    char *path;
    Range range;
    int direct;
    int custom;
    int rand_access;
} Args;

int vtpc_open(const char* path, int mode, int access);
int vtpc_close(int fd);
ssize_t vtpc_read(int fd, void* buf, size_t count);
ssize_t vtpc_write(int fd, const void* buf, size_t count);
off_t vtpc_lseek(int fd, off_t offset, int whence);
int vtpc_fsync(int fd);
void init_pages();

void file_work(Args *command);
Args fill_struct(char *args[]);
long parse_ll(char *str);
int parse_args(char *str);
