CC := gcc
CFLAGS := -std=c11 -Wall -Wextra -Iinclude -g
SRCDIR := src
INCDIR := include
OBJDIR := build
BINDIR := bin

SOURCES := $(wildcard $(SRCDIR)/*.c)
OBJECTS := $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(SOURCES))
TARGET := $(BINDIR)/controller

FIFOS := $(wildcard *.fifo)

.PHONY: all build clean run

all: build

build: $(TARGET)

$(OBJDIR)/%.o: $(SRCDIR)/%.c
	mkdir -p $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET): $(OBJECTS)
	mkdir -p $(BINDIR)
	$(CC) $(CFLAGS) $^ -o $@

run: build
	@echo "Running $(TARGET)"
	./$(TARGET)

clean:
	rm -rf $(OBJDIR) $(BINDIR)
	@if [ -n "$(FIFOS)" ]; then rm -f $(FIFOS); fi
	-find . -maxdepth 1 -type p -name '*.fifo' -exec rm -f {} + || true
	@echo "Cleaned build artifacts and any .fifo named pipes"
