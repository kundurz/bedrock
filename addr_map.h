#pragma once

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include "heap_internal.h"
#include "secure_utils.h"

enum map_type {
    LARGE = 0, 
    SMALL
};

enum alloc_type {
    ALLOC_TYPE_FREE = 0, 
    ALLOC_TYPE_SLAB, 
    ALLOC_TYPE_LARGE
};

/* HASH MAP ENTRY AND STATE STRUCTS */
struct large_meta {
    void *mmap_base;
    size_t total_size;
    size_t payload_size;
};

struct map_value {
    enum alloc_type type;
    union {
        struct slab slab;
        struct large_meta large;
    };
};

struct map_entry {
    uint64_t key;
    struct map_value value;

    uint8_t dib; 
    bool is_occupied;
};

struct hash_map_state {
    struct guarded_region guard_region;
    struct map_entry* base;

    size_t size; // size in bytes
    uint64_t capacity;
    uint64_t occupied_slots;

    uint64_t random_salt;
};

/* CORE INTERFACES */
int initialize_hash_map(); 
int addr_map_insert(enum map_type self, uintptr_t addr_key, struct map_value metadata_value); 
struct map_entry* addr_map_lookup(enum map_type self, uintptr_t addr_key); 
struct map_value construct_map_value(struct slab* slab_metadata, struct large_meta* large_metadata); 

/* INTERFACES FOR TESTING */
void print_map_state(enum map_type self); 
void addr_map_enumerate(enum map_type self); 

