#pragma once

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

struct guarded_region {
    void* usable_ptr;
    void* mmap_base; 
    size_t total_size; 

    ssize_t offset;
};

struct guarded_region create_gaurded_region(size_t length, bool offset); 
void destroy_guarded_region(struct guarded_region* region); 

void lock_page(void* base, size_t region_size);
void unlock_page(void* base, size_t region_size); 

void fisher_yates_shuffle(uint16_t *indicies, size_t length); 
bool add_size_t_safely(size_t num1, size_t num2, size_t* result); 
