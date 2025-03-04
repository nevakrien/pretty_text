#!/bin/bash

# Exit if any command fails
set -e

# Create build directory if it doesn't exist
if [ ! -d "build" ]; then
    mkdir build
fi

# Go to build directory
cd build

# Run CMake and build
cmake ..
make -j$(nproc)   # Use all available CPU cores

# Run the executable
./QtHelloWorld
