########################################################################
####################### Makefile Template ##############################
########################################################################

# Compiler settings - Can be customized.
CC=gcc
CFLAGS=-std=c89 -Wpedantic -D_POSIX_C_SOURCE=199309L

########################################################################
####################### Targets beginning here #########################
########################################################################

all: master.o port.o ship.o
	$(CC)  ship.o -o ship 
	$(CC)  port.o -o port 
	$(CC)  master.o -o master 

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

run:
	./master
