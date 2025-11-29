#!/bin/bash

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo "========================================"
echo "  EVFS Basic Functionality Tests"
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

# Step 3: List root directory
echo ""
echo -e "${YELLOW}Step 3: Listing root directory${NC}"
ls -la mnt
test_result $? "Listed root directory"

# Step 4: Get attributes of root
echo ""
echo -e "${YELLOW}Step 4: Getting root attributes${NC}"
stat mnt
test_result $? "Got root directory attributes"

# Step 5: Create a test file
echo ""
echo -e "${YELLOW}Step 5: Creating test file${NC}"
touch mnt/test.txt
test_result $? "Created test file"

# Step 6: Verify file exists
echo ""
echo -e "${YELLOW}Step 6: Verifying file exists${NC}"
ls -la mnt/
if [ -f mnt/test.txt ]; then
    test_result 0 "File exists in directory listing"
else
    test_result 1 "File not found in directory"
fi

# Step 7: Get file attributes
echo ""
echo -e "${YELLOW}Step 7: Getting file attributes${NC}"
stat mnt/test.txt
test_result $? "Got file attributes"

# Step 8: Create multiple files
echo ""
echo -e "${YELLOW}Step 8: Creating multiple files${NC}"
touch mnt/file1.txt
touch mnt/file2.txt
touch mnt/file3.txt
test_result $? "Created multiple files"

# Step 9: List all files
echo ""
echo -e "${YELLOW}Step 9: Listing all files${NC}"
echo "Files in mnt:"
ls -la mnt/
FILE_COUNT=$(ls mnt/ | wc -l)
if [ $FILE_COUNT -eq 4 ]; then
    test_result 0 "All 4 files are visible"
else
    test_result 1 "Expected 4 files, found $FILE_COUNT"
fi

echo ""
echo -e "${GREEN}========================================"
echo "  All Basic Tests Passed!"
echo "========================================${NC}"

echo ""
echo "Press Ctrl+C to unmount and exit..."
wait $EVFS_PID