#pragma once

#include <stdbool.h>
#include "secure_utils.h"

#define NUM_CACHE_BLOCKS 32

/* STRUCTS */
struct cache_entry {
    struct guarded_region region;
    bool valid;
};

/* FUNCTIONS */
int initialize_ring_cache(); 
void insert_cache_entry(struct guarded_region region); 
struct guarded_region get_best_fit_entry(size_t payload_size); 

