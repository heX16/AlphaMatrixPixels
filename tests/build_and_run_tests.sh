#!/usr/bin/env bash
# Build and run tests script using Meson
# Can be run from project root or from tests/ directory

set -e  # Exit on error

# Get script directory and project root
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

# Change to tests directory (where meson.build is located)
cd "$SCRIPT_DIR" || exit 1

echo "[run_tests] start"

# Old direct compilation method (kept for reference):
# rm -f pixel_matrix_tests.exe
# g++ -v -std=c++17 -O2 -Wall -Wextra -Isrc -static -static-libstdc++ -static-libgcc tests/pixel_matrix_tests.cpp -o pixel_matrix_tests.exe
# ./pixel_matrix_tests.exe

# Setup meson builddir if it doesn't exist
if [ ! -d "builddir" ]; then
    echo "[run_tests] Setting up Meson builddir..."
    meson setup builddir --buildtype=debug
fi

# Compile using Meson
echo "[run_tests] Compiling..."
meson compile -C builddir

# Run the executable
echo "[run_tests] Running tests..."
./builddir/pixel_matrix_tests.exe

echo "[run_tests] done"

