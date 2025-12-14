#!/bin/bash

# EVFS Project Setup Script
# Run this to set up the modular project structure

echo "=========================================="
echo "  EVFS Project Structure Setup"
echo "=========================================="
echo ""

# Check if we're in the right directory
if [ -f "evfs.c" ]; then
    echo "⚠️  Found old evfs.c file"
    echo "This script will create a modular structure."
    read -p "Continue? (y/n) " -n 1 -r
    echo ""
    if [[ ! $REPLY =~ ^[Yy]$ ]]; then
        echo "Aborted."
        exit 1
    fi
    
    # Backup old file
    echo "📦 Backing up evfs.c to evfs.c.backup"
    mv evfs.c evfs.c.backup
fi

echo ""
echo "✓ Current directory: $(pwd)"
echo ""

# Check for required tools
echo "Checking prerequisites..."
if ! command -v gcc &> /dev/null; then
    echo "❌ gcc not found. Install with: sudo apt-get install build-essential"
    exit 1
fi
echo "✓ gcc found"

if ! pkg-config --exists fuse; then
    echo "❌ FUSE not found. Install with: sudo apt-get install libfuse-dev"
    exit 1
fi
echo "✓ FUSE found"

echo ""
echo "=========================================="
echo "  Creating Project Files"
echo "=========================================="

# Create files (you'll need to paste the content)
echo ""
echo "Please create these files with the content provided:"
echo ""
echo "  1. evfs.h           - Header file"
echo "  2. main.c           - Main entry point"
echo "  3. evfs_core.c      - Core FUSE operations"
echo "  4. evfs_metadata.c  - Metadata management"
echo "  5. Makefile         - Build configuration"
echo ""

# Create mount point
echo "Creating mount point..."
mkdir -p mnt
echo "✓ Mount point 'mnt' created"

echo ""
echo "=========================================="
echo "  Setup Complete!"
echo "=========================================="
echo ""
echo "Next steps:"
echo "  1. Make sure all .c and .h files are in this directory"
echo "  2. Run: make clean && make"
echo "  3. Run: ./evfs -f mnt"
echo "  4. Test in another terminal: touch mnt/test.txt"
echo ""
echo "For team members:"
echo "  - Read README.md for module implementation guide"
echo "  - Each module goes in its own .c file"
echo "  - Update Makefile when adding new modules"
echo ""