#include <stdio.h>
#include <assert.h>
#include "heap.h"

static const size_t sizes[] = {
    2049,
    4095,
    4097,
    8191,
    8192,
    8193,
    64 * 1024,
    1024 * 1024,
    16 * 1024 * 1024, 
    64 * 1024 * 1024
};

int main() {

    // For these sorts of allocations
    char* ptr[2048];

    for (int k = 0; k <= 2048; k++) {
        for (int i = 0; i < 2048; i++) {
            ptr[i] = (char*)heap_alloc(k);
            assert(ptr[i] != NULL);
        }

        for (int i = 0; i < 2048; i++) {
            heap_free(ptr[i]);
        }
    }

    printf("[0K] Small allocation size tests pass (nothing crashed)\n");

    char* large_ptrs[16];
    for (long unsigned int i = 0; i < sizeof(sizes) / sizeof(size_t); i++) {
        for (int j = 0; j < 16; j++) {
            large_ptrs[j] = heap_alloc(sizes[i]);
            assert(large_ptrs[j] != NULL);
        }

        for (int k = 0 ; k < 16; k++) {
            heap_free(large_ptrs[k]);
        }
    }

    printf("[0K] Large allocation size tests pass (nothing crashed)\n");
    return 0;
}