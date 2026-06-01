#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include "heap_internal.h"
#include "utils.h"

struct fast_chunk **fast_chunk_bins;

char* metadata_start;
char* metadata_current;
char* metadata_end;

/* 
    _metadata_alloc() allocates memory space
    for heap metadata and related data structures.
*/
char* _metadata_alloc(int bytes) {
    char* metadata_return = metadata_current;
    metadata_current += bytes;

    return metadata_return;
}

/* 
    _heap_init() returns 0 on success and -1 on error.
*/
int _heap_init()
{
    // Allocate the metadata region of the heap.
    metadata_start = (char*)mmap(
        NULL, 
        4096,
        PROT_READ | PROT_WRITE,
        MAP_PRIVATE | MAP_ANON,
        -1, 
        0
    );

    // Check if mmap worked.
    if (metadata_start == MAP_FAILED) {
        perror("mmap: MAP FAILED");
        return -1;
    }

    // Pointers for managing heap metadata's memory.
    metadata_end = metadata_start + 4096;
    metadata_current = metadata_start;

    // Allocate fast chunk bins on the heap.
    fast_chunk_bins = (struct fast_chunk**)_metadata_alloc(8 * sizeof(struct fast_chunk*));
    //_allocate_fast_bin_page(16, &fast_chunk_bins[0]);
    //print_list(fast_chunk_bins[0]);

    return 0;
}
/*
    _allocate_fast_bin_page() is a function called to get more memory
    when a given fast bin is empty (there are no more chunks). 
*/
void _allocate_fast_bin_page(int size_class, struct fast_chunk** bin) 
{

    // Allocate a new page for whichever bin we are dealing with.
    char* new_page = (char*)mmap(
        NULL, 
        4096, 
        PROT_READ | PROT_WRITE, 
        MAP_PRIVATE | MAP_ANON, 
        -1, 
        0
    );

    // Pointers used to traverse the memory region. 
    // char* used because I want byte-level granularity.
    char* start_pointer = new_page; 
    char* current = start_pointer; 
    char* end_pointer = start_pointer + 4096;

    // Split up the memory page into chunks of the relevant size class.  
    struct fast_chunk* prev = NULL;
    while (current < end_pointer) {
        struct fast_chunk* current_fast_chunk = (struct fast_chunk*)current; 

        // Ensuring the head pointer is set properly. 
        if (prev != NULL) {
            prev->fd = current_fast_chunk;
        } else {
            *bin = current_fast_chunk;
        }

        // Setting metadata.
        current_fast_chunk->size_class = size_class;
        current_fast_chunk->fd = NULL;

        prev = current_fast_chunk;

        // Additiona accounts for desires size as well as metadata. 
        current += size_class + sizeof(struct fast_chunk);
    }


}

/*
    bin_pop() pops a chunk off of a fast chunk bin and returns it. 
*/
struct fast_chunk* bin_pop(struct fast_chunk** bin) 
{

    // Pop off an entry from the head; then set a new head.
    struct fast_chunk* to_pop = *bin;
    *bin = (*bin)->fd;

    // Move the pointer past the metadata region.
    return to_pop; 
}

/*
    bin_push() pushes a chunk onto a fast chunk bin and returns it.
*/
void bin_push(struct fast_chunk* to_push, struct fast_chunk** bin) 
{
    // Setting to_push as the head of the list.
    to_push->fd = *bin;
    *bin = to_push;
}

/*
    Function used for debugging.
*/
void print_list(struct fast_chunk* hd) 
{
    struct fast_chunk* curr = hd;
    int counter = 1;
    while (curr != NULL) {
        printf("%d: %d\n", counter, curr->size_class);
        curr = curr->fd;
        counter++;  
    }
}

/*
    Allocates some heap memory for the user.
*/
void* heap_alloc(size_t bytes) 
{
    // Determining the correct size class.
    int size_class = determine_size_class(bytes); 
    struct fast_chunk** bin = &fast_chunk_bins[get_fast_chunk_index(size_class)];

    // If the relevant size class bin is empty, fill it up again.
    if (*bin == NULL) { 
        _allocate_fast_bin_page(size_class, bin);
    } 

    // Return user data region of allocated chunk.
    struct fast_chunk* allocated_chunk = bin_pop(bin);
    return allocated_chunk + sizeof(struct fast_chunk);
}

/*
    heap_free() is an interface for the user to free heap memory.
*/
void heap_free(void* ptr) 
{
    // Peer into metadata
    struct fast_chunk* fast_chunk = (struct fast_chunk*)ptr - 1; 
    struct fast_chunk** bin = &fast_chunk_bins[get_fast_chunk_index(fast_chunk->size_class)];

    bin_push(fast_chunk, bin);
}
