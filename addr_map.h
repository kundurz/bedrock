#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include "heap_internal.h"

/* HASH MAP ENTRY AND STATE STRUCTS */
struct map_entry {
    uint64_t key;
    struct slab value;

    uint8_t dib; 
    bool is_occupied;
};

struct hash_map_state {
    struct map_entry* base;

    size_t size; // size in bytes
    uint64_t capacity;
    uint64_t occupied_slots;

    uint64_t random_salt;
};

/* FUNCTIONS */
int initialize_hash_map(); 
int addr_map_insert(uintptr_t addr_key, struct slab metadata_value); 
void addr_map_enumerate(); 

