#define _GNU_SOURCE
#define PAGE_SIZE_BYTES 4096

#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <stddef.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>
#include "heap_internal.h"
#include "utils.h"
#include "addr_map.h"
#include "ring_cache.h"
#include "large_allocations.h"
#include "slab_quarantine.h"
#include "secure_utils.h"

struct slab** fast_caches;

char* metadata_start;
char* metadata_current;
char* metadata_end;

int heap_initialized = 0;

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

    long page_size = sysconf(_SC_PAGESIZE);
    if (page_size == -1)
        return -1;
    
    // MAX page size supported is 16KiB
    // divide by 16 because thats the smallest size class.
    if ((size_t)page_size / 16 > MAX_SLAB_SLOTS)
        _exit(127);

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

    if (initialize_hash_map(SMALL) == -1) 
        _exit(127);

    if (initialize_hash_map(LARGE) == -1)
        _exit(127);

    if (initialize_ring_cache() == -1)
        _exit(127); 

    initialize_quarantine_queue();

    return 0;
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
    long page_size = sysconf(_SC_PAGESIZE);
    if (page_size == -1)
        _exit(127);

    char* new_page = (char*)mmap(
        NULL, 
        page_size, 
        PROT_READ | PROT_WRITE, 
        MAP_PRIVATE | MAP_ANON, 
        -1, 
        0
    );

    if (new_page == MAP_FAILED) 
        _exit(127);

    // Fill in the header.
    void* next_slab = (*cache);

    // Set the metadata
    struct slab metadata = { 0 };
    metadata.base = new_page;
    metadata.size_class = size_class;

    size_t slot_count = (size_t)page_size / (size_t)size_class;

    if (slot_count > MAX_SLAB_SLOTS)
        _exit(127);

    metadata.slot_count = (uint16_t)slot_count; 

    metadata.free_count = metadata.slot_count;
    metadata.next = next_slab;
    metadata.prev = NULL;
    metadata.free_top = 0;

    // Generate indicies using fisher-yates shuffle.
    fisher_yates_shuffle(metadata.indicies, metadata.slot_count);

    if (next_slab != NULL) {
        struct map_entry* next_slab_entry = addr_map_lookup(SMALL, (uintptr_t)next_slab);
        if (next_slab_entry != NULL) next_slab_entry->value.slab.prev = (struct slab*)new_page;
    }

    // Insert the metadata in the hash map
    struct map_value map_value = construct_map_value(&metadata, NULL);

    if (!addr_map_insert(SMALL, (uintptr_t)new_page, map_value)) 
        _exit(127);

    *cache = (struct slab*)new_page;
}

void _unlink_slab(struct slab** cache, struct slab* slab) {
    // I need to make this into a doubly linked list.
    if (slab->prev == NULL) 
        *cache = slab->next;

    struct map_entry* next_slab_entry = addr_map_lookup(SMALL, (uintptr_t)slab->next);
    struct map_entry* prev_slab_entry = addr_map_lookup(SMALL, (uintptr_t)slab->prev);


    if (next_slab_entry != NULL) next_slab_entry->value.slab.prev = slab->prev;
    if (prev_slab_entry != NULL) prev_slab_entry->value.slab.next = slab->next; 

    slab->next = NULL;
    slab->prev = NULL;
}

