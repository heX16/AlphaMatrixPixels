#!/usr/bin/env bash
# Build script for GUI_Test using Meson (always builds debug)
# This script should be run from dev/GUI_Test/ directory

set -e  # Exit on error

# Change to script directory (dev/GUI_Test/)
cd "$(dirname "$0")" || exit 1

BUILDDIR="builddir"

# Setup build directory (local to GUI_Test)
# meson setup --reconfigure "$BUILDDIR" --buildtype=debug

# Compile
meson compile -C "$BUILDDIR"

./builddir/HXLED_GUI_Test.exe
