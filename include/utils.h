#pragma once

/* header file for utils functions */
int determine_size_class(size_t size);
int get_slab_cache_index(size_t size_class);
size_t round_to_nearest_page(size_t num); 
size_t round_down_power_of_two(size_t value); 
void generic_swap(void *a, void *b, size_t size);