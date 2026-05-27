# Glibc Fundamentals 

#### Bins and Chunks
* A bin is a list (doubly or singly linked list) of free (non-allocated chunks). 
* Bins are differentiated based on the size of the chunks they contain
1. Fast bin
2. Unsorted bin
3. Small bin
4. Large bin
#### Fast Bins
* There are 10 fast bins. 
* Each of these bings maintains a single linked list. Addition and deletion happen from the front of the list (LIFO manner)
* Each bin has chunks of the same size.
* The 10 bins each have chunks of sizes: 16, 24, 32, 40, 48, 57, 64, 72, 80, and 88 (includes metadata as well). 
* No two contiguous free fast chunks coalesce together

#### Unsorted bin
* There is only 1 unsorted bin.
* Small and large chunks, when freed end up in this bin.
* The primary purpose fo this bin is to act as  acahce layer to speed up allocation and deallocation requsts.

#### Small bin
* There are 62 small bins.
* Small bins are faster than large bins but slower than fast bins.
* Each bin maintains a doubly-linked list. Insertions happen at the 'HEAD' while removals happen at the 'TAIL' (in FIFO manner).
* Like fast bins, each bin has chunks have the same size. The 62 bins have sizes: 16, 24, ... 504 bytes
* When freeing, small chunks may be coalesced together before ending up in unsorted bins.

#### Large bins
* There are 63 large bins.
* Each bin maintains a doubly-linked list.
* A particular large bin has chunks of different sizes, sorted in decreasing order (i.e. largest chunk at the 'HEAD' and smallest chunk at the 'TAIL')
* Insertions and removals happen at any position within the list
* The first 32 bins contain chunks which are 64-byte apart.
```
No. of Bins       Spacing between bins

64 bins of size       8  [ Small bins]
32 bins of size      64  [ Large bins]
16 bins of size     512  [ Large bins]
8 bins of size     4096  [ ..        ]
4 bins of size    32768
2 bins of size   262144
1 bin  of size what's left
```
* Like small chunks, when freeing, large chunks may be coalesced together before ending up in unsorted bins

#### Top Chunk
* It is the chunk which borders the top of an arena. 
* While serving `malloc` requests, it is used as the last resort
* If still more isze is required, it can grow using the `sbrk` system call. 
* The `PREV_INUSE` flag is alway set for the top chunk.

#### Last Remainder chunk
* It is the chunk obtained from the last split. Sometimes, when exact size chunks are not available, bigger chunks are split into two. 
* One part is returned to the user whereas the other becomes the last remainder chunk.


# Core Functions
`void *_int_malloc(mstate av, size_t bytes)`
* **Big Picture:**
	1. Normalize size
	2. get arena
	3. fastbin, if small enough
	4. smallbin, if exact small size
	5. consolidate fastbins, if large request
	6. unsorted bin
	7. largebin 
	8. next larger bins
	9. top chunk
	10. sysmalloc from OS
* An arena is an allocator state, it contains:
	* fastbins
	* smallbins
	* largebins
	* unsorted bin
	* top chunk
	* binmap
	* flags
	* system memory size
