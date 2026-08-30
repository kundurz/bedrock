# Bedrock: Security-Hardened Memory Allocator

Bedrock is a security-hardened dynamic memory allocator. It combines slab-based small allocations and guarded-large allocations with metadata isolation, randomized placement, quarantine, and allocation validitation.

**Author**: Linus Kundur-Zourntos<br>
**Language**: C | **Platform**: Linux | **Build**: Make 

```
                    bedrock_alloc(size)
                           │
                    ┌──────▼──────┐
                    │ size class? │
                    └───┬─────┬───┘
                       small   large
                         │       │
              ┌──────────▼ ─┐   ┌▼──────────────── ┐
              │ Randomized  │   │ Guarded mapping  │
              │ slab slot   │   │ + random offset  │
              └──────┬──────┘   └────────┬─────────┘
                     │                   │
                     ▼                   ▼
                User memory          User memory
                     │                   │
                     └─────────┬─────────┘
                               │
                         bedrock_free()
                               │
                    ┌──────────▼──────────┐
                    │ Address-map lookup  │
                    │ + pointer validation│
                    └──────────┬──────────┘
                               │
                       valid allocation?
                         │           │
                        NO          YES
                         │           │
                     terminate       ▼
                               ┌──────────────┐
                               │ Small / Large│
                               └──────┬───────┘
                                  │          │
                               small       large
                                  │          │
                           zero + quarantine │
                                  │       PROT_NONE
                                  │       MADV_DONTNEED
                                  │          │
                                  ▼          ▼
                               delayed     cache
                                reuse      / unmap
```

