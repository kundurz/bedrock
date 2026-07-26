#pragma once

/* header file for utils functions */
int determine_size_class(int size);
int get_fast_chunk_index(int size_class);
size_t round_to_nearest_page(int num); 
size_t round_down_power_of_two(size_t value); 
void generic_swap(void *a, void *b, size_t size);