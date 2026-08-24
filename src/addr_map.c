#include <sys/mman.h>
#include <sys/random.h>
#include <stdint.h>
#include <errno.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include "addr_map.h"
#include "utils.h"
#include "secure_utils.h"

#define GOLDEN_RATIO_64 0x9e3779b97f4a7c15ULL // Mathematical constant used to achieve highly uniform data distribution.

static struct hash_map_state slab_map; 
static struct hash_map_state large_map;

static int generate_salt(uint64_t *salt) {
    unsigned char* output = (unsigned char *)salt;
    size_t remaining = sizeof(*salt);

    while (remaining > 0) {
        ssize_t received = getrandom(output, remaining, 0);

        if (received < 0) {
            if (errno == EINTR)
                continue;
            return -1;
        }

        output += received;
        remaining -= (size_t)received;
    }

    return 0;
}

static uint64_t hash_address(uintptr_t addr, uint64_t salt)
{
    // Allocations are at least 16-byte aligned.
    uint64_t x = ((uint64_t)addr >> 4) + salt;

    x = (x ^ (x >> 30)) * UINT64_C(0xbf58476d1ce4e5b9);
    x = (x ^ (x >> 27)) * UINT64_C(0x94d049bb133111eb);
    return x ^ (x >> 31);
}

int initialize_hash_map(enum map_type self) {
    long page_size = sysconf(_SC_PAGESIZE);

    if (page_size == -1)
        _exit(127); 

    struct hash_map_state* map_state;

    if (self == LARGE) {
        map_state = &large_map;
    } else if (self == SMALL) {
        map_state = &slab_map;
    } else {
        return -1;
    }

    if (generate_salt(&(map_state->random_salt)) != 0)
        return -1;

    if (self == SMALL) map_state->guard_region = create_gaurded_region(page_size, false);
    else map_state->guard_region = create_gaurded_region(round_to_nearest_page(sizeof(struct large_meta) * 10), false);
    map_state->base = map_state->guard_region.usable_ptr;

    if (map_state->base == MAP_FAILED)
        return -1;

    map_state->size = page_size;
    map_state->capacity = round_down_power_of_two(map_state->size / sizeof(struct map_entry));
    map_state->occupied_slots = 0;

    return 0;
}

int map_select(enum map_type self, struct hash_map_state** map_state) {
    if (self == LARGE) {
        *map_state = &large_map;
    } else if (self == SMALL) {
        *map_state = &slab_map;
    } else {
        return -1;
    }

    return 0; // Success
}

struct map_value construct_map_value(struct slab* slab_metadata, struct large_meta* large_metadata) {
    struct map_value value; 
    if (slab_metadata != NULL) {
        value.type = ALLOC_TYPE_SLAB;
        value.slab = *slab_metadata;
    } else if (large_metadata != NULL) {
        value.type = ALLOC_TYPE_LARGE;
        value.large = *large_metadata;
    } 
    return value;
}

static int _addr_map_access_index_key(enum map_type self, int index) 
{
    struct hash_map_state* map_state;

    int success = map_select(self, &map_state); 

    if (success == -1)
        return -1;

    struct map_entry* relevant_entry = (struct map_entry*)map_state->base + index; 

    return relevant_entry->key;
}

// We need to have a "targetKey" and a "targetValue" variable.
// The targetkey variable can change based on an insertion, but
// we need to keep track of the inserted index that we inserted the very first thing.
static struct map_entry* robin_hood_resolution(enum map_type self, uintptr_t addr_key, struct map_value metadata_value) {
    struct hash_map_state* map_state;

    int success = map_select(self, &map_state); 

    if (success == -1)
        return NULL;

    int home_entry_index = hash_address(addr_key, map_state->random_salt) % map_state->capacity;
    
    uintptr_t targetKey = addr_key;
    struct map_value targetValue = metadata_value; 
    uint16_t targetDib = 0;
    struct map_entry* inserted_entry = NULL;

    // use modulo to bound everything later.
    // I USE TRUE HERE SO THAT IT WRAPS AROUND, DO NOT GUARD IT1
    for (int i = home_entry_index; true ; i++) {
        struct map_entry* curr = (struct map_entry*)map_state->base + (i & (map_state->capacity - 1)); 
        if (curr->is_occupied) {
            if (targetDib > curr->dib) {
                if (inserted_entry == NULL) inserted_entry = curr;

                generic_swap(&targetKey, &curr->key, sizeof(uintptr_t));
                generic_swap(&targetValue, &curr->value, sizeof(struct map_value));
                generic_swap(&targetDib, &curr->dib, sizeof(uint8_t));
            }
        } else {
            if (inserted_entry == NULL) inserted_entry = curr;
            curr->key = targetKey;
            curr->value = targetValue;
            curr->dib = targetDib;
            curr->is_occupied = true;
            
            return inserted_entry; // The point where we find an empty slot is when the search is over.
        }

        targetDib++;
    }

