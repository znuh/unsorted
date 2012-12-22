CC=gcc
CFLAGS=-Wall -g 
LDFLAGS=-lm -lSDL -lSDL_gfx -lSDL_ttf 

main: main.c
	$(CC) -o main main.c $(CFLAGS) $(LDFLAGS) 

all: main

clean:
	rm -f main *.o *~ 

