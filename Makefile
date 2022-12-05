########################################################################
####################### Makefile Template ##############################
########################################################################

# Compiler settings - Can be customized.
CC=gcc
CFLAGS=-std=c89 -Wpedantic

########################################################################
####################### Targets beginning here #########################
########################################################################

all: master.o port.o ship.o
	$(CC) ship.o -o ship 
	$(CC) port.o -o port 
	$(CC) master.o -o master 

master.o:
	$(CC) -c master.c 

port.o:
	$(CC) -c port.c 

ship.o:
	$(CC) -c ship.c 

clean:
	rm -f *.o
	rm port
	rm ship
	rm master

run:
	./master
