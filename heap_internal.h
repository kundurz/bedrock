#pragma once

#include <stdint.h>
#include <stdlib.h>

/* Structs for chunks and metadata  */
struct fast_chunk
{
    uint16_t size_class;
    struct fast_chunk *fd;
}; // you just add this size to the allocated chunk and there's the user data!

struct large_chunk
{
    uint64_t size;
    struct large_chunk *fd;
    struct large_chunk *bk;
};

/* Internal heap functions */
int _heap_init();
void _allocate_fast_bin_page(int size_class, struct fast_chunk** bin); 
void _allocate_large_bin_chunk(int size, struct large_chunk** bin);
void* heap_alloc(size_t bytes); 
void print_list(struct fast_chunk* hd); 
struct fast_chunk* bin_pop(struct fast_chunk** bin);
void heap_free(void* ptr);
