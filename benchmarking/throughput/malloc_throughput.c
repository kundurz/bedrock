// Measures steady-stat churn throughput
#define _POSIX_C_SOURCE 199309L

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define WINDOW_SIZE 10000
#define TOTAL_ITERATIONS 5000000

struct allocation_record {
    void* ptr; 
    size_t size;
};

int main() {

    struct allocation_record window[WINDOW_SIZE];
    size_t head = 0;

    // Phase 1: RAMP UP
    // Fill the window to establish the "live set" steady-state memory volume.
    for (size_t i = 0; i < WINDOW_SIZE ; i++) {
        size_t size = rand() % 2049;

        window[i].ptr = malloc(size);
        window[i].size = size;
    }

    // Phase 2: STEADY-STATE CHURN (MEASURED)
    // Continuously free an old block and allocate a new one.

    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    for (size_t i = 0 ; i < TOTAL_ITERATIONS ; i++) {
        // 1. Fre ethe oldest allocation in the window.
        free(window[head].ptr);

        // 2. Allocate a new block and replace it in the window.
        size_t new_size = rand() % 2049;
        window[head].ptr = malloc(new_size);
        window[head].size = new_size;

        head = (head + 1) % WINDOW_SIZE;
    }

    clock_gettime(CLOCK_MONOTONIC, &end); 

    // Phase 3: Teardown

    for (size_t i = 0; i < WINDOW_SIZE; i++) {
        free(window[head].ptr);
        head = (head + 1 ) % WINDOW_SIZE;
    }

    // calculate metrics
    double elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
    double ops_per_sec = (TOTAL_ITERATIONS * 2) / elapsed;

    printf("Steady-State Churn Throughput: %.2f ops/sec\n", ops_per_sec);
}