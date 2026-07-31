#define _GNU_SOURCE
#define PAGE_SIZE_BYTES 4096

#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <stddef.h>
#include <assert.h>
#include "heap_internal.h"
#include "utils.h"
#include "addr_map.h"

struct slab** fast_caches;
struct large_chunk** large_chunk_bin;

char* metadata_start;
char* metadata_current;
char* metadata_end;

int heap_initialized = 0;


void print_slab(struct slab* slab) {
    struct slab* curr = slab;

    while (curr != NULL) {
        printf("| SIZE = %ld, FREE_SLOTS = %ld| -> ", curr->size_class, curr->free_count);
        curr = curr->next;
    }
    printf("\n");
}

void print_all_slabs() {
    for (int i = 0; i < 8; i++)
        print_slab(fast_caches[i]);
}
/* Heap utility functions for large bin. */
struct large_chunk* next_physical_chunk(struct large_chunk* chunk) {
    struct large_chunk* next = (struct large_chunk*)((char*)chunk + sizeof(struct large_chunk) + chunk->size);

    if (chunk_in_span(chunk, next))
        return next;

    return NULL;
}

struct large_chunk* prev_physical_chunk(struct large_chunk* chunk) {
    struct large_chunk* prev = (struct large_chunk*)((char*)chunk - sizeof(struct large_chunk) - chunk->prev_size);

    if (chunk_in_span(chunk, prev))
        return prev;

    return NULL;
}

int chunk_in_span(struct large_chunk* ref_chunk, struct large_chunk* adj_chunk) {

    return (void*)adj_chunk >= (void*)ref_chunk->span.start && (void*)adj_chunk < (void*)ref_chunk->span.end;
}

void unlink_large_free_chunk(struct large_chunk* chunk) {
    if (chunk->bk == NULL) // In the case that it is the first chunk.
        *large_chunk_bin = chunk->fd;
    if (chunk->fd != NULL)
        chunk->fd->bk = chunk->bk; // this is the only one required since the chunk is at the beginning of the free list.
    if (chunk->bk != NULL)
        chunk->bk->fd = chunk->fd;
    chunk->allocated = 1;
}

void insert_large_free_chunk(struct large_chunk* chunk) {
    if (*large_chunk_bin != NULL) 
        (*large_chunk_bin)->bk = chunk;

    chunk->fd = *(large_chunk_bin);
    chunk->bk = NULL;
    *large_chunk_bin = chunk;
}

void print_large_bin_contents() {
    struct large_chunk* curr = *large_chunk_bin;

    while (curr != NULL) {
        printf(" | size = %ld, next = %p | -> ", curr->size, curr->fd);

        curr = curr->fd;
    }


    printf(" NULL\n");
}

/* 
    _metadata_alloc() allocates memory space
    for heap metadata and related data structures.
*/
char* _metadata_alloc(int bytes) {
    char* metadata_return = metadata_current;
    metadata_current += bytes;

    return metadata_return;
}

/* 
    _heap_init() returns 0 on success and -1 on error.
*/
int _heap_init()
{
    // Allocate the metadata region of the heap.
    metadata_start = (char*)mmap(
        NULL, 
        4096,
        PROT_READ | PROT_WRITE,
        MAP_PRIVATE | MAP_ANON,
        -1, 
        0
    );

    // Check if mmap worked./
    if (metadata_start == MAP_FAILED) {
        perror("mmap: MAP FAILED");
        return -1; 
    }

    // Pointers for managing heap metadata's memory.
    metadata_end = metadata_start + 4096;
    metadata_current = metadata_start;

    // Allocate fast chunk bins on the heap.
    fast_caches = (struct slab**)_metadata_alloc(8 * sizeof(struct slab*));

    //_allocate_fast_bin_page(16, &fast_chunk_bins[0]);
    //print_list(fast_chunk_bins[0]);

    // Allocate large chunk bins on the heap.
    large_chunk_bin = (struct large_chunk**)_metadata_alloc(sizeof(struct large_chunk*));

    initialize_hash_map();

    return 0;
}

