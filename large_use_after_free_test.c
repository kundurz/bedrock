#include "heap_internal.h"

#include <assert.h>
#include <signal.h>
#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>

static void attempt_write_after_large_free(void)
{
    volatile unsigned char *ptr = heap_alloc(5000);
    assert(ptr != NULL);

    ptr[0] = 0x5a;
    heap_free((void *)ptr);

    /* The cached payload pages should now have PROT_NONE. */
    ptr[0] = 0xa5;
}

int main(void)
{
    pid_t child = fork();
    assert(child >= 0);

    if (child == 0) {
        attempt_write_after_large_free();

        /* A normal return means stale writes were incorrectly permitted. */
        _exit(0);
    }

    int status;
    assert(waitpid(child, &status, 0) == child);
    assert(WIFSIGNALED(status));
    assert(WTERMSIG(status) == SIGSEGV || WTERMSIG(status) == SIGBUS);

    puts("[OK] writing to a cached large allocation terminated the process");
    return 0;
}
