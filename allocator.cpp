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

#include "allocator.h"
#include "test.h"
#include "lock.h"

struct node_t {
    void * ptr;
    size_t size;
    bool claimed;

    node_t(): claimed(false) // By default, all chunks are unclaimed
    {
    }
};

void * origin;
void * frontier;
bool track_expansion; // When set to true, it will begin tracking how often the frontier was expanded
size_t expansion;
allocation_method alloc_type;

bool text; // If this is true, a lot of print statements are thrown out

std::list<node_t> alloc_list;
std::list<node_t> dealloc_list;

void begin_test(){
    // Function to track how many bytes was the heap expanded through sbrk / brk
    track_expansion = true;
    expansion = 0;
}

void init(allocation_method type, bool verbose){
	origin = sbrk(0);
    frontier = sbrk(0);
    alloc_type = type;
    track_expansion = false;
    text = verbose;
    srand(time(0));

    init_locks();
}

void * move_frontier(size_t chunk_size){
    void * return_ptr;
    frontier = sbrk(chunk_size);
    if(text){
        std::cout << "#Not enough space. Moved the heap by " << chunk_size << " bytes of memory.#" << std::endl;
    }
    node_t node; // Create a node to be added to the allocated list
    node.ptr = frontier;
    node.size = chunk_size;
    alloc_list.push_back(node);
    if (track_expansion){
        expansion += chunk_size;
    }
    return_ptr = frontier;
    end_writing();
    return return_ptr;
}

void * alloc_first_fit(size_t chunk_size){

    std::list<node_t>::iterator it = dealloc_list.begin();
    for (unsigned i = 0; i < dealloc_list.size(); i++){
        // Unfortunately, this part is repeated code, because I couldn't figure out a way to pass the iterator properly through functions
        if (it->size >= chunk_size){
            pthread_mutex_lock(&dealloc_claim_lock);
            if(it->claimed == false){
                it->claimed = true;
                pthread_mutex_unlock(&dealloc_claim_lock);
                // This thread has found what it is looking for, it is now no longer a reader
                end_reading();
                begin_writing();
                if(text){
                    std::cout << "Allocating " << chunk_size << " bytes with first fit..." << std::endl;
                }
                void * return_ptr = it->ptr;
                // Must unclaim the chunk so that future readers can claim it
                it->claimed = false; 
                if(it->size == chunk_size){
                    // Does not need a split
                    alloc_list.splice(alloc_list.end(), dealloc_list, it);
                }
                else{
                    // Needs a split
                    node_t allocated_node, deallocated_node;
                    // Create new nodes to represent the split
                    allocated_node.ptr = it->ptr;
                    allocated_node.size = chunk_size;
                    alloc_list.push_back(allocated_node);

                    deallocated_node.ptr = (void*) ((char*) it->ptr + chunk_size);
                    deallocated_node.size = it->size - chunk_size;
                    *it = deallocated_node;
                    return_ptr = allocated_node.ptr;
                }
                end_writing();
                return return_ptr;
            }
            else{
                // Chunk was claimed by another thread, continue search
                pthread_mutex_unlock(&dealloc_claim_lock);
                it++;
                continue;
            }

        }
        else{
            it++;
        }
    }
    end_reading();
    begin_writing();
    return move_frontier(chunk_size);
}

