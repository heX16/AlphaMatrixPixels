#pragma once

#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "color_rgba.hpp"
#include "matrix_types.hpp"
#include "rect.hpp"
#include "math.hpp"

namespace amp {

using ::size_t;
using ::uint8_t;
using math::max;
using math::min;

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

    // Return full matrix bounds as a rectangle: (0, 0, width, height).
    [[nodiscard]] inline csRect getRect() const noexcept { return csRect{0, 0, width(), height()}; }

    // Overwrite pixel. Out-of-bounds writes are silently ignored.
    inline void setPixelRewrite(tMatrixPixelsCoord x, tMatrixPixelsCoord y, csColorRGBA color) noexcept {
        if (inside(x, y)) {
            pixels_[index(x, y)] = color;
        }
    }

    // Blend source color over destination pixel using SourceOver (straight alpha).
    // 'alpha' is an extra global multiplier for the source alpha channel.
    inline void setPixel(tMatrixPixelsCoord x, tMatrixPixelsCoord y, csColorRGBA color, uint8_t alpha) noexcept {
        if (inside(x, y)) {
            const csColorRGBA dst = pixels_[index(x, y)];
            pixels_[index(x, y)] = csColorRGBA::sourceOverStraight(dst, color, alpha);
        }
    }

    // Blend source color over destination pixel using SourceOver with only the pixel's own alpha.
    inline void setPixel(tMatrixPixelsCoord x, tMatrixPixelsCoord y, csColorRGBA color) noexcept {
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
        const tMatrixPixelsCoord start_x = max(to_coord(0), -dst_x);
        const tMatrixPixelsCoord start_y = max(to_coord(0), -dst_y);
        const tMatrixPixelsCoord end_x = min(to_coord(source.width()),
                                             to_coord(width()) - dst_x);
        const tMatrixPixelsCoord end_y = min(to_coord(source.height()),
                                             to_coord(height()) - dst_y);

        for (tMatrixPixelsCoord sy = start_y; sy < end_y; ++sy) {
            const tMatrixPixelsCoord dy = sy + dst_y;
            for (tMatrixPixelsCoord sx = start_x; sx < end_x; ++sx) {
                const tMatrixPixelsCoord dx = sx + dst_x;
                setPixel(dx, dy, source.getPixel(sx, sy), alpha);
            }
        }
    }

    // Clear matrix to transparent black.
    inline void clear() noexcept {
        const size_t bytes = count() * sizeof(csColorRGBA);
        if (bytes != 0 && pixels_) {
            // Fast zero-fill; csColorRGBA is 4 bytes (see static_assert in color_rgba.hpp).
            memset(static_cast<void*>(pixels_), 0, bytes);
        }
    }

    // calc average color of area. fast.
    [[nodiscard]] inline csColorRGBA getAreaColor(csRect area) const noexcept {
        const csRect bounded = area.intersect(getRect());
        if (bounded.empty()) {
            return csColorRGBA{0, 0, 0, 0};
        }

        const tMatrixPixelsSize w = bounded.width;
        const tMatrixPixelsSize h = bounded.height;
        const uint64_t n = static_cast<uint64_t>(w) * static_cast<uint64_t>(h);

        uint64_t sum_a = 0;
        uint64_t sum_r = 0;
        uint64_t sum_g = 0;
        uint64_t sum_b = 0;

        const size_t stride = size_x_;
        const size_t row_start = index(bounded.x, bounded.y);
        const size_t row_advance = stride - static_cast<size_t>(w);

        const csColorRGBA* ptr = pixels_ + row_start;
        for (tMatrixPixelsSize iy = 0; iy < h; ++iy) {
            for (tMatrixPixelsSize ix = 0; ix < w; ++ix) {
                const csColorRGBA c = *ptr++;
                sum_a += c.a;
                sum_r += c.r;
                sum_g += c.g;
                sum_b += c.b;
            }
            ptr += row_advance;
        }

        // Average per channel with rounding to nearest.
        const uint64_t half = n / 2u;
        return csColorRGBA{
            static_cast<uint8_t>((sum_a + half) / n),
            static_cast<uint8_t>((sum_r + half) / n),
            static_cast<uint8_t>((sum_g + half) / n),
            static_cast<uint8_t>((sum_b + half) / n)
        };
    }

    /*
    TODO:

    `void getScanLine(csRect area, offset_x, offset_y, OUT &* ptrLineOfColors, OUT & lineLen)`

    `void fill(csRect area, color)`

    // slow and nice
    `bool drawMatrixScale(csRect src, csRect dst, const csMatrixPixels& source)`

    // overwrite dst area. fast.
    `bool copyMatrix(dst_x, dst_y, const csMatrixPixels& source)`
    */

private:
    tMatrixPixelsSize size_x_;
    tMatrixPixelsSize size_y_;
    csColorRGBA* pixels_{nullptr};

    [[nodiscard]] inline bool inside(tMatrixPixelsCoord x, tMatrixPixelsCoord y) const noexcept {
        return x >= 0 && y >= 0 && x < static_cast<tMatrixPixelsCoord>(size_x_) && y < static_cast<tMatrixPixelsCoord>(size_y_);
    }

    [[nodiscard]] inline size_t index(tMatrixPixelsCoord x, tMatrixPixelsCoord y) const noexcept {
        return static_cast<size_t>(y) * size_x_ + static_cast<size_t>(x);
    }

    [[nodiscard]] inline size_t count() const noexcept { return static_cast<size_t>(size_x_) * size_y_; }

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


} // namespace amp

