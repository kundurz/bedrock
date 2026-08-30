/*
 * Public Bedrock allocator interface.
 *
 * 
 * This translation unit separates stable public API from the allocator's
 * internal implementation. Internal heap functions and data structures
 * are intentionally not exposed through bedrock.h
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