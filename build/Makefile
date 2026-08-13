CC = gcc
CFLAGS = -g -Wall -Wextra -I.. -I../include

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
	rm -rf obj main
