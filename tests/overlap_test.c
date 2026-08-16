#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "heap.h"

struct allocation {
    char* ptr;

    char byte;
    size_t length;
};

int main() {

    for (size_t alloc_size = 0; alloc_size < 2048; alloc_size++) {
        struct allocation allocs[255] = { 0 };
        for (int j = 0; j < 255; j++) {
            allocs[j].ptr = heap_alloc(alloc_size);
            allocs[j].length  = alloc_size;
            allocs[j].byte = j;
            for (size_t k = 0 ; k < alloc_size ; k++) {
                allocs[j].ptr[k] = j;
            }
        }

        for (int i = 0; i < 255; i++) {
            char byte = allocs[i].byte;

            for (size_t k = 0; k < allocs[i].length; k++)  
                assert(allocs[i].ptr[k] == byte);
        }
    }

    printf("[OK] Small allocation overflow test passed.\n");

    return 0;
}