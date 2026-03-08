#include <linux/limits.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <stdio.h>
#include "vtsh.h"

#define DECIMAL_BASE 10

#define NANOSECONDS_IN_MILLISECOND 1000000
#define MILLISECONDS_IN_SECOND 1000

/*gcc -o cpu_load cpu_load.c ../lib/loaders_parser.c -I../lib*/
/*--size 4 --repetitions 1*/

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
}*/

Cpu_conf fill_struct_cpu(char *args[]) {
    Cpu_conf command = {};

    int idx = 0;
    for (idx = 0; args[idx] != NULL; idx++) {
        if (strcmp(args[idx], "--size") == 0) {
            command.size = parse_ll(args[idx+1]);
            idx++;
        } 
        if (strcmp(args[idx], "--repetitions") == 0) {
            command.repetitions = parse_ll(args[idx+1]);
            idx++;
        }
    }
    (void) init_matrices(&command);
    return command;
}

void free_matrices(Matrix *matrix) {
    free(matrix->matrix_a);
    free(matrix->matrix_b);
    free(matrix->matrix_c);
}

Matrix init_matrices(Cpu_conf *command) {
    Matrix matrix;
    unsigned long size_mtrx = (*command).size * (*command).size * sizeof(long);
    long element_cnt = (*command).size * (*command).size;

    matrix.matrix_a = malloc(size_mtrx);
    matrix.matrix_b = malloc(size_mtrx);
    matrix.matrix_c = malloc(size_mtrx);

    if (!matrix.matrix_a || !matrix.matrix_b || !matrix.matrix_c) {
        free_matrices(&matrix);
        perror("malloc");
        _exit(-1);
    }

    long idx = 0;
    for (idx = 0; idx < element_cnt; idx++) {
        matrix.matrix_a[idx] = rand();
        matrix.matrix_b[idx] = rand();
    }
    
    transpose_matrix(command, &matrix, size_mtrx, element_cnt);
    return matrix;
}

void transpose_matrix(Cpu_conf *command, Matrix *matrix, unsigned long size_mtrx, long element_cnt) {
    long row = 0;
    unsigned long col = 0;
    long idx = 0;
    long repetitions = (*command).repetitions;
    long *matrix_bT = malloc(size_mtrx);

    long size = command->size;

    struct timeval start_tv;
    struct timeval end_tv;
    long long start = 0;
    long long end = 0;
    double final_time = 0;

    gettimeofday(&start_tv, NULL);

    for (row = 0; row < size; row++) {
        for (col = 0; col < size; col++) {
            matrix_bT[col*size + row] = matrix->matrix_b[row*size + col];
        }
    }
    for (idx = 0; idx < repetitions; idx++) {
        multiply_matrices(command, matrix, matrix_bT);
    }

    gettimeofday(&end_tv, NULL);

    start = start_tv.tv_sec * NANOSECONDS_IN_MILLISECOND + start_tv.tv_usec;
    end = end_tv.tv_sec * NANOSECONDS_IN_MILLISECOND + end_tv.tv_usec;
    final_time = (double)(end - start) / MILLISECONDS_IN_SECOND;

    printf("Время выполнения: %.3f мс\n", final_time);

    free(matrix_bT);
}

void multiply_matrices(Cpu_conf *command, Matrix *matrix, const long *matrix_bT) {
    long row = 0;
    unsigned long col = 0;
    long idx = 0;
    long size = command->size;

    for ( row = 0; row < size; row++) {
        for ( col = 0; col < size; col++) {
            long sum = 0;
            for (idx = 0; idx < size; idx++) {
                sum += matrix->matrix_a[row * size + idx] * matrix_bT[col * size + idx];
            }
            matrix->matrix_c[row * size + col] = sum;
        }
    }
}
