# segmented-memory-allocator
A program that simulates a segmented memory allocator found in many operating systems

This was done for an assignment.

How To Use:
.//allocator alloc_type file.txt number_of_threads verbose

alloc_type:
    - first_fit
    - best_fit
    - worst_fit

file.txt: Dataset to be used

number_of_threads: Number of threads to be used when allocating and deallocating the dataset
    - Maximum of 100 threads

verbose: Prints out text that tells you the details of the allocation.
    - true
    - false
    - Note: Setting this to false will make it look like the program is not doing anything, since the tests take a while, and nothing is printed out until the very end.
