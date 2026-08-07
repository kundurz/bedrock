#include <unistd.h>
#include <stdlib.h>
#include "addr_map.h"
#include "large_allocations.h"
#include "secure_utils.h"
#include "ring_cache.h"

void* hardened_large_alloc(size_t payload_size) {
    struct guarded_region region;

    // First we're gonna look for a ring cache
    region = get_best_fit_entry(payload_size); 
    if (region.usable_ptr == NULL) 
        region = create_gaurded_region(payload_size); 
    else
        unlock_page(region.usable_ptr, region.total_size - 2 * sysconf(_SC_PAGESIZE));

    struct large_meta large_metadata;

    large_metadata.mmap_base = region.mmap_base;
    large_metadata.payload_size = payload_size;
    large_metadata.total_size = region.total_size;

    struct map_value map_value = construct_map_value(NULL, &large_metadata);
    addr_map_insert(LARGE, region.usable_ptr, map_value);

    return region.usable_ptr;
}

void hardened_large_free(void* ptr) {

    struct map_entry* map_entry = addr_map_lookup(LARGE, (uintptr_t)ptr);

    if (map_entry == NULL)
        return;

    struct guarded_region region;
    region.mmap_base = map_entry->value.large.mmap_base;
    region.total_size = map_entry->value.large.total_size;
    region.usable_ptr = ptr;

    delete_entry(LARGE, (uintptr_t)ptr);

    lock_page(ptr, region.total_size - 2 * sysconf(_SC_PAGESIZE));
    insert_cache_entry(region); // This will handle it being destroyed later. 
}