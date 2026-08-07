#include "heap_internal.h"

#include <assert.h>
#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>

typedef void (*double_free_case)(void);

static void double_free_small_allocation(void)
{
    void *ptr = heap_alloc(32);
    assert(ptr != NULL);

    heap_free(ptr);
    heap_free(ptr);
}

static void double_free_large_allocation(void)
{
    void *ptr = heap_alloc(5000);
    assert(ptr != NULL);

    heap_free(ptr);
    heap_free(ptr);
}

static void assert_double_free_exits(const char *name, double_free_case test_case)
{
    pid_t child = fork();
    assert(child >= 0);

    if (child == 0) {
        test_case();

        /* Reaching here means the allocator accepted the double free. */
        _exit(0);
    }

    int status;
    assert(waitpid(child, &status, 0) == child);
    assert(WIFEXITED(status));
    assert(WEXITSTATUS(status) == 127);

    printf("[OK] %s double free exited with status 127\n", name);
}

int main(void)
{
    assert_double_free_exits("small allocation", double_free_small_allocation);
    assert_double_free_exits("large allocation", double_free_large_allocation);

    puts("All double-free tests passed.");
    return 0;
}
