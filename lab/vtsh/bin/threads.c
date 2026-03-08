#include <unistd.h>
#define _GNU_SOURCE

#include "vtsh.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*gcc -o threads threads.c cpu_load.c ema_join.c io_load.c -I../lib -pthread*/

void *cpu_load_thread(void *arg) {
    char **args = (char **)arg;
    fill_struct_cpu(args);
    return NULL;
}

void *io_load_thread(void *arg) {
    char **args = (char **)arg;
    fill_struct(args);
    return NULL;
}

void *join_load_thread(void *arg) {
    char **args = (char **)arg;
    fill_struct_ema(args);
    return NULL;
}

int main() {
    pthread_t threads[3];

    char *cpu_args[] = {
        "--size", "500",
        "--repetitions", "5",
        NULL
    };

    char *io_args[] = {
        "--rw", "write",
        "--block-size", "4096",
        "--block-count", "1000",
        "--file", "io_test_file.dat",
        "--range", "0-0",
        "--direct", "off",
        "--type", "random",
        NULL
    };

    char *join_args[] = {
        "--left-file", "left_table.txt",
        "--right-file", "right_table.txt",
        "--output-file", "join_output.txt",
        "--repetitions", "3",
        NULL
    };

    if (pthread_create(&threads[0], NULL, cpu_load_thread, cpu_args) != 0) {
        perror("pthread_create cpu_load");
        _exit(1);
    }
    if (pthread_create(&threads[1], NULL, io_load_thread, io_args) != 0) {
        perror("pthread_create io_load");
        _exit(1);
    }
    if (pthread_create(&threads[2], NULL, join_load_thread, join_args) != 0) {
        perror("pthread_create join_load");
        _exit(1);
    }

    for (int i = 0; i < 3; i++) {
        pthread_join(threads[i], NULL);
    }

    printf("Все нагрузчики завершены.\n");
    return 0;
}
