#define _GNU_SOURCE

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
#include "slab_metadata_allocator.h"
#include "heap_stats.h"

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

    metadata_start = (char*)mmap(
        NULL, 
        4096,
        PROT_READ | PROT_WRITE,
        MAP_PRIVATE | MAP_ANON,
        -1, 
        0
    );

    heap_stats_add_metadata_mapping(4096);

    if (metadata_start == MAP_FAILED) {
        perror("mmap: MAP FAILED");
        return -1; 
    }

    metadata_end = metadata_start + 4096;
    metadata_current = metadata_start;

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
    _allocate_fast_page() creates a slab for a particular size class. 

    The metadata for the slab is stored in a separate memory region
    than the slab itself.
*/
void _allocate_fast_page(int size_class, struct slab** cache) 
{
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


    heap_stats_add_slab_mapping(page_size);

    void* next_slab = (*cache);

    // Constructing the metadata.
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

    // A shuffled free-index stack is created to dictate which order the slots will be 
    // allocated in.
    fisher_yates_shuffle(metadata.indicies, metadata.slot_count);

    // The constructed metadata is inserted into a separate memory region.
    struct slab_metadata_slot* ptr = insert_slab_metadata(&metadata);

    if (next_slab != NULL) {
        struct map_entry* next_slab_entry = addr_map_lookup(SMALL, (uintptr_t)next_slab);
        if (next_slab_entry != NULL) next_slab_entry->value.slab->slab.prev = (struct slab*)new_page;
    }

    struct map_value map_value = construct_map_value(ptr, NULL);
    if (!addr_map_insert(SMALL, (uintptr_t)new_page, map_value)) 
        _exit(127);

    // The new slab is pushed to the front of its respective cache.
    *cache = (struct slab*)new_page;
}

void _unlink_slab(struct slab** cache, struct slab* slab) {
    if (slab->prev == NULL) 
        *cache = slab->next;

    struct map_entry* next_slab_entry = addr_map_lookup(SMALL, (uintptr_t)slab->next);
    struct map_entry* prev_slab_entry = addr_map_lookup(SMALL, (uintptr_t)slab->prev);


    if (next_slab_entry != NULL) next_slab_entry->value.slab->slab.prev = slab->prev;
    if (prev_slab_entry != NULL) prev_slab_entry->value.slab->slab.next = slab->next; 

    slab->next = NULL;
    slab->prev = NULL;
}

/*
    _push_slab() adds a slab back to the available
    slab list for its size class.

    The indicies field is a stack that speciifes the order in which slots
    are allocated.

    The free_top field indicates the top of the stack and indicates  which
    slot should be allocated next.
*/
void _push_slab(struct slab* slab) {
    size_t size_class = slab->size_class;

    slab->next = slab->prev = NULL;

    struct slab** cache = &(fast_caches[get_slab_cache_index(size_class)]);

    struct slab* next_slab = (*cache);

    if (next_slab != NULL) {
        struct map_entry* next_slab_entry = addr_map_lookup(SMALL, (uintptr_t)next_slab);
        next_slab_entry->value.slab->slab.prev = slab->base; 
    }

    slab->next = next_slab;
    *cache = (struct slab*)slab->base;
}

/*
    _slab_alloc() returns the address of a slot in a particular slab.
*/
void* _slab_alloc(struct slab** cache, struct map_entry* map_entry) {
    struct slab* free_slab = &map_entry->value.slab->slab;

    int bit_number = free_slab->indicies[free_slab->free_top];
    if (free_slab->free_top < free_slab->slot_count) free_slab->free_top = free_slab->free_top + 1;

    set_allocated(free_slab->alloc_bitmap, bit_number);
    free_slab->free_count--;


    // If a slab is completely full, it is removed
    // from the linked list.
    if (free_slab->free_count == 0) {
        _unlink_slab(cache, free_slab); 
    }

    void* slot_address = (char*)free_slab->base + (free_slab->size_class * bit_number);

    return slot_address;
}

