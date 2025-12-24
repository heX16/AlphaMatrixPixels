#pragma once

#include <stdint.h>
#include <FastLED.h>
#include "matrix_pixels.hpp"
#include "color_rgba.hpp"
#include "gamma8_lut.hpp"
#include "output_driver.hpp"

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
// Gamma correction is controlled by AMP_ENABLE_GAMMA macro (default: enabled).
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
            
            // Apply gamma correction if enabled (controlled by AMP_ENABLE_GAMMA macro).
            // Default: gamma correction is enabled (AMP_ENABLE_GAMMA = 1).
#ifndef AMP_ENABLE_GAMMA
            // If macro is not defined, enable gamma by default.
            const CRGB out(
                amp_gamma_correct8(c.r),
                amp_gamma_correct8(c.g),
                amp_gamma_correct8(c.b));
#else
            // If macro is defined, check its value.
#if AMP_ENABLE_GAMMA
            const CRGB out(
                amp_gamma_correct8(c.r),
                amp_gamma_correct8(c.g),
                amp_gamma_correct8(c.b));
#else
            const CRGB out(c.r, c.g, c.b);
#endif
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

