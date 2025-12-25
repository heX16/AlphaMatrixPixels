#!/usr/bin/env bash
# Build script for GUI_Test using Meson (always builds debug)

set -e  # Exit on error

BUILDDIR="../builddir"

# Change to project root directory (one level up from GUI_Test)
cd "$(dirname "$0")/.." || exit 1

# Setup build directory
meson setup --reconfigure "$BUILDDIR" --buildtype=debug

# Compile
meson compile -C "$BUILDDIR"