* `av` refers to the relevant arena, `bytes` refers to the requested sie. 
* `bytes` is the user-requested size.
* The things `_int_malloc` does:
	1. Convert user size into an internal chunk size taking into account things like metadata, alignment, minimum chunk size, and flags
	2. If there is no usable arena (`av` is NULL) it calls `sysmalloc`, which gets you one. This may happen through `mmap`
	3. **Fastbin path**: Next, glibc checks: Is this request small enough for fastbins? Fastbins are for very small chunks. They are: singly linked lists, LIFO, not immediately consolidated, very fast. 
	4. **smallbin path**: If the request is not serviced by fastbins, glibc checks wheteher its smallbin-sized request. Small bins are for small chunks, but unlike fastbins: they are doubly linked lists, they contain exact-size chunks, chunks are already consolidated. Each smallbin corresponds to one chunk size. 
	5. **Large request path preparation and fastbin consolidation**: If the request is larger than the smallbin range, glibc prepares for largebin logic, but first it checks: Does this arnea have fastbin chunks waiting around? Fastbin chunks are not merged immediately when freed, so before making larger alloctions, glibc may call: `malloc_consolidate(av)`, that function drains fastbins and merges adjacent free chunks. This is done because a larger alloccation may need memory that is currently fragmented across fastbins.
	6. **If fastbin/smallbin did not return anything**: At this point `__int_malloc` has not found a chunk: This could mean that it is a fastbin request, but matching fastbin was empty, or maybe a smallbin size request, but the matching smallbin was empty, or maybe a largebin size request. Now glibc checks the unsorted bin
	7. **Unsorted bin scan**: The unsorted bin contains recently freed normal chunks, when you free a non-fastbin chunk it often goes first to the `unsorted bin`, not directly to smallbin or large bin. This occurs because glibc hopes it can reuse it soon without sorting it into the smallbin or large bin. 
	8. **Special last remainder case**: if the request is smallbin-sized, and victim is last remainder and victim is the only unsorted chunk and victim is large enough, then glibc splits it. This is a performance heuristic. The last remainder is the leftover memory from a previosu split. glibc may prefer using this remainder because it encourages locality and avoids searching other bins. 
	9. **Exact match from unsorted bin**: If the unsorted chunk matches exactly the requested size, glibc returns it directly.
	10. **If unsorted chunk is not used, it gets sorted**: while scanning the unsorted bin, if glibc does not use a chunk, it removes it from the unsorted bin and places it into the proper real bin. If the chunk is small, move to smallbin, if large move to largebin. The unsorted bin scan does to things:
		1. opportunistically try to reuse chunks
		2. sort unused chunks into proper bins. So, chunks are foten first placed into unsorted bins, and later sorted into smallbins or largebins during malloc
	11. **Largebin insertion**: if an unsorted chunk is too large and not used immediately, it may be inserted into a large bin. Largebins are sorted approximately by size. The allocator wants to make future large allocations efficient by keeping chunks ordered. When inserting a new lagre chunk, glibc finds where it belongs. 
	12. **Limit on unsorted-bin scanning**: glibc does not scan forever, if the unsorted bin somehow has a huge number og chunks, glibc limits how much work it does. This is primarily a performance and safety measure. 
	13. **After unsorted bin, check large bins**: If the request is not smallbin-sized, glibc now checks the appropriate largebin, goal is to find the smallest chunk that is big enough. 
	14. **Splitting a largebin chunk**: If glibc finds a large chunk that is bigger than requested, it may split it. THe remainder usually goes in the unsorted bin because maybe iti will be reused soon. This is a recurring theme: newly free dor leftover chunks often go to unsorted first. 
	15. **Search bigger bins**: If the exact largebin does not work, glibc searches larger bins. This is where `binmap` comes in. The `binmap` is a bitmap saying approximately which bins may be non-empty. Instead of checking every bin one by one, glibc uses the bitmap to skip empty regions. 
	16. **Use a larger hbin chunk**: If glibc finds a bigger chunk in a later bin, it uses it. 
	17. **Top chunk**: If no bin can satisfy the request glibc tries the top chunk. The top chunk is the wilderness chunk at the end of the heap. 
	18. **If top chunk is not enough**: If the top chunk cannot satisfy the request, glibc checks again: Are there fastbin chunks that have not been consolidated? If yes: `malloc_consolidate(av)`, then glibc goes back and tires the unsorted-bin path again. Why? Because consolidation may have created a larger usable free chunk. 
	19. **sysmalloc**: If even the top chunk cannot satisfy the request, and consolidation did not help, glibc asks the OS for more memory. 

* The things `_int_free()` does
	1. **Validate Pointer**: checks if the pointer is "sane"
		1. **Check 1**: checks for pointer wrapping, integer overflow, and  impossible addresses
		2. **Check 2**: checks size >= MinSize and alignment is correct
	2. **Fastbin free case**: Now glibc checks if the chunk being freed belongs in the fastbin. Fasbin chunks are small and not consolidated immiedately, also stored in a singly linked list
		1. **Check next chunk size**. glibc looks at the next chunk in memory and ensures its size is sane.  
		2. **Set FASTCHUNKS_BIT**: This flag means there are unconsolidated fastbin chunks.
		3. **Double free check**: Is this chunk aready at the top of the fastbin? If yes, double free or corruption (fasttop)
		4. **Fastbin size consistency**: glibc checks does the top fastbin chunk match this bin size? This prevents cross-bin corruption and fake chunks.
		5. **Then insertion happens**: Chunk is inserted at the head. LIFO behaviour. THen `__int_free()` returns immediately. Fastbin frees stop here. No coalescing. No unsorted bin. No merging. 
	3. **Normal Free path**: If chunk is not fastbin-sized glibc enter snormal free logic. 
		1. **Check 1:** Is chunk the top chunk? If you try to free the top chunk `double free or corruption (top)`, top chunk is allocator-owned wilderness space. it should never appear in ordinary free flow. 
		2. **Check 2**: Is next chunk inside arena? glibc verifies that next chunk exists inside valid heap memory. This prevents: 
			1. forged chunk sizes
			2. walking outside heap 
		3. **Check 3:** PREV_INUSE consistency. glibc checks "Does next chunk think THIS chunk is in use?". Remember `PREV_INUSE` is stored in the NEXT chunk. If B already says A is free: A may already have been freed and metadata may be corrupted
	4. **free_perturb()**: This fills freed memory with a debug pattern. 
	5. **Backward Coalescing**: glibc checks `is previous chunk free?`, using `PREV_INUSE` bit. 
	6. **Forward COalescing**: Now glibc checks "is next chunk free", if yes, unlink next chunk and merge it too. 
	7. **Insert into Unsorted bin**: After merging: glibc inserts the resulting chunks into: unsorted bin, NOT idrectly into smallbin or largebin. 
	8. **Top Chunk merge**; if the next chunk is the top chunk, merge it with the top, instead of inserting into unsorted bin. 
	9. **mmaped chunks**: some large allocations use: `mmap`, instead of ordinary heap, when freed `munmap()`, returns memory directly ot the OS. 
