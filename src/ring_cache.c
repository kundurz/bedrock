#include <sys/mman.h>
#include <stdlib.h>
#include <unistd.h>
#include "ring_cache.h"
#include "addr_map.h"
#include "secure_utils.h"
#include "heap_stats.h"

struct cache_entry* cache_base;
static size_t next_entry_index; 

static struct cache_entry _construct_cache_entry(struct guarded_region region) {
    struct cache_entry entry;
    entry.region = region;
    entry.valid  = 1;

    return entry;
}

static int _search_for_non_occupied_entry() {
    for (int i = 0; i < NUM_CACHE_BLOCKS;  i++) {
        if (!cache_base[i].valid)
            return i;
    }

    return -1;
}

int initialize_ring_cache() {
    struct guarded_region region = create_gaurded_region(NUM_CACHE_BLOCKS * sizeof(struct cache_entry), false);
    heap_stats_add_metadata_mapping(region.total_size);

    if (region.mmap_base == MAP_FAILED)
        return -1;

    cache_base = region.usable_ptr;
    next_entry_index = 0;

    return 0;
}

void insert_cache_entry(struct guarded_region region) {
    // Construct cache entry
    struct cache_entry entry = _construct_cache_entry(region);
    struct cache_entry* next_entry_start = cache_base + next_entry_index;

    if (!next_entry_start->valid) {
        *next_entry_start = entry;
        next_entry_start->valid = 1;
    } else {
        int index = _search_for_non_occupied_entry();
        if (index == -1) {
            heap_stats_remove_large_mapping(next_entry_start->region.total_size);
            destroy_guarded_region(&(next_entry_start->region));
            *next_entry_start = entry;
            next_entry_start->valid = 1;
        } else {
            *(cache_base + index) = entry;
            (*(cache_base + index)).valid = 1;
        }
    }

    next_entry_index = (next_entry_index + 1) % NUM_CACHE_BLOCKS;
}

struct guarded_region get_best_fit_entry(size_t payload_size) {
    struct cache_entry* best_fit_entry = NULL;

    size_t best_fit_size = -1;
    for (int i = 0; i < NUM_CACHE_BLOCKS; i++) {
        if (!cache_base[i].valid) 
            continue;
        size_t usable_size = cache_base[i].region.total_size - 2 * sysconf(_SC_PAGESIZE) - cache_base[i].region.offset; 
        if (usable_size >= payload_size) {
            if (best_fit_size == -1 || usable_size < best_fit_size) {
                best_fit_size = usable_size;
                best_fit_entry = &cache_base[i];
            } 
        }
    }

    if (best_fit_entry == NULL) 
        return (struct guarded_region){.mmap_base = NULL, .usable_ptr = NULL, .total_size = 0};
    
    best_fit_entry->valid = 0;

    return best_fit_entry->region; 
}
