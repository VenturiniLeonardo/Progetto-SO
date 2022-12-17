########################################################################
####################### Makefile Template ##############################
########################################################################

# Compiler settings - Can be customized.
CC=gcc
CFLAGS=-std=c89 -Wpedantic -D_POSIX_C_SOURCE=199309L -D_GNU_SOURCE
LIBS = -lm

########################################################################
####################### Targets beginning here #########################
########################################################################

all: master.o port.o ship.o
	$(CC)  ship.o -o ship $(LIBS)
	$(CC)  port.o -o port $(LIBS)
	$(CC)  master.o -o master $(LIBS)


master.o:
	$(CC) $(CFLAGS) -c master.c 

port.o:
	$(CC) $(CFLAGS) -c port.c 

ship.o:
	$(CC) $(CFLAGS) -c ship.c 

clean:
	rm -f *.o
	rm port
	rm ship
	rm master
	ipcrm -a

run:
	./master
