#include <stdio.h>
#include <stddef.h>
#include "heap_internal.h"
#include "heap_stats.h"

#define ALLOCATION_COUNT 100000

struct allocation {
    void *ptr;
    size_t requested_size;
};

struct allocation allocations[ALLOCATION_COUNT];

void shuffle_allocations(struct allocation *allocations, size_t count)
{
    for (size_t i = count; i > 1; --i) {
        size_t j = (size_t)rand() % i;

        struct allocation temporary = allocations[i - 1];
        allocations[i - 1] = allocations[j];
        allocations[j] = temporary;
    }
}

size_t random_size_large(void) {
    return 2049 + (size_t)rand() % 6143;
}

size_t random_size_small(void) {
    return rand() % 2049;
}


void large_test() {
    size_t live_requested_bytes = 0;
    for (size_t i = 0 ; i < ALLOCATION_COUNT ; i++) {
        size_t size = random_size_large(); 

        allocations[i].ptr = heap_alloc(size);
        allocations[i].requested_size = size;
        live_requested_bytes += size;
    }

    size_t slab_bytes = heap_stats_mapped_slab_bytes();
    size_t large_bytes = heap_stats_mapped_large_bytes();
    size_t metadata_bytes = heap_stats_mapped_metadata_bytes(); 
    size_t payload_mapped_bytes = slab_bytes + large_bytes;
    size_t total_mapped_bytes = slab_bytes + metadata_bytes + large_bytes;

    double full_working_set_payload_overhead =
    ((double)payload_mapped_bytes / (double)live_requested_bytes - 1.0)
    * 100.0;

    double total_overhead =
    ((double)total_mapped_bytes / (double)live_requested_bytes - 1.0)
    * 100.0;


    printf("[LARGE] full working set payload overhead: %.2f %%\n", full_working_set_payload_overhead);
    printf("[LARGE] full working set total overhead: %.2f %%\n", total_overhead);

    shuffle_allocations(allocations, ALLOCATION_COUNT);

    for (size_t i = 0 ; i < ALLOCATION_COUNT / 2 ; i++) {
        heap_free(allocations[i].ptr);
        live_requested_bytes -= allocations[i].requested_size;
        allocations[i].ptr = NULL;
    }

    slab_bytes = heap_stats_mapped_slab_bytes();
    large_bytes = heap_stats_mapped_large_bytes();
    metadata_bytes = heap_stats_mapped_metadata_bytes(); 
    payload_mapped_bytes = slab_bytes + large_bytes;
    total_mapped_bytes = slab_bytes + metadata_bytes + large_bytes;

    double after_50_percent_random_free_payload_overhead =
    ((double)payload_mapped_bytes / (double)live_requested_bytes - 1.0)
    * 100.0;

    double after_50_percent_total_overhead =
    ((double)total_mapped_bytes / (double)live_requested_bytes - 1.0)
    * 100.0;


    printf("[LARGE] after 50 percent random free payload overhead: %.2f %%\n", after_50_percent_random_free_payload_overhead);
    printf("[LARGE] after 50 percent total overhead: %.2f %%\n", after_50_percent_total_overhead);
}

void small_test() {
    size_t live_requested_bytes = 0;
    for (size_t i = 0 ; i < ALLOCATION_COUNT ; i++) {
        size_t size = random_size_small(); 

        allocations[i].ptr = heap_alloc(size);
        allocations[i].requested_size = size;
        live_requested_bytes += size;
    }

    size_t slab_bytes = heap_stats_mapped_slab_bytes();
    size_t large_bytes = heap_stats_mapped_large_bytes();
    size_t metadata_bytes = heap_stats_mapped_metadata_bytes(); 
    size_t payload_mapped_bytes = slab_bytes + large_bytes;
    size_t total_mapped_bytes = slab_bytes + metadata_bytes + large_bytes;

    double full_working_set_payload_overhead =
    ((double)payload_mapped_bytes / (double)live_requested_bytes - 1.0)
    * 100.0;

    double total_overhead =
    ((double)total_mapped_bytes / (double)live_requested_bytes - 1.0)
    * 100.0;


    printf("[SMALL] full working set payload overhead: %.2f %%\n", full_working_set_payload_overhead);
    printf("[SMALL] full working set total overhead: %.2f %%\n", total_overhead);

    shuffle_allocations(allocations, ALLOCATION_COUNT);

    for (size_t i = 0 ; i < ALLOCATION_COUNT / 2 ; i++) {
        heap_free(allocations[i].ptr);
        live_requested_bytes -= allocations[i].requested_size;
        allocations[i].ptr = NULL;
    }

    slab_bytes = heap_stats_mapped_slab_bytes();
    large_bytes = heap_stats_mapped_large_bytes();
    metadata_bytes = heap_stats_mapped_metadata_bytes(); 
    payload_mapped_bytes = slab_bytes + large_bytes;
    total_mapped_bytes = slab_bytes + metadata_bytes + large_bytes;

    double after_50_percent_random_free_payload_overhead =
    ((double)payload_mapped_bytes / (double)live_requested_bytes - 1.0)
    * 100.0;

    double after_50_percent_total_overhead =
    ((double)total_mapped_bytes / (double)live_requested_bytes - 1.0)
    * 100.0;


    printf("[SMALL] after 50 percent random free payload overhead: %.2f %%\n", after_50_percent_random_free_payload_overhead);
    printf("[SMALL] after 50 percent total overhead: %.2f %%\n", after_50_percent_total_overhead);
}
int main() {

    srand(12345);
    large_test();
}