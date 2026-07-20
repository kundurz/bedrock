# Hash Tables

## Hashing
A hash function is a mapping `h(K)` that maps a key to an address of a slot on the table. 

## Using hashes for tables
Hashing techniques are almost always used in cases where there are many more distinct keys then there are table addresses.

When the hashes of two keys correspond to the same address in the table (`h(K1) = h(K2)`) what occurs is called a collision.

To deal with collisions you need what is called a collision resolution policy. Two examples of this are:
1) open addressing 
2) seperate chaining

## Open Addressing
This collision resolution method in which table entries containing keys are placed in open locations in the hash table. When a collision occurs a systematic search is conducted among other table locations to find an open address (or empty entry).


## Seperate Chaining
This collision resolution method uses a linked list (or chain) at each hash table index. When a collision occurs, the new key-value pair is simply added to the chain at that index.


