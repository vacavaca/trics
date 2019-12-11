TARGET = bin/cid
LIBS = -lm -lncurses
CC = gcc
CFLAGS = -g -Wall

.PHONY: default all clean

default: $(TARGET)
all: default

OBJECTS = $(patsubst %.c, %.o, $(wildcard *.c))
HEADERS = $(wildcard *.h)

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

.PRECIOUS: $(TARGET) $(OBJECTS)

bin:
	mkdir bin

$(TARGET): bin $(OBJECTS)
	$(CC) $(OBJECTS) -Wall $(LIBS) -o $@

clean:
	rm -f *.o
	rm -f $(TARGET)
	rm -rf bin
