heap.o: heap.c heap_internal.h 
	gcc -c heap.c -o heap.o 

main.o: main.c heap_internal.h
	gcc -c main.c -o main.o

utils.o: utils.c utils.h 
	gcc -c utils.c -o utils.o

util_test.o: utils_test.c utils.h
	gcc -c utils_test.c -o util_test.o 

main: heap.o main.o 
	gcc main.o heap.o -o main

util_test: utils.o util_test.o
	gcc util_test.o utils.o -o util_test
