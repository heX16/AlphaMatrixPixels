#pragma once

#include <stdint.h>

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
};

} // namespace amp


