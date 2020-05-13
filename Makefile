CC=gcc
CFLAGS=-Wall -ansi -pedantic
LIBS=-lpthread

headers=constantes.h liste.h transport.h taxi.h
objects=$(patsubst %.c,%.o,$(wildcard *.c))

main: $(objects) $(headers)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)
%.o: %.c $(headers)
	$(CC) -c $< $(CFLAGS)

.PHONY=clean tags
clean:
	rm *.o
tags:
	ctags *.{c,h}
