// This is a header file for all the test functions used, they are technically not mandatory for the allocator to work
#include <stdbool.h>

void dealloc_all();
void dealloc_random();
void log(char message[]);
void log_results();
void init_for_type(char alloc_type[], bool verbose);
void test_data(char initial_file[], char input_file[], bool random); // Not implemented for multi threading
void test_data_surv(char initial_file[], char input_file[]); // Not implemented for multi threading
void test_data_multi(char file[], int number);
void allocate_file(char filename[], bool random); // Not implemented for multi threading
void begin_test();
void set_freeable(bool freeable);