#!/usr/bin/env bash
g++ -v -std=c++17 -O2 -Wall -Wextra -I. -static -static-libstdc++ -static-libgcc tests/pixel_matrix_tests.cpp -o pixel_matrix_tests.exe

