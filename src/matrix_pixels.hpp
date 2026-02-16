#pragma once

#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "color_rgba.hpp"
#include "matrix_base.hpp"
#include "matrix_types.hpp"
#include "rect.hpp"
#include "math.hpp"
#include "fixed_point.hpp"

namespace amp {

using ::size_t;
using ::uint8_t;
using math::max;
using math::min;
using math::csFP16;

// Header-only RGBA pixel matrix with straight-alpha SourceOver blending.
// Color format: 0xAARRGGBB (A in the most significant byte).
class csMatrixPixels : public csMatrixBase {
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

    [[nodiscard]] tMatrixPixelsSize width() const noexcept override { return size_x_; }
    [[nodiscard]] tMatrixPixelsSize height() const noexcept override { return size_y_; }

    // Overwrite pixel. Out-of-bounds writes are silently ignored.
    inline void setPixelRewrite(tMatrixPixelsCoord x, tMatrixPixelsCoord y, csColorRGBA color) noexcept override {
        if (inside(x, y)) {
            pixels_[index(x, y)] = color;
        }
    }

    // Blend source color over destination pixel using SourceOver (straight alpha).
    void setPixel(tMatrixPixelsCoord x, tMatrixPixelsCoord y, csColorRGBA color) noexcept override {
        if (inside(x, y)) {
            const csColorRGBA dst = pixels_[index(x, y)];
            pixels_[index(x, y)] = csColorRGBA::sourceOverStraight(dst, color);
        }
    }

    // Blend source color over destination pixels using sub-pixel positioning.
    // Fixed-point coordinates allow positioning between pixels; color is distributed across 1-2 pixels
    // with alpha proportional to distance from pixel center.
    void setPixelFloat2(csFP16 x, csFP16 y, csColorRGBA color) noexcept {
        // Round to nearest pixel to find the center pixel
        const tMatrixPixelsCoord cx = static_cast<tMatrixPixelsCoord>(x.round_int());
        const tMatrixPixelsCoord cy = static_cast<tMatrixPixelsCoord>(y.round_int());

        // Check if exact pixel center (no fractional part)
        if (x.frac_abs_raw() == 0 && y.frac_abs_raw() == 0) {
            // Draw single pixel with full alpha
            setPixel(cx, cy, color);
            return;
        }

        // Calculate offset from rounded center to determine direction.
        // Compute offsets from the center pixel. These tell us how far the point is from the center
        // Note: cx/cy are integer pixel coords; we convert them to csFP16 via constructor
        // (exact, no float) so dx/dy are computed in fixed-point units.
        const csFP16 cx_fp = csFP16(cx);
        const csFP16 cy_fp = csFP16(cy);

        // dx/dy: signed sub-pixel offset from the rounded-center pixel (typically in [-0.5, +0.5]).
        // Used to pick the sign (+1/-1) for the secondary pixel direction and to compute alpha weights.
        const csFP16 dx = x - cx_fp;
        const csFP16 dy = y - cy_fp;

        // Select secondary pixel direction.
        // IMPORTANT: axis decision uses original fractional parts (relative to the integer grid),
        // not the abs offset relative to the rounded center (which can lose info, e.g. 2.75 -> -0.25).
        tMatrixPixelsCoord sx = cx;
        tMatrixPixelsCoord sy = cy;

        const uint8_t fx_axis = static_cast<uint8_t>(x.frac_abs_raw());
        const uint8_t fy_axis = static_cast<uint8_t>(y.frac_abs_raw());

        if (fy_axis > fx_axis) {
            // Vertical direction
            sy = cy + (dy.raw_value() >= 0 ? 1 : -1);
        } else if (fx_axis > fy_axis) {
            // Horizontal direction
            sx = cx + (dx.raw_value() >= 0 ? 1 : -1);
        } else {
            // Diagonal direction (equal components)
            sx = cx + (dx.raw_value() >= 0 ? 1 : -1);
            sy = cy + (dy.raw_value() >= 0 ? 1 : -1);
        }

        // Get absolute fractional parts for weight calculation
        const uint8_t fx_abs = static_cast<uint8_t>(dx.frac_abs_raw());
        const uint8_t fy_abs = static_cast<uint8_t>(dy.frac_abs_raw());

        // Calculate alpha weights using absolute fractional parts
        const uint8_t max_offset_raw = max(fx_abs, fy_abs);
        const uint8_t weight_uint8 = static_cast<uint8_t>((static_cast<uint16_t>(max_offset_raw) * 255u + 8u) / 16u);

        const uint8_t secondary_alpha = mul8(color.a, weight_uint8);
        const uint8_t center_alpha = color.a - secondary_alpha;

        if (center_alpha > 0) {
            setPixel(cx, cy, csColorRGBA{center_alpha, color.r, color.g, color.b});
        }
        if (secondary_alpha > 0) {
            setPixel(sx, sy, csColorRGBA{secondary_alpha, color.r, color.g, color.b});
        }
    }

