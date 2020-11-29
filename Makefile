CC = gcc
CXX = g++
CFLAGS = -Wall -Wextra -Wno-unused-result -Wno-unused-parameter -Wno-unused-but-set-variable -O3 -g
IFLAGS = -lcurl 
IFLAGS += $(shell pkg-config --libs libpistache)
IFLAGS += $(shell pkg-config --cflags libpistache)
CXXFLAGS = $(CFLAGS) -std=c++11 -mcmodel=medium $(IFLAGS)


all:	client worker

client:	client.o
	$(CXX) $^ $(CXXFLAGS) -o $@ 


worker: worker.o
	$(CXX) $^ $(CXXFLAGS) -o $@ 

client.o: client.cpp
	$(CXX) -c client.cpp


worker.o: worker.cpp
	$(CXX) $(IFLAGS) -c worker.cpp

clean:
	rm -f *\.o
	rm -f client
	rm -f worker
