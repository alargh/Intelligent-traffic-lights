CC = gcc
CFLAGS = -Wall -Wextra $(shell pkg-config --cflags glib-2.0)
LIBS = $(shell pkg-config --libs glib-2.0)

all: program

program: program.c
	$(CC) $(CFLAGS) -o program program.c $(LIBS)

clean:
	rm -f program

.PHONY: all clean