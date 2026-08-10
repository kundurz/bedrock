#pragma once

#include <stdint.h>
#include <stdlib.h>

// FOR TESTING
extern struct large_chunk **large_chunk_bin;

/* Struct that contains the start and end address for a mmaped region */
struct span 
{
    void* start; 
    void* end;
};

/* Structs for slabs and large chunks */
struct slab {
    void *base;
    uint16_t size_class;
    uint16_t slot_count; // How many available slots there are
    uint16_t free_count; // How many of these slots are free?
//    uint64_t alloc_bitmap[4];
    uint8_t alloc_bytemap[256];
    struct slab *next;
    struct slab *prev;

    /* Fisher-Yates Array */
    uint16_t free_top;
    uint16_t indicies[256];
};

/* Internal heap functions */
int _heap_init();
void _allocate_fast_bin_page(int size_class, struct slab** bin); 
void _split_large_chunk(struct large_chunk* chunk, int size); 
void* heap_alloc(size_t bytes); 
void heap_free(void* ptr);
void print_large_bin_contents(); 
int chunk_in_span(struct large_chunk* ref_chunk, struct large_chunk* adj_chunk);
void unlink_large_free_chunk(struct large_chunk* chunk); 
void insert_large_free_chunk(struct large_chunk* chunk);

void print_all_slabs(); 