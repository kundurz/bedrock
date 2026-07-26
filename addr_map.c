#include <sys/mman.h>
#include <sys/random.h>
#include <stdint.h>
#include <errno.h>
#include <stdio.h>
#include <stdbool.h>
#include "addr_map.h"
#include "utils.h"

#define GOLDEN_RATIO_64 0x9e3779b97f4a7c15ULL // Mathematical constant used to achieve highly uniform data distribution.

static struct hash_map_state map_state; 

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

int initialize_hash_map() {
    if (generate_salt(&map_state.random_salt) != 0)
        return -1;

    map_state.base = mmap(
        NULL, 
        4096,
        PROT_READ | PROT_WRITE,
        MAP_PRIVATE | MAP_ANON,
        -1, 
        0
    );

    if (map_state.base == MAP_FAILED)
        return -1;

    map_state.size = 4096;
    map_state.capacity = round_down_power_of_two(map_state.size / sizeof(struct map_entry));
    map_state.occupied_slots = 0;

    return 0;
}

static int _addr_map_access_index_key(int index) 
{
    struct map_entry* relevant_entry = (struct map_entry*)map_state.base + index; 

    return relevant_entry->key;
}

// We need to have a "targetKey" and a "targetValue" variable.
// The targetkey variable can change based on an insertion, but
// we need to keep track of the inserted index that we inserted the very first thing.
static struct map_entry* robin_hood_resolution(uintptr_t addr_key, struct slab metadata_value) {
    int home_entry_index = hash_address(addr_key, map_state.random_salt) % map_state.capacity;
    
    uintptr_t targetKey = addr_key;
    struct slab targetValue = metadata_value; 
    uint8_t targetDib = 0;
    struct map_entry* inserted_entry = NULL;

    // use modulo to bound everything later.
    for (int i = home_entry_index; true ; i++) {
        struct map_entry* curr = (struct map_entry*)map_state.base + (i % map_state.capacity); 
        if (curr->is_occupied) {
            if (targetDib > curr->dib) {
                if (inserted_entry == NULL) inserted_entry = curr;

                generic_swap(&targetKey, &curr->key, sizeof(uintptr_t));
                generic_swap(&targetValue, &curr->value, sizeof(struct slab));
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

/* This is returning an integer because if something goes wrong, i want to return an error. */
int addr_map_insert(uintptr_t addr_key, struct slab metadata_value) {
    int map_entry_index = hash_address(addr_key, map_state.random_salt) % map_state.capacity;

    struct map_entry* relevant_entry = robin_hood_resolution(addr_key, metadata_value);

    return relevant_entry != NULL;
}

void addr_map_enumerate() {
    for (int i = 0; i < map_state.capacity; i++) {
        struct map_entry* curr = map_state.base + i;
        printf("| key = %lx |\n", curr->key);
    }
}