/*
    _allocate_large_bin_chunk() allocates and initializes a new large bin chunk
    and places at the beginning of the large bin.

    List before:
    Large bin --> chunk1 <--> chunk2 --> NULL
    
    List After:
    Large bin --> new chunk <--> chunk1 <--> chunk2 --> NULL

    This is done because it is simplest, but I may opt for a more secure method later. 
*/
struct large_chunk* _allocate_large_bin_chunk(int size, struct large_chunk** bin) 
{
    size += sizeof(struct large_chunk);
    char* new_page = (char*)mmap(
        NULL, 
        size, 
        PROT_READ | PROT_WRITE, 
        MAP_PRIVATE | MAP_ANON, 
        -1,
        0
    );

    // I need to turn this into a valid chunk, first I need to round up to the next page size, 
    struct large_chunk* new_chunk = (struct large_chunk*)new_page;
    new_chunk->size = round_to_nearest_page(size) - sizeof(struct large_chunk);
    
    if ((*bin) != NULL)
        (*bin)->bk = new_chunk;

    new_chunk->allocated = 0;
    new_chunk->fd = *bin;
    new_chunk->bk = NULL;
    new_chunk->span.start = new_page;
    new_chunk->span.end = (char*)new_page + new_chunk->size + sizeof(struct large_chunk);
    new_chunk->prev_size = -1;


    *bin = new_chunk;

    return new_chunk;
}

/*
    _search_large_bin() goes through the large bin to
*/
struct large_chunk* _search_large_bin_first_fit(int size) {
    struct large_chunk* curr = *large_chunk_bin;

    // I'll start with first fit for simplicity.
    while (curr != NULL) {
        if (curr->allocated == 0 && curr->size >= size) {
            return curr;
        }
        curr = curr->fd;
    }

    return NULL;
}

/*
    _split_large_chunk() will modify chunk such that 
    size bytes are split off of it. 

    chunk is modified with a new size. 
*/
void _split_large_chunk(struct large_chunk* chunk, int size) {
    // We subtract the size of the chunk we want
    // Then we subtract the size of a header because
    // the new chunk will have to have a header added to it as well.
    // chunk->size is used because we count up to "size" bytes starting from the user data section.
    if (chunk->size - size < sizeof(struct large_chunk)) 
        return;

    size_t new_size = chunk->size - size; 

    char* free_space = ((char*)chunk + sizeof(struct large_chunk));
    struct large_chunk* split_chunk = (struct large_chunk*)(free_space + size);

    split_chunk->fd = chunk->fd;
    split_chunk->bk = chunk;
    split_chunk->allocated = 0;
    split_chunk->prev_size = size;
    split_chunk->span = chunk->span;
    split_chunk->size = new_size - sizeof(struct large_chunk);

    chunk->fd = split_chunk; 
    chunk->size = size;
    chunk->allocated = 0; 

    // Update next chunk in doubly linked list.
    if (split_chunk->fd != NULL) 
        split_chunk->fd->bk = split_chunk;

    // Update physical next chunk
    struct large_chunk *next = next_physical_chunk(split_chunk);

    if (next != NULL) 
        next->prev_size = split_chunk->size;
    
    
}

/*
    _merge_two_large_chunks() 

    Assuming chunk1 at a lower address than chunk2
*/
void _merge_two_large_chunks(struct large_chunk* chunk1, struct large_chunk* chunk2) {
    
    // UPDATE RELEVANT METADATA
    chunk1->size += chunk2->size + sizeof(struct large_chunk); // We can now use the header from chunk2 as free space
    struct large_chunk* next = (struct large_chunk*)((char*)chunk1 + sizeof(struct large_chunk) + chunk1->size);

    if ((char*)next < (char*)chunk1->span.end) {
        next->prev_size = chunk1->size;
    }

    if (chunk2->bk != NULL) {
        chunk2->bk->fd = chunk2->fd;
    } else {
        *large_chunk_bin = chunk2->fd;
    }

    if (chunk2->fd != NULL) {
        chunk2->fd->bk = chunk2->bk;
    }

    chunk2->fd = NULL;
    chunk2->bk = NULL;

    // It may be beneifical to zero out chunk 2's metadata at some point,
    chunk1->allocated = 0;
}

