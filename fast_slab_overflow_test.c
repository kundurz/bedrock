#include "heap_internal.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ALLOCATION_COUNT 8192
#define VERIFY_INTERVAL 128
#define MAX_FAST_SIZE 2048

struct allocation {
    void *ptr;
    size_t size;
    unsigned char pattern;
};

static size_t random_fast_size(void)
{
    return 1u + (size_t)(rand() % MAX_FAST_SIZE);
}

static unsigned char pattern_for(size_t index, size_t size)
{
    return (unsigned char)((index * 131u + size * 17u + 0x5au) & 0xffu);
}

static void fill_allocation(const struct allocation *alloc)
{
    memset(alloc->ptr, alloc->pattern, alloc->size);
}

static void verify_allocation(const struct allocation *alloc)
{
    unsigned char *p = alloc->ptr;

    for (size_t i = 0; i < alloc->size; i++) {
        assert(p[i] == alloc->pattern);
    }
}

static void verify_all_allocations(const struct allocation *allocs, size_t count)
{
    for (size_t i = 0; i < count; i++) {
        verify_allocation(&allocs[i]);
    }
}

int main(void)
{
    struct allocation allocs[ALLOCATION_COUNT];

    srand(0x51ab500u);

    for (size_t i = 0; i < ALLOCATION_COUNT; i++) {
        allocs[i].size = random_fast_size();
        allocs[i].ptr = heap_alloc(allocs[i].size);
        allocs[i].pattern = pattern_for(i, allocs[i].size);

        assert(allocs[i].ptr != NULL);
        fill_allocation(&allocs[i]);

        if ((i % VERIFY_INTERVAL) == 0) {
            verify_all_allocations(allocs, i + 1);
        }
    }

    verify_all_allocations(allocs, ALLOCATION_COUNT);

    puts("[OK] fast slab heap_alloc overflow stress test passed");
    return 0;
}
