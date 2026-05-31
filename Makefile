heap.o: heap.c heap_internal.h 
	gcc -c heap.c -o heap.o 

main.o: main.c heap_internal.h
	gcc -c main.c -o main.o

main: heap.o main.o
	gcc main.o heap.o -o main
