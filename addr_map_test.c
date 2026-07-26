#include <stdio.h>
#include <sys/mman.h>
#include "addr_map.h"
#include "heap_internal.h"

int main() 
{
    initialize_hash_map();


    struct slab slab_metadata = { 0 }; 

    print_map_state();

    for (int i = 0; i < 8; i++) {
        void* my_addr = mmap(
            NULL, 
            4096,
            PROT_READ | PROT_WRITE,
            MAP_PRIVATE | MAP_ANON,
            -1, 
            0
        );
        addr_map_insert((uintptr_t)my_addr, slab_metadata);
        printf("-----\n");
        addr_map_enumerate();

        getchar();
    }

    return 0;
}