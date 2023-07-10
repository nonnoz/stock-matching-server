CC=g++
CFLAGS=-O3
EXTRAFLAGS=-lpqxx -lpq -pthread

all: test

test: main.cpp database.h database.cpp XMLparser.hpp XMLparser.cpp server.h server.cpp
	$(CC) $(CFLAGS) -o test main.cpp server.cpp database.cpp XMLparser.cpp $(EXTRAFLAGS)

clean:
	rm -f *~ *.o test

clobber:
	rm -f *~ *.o

# CFLAGS=-std=gnu++11 -pedantic -ggdb3
# EXTRAFLAGS=-lpqxx -lpq -pthread

# all: server

# server: main.cpp database.h database.cpp XMLparser.hpp XMLparser.cpp
# 	g++ $(CFLAGS) -o server main.cpp database.cpp XMLparser.cpp $(EXTRAFLAGS)

# clean:
# 	rm -f *~ *.o server

# clobber:
# 	rm -f *~ *.o