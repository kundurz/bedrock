CC = gcc
AR = ar
CFLAGS = -O3 -g -Wall -Wextra -I.. -I../include
PIC_CFLAGS = $(CFLAGS) -fPIC
STATS_CFLAGS = $(CFLAGS) -DHEAP_ENABLE_STATS

.DEFAULT_GOAL := all

.PHONY: all libraries help test tests examples clean

# Aggregate commands

all: libraries

libraries: libbedrock.a libbedrock.so

help:
	@echo "make           Build libbedrock.a and libbedrock.so"
	@echo "make libraries Build libbedrock.a and libbedrock.so"
	@echo "make tests     Build and run all tests"
	@echo "make examples  Build and run all examples"
	@echo "make clean     Remove all build artifacts"

test: tests

tests:
	$(MAKE) -B sanity_test correct_alignment_test read_write_test overlap_test \
		free_tests page_protection_test reuse_test mixed_workload
	test_status=0; \
	./sanity_test || test_status=1; \
	./correct_alignment_test || test_status=1; \
	./read_write_test || test_status=1; \
	./overlap_test || test_status=1; \
	./free_tests || test_status=1; \
	./page_protection_test || test_status=1; \
	./reuse_test || test_status=1; \
	./mixed_workload || test_status=1; \
	exit $$test_status

examples:
	$(MAKE) -B use_after_free_demo large_allocation_overflow_demo
	example_status=0; \
	./use_after_free_demo || example_status=1; \
	./large_allocation_overflow_demo || example_status=1; \
	exit $$example_status

# Libraries

libbedrock.a: obj/bedrock.o obj/heap.o obj/addr_map.o \
	obj/large_allocations.o obj/ring_cache.o obj/secure_utils.o \
	obj/slab_quarantine.o obj/slab_metadata_allocator.o obj/utils.o
	$(AR) rcs libbedrock.a obj/bedrock.o obj/heap.o obj/addr_map.o \
		obj/large_allocations.o obj/ring_cache.o obj/secure_utils.o \
		obj/slab_quarantine.o obj/slab_metadata_allocator.o obj/utils.o

libbedrock.so: obj/pic/bedrock.o obj/pic/heap.o obj/pic/addr_map.o \
	obj/pic/large_allocations.o obj/pic/ring_cache.o obj/pic/secure_utils.o \
	obj/pic/slab_quarantine.o obj/pic/slab_metadata_allocator.o \
	obj/pic/utils.o
	$(CC) -shared obj/pic/bedrock.o obj/pic/heap.o obj/pic/addr_map.o \
		obj/pic/large_allocations.o obj/pic/ring_cache.o \
		obj/pic/secure_utils.o obj/pic/slab_quarantine.o \
		obj/pic/slab_metadata_allocator.o obj/pic/utils.o \
		-o libbedrock.so

# Examples

use_after_free_demo: obj/use_after_free_demo.o libbedrock.so
	$(CC) obj/use_after_free_demo.o -L. -lbedrock \
		-Wl,-rpath,'$$ORIGIN' -o use_after_free_demo

obj/use_after_free_demo.o: ../examples/use_after_free_demo.c \
	../include/bedrock.h
	mkdir -p obj
	$(CC) -g -Wall -Wextra -I.. -I../include \
		-c ../examples/use_after_free_demo.c \
		-o obj/use_after_free_demo.o

obj/bedrock.o: ../src/bedrock.c ../include/bedrock.h ../include/heap_internal.h
	mkdir -p obj
	$(CC) $(CFLAGS) -c ../src/bedrock.c -o obj/bedrock.o

large_allocation_overflow_demo: obj/large_allocation_overflow_demo.o \
	libbedrock.so
	$(CC) obj/large_allocation_overflow_demo.o -L. -lbedrock \
		-Wl,-rpath,'$$ORIGIN' -o large_allocation_overflow_demo

