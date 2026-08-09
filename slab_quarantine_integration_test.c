#include "heap_internal.h"
#include "slab_quarantine.h"

#include <assert.h>
#include <stdio.h>

#define TEST_ALLOCATION_SIZE 64

int main(void)
{
    void *allocations[QUEUE_CAPACITY + 1];
    void *replacement;
    void *target;

    /* Keep enough same-size allocations live to drive the quarantine. */
    for (size_t i = 0; i < QUEUE_CAPACITY + 1; i++) {
        allocations[i] = heap_alloc(TEST_ALLOCATION_SIZE);
        assert(allocations[i] != NULL);
    }

    target = allocations[0];
    heap_free(target);

    /* The target must remain unavailable for the next 29 frees. */
    for (size_t i = 1; i < QUEUE_CAPACITY; i++) {
        heap_free(allocations[i]);
        replacement = heap_alloc(TEST_ALLOCATION_SIZE);
        assert(replacement != NULL);
        assert(replacement != target);
    }

    /* Crossing the 30-free threshold makes the oldest slot reusable. */
    heap_free(allocations[QUEUE_CAPACITY]);
    replacement = heap_alloc(TEST_ALLOCATION_SIZE);
    assert(replacement == target);

    puts("[OK] a freed slab slot was quarantined for at least 30 frees");
    return 0;
}
