CC = gcc
CXX = g++
CFLAGS = -Wall -Wextra -Wno-unused-result -Wno-unused-parameter -Wno-unused-but-set-variable -O3 -g
CXXFLAGS = $(CFLAGS) -std=c++11 -mcmodel=medium -lcurl

all:	client worker

client:	client.o
	$(CXX) $^ $(CXXFLAGS) -o $@ 


worker: worker.o
	$(CXX) $^ $(CXXFLAGS) -o $@ 

client.o: client.cpp
	$(CXX) -c client.cpp


worker.o: worker.cpp
	$(CXX) -c worker.cpp

clean:
	rm -f *o
	rm -f client
	rm -f worker
