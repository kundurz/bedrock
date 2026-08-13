#define _GNU_SOURCE

#include <sys/mman.h>
#include <sys/random.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <time.h>
#include "secure_utils.h"
#include "utils.h"

struct guarded_region create_gaurded_region(size_t length, bool add_offset) {
    long page_size = sysconf(_SC_PAGESIZE);

    // Round payloa dup to nearest page boundary
    size_t rounded_payload_size = round_to_nearest_page(length);

    size_t offset = 0;
    if (add_offset) {
        offset = _generate_random_number(rounded_payload_size);

        size_t alignment = _Alignof(max_align_t);
        offset = (offset + alignment - 1) & ~(alignment - 1);
    }

    rounded_payload_size = round_to_nearest_page(rounded_payload_size + offset);

    // The total region is the requested length + 2 guard pages
    size_t total_size = rounded_payload_size + (2 * page_size);


    void *base = mmap(
        NULL, 
        total_size,
        PROT_READ | PROT_WRITE,
        MAP_PRIVATE | MAP_ANONYMOUS, 
        -1, 
        0
    );

    if (base == MAP_FAILED) {
        perror("mmap failed");
        exit(EXIT_FAILURE);
    }
    
    if (mprotect(base, page_size, PROT_NONE) != 0) {
        perror("mprotect front guard failed");
        exit(EXIT_FAILURE);
    }

    void* tail_guard = (char*)base + page_size + rounded_payload_size;
    if (mprotect(tail_guard, page_size, PROT_NONE) != 0) {
        perror("mprotect tail guard failed");
        exit(EXIT_FAILURE);
    }

    struct guarded_region region;
    region.mmap_base = base;
    region.usable_ptr = (char*)base + page_size;
    region.total_size = total_size;
    region.offset = offset;

    return region;
}

void destroy_guarded_region(struct guarded_region* region) {
    if (munmap(region->mmap_base, region->total_size) != 0) {
        perror("munmap failed");
    }
}

void lock_page(void* base, size_t region_size) {
    if (mprotect(base, region_size, PROT_NONE) != 0) {
        perror("mprotect page protect has failed");
        exit(EXIT_FAILURE);
    }

    if (madvise(base, region_size, MADV_DONTNEED) != 0) {
        perror("madvise MADV_DONTNEED failed"); 
        exit(EXIT_FAILURE);
    }
}

void unlock_page(void* base, size_t region_size) {
    if (mprotect(base, region_size, PROT_READ | PROT_WRITE) != 0) {
        perror("mprotect page unprotect has failed");
        exit(EXIT_FAILURE);
    }
}

int _generate_random_number(size_t upper_bound) {
    unsigned int raw_bytes;

    ssize_t result = getrandom(&raw_bytes, sizeof(raw_bytes), 0); 

    if (result < 0) {
        return -1;
    }

    unsigned int final_number = raw_bytes % (upper_bound + 1);

    return final_number;
}

void fisher_yates_shuffle(uint16_t *indicies, size_t length) {

    for (int i = 0; i < length; i++) indicies[i] = i;

    for (int i = length - 1; i > 0; i--) {
        int j = _generate_random_number(i);

        int tmp = indicies[i];
        indicies[i] = indicies[j];
        indicies[j] = tmp;
    }
}