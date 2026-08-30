#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h> 
#include <time.h>

#include "heap_internal.h"

#define WINDOW_SIZE 5000
#define NUM_MEASUREMENTS 100000

uint64_t get_time_ns() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);

    return (uint64_t)ts.tv_sec * 1000000000ULL + ts.tv_nsec;
}

int compare_uint64(const void* a, const void* b) {
    uint64_t arg1 = *(const uint64_t*)a;
    uint64_t arg2 = *(const uint64_t*)b;

    if (arg1 < arg2) return -1;
    if (arg1 > arg2) return 1;

    return 0;
}

void measure_separate_latencies() {
    void* window[WINDOW_SIZE] = {0};

    for (int i = 0; i < WINDOW_SIZE; i++) {
        window[i] = malloc((rand() % 256) + 16);
    }

    uint64_t* alloc_latencies = malloc(NUM_MEASUREMENTS * sizeof(uint64_t));
    uint64_t* free_latencies = malloc(NUM_MEASUREMENTS * sizeof(uint64_t));

    int head = 0; 

    for (int i = 0 ; i < NUM_MEASUREMENTS ; i++) {
        size_t size = (rand() % 256) + 16;
        void* old_ptr = window[head];

        // Measure free latency only
        uint64_t free_start = get_time_ns();
        free(old_ptr);
        uint64_t free_end = get_time_ns();
        free_latencies[i] = free_end - free_start;

        // Measure alloc latency only
        uint64_t alloc_start = get_time_ns();
        window[head] = malloc(size);
        uint64_t alloc_end = get_time_ns();
        alloc_latencies[i] = alloc_end - alloc_start;

        head = (head + 1) % WINDOW_SIZE; 
    }

    qsort(alloc_latencies, NUM_MEASUREMENTS, sizeof(uint64_t), compare_uint64);
    printf("--- malloc() Latency (ns) ---\n");
    printf("  Median (p50): %llu\n", (unsigned long long)alloc_latencies[(int)(NUM_MEASUREMENTS * 0.50)]);
    printf("  p95:          %llu\n", (unsigned long long)alloc_latencies[(int)(NUM_MEASUREMENTS * 0.95)]);
    printf("  p99:          %llu\n", (unsigned long long)alloc_latencies[(int)(NUM_MEASUREMENTS * 0.99)]);
    printf("  Maximum:      %llu\n", (unsigned long long)alloc_latencies[NUM_MEASUREMENTS - 1]);

    // --- SORT AND REPORT FREE LATENCIES ---
    qsort(free_latencies, NUM_MEASUREMENTS, sizeof(uint64_t), compare_uint64);
    printf("\n--- free() Latency (ns) ---\n");
    printf("  Median (p50): %llu\n", (unsigned long long)free_latencies[(int)(NUM_MEASUREMENTS * 0.50)]);
    printf("  p95:          %llu\n", (unsigned long long)free_latencies[(int)(NUM_MEASUREMENTS * 0.95)]);
    printf("  p99:          %llu\n", (unsigned long long)free_latencies[(int)(NUM_MEASUREMENTS * 0.99)]);
    printf("  Maximum:      %llu\n", (unsigned long long)free_latencies[NUM_MEASUREMENTS - 1]);

    // Cleanup
    free(alloc_latencies);
    free(free_latencies);
    for (int i = 0; i < WINDOW_SIZE; i++) {
        if (window[i]) free(window[i]);
    }

}

int main() {

    measure_separate_latencies();
    return 0;
}