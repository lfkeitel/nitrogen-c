CC=gcc
CFLAGS=-std=c99 -c -Wall
LDFLAGS=-ledit -lm -g
SOURCES=nitrogen.c builtins.c mpc.c ncore.c
OBJECTS=$(SOURCES:.c=.o)
EXECUTABLE=nitrogen

all: $(SOURCES) $(EXECUTABLE)
	make clean

$(EXECUTABLE): $(OBJECTS) 
	$(CC) $(OBJECTS) $(LDFLAGS) -o $@

.c.o:
	$(CC) $(CFLAGS) $< -o $@

cleanall:
	rm -rf *o nitrogen

clean:
	rm -rf *o