#include <pthread.h>
#include "lock.h"

//extern variables
pthread_mutex_t read_lock, write_lock;
pthread_mutex_t alloc_claim_lock, dealloc_claim_lock; 

int readers;
pthread_cond_t read_cond;

bool writer_exists;
pthread_cond_t write_cond;

void init_locks(){
    readers = 0;
    writer_exists = false;

    pthread_mutex_init(&read_lock, NULL);
    pthread_cond_init(&read_cond, NULL);

    pthread_mutex_init(&write_lock, NULL);
    pthread_cond_init(&write_cond, NULL);

    pthread_mutex_init(&alloc_claim_lock, NULL);
    pthread_mutex_init(&dealloc_claim_lock, NULL);
}

void begin_reading(){
    pthread_mutex_lock(&read_lock);
    //std::cout << "begin_reading()" << pthread_self() << std::endl;
    while(writer_exists){
        //std::cout << "Wait: Writer exists " << pthread_self() << std::endl;
        pthread_cond_wait(&write_cond, &read_lock);
    }
    readers++;
    pthread_mutex_unlock(&read_lock);
}

void end_reading(){
    pthread_mutex_lock(&read_lock);
    //std::cout << "end_reading()"  << pthread_self() << std::endl;
    readers--;
    pthread_cond_signal(&read_cond);
    pthread_mutex_unlock(&read_lock);
}

void begin_writing(){
    pthread_mutex_lock(&write_lock);
    pthread_mutex_lock(&read_lock);
    //std::cout << "begin_writing()"  << pthread_self() << std::endl;
    // Place read lock to prevent more readers from entering while checking for the number of readers, as well as for the duration of the writing
    writer_exists = true;
    // This variable is set to true because the lock will be returned so that existing readers can still finish their reading, but new readers are forced to wait
    while(readers > 0) {
        //std::cout << readers << " Readers exists " << pthread_self() << std::endl;
        pthread_cond_wait(&read_cond, &read_lock);
    }
}

void end_writing(){
    //std::cout << "end_writing()" << pthread_self() << std::endl;
    writer_exists = false;
    pthread_mutex_unlock(&read_lock);
    pthread_mutex_unlock(&write_lock);
    //std::cout << "Signal: Writing complete!" << pthread_self() << std::endl;
    pthread_cond_broadcast(&write_cond);
}