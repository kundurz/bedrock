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

static int pointer_is_one_of(void *ptr, void *const *expected, size_t count)
{
    for (size_t i = 0; i < count; i++) {
        if (ptr == expected[i])
            return 1;
    }

    return 0;
}

static void test_fast_free_reuses_full_slab(void)
{
    enum {
        SIZE_CLASS = 2048,
        SLOTS_PER_SLAB = TEST_PAGE_SIZE / SIZE_CLASS
    };

    void *original[SLOTS_PER_SLAB];
    void *reused[SLOTS_PER_SLAB];

    for (size_t i = 0; i < SLOTS_PER_SLAB; i++) {
        original[i] = heap_alloc(SIZE_CLASS);
        assert(original[i] != NULL);
        memset(original[i], 0xa5, SIZE_CLASS);
    }

    for (size_t i = 0; i < SLOTS_PER_SLAB; i++) {
        heap_free(original[i]);
    }

    for (size_t i = 0; i < SLOTS_PER_SLAB; i++) {
        reused[i] = heap_alloc(SIZE_CLASS);
        assert(reused[i] != NULL);
        assert(pointer_is_one_of(reused[i], original, SLOTS_PER_SLAB));

        for (size_t j = 0; j < i; j++) {
            assert(reused[i] != reused[j]);
        }
    }

    puts("[OK] fast frees make a full slab reusable");
}

int main(void)
{
    struct allocation allocs[ALLOCATION_COUNT];

    srand(0x51ab500u);
    test_fast_free_reuses_full_slab();

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

    for (size_t i = ALLOCATION_COUNT; i > 0; i--) {
        heap_free(allocs[i - 1].ptr);
    }

    puts("[OK] fast slab heap_alloc overflow stress test passed");
    puts("[OK] all fast stress allocations freed");
    return 0;
}
