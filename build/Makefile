CC = gcc
CFLAGS = -O3 -g -Wall -Wextra -I.. -I../include

OBJECTS = obj/main.o \
	obj/heap.o \
	obj/addr_map.o \
	obj/large_allocations.o \
	obj/ring_cache.o \
	obj/secure_utils.o \
	obj/slab_quarantine.o \
	obj/utils.o

.PHONY: all clean

all: main

main: $(OBJECTS)
	$(CC) $(OBJECTS) -o main

sanity_test: obj/sanity_test.o obj/heap.o obj/addr_map.o \
	obj/large_allocations.o obj/ring_cache.o obj/secure_utils.o \
	obj/slab_quarantine.o obj/utils.o
	$(CC) obj/sanity_test.o obj/heap.o obj/addr_map.o \
		obj/large_allocations.o obj/ring_cache.o obj/secure_utils.o \
		obj/slab_quarantine.o obj/utils.o -o sanity_test

obj/sanity_test.o: ../tests/sanity_test.c ../include/heap.h
	mkdir -p obj
	$(CC) $(CFLAGS) -c ../tests/sanity_test.c -o obj/sanity_test.o

correct_alignment_test: obj/correct_alignment_test.o obj/heap.o obj/addr_map.o \
	obj/large_allocations.o obj/ring_cache.o obj/secure_utils.o \
	obj/slab_quarantine.o obj/utils.o
	$(CC) obj/correct_alignment_test.o obj/heap.o obj/addr_map.o \
		obj/large_allocations.o obj/ring_cache.o obj/secure_utils.o \
		obj/slab_quarantine.o obj/utils.o -o correct_alignment_test

obj/correct_alignment_test.o: ../tests/correct_alignment_test.c ../include/heap.h
	mkdir -p obj
	$(CC) $(CFLAGS) -c ../tests/correct_alignment_test.c \
		-o obj/correct_alignment_test.o

read_write_test: obj/read_write_test.o obj/heap.o obj/addr_map.o \
	obj/large_allocations.o obj/ring_cache.o obj/secure_utils.o \
	obj/slab_quarantine.o obj/utils.o
	$(CC) obj/read_write_test.o obj/heap.o obj/addr_map.o \
		obj/large_allocations.o obj/ring_cache.o obj/secure_utils.o \
		obj/slab_quarantine.o obj/utils.o -o read_write_test

obj/read_write_test.o: ../tests/read_write_test.c ../include/heap.h
	mkdir -p obj
	$(CC) $(CFLAGS) -c ../tests/read_write_test.c -o obj/read_write_test.o

overlap_test: obj/overlap_test.o obj/heap.o obj/addr_map.o \
	obj/large_allocations.o obj/ring_cache.o obj/secure_utils.o \
	obj/slab_quarantine.o obj/utils.o
	$(CC) obj/overlap_test.o obj/heap.o obj/addr_map.o \
		obj/large_allocations.o obj/ring_cache.o obj/secure_utils.o \
		obj/slab_quarantine.o obj/utils.o -o overlap_test

obj/overlap_test.o: ../tests/overlap_test.c ../include/heap.h
	mkdir -p obj
	$(CC) $(CFLAGS) -c ../tests/overlap_test.c -o obj/overlap_test.o

free_tests: obj/free_tests.o obj/heap.o obj/addr_map.o \
	obj/large_allocations.o obj/ring_cache.o obj/secure_utils.o \
	obj/slab_quarantine.o obj/utils.o
	$(CC) obj/free_tests.o obj/heap.o obj/addr_map.o \
		obj/large_allocations.o obj/ring_cache.o obj/secure_utils.o \
		obj/slab_quarantine.o obj/utils.o -o free_tests

obj/free_tests.o: ../tests/free_tests.c ../include/heap.h
	mkdir -p obj
	$(CC) $(CFLAGS) -c ../tests/free_tests.c -o obj/free_tests.o

page_protection_test: obj/page_protection_test.o obj/heap.o obj/addr_map.o \
	obj/large_allocations.o obj/ring_cache.o obj/secure_utils.o \
	obj/slab_quarantine.o obj/utils.o
	$(CC) obj/page_protection_test.o obj/heap.o obj/addr_map.o \
		obj/large_allocations.o obj/ring_cache.o obj/secure_utils.o \
		obj/slab_quarantine.o obj/utils.o -o page_protection_test

obj/page_protection_test.o: ../tests/page_protection_test.c ../include/heap.h
	mkdir -p obj
	$(CC) $(CFLAGS) -c ../tests/page_protection_test.c \
		-o obj/page_protection_test.o

