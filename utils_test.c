#include <stdio.h>
#include "utils.h"

int main() {
    int sizes[] = {15, 16, 17, 0};

    for (int i = 0; i < 4; i++) 
        printf("%d\n", determine_size_class(sizes[i]));

    return 0; 
}