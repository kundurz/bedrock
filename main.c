#include "heap_internal.h"
#include "utils.h"
#include <stdio.h>

int main() {
    int return_value = heap_init();

    if (return_value == 0) {
        printf("So far, success!\n");
    }
    return 0;
}