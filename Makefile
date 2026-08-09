heap.o: heap.c heap_internal.h 
	gcc -c heap.c -o heap.o 

main.o: main.c heap_internal.h
	gcc -c main.c -o main.o

utils.o: utils.c utils.h 
	gcc -c utils.c -o utils.o

secure_utils.o: secure_utils.c secure_utils.h
	gcc -c secure_utils.c -o secure_utils.o

util_test.o: utils_test.c utils.h
	gcc -c utils_test.c -o util_test.o 

slab_slot_test.o: slab_slot_test.c heap_internal.h utils.h
	gcc -c slab_slot_test.c -o slab_slot_test.o

addr_map.o: addr_map.c addr_map.h
	gcc -c -g addr_map.c -o addr_map.o

addr_map_test.o: addr_map_test.c addr_map.h heap_internal.h
	gcc -c addr_map_test.c -o addr_map_test.o

mechanics_test.o: mechanics_test.c heap_internal.h utils.h
	gcc -c mechanics_test.c -o mechanics_test.o

fast_slab_overflow_test.o: fast_slab_overflow_test.c heap_internal.h
	gcc -c fast_slab_overflow_test.c -o fast_slab_overflow_test.o

large_allocations_test.o: large_allocations_test.c large_allocations.h addr_map.h ring_cache.h
	gcc -c -g large_allocations_test.c -o large_allocations_test.o

large_allocations.o: large_allocations.c large_allocations.h addr_map.h ring_cache.h secure_utils.h
	gcc -c -g large_allocations.c -o large_allocations.o

ring_cache.o: ring_cache.c ring_cache.h secure_utils.h addr_map.h
	gcc -c -g ring_cache.c -o ring_cache.o

double_free_test.o: double_free_test.c heap_internal.h
	gcc -c -g double_free_test.c -o double_free_test.o

overflow_test.o: overflow_test.c heap_internal.h
	gcc -c -g overflow_test.c -o overflow_test.o

large_use_after_free_test.o: large_use_after_free_test.c heap_internal.h
	gcc -c -g large_use_after_free_test.c -o large_use_after_free_test.o

slab_quarantine.o: slab_quarantine.c slab_quarantine.h secure_utils.h
	gcc -c -g slab_quarantine.c -o slab_quarantine.o

slab_quarantine_test.o: slab_quarantine_test.c slab_quarantine.h
	gcc -c -g slab_quarantine_test.c -o slab_quarantine_test.o

slab_quarantine_integration_test.o: slab_quarantine_integration_test.c heap_internal.h slab_quarantine.h
	gcc -c -g slab_quarantine_integration_test.c -o slab_quarantine_integration_test.o

addr_map_test: addr_map.o addr_map_test.o utils.o secure_utils.o
	gcc addr_map.o addr_map_test.o utils.o secure_utils.o -o addr_map_test

mechanics_test: heap.o addr_map.o utils.o mechanics_test.o secure_utils.o
	gcc mechanics_test.o heap.o addr_map.o utils.o secure_utils.o -o mechanics_test

main: heap.o main.o utils.o 
	gcc main.o heap.o utils.o -o main

util_test: utils.o util_test.o
	gcc util_test.o utils.o -o util_test

slab_slot_test: heap.o utils.o slab_slot_test.o
	gcc slab_slot_test.o heap.o utils.o -o slab_slot_test

fast_slab_overflow_test: heap.o addr_map.o utils.o fast_slab_overflow_test.o secure_utils.o
	gcc -g fast_slab_overflow_test.o heap.o addr_map.o utils.o secure_utils.o -o fast_slab_overflow_test

large_allocations_test: large_allocations_test.o large_allocations.o ring_cache.o addr_map.o secure_utils.o utils.o
	gcc -g large_allocations_test.o large_allocations.o ring_cache.o addr_map.o secure_utils.o utils.o -o large_allocations_test

double_free_test: double_free_test.o heap.o large_allocations.o ring_cache.o addr_map.o secure_utils.o utils.o
	gcc -g double_free_test.o heap.o large_allocations.o ring_cache.o addr_map.o secure_utils.o utils.o -o double_free_test

overflow_test: overflow_test.o heap.o large_allocations.o ring_cache.o addr_map.o secure_utils.o utils.o
	gcc -g overflow_test.o heap.o large_allocations.o ring_cache.o addr_map.o secure_utils.o utils.o -o overflow_test

large_use_after_free_test: large_use_after_free_test.o heap.o large_allocations.o ring_cache.o addr_map.o secure_utils.o utils.o
	gcc -g large_use_after_free_test.o heap.o large_allocations.o ring_cache.o addr_map.o secure_utils.o utils.o -o large_use_after_free_test

slab_quarantine_test: slab_quarantine_test.o slab_quarantine.o secure_utils.o utils.o
	gcc -g slab_quarantine_test.o slab_quarantine.o secure_utils.o utils.o -o slab_quarantine_test

slab_quarantine_integration_test: slab_quarantine_integration_test.o heap.o slab_quarantine.o large_allocations.o ring_cache.o addr_map.o secure_utils.o utils.o
	gcc -g slab_quarantine_integration_test.o heap.o slab_quarantine.o large_allocations.o ring_cache.o addr_map.o secure_utils.o utils.o -o slab_quarantine_integration_test

test: slab_slot_test
	./slab_slot_test

fast-test: fast_slab_overflow_test
	./fast_slab_overflow_test

large-test: large_allocations_test
	./large_allocations_test

double-free-test: double_free_test
	./double_free_test

stress-test: overflow_test
	./overflow_test

large-uaf-test: large_use_after_free_test
	./large_use_after_free_test

quarantine-test: slab_quarantine_test
	./slab_quarantine_test

quarantine-integration-test: slab_quarantine_integration_test
	./slab_quarantine_integration_test
