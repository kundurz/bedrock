#include "heap_internal.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(void)
{
    // I'm going to have to test this stuff myself.
    // 1) The size class must be appropriate --> I will trust this. I think it should be fine.
    // 2) The slot is from the correct slab in the correct bin
    // 3) You add a new slab when there are no more slots left. 

    heap_alloc(16);
    heap_alloc(32);
    heap_alloc(64);
    heap_alloc(128);
    heap_alloc(256);
    heap_alloc(512);
    heap_alloc(1024);
    heap_alloc(2048);



    // Yes (2) is now going to be verified by this.

    for (int i = 0; i < 255; i++) {
        heap_alloc(16);
    }
    print_all_slabs();

    puts("=== EXAMINE HERE!!! ===");
    heap_alloc(16);
    print_all_slabs();

    return 0;
}