## Table of Contents
- [Motivation](#motivation)
- [Threat Model](#threat-model)
- [Allocator Design](#design--implementation)
- [Correctness & Security Testing](#correctness--security-testing)
- [Exploit Demonstrations](#exploit-demonstrations)
- [Performance Evaluation](#performance-evaluation)
- [Limitations & Future Work](#limitations--future-work) 
- [Build & Usage](#build--usage)
- [LICENSE](#license)

## Motivation

Because modern operating systems have successfully mitigated standard stack-based attacks, the modern security landscape has heavily shifted toward the heap.

Modern allocator engineering is usually governed by three pillars: 
1) Performance (throughput)
2) Memory efficiency (fragmentation)
3) Security (hardening & isolation). 

Bedrock  explores how allocator architecture can be highly optimized for security, reducing the impact of common memory corruption vulnerabilities, such as use-after-free, double-free, invalid free, and metadata corruption.

The allocator does not attempt to make memory corruption impossible. Instead, it introduces mitigations that aim to make common exploit primitives fail closed.

## Threat Model

### Attacker Capabilities
It is assumed the attacker can influence:
* Allocation sizes
* Allocation and free order
* Data written into allocated objects
* Repeated allocation patterns
* Calls to `bedrock_free()` with NULL, stale, interior, duplicate or arbitrary pointres
* Linear overflows and underflows originating in user allocations
* Use of stale pointers after an object has been freed 

### Protected Assets
The allocator attempts to protect
* Slab and large allocation metadata
* Previously freed user data
* Adjacent virtual memory regions 
* The integrity of allocation and free operations
* Data structures pertaining to metadata

## Security Mitigations

### Metadata isolation
Small and large allocation metadata is stored in a memory region seperate from user-controlled allocation regions. A linear overflow from a user allocation region therefore cannot directly overwrite chunk headers or free-list pointers. This does not protect metadata from exploits that utilize arbitrary address write vulnerabilities.

### Invalid-free detection
The allocator rejcts:
* Small-allocation double  frees
* Large-allocation double frees
* Interior small-allocation pointers
* Interior large-allocation pointers
* Arbitrary pointers not owned by the allocator

Small allocations are validated using allocation bitmap state, slot alignment, and user page lookup. Large allocatiosn require an exact address-map match. 

Detected violations terminate the process.

### Large allocation spatial protection
Large allocations are surrounded by page-granularity redzones. Sufficiently large overflows or underflows reachign these pages cause a fault instead of crossing into another mapping.

### Use-after-free mitigation
Freed small allocations are zeroed out and placed in a fixed-length quarantine queue before their slots become reusable. 

Freed large mappigns are:
* Removed from the active address map
* Made inaccessible using `PROT_NONE`
* Discarded using `MADV_DONTNEED`
* Retained only in a bounded cache or unmapped.

The allocator also uses operating-system entropy to:
* Salt address-map hashing
* Shuffle slab allocation order
* Randomize large-allocation offsets

This reduces deterministic slot reuse and predictable address placement. 

### Integer-overflow protection
Mapping-size and page-rounding calculations use checked-arithmetic. A deteced overflow terminates the process.

## Design & Implementation

### Allocation Architecture
```
                    Allocation
                         │
              ┌──────────┴──────────┐
              │                     │
           Small                  Large
              │                     │
            Slabs               mmap region
              │                     │
       fixed-size slots       guarded mapping
```
Bedrock seperates allocations into a small path and a large path. Small allocations are rounded up to one of eight power-of-two size classes ranging from 16 to 2048. This is done to guarantee predictable performance. Large allocations recieve dedicated virtual-memory mappigns that allow page-level protection and randomized placement. This was chosen over a traditional free-list design because splitting and coalescing mechanics are prime targets for heap exploitation.

### Metadata Management
As mentioned earlier in this document, metadata is never stored inline and is always maintained in a seperate memory region. Each large allocation has its own metadata. For small allocations metadata is per-slab (not per-slot). 

Each slab is identified by its page-aligned address. The allocator uses that address as a key in an address map whose value points to the slab's metadata. 

Each large allocation is identified by the exact pointer returned to the caller from `bedrock_alloc()`. The allocator uses that pointer as a key in a seperate address map whoe value contains the allocation's metadata. 

```
                  BEDROCK ALLOCATION METADATA
                  ════════════════════════════

       SMALL ALLOCATIONS                    LARGE ALLOCATIONS
       ════════════════                     ═════════════════

       ┌───────────────────┐                ┌───────────────────┐
       │       SLAB        │                │  LARGE ALLOCATION │
       │                   │                │                   │
       │ [slot][slot]...   │                │    user memory    │
       │                   │                │                   │
       └─────────┬─────────┘                └─────────┬─────────┘
                 │                                    │
                 │ page-aligned                       │ exact pointer
                 │ slab address                       │ returned to caller
                 ▼                                    ▼
       ┌───────────────────┐                ┌───────────────────┐
       │   SMALL ADDRESS   │                │   LARGE ADDRESS   │
       │        MAP        │                │        MAP        │
       ├───────────────────┤                ├───────────────────┤
       │ slab address      │                │ allocation ptr    │
       │       │           │                │       │           │
       │       ▼           │                │       ▼           │
       │ slab metadata ────┼───┐            │ allocation ───────┼───┐
       └───────────────────┘   │            │ metadata          │   │
                               │            └───────────────────┘   │
                               │                                    │
                               ▼                                    ▼
                        ┌───────────────┐                    ┌───────────────┐
                        │ SLAB METADATA │                    │ LARGE ALLOC.  │
                        │               │                    │   METADATA    │
                        │ size class    │                    │               │
                        │ allocation    │                    │ mapping size  │
                        │ bitmap        │                    │ requested size│
                        │ slot state    │                    │ cache state   │
                        │ ...           │                    │ ...           │
                        └───────────────┘                    └───────────────┘
```



### Address maps

The allocator maintains seperate address maps for small slabs and large allocations. Both use open-addressed Robin Hood hashing with random per-map salt. 

#### Collision Resolution Policy
Robin hood hashing was the chosen collision resolution policy because address lookup occurs on critical allocation and free paths. With ordianry linear probing, entires that suffer early collisions can develop long and uneven probe chains. Robin hood hashing seeks to equalize the lookup length of different entires and avoids a small number of entires becoming disproportionately expensive to find. 

#### Salted address hashing
Salted address maps are implemented to make table layout unpredictable and deliberate collision construction more difficult.Because of this, an attacker who knows the table size and hash function cannot predict which addresses colide. A per-process random-salt makes collision patterns dependent on runtime entropy. 

Having seperate salts for large and small address maps ensures that learning about the collision behaviour of one map does not yield any meaningful insight into the layout of the other. 

#### Load-factor limit
Open-addressed hash tables become increasingly expensive as they approach full capacity, as high occupancy creates longer probe clusters. 

The address maps resize at approximately 70% load factor. This strikes a balance between:
* Memory consumption
* Cache and dTLB footprint
* Average probe length
* Worst-case collision behaviour

#### Backward-shift deletion
Deleting an entry from an open-addressed table cannot be implmented by simply marking its bucket as empty. 

Consider the following collision chain:
```
home bucket --> entry A --> entry B --> entry C --> empty bucket
```

If entry A were cleared withotu repairing the chain, a lookup for entry B or entry C could encounter the new empty bucket and incorrectly conclude that the key is absent. 

Many hash tables solve with with tombstones, which mark a bucket as deleted without terminating searches. The problem is tombstones can accumulate over time and lengthen future lookups unless the table is periodically rebuilt. 

Bedrock instead uses backward-shift deletion. After removing an entry, subsequent displaced entries are moved backward until the algorithm reaches either an empty bucket or an entry already occupying its ideal bucket.

Conceptually:
```
Before:
[A][B][C][empty]

Delete A:
[empty][B][C][empty]

Shift:
[B][C][empty][empty]
```
### Allocation Lifecycle 
For small allocations observe the following state machine:
```
AVAILABLE SLOT
    │
    ▼
ALLOCATED SLOT
    │
   free
    ▼
QUARANTINED SLOT
    │
    │ quarantine expires
    ▼
AVAILABLE SLOT
```
Slots shall remain in the quarantine until `bedrock_free()` has been called 30 seperate times.

The following state machine reflects large allocations:
```
UNMAPPED
    │
    ▼
ALLOCATED
    │
   free
    ▼
PROTECTED RING CACHE
    │
    ├── reuse → ALLOCATED
    │
    └── eviction → UNMAPPED
```

The protected ring cache contains 32 entries. When the caller frees a large allocation, it is inserted into the cache. From there it can either be reused (meaning it was selected to be returned during a best-fit search) or if it remains in the cache for too long (for more than 32 frees of large allocations), it is evicted.

### Memory Reclamation
The following memory reclamation strategy is utilized:
* empty slabs --> unmapped
* unused metadata slots --> reclaimed
* empty metadata arenas --> unmapped
* cached large mappings --> bounded
* evicted large mappings --> unmapped 
* `MADV_DONTNEED` on freed large mappings

Bedrock intentionally reatins some freed memory to offset the performance overhead of repeated calls to syscalls like `mmap`, but bounds this retention to prevent quarnatine and caching from growing without limit. 

## Correctness & Security Testing

### Correctness Tests
| Test | Executable | Behavior verified |
|---|---|---|
| Small allocation sanity | `sanity_test` | Allocations succeed across the supported small size classes and can be freed without failure |
| Large allocation sanity | `sanity_test` | Allocations succeed across multiple large request sizes and can be freed correctly |
| Small alignment | `correct_alignment_test` | Returned small-allocation pointers satisfy `_Alignof(max_align_t)` |
| Large alignment | `correct_alignment_test` | Randomized large-allocation pointers remain correctly aligned |
| Small read/write | `read_write_test` | Every requested byte can be written and read without corruption |
| Large read/write | `read_write_test` | Every requested byte in a guarded large allocation remains usable |
| Allocation non-overlap | `overlap_test` | Simultaneously live small allocations do not overlap or corrupt one another |
| Null free | `free_tests` | `heap_free(NULL)` behaves as a no-op |
| Large-cache reuse | `reuse_test` | Eligible large requests reuse cached mappings rather than creating unnecessary new mappings |
| Mixed-workload stress | `mixed_workload` | Mixed small and large allocations preserve their contents and remain valid when freed in randomized order |
| Repeated stress rounds | `mixed_workload` | The allocator completes 25 rounds of 4,096 randomized allocations and frees

### Security Property Coverage
| Test | Executable | Protection verified |
|---|---|---|
| Small double free | `free_tests` | A second free of a small allocation is detected and terminates the offending process |
| Large double free | `free_tests` | A second free of a large allocation is rejected after its address-map entry has been removed |
| Small interior-pointer free | `free_tests` | A pointer not aligned to the beginning of a slab slot is rejected |
| Large interior-pointer free | `free_tests` | A pointer inside a large allocation is rejected because it does not exactly match an address-map key |
| Arbitrary-pointer free | `free_tests` | An address not owned by the allocator is rejected |
| Large trailing guard page | `page_protection_test` | A write extending beyond a large allocation into its guard page causes a protection fault |
| Large use after free | `page_protection_test` | A freed large mapping becomes inaccessible while retained in the cache |
| Quarantine-delayed reuse | `reuse_test` | A recently freed small slot is withheld for 30 intervening frees before becoming reusable |
| Randomized allocation integrity | `mixed_workload` | Random slot selection and randomized large placement do not cause overlap or data corruption under stress |

## Exploit Demonstrations 

### 1. UAF Information Disclosure
**Objective:** Demonstreate that freed memory can expose data from a subsequent allocation when memory is immediately reused.

**glibc**: succeeds
**Bedrock**: mitigated through zeroing + quarantine. 

#### Exploit
```c
char *victim = malloc(32);
strcpy(victim, "SECRET_DATA");

free(victim);

char *replacement = malloc(32);
strcpy(replacement, "ATTACKER_DATA");

printf("Stale pointer: %s\n", victim);
``` 

#### Baseline: malloc
```
$ ./uaf_demo
Stale pointer: ATTACKER_DATA
```

#### Bedrock
```
$ ./uaf_demo
Stale pointer:
```

#### Why the behaviour differs 
Using glibc malloc, the freed region is immediately reused, allowing the attacker to influence the contents of that region if it is incorrectly used after it is freed. Bedrock zeroes freed small allocations and places them into quarantine, preventing exploitation even if human error is present.

### 2. Large Allocation Overflow
**Objective**: Demonstrate taht out-of-bounds write beyond a large allocation is contained by Bedrock's page-granularity guard pages. 

**glibc**: succeeds
**Bedrock**: address_map + alignment validation rejects it. 

#### Exploit
```c
#include <stddef.h>
#include <stdlib.h>

#define SIZE 4096

int main(void)
{
    unsigned char *p = malloc(SIZE);

    for (size_t i = 0; i < SIZE + 4096; i++)
        p[i] = 'A';

    return 0;
}
```

#### Baseline: malloc
```
$ ./overflow_demo
Overflow completed without an immediate fault

```
#### Bedrock
```
$ ./overflow_demo
Segmentation fault
```

#### Why the behaviour differs
Bedrock places a guard page immediately after each large allocation. The out-of-bounds write therefore enters an inacessible page and triggers a segfault. 

## Performance Evaluation

For information on the benchmarking environment please see: [benchmarking details](benchmarking/README.md)

The unhardened free-list implementation used in some of the results is preserved in the repository's Git history at commit `9f0ace4` and was checked out in a seperate Git worktree for benchmarking.



### Throughput
---
| Allocator | Small-allocation throughput | Relative throughput |
|---|---:|---:|
| Unhardened free-list design | 36.23 million ops/s | 100% |
| glibc `malloc` | 24.25 million ops/s | 66.93% |
| Hardened Bedrock allocator | 6.30 million ops/s | 17.38% |

Bedrock currently achieves a 6.30 M operations/s on the measured small-allocation workload, compared with 36.23M operations/s for the unhardened allocator and 24.25M operations/s for glibc. THis represents an 82.6% throughput reduction relative to the unhardened implementation.

This overhead is expected from Bedrock's additional validation, randomized placement, metadata isolation, quarantine, and memory-protection mechanisms. The benchmark therefore illustrates the cost of the allocator's security-oriented design rather than attempting to demonstreate superiority over production allocators.

### Allocation & Free Latency
---
| Percentile | Unhardened alloc | Bedrock alloc | Unhardened free | Bedrock free |
| ---------- | ---------------: | ------------: | --------------: | -----------: |
| p50        |            23 ns |         52 ns |           24 ns |        82 ns |
| p95        |            38 ns |         89 ns |           40 ns |       139 ns |
| p99        |            43 ns |        115 ns |          100 ns |       183 ns |

Hardening increased the median allocation latency from 23 ns to 52 ns and median free latency from 23 ns to 82 ns. This corresponds to an additional 29 ns per allocation and 58 ns per free. The larger free path cost reflects exact-pointer validation, address-map lookup, allocation-bitmap checking, memory zeroing, quarantine processing, and delayed-reuse bookkeeping. Although the relative increases are substantial, the absolute median cost remains on the order of tens of nanoseconds. Maximum values were omitted because they are particularly sensitive to scheduling and interrupts.

### Cache Locality
---
The address map originally stored the complete slab metadata structure directly inside each map entry. This made entries large, causing hash map probes to touch more pages.

The design was refactored to store only a pointer to slab metadata in each map entry. The metadata itselflr resides in dedicated guarded arenas. This reduces map-entry size and allows mor eentires to occupy each page.

Cache behaviour was measured with `perf stat` over 30 repetitions of small allocation throughput workload
| Measurement | Before refactor | After refactor | Change |
|---|---:|---:|---:|
| Cycles per operation | 467.32 | 434.77 | −6.97% |
| Instructions per operation | 427.18 | 425.88 | −0.30% |
| Instructions per cycle | 0.91 | 0.98 | +7.69% |
| Cache references per operation | 21.31 | 20.63 | −3.19% |
| Cache misses per operation | 14.23 | 9.09 | −36.12% |
| Cache-miss rate | 66.77% | 44.07% | −22.70 points |
| L1 data-cache misses per operation | 3.83 | 4.11 | +7.31% |
| L1 data-cache miss rate | 5.37% | 5.62% | +0.25 points |
| dTLB misses per operation | 0.50 | 0.30 | −40.00% |
| dTLB miss rate | 0.71% | 0.41% | −0.30 points |

The strongest result was the 40% reduction in dTLB misses. Having more compact map entires allowed probing to touch fewer distinct virtual memory pages, reducing address-translation pressure and improving this metric. This helped contribute to a 6.97% reduction in cycles per operation despite intstructions per operation remaining nearly unchanged. 

L1 misses increased slightly because accessing the selected slab now requires an additional address dereference. A small amount of L1 locality has been traded for substantially better dTLB locality and lower overall execution cost. 

### Memory Fragmentation and Mapping Overhead
---
The fragmentation ebnchmark performed 100,000 allocations using a fixed random seed. It measured allocation region-overhead with the complete working set live and after freeing 50% of allocations in random order.
```
overhead = (mapped allocation bytes / live requested size - 1) x 100
```
Allocator metadata was excluded from the primary results so that fragmentation and allocator-mapping costs were not combined with address maps, quarantine storage, metadata aaarenas, and other bookkeeping

#### Small allocations
The small workload used uniforming generated request sizes between 0 and 2048 bytes.
| Workload state | Payload overhead | Mapped bytes per live byte |
|---|---:|---:|
| Full working set | 33.37% | 1.33× |
| After 50% random frees | 113.99% | 2.14× |

The full working-set overhead primarily reflects power-of-two size-class rounding and partially occupied termianl slabs.

After 50% of all allocations were freed randomly, surviving objects remained distributed across partially occupied slabs. Because a slab cannot be unmapped while it contains live or quarantined slots, the mapped-memory cost is increased to 2.14 bytes per live requested byte.

This increase is expected for a slab allocator under random deletion and does not indicate incorrect memory reclamation

#### Large allocations
The large workload used uniformly requested sizes between 2049 and 8191 bytes.
| Workload state | Mapping overhead | Mapped bytes per live byte |
|---|---:|---:|
| Full working set | 300.30% | 4.00× |
| After 50% random frees | 300.69% | 4.01× |

Large allocations use a distinct guarded mapping for each request. Their approximately 300% baseline overhead is dominated by:
* A guard page before the usable region
* A guard page after the usable region
* Page-size rounding
* Randomized allocation offsets

This is deliberate security overhead rather than conventional external fragmentation.

After freeing 50% of allocations, mapping overhead increased by only 0.39 percentage points. This indicates that large mappings were released effectively and that the bounded large-allocation cache introduced little additional retention for this workload.

## Limitations & Future Work

### Limitations
#### 1. No protection against arbitrary writess
Bedrock's out-of-band metadata prevents the direct corruption of allocator metadata through buffer overflows orginating in user allocations. This does not protect metadata agianst exploits that take advantage of arbitrary-address write vulnerabilities.

#### 2. UAF mitigation is not complete prevention
While quarantining slots after they are freed makes exploiting UAF vulnerabilities less straightfoward, the quarantine has a finite size and eventually a slot will be reused. For this reason, complete memory safety cannot be guaranteed.

#### 3. Guard pages don't catch every overflow
Guard pages around large allocations make it harder to perform a linear overflow into memory that is outside of the region returned to the caller. However, if the overflow occurs strictly within the writable region returned to the user, it will not be detected. 

#### 4. Single-threaded
The current allocator is single-threaded and does not provide syncrhonization for concurrent allocation or deallocation.

#### 5. Performance / memory overhead
As introduced in the performance evaluation section, Bedrock's security mechanisms introduce susbtantial throughput, latency, and virtual memory overhead relative to the unhardened allocator, particularly due to metadata segregation, quarantine, guard pages, and slab-slot randomization. 

#### 6. Workload Evaluation
Performance measurements currently focus mostly on controlled synthetic workloads and are not totally representative of how the allocator would perform under general-purpsoe application workloads.

### Future Work

#### 1. Concurrent allocator
Bedrock can be extended to support concurrent allocation and deallocation through per-thread/per-CPU caches.

#### 2. Fuzzing
Introduce coverage-guided fuzzing of allocation/free sequences, pointer manipulation, randomized sizes, and allocator state transititions.

#### 3. Larger benchmark suite
The benchmark suite could be expanded to focus on additional metrics and scenarios if such information would be useful in continued development.

## Build & Usage

### Requirements

Bedrock currently targets 64-bit Linux and requires:

* GCC or another compatble C compiler
* GNU Make
* A system supporting `mmap()`, `mprotect()`, and `getrandom()`

### Public Interface

Bedrock exposes a miminal allocation interface throigh `bedrock.h`

| Function | Description |
|---|---|
| `bedrock_alloc(size)` | Allocates `size` bytes and returns an aligned pointer, or `NULL` on allocation failure. |
| `bedrock_free(ptr)` | Releases an allocation previously returned by `bedrock_alloc()`. Invalid frees terminate the process. |


### Building Bedrock
Clone the repository and build the library variants
```bash
git clone 
cd bedrock/build
make
```
This produces:
* `libbedrock.a`: Static Bedrock library
* `libbedrock.so`: Dynamic Bedrock library

### Using Bedrock
Include the public header:
```c
#include <bedrock.h>

int main(void)
{
    char *buffer = bedrock_alloc(256);
    if (buffer == NULL)
        return 1;

    /* Use buffer. */

    bedrock_free(buffer);
    return 0;
}
```

#### Dynamic Linking
From the repository's `build/` directory:
```
gcc program.c -I../include -L. -lbedrock \
    -Wl,-rpath,'$ORIGIN' -o program
```

The executable and `libbedrock.so` must reamin in the same directory when using the $ORIGIN configuration. $ORIGIN is a special Linux dynamic-linker variable meaning "the directory containing the executable".

#### Static Linking
```
gcc program.c -I../include ./libbedrock.a -o program
```

#### Running the Test Suite
From `build/`:
```
make tests
```

#### Running the Examples
```
make examples
```

This recompiles and runs every example using the dynamic Bedrock library.

Individual examples can also be built and run:
```
make use_after_free_demo
./use_after_free_demo

make large_allocation_overflow_demo
./large_allocation_overflow_demo
```

#### Cleaning Build Artifacts
```
make clean
```

## LICENSE 

Bedrock is licensed under the Apache License, Version 2.0.

See [LICENSE](LICENSE) for the full license text.






