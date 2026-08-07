#include "addr_map.h"

#include <assert.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>

#define ENTRY_COUNT 6
#define MAX_INITIALIZATION_ATTEMPTS 128

struct inserted_value {
    uintptr_t key;
    struct slab metadata;
};

static struct inserted_value inserted[ENTRY_COUNT];

static void populate_map(unsigned int attempt)
{
    assert(initialize_hash_map(SMALL) == 0);

    for (size_t i = 0; i < ENTRY_COUNT; i++) {
        uintptr_t sequence = (uintptr_t)(attempt * ENTRY_COUNT + i + 1);

        inserted[i].key = UINT64_C(0x10000000) + sequence * UINT64_C(0x1000);
        inserted[i].metadata = (struct slab) {
            .base = (void *)inserted[i].key,
            .size_class = 16 + i * 16,
            .slot_count = 100 + i,
            .free_count = 50 + i,
            .start_of_next_slot = (void *)(inserted[i].key + 0x80),
        };

        struct map_value value = construct_map_value(&inserted[i].metadata, NULL);
        assert(addr_map_insert(SMALL, inserted[i].key, value) == 1);
    }
}

/*
 * Find adjacent entries where the latter is displaced from its home bucket.
 * Deleting the former must therefore backshift the latter into its slot.
 */
static int find_backshift_pair(size_t *deleted_index, size_t *shifted_index)
{
    for (size_t shifted = 0; shifted < ENTRY_COUNT; shifted++) {
        struct map_entry *shifted_entry =
            addr_map_lookup(SMALL, inserted[shifted].key);
        assert(shifted_entry != NULL);

        if (shifted_entry->dib == 0)
            continue;

        for (size_t deleted = 0; deleted < ENTRY_COUNT; deleted++) {
            if (deleted == shifted)
                continue;

            struct map_entry *deleted_entry =
                addr_map_lookup(SMALL, inserted[deleted].key);
            assert(deleted_entry != NULL);

            if (deleted_entry + 1 == shifted_entry) {
                *deleted_index = deleted;
                *shifted_index = shifted;
                return 1;
            }
        }
    }

    return 0;
}

static void print_and_verify_remaining_metadata(size_t deleted_index)
{
    puts("\nMap entries after backshift deletion:");
    addr_map_enumerate(SMALL);

    for (size_t i = 0; i < ENTRY_COUNT; i++) {
        struct map_entry *entry = addr_map_lookup(SMALL, inserted[i].key);

        if (i == deleted_index) {
            assert(entry == NULL);
            printf("deleted key=%#" PRIxPTR " [gone]\n", inserted[i].key);
            continue;
        }

        assert(entry != NULL);
        assert(entry->key == inserted[i].key);
        assert(entry->value.slab.base == inserted[i].metadata.base);
        assert(entry->value.slab.size_class == inserted[i].metadata.size_class);
        assert(entry->value.slab.slot_count == inserted[i].metadata.slot_count);
        assert(entry->value.slab.free_count == inserted[i].metadata.free_count);
        assert(entry->value.slab.start_of_next_slot ==
               inserted[i].metadata.start_of_next_slot);

        printf("key=%#" PRIxPTR
               " base=%p size_class=%zu slots=%zu free=%zu next=%p dib=%u\n",
               inserted[i].key,
               entry->value.slab.base,
               entry->value.slab.size_class,
               entry->value.slab.slot_count,
               entry->value.slab.free_count,
               entry->value.slab.start_of_next_slot,
               entry->dib);
    }
}

int main(void)
{
    setbuf(stdout, NULL);

    size_t deleted_index = 0;
    size_t shifted_index = 0;
    unsigned int attempt;

    for (attempt = 0; attempt < MAX_INITIALIZATION_ATTEMPTS; attempt++) {
        populate_map(attempt);
        if (find_backshift_pair(&deleted_index, &shifted_index))
            break;
    }
    assert(attempt < MAX_INITIALIZATION_ATTEMPTS);

    struct map_entry *deleted_entry =
        addr_map_lookup(SMALL, inserted[deleted_index].key);
    struct map_entry *shifted_entry =
        addr_map_lookup(SMALL, inserted[shifted_index].key);
    assert(deleted_entry != NULL);
    assert(shifted_entry != NULL);

    struct map_entry *expected_shifted_location = deleted_entry;
    uint8_t old_shifted_dib = shifted_entry->dib;

    printf("Deleting key=%#" PRIxPTR " before displaced key=%#" PRIxPTR
           " (dib=%u)\n",
           inserted[deleted_index].key,
           inserted[shifted_index].key,
           old_shifted_dib);

    delete_entry(SMALL, inserted[deleted_index].key);
    print_and_verify_remaining_metadata(deleted_index);

    shifted_entry = addr_map_lookup(SMALL, inserted[shifted_index].key);
    assert(shifted_entry == expected_shifted_location);
    assert(shifted_entry->dib == (uint8_t)(old_shifted_dib - 1));

    puts("\n[OK] backshift deletion preserves remaining entries and metadata");
    return 0;
}
