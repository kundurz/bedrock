#include <sys/mman.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "utils.h"

/*
    Determines the fast chunk size class 
    a size request would correspond to. 
    
    AVAILABLE SIZE CLASSES:
    16 B 
    32 B 
    64 B 
    128 B 
    256 B 
    512 B 
    1024 B
    2048 B

    Return value:
    Returns the size class or -1. 
    -1 indicates that the requested chunk is actually a large
    chunk.
*/
int determine_size_class(size_t size)
{
    if (size > 2048) return -1;
    if (size <= 16)
        return 16;

    int cls = 16;

    while (cls < size && cls < 2048)
        cls <<= 1;

    return (cls > 2048) ? -1 : cls;
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
            return -1; // Something went wrong.
    }
}

size_t round_to_nearest_page(size_t num) {
    long page_size = sysconf(_SC_PAGESIZE);

    return (num + page_size - 1) & ~(page_size - 1);
    //return (num + 4095) & ~(size_t)4095;
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