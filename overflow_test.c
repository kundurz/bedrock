#include "heap_internal.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FAST_ALLOCS 1000
#define LARGE_ALLOCS 1000
#define MAX_LIVE 512

struct allocation {
    void *ptr;
    size_t size;
    unsigned char pattern;
};

static unsigned char pattern_for(size_t id, size_t size)
{
    return (unsigned char)((id * 131u + size * 17u) & 0xffu);
}

static void fill_allocation(struct allocation *alloc)
{
    memset(alloc->ptr, alloc->pattern, alloc->size);
}

static void verify_allocation(const struct allocation *alloc)
{
    unsigned char *p = alloc->ptr;

    for (size_t i = 0; i < alloc->size; i++) {
        assert(p[i] == alloc->pattern);
    }
}

static void verify_all_live(const struct allocation *live, size_t live_count)
{
    for (size_t i = 0; i < live_count; i++) {
        verify_allocation(&live[i]);
    }
}

static size_t random_fast_size(void)
{
    return (size_t)(rand() % 2049);
}

static size_t random_large_size(void)
{
    return 2049u + (size_t)(rand() % 14000);
}

static void remove_live_allocation(struct allocation *live, size_t *live_count, size_t index)
{
    live[index] = live[*live_count - 1];
    (*live_count)--;
}

int main(void)
{
    struct allocation live[MAX_LIVE];
    size_t live_count = 0;
    size_t fast_done = 0;
    size_t large_done = 0;
    size_t allocation_id = 1;

    srand(0xC0FFEE);
    assert(_heap_init() == 0);

    while (fast_done < FAST_ALLOCS || large_done < LARGE_ALLOCS) {
        int should_free = live_count > 0 && (live_count == MAX_LIVE || (rand() % 100) < 45);

        if (should_free) {
            size_t index = (size_t)(rand() % live_count);

            verify_allocation(&live[index]);
            heap_free(live[index].ptr);
            remove_live_allocation(live, &live_count, index);
        } else {
            int choose_fast;

            if (fast_done == FAST_ALLOCS) {
                choose_fast = 0;
            } else if (large_done == LARGE_ALLOCS) {
                choose_fast = 1;
            } else {
                choose_fast = rand() & 1;
            }

            size_t size = choose_fast ? random_fast_size() : random_large_size();
            void *ptr = heap_alloc(size);

            assert(ptr != NULL);
            assert(live_count < MAX_LIVE);

            live[live_count].ptr = ptr;
            live[live_count].size = size;
            live[live_count].pattern = pattern_for(allocation_id++, size);
            fill_allocation(&live[live_count]);
            live_count++;

            if (choose_fast) {
                fast_done++;
            } else {
                large_done++;
            }
        }

        if (((fast_done + large_done) % 100) == 0) {
            verify_all_live(live, live_count);
        }
    }

    while (live_count > 0) {
        size_t index = (size_t)(rand() % live_count);

        verify_allocation(&live[index]);
        heap_free(live[index].ptr);
        remove_live_allocation(live, &live_count, index);
    }

    assert(fast_done == FAST_ALLOCS);
    assert(large_done == LARGE_ALLOCS);

    puts("[OK] randomized allocator overwrite test passed");
    return 0;
}