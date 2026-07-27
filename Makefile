heap.o: heap.c heap_internal.h 
	gcc -c heap.c -o heap.o 

main.o: main.c heap_internal.h
	gcc -c main.c -o main.o

utils.o: utils.c utils.h 
	gcc -c utils.c -o utils.o

util_test.o: utils_test.c utils.h
	gcc -c utils_test.c -o util_test.o 

slab_slot_test.o: slab_slot_test.c heap_internal.h utils.h
	gcc -c slab_slot_test.c -o slab_slot_test.o

addr_map.o: addr_map.c addr_map.h
	gcc -c addr_map.c -o addr_map.o

addr_map_test.o: addr_map_test.c addr_map.h heap_internal.h
	gcc -c addr_map_test.c -o addr_map_test.o

mechanics_test.o: mechanics_test.c heap_internal.h utils.h
	gcc -c mechanics_test.c -o mechanics_test.o

fast_slab_overflow_test.o: fast_slab_overflow_test.c heap_internal.h
	gcc -c fast_slab_overflow_test.c -o fast_slab_overflow_test.o

addr_map_test: addr_map.o addr_map_test.o utils.o
	gcc addr_map.o addr_map_test.o utils.o -o addr_map_test

mechanics_test: heap.o addr_map.o utils.o mechanics_test.o
	gcc mechanics_test.o heap.o addr_map.o utils.o -o mechanics_test

main: heap.o main.o utils.o 
	gcc main.o heap.o utils.o -o main

util_test: utils.o util_test.o
	gcc util_test.o utils.o -o util_test

slab_slot_test: heap.o utils.o slab_slot_test.o
	gcc slab_slot_test.o heap.o utils.o -o slab_slot_test

fast_slab_overflow_test: heap.o addr_map.o utils.o fast_slab_overflow_test.o
	gcc fast_slab_overflow_test.o heap.o addr_map.o utils.o -o fast_slab_overflow_test

test: slab_slot_test
	./slab_slot_test

fast-test: fast_slab_overflow_test
	./fast_slab_overflow_test
