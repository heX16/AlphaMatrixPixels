#pragma once

#include <stdint.h>
#include <FastLED.h>
#include "matrix_pixels.hpp"
#include "color_rgba.hpp"
#include "gamma8_lut.hpp"
#include "output_driver.hpp"

// Enable gamma correction by default if AMP_ENABLE_GAMMA is not defined.
#ifndef AMP_ENABLE_GAMMA
#define AMP_ENABLE_GAMMA 1
#endif

namespace amp {

// Copy matrix pixels to FastLED CRGB array with coordinate mapping and optional gamma correction.
// 
// Parameters:
//   matrix - source pixel matrix
//   leds - destination FastLED CRGB array
//   numLeds - size of leds array
//   pattern - mapping pattern (default: Serpentine)
//   customMapping - optional custom mapping function (nullptr to use pattern)
//
// Gamma correction is controlled by AMP_ENABLE_GAMMA macro.
// If macro is not defined, gamma correction is enabled by default.
// Define AMP_ENABLE_GAMMA to 0 to disable gamma correction.
// If customMapping is provided, pattern parameter is ignored.
inline void copyMatrixToFastLED(
    const csMatrixPixels& matrix,
    CRGB* leds,
    uint16_t numLeds,
    csMappingPattern pattern = csMappingPattern::Serpentine,
    tMappingFunc customMapping = nullptr
) noexcept {
    if (!leds || numLeds == 0) {
        return;
    }

    const tMatrixPixelsSize width = matrix.width();
    const tMatrixPixelsSize height = matrix.height();
    const uint16_t expectedLeds = static_cast<uint16_t>(width) * static_cast<uint16_t>(height);

    // Check if array size matches matrix size (warning: mismatch may cause out-of-bounds access).
    if (numLeds < expectedLeds) {
        return;
    }

    // Get mapping function from output_driver module.
    const tMappingFunc mapper = getMappingFunc(pattern, customMapping);

    // Copy pixels with mapping and optional gamma correction.
    for (tMatrixPixelsSize y = 0; y < height; ++y) {
        for (tMatrixPixelsSize x = 0; x < width; ++x) {
            const csColorRGBA c = matrix.getPixel(x, y);
            
            // Apply alpha channel as brightness multiplier (premultiplied alpha).
            // Multiply RGB channels by alpha/255 to respect transparency.
            const uint8_t r_scaled = mul8(c.r, c.a);
            const uint8_t g_scaled = mul8(c.g, c.a);
            const uint8_t b_scaled = mul8(c.b, c.a);
            
            // Apply gamma correction if enabled (controlled by AMP_ENABLE_GAMMA macro).
            #if AMP_ENABLE_GAMMA
            const CRGB out(
                amp_gamma_correct8(r_scaled),
                amp_gamma_correct8(g_scaled),
                amp_gamma_correct8(b_scaled));
            #else
            const CRGB out(r_scaled, g_scaled, b_scaled);
            #endif

            const uint16_t index = mapper(static_cast<uint8_t>(x), static_cast<uint8_t>(y),
                                          static_cast<uint8_t>(width), static_cast<uint8_t>(height));
            if (index < numLeds) {
                leds[index] = out;
            }
        }
    }
}

} // namespace amp

