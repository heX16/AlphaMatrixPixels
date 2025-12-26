#pragma once

// AlphaMatrixPixels - Header-only library for LED matrix rendering
// Main library header that includes all core components

// Core types and utilities
#include "matrix_types.hpp"
#include "color_rgba.hpp"
#include "color_rgba_colors.hpp"
#include "rect.hpp"
#include "math.hpp"
#include "fixed_point.hpp"
#include "amp_constexpr.hpp"

// Matrix classes
#include "matrix_pixels.hpp"
#include "matrix_boolean.hpp"

// Rendering
#include "matrix_render.hpp"
#include "matrix_render_efffects.hpp"

// Fonts
#include "font_base.hpp"
#include "fonts.h"

// Output drivers
#include "output_driver.hpp"
#include "fastled_output.hpp"
#include "gamma8_lut.hpp"

