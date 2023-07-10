all: test

test: test.cpp client.h
	g++ -O3 -std=c++11 -pthread -o test test.cpp

.PHONY:
clean:
	rm -f test *.o *~
