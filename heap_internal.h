#pragma once

#include <stdint.h>
#include <stdlib.h>

/* Struct that contains the start and end address for a mmaped region */
struct span 
{
    void* start; 
    void* end;
};

/* Structs for chunks and metadata  */
/* Size must be at the end of both structs so that pointer arithmetic
may be used to determine the size of */
struct fast_chunk
{
    struct fast_chunk *fd;
    uint64_t size_class;
}; // you just add this size to the allocated chunk and there's the user data!

/* Structs for large chunks */
struct large_chunk
{
    struct span span;
    size_t prev_size;
    int allocated; // 0 if free, 1 if allocated. 
    struct large_chunk *fd;
    struct large_chunk *bk;
    uint64_t size;
};

/* Internal heap functions */
int _heap_init();
void _allocate_fast_bin_page(int size_class, struct fast_chunk** bin); 
struct large_chunk* _allocate_large_bin_chunk(int size, struct large_chunk** bin);
void _split_large_chunk(struct large_chunk* chunk, int size); 
void* heap_alloc(size_t bytes); 
void print_list(struct fast_chunk* hd); 
struct large_chunk* _search_large_bin_first_fit(int size); 
struct fast_chunk* bin_pop(struct fast_chunk** bin);
void heap_free(void* ptr);
void print_large_bin_contents(); 


int chunk_in_span(struct large_chunk* ref_chunk, struct large_chunk* adj_chunk);
struct large_chunk* prev_physical_chunk(struct large_chunk* chunk); 
struct large_chunk* next_physical_chunk(struct large_chunk* chunk); 