obj/large_allocation_overflow_demo.o: \
	../examples/large_allocation_overflow_demo.c ../include/bedrock.h
	mkdir -p obj
	$(CC) -g -Wall -Wextra -I.. -I../include \
		-c ../examples/large_allocation_overflow_demo.c \
		-o obj/large_allocation_overflow_demo.o

# Tests

sanity_test: obj/sanity_test.o obj/heap.o obj/addr_map.o \
	obj/large_allocations.o obj/ring_cache.o obj/secure_utils.o \
	obj/slab_quarantine.o obj/slab_metadata_allocator.o obj/utils.o
	$(CC) obj/sanity_test.o obj/heap.o obj/addr_map.o \
		obj/large_allocations.o obj/ring_cache.o obj/secure_utils.o \
		obj/slab_quarantine.o obj/slab_metadata_allocator.o obj/utils.o \
		-o sanity_test

obj/sanity_test.o: ../tests/sanity_test.c ../include/heap_internal.h
	mkdir -p obj
	$(CC) $(CFLAGS) -c ../tests/sanity_test.c -o obj/sanity_test.o

correct_alignment_test: obj/correct_alignment_test.o obj/heap.o obj/addr_map.o \
	obj/large_allocations.o obj/ring_cache.o obj/secure_utils.o \
	obj/slab_quarantine.o obj/slab_metadata_allocator.o obj/utils.o
	$(CC) obj/correct_alignment_test.o obj/heap.o obj/addr_map.o \
		obj/large_allocations.o obj/ring_cache.o obj/secure_utils.o \
		obj/slab_quarantine.o obj/slab_metadata_allocator.o obj/utils.o \
		-o correct_alignment_test

obj/correct_alignment_test.o: ../tests/correct_alignment_test.c \
	../include/heap_internal.h
	mkdir -p obj
	$(CC) $(CFLAGS) -c ../tests/correct_alignment_test.c \
		-o obj/correct_alignment_test.o

read_write_test: obj/read_write_test.o obj/heap.o obj/addr_map.o \
	obj/large_allocations.o obj/ring_cache.o obj/secure_utils.o \
	obj/slab_quarantine.o obj/slab_metadata_allocator.o obj/utils.o
	$(CC) obj/read_write_test.o obj/heap.o obj/addr_map.o \
		obj/large_allocations.o obj/ring_cache.o obj/secure_utils.o \
		obj/slab_quarantine.o obj/slab_metadata_allocator.o obj/utils.o \
		-o read_write_test

obj/read_write_test.o: ../tests/read_write_test.c ../include/heap_internal.h
	mkdir -p obj
	$(CC) $(CFLAGS) -c ../tests/read_write_test.c -o obj/read_write_test.o

overlap_test: obj/overlap_test.o obj/heap.o obj/addr_map.o \
	obj/large_allocations.o obj/ring_cache.o obj/secure_utils.o \
	obj/slab_quarantine.o obj/slab_metadata_allocator.o obj/utils.o
	$(CC) obj/overlap_test.o obj/heap.o obj/addr_map.o \
		obj/large_allocations.o obj/ring_cache.o obj/secure_utils.o \
		obj/slab_quarantine.o obj/slab_metadata_allocator.o obj/utils.o \
		-o overlap_test

obj/overlap_test.o: ../tests/overlap_test.c ../include/heap_internal.h
	mkdir -p obj
	$(CC) $(CFLAGS) -c ../tests/overlap_test.c -o obj/overlap_test.o

free_tests: obj/free_tests.o obj/heap.o obj/addr_map.o \
	obj/large_allocations.o obj/ring_cache.o obj/secure_utils.o \
	obj/slab_quarantine.o obj/slab_metadata_allocator.o obj/utils.o
	$(CC) obj/free_tests.o obj/heap.o obj/addr_map.o \
		obj/large_allocations.o obj/ring_cache.o obj/secure_utils.o \
		obj/slab_quarantine.o obj/slab_metadata_allocator.o obj/utils.o \
		-o free_tests

