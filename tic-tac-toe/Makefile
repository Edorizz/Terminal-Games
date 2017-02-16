CC=gcc
CFLAGS=-c -std=c89 -pedantic -Wall
LFLAGS=-lncurses

tictac: main.o
	$(CC) main.o $(LFLAGS) -o tictac

main.o: main.c
	$(CC) $(CFLAGS) main.c

clean:
	rm *.o tictac
