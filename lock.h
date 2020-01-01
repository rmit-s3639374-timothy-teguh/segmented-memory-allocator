// Helper functions utilizing locks and condition variables
void begin_reading();
void end_reading();
void begin_writing();
void end_writing();

void init_locks();

extern pthread_mutex_t read_lock, write_lock;

// Locks for when a thread wants to claim a chunk. For instance, alloc_claim_lock is a lock on claiming chunks in the allocated list
extern pthread_mutex_t alloc_claim_lock, dealloc_claim_lock; 

extern int readers; // Variable that holds the number of readers reading from lists
extern pthread_cond_t read_cond; // Lock for the condition

extern bool writer_exists; // Variable that says that a writer wants to write to the lists
extern pthread_cond_t write_cond; // Lock for the condition