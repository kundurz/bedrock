#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <assert.h>
#include "heap_internal.h"

int main() {
    for (int i = 0; i < 10000; i++) {
        int random_bytes = rand() % 2049;
        uintptr_t ptr = (uintptr_t)heap_alloc(random_bytes);
        assert(ptr % _Alignof(max_align_t) == 0);
        heap_free((void*)ptr);
    }

    printf("[OK] Small alignment tests pass.\n");

    for (int i = 0; i < 10000; i++) {
        int random_bytes = 2049 + (rand() % (64 * 1024 - 2049 +1));
        uintptr_t ptr = (uintptr_t)heap_alloc(random_bytes);
        assert(ptr % _Alignof(max_align_t) == 0);
        heap_free((void*)ptr);
    }

    printf("[OK] Large alignment tests pass.\n");
    return 0;
}
