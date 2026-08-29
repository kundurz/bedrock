/*
 * Public allocator interface.
 *
 * Security-critical state is maintained outside user-controlled
 * allocation regions.
 */

#include <stddef.h>
#include "heap_internal.h"
#include "bedrock.h"

void* bedrock_alloc(size_t bytes) {
    return heap_alloc(bytes);
}

void bedrock_free(void* pointer) {
    heap_free(pointer);
}