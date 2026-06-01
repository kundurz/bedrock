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
    metadata_start = (char*)mmap(
        NULL, 
        4096,
        PROT_READ | PROT_WRITE,
        MAP_PRIVATE | MAP_ANON,
        -1, 
        0
    );

    if (metadata_start == MAP_FAILED) {
        perror("mmap: MAP FAILED");
        return -1;
    }

    metadata_end = metadata_start + 4096;
    metadata_current = metadata_start;

    fast_chunk_bins = (struct fast_chunk**)_metadata_alloc(8 * sizeof(struct fast_chunk*));

    return 0;
}

void _allocate_fast_bin_page(int size_class, struct fast_chunk** bin) {
    char* new_page = (char*)mmap(
        NULL, 
        4096, 
        PROT_READ | PROT_WRITE, 
        MAP_PRIVATE | MAP_ANON, 
        -1, 
        0
    );

    char* start_pointer = new_page; 
    char* current = start_pointer; 
    char* end_pointer = start_pointer + 4096;

    struct fast_chunk* prev = NULL;
    while (current != end_pointer) {
        struct fast_chunk* current_fast_chunk = (struct fast_chunk*)current; 

        if (prev != NULL) {
            prev->fd = current_fast_chunk;
        } else {
            *bin = current_fast_chunk;
        }

        current_fast_chunk->allocated = 0;
        current_fast_chunk->size_class = size_class;
        current_fast_chunk->fd = NULL;

        prev = current_fast_chunk;
        current += size_class;
    }
}

struct fast_chunk* bin_pop(struct fast_chunk** bin) {
    struct fast_chunk* to_pop = *bin;
    *bin = (*bin)->fd;
    
    return to_pop;
}

void print_list(struct fast_chunk* hd) {
    struct fast_chunk* curr = hd;
    while (curr != NULL) {
        printf("%d -> ", curr->size_class);
        curr = curr->fd;
    }
}

int heap_alloc(int bytes) {
    // determining the fast chunk size.
    int size_class = determine_size_class(bytes); 
    struct fast_chunk* bin = fast_chunk_bins[get_fast_chunk_index(size_class)];

    if (bin == NULL) { 
        _allocate_fast_bin_page(size_class, &bin);
    } 

}