#include "addr_map.h"
#include "heap_internal.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define ALLOCATION_COUNT 16

static void verify_offset_allocation(unsigned char *ptr, size_t payload_size)
{
    struct map_entry *entry = addr_map_lookup(LARGE, (uintptr_t)ptr);
    assert(entry != NULL);

    long page_size = sysconf(_SC_PAGESIZE);
    assert(page_size > 0);

    uintptr_t mapping_base = (uintptr_t)entry->value.large.mmap_base;
    uintptr_t usable_base = mapping_base + (uintptr_t)page_size;
    uintptr_t tail_guard =
        mapping_base + entry->value.large.total_size - (uintptr_t)page_size;

    assert((uintptr_t)ptr == usable_base + entry->value.large.offset);
    assert((tail_guard % (uintptr_t)page_size) == 0);
    assert((uintptr_t)ptr + payload_size <= tail_guard);
    assert(entry->value.large.payload_size == payload_size);

    memset(ptr, 0xa5, payload_size);
    assert(ptr[0] == 0xa5);
    assert(ptr[payload_size - 1] == 0xa5);
}

static void test_cache_fit_accounts_for_offset(void)
{
    long page_size = sysconf(_SC_PAGESIZE);
    assert(page_size > 0);

    unsigned char *ptr = heap_alloc(5000);
    assert(ptr != NULL);

    struct map_entry *entry = addr_map_lookup(LARGE, (uintptr_t)ptr);
    assert(entry != NULL);
    assert(entry->value.large.offset > 0);

    uintptr_t old_mapping_base = (uintptr_t)entry->value.large.mmap_base;
    size_t usable_size =
        entry->value.large.total_size - 2 * (size_t)page_size;
    size_t capacity_after_offset =
        usable_size - entry->value.large.offset;

    heap_free(ptr);

    /* This fits the mapping as a whole, but not after its old offset. */
    size_t larger_size = capacity_after_offset + 1;
    assert(larger_size <= usable_size);

    unsigned char *larger = heap_alloc(larger_size);
    assert(larger != NULL);

    entry = addr_map_lookup(LARGE, (uintptr_t)larger);
    assert(entry != NULL);
    assert((uintptr_t)entry->value.large.mmap_base != old_mapping_base);
    verify_offset_allocation(larger, larger_size);
    heap_free(larger);
}

int main(void)
{
    unsigned char *allocations[ALLOCATION_COUNT];
    size_t sizes[ALLOCATION_COUNT];
    int saw_nonzero_offset = 0;

    test_cache_fit_accounts_for_offset();

    for (size_t i = 0; i < ALLOCATION_COUNT; i++) {
        sizes[i] = 4097 + i * 257;
        allocations[i] = heap_alloc(sizes[i]);
        assert(allocations[i] != NULL);

        verify_offset_allocation(allocations[i], sizes[i]);

        struct map_entry *entry =
            addr_map_lookup(LARGE, (uintptr_t)allocations[i]);
        assert(entry != NULL);
        saw_nonzero_offset |= entry->value.large.offset != 0;
    }

    assert(saw_nonzero_offset);

    for (size_t i = 0; i < ALLOCATION_COUNT; i++)
        heap_free(allocations[i]);

    /* Exercise the cached-region path with offsets preserved. */
    for (size_t i = 0; i < ALLOCATION_COUNT; i++) {
        unsigned char *ptr = heap_alloc(sizes[i]);
        assert(ptr != NULL);
        verify_offset_allocation(ptr, sizes[i]);
        heap_free(ptr);
    }

    puts("[OK] large allocations preserve byte offsets and usable bounds");
    puts("[OK] offset large allocations remain valid after cache reuse");
    return 0;
}
