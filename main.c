#include "heap_internal.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NUM_POINTERS 100
#define NUM_OPERATIONS 1000

int main(void)
{

    printf("Initializing heap...\n");
    _heap_init();
    /* FAST BIN TESTS */

    // I need to test the following things:
    // 1) Does the allocator return unique addresses for each allocation? 
    // 2) Can things be freed in an arbitrary order and still work?
    // 3) What happens once a bin is empty? Does it allocate a new page? 

    // Let's start by testing 1) and 3)
    // This will also test if the allocator is able to refill itself.
    // If there are no duplicate addresses then we're good!
    // Another thing is to ensure that all the tests 

    void* pointers[100];
    for (int i = 0; i < 100; i++) {
        size_t requested_size = rand()  % 2049;
        printf("========================\n");
        printf("TEST NUMBER %d\n", i + 1);
        printf("requested size: %ld\n", requested_size);
        void* myptr = heap_alloc(requested_size);
        pointers[i] = myptr;
        printf("User data address: %p\n", myptr);
        printf("========================");
    }

    for (int i = 0; i < 100; i++) {
        printf("========================\n");
        printf("FREE NUMBER %d\n", i + 1);
        printf("Address: %p", pointers[i]);
        heap_free(pointers[i]);
        printf("========================");
    }

    /* LARGE BIN TESTS */

    return 0;
}