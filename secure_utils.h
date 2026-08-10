#pragma once

#include <stdlib.h>
#include <stdint.h>

struct guarded_region {
    void* usable_ptr;
    void* mmap_base; 
    size_t total_size; 
};

struct guarded_region create_gaurded_region(size_t length); 
void destroy_guarded_region(struct guarded_region* region); 

void lock_page(void* base, size_t region_size);
void unlock_page(void* base, size_t region_size); 

void fisher_yates_shuffle(uint16_t *indicies, size_t length); 