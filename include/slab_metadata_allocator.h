#pragma once

#include <stdint.h>

#include "heap_internal.h"
#include "secure_utils.h"

// This is good. It's small enough to fit within a cache line.
struct slab_metadata_arena  {
    struct guarded_region region;

    struct slab_metadata_arena* next; 
    struct slab_metadata_arena* prev;

    uint16_t bitmap;
};

struct slab_metadata_slot {
    struct slab_metadata_arena* owner;
    struct slab slab;
};

struct slab_metadata_slot* insert_slab_metadata(struct slab* slab_metadata); 
void delete_slab_metadata(struct slab_metadata_slot* metadata_ptr); 
