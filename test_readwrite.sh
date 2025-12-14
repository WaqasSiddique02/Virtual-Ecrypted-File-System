#!/bin/bash

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo "========================================"
echo "  EVFS Read/Write Operations Tests"
echo "========================================"

# Function to print test results
test_result() {
    if [ $1 -eq 0 ]; then
        echo -e "${GREEN}✓ PASS${NC}: $2"
    else
        echo -e "${RED}✗ FAIL${NC}: $2"
        exit 1
    fi
}

# Clean up function
cleanup() {
    echo ""
    echo "Cleaning up..."
    fusermount -u mnt 2>/dev/null || umount mnt 2>/dev/null
    sleep 1
    echo "Cleanup complete."
}

# Set trap to cleanup on exit
trap cleanup EXIT

# Build the project
echo ""
echo -e "${YELLOW}Building EVFS...${NC}"
make clean && make
test_result $? "Project built successfully"

# Step 1: Create mount point
echo ""
echo -e "${YELLOW}Step 1: Creating mount point${NC}"
mkdir -p mnt
test_result $? "Mount point 'mnt' created"

# Step 2: Mount the filesystem
echo ""
echo -e "${YELLOW}Step 2: Mounting EVFS${NC}"
./evfs -f mnt &
EVFS_PID=$!
sleep 2

# Check if mount was successful
if mountpoint -q mnt; then
    test_result 0 "EVFS mounted successfully"
else
    test_result 1 "EVFS mount failed"
fi

# Step 3: Create a test file
echo ""
echo -e "${YELLOW}Step 3: Creating test file${NC}"
touch mnt/test.txt
test_result $? "Created test file"

# Step 4: Write data to file
echo ""
echo -e "${YELLOW}Step 4: Writing data to file${NC}"
echo "Hello, EVFS!" > mnt/test.txt
test_result $? "Wrote data to file"

# Step 5: Read data back
echo ""
echo -e "${YELLOW}Step 5: Reading data from file${NC}"
CONTENT=$(cat mnt/test.txt)
if [ "$CONTENT" = "Hello, EVFS!" ]; then
    test_result 0 "Read correct data from file"
else
    echo "Expected: 'Hello, EVFS!'"
    echo "Got: '$CONTENT'"
    test_result 1 "Read incorrect data from file"
fi

# Step 6: Append data
echo ""
echo -e "${YELLOW}Step 6: Appending data${NC}"
echo "Second line" >> mnt/test.txt
CONTENT=$(cat mnt/test.txt)
EXPECTED="Hello, EVFS!
Second line"
if [ "$CONTENT" = "$EXPECTED" ]; then
    test_result 0 "Appended data correctly"
else
    echo "Expected: '$EXPECTED'"
    echo "Got: '$CONTENT'"
    test_result 1 "Append failed"
fi

# Step 7: Check file size
echo ""
echo -e "${YELLOW}Step 7: Checking file size${NC}"
SIZE=$(stat -c%s mnt/test.txt)
echo "File size: $SIZE bytes"
if [ $SIZE -gt 0 ]; then
    test_result 0 "File has correct size"
else
    test_result 1 "File size is zero"
fi

# Step 8: Write large data
echo ""
echo -e "${YELLOW}Step 8: Writing large data${NC}"
dd if=/dev/urandom of=mnt/large.dat bs=1K count=100 2>/dev/null
test_result $? "Wrote 100KB of data"

# Step 9: Verify large file
echo ""
echo -e "${YELLOW}Step 9: Verifying large file${NC}"
LARGE_SIZE=$(stat -c%s mnt/large.dat)
echo "Large file size: $LARGE_SIZE bytes"
if [ $LARGE_SIZE -eq 102400 ]; then
    test_result 0 "Large file has correct size"
else
    echo "Expected: 102400, Got: $LARGE_SIZE"
    test_result 1 "Large file size incorrect"
fi

# Step 10: Create multiple files
echo ""
echo -e "${YELLOW}Step 10: Creating multiple files${NC}"
for i in {1..5}; do
    echo "File $i content" > mnt/file$i.txt
done
test_result $? "Created 5 files"

# Step 11: Read all files
echo ""
echo -e "${YELLOW}Step 11: Reading all files${NC}"
SUCCESS=1
for i in {1..5}; do
    CONTENT=$(cat mnt/file$i.txt)
    EXPECTED="File $i content"
    if [ "$CONTENT" != "$EXPECTED" ]; then
        echo "File $i: Expected '$EXPECTED', Got '$CONTENT'"
        SUCCESS=0
    fi
done
test_result $SUCCESS "All files read correctly"

# Step 12: Create directory
echo ""
echo -e "${YELLOW}Step 12: Creating directory${NC}"
mkdir mnt/testdir
test_result $? "Created directory"

# Step 13: Create file in directory
echo ""
echo -e "${YELLOW}Step 13: Creating file in directory${NC}"
echo "Directory file" > mnt/testdir/dirfile.txt
test_result $? "Created file in directory"

# Step 14: List directory contents
echo ""
echo -e "${YELLOW}Step 14: Listing directory contents${NC}"
echo "Root directory:"
ls -lh mnt/
echo ""
echo "Test directory:"
ls -lh mnt/testdir/

# Step 15: Delete a file
echo ""
echo -e "${YELLOW}Step 15: Deleting a file${NC}"
rm mnt/file1.txt
if [ ! -f mnt/file1.txt ]; then
    test_result 0 "File deleted successfully"
else
    test_result 1 "File still exists after deletion"
fi

# Step 16: Rename a file
echo ""
echo -e "${YELLOW}Step 16: Renaming a file${NC}"
mv mnt/file2.txt mnt/renamed.txt
if [ -f mnt/renamed.txt ] && [ ! -f mnt/file2.txt ]; then
    test_result 0 "File renamed successfully"
else
    test_result 1 "File rename failed"
fi

# Step 17: Truncate a file
echo ""
echo -e "${YELLOW}Step 17: Truncating a file${NC}"
echo "Long content that will be truncated" > mnt/truncate.txt
truncate -s 10 mnt/truncate.txt
SIZE=$(stat -c%s mnt/truncate.txt)
if [ $SIZE -eq 10 ]; then
    test_result 0 "File truncated correctly"
else
    echo "Expected size: 10, Got: $SIZE"
    test_result 1 "File truncate failed"
fi

# Step 18: Performance test - many writes
echo ""
echo -e "${YELLOW}Step 18: Performance test (many writes)${NC}"
START=$(date +%s)
for i in {1..100}; do
    echo "Line $i" >> mnt/perf.txt
done
END=$(date +%s)
DURATION=$((END - START))
echo "Wrote 100 lines in $DURATION seconds"
test_result 0 "Performance test completed"

# Step 19: Verify performance test file
echo ""
echo -e "${YELLOW}Step 19: Verifying performance test file${NC}"
LINE_COUNT=$(wc -l < mnt/perf.txt)
if [ $LINE_COUNT -eq 100 ]; then
    test_result 0 "All 100 lines written correctly"
else
    echo "Expected 100 lines, got $LINE_COUNT"
    test_result 1 "Line count mismatch"
fi

# Step 20: Final summary
echo ""
echo -e "${GREEN}========================================"
echo "  All Read/Write Tests Passed!"
echo "========================================${NC}"
echo ""
echo "File system contents:"
ls -lhR mnt/
echo ""
echo "Total files created: $(find mnt -type f | wc -l)"
echo "Total directories: $(find mnt -type d | wc -l)"
echo ""
echo -e "${BLUE}Press Ctrl+C to unmount and exit...${NC}"
wait $EVFS_PID