#include <sys/mman.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include "secure_utils.h"
#include "utils.h"

struct guarded_region create_gaurded_region(size_t length) {
    long page_size = sysconf(_SC_PAGESIZE);

    // Round payloa dup to nearest page boundary
    size_t rounded_payload_size = round_to_nearest_page(length);

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
}

void unlock_page(void* base, size_t region_size) {
    if (mprotect(base, region_size, PROT_NONE) != 0) {
        perror("mprotect page unprotect has failed");
        exit(EXIT_FAILURE);
    }
}