#pragma once

#include <stdint.h>
#include "matrix_types.hpp"
#include "matrix_boolean.hpp"

namespace amp {

// Runtime font interface.
//
// Contract:
// - rowBits() returns a bitmask for a given glyph row.
// - Bits must be aligned to MSB of uint32_t (bit31 is x=0, bit30 is x=1, ...).
// - Implementations should return 0 for out-of-range inputs.
class csFontBase {
public:
    virtual ~csFontBase() = default;

    virtual uint16_t width() const noexcept = 0;
    virtual uint16_t height() const noexcept = 0;
    virtual uint16_t count() const noexcept = 0;

    virtual uint32_t rowBits(uint16_t glyphIndex, uint16_t y) const noexcept = 0;

    // Get glyph as csMatrixBoolean (bridge method).
    // Returns nullptr if glyphIndex is out of range.
    // Uses virtual methods width(), height(), rowBits() to build the matrix.
    const csMatrixBoolean* getGlyphMatrix(uint16_t glyphIndex) const noexcept {
        if (glyphIndex >= count()) {
            return nullptr;
        }
        csMatrixBoolean * temp = 
            new csMatrixBoolean(to_size(width()), to_size(height()));
        for (uint16_t y = 0; y < height(); ++y) {
            const uint32_t row = rowBits(glyphIndex, y);
            for (uint16_t x = 0; x < width(); ++x) {
                const bool bit = (row & (0x80000000U >> x)) != 0;
                temp->setPixel(x, y, bit);
            }
        }
        return temp;
    }
};

} // namespace amp


