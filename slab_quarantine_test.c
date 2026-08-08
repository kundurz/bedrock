#include "slab_quarantine.h"

#include <assert.h>
#include <stdio.h>

int main(void)
{
    int entries[QUEUE_CAPACITY + 2];

    initialize_quarantine_queue();

    /* Dequeuing an empty queue is harmless. */
    assert(quarantine_dequeue() == NULL);

    /* Entries can be enqueued, dequeued in FIFO order, and drained. */
    quarantine_enqueue(&entries[0]);
    quarantine_enqueue(&entries[1]);
    quarantine_enqueue(&entries[2]);
    assert(quarantine_dequeue() == &entries[0]);
    assert(quarantine_dequeue() == &entries[1]);
    assert(quarantine_dequeue() == &entries[2]);
    assert(quarantine_dequeue() == NULL);

    /* Once full, additional entries are ignored. */
    for (size_t i = 0; i < QUEUE_CAPACITY; i++)
        quarantine_enqueue(&entries[i]);
    quarantine_enqueue(&entries[QUEUE_CAPACITY]);
    quarantine_enqueue(&entries[QUEUE_CAPACITY + 1]);

    for (size_t i = 0; i < QUEUE_CAPACITY; i++)
        assert(quarantine_dequeue() == &entries[i]);
    assert(quarantine_dequeue() == NULL);

    /* A drained queue can be used again. */
    quarantine_enqueue(&entries[QUEUE_CAPACITY]);
    assert(quarantine_dequeue() == &entries[QUEUE_CAPACITY]);
    assert(quarantine_dequeue() == NULL);

    puts("[OK] slab quarantine handles empty, full, and reused queues");
    return 0;
}
