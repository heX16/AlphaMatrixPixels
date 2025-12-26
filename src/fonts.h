#pragma once

#include <stdint.h>

#include "font_base.hpp"

namespace amp {

// Template function for font accessors: avoids global objects and ODR issues in header-only mode.
template<typename FontType>
inline const FontType& getFont() noexcept {
    static FontType f;
    return f;
}

// Monospace 3x5 digit font (glyphs 0..9).
//
// Storage format:
// - Each row is stored in the top bits of a byte: bit7 is x=0, bit6 is x=1, etc.
// - getRowBits() returns the same row aligned to MSB of uint32_t: bit31 is x=0.
class csFont3x5Digits final : public csFontBase {
public:
    using Row = uint8_t;

    static constexpr uint16_t kWidth = 3;
    static constexpr uint16_t kHeight = 5;
    static constexpr uint16_t kCount = 10;

    uint16_t width() const noexcept override { return kWidth; }
    uint16_t height() const noexcept override { return kHeight; }
    uint16_t count() const noexcept override { return kCount; }

    uint32_t getRowBits(uint16_t glyphIndex, uint16_t y) const noexcept override {
        if (glyphIndex >= count() || y >= height()) {
            return 0;
        }
        // Shift 8-bit row (MSB is bit7) into uint32 MSB (bit31).
        return static_cast<uint32_t>(kRows[glyphIndex][y]) << 24;
    }

private:
    // Header-only safe in C++17: inline variable, no multiple-definition issues.
    inline static constexpr Row kRows[kCount][kHeight] = {
        { // 0
            0b11100000,
            0b10100000,
            0b10100000,
            0b10100000,
            0b11100000,
        },
        { // 1
            0b00100000,
            0b00100000,
            0b00100000,
            0b00100000,
            0b00100000,
        },
        { // 2
            0b11100000,
            0b00100000,
            0b11100000,
            0b10000000,
            0b11100000,
        },
        { // 3
            0b11100000,
            0b00100000,
            0b11100000,
            0b00100000,
            0b11100000,
        },
        { // 4
            0b10100000,
            0b10100000,
            0b11100000,
            0b00100000,
            0b00100000,
        },
        { // 5
            0b11100000,
            0b10000000,
            0b11100000,
            0b00100000,
            0b11100000,
        },
        { // 6
            0b11100000,
            0b10000000,
            0b11100000,
            0b10100000,
            0b11100000,
        },
        { // 7
            0b11100000,
            0b00100000,
            0b00100000,
            0b00100000,
            0b00100000,
        },
        { // 8
            0b11100000,
            0b10100000,
            0b11100000,
            0b10100000,
            0b11100000,
        },
        { // 9
            0b11100000,
            0b10100000,
            0b11100000,
            0b00100000,
            0b11100000,
        },
    };
};

// Monospace 4x7 digit font (glyphs 0..9).
//
// Storage format:
// - Each row is stored in the top bits of a byte: bit7 is x=0, bit6 is x=1, etc.
// - getRowBits() returns the same row aligned to MSB of uint32_t: bit31 is x=0.
class csFont4x7Digits final : public csFontBase {
public:
    using Row = uint8_t;

    static constexpr uint16_t kWidth = 4;
    static constexpr uint16_t kHeight = 7;
    static constexpr uint16_t kCount = 10;

    uint16_t width() const noexcept override { return kWidth; }
    uint16_t height() const noexcept override { return kHeight; }
    uint16_t count() const noexcept override { return kCount; }

    uint32_t getRowBits(uint16_t glyphIndex, uint16_t y) const noexcept override {
        if (glyphIndex >= count() || y >= height()) {
            return 0;
        }
        // Shift 8-bit row (MSB is bit7) into uint32 MSB (bit31).
        return static_cast<uint32_t>(kRows[glyphIndex][y]) << 24;
    }

private:
    // Header-only safe in C++17: inline variable, no multiple-definition issues.
    inline static constexpr Row kRows[kCount][kHeight] = {
        { // 0
            0b11110000,
            0b10010000,
            0b10010000,
            0b10010000,
            0b10010000,
            0b10010000,
            0b11110000,
        },
        { // 1
            0b00010000,
            0b00010000,
            0b00010000,
            0b00010000,
            0b00010000,
            0b00010000,
            0b00010000,
        },
        { // 2
            0b11110000,
            0b00010000,
            0b00010000,
            0b11110000,
            0b10000000,
            0b10000000,
            0b11110000,
        },
        { // 3
            0b11110000,
            0b00010000,
            0b00010000,
            0b11110000,
            0b00010000,
            0b00010000,
            0b11110000,
        },
        { // 4
            0b10010000,
            0b10010000,
            0b10010000,
            0b11110000,
            0b00010000,
            0b00010000,
            0b00010000,
        },
        { // 5
            0b11110000,
            0b10000000,
            0b10000000,
            0b11110000,
            0b00010000,
            0b00010000,
            0b11110000,
        },
        { // 6
            0b11110000,
            0b10000000,
            0b10000000,
            0b11110000,
            0b10010000,
            0b10010000,
            0b11110000,
        },
        { // 7
            0b11110000,
            0b00010000,
            0b00010000,
            0b00010000,
            0b00010000,
            0b00010000,
            0b00010000,
        },
        { // 8
            0b11110000,
            0b10010000,
            0b10010000,
            0b11110000,
            0b10010000,
            0b10010000,
            0b11110000,
        },
        { // 9
            0b11110000,
            0b10010000,
            0b10010000,
            0b11110000,
            0b00010000,
            0b00010000,
            0b11110000,
        },
    };
};

// Monospace 4x7 digit font (glyphs 0..9).
//
// Storage format:
// - Each row is stored in the top bits of a byte: bit7 is x=0, bit6 is x=1, etc.
// - getRowBits() returns the same row aligned to MSB of uint32_t: bit31 is x=0.
class csFont4x7DigitsRound final : public csFontBase {
public:
    using Row = uint8_t;

