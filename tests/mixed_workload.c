#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "heap.h"

#define WORKLOAD_ROUNDS 25
#define ALLOCATIONS_PER_ROUND 4096
#define SMALL_ALLOCATION_LIMIT 2048
#define MAX_LARGE_ALLOCATION (1024 * 1024)

struct allocation {
    unsigned char *pointer;
    size_t size;
    unsigned char pattern;
};

static uint64_t random_state = UINT64_C(0x4d595df4d0f33173);

static uint64_t next_random(void)
{
    uint64_t value = random_state;

    value ^= value >> 12;
    value ^= value << 25;
    value ^= value >> 27;

    random_state = value;
    return value * UINT64_C(0x2545f4914f6cdd1d);
}

static size_t random_allocation_size(void)
{
    uint64_t value = next_random();

    if (value % 4 != 0)
        return value % (SMALL_ALLOCATION_LIMIT + 1);

    return SMALL_ALLOCATION_LIMIT + 1 +
        value % (MAX_LARGE_ALLOCATION - SMALL_ALLOCATION_LIMIT);
}

static void shuffle_allocations(
    struct allocation allocations[], size_t allocation_count
)
{
    for (size_t i = allocation_count - 1; i > 0; i--) {
        size_t other = next_random() % (i + 1);
        struct allocation temporary = allocations[i];

        allocations[i] = allocations[other];
        allocations[other] = temporary;
    }
}

static void verify_allocation(const struct allocation *allocation)
{
    for (size_t i = 0; i < allocation->size; i++)
        assert(allocation->pointer[i] == allocation->pattern);
}

int main(void)
{
    struct allocation allocations[ALLOCATIONS_PER_ROUND];

    for (size_t round = 0; round < WORKLOAD_ROUNDS; round++) {
        for (size_t i = 0; i < ALLOCATIONS_PER_ROUND; i++) {
            size_t size = random_allocation_size();
            unsigned char pattern = (unsigned char)(next_random() | 1);
            unsigned char *pointer = heap_alloc(size);

            assert(pointer != NULL);

            allocations[i].pointer = pointer;
            allocations[i].size = size;
            allocations[i].pattern = pattern;

            memset(pointer, pattern, size);
        }

        for (size_t i = 0; i < ALLOCATIONS_PER_ROUND; i++)
            verify_allocation(&allocations[i]);

        shuffle_allocations(allocations, ALLOCATIONS_PER_ROUND);

        for (size_t i = 0; i < ALLOCATIONS_PER_ROUND; i++) {
            verify_allocation(&allocations[i]);
            heap_free(allocations[i].pointer);
        }
    }

    printf(
        "[OK] Mixed workload stress test passed: "
        "%d rounds, %d allocations per round.\n",
        WORKLOAD_ROUNDS,
        ALLOCATIONS_PER_ROUND
    );

    return 0;
}
