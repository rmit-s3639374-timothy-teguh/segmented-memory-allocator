all: allocator

allocator: main.o allocator.o lock.o
	g++ -pthread main.o allocator.o lock.o -o allocator

main.o: main.cpp
	g++ -c -std=c++11 main.cpp

allocator.o: allocator.cpp
	g++ -c -std=c++11 allocator.cpp

lock.o: lock.cpp
	g++ -c -std=c++11 lock.cpp