obj/free_tests.o: ../tests/free_tests.c ../include/heap_internal.h
	mkdir -p obj
	$(CC) $(CFLAGS) -c ../tests/free_tests.c -o obj/free_tests.o

page_protection_test: obj/page_protection_test.o obj/heap.o obj/addr_map.o \
	obj/large_allocations.o obj/ring_cache.o obj/secure_utils.o \
	obj/slab_quarantine.o obj/slab_metadata_allocator.o obj/utils.o
	$(CC) obj/page_protection_test.o obj/heap.o obj/addr_map.o \
		obj/large_allocations.o obj/ring_cache.o obj/secure_utils.o \
		obj/slab_quarantine.o obj/slab_metadata_allocator.o obj/utils.o \
		-o page_protection_test

obj/page_protection_test.o: ../tests/page_protection_test.c \
	../include/heap_internal.h
	mkdir -p obj
	$(CC) $(CFLAGS) -c ../tests/page_protection_test.c \
		-o obj/page_protection_test.o

reuse_test: obj/reuse_test.o obj/heap.o obj/addr_map.o \
	obj/large_allocations.o obj/ring_cache.o obj/secure_utils.o \
	obj/slab_quarantine.o obj/slab_metadata_allocator.o obj/utils.o
	$(CC) obj/reuse_test.o obj/heap.o obj/addr_map.o \
		obj/large_allocations.o obj/ring_cache.o obj/secure_utils.o \
		obj/slab_quarantine.o obj/slab_metadata_allocator.o obj/utils.o \
		-o reuse_test

obj/reuse_test.o: ../tests/reuse_test.c ../include/heap_internal.h
	mkdir -p obj
	$(CC) $(CFLAGS) -c ../tests/reuse_test.c -o obj/reuse_test.o

mixed_workload: obj/mixed_workload.o obj/heap.o obj/addr_map.o \
	obj/large_allocations.o obj/ring_cache.o obj/secure_utils.o \
	obj/slab_quarantine.o obj/slab_metadata_allocator.o obj/utils.o
	$(CC) obj/mixed_workload.o obj/heap.o obj/addr_map.o \
		obj/large_allocations.o obj/ring_cache.o obj/secure_utils.o \
		obj/slab_quarantine.o obj/slab_metadata_allocator.o obj/utils.o \
		-o mixed_workload

obj/mixed_workload.o: ../tests/mixed_workload.c ../include/heap_internal.h
	mkdir -p obj
	$(CC) $(CFLAGS) -c ../tests/mixed_workload.c -o obj/mixed_workload.o

# Benchmarks

throughput: obj/throughput.o obj/heap.o obj/addr_map.o \
	obj/large_allocations.o obj/ring_cache.o obj/secure_utils.o \
	obj/slab_quarantine.o obj/slab_metadata_allocator.o obj/utils.o
	$(CC) obj/throughput.o obj/heap.o obj/addr_map.o \
		obj/large_allocations.o obj/ring_cache.o obj/secure_utils.o \
		obj/slab_quarantine.o obj/slab_metadata_allocator.o obj/utils.o \
		-o throughput

obj/throughput.o: ../benchmarking/throughput/throughput.c \
	../include/heap_internal.h
	mkdir -p obj
	$(CC) $(CFLAGS) -c ../benchmarking/throughput/throughput.c -o obj/throughput.o

malloc_throughput: obj/malloc_throughput.o
	$(CC) obj/malloc_throughput.o -o malloc_throughput

obj/malloc_throughput.o: ../benchmarking/throughput/malloc_throughput.c
	mkdir -p obj
	$(CC) $(CFLAGS) -c ../benchmarking/throughput/malloc_throughput.c \
		-o obj/malloc_throughput.o

