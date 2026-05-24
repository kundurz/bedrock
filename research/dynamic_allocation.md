# Dynamic Allocation

* A dynamic memory allocator maintains an area of a process's virtual memory known as the heap.
* The heap is an area of demand-zero memory that begins immediately after the uninitialized data area grows upward (toward higher addresses). 
* For each process, the kernel maintians a variable `brk` pronounced "break" that points to the top of the heap.
* An allocator maintains the heap as a collection of various-size *blocks*. 
* Each block is a contiguous chunk of virtual memory that is either *allocated* or *free*. 
	* An allocated block has been explicitly reserved for use by the application. 
	* A free block is available to be allocated. It remains free until it is explicitly allocated by the application. 
* Allocators come in two basic styles. Both styles require the application to explicitly allocate blocks. They differ about which entity is responsible for freeing the allocated blocks.
	* *Explicit allocators* require the application to explicitly free any allocated blocks. For example, the C standard library provides an explicit allocator called the `malloc` package. C programs allocate a block by calling the `malloc` function and free .
	* *Implicit allocators*: require the allocator to detect when an allocated block is no longer beign used by the program and then free the block. Implicit allocators are also known as garbage collectors.
* The heap grows up and the stack grow sdown. 

### the `malloc` and `free` functions
* The C standard library provides an explicit allocator known as the `malloc` package. Programs allocate blocks from the heap by calling the `malloc` function. 
#### malloc
* The `malloc` function returns a pointer to a block of memory of at least `size` bytes that is suitabily aligned for any kind of object that might be contained in the block.
* In practice, the alignment depends on whether the code is compiled to run in 32-bit mode or 64-bit mode.
* If malloc encouters a problem, then it returns NULL and sets `errno`. 
* `malloc` does not initialize the memory it returns. Applications that want initialized dynamic memory can use `calloc`, a think wrapper around the `malloc` function that initializes the allocated memory to zero. 
* Applications that want to change the size of a previously allocated block can use the `realloc` function.
* Dynamic memory allocators such as `malloc`, can allocate or deallocate memory explicitly by using the `mmap` and `munmap` functions or they can use the `sbrk` function.

#### sbrk
```
#include <unistd.h>

void *sbrk(intptr_t incr);
```
* The `sbrk` function grows or shrinks the heap by adding incr to the kernel's `brk` pointer. If successful, it returns the old value of `brk`, otherwise it returns -1 and sets `errno` to ENOMEM. If `incr` is zero, then `sbrk` returns the current value of `brk`. Calling `sbrk` with a negative `incr` is legal but tricky because the return value (the oldv value of brk) points to abs(incr) bytes past the new top of the heap. 

#### free
* The `ptr` agrument must point to the beginning of an allocated block that was obtained from one of the dynamic allocation functions, whether it be `malloc`, `calloc`, or `realloc`
* If the argument does not point to the beginning of a block allocated on the heap, the behaviour of free is undefined. 
* Even worse, since it return nothing, it gives no indication to the application that something has gone wrong. 

## Why Dynamic Memory Allocation?
* Most important reason is that the size of certain data structures  may be unknown until runtime. 

#### Allocator requirements and goals
* *Handling arbitrary request sequences*: An application can make an arbitrary sequence of allocate and free requests, subject to the constraint that each free request must correspond to a currently allocated block obtained from a previous allocate request. Thus, **the allocator cannot make any assumptions about the ordering of the allocate and free requests. The allocator may not assume that each allocate request is accompanied by a matching free request, or that matching allocate and free requests are nested**
* *Making immediate responses to requests*. The allocator must respond immediately to allocate requests. Thus, the allocator is not allowed to reorder or buffer requests in order to improve performance.
* *Using only the heap*. In order for the allocator to be scalable, any nonscalar data structures used by the allocator must be stored in the heap itself.
* *Alinging blocks (alignment requirement)*: The allocator  must align blocks in sucha  way that they can hold any type of data object.
* *Not modify allocated blocks*: Allocators can only manipulate or change free blocks. In particular, they are not allowed to modify or move blocks once they ar eallocated. Thus, techniques such as compaction of allocated blocks are not permitted.

#### Goals
* Goal 1: Maximizing throughput. Throughput is defined as the number of requests that the allocator completes per unit time. For example, if an allocator completes 500 allocate and 500 free requests in 1 second, then its throughput is 1,000 operations per second. In general, we can maximuze throughput by minimizing the average time to satisfy allocate and free requests. 
* Goal 2: *Maximizing memory utilization*. Good programmers know that virtual memory is a finite resource and must be used efficiently. This is especially true for dynamic memory allocators that might be asked to allocate and free large blocks of memory. There are a number of ways to assess this, but a very useful metric is *peak utilization*

### Fragmentation
* The primary cause of poor heap utilization is a phenomenon known as fragmentation, which occurs when otherwise unused memory is not available to satisfy allocate requests.
* There are two forms of fragmentation: *internal fragmentation* and *external fragmentation*. 

#### Internal fragmentation
* Occurs when an allocated block is larger than the payload. This might happen for a number of reasons. For example, the implementation of an allocator might impose a minimum size on allocated blocks that is greater than some requested payload, or the allocator might increase the block size to satisfy an alignment constraint. 
* Straightfoward to quantify. It is simply the sum of the differences between the sizes of the allocated blocks and their payloads. Thus, at any point in time, the amount of internal fragmentaiton depends only on the pattern of previous requests and the allocator implementation. 

