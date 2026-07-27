#include "heap_internal.h"
#include "utils.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ALLOCATION_COUNT 8192
#define VERIFY_INTERVAL 128
#define MAX_FAST_SIZE 2048
#define TEST_PAGE_SIZE 4096

struct allocation {
    void *ptr;
    size_t requested_size;
    size_t usable_size;
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

static void verify_slot_bounds(const struct allocation *alloc)
{
    uintptr_t address = (uintptr_t)alloc->ptr;
    size_t page_offset = address & (TEST_PAGE_SIZE - 1);

    assert(alloc->usable_size >= alloc->requested_size);
    assert((page_offset % alloc->usable_size) == 0);
    assert(page_offset + alloc->usable_size <= TEST_PAGE_SIZE);
}

static void fill_allocation(const struct allocation *alloc)
{
    memset(alloc->ptr, alloc->pattern, alloc->usable_size);
}

static void verify_allocation(const struct allocation *alloc)
{
    unsigned char *p = alloc->ptr;

    verify_slot_bounds(alloc);

    for (size_t i = 0; i < alloc->usable_size; i++) {
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
        allocs[i].requested_size = random_fast_size();
        allocs[i].usable_size =
            (size_t)determine_size_class((int)allocs[i].requested_size);
        allocs[i].ptr = heap_alloc(allocs[i].requested_size);
        allocs[i].pattern =
            pattern_for(i, allocs[i].requested_size);

        assert(allocs[i].ptr != NULL);
        verify_slot_bounds(&allocs[i]);
        fill_allocation(&allocs[i]);

        if ((i % VERIFY_INTERVAL) == 0) {
            verify_all_allocations(allocs, i + 1);
        }
    }

    verify_all_allocations(allocs, ALLOCATION_COUNT);

    puts("[OK] fast slab heap_alloc overflow stress test passed");
    return 0;
}
