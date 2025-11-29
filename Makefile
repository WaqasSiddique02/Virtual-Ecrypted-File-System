CC = gcc
CFLAGS = -Wall -D_FILE_OFFSET_BITS=64 $(shell pkg-config fuse --cflags)
LDFLAGS = $(shell pkg-config fuse --libs)

TARGET = evfs
OBJS = main.o evfs_core.o evfs_metadata.o

# Add future module objects here as they're developed:
# OBJS += evfs_encryption.o evfs_storage.o evfs_readwrite.o

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS) $(LDFLAGS)
	@echo ""
	@echo "Build successful! Executable: ./$(TARGET)"
	@echo ""

main.o: main.c evfs.h
	$(CC) $(CFLAGS) -c main.c

evfs_core.o: evfs_core.c evfs.h
	$(CC) $(CFLAGS) -c evfs_core.c

evfs_metadata.o: evfs_metadata.c evfs.h
	$(CC) $(CFLAGS) -c evfs_metadata.c

# Templates for future modules (uncomment as needed):
# evfs_encryption.o: evfs_encryption.c evfs.h
# 	$(CC) $(CFLAGS) -c evfs_encryption.c

# evfs_storage.o: evfs_storage.c evfs.h
# 	$(CC) $(CFLAGS) -c evfs_storage.c

# evfs_readwrite.o: evfs_readwrite.c evfs.h
# 	$(CC) $(CFLAGS) -c evfs_readwrite.c

clean:
	rm -f $(TARGET) $(OBJS)
	@echo "Cleaned build files"

test: $(TARGET)
	@echo "Creating mount point..."
	mkdir -p mnt
	@echo ""
	@echo "Mount point 'mnt' ready."
	@echo "Run: ./$(TARGET) -f mnt"
	@echo ""

unmount:
	fusermount -u mnt 2>/dev/null || umount mnt 2>/dev/null || true
	@echo "Unmounted (if it was mounted)"

run: $(TARGET)
	@mkdir -p mnt
	./$(TARGET) -f mnt

.PHONY: all clean test unmount run