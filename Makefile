# Makefile for EVFS (Encrypted Virtual File System)

CC = gcc
CFLAGS = -Wall -Wextra -g `pkg-config fuse --cflags`
LDFLAGS = `pkg-config fuse --libs`

TARGET = evfs
SOURCES = main.c evfs_core.c evfs_metadata.c evfs_storage.c evfs_readwrite.c
OBJECTS = $(SOURCES:.c=.o)
HEADER = evfs.h

.PHONY: all clean test mount unmount

all: $(TARGET)

$(TARGET): $(OBJECTS)
	@echo "Linking $(TARGET)..."
	$(CC) $(OBJECTS) -o $(TARGET) $(LDFLAGS)
	@echo "Build complete!"

%.o: %.c $(HEADER)
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	@echo "Cleaning build files..."
	rm -f $(TARGET) $(OBJECTS) evfs_data.bin
	@echo "Clean complete!"

mount: $(TARGET)
	@echo "Mounting EVFS..."
	mkdir -p mnt
	./$(TARGET) -f mnt

unmount:
	@echo "Unmounting EVFS..."
	fusermount -u mnt || umount mnt

test: $(TARGET)
	@echo "Running tests..."
	bash test_basic.sh

help:
	@echo "EVFS Makefile Commands:"
	@echo "  make           - Build the project"
	@echo "  make clean     - Remove build files"
	@echo "  make mount     - Build and mount filesystem"
	@echo "  make unmount   - Unmount filesystem"
	@echo "  make test      - Run basic tests"
	@echo ""
	@echo "Manual usage:"
	@echo "  ./evfs -f mnt  - Run in foreground mode"
	@echo "  ./evfs -d mnt  - Run in debug mode"