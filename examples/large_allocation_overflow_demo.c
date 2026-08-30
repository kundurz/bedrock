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

#define BOLD "\033[1m"
#define GREEN "\033[32m"
#define RED "\033[31m"
#define CYAN "\033[36m"
#define RESET "\033[0m"

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

int run_scenario(void (*scenario)(void), int *signal_number)
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
        *signal_number = WTERMSIG(status);
        return 1;
    }

    if (WIFEXITED(status) && WEXITSTATUS(status) == 0)
        return 0;

    return -1;
}

int main(void)
{
    int glibc_signal = 0;
    int bedrock_signal = 0;
    int glibc_result = run_scenario(glibc_overflow, &glibc_signal);
    int bedrock_result = run_scenario(bedrock_overflow, &bedrock_signal);

    printf("\n" BOLD CYAN);
    puts("╭────────────────────────────────────────────────────────────╮");
    puts("│       BEDROCK SECURITY DEMO · LARGE BUFFER OVERFLOW        │");
    puts("╰────────────────────────────────────────────────────────────╯" RESET);

    printf("\n  %-22s %d bytes\n", "requested allocation", REQUEST_SIZE);
    printf("  %-22s %d bytes\n", "attempted write", WRITE_SIZE);
    printf("  %-22s +%d bytes\n", "overflow", WRITE_SIZE - REQUEST_SIZE);

    printf("\n" BOLD "  Allocator             Outcome" RESET "\n");
    puts("  ──────────────────────────────────────────────────────────");

    if (glibc_result == 0)
        printf("  %-21s " RED "overflow completed" RESET "\n", "glibc malloc");
    else if (glibc_result == 1)
        printf("  %-21s stopped by signal %d\n", "glibc malloc", glibc_signal);
    else
        printf("  %-21s scenario error\n", "glibc malloc");

    if (bedrock_result == 1)
        printf("  %-21s " GREEN "blocked by guard page (signal %d)" RESET "\n",
               "Bedrock", bedrock_signal);
    else if (bedrock_result == 0)
        printf("  %-21s " RED "overflow completed" RESET "\n", "Bedrock");
    else
        printf("  %-21s scenario error\n", "Bedrock");

    if (glibc_result == 0 && bedrock_result == 1) {
        printf("\n" GREEN BOLD "  ✓ Bedrock's trailing guard page stopped the overflow." RESET "\n\n");
        return 0;
    }

    printf("\n" RED BOLD "  ✗ The expected comparison was not observed." RESET "\n\n");
    return 1;
}
