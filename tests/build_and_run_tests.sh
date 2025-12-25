#!/usr/bin/env bash
echo "[run_tests] start"
rm -f pixel_matrix_tests.exe
g++ -v -std=c++17 -O2 -Wall -Wextra -Isrc -static -static-libstdc++ -static-libgcc tests/pixel_matrix_tests.cpp -o pixel_matrix_tests.exe
./pixel_matrix_tests.exe
echo "[run_tests] done"