latency: obj/latency.o obj/heap.o obj/addr_map.o \
	obj/large_allocations.o obj/ring_cache.o obj/secure_utils.o \
	obj/slab_quarantine.o obj/slab_metadata_allocator.o obj/utils.o
	$(CC) obj/latency.o obj/heap.o obj/addr_map.o \
		obj/large_allocations.o obj/ring_cache.o obj/secure_utils.o \
		obj/slab_quarantine.o obj/slab_metadata_allocator.o obj/utils.o \
		-o latency

obj/latency.o: ../benchmarking/latency/latency.c \
	../include/heap_internal.h
	mkdir -p obj
	$(CC) $(CFLAGS) -c ../benchmarking/latency/latency.c -o obj/latency.o

malloc_latency: obj/malloc_latency.o
	$(CC) obj/malloc_latency.o -o malloc_latency

obj/malloc_latency.o: ../benchmarking/latency/malloc_latency.c
	mkdir -p obj
	$(CC) $(CFLAGS) -c ../benchmarking/latency/malloc_latency.c \
		-o obj/malloc_latency.o

fragmentation: obj/fragmentation.o obj/stats_heap.o obj/stats_addr_map.o \
	obj/stats_large_allocations.o obj/stats_ring_cache.o \
	obj/stats_slab_quarantine.o obj/stats_slab_metadata_allocator.o \
	obj/heap_stats.o obj/secure_utils.o obj/utils.o
	$(CC) obj/fragmentation.o obj/stats_heap.o obj/stats_addr_map.o \
		obj/stats_large_allocations.o obj/stats_ring_cache.o \
		obj/stats_slab_quarantine.o obj/stats_slab_metadata_allocator.o \
		obj/heap_stats.o obj/secure_utils.o obj/utils.o -o fragmentation

obj/fragmentation.o: ../benchmarking/fragmentation/fragmentation.c \
	../include/heap_internal.h ../include/heap_stats.h
	mkdir -p obj
	$(CC) $(STATS_CFLAGS) -c ../benchmarking/fragmentation/fragmentation.c \
		-o obj/fragmentation.o

obj/stats_heap.o: ../src/heap.c ../include/heap_internal.h \
	../include/heap_stats.h
	mkdir -p obj
	$(CC) $(STATS_CFLAGS) -c ../src/heap.c -o obj/stats_heap.o

obj/stats_addr_map.o: ../src/addr_map.c ../include/addr_map.h \
	../include/heap_stats.h
	mkdir -p obj
	$(CC) $(STATS_CFLAGS) -c ../src/addr_map.c -o obj/stats_addr_map.o

obj/stats_large_allocations.o: ../src/large_allocations.c \
	../include/large_allocations.h ../include/heap_stats.h
	mkdir -p obj
	$(CC) $(STATS_CFLAGS) -c ../src/large_allocations.c \
		-o obj/stats_large_allocations.o

obj/stats_ring_cache.o: ../src/ring_cache.c ../include/ring_cache.h \
	../include/heap_stats.h
	mkdir -p obj
	$(CC) $(STATS_CFLAGS) -c ../src/ring_cache.c -o obj/stats_ring_cache.o

obj/stats_slab_quarantine.o: ../src/slab_quarantine.c \
	../include/slab_quarantine.h ../include/heap_stats.h
	mkdir -p obj
	$(CC) $(STATS_CFLAGS) -c ../src/slab_quarantine.c \
		-o obj/stats_slab_quarantine.o

obj/stats_slab_metadata_allocator.o: ../src/slab_metadata_allocator.c \
	../include/slab_metadata_allocator.h ../include/heap_stats.h
	mkdir -p obj
	$(CC) $(STATS_CFLAGS) -c ../src/slab_metadata_allocator.c \
		-o obj/stats_slab_metadata_allocator.o

obj/heap_stats.o: ../src/heap_stats.c ../include/heap_stats.h
	mkdir -p obj
	$(CC) $(STATS_CFLAGS) -c ../src/heap_stats.c -o obj/heap_stats.o

# Allocator objects

