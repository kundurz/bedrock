#include "heap_internal.h"
#include "utils.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static void test_fast_alloc_unique_and_writable(void)
{
    puts("hii!");
    enum { N = 64 };
    void *ptrs[N];

    for (int i = 0; i < N; i++) {
        ptrs[i] = heap_alloc(32);
        assert(ptrs[i] != NULL);

        memset(ptrs[i], 0x41 + (i % 26), 32);

        for (int j = 0; j < i; j++) {
            assert(ptrs[i] != ptrs[j]);
        }
    }

    // Temporarily commenting this out before i port over heap free.
    for (int i = 0; i < N; i++) {
        heap_free(ptrs[i]);
    }

    puts("[OK] fast allocations are unique and writable");
}

static void test_large_alloc_writable(void)
{
    char *p = heap_alloc(5000);
    assert(p != NULL);

    memset(p, 'A', 5000);

    for (int i = 0; i < 5000; i++) {
        assert(p[i] == 'A');
    }

    heap_free(p);

    puts("[OK] large allocation is writable");
}

static void test_large_split_and_coalesce(void)
{
    char *base = heap_alloc(10000);
    assert(base != NULL);
    heap_free(base);

    char *a = heap_alloc(2049);
    char *b = heap_alloc(2049);
    char *c = heap_alloc(2049);
    char *d = heap_alloc(5917);

    assert(a != NULL);
    assert(b != NULL);
    assert(c != NULL);
    assert(d != NULL);

    heap_free(a);
    heap_free(c);
    heap_free(b);
    heap_free(d);

    assert(*large_chunk_bin != NULL);
    assert((*large_chunk_bin)->size >= 10000);
    //assert((*large_chunk_bin)->fd == NULL); <-- bad assert, does not assume that tests share state.

    puts("[OK] large chunks split and coalesce");
}

static void test_large_reuse_after_coalesce(void)
{
    char *p = heap_alloc(10000);
    assert(p != NULL);

    memset(p, 'Z', 10000);
    heap_free(p);

    char *q = heap_alloc(9000);
    assert(q != NULL);

    memset(q, 'Q', 9000);
    heap_free(q);

    puts("[OK] coalesced large chunk can be reused");
}

int main(void)
{
    test_fast_alloc_unique_and_writable();
    puts("Fast allocator tests passed.");
    test_large_alloc_writable();
    test_large_split_and_coalesce();
    test_large_reuse_after_coalesce();

    puts("All allocator tests passed.");
    return 0;
}