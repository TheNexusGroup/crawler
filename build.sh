#!/bin/bash

# Build configuration
CC=gcc
CFLAGS="-Wall -Wextra -O2 -I./src -std=c99 -pthread"
LDFLAGS="-pthread -lm"
BUILD_DIR="build"
SRC_DIR="src"

# Create build directory if it doesn't exist
mkdir -p $BUILD_DIR

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
NC='\033[0m' # No Color

# Source files
SOURCES=(
    "src/crawler.c"
    "src/analyzers.c"
    "src/syntax_map.c"
    "src/syntaxes.c"
    "src/grammars.c"
    "src/pattern_cache.c"
    "src/structure.c"
    "src/dependency.c"
    "src/output_formatter.c"
    "src/language_analyzers.c"
    "main.c"
    "src/logger.c"
)

# Object files
OBJECTS=()

echo -e "${GREEN}Building Dependency Crawler...${NC}"

# Compile each source file
for source in "${SOURCES[@]}"; do
    if [ -f "$source" ]; then
        object="$BUILD_DIR/$(basename "${source%.*}").o"
        OBJECTS+=("$object")
        
        echo "Compiling $source..."
        $CC $CFLAGS -c "$source" -o "$object"
        
        if [ $? -ne 0 ]; then
            echo -e "${RED}Error compiling $source${NC}"
            exit 1
        fi
    else
        echo -e "${RED}Warning: Source file $source not found${NC}"
    fi
done

# Link the program
echo "Linking..."
$CC "${OBJECTS[@]}" $LDFLAGS -o "$BUILD_DIR/crawler"

if [ $? -eq 0 ]; then
    echo -e "${GREEN}Build successful! Binary located at $BUILD_DIR/crawler${NC}"
    # Create a symlink in the root directory for easier access
    # ln -sf "$BUILD_DIR/crawler" crawler
    # echo "Symlink 'crawler' created in root directory"
else
    echo -e "${RED}Linking failed${NC}"
    exit 1
fi