CC = gcc
CPPFLAGS=
CFLAGS = -Wall -std=c11 -g -Iinclude
LDFLAGS= -L.

all: list parser

list: bin/liblist.a
parser: bin/libparse.a

main: bin/libparse.a bin/liblist.a main.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o main main.o bin/libparse.a bin/liblist.a

main.o: main.c
	$(CC) $(CFLAGS) -c main.c

bin/liblist.a: LinkedListAPI.o
	ar cr bin/liblist.a LinkedListAPI.o

bin/libparse.a: GEDCOMutilities.o 
	ar cr bin/libparse.a GEDCOMutilities.o


LinkedListAPI.o: LinkedListAPI.c LinkedListAPI.h
	$(CC) $(CFLAGS) -c LinkedListAPI.c

GEDCOMutilities.o: GEDCOMutilities.c LinkedListAPI.h
	$(CC) $(CFLAGS) -c GEDCOMutilities.c

clean:
	rm -rf GEDCOMutilities *.o *.a bin/*
