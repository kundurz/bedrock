#define _GNU_SOURCE

#include <stdlib.h>
#include <sys/mman.h>
#include "heap_internal.h"

struct fast_chunk **fast_chunk_bins;

int heap_init()
{
    // Allocate space for fast chunk bins.
    fast_chunk_bins = (struct fast_chunk **)mmap(
        NULL,
        8 * sizeof(struct fast_chunk *),
        PROT_READ | PROT_WRITE,
        MAP_PRIVATE | MAP_ANON,
        -1,
        0
    );

    return 0;
}