obj/heap.o: ../src/heap.c ../include/heap_internal.h ../include/utils.h \
	../include/addr_map.h ../include/ring_cache.h \
	../include/large_allocations.h ../include/slab_quarantine.h \
	../include/secure_utils.h ../include/slab_metadata_allocator.h
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

obj/slab_metadata_allocator.o: ../src/slab_metadata_allocator.c \
	../include/slab_metadata_allocator.h ../include/heap_internal.h \
	../include/secure_utils.h
	mkdir -p obj
	$(CC) $(CFLAGS) -c ../src/slab_metadata_allocator.c \
		-o obj/slab_metadata_allocator.o

obj/utils.o: ../src/utils.c ../include/utils.h
	mkdir -p obj
	$(CC) $(CFLAGS) -c ../src/utils.c -o obj/utils.o

# Position-independent allocator objects

obj/pic/bedrock.o: ../src/bedrock.c ../include/bedrock.h \
	../include/heap_internal.h
	mkdir -p obj/pic
	$(CC) $(PIC_CFLAGS) -c ../src/bedrock.c -o obj/pic/bedrock.o

obj/pic/heap.o: ../src/heap.c ../include/heap_internal.h ../include/utils.h \
	../include/addr_map.h ../include/ring_cache.h \
	../include/large_allocations.h ../include/slab_quarantine.h \
	../include/secure_utils.h ../include/slab_metadata_allocator.h
	mkdir -p obj/pic
	$(CC) $(PIC_CFLAGS) -c ../src/heap.c -o obj/pic/heap.o

obj/pic/addr_map.o: ../src/addr_map.c ../include/addr_map.h \
	../include/heap_internal.h ../include/utils.h ../include/secure_utils.h
	mkdir -p obj/pic
	$(CC) $(PIC_CFLAGS) -c ../src/addr_map.c -o obj/pic/addr_map.o

obj/pic/large_allocations.o: ../src/large_allocations.c \
	../include/large_allocations.h ../include/addr_map.h \
	../include/ring_cache.h ../include/secure_utils.h
	mkdir -p obj/pic
	$(CC) $(PIC_CFLAGS) -c ../src/large_allocations.c \
		-o obj/pic/large_allocations.o

obj/pic/ring_cache.o: ../src/ring_cache.c ../include/ring_cache.h \
	../include/addr_map.h ../include/secure_utils.h
	mkdir -p obj/pic
	$(CC) $(PIC_CFLAGS) -c ../src/ring_cache.c -o obj/pic/ring_cache.o

obj/pic/secure_utils.o: ../src/secure_utils.c ../include/secure_utils.h \
	../include/utils.h
	mkdir -p obj/pic
	$(CC) $(PIC_CFLAGS) -c ../src/secure_utils.c -o obj/pic/secure_utils.o

obj/pic/slab_quarantine.o: ../src/slab_quarantine.c \
	../include/slab_quarantine.h ../include/secure_utils.h
	mkdir -p obj/pic
	$(CC) $(PIC_CFLAGS) -c ../src/slab_quarantine.c \
		-o obj/pic/slab_quarantine.o

obj/pic/slab_metadata_allocator.o: ../src/slab_metadata_allocator.c \
	../include/slab_metadata_allocator.h ../include/heap_internal.h \
	../include/secure_utils.h
	mkdir -p obj/pic
	$(CC) $(PIC_CFLAGS) -c ../src/slab_metadata_allocator.c \
		-o obj/pic/slab_metadata_allocator.o

obj/pic/utils.o: ../src/utils.c ../include/utils.h
	mkdir -p obj/pic
	$(CC) $(PIC_CFLAGS) -c ../src/utils.c -o obj/pic/utils.o

# Cleanup

clean:
	rm -rf obj sanity_test correct_alignment_test read_write_test overlap_test \
		free_tests page_protection_test reuse_test mixed_workload throughput \
		malloc_throughput latency malloc_latency fragmentation \
		use_after_free_demo large_allocation_overflow_demo \
		libbedrock.a libbedrock.so
