#include "heap_internal.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NUM_POINTERS 100
#define NUM_OPERATIONS 1000

int main(void)
{

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

    //void* pointers[100];
    //for (int i = 0; i < 100; i++) {
    //    size_t requested_size = rand()  % 2049;
    //    printf("========================\n");
    //    printf("TEST NUMBER %d\n", i + 1);
    //    printf("requested size: %ld\n", requested_size);
    //    void* myptr = heap_alloc(requested_size);
    //    pointers[i] = myptr;
    //    printf("User data address: %p\n", myptr);
    //    printf("========================");
    //}

    //for (int i = 0; i < 100; i++) {
    //    printf("========================\n");
    //    printf("FREE NUMBER %d\n", i + 1);
    //    printf("Address: %p", pointers[i]);
    //    heap_free(pointers[i]);
    //    printf("========================");
    //}

    /* LARGE BIN TESTS */
    // So I need to test:
    // 1) Large bin initializes itself correctly
    // 2) Splitting works corrrectly (Split followed by removing the split component from the list and re-inserting it afterwards)
    // 3) Coalescing after free.
    // 4) Ensuring we can write to the region of memory allocated, as well as read from it.
    // Then I need to do one large test

    //printf("SIZE OF LARGE CHUNK: %ld\n", sizeof(struct large_chunk));
    //// Testing of merging and all 
    //printf("**** FIRST ALLOCATION ****\n");
    //char* my_ptr = (char*)heap_alloc(5000);
    //printf("\n\n\n");

    //printf("**** SECOND ALLOCATION ****\n");
    //char *my_ptr2 = (char*)heap_alloc(2049);
    //printf("\n\n\n");

    //printf("****** SECOND FREE ******\n");
    //heap_free(my_ptr2);
    //printf("\n\n\n");

    //printf("****** FIRST FREE ******\n");
    //heap_free(my_ptr);
    //printf("\n\n\n");

    printf("**** THIRD ALLOCATION ****\n");
    char *my_ptr = (char*)heap_alloc(10000);
    printf("\n\n\n");

    printf("**** THIRD FREE ****\n");
    heap_free(my_ptr);
    printf("\n\n\n");

    printf("**** FOURTH ALLOCATION ****\n");
    char* my_ptr2 = (char*)heap_alloc(2049);
    printf("\n\n\n");

    printf("**** FIFTH ALLOCATION ****\n");
    char* my_ptr3 = (char*)heap_alloc(2049);
    printf("\n\n\n");

    printf("**** SIXTH ALLOCATION ****\n");
    char* my_ptr4 = (char*)heap_alloc(2049);
    printf("\n\n\n");

    printf("**** SEVENTH ALLOCATION ****\n");
    char* my_ptr5 = (char*)heap_alloc(5917);
    printf("\n\n\n");

    printf("**** FORUTH FREE ****\n");
    heap_free(my_ptr2); 
    printf("\n\n\n");

    printf("**** SIXTH FREE ****\n");
    heap_free(my_ptr4);
    printf("\n\n\n");

    printf("**** FIFTH FREE ****\n");
    heap_free(my_ptr3);
    printf("\n\n\n");


    printf("**** SEVENTH FREE ****\n");
    heap_free(my_ptr5);
    printf("\n\n\n");

    return 0;
}