/*
    heap_alloc() splits the allocation path between small
    allocations and large allocations.

    A slab allocator handles small allocations while a large allocation
    request results in a guarded mmap-ed region being returned to the caller.

    Small are rounded to one of eight power-of-two size classes 
    ranging from 16 B - 2048 B. 
*/
void* heap_alloc(size_t bytes) 
{
    if (!heap_initialized) {
        if (_heap_init() == -1)
            _exit(127);

        heap_initialized = 1;
    }

    int size_class = determine_size_class(bytes); 

    if (size_class != -1) {
        struct slab** cache = &(fast_caches[get_slab_cache_index(size_class)]);

        struct map_entry* entry = addr_map_lookup(SMALL, (uintptr_t)*cache);

        if ((entry == NULL) || (entry->value.slab->slab.free_count == 0)) {
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
    heap_free() frees a pointer returned from heap_alloc() 
    that has not been freed since the call to heap_alloc().

    For slab bitmaps: 
    0 --> not allocated to the user (could either be free or quarantined)
    1 --> allocated to the user
*/
void heap_free(void* ptr) 
{
    if (ptr == NULL)
        return;

    long page_size = sysconf(_SC_PAGESIZE);
    if (page_size == -1)
        _exit(127); 

    
    uintptr_t page_mask = ((uintptr_t)sysconf(_SC_PAGESIZE) - 1);

    // For small allocations, we use the page mask to retrieve the slab's base address
    // from the returned slot address.
    uintptr_t page_base = (uintptr_t)ptr & ~page_mask;
    uintptr_t slot_start = (uintptr_t)ptr & page_mask;

    bool is_large = false;

    struct map_entry* entry = addr_map_lookup(SMALL, (uintptr_t)page_base);
    if (entry == NULL) {
        entry = addr_map_lookup(LARGE, (uintptr_t)ptr); 
        is_large = true;
    }

    // If after both address map lookups it is not found
    // the process is terminated because the free is invalid.
    if (entry == NULL)  
        _handle_invalid_free();

    if (!is_large) {
        size_t size = entry->value.slab->slab.size_class;

        if (slot_start % size != 0) 
            _handle_invalid_free();

        int bit_number = slot_start / size;

        // Reject a double free if the slot is no longer marked as allocated.
        if (check_free(entry->value.slab->slab.alloc_bitmap, bit_number))
            _handle_invalid_free();

        // Dequeue a slot from the quarantine to make space 
        // for the slot that has just been freed. 
        // The dequeued slot is out of the quarantine
        // and can be made available to the user.
        void* dq_entry = quarantine_dequeue(); 

        explicit_bzero(ptr, size);

        // The current slot is set to unallocated and is put 
        // into the quarantine. 
        set_free(entry->value.slab->slab.alloc_bitmap, bit_number);
        quarantine_enqueue(ptr);

        if (dq_entry == NULL) 
            return;

        // From the dequeued slot, the page mask is used to get the 
        // base address of its slab.
        uintptr_t dq_page_base = (uintptr_t)dq_entry & ~page_mask;
        uintptr_t dq_slot_start = (uintptr_t)dq_entry & page_mask;

        // Update the dequeued slot's slab's metadata.
        struct map_entry* dq_map_entry = addr_map_lookup(SMALL, dq_page_base);
        size_t dq_size = dq_map_entry->value.slab->slab.size_class;
        int dq_bit_number = dq_slot_start / dq_size;

        dq_map_entry->value.slab->slab.free_count += 1;

        dq_map_entry->value.slab->slab.free_top--;
        int free_slot = dq_map_entry->value.slab->slab.free_top;

        dq_map_entry->value.slab->slab.indicies[free_slot] = dq_bit_number; 
        
        /* 
         If the slab was previously full and now has an empty slot (free_count == 1),
         it is added back to its available slab list.

         If the slab is completely empty it is unlinked and its metadata
         is destroyed.
        */ 
        if (dq_map_entry->value.slab->slab.free_count == 1) {
            _push_slab(&dq_map_entry->value.slab->slab);
        } else if (dq_map_entry->value.slab->slab.free_count == dq_map_entry->value.slab->slab.slot_count) {
            struct slab** cache = &(fast_caches[get_slab_cache_index(dq_map_entry->value.slab->slab.size_class)]);

            struct slab_metadata_slot* metadata_slot = dq_map_entry->value.slab;
            void* slab_base = metadata_slot->slab.base;
            uintptr_t map_key = dq_map_entry->key;

            _unlink_slab(cache, &metadata_slot->slab);
            delete_entry(SMALL, map_key);

            if (munmap(slab_base, page_size) == -1)
                _exit(127);
            delete_slab_metadata(metadata_slot);

            heap_stats_remove_slab_mapping(page_size);
        }

    } else {
        hardened_large_free(ptr);
    }
}