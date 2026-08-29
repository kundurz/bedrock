#define _GNU_SOURCE

#include <signal.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

#include <bedrock.h>

#define REQUEST_SIZE 4096
#define WRITE_SIZE (REQUEST_SIZE + 4096)

void overwrite_allocation(void *pointer, size_t bytes)
{
    volatile unsigned char *cursor = pointer;

    for (size_t i = 0; i < bytes; ++i)
        cursor[i] = 'A';
}

void glibc_overflow(void)
{
    void *pointer = malloc(REQUEST_SIZE);
    if (pointer == NULL)
        _exit(2);

    overwrite_allocation(pointer, WRITE_SIZE);
    _exit(0);
}

void bedrock_overflow(void)
{
    void *pointer = bedrock_alloc(REQUEST_SIZE);
    if (pointer == NULL)
        _exit(2);

    overwrite_allocation(pointer, WRITE_SIZE);
    _exit(0);
}

int run_scenario(const char *name, void (*scenario)(void))
{
    pid_t child = fork();
    if (child == -1) {
        perror("fork");
        return -1;
    }

    if (child == 0)
        scenario();

    int status;
    if (waitpid(child, &status, 0) == -1) {
        perror("waitpid");
        return -1;
    }

    if (WIFSIGNALED(status)) {
        printf("%s: overflow stopped by signal %d\n",
               name, WTERMSIG(status));
        return 1;
    }

    if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
        printf("%s: overflow completed without an immediate fault\n", name);
        return 0;
    }

    printf("%s: scenario failed with exit status %d\n",
           name, WEXITSTATUS(status));
    return -1;
}

int main(void)
{
    printf("Requested bytes: %d\n", REQUEST_SIZE);
    printf("Bytes written:   %d\n\n", WRITE_SIZE);

    int glibc_result = run_scenario("glibc malloc", glibc_overflow);
    int bedrock_result = run_scenario("Bedrock", bedrock_overflow);

    if (glibc_result == 0 && bedrock_result == 1) {
        puts("\nBedrock's trailing guard page stopped the overflow.");
        return 0;
    }

    puts("\nThe expected comparison was not observed on this run.");
    return 1;
}
