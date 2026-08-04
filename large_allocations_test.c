#include "addr_map.h"
#include "large_allocations.h"
#include "ring_cache.h"

#include <assert.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

static void assert_write_faults(volatile unsigned char *address)
{
    pid_t child = fork();
    assert(child >= 0);

    if (child == 0) {
        *address = 0xa5;
        _exit(0);
    }

    int status;
    assert(waitpid(child, &status, 0) == child);
    assert(WIFSIGNALED(status));
    assert(WTERMSIG(status) == SIGSEGV || WTERMSIG(status) == SIGBUS);
}

static void test_first_allocation_is_guarded_and_usable(void)
{
    const size_t size = 5000;
    unsigned char *ptr = hardened_large_malloc(size);
    assert(ptr != NULL);

    struct map_entry *entry = addr_map_lookup(LARGE, (uintptr_t)ptr);
    assert(entry != NULL);
    assert(entry->value.large.payload_size == size);

    long page_size = sysconf(_SC_PAGESIZE);
    assert(page_size > 0);
    assert(ptr == (unsigned char *)entry->value.large.mmap_base + page_size);
    assert(entry->value.large.total_size >= size + (2 * (size_t)page_size));

    memset(ptr, 0x5a, size);
    for (size_t i = 0; i < size; i++)
        assert(ptr[i] == 0x5a);

    unsigned char *base = entry->value.large.mmap_base;
    assert_write_faults(base);
    assert_write_faults(base + entry->value.large.total_size - 1);

    puts("[OK] first large allocation has guard pages and a usable region");
}

static void test_allocations_are_unique_writable_and_isolated(void)
{
    enum { ALLOCATION_COUNT = 96 };
    unsigned char *ptrs[ALLOCATION_COUNT];
    size_t sizes[ALLOCATION_COUNT];
    unsigned char patterns[ALLOCATION_COUNT];

    for (size_t i = 0; i < ALLOCATION_COUNT; i++) {
        /* Deliberately cross page boundaries and use different chunk sizes. */
        sizes[i] = 4097 + ((i * 977) % 12000);
        patterns[i] = (unsigned char)(i + 1);
        ptrs[i] = hardened_large_malloc(sizes[i]);
        assert(ptrs[i] != NULL);

        for (size_t j = 0; j < i; j++)
            assert(ptrs[i] != ptrs[j]);

        memset(ptrs[i], patterns[i], sizes[i]);

        /* A new allocation must not overwrite any earlier allocation. */
        for (size_t j = 0; j <= i; j++) {
            for (size_t offset = 0; offset < sizes[j]; offset++)
                assert(ptrs[j][offset] == patterns[j]);
        }
    }

    /* Writing every chunk again must not affect any differently-sized peer. */
    for (size_t i = 0; i < ALLOCATION_COUNT; i++)
        memset(ptrs[i], (unsigned char)(0xff - i), sizes[i]);

    for (size_t i = 0; i < ALLOCATION_COUNT; i++) {
        for (size_t offset = 0; offset < sizes[i]; offset++)
            assert(ptrs[i][offset] == (unsigned char)(0xff - i));
    }

    puts("[OK] large allocations are unique, readable, writable, and isolated");
}

int main(void)
{
    assert(initialize_hash_map(LARGE) == 0);
    initialize_ring_cache();

    test_first_allocation_is_guarded_and_usable();
    test_allocations_are_unique_writable_and_isolated();

    puts("All large allocation tests passed.");
    return 0;
}
