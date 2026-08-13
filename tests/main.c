#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "include/heap.h"


int main() {
    size_t name_length;
    printf("Please enter the size of your name: ");
    scanf("%d", &name_length);

    char* my_str = heap_alloc(name_length + 1);

    printf("\nPlease enter your name: ");
    scanf("%s", my_str);

    if (strlen(my_str) > name_length) {
        perror("Overflow!");
        return -1;
    }

    printf("Your name is: %s\n", my_str);
    
    return 0;
}