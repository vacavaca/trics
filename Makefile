TARGET = bin/trics
LIBS = -lm -lncurses
CC = gcc
CFLAGS = -g -Wall -Wextra -Wpedantic \
          -Wformat=2 -Wno-unused-parameter -Wshadow \
          -Wwrite-strings -Wstrict-prototypes -Wold-style-definition \
          -Wredundant-decls -Wnested-externs -Wmissing-include-dirs

.PHONY: default all clean run run-check

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

run: default
	./$(TARGET)

run-check: default
	valgrind \
		--gen-suppressions=all \
		--leak-check=full \
		--show-leak-kinds=all \
		--leak-check=full \
		--log-file="bin/valgrind.log" \
		--tool=memcheck \
		--suppressions=ncurses.supp  \
		$(TARGET)