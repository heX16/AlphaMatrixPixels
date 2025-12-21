#pragma once

#include <cstddef>
#include <cstdint>
#include "cs_color_rgba.hpp"

using std::size_t;
using std::uint8_t;
using std::uint16_t;

using tPixelMatrixCoord = std::int32_t;
using tPixelMatrixSize = std::uint16_t;

template <typename T>
constexpr T max_c(T a, T b) noexcept {
    return (a > b) ? a : b;
}

template <typename T>
constexpr T min_c(T a, T b) noexcept {
    return (a < b) ? a : b;
}

// Header-only RGBA pixel matrix with straight-alpha SourceOver blending.
// Color format: 0xRRGGBBAA (R in the most significant byte, A in the least).
class csPixelMatrix {
public:
    // Construct matrix with given size, all pixels cleared.
    csPixelMatrix(tPixelMatrixSize size_x, tPixelMatrixSize size_y)
        : size_x_{size_x}, size_y_{size_y}, pixels_(allocate(size_x, size_y)) {}

    // Copy constructor: makes deep copy of pixel buffer.
    // Triggers when you pass by value or return by value, e.g.:
    //   csPixelMatrix b = a;  // invokes copy ctor
    //   auto f() { return a; } // NRVO elided or copy/move ctor
    csPixelMatrix(const csPixelMatrix& other)
        : size_x_{other.size_x_}, size_y_{other.size_y_}, pixels_(allocate(size_x_, size_y_)) {
        copyPixels(pixels_, other.pixels_, count());
    }

    // Move constructor: transfers ownership of buffer, leaving source empty.
    // Triggers on std::move or returning a temporary, e.g.:
    //   csPixelMatrix b = std::move(a);
    //   return csPixelMatrix{w,h};
    csPixelMatrix(csPixelMatrix&& other) noexcept
        : size_x_{other.size_x_}, size_y_{other.size_y_}, pixels_{other.pixels_} {
        other.pixels_ = nullptr;
        other.size_x_ = 0;
        other.size_y_ = 0;
    }

    // Copy assignment: deep copy when assigning existing object.
    //   b = a;
    csPixelMatrix& operator=(const csPixelMatrix& other) {
        if (this != &other) {
            resize(other.size_x_, other.size_y_);
            copyPixels(pixels_, other.pixels_, count());
        }
        return *this;
    }

    // Move assignment: steal buffer from other, reset other to empty.
    //   b = std::move(a);
    csPixelMatrix& operator=(csPixelMatrix&& other) noexcept {
        if (this != &other) {
            delete[] pixels_;
            size_x_ = other.size_x_;
            size_y_ = other.size_y_;
            pixels_ = other.pixels_;
            other.pixels_ = nullptr;
            other.size_x_ = 0;
            other.size_y_ = 0;
        }
        return *this;
    }

    ~csPixelMatrix() { delete[] pixels_; }

    [[nodiscard]] uint16_t width() const noexcept { return size_x_; }
    [[nodiscard]] uint16_t height() const noexcept { return size_y_; }

    // Overwrite pixel. Out-of-bounds writes are silently ignored.
    inline void setPixel(tPixelMatrixCoord x, tPixelMatrixCoord y, csColorRGBA color) noexcept {
        if (inside(x, y)) {
            pixels_[index(x, y)] = color;
        }
    }

    // Blend source color over destination pixel using SourceOver (straight alpha).
    // 'alpha' is an extra global multiplier for the source alpha channel.
    inline void setPixelBlend(tPixelMatrixCoord x, tPixelMatrixCoord y, csColorRGBA color, uint8_t alpha) noexcept {
        if (inside(x, y)) {
            const csColorRGBA dst = pixels_[index(x, y)];
            pixels_[index(x, y)] = csColorRGBA::sourceOverStraight(dst, color, alpha);
        }
    }

