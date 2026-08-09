# Design Features

## Fast Bins
* This is for allocations between 0 B - 2048 B (inclusive)
* Each allocation will fall into a  fixed-size bin so each allocation gets **at least** as much as it asks for. 
* The size classes (in bytes) are as follows:
  * 16 B 
  * 32 B
  * 64 B
  * 128 B
  * 256 B
  * 512 B 
  * 1024 B
  * 2048 B
* Having fixed size classes means that the size of each fast bin is known, which improves allocator throughput at the cost of some internal fragmentation. 
* Each fast bin functions similar to a stack data structure.
* When a bin is empty, a new page is allocated using `mmap`, and the page is divided into fixed-size chunks which will be used to populate the free list. This was both simple to implement and has added security benefit.
  
### Allocations and frees
* When `heap_alloc` is called, it pops a free chunk off of the relevant fast bin. 
* When `heap_free` is called, it pushes the free chunk to the head of its corresponding fast bin's free list.
* This policy was chosen because of its simplicity and the fact that it allows allocations and frees to be performed in `O(1)` (constant time), assuming the free list does not need to be re-populated. 

## Large Bins
* For large allocations (greater than 2048 B), chunks will be taken from the large bin
* Unlike fast allocations, large allocations usually give the user the exact space they ask for. If the leftover space can't hold another `struct large_chunk`, it is too small and the user effectively gets the whole chunk. 
* This is done to mitigate internal fragmentation which can become more costly for larger allocations.

### Allocations and frees
#### Allocation
* Upon a call to `heap_alloc` first-fit search through the large bin free list is performed.
* If there is not enough memory in the large bin (due to the absence of enough contiguous free memory), a new chunk (rounded to the next page boundary) is added to the free list. 
* Once a valid free chunk is found, it is split if the remainder can hold another large chunk header.
#### Free
* Note: The large bin free list order is independent of physical memory order. Coalescing uses physical neighbor metadata and span bounds, not fd/bk list adjacency.
* Upon a call to `heap_free`, the chunk is returned to the head of the large bin free list.
* It then checks the physically adjacent chunks on the left and right...
* If chunks are coalesced together, the one with the lowest address in memory involved in the merge is kept in the free list, with its metadata updated to reflect the merge. The other chunks are unlinked from the free list.