void _push_slab(struct slab* slab) {
    // Determine its size class. 
    size_t size_class = slab->size_class;

    slab->next = slab->prev = NULL;

    struct slab** cache = &(fast_caches[get_slab_cache_index(size_class)]);

    struct slab* next_slab = (*cache);

    if (next_slab != NULL) {
        struct map_entry* next_slab_entry = addr_map_lookup(SMALL, (uintptr_t)next_slab);
        next_slab_entry->value.slab.prev = slab->base; 
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
void* _slab_alloc(struct slab** cache, struct map_entry* map_entry) {
    struct slab* free_slab = &(map_entry->value.slab);

    int bit_number = free_slab->indicies[free_slab->free_top];
    if (free_slab->free_top < free_slab->slot_count) free_slab->free_top = free_slab->free_top + 1;

    set_allocated(free_slab->alloc_bitmap, bit_number);
    free_slab->free_count--;


    if (free_slab->free_count == 0) {
        _unlink_slab(cache, free_slab); 
    }

    void* slot_address = (char*)free_slab->base + (free_slab->size_class * bit_number);

    return slot_address;
}

/*
    Allocates some heap memory for the user.
*/
void* heap_alloc(size_t bytes) 
{
    if (!heap_initialized) {
        if (_heap_init() == -1)
            _exit(127);

        heap_initialized = 1;
    }

    // Determining the correct size class.
    int size_class = determine_size_class(bytes); 

    if (size_class != -1) {
        struct slab** cache = &(fast_caches[get_slab_cache_index(size_class)]);
    
        struct map_entry* entry = addr_map_lookup(SMALL, (uintptr_t)*cache);

        if ((entry == NULL) || (entry->value.slab.free_count == 0)) {
            _allocate_fast_page(size_class, cache); 
            entry = addr_map_lookup(SMALL, (uintptr_t)*cache);
        }
        
        void* pointer = _slab_alloc(cache, entry); 

        return pointer; 

    }  
    
    return hardened_large_alloc(bytes);
}

void _handle_invalid_free() {
    _exit(127); 
}

/*
    heap_free() is an interface for the user to free heap memory.
*/
void heap_free(void* ptr) 
{
    if (ptr == NULL)
        return;

    long page_size = sysconf(_SC_PAGESIZE);
    if (page_size == -1)
        _exit(127); 

    uintptr_t page_mask = ((uintptr_t)sysconf(_SC_PAGESIZE) - 1);

    uintptr_t page_base = (uintptr_t)ptr & ~page_mask;
    uintptr_t slot_start = (uintptr_t)ptr & page_mask;

    bool is_large = false;

    struct map_entry* entry = addr_map_lookup(SMALL, (uintptr_t)page_base);
    if (entry == NULL) {
        entry = addr_map_lookup(LARGE, (uintptr_t)ptr); // The regions must be inserted into the hash map using mmap baes. 
        is_large = true;
    }

    // The attacker has attempted to free an invalid chunk.
    if (entry == NULL)  
        _handle_invalid_free();

    if (!is_large) {
        size_t size = entry->value.slab.size_class;

        if (slot_start % size != 0) 
            _handle_invalid_free();

        int bytemap_index = slot_start / size;
        int bit_number = slot_start / size;

        if (check_free(entry->value.slab.alloc_bitmap, bit_number))
            _handle_invalid_free();


        void* dq_entry = quarantine_dequeue(); 

        explicit_bzero(ptr, size);
        set_free(entry->value.slab.alloc_bitmap, bit_number);
        quarantine_enqueue(ptr);

        if (dq_entry == NULL) 
            return;

        uintptr_t dq_page_base = (uintptr_t)dq_entry & ~page_mask;
        uintptr_t dq_slot_start = (uintptr_t)dq_entry & page_mask;

        struct map_entry* dq_map_entry = addr_map_lookup(SMALL, dq_page_base);
        size_t dq_size = dq_map_entry->value.slab.size_class;
        int dq_bit_number = dq_slot_start / dq_size;


        dq_map_entry->value.slab.free_count += 1;

        dq_map_entry->value.slab.free_top--;
        int free_slot = dq_map_entry->value.slab.free_top;

        dq_map_entry->value.slab.indicies[free_slot] = dq_bit_number; 
        
        // This is gonna be for the slab that was freed.
        if (dq_map_entry->value.slab.free_count == 1) {
            _push_slab(&(dq_map_entry->value.slab));
        }

    } else {
        hardened_large_free(ptr);
    }
}