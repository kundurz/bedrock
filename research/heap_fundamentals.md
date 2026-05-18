# Heap Fundamnetals

#### What is Heap?
* Heap is a memory region allotted to every program.
* Unlike stack, heap can be dynamically allocated.
* This means that the program can 'request' and 'release' memory from the heap segment whenever it requires. 
* This memory is global, it can be accessed and modified from anywhere within the program and is not localized to the function where it is allocated. 
* This is accomplished using 'pointers' to reference dynamically allocated memory, which in turn leads to a small degradation in performance as compared to using local variables (on the stack)
* It is the responsibility of the developer to 'free' any allocated memory after using it *exactly* once. 
* Internally, these functions use two system calls `sbrk` and `mmap` to request and release heap memory from the operating system. 

#### Diving into glibc heap
`malloc_chunk`
* This structure represents a particular chunk of memory. The various fields have different meaning for allocated and unallocated chunks.
```
struct malloc_chunk {
	INTERNAL_SIZE_T mchunk_prev_size; /* Size of previous chunk, if it is free */
	INTERNAL_SIZE_T mchunk_size       /* Size in bytes, including overhead */ 
	struct malloc_chunk *fd           /* double links -- used only if this chunk is free.*/
	struct malloc_chunk *bk; 
	/* Only used for large blocks: pointer to next larger size */
	struct malloc_chunk* fd_nextsize; /* double links --- used only if this chunk is free*/ 
	struct malloc_chunk* bk_nextsize;
};

typedef struct malloc_chunk* mchunkptr;
```

Allocated chunk:
```
    chunk-> +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
            |             Size of previous chunk, if unallocated (P clear)  |
            +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
            |             Size of chunk, in bytes                     |A|M|P|
      mem-> +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
            |             User data starts here...                          .
            .                                                               .
            .             (malloc_usable_size() bytes)                      .
            .                                                               |
nextchunk-> +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
            |             (size of chunk, but used for application data)    |
            +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
            |             Size of next chunk, in bytes                |A|0|1|
            +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
```

Free Chunk:  
Free chunks maintain themselves in a circular doubly linked list 
```
| Size of previous chunk, if unallocated (P clear) | +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ `head:' | Size of chunk, in bytes |A|0|P| mem-> +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ | Forward pointer to next chunk in list | +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ | Back pointer to previous chunk in list | +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ | Unused space (may be 0 bytes long) . . . . | nextchunk-> +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ `foot:' | Size of chunk, in bytes | +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ | Size of next chunk, in bytes |A|0|0| +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
```
* As we can see allocator chunks optimize for user space, whilst free chunks optimize for 

#### malloc_state
* To deal with multithreading, the glibc allocator creates separate "arenas", which are like mini allocators with their own state, to avoid collisions.
* The main thread's arena is a global variable inside libc and not part of the heap segment. It can be heavily reused across the program lifetime, which makes it stable, predictable. It is often the source of libc leaks.
* Arena headers (`malloc_state` structures_ for other threads are tehmselves stored in the heap segment). Non main arenas can have multiple heaps ('heap' here refers to the internal structure used instead of the heap segment) associated with them. 

```c
struct malloc_state
{
  /* Serialize access.  */
  __libc_lock_define (, mutex);
  /* Flags (formerly in max_fast).  */
  int flags;

  /* Fastbins */
  mfastbinptr fastbinsY[NFASTBINS];
  /* Base of the topmost chunk -- not otherwise kept in a bin */
  mchunkptr top;
  /* The remainder from the most recent split of a small request */
  mchunkptr last_remainder;
  /* Normal bins packed as described above */
  mchunkptr bins[NBINS * 2 - 2];

  /* Bitmap of bins */
  unsigned int binmap[BINMAPSIZE];

  /* Linked list */
  struct malloc_state *next;
  /* Linked list for free arenas.  Access to this field is serialized
     by free_list_lock in arena.c.  */
  struct malloc_state *next_free;
  /* Number of threads attached to this arena.  0 if the arena is on
     the free list.  Access to this field is serialized by
     free_list_lock in arena.c.  */

  INTERNAL_SIZE_T attached_threads;
  /* Memory allocated from the system in this arena.  */
  INTERNAL_SIZE_T system_mem;
  INTERNAL_SIZE_T max_system_mem;
};

typedef struct malloc_state *mstate;
```


#### Bins and Chunks
* A bin is a list (doubly or singly linked list) of free (non-allocated) chunks. Bins are differentiated base on the size of cunks they contain
1. Fast bin
2. Unsorted bin
3. Small bin
4. Large bin

Fast bins are maintained using
```
typedef struct malloc_chunk *mfastbinptr;

mfastbinptr fastbinsY[]; // Array of pointers to chunks
```
* Unsorted, small and large bins are maintained using a single array:
```
typedef struct malloc_chunk* mchunkptr; 

mchunkptr bins[]; // Array of pointers to chunks.
```
* Initially, during the initialization process, small and large bins are empty. 
* Each bin is represented by two values in the bins array.

###### Fast bins
* There are 10 fat bins. Each of these bins maintains a single linked list. Addition and deletion happen from the front of this list (LIFO manner).
* Each bin has chunks of the same size. The 10 bins each have chunks of size: 16, 24, 32, 40, 48, 56, 64, 72, 80, and 88.
* Sizes mentioned here include metadata as well.
* To store chunks, 4 fewer bytes will be available (on a platform where pointers use 4 bytes). Only the `prev_size` and `size` field of this chunk will hold meta data for allocated chunks. `prev_size` of next contiguous chunk will hold user data.
* No two contiguous free fast chunks coalesce together. 

##### Unsorted bin
* There is only 1 unsorted bin. Small and large chunks, when freed, end up in this bin. The primary purpose of this bin is to act as a cache layer (kind of) to speed up allocation and deallocation requests.

##### Small bins
* There are 62 small bins. 
* Small bins are faster than large bins, but can be slower than fast bins. Each bin maintains a doubly-linked list. 
* Insertions happen at the `HEAD` while removals happen at the `TAIL`) in a FIFO manner.
* Like fast bins, each bin has chunks of the same size. The 62 bins have sizes: 16, 24, ..., 504 bytes
* While freeing small chunks may be coalesced together before enduing up in unsorted bins.

##### Large bins
* There are 63 large bins. Each bin maintains a doubly-linked list.
* A particular large bin has chunks of different sizes, sorted in decreasing order (i.e. largest chunk at the 'HEAD' and smallest chunk at the 'TAIL')
* The first 32 bins contain chunks, which are 64 bytes apart:
	* 1st bin: 512-568 bytes
	* 2nd bin: 575 - 632 bytes
	* .
	* .
	* To summarisze:

There are two special chunks which are not part of any bin:

1. Top chunk: it is the chunk which borderes the top of an arena. While servicing 'malloc' requests, it is used as the last resort. If still more size is required, it can grow using the `sbrk` system call. The `PREV_INUSE` flag is always set for the top chunk. 
2. Last remainder chunk: The chunk obtained from the last split. Sometimes, when exact size chunks are not available bigger chunks are split in two. One part is returned to the user and the other becomes the last remainder chunk.
