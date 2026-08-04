#pragma once

#include <stddef.h>

void *hardened_large_malloc(size_t payload_size);
void hardened_large_free(void *ptr);
