#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>

#include "secure_utils.h"
#include "slab_metadata_allocator.h"
#include "heap_stats.h"

struct slab_metadata_arena* head;

static void _create_arena() {

    size_t full_region_size = sizeof(struct slab_metadata_arena) + 16 * sizeof(struct slab_metadata_slot);
    struct guarded_region region = create_gaurded_region(full_region_size, false);

    heap_stats_add_metadata_mapping(region.total_size);

    struct slab_metadata_arena* metadata = (struct slab_metadata_arena*)region.usable_ptr;

    metadata->region = region;
    metadata->next = head;
    metadata->prev = NULL;

    if (head != NULL)
        head->prev = region.usable_ptr;

    head = region.usable_ptr;

    struct slab_metadata_slot* slots_start = (struct slab_metadata_slot*)((char*)metadata + sizeof(struct slab_metadata_arena));

    struct slab_metadata_slot* curr = slots_start;
    for (int i = 0; i < 16 ; i++) {
        curr->owner = metadata;
        
        curr = curr + 1;
    }
}

static void mark_occupied(uint16_t* bitmap, int bit_index) {
    uint16_t bitmask = UINT16_C(1) << bit_index;

    *bitmap |= bitmask; 
}

static void mark_free(uint16_t* bitmap, int bit_index) {
    uint16_t bitmask = UINT16_C(1) << bit_index;

    *bitmap &= ~bitmask; 
}

static int find_free_index(uint16_t bitmap) {
    if (bitmap == 0XFFFF) return -1; 

    uint16_t inverted = (uint16_t)~bitmap; 

    return __builtin_ctz(inverted);
}

static void push_slab_arena(struct slab_metadata_arena* arena) {
    arena->next = head;

    if (head != NULL)
        head->prev = arena;

    head = arena;
}

static void unlink_slab_arena(struct slab_metadata_arena* arena) {
    if (arena == head) 
        head = arena->next;

    if (arena->prev != NULL) arena->prev->next = arena->next;
    if (arena->next != NULL) arena->next->prev = arena->prev;

    arena->prev = arena->next = NULL;
}

struct slab_metadata_slot* insert_slab_metadata(struct slab* slab_metadata) {

    if (head == NULL) 
        _create_arena();
    
    int bit_index = find_free_index(head->bitmap);

    if (bit_index == -1)
        _exit(127);

    struct slab_metadata_slot* slot_array_start = (struct slab_metadata_slot*)((char*)head + sizeof(struct slab_metadata_arena));

    struct slab_metadata_slot* free_slot = slot_array_start + bit_index; 

    free_slot->slab = *slab_metadata;

    mark_occupied(&(head->bitmap), bit_index);

    if (head->bitmap == 0xFFFF) {
        struct slab_metadata_arena* next = head->next;
        if (head->next != NULL) head->next->prev = NULL;
        head->next = NULL;
        head->prev = NULL;
        
        head = next;
    }

    return free_slot;
}

void delete_slab_metadata(struct slab_metadata_slot* metadata_ptr) {
    struct slab_metadata_arena* arena_metadata = metadata_ptr->owner; 

    int bit_index = ((uintptr_t)metadata_ptr - (uintptr_t)arena_metadata) / sizeof(struct slab_metadata_slot);

    mark_free(&(arena_metadata->bitmap), bit_index);

    if (arena_metadata->bitmap == 0) { 
        unlink_slab_arena(arena_metadata);
        heap_stats_remove_metadata_mapping(arena_metadata->region.total_size);
        destroy_guarded_region(&(arena_metadata->region));
    } else if (arena_metadata->next == NULL && arena_metadata->prev == NULL && head != arena_metadata) {
        push_slab_arena(arena_metadata);
    }
}