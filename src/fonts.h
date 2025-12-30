#pragma once

#include <stdint.h>

#include "font_base.hpp"
#include "amp_progmem.hpp"

namespace amp {

/*
Template function for font accessors: avoids global objects and ODR issues in header-only mode.
Example usage:
```
  const auto& font = getStaticFontTemplate<csFont3x5DigitalClock>();
  glyph->setFont(font);
```
*/
template<typename FontType>
inline const FontType& getStaticFontTemplate() noexcept {
    static FontType f;
    return f;
}

// Monospace 3x5 digit font (glyphs 0..9).
class csFont3x5Digits : public csFontBase {
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
        return static_cast<uint32_t>(pgm_read_byte(&kRows[glyphIndex][y])) << 24;
    }

private:
    static const Row kRows[kCount][kHeight] PROGMEM;
};

const csFont3x5Digits::Row csFont3x5Digits::kRows[csFont3x5Digits::kCount][csFont3x5Digits::kHeight] PROGMEM = {
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



/*
Monospace 3x5 digit font (glyphs 0..9).

In 7-segment displays, segments are labeled A–G. Standard layout:
```
    A
   ---
F |   | B
   -G-
E |   | C
   ---
    D
```

A — top horizontal
B — top right vertical
C — bottom right vertical
D — bottom horizontal
E — bottom left vertical
F — top left vertical
G — middle horizontal

The font rows below map segments to bit positions in each row:
```

.A.   0b0A000000,
F.B   0bF0B00000,
.G.   0b0G000000,
E.C   0bE0C00000,
.D.   0b0D000000,

```
*/
class csFont3x5DigitalClock : public csFontBase {
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
        return static_cast<uint32_t>(pgm_read_byte(&kRows[glyphIndex][y])) << 24;
    }

private:
    static const Row kRows[kCount][kHeight] PROGMEM;
};

const csFont3x5DigitalClock::Row csFont3x5DigitalClock::kRows[csFont3x5DigitalClock::kCount][csFont3x5DigitalClock::kHeight] PROGMEM = {
    { // 0
        0b01000000,
        0b10100000,
        0b00000000,
        0b10100000,
        0b01000000,
    },
    { // 1
        0b00000000,
        0b00100000,
        0b00000000,
        0b00100000,
        0b00000000,
    },
    { // 2
        0b01000000,
        0b00100000,
        0b01000000,
        0b10000000,
        0b01000000,
    },
    { // 3
        0b01000000,
        0b00100000,
        0b01000000,
        0b00100000,
        0b01000000,
    },
    { // 4
        0b00000000,
        0b10100000,
        0b01000000,
        0b00100000,
        0b00000000,
    },
    { // 5
        0b01000000,
        0b10000000,
        0b01000000,
        0b00100000,
        0b01000000,
    },
    { // 6
        0b01000000,
        0b10000000,
        0b01000000,
        0b10100000,
        0b01000000,
    },
    { // 7
        0b01000000,
        0b00100000,
        0b00000000,
        0b00100000,
        0b00000000,
    },
    { // 8
        0b01000000,
        0b10100000,
        0b01000000,
        0b10100000,
        0b01000000,
    },
    { // 9
        0b01000000,
        0b10100000,
        0b01000000,
        0b00100000,
        0b01000000,
    },
};



// Monospace 4x7 digit font (glyphs 0..9).
class csFont4x7Digits : public csFontBase {
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
        return static_cast<uint32_t>(pgm_read_byte(&kRows[glyphIndex][y])) << 24;
    }

private:
    static const Row kRows[kCount][kHeight] PROGMEM;
};

const csFont4x7Digits::Row csFont4x7Digits::kRows[csFont4x7Digits::kCount][csFont4x7Digits::kHeight] PROGMEM = {
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

// Monospace 4x7 digit font (glyphs 0..9).
class csFont4x7DigitsRound : public csFontBase {
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
        return static_cast<uint32_t>(pgm_read_byte(&kRows[glyphIndex][y])) << 24;
    }

private:
    static const Row kRows[kCount][kHeight] PROGMEM;
};

const csFont4x7DigitsRound::Row csFont4x7DigitsRound::kRows[csFont4x7DigitsRound::kCount][csFont4x7DigitsRound::kHeight] PROGMEM = {
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



/*
Monospace 4x7 digit font (glyphs 0..9).

```
    ## 
   #  #
   #  #
    ## 
   #  #
   #  #
    ## 
```

In 7-segment displays, segments are labeled A–G. Standard layout:
```
    A
   ---
F |   | B
   -G-
E |   | C
   ---
    D
```

A — top horizontal
B — top right vertical
C — bottom right vertical
D — bottom horizontal
E — bottom left vertical
F — top left vertical
G — middle horizontal

The font rows below map segments to bit positions in each row:

```
.A.   0b0A000000,
F.B   0bF0B00000,
F.B   0bF0B00000,
.G.   0b0G000000,
E.C   0bE0C00000,
E.C   0bE0C00000,
.D.   0b0D000000,
```
*/
class csFont4x7DigitalClock : public csFontBase {
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
        return static_cast<uint32_t>(pgm_read_byte(&kRows[glyphIndex][y])) << 24;
    }

private:
    static const Row kRows[kCount][kHeight] PROGMEM;
};

const csFont4x7DigitalClock::Row csFont4x7DigitalClock::kRows[csFont4x7DigitalClock::kCount][csFont4x7DigitalClock::kHeight] PROGMEM = {
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
        0b01100000,
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

} // namespace amp
