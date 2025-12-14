# Makefile for EVFS (Encrypted Virtual File System with AES-256)

CC = gcc
CFLAGS = -Wall -Wextra -g `pkg-config fuse --cflags` -I/usr/include/openssl
LDFLAGS = `pkg-config fuse --libs` -lcrypto -lssl

TARGET = evfs
SOURCES = main.c evfs_core.c evfs_metadata.c evfs_storage.c evfs_readwrite.c evfs_crypto.c
OBJECTS = $(SOURCES:.c=.o)
HEADER = evfs.h evfs_crypto.h

.PHONY: all clean test mount unmount check-openssl

all: check-openssl $(TARGET)

check-openssl:
	@echo "Checking for OpenSSL..."
	@pkg-config --exists openssl || (echo "ERROR: OpenSSL not found. Install with: sudo apt-get install libssl-dev" && exit 1)
	@echo "OpenSSL found ✓"

$(TARGET): $(OBJECTS)
	@echo "Linking $(TARGET)..."
	$(CC) $(OBJECTS) -o $(TARGET) $(LDFLAGS)
	@echo "Build complete! AES-256 encryption enabled ✓"

%.o: %.c $(HEADER)
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	@echo "Cleaning build files..."
	rm -f $(TARGET) $(OBJECTS) evfs_data.bin
	@echo "Clean complete!"

mount: $(TARGET)
	@echo "Mounting EVFS with AES-256 encryption..."
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
	@echo "  make           - Build the project with AES-256 encryption"
	@echo "  make clean     - Remove build files and backing file"
	@echo "  make mount     - Build and mount filesystem"
	@echo "  make unmount   - Unmount filesystem"
	@echo "  make test      - Run basic tests"
	@echo ""
	@echo "Manual usage:"
	@echo "  ./evfs -f mnt  - Run in foreground mode"
	@echo "  ./evfs -d mnt  - Run in debug mode"
	@echo ""
	@echo "Requirements:"
	@echo "  - FUSE library (libfuse-dev)"
	@echo "  - OpenSSL library (libssl-dev)"