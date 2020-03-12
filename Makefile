<<<<<<< HEAD
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
=======
CC = gcc
CFLAGS = -c -g -Wall -Wextra
LFLAGS = -Wall -Wextra -pthread


all: multi-lookup

multi-lookup: multi-lookup.o util.o
	$(CC) $(LFLAGS) $^ -o $@

multi-lookup.o: multi-lookup.c
	$(CC) $(CFLAGS) $<

util.o: util.c util.h
	$(CC) $(CFLAGS) $<

clean:
	rm -f multi-lookup
	rm -f *.o
	rm -f *~
>>>>>>> e0c095eaf3ff75abe7fbdffccfc2edff5f7852db