    return NULL;
}

// When we resize a hash map, we almost have to create a new one.
// This should also take place releti
static void resize_map(enum map_type self) {
    struct hash_map_state* map_state;

    int success = map_select(self, &map_state); 

    if (success == -1)
        return;

    struct guarded_region guarded_region = map_state->guard_region;
    struct map_entry* old_base = map_state->base;
    uint64_t old_capacity = map_state->capacity;

    if (!add_size_t_safely(map_state->size, map_state->size, &(map_state->size)))
        _exit(127);

    if (!add_size_t_safely(map_state->capacity, map_state->capacity, &(map_state->capacity)))
        _exit(127);

    map_state->guard_region = create_gaurded_region(map_state->size, false);
    map_state->base = map_state->guard_region.usable_ptr;

    for (long unsigned int i = 0; i < old_capacity; i++) {
        struct map_entry* curr = old_base + i;
        if (curr->is_occupied)
            robin_hood_resolution(self, curr->key, curr->value);
    }

    destroy_guarded_region(&guarded_region); 
}

/* This is returning an integer because if something goes wrong, i want to return an error. */
int addr_map_insert(enum map_type self, uintptr_t addr_key, struct map_value metadata_value) {
    struct hash_map_state* map_state;

    int success = map_select(self, &map_state); 

    if (success == -1)
        return -1;

    double load_factor = (double)map_state->occupied_slots / map_state->capacity;

    if (load_factor > 0.7) {
        resize_map(self);
    }

    struct map_entry* relevant_entry = robin_hood_resolution(self, addr_key, metadata_value);
    
    if (relevant_entry != NULL) map_state->occupied_slots += 1;

    return relevant_entry != NULL;
}

// I think we can just search linearly form the starting position. 
struct map_entry* addr_map_lookup(enum map_type self, uintptr_t addr_key) {
    struct hash_map_state* map_state;

    int success = map_select(self, &map_state); 

    if (success == -1)
        return NULL;

    if ((void*)addr_key == NULL) 
        return NULL;

    int index = hash_address(addr_key, map_state->random_salt) % map_state->capacity;

    bool entry_is_occupied = true;
    uint64_t num_probes = 0;
    while (entry_is_occupied && num_probes < map_state->capacity) {
        struct map_entry* curr = (struct map_entry*)map_state->base + (index); 
        if (curr->is_occupied) {
            if (curr->key == addr_key)
                return curr;
        } else {
            break;
        }

        index = (index + 1 ) & (map_state->capacity - 1);
        num_probes++;

    }

    return NULL; // entry not found.
}

void addr_map_enumerate(enum map_type self) {
    struct hash_map_state* map_state;

    int success = map_select(self, &map_state); 

    if (success == -1)
        return;

    for (long unsigned int i = 0; i < map_state->capacity; i++) {
        struct map_entry* curr = map_state->base + i;
        printf("| key = %lx |\n", curr->key);
    }
}

void print_map_state(enum map_type self) {
    struct hash_map_state* map_state;

    int success = map_select(self, &map_state); 

    if (success == -1)
        return;

    printf("--- CURRENT MAP STATE ---\n");
    printf("MAP SIZE: %ld BYTES\n", map_state->size);
    printf("MAP CAPACITY: %ld ENTRIES\n", map_state->capacity);
}

// THIS UTILIZES THE WRAPAROUND 
static struct map_entry* _increment_map_address(enum map_type self, struct map_entry* ptr) {
    struct hash_map_state* map_state;

    int success = map_select(self, &map_state); 

    if (success == -1)
        return NULL;


    struct map_entry* next = ptr + 1;
    if (next >= map_state->base + map_state->capacity) {
            next = map_state->base;
    }

    return next;
} 

static int _backshift_delete(enum map_type self, uintptr_t addr_key) {
    struct hash_map_state* map_state;

    int success = map_select(self, &map_state); 

    if (success == -1)
        return -1;

    struct map_entry* entry = addr_map_lookup(self, addr_key);

    if (entry == NULL)
        return -1;

    entry->is_occupied = false;
    map_state->occupied_slots -= 1; 

    struct map_entry* curr = entry;
    struct map_entry* next = _increment_map_address(self, curr);
    
    while (next->is_occupied && next->dib > 0) {
        // wrap around
        if ((uintptr_t)next > (uintptr_t)((char*)map_state->base + map_state->size)) {
            next = map_state->base;
        }
        if ((uintptr_t)curr > (uintptr_t)((char*)map_state->base + map_state->size)) {
            curr = map_state->base; 
        }

        *curr = *next;
        curr->dib--;
        memset(next, 0, sizeof(struct map_entry));
    

        curr = _increment_map_address(self, curr);
        next = _increment_map_address(self, next); 
    }

    return 0;
}

int delete_entry(enum map_type self, uintptr_t addr_key) {
    return _backshift_delete(self, addr_key);
}
