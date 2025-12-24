#pragma once

#include <stdint.h>
#include "matrix_types.hpp"

namespace amp {

using ::uint8_t;
using ::uint16_t;

// Mapping pattern types for converting 2D matrix coordinates to 1D output index.
// Used by all output drivers (FastLED, WS2812, SPI, etc.).
enum class csMappingPattern {
    RowMajor,                    // Standard row-by-row: index = y * width + x
    ColumnMajor,                 // Column-by-column: index = x * height + y
    Serpentine,                  // Zigzag pattern (horizontal): odd rows reversed
    SerpentineHorizontal,        // Zigzag pattern (horizontal): odd rows reversed
    SerpentineHorizontalInverted, // Zigzag pattern (horizontal) with inverted Y: odd rows reversed, starting from bottom
    SerpentineVertical,          // Zigzag pattern (vertical): odd columns reversed
    SerpentineVerticalInverted   // Zigzag pattern (vertical) with inverted X: odd columns reversed, starting from right
};

// Custom mapping function type: (x, y, width, height) -> output index
// Returns the 1D index for a given 2D coordinate in a matrix of given dimensions.
using tMappingFunc = uint16_t (*)(uint8_t x, uint8_t y, uint8_t width, uint8_t height);

// Map coordinates using row-major order (standard left-to-right, top-to-bottom).
// Formula: index = y * width + x
[[nodiscard]] inline uint16_t mapRowMajor(uint8_t x, uint8_t y, uint8_t width, uint8_t /*height*/) noexcept {
    return static_cast<uint16_t>(y) * static_cast<uint16_t>(width) + static_cast<uint16_t>(x);
}

// Map coordinates using column-major order (top-to-bottom, left-to-right).
// Formula: index = x * height + y
[[nodiscard]] inline uint16_t mapColumnMajor(uint8_t x, uint8_t y, uint8_t /*width*/, uint8_t height) noexcept {
    return static_cast<uint16_t>(x) * static_cast<uint16_t>(height) + static_cast<uint16_t>(y);
}

// Map coordinates using serpentine (zigzag) pattern (horizontal): odd rows are reversed.
// Formula: 
//   - Even rows (y % 2 == 0): index = y * width + x
//   - Odd rows (y % 2 == 1): index = y * width + (width - 1 - x)
[[nodiscard]] inline uint16_t mapSerpentine(uint8_t x, uint8_t y, uint8_t width, uint8_t /*height*/) noexcept {
    return (y & 1u) ? static_cast<uint16_t>(y) * static_cast<uint16_t>(width) + (static_cast<uint16_t>(width) - 1u - static_cast<uint16_t>(x))
                    : static_cast<uint16_t>(y) * static_cast<uint16_t>(width) + static_cast<uint16_t>(x);
}

// Map coordinates using serpentine (zigzag) pattern (horizontal) with inverted Y coordinate.
// Rows are processed from bottom to top, with odd rows (in inverted space) reversed.
// Formula:
//   - inverted_y = height - 1 - y
//   - Even inverted rows: index = inverted_y * width + x
//   - Odd inverted rows: index = inverted_y * width + (width - 1 - x)
[[nodiscard]] inline uint16_t mapSerpentineHorizontalInverted(uint8_t x, uint8_t y, uint8_t width, uint8_t height) noexcept {
    const uint8_t inverted_y = static_cast<uint8_t>(height - 1u - static_cast<uint16_t>(y));
    return (inverted_y & 1u) ? static_cast<uint16_t>(inverted_y) * static_cast<uint16_t>(width) + (static_cast<uint16_t>(width) - 1u - static_cast<uint16_t>(x))
                             : static_cast<uint16_t>(inverted_y) * static_cast<uint16_t>(width) + static_cast<uint16_t>(x);
}

// Map coordinates using serpentine (zigzag) pattern (vertical): odd columns are reversed.
// Formula:
//   - Even columns (x % 2 == 0): index = x * height + y
//   - Odd columns (x % 2 == 1): index = x * height + (height - 1 - y)
[[nodiscard]] inline uint16_t mapSerpentineVertical(uint8_t x, uint8_t y, uint8_t /*width*/, uint8_t height) noexcept {
    return (x & 1u) ? static_cast<uint16_t>(x) * static_cast<uint16_t>(height) + (static_cast<uint16_t>(height) - 1u - static_cast<uint16_t>(y))
                    : static_cast<uint16_t>(x) * static_cast<uint16_t>(height) + static_cast<uint16_t>(y);
}

// Map coordinates using serpentine (zigzag) pattern (vertical) with inverted X coordinate.
// Columns are processed from right to left, with odd columns (in inverted space) reversed.
// Formula:
//   - inverted_x = width - 1 - x
//   - Even inverted columns: index = inverted_x * height + y
//   - Odd inverted columns: index = inverted_x * height + (height - 1 - y)
[[nodiscard]] inline uint16_t mapSerpentineVerticalInverted(uint8_t x, uint8_t y, uint8_t width, uint8_t height) noexcept {
    const uint8_t inverted_x = static_cast<uint8_t>(width - 1u - static_cast<uint16_t>(x));
    return (inverted_x & 1u) ? static_cast<uint16_t>(inverted_x) * static_cast<uint16_t>(height) + (static_cast<uint16_t>(height) - 1u - static_cast<uint16_t>(y))
                             : static_cast<uint16_t>(inverted_x) * static_cast<uint16_t>(height) + static_cast<uint16_t>(y);
}

// Get mapping function for a given pattern.
// Returns nullptr if pattern is invalid (should not happen with enum class).
// If customMapping is provided (not nullptr), it is returned directly, ignoring pattern.
[[nodiscard]] inline tMappingFunc getMappingFunc(csMappingPattern pattern, tMappingFunc customMapping = nullptr) noexcept {
    if (customMapping != nullptr) {
        return customMapping;
    }
    
    switch (pattern) {
        case csMappingPattern::RowMajor:
            return mapRowMajor;
        case csMappingPattern::ColumnMajor:
            return mapColumnMajor;
        case csMappingPattern::Serpentine:
        case csMappingPattern::SerpentineHorizontal:
            return mapSerpentine;
        case csMappingPattern::SerpentineHorizontalInverted:
            return mapSerpentineHorizontalInverted;
        case csMappingPattern::SerpentineVertical:
            return mapSerpentineVertical;
        case csMappingPattern::SerpentineVerticalInverted:
            return mapSerpentineVerticalInverted;
        default:
            return mapSerpentine;
    }
}

} // namespace amp

