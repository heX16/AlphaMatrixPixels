#pragma once

#include <cmath>
#include <cstdint>

#include "color_rgba.hpp"
#include "matrix_pixels.hpp"

// Simple time-driven gradient effect.
inline void updateGradient(csMatrixPixels& matrix, std::uint32_t ticks_ms) {
    const float t = static_cast<float>(ticks_ms) * 0.001f;
    auto wave = [](float v) -> uint8_t {
        return static_cast<uint8_t>((std::sin(v) * 0.5f + 0.5f) * 255.0f);
    };

    const int width = static_cast<int>(matrix.width());
    const int height = static_cast<int>(matrix.height());

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            const float xf = static_cast<float>(x) * 0.4f;
            const float yf = static_cast<float>(y) * 0.4f;
            const uint8_t r = wave(t * 0.8f + xf);
            const uint8_t g = wave(t * 1.0f + yf);
            const uint8_t b = wave(t * 0.6f + xf + yf * 0.5f);
            matrix.setPixel(x, y, csColorRGBA{255, r, g, b});
        }
    }
}

