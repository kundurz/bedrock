# Heap Fundamentals

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
