#pragma once

#include <stdint.h>
#include "matrix_pixels.hpp"
#include "color_rgba.hpp"

// Arduino Serial support
#if defined(ARDUINO) && ARDUINO >= 100
#include <Arduino.h>
#elif defined(ARDUINO)
#include <WProgram.h>
#endif

namespace amp {

using ::uint8_t;
using ::uint16_t;

// Print matrix to Serial in JSON format.
// Format: array of rows, each row is array of colors as 0xFFFFFFFF (0xAARRGGBB).
// Example output:
//   [[0xFF000000,0xFF0000FF],[0xFF00FF00,0xFFFF0000]]
//
// Parameters:
//   matrix - source pixel matrix
//   prettyPrint - if true, adds newlines and indentation (default: false)
inline void printMatrixToSerialJSON(
    const csMatrixPixels& matrix,
    bool prettyPrint = false
) noexcept {
#if defined(ARDUINO)
    const tMatrixPixelsSize width = matrix.width();
    const tMatrixPixelsSize height = matrix.height();

    Serial.print('[');
    if (prettyPrint) {
        Serial.println();
    }

    for (tMatrixPixelsSize y = 0; y < height; ++y) {
        if (prettyPrint) {
            Serial.print(F("  ["));
        } else {
            Serial.print('[');
        }

        for (tMatrixPixelsSize x = 0; x < width; ++x) {
            const csColorRGBA c = matrix.getPixel(x, y);
            Serial.print(F("0x"));
            
            // Print as 0xAARRGGBB (8 hex digits)
            uint32_t val = c.value;
            for (int i = 7; i >= 0; --i) {
                uint8_t nibble = static_cast<uint8_t>((val >> (i * 4)) & 0x0F);
                if (nibble < 10) {
                    Serial.print(static_cast<char>('0' + nibble));
                } else {
                    Serial.print(static_cast<char>('A' + nibble - 10));
                }
            }

            if (x < width - 1) {
                Serial.print(',');
                if (prettyPrint) {
                    Serial.print(' ');
                }
            }
        }

        Serial.print(']');
        if (y < height - 1) {
            Serial.print(',');
        }
        if (prettyPrint) {
            Serial.println();
        }
    }

    Serial.print(']');
    if (prettyPrint) {
        Serial.println();
    }
#endif
}

// Print matrix to Serial as raw bytes (ARGB format, 4 bytes per pixel).
// Format: all pixels written sequentially as [A][R][G][B] bytes, row by row.
// No headers, no delimiters - pure binary data.
//
// Parameters:
//   matrix - source pixel matrix
inline void printMatrixToSerialRaw(
    const csMatrixPixels& matrix
) noexcept {
#if defined(ARDUINO)
    const tMatrixPixelsSize width = matrix.width();
    const tMatrixPixelsSize height = matrix.height();

    for (tMatrixPixelsSize y = 0; y < height; ++y) {
        for (tMatrixPixelsSize x = 0; x < width; ++x) {
            const csColorRGBA c = matrix.getPixel(x, y);
            Serial.write(c.a);
            Serial.write(c.r);
            Serial.write(c.g);
            Serial.write(c.b);
        }
    }
#endif
}

// Print matrix to Serial as compact debug format.
// Format: black pixels (all channels zero) are printed as space ' ',
//         any non-black pixel is printed as '#'.
// Output is aligned as a matrix with newlines after each row.
// Example output for 3x2 matrix:
//   # #
//   # #
//    #
//
// Parameters:
//   matrix - source pixel matrix
inline void printMatrixToSerialDebug(
    const csMatrixPixels& matrix
) noexcept {
#if defined(ARDUINO)
    const tMatrixPixelsSize width = matrix.width();
    const tMatrixPixelsSize height = matrix.height();

    for (tMatrixPixelsSize y = 0; y < height; ++y) {
        for (tMatrixPixelsSize x = 0; x < width; ++x) {
            const csColorRGBA c = matrix.getPixel(x, y);
            
            // Check if pixel is black (all channels zero or very low alpha)
            if (c.value == 0 || (c.r == 0 && c.g == 0 && c.b == 0)) {
                Serial.print(' ');
            } else {
                Serial.print('#');
            }
        }
        Serial.println();
    }
#endif
}

} // namespace amp

