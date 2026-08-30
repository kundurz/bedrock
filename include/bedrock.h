#pragma once

#include <stddef.h>

/*
 * Allocate at least `bytes` bytes of suitably aligned memory.
 *
 * Returns a pointer to the allocation on success. The returned memory remains
 * valid until released with bedrock_free(). Returns NULL if the allocation
 * cannot be completed.
 */
void* bedrock_alloc(size_t bytes); 

/*
 * Release an allocation returned by bedrock_alloc().
 *
 * Passing NULL has no effect. Passing an invalid pointer, an interior pointer,
 * or a pointer that has already been freed terminates the process.
 */
void bedrock_free(void* pointer);