#### External Fragmentation
* Occurs when there *is* enough aggregrate free memory to satisfy the allocate request, but no single free block is large enough to handle the request.
* Much more difficult to quantify than internal fragmentation because it depends not only on the pattern of previous requests and the allocator implementation, but also on the pattern of future requests.
* Since external fragmentation is difficult to quantify and impossible to predict, allocators typically employ heuristics that attempt to maintain small numbers of larger free blocks rather than large numbers of smaller free blocks.

### Implementation Issues 
* A practical allocator that strikes a better balance between throughput and utilization must consider the following issues:
* *Free block organization*: How do we keep track of free blocks?
* *Placement*: How do we choose an appropriate free block in which to place a newly allocated block?
* *Splitting*: After we place a newly allocated block in some free block, what do we do with the remainder of the free block/
* *Coalescing*: What do we do with a block that has just been freed?

### Implicit Free Lists
* Any practical allocator needs some data structure tha tallows it to distinguish block boundaries and to distinguish between allocated and free blocks. 
* Most allocators embed this information in the blocks themselves. 
* An example could be:
	* A block consists of a one-word header, the payload, and possibly some additional *padding*. 
	* The header encodes the block size, as well as whether the block is allocated or free. If we impose a double-word alignment constraint, then the block size is always a multiple of 8, and the 3 low-order bits of the block size are always zero. Thus, we need to store only the 29 high-order bits of the block size, freeing the remaining 3 bits to encode other information. 
	* In this case, we are using the least significant of these bits to indicate whether the block is allocated or free. 
* There are many potential reasons for padding: it may be part of the allocator's strategy for combating external fragmentation. Or it might be needed to satisfy the alignment requirement.
* An implicit free list involves organizing the heap as a sequence of contiguous allocated and free blocks. It's called an "implciit" free list, because the allocator can indirectly traverse all free blocks by traversing all blocks in the heap. 
	* You need some kind of specially marked end block
* **Advantage**: simplcity.
* **Disadvantage**: The cost of any operation that requires a search of the free list, such as placing allocated blocks, will be linear to the total number of allocated and free blocks in the heap.
* It is important to realize that the system's alignment requirement and the allocator's choice of block format impose a minmum block size on the allocator. Even if the application were to request a single byte, the allocator would still create a two-word blocl.

#### Placing allocated blocks
* When an application requests a block of k bytes, the allocator searches the free list for the free block that is large enough to hold the requested block. 
* The manner i which the allocator performs this search is determined by the placement policy:
	* First fit: Searches the free list from the beginning and chooses the first free block that fits
		* Advantage: retains large free blocks at the end of the list.
		* Disadvantage: tends to leave "splinters" of small free blocks towards the beginning of the list, which will increase the search time for larger blocks.
	* Next fit: similar to first fit, but instead of starting each search at the beginning of the list, it starts the search where the previous search left off.
		* Can run significantly faster than first fit, espeically if the front of the list becomes littered with many small splinters.
		* Suffers from worse emmory utilization than first fit. 
	* Best fit: Examines every free block and chooses the free block with the smallest size that fits. 
		* Generally has best memory utilization. 
		* Disadvantage is that it requires an exhaustive search of the heap.

#### Splitting Free Blocks
* Once the allocator has located a free block that fits, it must make another policiy decision about how much of the free block to allocate.
	* Use entire free block: simple and fast, but introduces internal fragmentation. If the placement policy tends to produce good fits, then some additional internal fragmentation might be acceptable, however if the fit is not good, the allocator wiill usually opt to split the free blocks into two parts.

#### Getting Additional Heap Memory
* What happens if the allocator is unable to find a fit for the requested block? 
* One option is to merge adjacent free blocks, however, if this does not yield a sufficiently large block, or if the free blocks are already maximally coalesced, then the allocator asskt he kernel for additional heap memory by calling the `sbrk` funtion. The allocator transforms the additional memory into one large free block, inserts the block into the free list, and  then places the requested block in this new free block.

#### Coalescing Free Blocks
* When the allocator frees an allocated block, there might be other free blocks that are adjacent to the newly freed block. 
* Such adjacent free blocks can ccause a phenomenon known as *false fragmentation*, where there is a lot of available free memory in adjacent positions, just chopped up itnto chunkss, 
* Any pracitcal allocator must merge adjacent free blocks.
* Fast allocators often opt for some form of deferred coalescing.

#### Coalescing with Boundary Tags
* A clever and generatl technique developed by Knuth, used when you want to coalescing the previous block
* Allows for constant-time coalescing of the previous block.
* You need this because 
* Add a *footer* at the end of each block, where the footer is a replica of the header.
* If each block includes such a footer, then the allocator can determine the starting location and status of the previous block by inspecting its footer,w hich is always one word away from the start of the current block.
* Consider all the cases that can exit when the allocator frees the current block: 
	* The previous and next blocks are both allocated
		* Both adjacent blocks ar eallocated and no coalescing is possible.
		* Status of current block is simply changed from allocated to free.
	* The previous block is allocated and the next block is free
		* Current block merged with the next block.
		* Header of the current block and footeer of the next block are updated with the combined sizes of current and next blocks.
	* The previous block is free and the next block is allocated.
		* Current block is merged with the previous block.
		* Header of the previous block and footer of the current block are updated with the combined sizes of the current and next blocks.
	* The previous and next blocks are both free.
		* All three blocks are merged to form a single free block, with the header of the previous block and the footer of the next block updated with the combined sizes of the free blocks.
		* In each case, the coalescing is performed in constant time.
* Potential disadvantage: Requiring each block to contain a header and a footer can introduce significant memory overhead.
* Clever optimization: if w ewere to store the allocated/free bit of the previous block in one of the excess low-order bits of the current block, then allocated blocks would not need footers and we oculd use the extra space for the payload. Note however, that free blocks would still need footers.
