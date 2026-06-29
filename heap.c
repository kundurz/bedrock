#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <stddef.h>
#include "heap_internal.h"
#include "utils.h"

struct fast_chunk** fast_chunk_bins;
struct large_chunk** large_chunk_bin;

char* metadata_start;
char* metadata_current;
char* metadata_end;


/* Heap utility functions for large bin. */
struct large_chunk* next_physical_chunk(struct large_chunk* chunk) {
    struct large_chunk* next = (struct large_chunk*)((char*)chunk + sizeof(struct large_chunk) + chunk->size);

    if (chunk_in_span(chunk, next))
        return next;

    return NULL;
}

struct large_chunk* prev_physical_chunk(struct large_chunk* chunk) {
    struct large_chunk* prev = (struct large_chunk*)((char*)chunk - sizeof(struct large_chunk) - chunk->prev_size);

    if (chunk_in_span(chunk, prev))
        return prev;

    return NULL;
}

int chunk_in_span(struct large_chunk* ref_chunk, struct large_chunk* adj_chunk) {

    return (void*)adj_chunk >= (void*)ref_chunk->span.start && (void*)adj_chunk < (void*)ref_chunk->span.end;
}

void unlink_large_free_chunk(struct large_chunk* chunk) {
    if (chunk->bk == NULL) // In the case that it is the first chunk.
        *large_chunk_bin = chunk->fd;
    if (chunk->fd != NULL)
        chunk->fd->bk = chunk->bk; // this is the only one required since the chunk is at the beginning of the free list.
    if (chunk->bk != NULL)
        chunk->bk->fd = chunk->fd;
    chunk->allocated = 1;
}

void insert_large_free_chunk(struct large_chunk* chunk) {
    if (*large_chunk_bin != NULL) 
        (*large_chunk_bin)->bk = chunk;

    chunk->fd = *(large_chunk_bin);
    chunk->bk = NULL;
    *large_chunk_bin = chunk;
}

void print_large_bin_contents() {
    struct large_chunk* curr = *large_chunk_bin;

    while (curr != NULL) {
        printf(" | size = %ld, next = %p | -> ", curr->size, curr->fd);

        curr = curr->fd;
    }


    printf(" NULL\n");
}

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

    // Allocate large chunk bins on the heap.
    large_chunk_bin = (struct large_chunk**)_metadata_alloc(sizeof(struct large_chunk*));

    return 0;
}

/*
    _allocate_large_bin_chunk() allocates and initializes a new large bin chunk
    and places at the beginning of the large bin.

    List before:
    Large bin --> chunk1 <--> chunk2 --> NULL
    
    List After:
    Large bin --> new chunk <--> chunk1 <--> chunk2 --> NULL

    This is done because it is simplest, but I may opt for a more secure method later. 
*/
struct large_chunk* _allocate_large_bin_chunk(int size, struct large_chunk** bin) 
{
    size += sizeof(struct large_chunk);
    char* new_page = (char*)mmap(
        NULL, 
        size, 
        PROT_READ | PROT_WRITE, 
        MAP_PRIVATE | MAP_ANON, 
        -1,
        0
    );

    // I need to turn this into a valid chunk, first I need to round up to the next page size, 
    struct large_chunk* new_chunk = (struct large_chunk*)new_page;
    new_chunk->size = round_to_nearest_page(size) - sizeof(struct large_chunk);
    
    if ((*bin) != NULL)
        (*bin)->bk = new_chunk;

    new_chunk->allocated = 0;
    new_chunk->fd = *bin;
    new_chunk->bk = NULL;
    new_chunk->span.start = new_page;
    new_chunk->span.end = (char*)new_page + new_chunk->size + sizeof(struct large_chunk);
    new_chunk->prev_size = -1;

    *bin = new_chunk;

    return new_chunk;
}

/*
    _search_large_bin() goes through the large bin to
*/
struct large_chunk* _search_large_bin_first_fit(int size) {
    struct large_chunk* curr = *large_chunk_bin;

    // I'll start with first fit for simplicity.
    while (curr != NULL) {
        if (curr->size >= size) {
            return curr;
        }
        curr = curr->fd;
    }

    return NULL;
}

