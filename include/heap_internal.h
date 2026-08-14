#pragma once

#include <stdint.h>
#include <stdlib.h>

#define MAX_SLAB_SLOTS 1024

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
    uint8_t alloc_bytemap[MAX_SLAB_SLOTS];
    struct slab *next;
    struct slab *prev;

    /* Fisher-Yates Array */
    uint16_t free_top;
    uint16_t indicies[MAX_SLAB_SLOTS];
};

/* Internal heap functions */
int _heap_init();
void _allocate_fast_bin_page(int size_class, struct slab** bin); 
void* heap_alloc(size_t bytes); 
void heap_free(void* ptr);
void print_large_bin_contents(); 
void print_all_slabs(); 