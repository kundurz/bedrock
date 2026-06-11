#include "heap_internal.h"
#include "utils.h"
#include <stdio.h>
#include <string.h>

int main() {
    int return_value = _heap_init();

    char* mystr = (char*)heap_alloc(2049);
    strcpy(mystr, "hello!");
    printf("%s\n", mystr);

    heap_free(mystr);
    //char* my_second_str = (char*)heap_alloc(12);
    //printf("%s\n", mystr);
    return 0;
}