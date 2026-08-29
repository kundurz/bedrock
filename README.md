# Bedrock: Security-Hardened Memory Allocator

Bedrock is a security-hardened dynamic memory allocator. It combines slab-based small allocations and guarded-large allocations with metadata isolation, randomized placement, quarantine, and allocation validitation.

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







