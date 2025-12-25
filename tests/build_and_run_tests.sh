#!/usr/bin/env bash
# Build and run tests script
# Can be run from project root or from tests/ directory

set -e  # Exit on error

# Get script directory and project root
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

# Change to project root
cd "$PROJECT_ROOT" || exit 1

echo "[run_tests] start"
rm -f pixel_matrix_tests.exe
g++ -v -std=c++17 -O2 -Wall -Wextra -Isrc -static -static-libstdc++ -static-libgcc tests/pixel_matrix_tests.cpp -o pixel_matrix_tests.exe
./pixel_matrix_tests.exe
echo "[run_tests] done"

