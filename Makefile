CC := gcc
CFLAGS := -std=c11 -Wall -Wextra -Iinclude -g
SOURCES := $(wildcard src/*.c)
OBJECTS := $(patsubst src/%.c, build/%.o, $(SOURCES))
TARGET := bin/controller

.PHONY: all build clean run

all: build

build: $(TARGET)

build/%.o: src/%.c
	mkdir -p build
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET): $(OBJECTS)
	mkdir -p bin
	$(CC) $(CFLAGS) $^ -o $@

run: build
	./$(TARGET)

clean:
	rm -rf build bin
	rm -f /tmp/domotics_*
	@echo "Cleaned build artifacts and named pipes"
