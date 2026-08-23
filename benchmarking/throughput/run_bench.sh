#!/bin/bash
for i in $(seq 1 31); do
    taskset -c 2 ../../build/throughput
done

echo "Now running malloc benchmark..."

#!/bin/bash
for i in $(seq 1 31); do
    taskset -c 2 ../../build/malloc_throughput
done
