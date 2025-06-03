CC=gcc
CFLAGS=-std=c++0x -Wall -D __STDC_LIMIT_MACROS -D __STDC_FORMAT_MACROS -O3 -g -fpermissive
CXX=g++

INCLUDE=-Iminisat -Iminisat/minisat/core -Iminisat/minisat/mtl -Iminisat/minisat/simp -Iaiger -I/usr/include/eigen3

all:	ic3

ic3:	minisat/build/release/lib/libminisat.a aiger/aiger.o Model.o IC3.o MAB.o ARM.o main.o
	$(CXX) $(CFLAGS) $(INCLUDE) -static -o IC3 \
		aiger/aiger.o Model.o IC3.o MAB.o ARM.o main.o \
		minisat/build/release/lib/libminisat.a -lm -lpthread

aiger/aiger.o: aiger/aiger.c aiger/aiger.h
	$(CC) -c $(INCLUDE) -o aiger/aiger.o aiger/aiger.c

.c.o:
	$(CC) -g -O3 $(INCLUDE) $< -c

.cpp.o:	
	$(CXX) $(CFLAGS) $(INCLUDE) $< -c

clean:
	rm -f *.o ic3

dist:
	cd ..; tar cf ic3ref/IC3ref.tar ic3ref/*.h ic3ref/*.cpp ic3ref/Makefile ic3ref/LICENSE ic3ref/README; gzip ic3ref/IC3ref.tar
