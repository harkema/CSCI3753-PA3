#Makefile
CC = gcc
#INCLUDE = /usr/Desktop/Pa/pa3/input/
LIBS = -pthread
#OBJS =

CFLAGS = -g -Wall -Wextra
LFLAGS = -Wall -Wextra -pthread

all: multi-lookup

multi-lookup: multi-lookup.o util.o
	$(CC) $(CFLAGS) $(LIBS) $^ -o $@
multi-lookup.o: multi-lookup.c multi-lookup.h
	$(CC) -c $(CFLAGS) $< $(LIBS)
util.o: util.c util.h
	$(CC) -c $(CFLAGS) $< $(LIBS)
#pgm4: pgm4.c
#	$(CC) -o pgm4 pgm4.c $(CFLAGS) $(LIBS)
#pgm5: pgm5.c
#	$(CC) -o pgm5 pgm5.c $(CFLAGS) $(LIBS)

clean:
	rm -f multi-lookup result.txt *.o *~ serviced.txt
