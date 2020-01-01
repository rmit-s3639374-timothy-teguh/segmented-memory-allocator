#include <stdbool.h>

enum allocation_method {FIRST_FIT, BEST_FIT, WORST_FIT};

void init(allocation_method type, bool verbose);

void * alloc(size_t chunk_size);
void dealloc(void * chunk);

void * alloc_first_fit(size_t chunk_size);
void * alloc_best_worst_fit(size_t chunk_size, bool best_fit);
void * move_frontier(size_t chunk_size);