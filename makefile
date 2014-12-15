CC=gcc
CFLAGS=-std=c99 -c -Wall
LDFLAGS=-ledit -lm
SOURCES=nitrogen.c builtins.c mpc.c ncore.c
OBJECTS=$(SOURCES:.c=.o)
EXECUTABLE=nitrogen

all: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS) 
	$(CC) $(OBJECTS) $(LDFLAGS) -o $@

.c.o:
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -rf *o nitrogen