void * alloc_best_worst_fit(size_t chunk_size, bool best_fit){

    std::list<node_t>::iterator ideal_node;
    while(true){
        // This is an infinite loop until break is called so that readers are able to look through the list again in case another thread has taken their ideal node
        ideal_node = dealloc_list.end(); // "Uninitialized" pointer, used like a null ptr
        // Unfortunately, this part is repeated code, because I couldn't figure out a way to pass the iterator properly through functions
        for (std::list<node_t>::iterator current_node = dealloc_list.begin(); current_node != dealloc_list.end(); ++current_node){ // Search for best node
            if (current_node->size < chunk_size){
                // Size of current chunk cannot be smaller than is demanded
                continue;
            }
            if(ideal_node == dealloc_list.end() && current_node->claimed == false){
                 // First usable chunk found, continue the search
                ideal_node = current_node;
                continue;
            }
            if(best_fit){
                if(current_node->size < ideal_node->size && current_node->claimed == false){
                    ideal_node = current_node;
                }
            }
            else{
                if(current_node->size > ideal_node->size && current_node->claimed == false){
                    ideal_node = current_node;
                }
            }
        }
        if(ideal_node == dealloc_list.end()){
            // All chunks are too small to be used, since ideal_node is still uninitialized, must expand the heap
            end_reading();
            begin_writing();
            return move_frontier(chunk_size);
        }
        pthread_mutex_lock(&dealloc_claim_lock);
        if(ideal_node->claimed == false){
            // Node wanted is not claimed, so we claim it and end the loop
            ideal_node->claimed = true;
            pthread_mutex_unlock(&dealloc_claim_lock);
            break;
        }
        else{
            pthread_mutex_unlock(&dealloc_claim_lock);
        }
    }
    end_reading();
    begin_writing();
    ideal_node->claimed = false;
    // Unclaim chunk so that future threads can use it
    // Note that begin_writing locks both read and write locks because writers need exclusive access to the list
    if(text){
        if(best_fit){
            std::cout << "Allocating " << chunk_size << " bytes with best fit..." << std::endl;
        }
        else{
            std::cout << "Allocating " << chunk_size << " bytes with worst fit..." << std::endl;
        }
    }
    if (ideal_node->size == chunk_size){
        alloc_list.splice(alloc_list.end(), dealloc_list, ideal_node);
        end_writing();
        return ideal_node->ptr;
    }
    else {
        node_t allocated_node, deallocated_node; // Create new nodes to represent the split
        allocated_node.ptr = ideal_node->ptr;
        allocated_node.size = chunk_size;
        alloc_list.push_back(allocated_node);

        deallocated_node.ptr = (void*) ((char*) ideal_node->ptr + chunk_size);
        deallocated_node.size = ideal_node->size - chunk_size;
        *ideal_node = deallocated_node;
        end_writing();
        return allocated_node.ptr;
    }
}

void * alloc(size_t chunk_size){
    begin_reading();
    void * ptr;
    switch(alloc_type){
        case FIRST_FIT:
            ptr = alloc_first_fit(chunk_size);
            break;
        case BEST_FIT:
            ptr = alloc_best_worst_fit(chunk_size, true);
            break;
        case WORST_FIT:
            ptr = alloc_best_worst_fit(chunk_size, false);
            break;
        default:
            std::cout << "Error! Allocation type does not exist!" << std::endl;
            abort();
    }
    return ptr;
}

void dealloc(void * chunk){
    begin_reading();
    std::list<node_t>::iterator it = alloc_list.begin();
    for (unsigned i = 0; i < alloc_list.size(); i++){
        node_t node = *it;
        
        if (node.ptr == chunk){
            pthread_mutex_lock(&alloc_claim_lock);
            if(it->claimed == false){
                it->claimed = true;
                pthread_mutex_unlock(&alloc_claim_lock);
                // This thread has found what it is looking for, it is now no longer a reader
                end_reading();
                begin_writing();
                it->claimed = false; 
                if(text){
                    std::cout << "Deallocating " << node.size << " bytes to remove " << (char*) node.ptr << " starting from " << node.ptr << std::endl;
                }
                dealloc_list.splice(dealloc_list.end(), alloc_list, it);
                end_writing();
                return;
            }
            else {
                pthread_mutex_unlock(&alloc_claim_lock);
                end_reading();
                std::cout << "Error! Unable to deallocate because " << chunk << " is already being deallocated by another thread!" << std::endl;
                abort();
            }
        }
        else{
            it++;
        }
    }
    std::cout << "Error! Unable to deallocate because " << chunk << " is not allocated!" << std::endl;
    abort();
}

void log(char message[]){
    std::cout << message << std::endl;
}

void log_results(){
    frontier = sbrk(0);
    std::cout << std::endl;
    std::cout << "#Test finished!#" << std::endl;
    std::cout << "See below for details" << std::endl;

    if(text){
        std::cout << std::endl;
        std::cout << "#Allocated chunks#" << std::endl;
        for (std::list<node_t>::iterator it = alloc_list.begin(); it != alloc_list.end(); ++it){
            std::cout << "------------------------------" << std::endl;
            std::cout << "Pointer: " << it->ptr << std::endl;
            std::cout << "Content: " << (char*) it->ptr << std::endl;
            std::cout << "Size: " << it->size << std::endl;
            std::cout << "Claimed: " << it->claimed << std::endl;
        }
            std::cout << std::endl;
            std::cout << "#Deallocated chunks#" << std::endl;
        for (std::list<node_t>::iterator it = dealloc_list.begin(); it != dealloc_list.end(); ++it){
            std::cout << "------------------------------" << std::endl;
            std::cout << "Pointer: " << it->ptr << std::endl;
            std::cout << "Size: " << it->size << std::endl;
            std::cout << "Claimed: " << it->claimed << std::endl;
        }
    }
    std::cout << std::endl;
    std::cout << "#Statistics#" << std::endl;
    std::cout << "Origin: " << origin << std::endl;
    std::cout << "Frontier: " << frontier << std::endl;
    std::cout << "Expansion: " << expansion << " bytes" << std::endl;
}