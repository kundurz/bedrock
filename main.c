#include "heap_internal.h"
#include "utils.h"
#include <stdio.h>
#include <string.h>

int main() {
    int return_value = _heap_init();

    char* mystr = (char*)heap_alloc(12);
    strcpy(mystr, "hello!");
    printf("%s\n", mystr);
    return 0;
}