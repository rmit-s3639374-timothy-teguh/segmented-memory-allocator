#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <unistd.h>
#include <stdbool.h> 
#include <pthread.h>

#include <iostream>
#include <fstream>
#include <list>
#include <string>
#include <chrono> 

#include "allocator.h"
#include "test.h"

#define MAX_THREADS 100

std::ifstream file;

pthread_mutex_t thread_lock;

bool verbose;

//scl enable devtoolset-8 bash
void init_for_type(char type[], bool v){
	if (strcmp(type, "first_fit") == 0){
		init(FIRST_FIT, v);
	}
	else if (strcmp(type, "best_fit") == 0){
		init(BEST_FIT, v);
	}
	else if (strcmp(type, "worst_fit") == 0){
		init(WORST_FIT, v);
	}
	else{
		std::cout << "Error! Allocation type not implemented!" << std::endl;
		exit(EXIT_FAILURE);
	}
}

void * runner (void * param){
	// Test: Allocates a string, use it, and then deallocates it
	std::string line = *static_cast<std::string*>(param);
	char* data = (char*) alloc(line.size()); // Gives a pointer to be used to point to the first character in a string
	for (unsigned i = 0; line.size() - 1 > i; i++) // I reduced it by 1 because I don't want the \n character
	{
		data[i] = line.at(i);
		data[i + 1] = '\0'; // Put a null character to end the string
	}
	// Runs a check to see if the data allocated was properly done, by using it
	char * line_c = (char*) line.c_str();
	// Need to put null character since that is what I've done earlier
	line_c[strlen(line_c) - 1] = '\0';
	if(strcmp(line_c, data) != 0){
		// Abort the program, something went wrong
 		std::cout << "Error! Something wrong with the allocation for " << data << std::endl;
    	abort();
	}
	dealloc((void*) data);
	pthread_exit(EXIT_SUCCESS);
}

void test_data_multi(char filename[], int number_of_threads){
	// A test where after allocation, immediately deallocate it, based on a test from assignment 1
	pthread_t tid[MAX_THREADS];
	std::string content[MAX_THREADS];

	if (number_of_threads < 1){
		std::cout << "Error! Incorrect number of threads!" << std::endl;
    	exit(EXIT_FAILURE);
	}
	if (number_of_threads > MAX_THREADS){
		std::cout << "Error! Number of threads cannot exceed 100!" << std::endl;
    	exit(EXIT_FAILURE);
	}
	log((char*) "#Starting a new test#");
    log((char*) "");

	std::ifstream file;
	file.open(filename);
	if(!file.is_open()) {
    	std::cout << "Error! " << filename << " cannot be opened!" << std::endl;
    	exit(EXIT_FAILURE);
   	}
	begin_test();
	auto start = std::chrono::high_resolution_clock::now(); 
	while(!file.eof()){
		// Variable to prevent the test from creating more threads than necessary (For example, allocating 3 lines will not create 5 threads)
		int leftover = 0;
		for(int i = 0; i < number_of_threads; i++){
			if (std::getline(file, content[i])) {
        		int result = pthread_create(&tid[i], NULL, runner, &content[i]);
				if (result != 0) {
            		std::cout << "Error Creating Thread! Error Code: " << strerror(result) << std::endl;
       			}
			}
			else{
				//std::cout << "Found some leftovers!" << std::endl;
				leftover = number_of_threads - i;
				break;
			}
		}
		for (int i = 0; i < number_of_threads - leftover; i++){
        	int result = pthread_join(tid[i], NULL);
			if (result != 0) {
            	std::cout << "Error Joining Thread! Error Code: " << strerror(result) << std::endl;
            	abort();
        	}
    	}
	}
	log_results();
	auto stop = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start);
	std::cout << "Total Time Taken: " << duration.count() << " ms" << std::endl;
	std::cout << std::endl;
}

int main(int argc, char *argv[]){
	if (argc != 5){
		std::cout << "Error! Incorrect number of arguments! Please check readme.txt" << std::endl;
		return EXIT_FAILURE;
	}
	 // Run the experiment
	if(strcmp(argv[4], "true") == 0){
		verbose = true;
	}
	else if(strcmp(argv[4], "false") == 0){
		verbose = false;
	}
	else{
		std::cout << "Error! Incorrect verbose argument!" << std::endl;
	}
	init_for_type(argv[1], verbose);
	test_data_multi(argv[2], atoi(argv[3]));
	std::cout << "Test completed!" << std::endl;
	return EXIT_SUCCESS;
}