    // Blend source color over destination pixels using classical 4-tap bilinear splat.
    // Integer coordinates are treated as pixel centers, so (10.0, 1.0) affects exactly one pixel.
    // The source color is distributed to the 4 neighboring pixel centers around floor(x), floor(y).
    void setPixelFloat4(csFP16 x, csFP16 y, csColorRGBA color) noexcept {
        // Fast path: exact pixel center.
        if (x.frac_abs_raw() == 0 && y.frac_abs_raw() == 0) {
            setPixel(static_cast<tMatrixPixelsCoord>(x.int_trunc()),
                     static_cast<tMatrixPixelsCoord>(y.int_trunc()),
                     color);
            return;
        }

        // Use floor-based cell origin so fractions are always in [0,1) even for negative coordinates.
        const tMatrixPixelsCoord x0 = static_cast<tMatrixPixelsCoord>(x.floor_int());
        const tMatrixPixelsCoord y0 = static_cast<tMatrixPixelsCoord>(y.floor_int());

        const csFP16 fx = x - csFP16(x0);
        const csFP16 fy = y - csFP16(y0);

        // csFP16 is 12.4, so the fractional part is 0..15 (scale=16).
        const uint16_t fx_raw = static_cast<uint16_t>(fx.frac_abs_raw());
        const uint16_t fy_raw = static_cast<uint16_t>(fy.frac_abs_raw());
        const uint16_t inv_fx = static_cast<uint16_t>(csFP16::scale) - fx_raw; // 16 - fx
        const uint16_t inv_fy = static_cast<uint16_t>(csFP16::scale) - fy_raw; // 16 - fy

        // Weights sum to 256 (16*16). Each weight is in [0..256].
        const uint16_t w00 = static_cast<uint16_t>(inv_fx * inv_fy);
        const uint16_t w10 = static_cast<uint16_t>(fx_raw * inv_fy);
        const uint16_t w01 = static_cast<uint16_t>(inv_fx * fy_raw);
        const uint16_t w11 = static_cast<uint16_t>(fx_raw * fy_raw);

        auto weight_to_alpha = [&](uint16_t w) noexcept -> uint8_t {
            // Round to nearest: (a*w)/256
            return static_cast<uint8_t>((static_cast<uint32_t>(color.a) * static_cast<uint32_t>(w) + 128u) >> 8);
        };

        const uint8_t a00 = weight_to_alpha(w00);
        const uint8_t a10 = weight_to_alpha(w10);
        const uint8_t a01 = weight_to_alpha(w01);
        const uint8_t a11 = weight_to_alpha(w11);

        if (a00 > 0) {
            setPixel(x0, y0, csColorRGBA{a00, color.r, color.g, color.b});
        }
        if (a10 > 0) {
            setPixel(x0 + 1, y0, csColorRGBA{a10, color.r, color.g, color.b});
        }
        if (a01 > 0) {
            setPixel(x0, y0 + 1, csColorRGBA{a01, color.r, color.g, color.b});
        }
        if (a11 > 0) {
            setPixel(x0 + 1, y0 + 1, csColorRGBA{a11, color.r, color.g, color.b});
        }
    }

    // Read pixel; returns transparent black when out of bounds.
    [[nodiscard]] inline csColorRGBA getPixel(tMatrixPixelsCoord x, tMatrixPixelsCoord y) const noexcept override {
        if (inside(x, y)) {
            return pixels_[index(x, y)];
        }
        return csColorRGBA{0, 0, 0, 0};
    }

    // Clear matrix to transparent black.
    void clear() noexcept {
        const size_t bytes = count() * sizeof(csColorRGBA);
        if (bytes != 0 && pixels_) {
            // Fast zero-fill; csColorRGBA is 4 bytes (see static_assert in color_rgba.hpp).
            memset(static_cast<void*>(pixels_), 0, bytes);
        }
    }

    /*
    TODO:

    `void getScanLine(csRect area, offset_x, offset_y, OUT &* ptrLineOfColors, OUT & lineLen)`

    `void fill(csRect area, color)`

    // overwrite dst area. fast.
    `bool copyMatrix(dst_x, dst_y, const csMatrixPixels& source)`
    */

    // Resize matrix to new dimensions. Existing pixels are lost (matrix is cleared).
    void resize(uint16_t sx, uint16_t sy) {
        if (sx == size_x_ && sy == size_y_) {
            return;
        }
        delete[] pixels_;
        // TODO: добавиь проверку на нулевой размер - в таком случае `pixels_ = nullptr`.
        size_x_ = sx;
        size_y_ = sy;
        pixels_ = allocate(size_x_, size_y_);
    }

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
};


} // namespace amp

