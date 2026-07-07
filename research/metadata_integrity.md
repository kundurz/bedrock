### Metadata segregation 
* You must completely remove size headers and free list pointers from the user data chunks.
* Create a dedicated memory region that is entirely separate. This region holds an array of metadata descriptors.
* Do not share a pointer from the data chunk to the metadata. Instead, use math. 
* If your object pool starts at address `0x7000...0`, an object at index `0x7000..020` is at index 2. Look up index 2 in your metadata array. 

### Guard Pages & VMM Protections
* Page-level isolation: Place your metadata array in a memory region surrounded by unmapped virtual memory pages (Guard Pages).
* Exploit Mitigation: If an attacker triggers a huge sequential overflow in the data in the heap, they will hit a guard page and crash before they can even reach your metadata memory space.
* 


