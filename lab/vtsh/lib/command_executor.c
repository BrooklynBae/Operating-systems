#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/time.h>

#define MAGIC_ERR 127
#define NANOSECONDS_IN_MILLISECOND 1000000
#define MILLISECONDS_IN_SECOND 1000

int execute_command(char *args[], int is_interractive) {

    struct timeval start_tv;
    struct timeval end_tv;
    long long start = 0;
    long long end = 0;
    double final_time = 0;

    int vfork_status = 0;
    int idx = 0;

    gettimeofday(&start_tv, NULL);

    pid_t pid = vfork();

    if (pid == 0) {
        execvp(args[0],args);
        printf("Command not found\n");
        _exit(MAGIC_ERR);
    } else if (pid > 0) {
        waitpid(pid, &vfork_status, 0);

        gettimeofday(&end_tv, NULL);
        start = start_tv.tv_sec * NANOSECONDS_IN_MILLISECOND + start_tv.tv_usec;
        end = end_tv.tv_sec * NANOSECONDS_IN_MILLISECOND + end_tv.tv_usec;
        final_time = (double)(end - start) / MILLISECONDS_IN_SECOND;
        if (is_interractive == 1) {
            printf("Время выполнения: %.3f мс\n", final_time);
        }

        if (WIFEXITED(vfork_status)) {
            return WEXITSTATUS(vfork_status);
        }
        return 1;
    } else {
        perror("vfork");
        return -1;
    }
}