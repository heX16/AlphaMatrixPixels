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

    // Calc average color of area. Fast.
    // Uses two-level hierarchical averaging to avoid overflow in uint16_t accumulators.
    // Max area: 65536 pixels (256x256). Larger areas return transparent black.
    [[nodiscard]] inline csColorRGBA getAreaColor(csRect area) const noexcept {
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

