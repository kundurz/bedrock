#pragma once

#include <stdlib.h>
#include "secure_utils.h"

#define QUEUE_CAPACITY 30

struct metadata_attributes {
    struct guarded_region region;

    struct quarantine_entry* curr;

    struct quarantine_entry* usable_start;
    struct quarantine_entry* usable_end;
};

struct quarantine_queue {
    struct quarantine_entry* head;
    struct quarantine_entry* tail;   

    size_t length;
};

struct quarantine_entry {
    void* slot_base;

    struct quarantine_entry* prev; 
    struct quarantine_entry* next;
};

/* Interfaces */
void initialize_quarantine_queue();
void quarantine_enqueue(void* slot_base); 
void* quarantine_dequeue(); 