    // Blend source color over destination pixel using SourceOver with only the pixel's own alpha.
    inline void setPixelBlend(tPixelMatrixCoord x, tPixelMatrixCoord y, csColorRGBA color) noexcept {
        if (inside(x, y)) {
            const csColorRGBA dst = pixels_[index(x, y)];
            pixels_[index(x, y)] = csColorRGBA::sourceOverStraight(dst, color);
        }
    }

    // Read pixel; returns transparent black when out of bounds.
    [[nodiscard]] inline csColorRGBA getPixel(tPixelMatrixCoord x, tPixelMatrixCoord y) const noexcept {
        if (inside(x, y)) {
            return pixels_[index(x, y)];
        }
        return csColorRGBA{0, 0, 0, 0};
    }

    // Compute blended color of matrix pixel over bgColor; does not modify matrix.
    // Out-of-bounds returns bgColor unchanged.
    [[nodiscard]] inline csColorRGBA getPixelBlend(tPixelMatrixCoord x, tPixelMatrixCoord y, csColorRGBA bgColor) const noexcept {
        if (inside(x, y)) {
            return csColorRGBA::sourceOverStraight(bgColor, pixels_[index(x, y)]);
        }
        return bgColor;
    }

    // Draw another matrix over this one with clipping. Source alpha is respected and additionally scaled by 'alpha'.
    inline void drawMatrix(tPixelMatrixCoord dst_x, tPixelMatrixCoord dst_y, const csPixelMatrix& source, uint8_t alpha = 255) noexcept {
        const tPixelMatrixCoord start_x = max_c<tPixelMatrixCoord>(0, -dst_x);
        const tPixelMatrixCoord start_y = max_c<tPixelMatrixCoord>(0, -dst_y);
        const tPixelMatrixCoord end_x = min_c<tPixelMatrixCoord>(source.width(), static_cast<tPixelMatrixCoord>(width()) - dst_x);
        const tPixelMatrixCoord end_y = min_c<tPixelMatrixCoord>(source.height(), static_cast<tPixelMatrixCoord>(height()) - dst_y);

        for (tPixelMatrixCoord sy = start_y; sy < end_y; ++sy) {
            const tPixelMatrixCoord dy = sy + dst_y;
            for (tPixelMatrixCoord sx = start_x; sx < end_x; ++sx) {
                const tPixelMatrixCoord dx = sx + dst_x;
                setPixelBlend(dx, dy, source.getPixel(sx, sy), alpha);
            }
        }
    }

private:
    tPixelMatrixSize size_x_;
    tPixelMatrixSize size_y_;
    csColorRGBA* pixels_{nullptr};

    [[nodiscard]] constexpr bool inside(tPixelMatrixCoord x, tPixelMatrixCoord y) const noexcept {
        return x >= 0 && y >= 0 && x < static_cast<tPixelMatrixCoord>(size_x_) && y < static_cast<tPixelMatrixCoord>(size_y_);
    }

    [[nodiscard]] constexpr size_t index(tPixelMatrixCoord x, tPixelMatrixCoord y) const noexcept {
        return static_cast<size_t>(y) * size_x_ + static_cast<size_t>(x);
    }

    [[nodiscard]] constexpr size_t count() const noexcept { return static_cast<size_t>(size_x_) * size_y_; }

    [[nodiscard]] static csColorRGBA* allocate(uint16_t sx, uint16_t sy) {
        const size_t n = static_cast<size_t>(sx) * static_cast<size_t>(sy);
        csColorRGBA* buf = n ? new csColorRGBA[n] : nullptr;
        for (size_t i = 0; i < n; ++i) {
            buf[i] = csColorRGBA{0, 0, 0, 0};
        }
        return buf;
    }

    static void copyPixels(csColorRGBA* dst, const csColorRGBA* src, size_t n) {
        for (size_t i = 0; i < n; ++i) {
            dst[i] = src[i];
        }
    }

    void resize(uint16_t sx, uint16_t sy) {
        if (sx == size_x_ && sy == size_y_) {
            return;
        }
        delete[] pixels_;
        size_x_ = sx;
        size_y_ = sy;
        pixels_ = allocate(size_x_, size_y_);
    }
};