reuse_test: obj/reuse_test.o obj/heap.o obj/addr_map.o \
	obj/large_allocations.o obj/ring_cache.o obj/secure_utils.o \
	obj/slab_quarantine.o obj/utils.o
	$(CC) obj/reuse_test.o obj/heap.o obj/addr_map.o \
		obj/large_allocations.o obj/ring_cache.o obj/secure_utils.o \
		obj/slab_quarantine.o obj/utils.o -o reuse_test

obj/reuse_test.o: ../tests/reuse_test.c ../include/heap.h
	mkdir -p obj
	$(CC) $(CFLAGS) -c ../tests/reuse_test.c -o obj/reuse_test.o

mixed_workload: obj/mixed_workload.o obj/heap.o obj/addr_map.o \
	obj/large_allocations.o obj/ring_cache.o obj/secure_utils.o \
	obj/slab_quarantine.o obj/utils.o
	$(CC) obj/mixed_workload.o obj/heap.o obj/addr_map.o \
		obj/large_allocations.o obj/ring_cache.o obj/secure_utils.o \
		obj/slab_quarantine.o obj/utils.o -o mixed_workload

obj/mixed_workload.o: ../tests/mixed_workload.c ../include/heap.h
	mkdir -p obj
	$(CC) $(CFLAGS) -c ../tests/mixed_workload.c -o obj/mixed_workload.o

throughput: obj/throughput.o obj/heap.o obj/addr_map.o \
	obj/large_allocations.o obj/ring_cache.o obj/secure_utils.o \
	obj/slab_quarantine.o obj/utils.o
	$(CC) obj/throughput.o obj/heap.o obj/addr_map.o \
		obj/large_allocations.o obj/ring_cache.o obj/secure_utils.o \
		obj/slab_quarantine.o obj/utils.o -o throughput

obj/throughput.o: ../benchmarking/throughput/throughput.c ../include/heap.h
	mkdir -p obj
	$(CC) $(CFLAGS) -c ../benchmarking/throughput/throughput.c -o obj/throughput.o

malloc_throughput: obj/malloc_throughput.o
	$(CC) obj/malloc_throughput.o -o malloc_throughput

obj/malloc_throughput.o: ../benchmarking/throughput/malloc_throughput.c
	mkdir -p obj
	$(CC) $(CFLAGS) -c ../benchmarking/throughput/malloc_throughput.c \
		-o obj/malloc_throughput.o

obj/main.o: ../tests/main.c ../include/heap_internal.h
	mkdir -p obj
	$(CC) $(CFLAGS) -c ../tests/main.c -o obj/main.o

obj/heap.o: ../src/heap.c ../include/heap_internal.h ../include/utils.h \
	../include/addr_map.h ../include/ring_cache.h \
	../include/large_allocations.h ../include/slab_quarantine.h \
	../include/secure_utils.h
	mkdir -p obj
	$(CC) $(CFLAGS) -c ../src/heap.c -o obj/heap.o

obj/addr_map.o: ../src/addr_map.c ../include/addr_map.h \
	../include/heap_internal.h ../include/utils.h ../include/secure_utils.h
	mkdir -p obj
	$(CC) $(CFLAGS) -c ../src/addr_map.c -o obj/addr_map.o

obj/large_allocations.o: ../src/large_allocations.c \
	../include/large_allocations.h ../include/addr_map.h \
	../include/ring_cache.h ../include/secure_utils.h
	mkdir -p obj
	$(CC) $(CFLAGS) -c ../src/large_allocations.c -o obj/large_allocations.o

obj/ring_cache.o: ../src/ring_cache.c ../include/ring_cache.h \
	../include/addr_map.h ../include/secure_utils.h
	mkdir -p obj
	$(CC) $(CFLAGS) -c ../src/ring_cache.c -o obj/ring_cache.o

obj/secure_utils.o: ../src/secure_utils.c ../include/secure_utils.h \
	../include/utils.h
	mkdir -p obj
	$(CC) $(CFLAGS) -c ../src/secure_utils.c -o obj/secure_utils.o

obj/slab_quarantine.o: ../src/slab_quarantine.c \
	../include/slab_quarantine.h ../include/secure_utils.h
	mkdir -p obj
	$(CC) $(CFLAGS) -c ../src/slab_quarantine.c -o obj/slab_quarantine.o

obj/utils.o: ../src/utils.c ../include/utils.h
	mkdir -p obj
	$(CC) $(CFLAGS) -c ../src/utils.c -o obj/utils.o

clean:
	rm -rf obj main sanity_test correct_alignment_test read_write_test overlap_test \
		free_tests page_protection_test reuse_test mixed_workload throughput \
		malloc_throughput
