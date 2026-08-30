#include "slab_quarantine.h"
#include "secure_utils.h"
#include "heap_stats.h"

struct metadata_attributes meta_attributes;
struct quarantine_queue slot_quarantine;

static struct quarantine_entry* _metadata_alloc() {
    static struct quarantine_entry* entry_ptr;
    entry_ptr = meta_attributes.curr;

    meta_attributes.curr += 1;
    if (meta_attributes.curr >= meta_attributes.usable_end) {
        meta_attributes.curr = meta_attributes.usable_start;
    }

    return entry_ptr;
}

void initialize_quarantine_queue() {
    meta_attributes.region = create_guarded_region(QUEUE_CAPACITY * sizeof(struct quarantine_entry), false);
    heap_stats_add_metadata_mapping(meta_attributes.region.total_size); 
    meta_attributes.usable_start = meta_attributes.region.usable_ptr;
    meta_attributes.usable_end = meta_attributes.usable_start + QUEUE_CAPACITY;
    meta_attributes.curr = meta_attributes.usable_start;

    slot_quarantine.head = NULL; 
    slot_quarantine.tail = NULL;
    slot_quarantine.length = 0;
}

/* 
    quarantine_enqueue() adds a slot to the quarantine.
    When a slot is inside the quarantine it is no longer usable.
*/
void quarantine_enqueue(void* slot_base) {
    if (slot_quarantine.length >= QUEUE_CAPACITY)
        return;

    struct quarantine_entry* entry = _metadata_alloc(); 
    entry->slot_base = slot_base; 

    slot_quarantine.length += 1;
    entry->next = slot_quarantine.head;
    entry->prev = NULL;


    if (slot_quarantine.head != NULL) {
        slot_quarantine.head->prev = entry;    
    } 
    
    slot_quarantine.head = entry;
    if (slot_quarantine.length == 1) 
        slot_quarantine.tail = slot_quarantine.head;

}

/*
    quarantine_deqeuue() removes a slot from the quarantine.

    This only begins at a length of 30. Each time a slot is freed, 
    it is enqueued into the quarantine. A slot is only released from
    the quarantine after heap_free() has been called 30 times. 

    Therefore, the quarantine must be completely full before any 
    slot is ready to be freed. 
*/
void* quarantine_dequeue() {
    if (slot_quarantine.length < 30)
        return NULL;

    struct quarantine_entry* entry = slot_quarantine.tail; 

    if (entry->prev != NULL) 
        entry->prev->next = NULL;
    slot_quarantine.tail = entry->prev;
    entry->prev = NULL;

    slot_quarantine.length -= 1;

    if (slot_quarantine.length == 0)
        slot_quarantine.head = NULL;

    return entry->slot_base;
}
