#pragma once

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
    int rand_access;
} Args;

typedef struct {
    long size;
    long repetitions;
} Cpu_conf;

typedef struct {
    long *matrix_a;
    long *matrix_b;
    long *matrix_c;
} Matrix;

typedef struct {
    char *left;
    char *right;
    char *output;
    unsigned int repetitions;
} JoinConf;

const char* vtsh_prompt();
int parse_str(char *str, int is_interractive);
int execute_command(char *args[], int is_interractive);
void prepare_commands(char *str, int is_interractive);
int parse_args(char *str);
long parse_ll(char *str);
Range parse_range(char *str);
Args fill_struct(char *args[]);
Cpu_conf fill_struct_cpu(char *args[]);
void file_work(Args *command);
Matrix init_matrices(Cpu_conf *command);
void transpose_matrix(Cpu_conf *command, Matrix *matrix, unsigned long size_mtrx, long element_cnt);
void multiply_matrices(Cpu_conf *command, Matrix *matrix, const long *matrix_bT);

void fill_struct_ema(char *args[]);
void nested_loop_join(JoinConf *command);