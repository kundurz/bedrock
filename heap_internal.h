#pragma once

#include <stdint.h>

/* Structs for chunks and metadata  */
struct fast_chunk
{
    int allocated : 1;
    uint16_t size_class;
    struct fast_chunk *fd;
    char *user_data;
};

struct large_chunk
{
    int allocated : 1;
    uint64_t size_class;
    struct large_chunk *fd;
    struct large_chunk *bk;
    char *user_data;
};

/* Internal heap functions */
int _heap_init();
void _allocate_fast_bin_page(int size_class, struct fast_chunk** bin); 
int heap_alloc(int bytes); 
void print_list(struct fast_chunk* hd); 
