#pragma once

#include <cstddef>
#include <cstdint>
#include "color_rgba.hpp"

using std::size_t;
using std::uint8_t;
using std::uint16_t;

using tMatrixPixelsCoord = std::int32_t;
using tMatrixPixelsSize = std::uint16_t;

template <typename T>
constexpr T max_c(T a, T b) noexcept {
    return (a > b) ? a : b;
}

template <typename T>
constexpr T min_c(T a, T b) noexcept {
    return (a < b) ? a : b;
}

// Header-only RGBA pixel matrix with straight-alpha SourceOver blending.
// Color format: 0xAARRGGBB (A in the most significant byte).
class csMatrixPixels {
public:
    // Construct matrix with given size, all pixels cleared.
    csMatrixPixels(tMatrixPixelsSize size_x, tMatrixPixelsSize size_y)
        : size_x_{size_x}, size_y_{size_y}, pixels_(allocate(size_x, size_y)) {}

    // Copy constructor: makes deep copy of pixel buffer.
    // Triggers when you pass by value or return by value, e.g.:
    //   csMatrixPixels b = a;  // invokes copy ctor
    //   auto f() { return a; } // NRVO elided or copy/move ctor
    csMatrixPixels(const csMatrixPixels& other)
        : size_x_{other.size_x_}, size_y_{other.size_y_}, pixels_(allocate(size_x_, size_y_)) {
        copyPixels(pixels_, other.pixels_, count());
    }

    // Move constructor: transfers ownership of buffer, leaving source empty.
    // Triggers on std::move or returning a temporary, e.g.:
    //   csMatrixPixels b = std::move(a);
    //   return csMatrixPixels{w,h};
    csMatrixPixels(csMatrixPixels&& other) noexcept
        : size_x_{other.size_x_}, size_y_{other.size_y_}, pixels_{other.pixels_} {
        other.pixels_ = nullptr;
        other.size_x_ = 0;
        other.size_y_ = 0;
    }

    // Copy assignment: deep copy when assigning existing object.
    //   b = a;
    csMatrixPixels& operator=(const csMatrixPixels& other) {
        if (this != &other) {
            resize(other.size_x_, other.size_y_);
            copyPixels(pixels_, other.pixels_, count());
        }
        return *this;
    }

    // Move assignment: steal buffer from other, reset other to empty.
    //   b = std::move(a);
    csMatrixPixels& operator=(csMatrixPixels&& other) noexcept {
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

    ~csMatrixPixels() { delete[] pixels_; }

    [[nodiscard]] tMatrixPixelsSize width() const noexcept { return size_x_; }
    [[nodiscard]] tMatrixPixelsSize height() const noexcept { return size_y_; }

    // Overwrite pixel. Out-of-bounds writes are silently ignored.
    inline void setPixel(tMatrixPixelsCoord x, tMatrixPixelsCoord y, csColorRGBA color) noexcept {
        if (inside(x, y)) {
            pixels_[index(x, y)] = color;
        }
    }

    // Blend source color over destination pixel using SourceOver (straight alpha).
    // 'alpha' is an extra global multiplier for the source alpha channel.
    inline void setPixelBlend(tMatrixPixelsCoord x, tMatrixPixelsCoord y, csColorRGBA color, uint8_t alpha) noexcept {
        if (inside(x, y)) {
            const csColorRGBA dst = pixels_[index(x, y)];
            pixels_[index(x, y)] = csColorRGBA::sourceOverStraight(dst, color, alpha);
        }
    }

    // Blend source color over destination pixel using SourceOver with only the pixel's own alpha.
    inline void setPixelBlend(tMatrixPixelsCoord x, tMatrixPixelsCoord y, csColorRGBA color) noexcept {
        if (inside(x, y)) {
            const csColorRGBA dst = pixels_[index(x, y)];
            pixels_[index(x, y)] = csColorRGBA::sourceOverStraight(dst, color);
        }
    }

    // Read pixel; returns transparent black when out of bounds.
    [[nodiscard]] inline csColorRGBA getPixel(tMatrixPixelsCoord x, tMatrixPixelsCoord y) const noexcept {
        if (inside(x, y)) {
            return pixels_[index(x, y)];
        }
        return csColorRGBA{0, 0, 0, 0};
    }

    // Compute blended color of matrix pixel over bgColor; does not modify matrix.
    // Out-of-bounds returns bgColor unchanged.
    [[nodiscard]] inline csColorRGBA getPixelBlend(tMatrixPixelsCoord x, tMatrixPixelsCoord y, csColorRGBA bgColor) const noexcept {
        if (inside(x, y)) {
            return csColorRGBA::sourceOverStraight(bgColor, pixels_[index(x, y)]);
        }
        return bgColor;
    }

    // Draw another matrix over this one with clipping. Source alpha is respected and additionally scaled by 'alpha'.
    inline void drawMatrix(tMatrixPixelsCoord dst_x, tMatrixPixelsCoord dst_y, const csMatrixPixels& source, uint8_t alpha = 255) noexcept {
        const tMatrixPixelsCoord start_x = max_c<tMatrixPixelsCoord>(0, -dst_x);
        const tMatrixPixelsCoord start_y = max_c<tMatrixPixelsCoord>(0, -dst_y);
        const tMatrixPixelsCoord end_x = min_c<tMatrixPixelsCoord>(source.width(), static_cast<tMatrixPixelsCoord>(width()) - dst_x);
        const tMatrixPixelsCoord end_y = min_c<tMatrixPixelsCoord>(source.height(), static_cast<tMatrixPixelsCoord>(height()) - dst_y);

        for (tMatrixPixelsCoord sy = start_y; sy < end_y; ++sy) {
            const tMatrixPixelsCoord dy = sy + dst_y;
            for (tMatrixPixelsCoord sx = start_x; sx < end_x; ++sx) {
                const tMatrixPixelsCoord dx = sx + dst_x;
                setPixelBlend(dx, dy, source.getPixel(sx, sy), alpha);
            }
        }
    }

private:
    tMatrixPixelsSize size_x_;
    tMatrixPixelsSize size_y_;
    csColorRGBA* pixels_{nullptr};

    [[nodiscard]] constexpr bool inside(tMatrixPixelsCoord x, tMatrixPixelsCoord y) const noexcept {
        return x >= 0 && y >= 0 && x < static_cast<tMatrixPixelsCoord>(size_x_) && y < static_cast<tMatrixPixelsCoord>(size_y_);
    }

    [[nodiscard]] constexpr size_t index(tMatrixPixelsCoord x, tMatrixPixelsCoord y) const noexcept {
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


