#include <sys/mman.h>
#include <sys/random.h>
#include <stdint.h>
#include <errno.h>
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