/*
    _split_large_chunk() will modify chunk such that 
    size bytes are split off of it. 

    chunk is modified with a new size. 
*/
void _split_large_chunk(struct large_chunk* chunk, int size) {
    // We subtract the size of the chunk we want
    // Then we subtract the size of a header because
    // the new chunk will have to have a header added to it as well.
    // chunk->size is used because we count up to "size" bytes starting from the user data section.
    if (chunk->size - size < sizeof(struct large_chunk)) 
        return;

    size_t new_size = chunk->size - size; 

    char* free_space = ((char*)chunk + sizeof(struct large_chunk));
    struct large_chunk* split_chunk = (struct large_chunk*)(free_space + size);

    split_chunk->fd = chunk->fd;
    split_chunk->bk = chunk;
    split_chunk->allocated = 0;
    split_chunk->prev_size = size;
    split_chunk->span = chunk->span;
    split_chunk->size = new_size - sizeof(struct large_chunk);

    chunk->fd = split_chunk; 
    chunk->size = size;
    chunk->allocated = 0; 
}

/*
    _merge_two_large_chunks() 

    Assuming chunk1 at a lower address than chunk2
*/
void _merge_two_large_chunks(struct large_chunk* chunk1, struct large_chunk* chunk2) {
    
    // UPDATE RELEVANT METADATA
    chunk1->size += chunk2->size + sizeof(struct large_chunk); // We can now use the header from chunk2 as free space
    struct large_chunk* next = (struct large_chunk*)((char*)chunk1 + sizeof(struct large_chunk) + chunk1->size);

    if ((char*)next < (char*)chunk1->span.end) {
        next->prev_size = chunk1->size;
    }

    if (chunk2->bk != NULL) {
        chunk2->bk->fd = chunk2->fd;
    } else {
        *large_chunk_bin = chunk2->fd;
    }

    if (chunk2->fd != NULL) {
        chunk2->fd->bk = chunk2->bk;
    }

    chunk2->fd = NULL;
    chunk2->bk = NULL;

    // It may be beneifical to zero out chunk 2's metadata at some point,
    chunk1->allocated = 0;
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

    if (size_class != -1) {
        struct fast_chunk** bin = &fast_chunk_bins[get_fast_chunk_index(size_class)];

        // If the relevant size class bin is empty, fill it up again.
        if (*bin == NULL) { 
            _allocate_fast_bin_page(size_class, bin);
        } 

        // Return user data region of allocated chunk.
        struct fast_chunk* allocated_chunk = bin_pop(bin);
        return (void*)((char*)allocated_chunk + sizeof(struct fast_chunk));
    } else {
        // Search for a valid size large chunk.
        struct large_chunk* chunk = _search_large_bin_first_fit(bytes); // Looks ok 

        if (chunk == NULL) {
            chunk = _allocate_large_bin_chunk(bytes, large_chunk_bin); 
        } 

        _split_large_chunk(chunk, bytes); // chunk is now the correct size


        unlink_large_free_chunk(chunk);

        return (void*)((char*)chunk + sizeof(struct large_chunk));
    }
}

/*
    heap_free() is an interface for the user to free heap memory.
*/
void heap_free(void* ptr) 
{
    uint64_t size = *(uint64_t *)(
    (char*)ptr
    - sizeof(struct large_chunk)
    + offsetof(struct large_chunk, size)
    );


    if (size <= 2048) {
        // We're going to need some way to check the size. 
        // Peer into metadata
        struct fast_chunk* fast_chunk = (struct fast_chunk*)ptr - 1; 
        struct fast_chunk** bin = &fast_chunk_bins[get_fast_chunk_index(fast_chunk->size_class)];
        bin_push(fast_chunk, bin);
    } else {
        struct large_chunk* large_chunk = (struct large_chunk*)ptr - 1;

        //struct large_chunk* forward_adj_chunk = (struct large_chunk*)((char*)large_chunk + sizeof(struct large_chunk) + large_chunk->size);
        struct large_chunk* forward_adj_chunk = next_physical_chunk(large_chunk);

        // We subtract sizeof(struct large chunk) twice because we need to get to the BEGINNING of the previous chunk.
        struct large_chunk* backward_adj_chunk = prev_physical_chunk(large_chunk);

        // insert_large_free_chunk
        insert_large_free_chunk(large_chunk);
                

        int merge_occurred = 0; 
        if (forward_adj_chunk != NULL && forward_adj_chunk->allocated == 0) {
            merge_occurred = 1;
            _merge_two_large_chunks(large_chunk, forward_adj_chunk); // I think it matters the order in which you merge the chunks. Whichever has the lower memory address should be the one that stays in the list.
        }
        
        if (backward_adj_chunk != NULL && backward_adj_chunk->allocated == 0) {            
            merge_occurred = 1;
            _merge_two_large_chunks(backward_adj_chunk, large_chunk);
        }

        if (!merge_occurred) large_chunk->allocated = 0;

    }
}