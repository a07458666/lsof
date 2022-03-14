CC = gcc
CXX = g++
CFLAGS = -Wall -g

PROGS = hw1

all: $(PROGS)

%: %.c
	$(CC) -o $@ $(CFLAGS) $< $(LDFLAGS)

%: %.cpp
	$(CXX) -o $@ $(CFLAGS) $< $(LDFLAGS)


hw1: lsof.o hw1.o
	$(CXX) -o $@ $^ $(LDFLAGS)

clean:
	rm -f *~ $(PROGS)
	rm -f *.o