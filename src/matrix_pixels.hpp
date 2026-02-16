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

    // Compute blended color of matrix pixel over bgColor; does not modify matrix.
    // Out-of-bounds returns bgColor unchanged.
    [[nodiscard]] inline csColorRGBA getPixelBlend(tMatrixPixelsCoord x, tMatrixPixelsCoord y, csColorRGBA bgColor) const noexcept {
        if (inside(x, y)) {
            return csColorRGBA::sourceOverStraight(bgColor, pixels_[index(x, y)]);
        }
        return bgColor;
    }

    // Draw another matrix over this one with clipping. Source alpha is respected and additionally scaled by 'alpha'.
    void drawMatrix(tMatrixPixelsCoord dst_x, tMatrixPixelsCoord dst_y, const csMatrixPixels& source, uint8_t alpha = 255) noexcept {
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
                setPixel(dx, dy, source.getPixel(sx, sy).alpha(alpha));
            }
        }
    }

    // Draw specific source area to destination coordinates with clipping.
    // 'src' is the area to copy from source matrix, (dst_x, dst_y) is where to draw in this matrix.
    // Source alpha is respected and additionally scaled by 'alpha'.
    void drawMatrixArea(csRect src, tMatrixPixelsCoord dst_x, tMatrixPixelsCoord dst_y,
                       const csMatrixPixels& source, uint8_t alpha = 255) noexcept {
        // Clip source rectangle to source matrix bounds
        const csRect srcBounds = source.getRect();
        const csRect srcClipped = src.intersect(srcBounds);
        if (srcClipped.empty()) {
            return;
        }
        
        // Copy pixels from source area to destination coordinates
        for (tMatrixPixelsSize y = 0; y < srcClipped.height; ++y) {
            for (tMatrixPixelsSize x = 0; x < srcClipped.width; ++x) {
                const csColorRGBA pixel = source.getPixel(srcClipped.x + static_cast<tMatrixPixelsCoord>(x), 
                                                           srcClipped.y + static_cast<tMatrixPixelsCoord>(y));
                setPixel(dst_x + static_cast<tMatrixPixelsCoord>(x),
                        dst_y + static_cast<tMatrixPixelsCoord>(y),
                        pixel.alpha(alpha));
            }
        }
    }

    // Draw specific source area to destination coordinates with clipping, overwriting destination pixels.
    // Unlike drawMatrixArea(), this does NOT blend; it writes pixels directly.
    // Useful for internal buffers where pixels are treated as data (e.g. heat fields), not composited imagery.
    void drawMatrixAreaRewrite(csRect src, tMatrixPixelsCoord dst_x, tMatrixPixelsCoord dst_y,
                              const csMatrixPixels& source) noexcept {
        // Clip source rectangle to source matrix bounds
        const csRect srcBounds = source.getRect();
        const csRect srcClipped = src.intersect(srcBounds);
        if (srcClipped.empty()) {
            return;
        }

        // Apply global alpha multiplier by scaling the source alpha channel, then overwrite.
        for (tMatrixPixelsSize y = 0; y < srcClipped.height; ++y) {
            for (tMatrixPixelsSize x = 0; x < srcClipped.width; ++x) {
                const csColorRGBA pixel = source.getPixel(srcClipped.x + static_cast<tMatrixPixelsCoord>(x),
                                                          srcClipped.y + static_cast<tMatrixPixelsCoord>(y));
                setPixelRewrite(dst_x + static_cast<tMatrixPixelsCoord>(x),
                                dst_y + static_cast<tMatrixPixelsCoord>(y),
                                pixel);
            }
        }
    }

    // Clear matrix to transparent black.
    void clear() noexcept {
        const size_t bytes = count() * sizeof(csColorRGBA);
        if (bytes != 0 && pixels_) {
            // Fast zero-fill; csColorRGBA is 4 bytes (see static_assert in color_rgba.hpp).
            memset(static_cast<void*>(pixels_), 0, bytes);
        }
    }

    // Fill rectangular area with color. Area is clipped to matrix bounds.
    void fillArea(csRect area, csColorRGBA color) noexcept {
        const csRect target = area.intersect(getRect());
        if (target.empty()) {
            return;
        }
        const tMatrixPixelsCoord endX = target.x + to_coord(target.width);
        const tMatrixPixelsCoord endY = target.y + to_coord(target.height);
        for (tMatrixPixelsCoord y = target.y; y < endY; ++y) {
            for (tMatrixPixelsCoord x = target.x; x < endX; ++x) {
                setPixel(x, y, color);
            }
        }
    }

    // Calc average color of area. Fast.
    // Uses two-level hierarchical averaging to avoid overflow in uint16_t accumulators.
    // Max area: 65536 pixels (256x256). Larger areas return transparent black.
    [[nodiscard]] csColorRGBA getAreaColor(csRect area) const noexcept {
        const csRect bounded = area.intersect(getRect());
        if (bounded.empty()) {
            return csColorRGBA{0, 0, 0, 0};
        }

        const tMatrixPixelsSize w = bounded.width;
        const tMatrixPixelsSize h = bounded.height;
        const uint32_t pixel_count = static_cast<uint32_t>(w) * static_cast<uint32_t>(h);

        // Two-level averaging: sum1 accumulates up to kChunk pixels, then averages into sum2.
        // kChunk chosen so sum1 doesn't overflow: 255 * 256 = 65280 < 65535 (uint16_t max).
        static constexpr uint16_t kChunk = 256;
        
        // kMaxPixels = kChunk * kChunk, ensuring sum2 doesn't overflow:
        // After div(kChunk), each component is max 255, so 255 * 256 = 65280 < 65535.
        static constexpr uint32_t kMaxPixels = static_cast<uint32_t>(kChunk) * kChunk;
        
        // Reject too large areas to avoid overflow; user should handle this.
        if (pixel_count > kMaxPixels) {
            return csColorRGBA{0, 0, 0, 0};
        }

        const csColorRGBA kZero{0, 0, 0, 0};

        csColorRGBA16 sum1;
        uint16_t count1 = 0;

        csColorRGBA16 sum2;
        uint16_t count2 = 0;

        const size_t stride = size_x_;
        const size_t row_start = index(bounded.x, bounded.y);
        const size_t row_advance = stride - static_cast<size_t>(w);

        const csColorRGBA* ptr = pixels_ + row_start;
        for (tMatrixPixelsSize iy = 0; iy < h; ++iy) {
            for (tMatrixPixelsSize ix = 0; ix < w; ++ix) {
                sum1 += (*ptr++).sum(kZero);
                ++count1;

                if (count1 == kChunk) {
                    sum2 += sum1.div(kChunk);
                    ++count2;
                    sum1 = csColorRGBA16{};
                    count1 = 0;
                }
            }
            ptr += row_advance;
        }

        if (count1 > 0) {
            sum2 += sum1.div(count1);
            ++count2;
        }

        if (count2 == 0) {
            return csColorRGBA{0, 0, 0, 0};
        }

        return sum2.toColor8(count2);
    }

    // Scale and draw source matrix onto this matrix using bilinear interpolation.
    // 'src' is the source rectangle in 'source' matrix, 'dst' is the destination rectangle in this matrix.
    // Returns false if src or dst is empty, or if src is out of bounds of source matrix.
    bool drawMatrixScale(csRect src, csRect dst, const csMatrixPixels& source) noexcept {
        // Validate input rectangles
        if (src.empty() || dst.empty()) {
            return false;
        }

        // Check if source rectangle is within source matrix bounds
        const csRect sourceBounds = source.getRect();
        const csRect srcBounded = src.intersect(sourceBounds);
        if (srcBounded.empty() || srcBounded.width != src.width || srcBounded.height != src.height) {
            return false;
        }

        // Clip destination rectangle to this matrix bounds
        const csRect dstBounded = dst.intersect(getRect());
        if (dstBounded.empty()) {
            return true; // Nothing to draw, but operation is valid
        }

        // Calculate scale factors based on original dst size (using fixed-point arithmetic for precision)
        // We use 16.16 fixed-point: multiply by 65536, then shift right by 16
        const int32_t scale_x_fp = (static_cast<int32_t>(src.width) << 16) / static_cast<int32_t>(dst.width);
        const int32_t scale_y_fp = (static_cast<int32_t>(src.height) << 16) / static_cast<int32_t>(dst.height);

        // Iterate over each pixel in clipped destination rectangle
        for (tMatrixPixelsSize dy = 0; dy < dstBounded.height; ++dy) {
            const tMatrixPixelsCoord dst_y = dstBounded.y + static_cast<tMatrixPixelsCoord>(dy);
            
            for (tMatrixPixelsSize dx = 0; dx < dstBounded.width; ++dx) {
                const tMatrixPixelsCoord dst_x = dstBounded.x + static_cast<tMatrixPixelsCoord>(dx);

                // Calculate corresponding position in source (fixed-point)
                // Map from dst coordinates to src coordinates, accounting for offset from dst origin
                const int32_t offset_x = dst_x - dst.x;
                const int32_t offset_y = dst_y - dst.y;
                const int32_t src_x_fp = (static_cast<int32_t>(src.x) << 16) + 
                                         (offset_x * scale_x_fp);
                const int32_t src_y_fp = (static_cast<int32_t>(src.y) << 16) + 
                                         (offset_y * scale_y_fp);

                // Extract integer and fractional parts
                const int32_t src_x_int = src_x_fp >> 16;
                const int32_t src_y_int = src_y_fp >> 16;
                const uint8_t fx = static_cast<uint8_t>((src_x_fp & 0xFFFF) >> 8); // 8-bit fractional part
                const uint8_t fy = static_cast<uint8_t>((src_y_fp & 0xFFFF) >> 8);

                // Get 4 neighboring pixels for bilinear interpolation
                const csColorRGBA p00 = source.getPixel(src_x_int, src_y_int);
                const csColorRGBA p10 = source.getPixel(src_x_int + 1, src_y_int);
                const csColorRGBA p01 = source.getPixel(src_x_int, src_y_int + 1);
                const csColorRGBA p11 = source.getPixel(src_x_int + 1, src_y_int + 1);

                // Bilinear interpolation: first interpolate horizontally (top and bottom)
                const csColorRGBA top = lerp(p00, p10, fx);
                const csColorRGBA bottom = lerp(p01, p11, fx);

                // Then interpolate vertically
                const csColorRGBA result = lerp(top, bottom, fy);

                // Draw the interpolated pixel with alpha blending
                setPixel(dst_x, dst_y, result);
            }
        }

        return true;
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

