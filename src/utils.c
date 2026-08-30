#include <sys/mman.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "utils.h"
#include "secure_utils.h"

int determine_size_class(size_t size)
{
    if (size > 2048) return -1;
    if (size <= 16)
        return 16;

    size_t cls = 16;

    while (cls < size && cls < 2048)
        cls <<= 1;

    if (cls > 2048)
        return -1;

    return (int)cls;
}

int get_slab_cache_index(size_t size_class) {
    switch(size_class) {
        case 16:
            return 0;
        case 32: 
            return 1;
        case 64:
            return 2;
        case 128:
            return 3;
        case 256:
            return 4;
        case 512:
            return 5;
        case 1024:
            return 6;
        case 2048:
            return 7;
        default:
            return -1;
    }
}

size_t round_to_nearest_page(size_t num) {
    long page_size = sysconf(_SC_PAGESIZE);

    size_t sum;
    if (!add_size_t_safely(num, page_size - 1, &sum))
        _exit(127);

    return (sum) & ~(page_size - 1);
}

size_t round_down_power_of_two(size_t value) {
    if (value == 0)
        return 0;

    size_t result = 1;

    while (result <= value / 2)
        result *= 2;

    return result;
}

void generic_swap(void *a, void *b, size_t size) {
    char temp[size]; 
    memcpy(temp, a, size);
    memcpy(a, b, size);
    memcpy(b, temp, size);
}


void set_free(uint64_t* bitmap, int bit_number) {

    int region_number = bit_number / 64;
    int bit_index = bit_number % 64;

    uint64_t bitmask = UINT64_C(1) << bit_index;
    
    bitmap[region_number] &= ~bitmask;

}

void set_allocated(uint64_t* bitmap, int bit_number) {

    int region_number = bit_number / 64;
    int bit_index = bit_number % 64;

    uint64_t bitmask = UINT64_C(1) << bit_index;
    
    bitmap[region_number] |= bitmask;

}

bool check_free(uint64_t* bitmap, int bit_number) {
    int region_number = bit_number / 64;
    int bit_index = bit_number % 64; 

    uint64_t bitmask = UINT64_C(1) << bit_index;
    uint64_t bitmap_region = bitmap[region_number];

    return (bitmap_region & bitmask) == 0;

}