/*
    _allocate_fast_page() is a function called to get more memory
    when a given fast bin is empty (there are no more chunks). 

    One nice thing about pushing the new page to the head is that 
    when performing a linear search you can actually get a free page faster. 

    Actually, we may not even need a linear search since if the first bin isnt free
    then none of the bins should be free.
*/
void _allocate_fast_page(int size_class, struct slab** cache) 
{
    // Allocate a new page for whichever bin we are dealing with.
    char* new_page = (char*)mmap(
        NULL, 
        4096, 
        PROT_READ | PROT_WRITE, 
        MAP_PRIVATE | MAP_ANON, 
        -1, 
        0
    );

    // Fill in the header.
    void* next_slab = (*cache);

    // Set the metadata
    struct slab metadata = { 0 };
    metadata.base = new_page;
    metadata.size_class = size_class;
    metadata.slot_count = PAGE_SIZE_BYTES / size_class;
    metadata.free_count = metadata.slot_count;
    metadata.next = next_slab;
    metadata.prev = NULL;


    if (next_slab != NULL) {
        struct map_entry* next_slab_entry = addr_map_lookup((uintptr_t)next_slab);
        if (next_slab_entry != NULL) next_slab_entry->value.prev = (struct slab*)new_page;
    }

    // Insert the metadata in the hash map
    addr_map_insert((uintptr_t)new_page, metadata);

    *cache = (struct slab*)new_page;
}

void _unlink_slab(struct slab** cache, struct slab* slab) {
    // I need to make this into a doubly linked list.
    if (slab->prev == NULL) 
        *cache = slab->next;

    struct map_entry* next_slab_entry = addr_map_lookup((uintptr_t)slab->next);
    struct map_entry* prev_slab_entry = addr_map_lookup((uintptr_t)slab->prev);


    if (next_slab_entry != NULL) next_slab_entry->value.prev = slab->prev;
    if (prev_slab_entry != NULL) prev_slab_entry->value.next = slab->next; 

    slab->next = NULL;
    slab->prev = NULL;
}

void _push_slab(struct slab* slab) {
    // Determine its size class. 
    size_t size_class = slab->size_class;

    slab->next = slab->prev = NULL;

    struct slab** cache = &(fast_caches[get_fast_chunk_index(size_class)]);

    struct slab* next_slab = (*cache);

    if (next_slab != NULL) {
        struct map_entry* next_slab_entry = addr_map_lookup((uintptr_t)next_slab);
        next_slab_entry->value.prev = slab->base; 
    }

    slab->next = next_slab;
    *cache = (struct slab*)slab->base;
}

/*
    _slab_alloc()

    USE A BYTEMAP. DO NOT USE A BITMAP. IT WILL MAKE YOUR LIFE FAR EASIER
    THEN COME BACK AND USE A BITMAP.

    I'm wondering if this even accounts for the case where 
*/
void* _slab_alloc(struct slab** cache) {
    int i = 0;

    struct map_entry* map_entry = addr_map_lookup((uintptr_t)*cache);
    struct slab* free_slab = &(map_entry->value);


    for (; i < free_slab->slot_count; i++) {
        if (free_slab->alloc_bytemap[i] == 0)
            break;
    }


    if (i == free_slab->slot_count) // Prevents silent out of bounds writes if free_count gets out of sync.
        return NULL;

   
    free_slab->alloc_bytemap[i] = 0xff;
    free_slab->free_count--;


    if (free_slab->free_count == 0) {
        _unlink_slab(cache, free_slab); 
    }

    void* slot_address = (char*)free_slab->base + (free_slab->size_class * i);

    return slot_address;
}

