#include <stdio.h>
#include <assert.h>
#include "heap.h"

struct allocation {
    char* ptr;
    size_t length;
};

int main() {
    struct allocation small_ptrs[10000];
    for (int i = 0; i < 10000; i++) {
        int random_bytes = rand() % 2049;
        char* str = heap_alloc(random_bytes); 

        small_ptrs[i].ptr = str;
        small_ptrs[i].length = random_bytes;

        for (int j = 0; j < random_bytes; j++) 
            str[j] = 'A';
    }

    for (int i = 0; i < 10000; i++) {
        for (long unsigned int j = 0; j < small_ptrs[i].length; j++) {
            char c = small_ptrs[i].ptr[j];
            assert(c == 'A');
        }
    }

    printf("[OK] Small read/write tests passed.\n");

    struct allocation large_ptrs[10000];
    for (int i = 0; i < 10000; i++) {
        int random_bytes = 2049 + (rand() % (64 * 1024 - 2049 +1));

        char* str = heap_alloc(random_bytes); 

        large_ptrs[i].ptr = str;
        large_ptrs[i].length = random_bytes;

        for (int j = 0; j < random_bytes; j++) 
            str[j] = 'A';
    }

    for (int i = 0; i < 10000; i++) {
        for (long unsigned int j = 0; j < large_ptrs[i].length; j++) {
            char c = large_ptrs[i].ptr[j];
            assert(c == 'A');
        }
    }

    printf("[OK] Large read/write tests passed.\n");
     
    return 0;
}