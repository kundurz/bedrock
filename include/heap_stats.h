#pragma once

#include <stddef.h>

#ifdef HEAP_ENABLE_STATS
void heap_stats_add_large_mapping(size_t bytes);
void heap_stats_remove_large_mapping(size_t bytes);

void heap_stats_add_slab_mapping(size_t bytes);
void heap_stats_remove_slab_mapping(size_t bytes);

void heap_stats_add_metadata_mapping(size_t bytes);
void heap_stats_remove_metadata_mapping(size_t bytes);

size_t heap_stats_mapped_slab_bytes(void);
size_t heap_stats_mapped_large_bytes(void);
size_t heap_stats_mapped_metadata_bytes(void);


#else

static inline void heap_stats_add_large_mapping(size_t bytes) {
    (void)bytes;
}

static inline void heap_stats_remove_large_mapping(size_t bytes) {
    (void)bytes;
}

static inline void heap_stats_add_metadata_mapping(size_t bytes) {
    (void)bytes;
}

static inline void heap_stats_remove_metadata_mapping(size_t bytes) {
    (void)bytes;
}

static inline void heap_stats_add_slab_mapping(size_t bytes) {
    (void)bytes;
}

static inline void heap_stats_remove_slab_mapping(size_t bytes) {
    (void)bytes;
}

static inline size_t heap_stats_mapped_slab_bytes(void) {
    return 0;
}

static inline size_t heap_stats_mapped_large_bytes(void) {
    return 0;
}

static inline size_t heap_stats_mapped_metadata_bytes(void) {
    return 0;
}

#endif