    static constexpr uint16_t kWidth = 4;
    static constexpr uint16_t kHeight = 7;
    static constexpr uint16_t kCount = 10;

    uint16_t width() const noexcept override { return kWidth; }
    uint16_t height() const noexcept override { return kHeight; }
    uint16_t count() const noexcept override { return kCount; }

    uint32_t getRowBits(uint16_t glyphIndex, uint16_t y) const noexcept override {
        if (glyphIndex >= count() || y >= height()) {
            return 0;
        }
        // Shift 8-bit row (MSB is bit7) into uint32 MSB (bit31).
        return static_cast<uint32_t>(kRows[glyphIndex][y]) << 24;
    }

private:
    // Header-only safe in C++17: inline variable, no multiple-definition issues.
    inline static constexpr Row kRows[kCount][kHeight] = {
        { // 0
            0b01100000,
            0b10010000,
            0b10010000,
            0b10010000,
            0b10010000,
            0b10010000,
            0b01100000,
        },
        { // 1
            0b00010000,
            0b00010000,
            0b00010000,
            0b00010000,
            0b00010000,
            0b00010000,
            0b00010000,
        },
        { // 2
            0b11100000,
            0b00010000,
            0b00010000,
            0b01100000,
            0b10000000,
            0b10000000,
            0b11110000,
        },
        { // 3
            0b11100000,
            0b00010000,
            0b00010000,
            0b11100000,
            0b00010000,
            0b00010000,
            0b11100000,
        },
        { // 4
            0b10010000,
            0b10010000,
            0b10010000,
            0b11110000,
            0b00010000,
            0b00010000,
            0b00010000,
        },
        { // 5
            0b11110000,
            0b10000000,
            0b10000000,
            0b11100000,
            0b00010000,
            0b00010000,
            0b11100000,
        },
        { // 6
            0b11110000,
            0b10000000,
            0b10000000,
            0b11100000,
            0b10010000,
            0b10010000,
            0b01100000,
        },
        { // 7
            0b11110000,
            0b00010000,
            0b00010000,
            0b00010000,
            0b00010000,
            0b00010000,
            0b00010000,
        },
        { // 8
            0b01100000,
            0b10010000,
            0b10010000,
            0b01100000,
            0b10010000,
            0b10010000,
            0b01100000,
        },
        { // 9
            0b01100000,
            0b10010000,
            0b10010000,
            0b01110000,
            0b00010000,
            0b00010000,
            0b11100000,
        },
    };
};




class csFont4x7DigitClock final : public csFontBase {
public:
    using Row = uint8_t;

    static constexpr uint16_t kWidth = 4;
    static constexpr uint16_t kHeight = 7;
    static constexpr uint16_t kCount = 10;

    uint16_t width() const noexcept override { return kWidth; }
    uint16_t height() const noexcept override { return kHeight; }
    uint16_t count() const noexcept override { return kCount; }

    uint32_t getRowBits(uint16_t glyphIndex, uint16_t y) const noexcept override {
        if (glyphIndex >= count() || y >= height()) {
            return 0;
        }
        // Shift 8-bit row (MSB is bit7) into uint32 MSB (bit31).
        return static_cast<uint32_t>(kRows[glyphIndex][y]) << 24;
    }

private:
    // Header-only safe in C++17: inline variable, no multiple-definition issues.
    inline static constexpr Row kRows[kCount][kHeight] = {
        { // 0
            0b01100000,
            0b10010000,
            0b10010000,
            0b00000000,
            0b10010000,
            0b10010000,
            0b01100000,
        },
        { // 1
            0b00000000,
            0b00010000,
            0b00010000,
            0b00000000,
            0b00010000,
            0b00010000,
            0b00000000,
        },
        { // 2
            0b01100000,
            0b00010000,
            0b00010000,
            0b01100000,
            0b10000000,
            0b10000000,
            0b01100000,
        },
        { // 3
            0b01100000,
            0b00010000,
            0b00010000,
            0b01100000,
            0b00010000,
            0b00010000,
            0b01100000,
        },
        { // 4
            0b00000000,
            0b10010000,
            0b10010000,
            0b01100000,
            0b00010000,
            0b00010000,
            0b00000000,
        },
        { // 5
            0b01100000,
            0b10000000,
            0b10000000,
            0b01100000,
            0b00010000,
            0b00010000,
            0b01100000,
        },
        { // 6
            0b01110000,
            0b10000000,
            0b10000000,
            0b01100000,
            0b10010000,
            0b10010000,
            0b01100000,
        },
        { // 7
            0b01100000,
            0b00010000,
            0b00010000,
            0b00000000,
            0b00010000,
            0b00010000,
            0b00000000,
        },
        { // 8
            0b01100000,
            0b10010000,
            0b10010000,
            0b01100000,
            0b10010000,
            0b10010000,
            0b01100000,
        },
        { // 9
            0b01100000,
            0b10010000,
            0b10010000,
            0b01100000,
            0b00010000,
            0b00010000,
            0b01100000,
        },
    };
};

} // namespace amp
