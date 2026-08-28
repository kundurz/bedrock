#include <unistd.h>
#include <stdlib.h>
#include "addr_map.h"
#include "large_allocations.h"
#include "secure_utils.h"
#include "ring_cache.h"
#include "heap_stats.h"

void* hardened_large_alloc(size_t payload_size) {
    long page_size = sysconf(_SC_PAGESIZE);
    if (page_size == -1)
        _exit(127);

    struct guarded_region region;

    // First we're gonna look for a ring cache
    region = get_best_fit_entry(payload_size); 
    if (region.usable_ptr == NULL) {
        region = create_gaurded_region(payload_size, true); 
        heap_stats_add_large_mapping(region.total_size);
    } else {
        unlock_page(region.usable_ptr, region.total_size - 2 * page_size);
    }

    struct large_meta large_metadata;

    large_metadata.mmap_base = region.mmap_base;
    large_metadata.payload_size = payload_size;
    large_metadata.total_size = region.total_size;
    large_metadata.offset = region.offset;

    struct map_value map_value = construct_map_value(NULL, &large_metadata);
    if (!addr_map_insert(LARGE, (char*)region.usable_ptr + region.offset, map_value)) {
        return NULL;
    }

    return (void*)((char*)region.usable_ptr + region.offset);
}

void hardened_large_free(void* ptr) {
    long page_size = sysconf(_SC_PAGESIZE); 
    if (page_size == -1)
        return;

    struct map_entry* map_entry = addr_map_lookup(LARGE, (uintptr_t)ptr);

    if (map_entry == NULL)
        return;

    struct guarded_region region;
    region.mmap_base = map_entry->value.large.mmap_base;
    region.total_size = map_entry->value.large.total_size;
    region.usable_ptr = (char*)region.mmap_base + page_size; 
    region.offset = map_entry->value.large.offset;

    if (delete_entry(LARGE, (uintptr_t)ptr) == -1)
        return;

    lock_page((char*)ptr - region.offset, region.total_size - 2 * page_size);
    insert_cache_entry(region); // This will handle it being destroyed later. 
}