/*
    Function used for debugging.
*/
void print_list(struct fast_chunk* hd) 
{
    struct fast_chunk* curr = hd;
    int counter = 1;
    while (curr != NULL) {
        curr = curr->fd;
        counter++;  
    }
}

/*
    Allocates some heap memory for the user.

    OK I think I need a function for something. 
*/
void* heap_alloc(size_t bytes) 
{
    if (!heap_initialized) {
        heap_initialized = 1;
        _heap_init();
    }
    // Determining the correct size class.
    int size_class = determine_size_class(bytes); 

    if (size_class != -1) {
        struct slab** cache = &(fast_caches[get_fast_chunk_index(size_class)]);
    
        struct map_entry* entry = addr_map_lookup((uintptr_t)*cache);


        /* THERE MUST BE SOME ADDITIONAL LOGIC HERE TO FIND A FREE CACHE, EVEN IF THE ONE AT THE BEGINNING ISNT FREE, THERE COULD BE A FREE SLOT IN A LATER CACHE 
           I CAN MAKE A HELPER FOR THIS */
        if ((entry == NULL) || (entry->value.free_count == 0)) {
            _allocate_fast_page(size_class, cache); 
        }
        
        void* pointer = _slab_alloc(cache); 

        return pointer; 

    } else {
        // Search for a valid size large chunk.
        struct large_chunk* chunk = _search_large_bin_first_fit(bytes); // Looks ok 

        if (chunk == NULL) {
            chunk = _allocate_large_bin_chunk(bytes, large_chunk_bin); 
        } 

        _split_large_chunk(chunk, bytes); // chunk is now the correct size

        unlink_large_free_chunk(chunk);

        return (void*)((char*)chunk + sizeof(struct large_chunk));
    }
}

/*
    heap_free() is an interface for the user to free heap memory.
*/
void heap_free(void* ptr) 
{
    // Take out the ptr last 3 bits
    uintptr_t page_base = (uintptr_t)ptr & ~0xFFF;
    uintptr_t slot_start = (uintptr_t)ptr & 0xFFF;

    struct map_entry* entry = addr_map_lookup((uintptr_t)page_base);

    if (entry == NULL)  // This could also mean its a lerge chunk, so later we'll have to make this check pertian to that case. 
        return;

    size_t size = entry->value.size_class;

    if (size <= 2048) {
        int bytemap_index = slot_start / size;
        entry->value.alloc_bytemap[bytemap_index] = 0x00;
        entry->value.free_count++;

        if (entry->value.free_count == 1) {
            _push_slab(&(entry->value));
        }

    } else {
        struct large_chunk* large_chunk = (struct large_chunk*)ptr - 1;

        //struct large_chunk* forward_adj_chunk = (struct large_chunk*)((char*)large_chunk + sizeof(struct large_chunk) + large_chunk->size);
        struct large_chunk* forward_adj_chunk = next_physical_chunk(large_chunk);

        // We subtract sizeof(struct large chunk) twice because we need to get to the BEGINNING of the previous chunk.
        struct large_chunk* backward_adj_chunk = prev_physical_chunk(large_chunk);

        // insert_large_free_chunk
        insert_large_free_chunk(large_chunk);
                

        int merge_occurred = 0; 
        if (forward_adj_chunk != NULL && forward_adj_chunk->allocated == 0) {
            merge_occurred = 1;
            _merge_two_large_chunks(large_chunk, forward_adj_chunk); // I think it matters the order in which you merge the chunks. Whichever has the lower memory address should be the one that stays in the list.
        }
        
        if (backward_adj_chunk != NULL && backward_adj_chunk->allocated == 0) {            
            merge_occurred = 1;
            _merge_two_large_chunks(backward_adj_chunk, large_chunk);
        }

        if (!merge_occurred) large_chunk->allocated = 0;

    }
}