#include <stddef.h> 
#include "heap_stats.h"

#ifdef HEAP_ENABLE_STATS

static size_t mapped_slab_bytes;
static size_t mapped_large_bytes;
static size_t mapped_metadata_bytes; 

void heap_stats_add_slab_mapping(size_t bytes) {
    mapped_slab_bytes += bytes;
}

void heap_stats_remove_slab_mapping(size_t bytes) {
    mapped_slab_bytes -= bytes;
}

void heap_stats_add_large_mapping(size_t bytes) {
   mapped_large_bytes += bytes; 
}

void heap_stats_remove_large_mapping(size_t bytes) {
    mapped_large_bytes -= bytes;
}

void heap_stats_add_metadata_mapping(size_t bytes) {
    mapped_metadata_bytes += bytes;
}

void heap_stats_remove_metadata_mapping(size_t bytes) {
    mapped_metadata_bytes -= bytes;
}

size_t heap_stats_mapped_slab_bytes(void) {
    return mapped_slab_bytes;
}

size_t heap_stats_mapped_large_bytes(void) {
    return mapped_large_bytes;
}

size_t heap_stats_mapped_metadata_bytes(void) {
    return mapped_metadata_bytes;
}

#endif