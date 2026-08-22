#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

#include "heap.h"

#define LARGE_CACHE_CAPACITY 32
#define QUARANTINE_CAPACITY 30
#define LARGE_ALLOCATION_SIZE 4096
#define SMALL_ALLOCATION_SIZE 32

static bool pointer_is_in_array(void *pointer, void *pointers[], size_t length)
{
    for (size_t i = 0; i < length; i++) {
        if (pointers[i] == pointer)
            return true;
    }

    return false;
}

static void test_large_cache_reuse(void)
{
    void *cached[LARGE_CACHE_CAPACITY];
    void *reused[LARGE_CACHE_CAPACITY];

    for (size_t i = 0; i < LARGE_CACHE_CAPACITY; i++) {
        cached[i] = heap_alloc(LARGE_ALLOCATION_SIZE);
        assert(cached[i] != NULL);
    }

    for (size_t i = 0; i < LARGE_CACHE_CAPACITY; i++)
        heap_free(cached[i]);

    for (size_t i = 0; i < LARGE_CACHE_CAPACITY; i++) {
        reused[i] = heap_alloc(LARGE_ALLOCATION_SIZE);
        assert(reused[i] != NULL);
        assert(pointer_is_in_array(reused[i], cached, LARGE_CACHE_CAPACITY));

        for (size_t j = 0; j < i; j++)
            assert(reused[j] != reused[i]);
    }

    for (size_t i = 0; i < LARGE_CACHE_CAPACITY; i++)
        heap_free(reused[i]);

    printf("[OK] Large allocation cache reuse test passed.\n");
}

static void test_quarantine_delayed_reuse(void)
{
    void *quarantined = heap_alloc(SMALL_ALLOCATION_SIZE);
    assert(quarantined != NULL);

    heap_free(quarantined);

    for (size_t i = 0; i < QUARANTINE_CAPACITY; i++) {
        void *pointer = heap_alloc(SMALL_ALLOCATION_SIZE);
        assert(pointer != NULL);
        assert(pointer != quarantined);
        heap_free(pointer);
    }

    void *reused = heap_alloc(SMALL_ALLOCATION_SIZE);
    assert(reused == quarantined);
    heap_free(reused);

    printf("[OK] Quarantine delayed reuse test passed.\n");
}

int main(void)
{
    test_large_cache_reuse();
    test_quarantine_delayed_reuse();

    return 0;
}
