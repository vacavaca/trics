BIN_NAME = trics
BUILD_DIR = build
SRC_DIR = src
TARGET = $(BUILD_DIR)/$(BIN_NAME)
LIBS = -lm -lncurses -lSDL2
CC = gcc
ifeq ($(BUILD), debug)
CFLAGS = -g -Wall -Wextra -Wpedantic \
          -Wformat=2 -Wno-unused-parameter -Wshadow \
          -Wwrite-strings -Wstrict-prototypes -Wold-style-definition \
          -Wredundant-decls -Wnested-externs -Wmissing-include-dirs
else
CFLAGS = -Wall -Wextra -Wpedantic \
          -Wformat=2 -Wno-unused-parameter -Wshadow \
          -Wwrite-strings -Wstrict-prototypes -Wold-style-definition \
          -Wredundant-decls -Wnested-externs -Wmissing-include-dirs
endif

default: $(TARGET)
all: default

OBJECTS = $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(wildcard $(SRC_DIR)/*.c))
HEADERS = $(wildcard $(SRC_DIR)/*.h)

test:
	echo $(HEADERS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR):
	mkdir $(BUILD_DIR)

$(TARGET): $(BUILD_DIR) $(OBJECTS)
	$(CC) $(OBJECTS) -Wall $(LIBS) -o $@

clean:
	rm -rf $(BUILD_DIR)

run: default
	./$(TARGET)

run-check: default
	valgrind \
		--gen-suppressions=all \
		--leak-check=full \
		--show-leak-kinds=all \
		--leak-check=full \
		--log-file="$(BUILD_DIR)/valgrind.log" \
		--tool=memcheck \
		--suppressions=ncurses.supp  \
		$(TARGET)

.PHONY: default all clean run run-check
