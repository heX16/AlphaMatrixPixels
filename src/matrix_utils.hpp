#pragma once

#include "color_rgba.hpp"
#include "matrix_base.hpp"
#include "matrix_types.hpp"
#include "rect.hpp"
#include "math.hpp"

namespace amp {
namespace matrix_utils {

using ::uint8_t;
using ::uint16_t;
using ::uint32_t;
using ::int32_t;
using ::size_t;
using math::max;
using math::min;

// Draw another matrix over destination with clipping. Source alpha is respected and additionally scaled by 'alpha'.
inline void drawMatrix(csMatrixBase& dst, tMatrixPixelsCoord dst_x, tMatrixPixelsCoord dst_y, 
                       const csMatrixBase& src, uint8_t alpha = 255) noexcept {
    const tMatrixPixelsCoord start_x = max(to_coord(0), -dst_x);
    const tMatrixPixelsCoord start_y = max(to_coord(0), -dst_y);
    const tMatrixPixelsCoord end_x = min(to_coord(src.width()),
                                         to_coord(dst.width()) - dst_x);
    const tMatrixPixelsCoord end_y = min(to_coord(src.height()),
                                         to_coord(dst.height()) - dst_y);

    for (tMatrixPixelsCoord sy = start_y; sy < end_y; ++sy) {
        const tMatrixPixelsCoord dy = sy + dst_y;
        for (tMatrixPixelsCoord sx = start_x; sx < end_x; ++sx) {
            const tMatrixPixelsCoord dx = sx + dst_x;
            const csColorRGBA pixel = src.getPixel(sx, sy);
            dst.setPixel(dx, dy, pixel.alpha(alpha));
        }
    }
}

// Draw specific source area to destination coordinates with clipping.
// 'srcRect' is the area to copy from source matrix, (dst_x, dst_y) is where to draw in destination matrix.
// Source alpha is respected and additionally scaled by 'alpha'.
inline void drawMatrixArea(csMatrixBase& dst, csRect srcRect, tMatrixPixelsCoord dst_x, tMatrixPixelsCoord dst_y,
                          const csMatrixBase& src, uint8_t alpha = 255) noexcept {
    // Clip source rectangle to source matrix bounds
    const csRect srcBounds = src.getRect();
    const csRect srcClipped = srcRect.intersect(srcBounds);
    if (srcClipped.empty()) {
        return;
    }
    
    // Copy pixels from source area to destination coordinates
    for (tMatrixPixelsSize y = 0; y < srcClipped.height; ++y) {
        for (tMatrixPixelsSize x = 0; x < srcClipped.width; ++x) {
            const csColorRGBA pixel = src.getPixel(srcClipped.x + static_cast<tMatrixPixelsCoord>(x), 
                                                   srcClipped.y + static_cast<tMatrixPixelsCoord>(y));
            dst.setPixel(dst_x + static_cast<tMatrixPixelsCoord>(x),
                        dst_y + static_cast<tMatrixPixelsCoord>(y),
                        pixel.alpha(alpha));
        }
    }
}

// Draw specific source area to destination coordinates with clipping, overwriting destination pixels.
// Unlike drawMatrixArea(), this does NOT blend; it writes pixels directly.
// Useful for internal buffers where pixels are treated as data (e.g. heat fields), not composited imagery.
inline void drawMatrixAreaRewrite(csMatrixBase& dst, csRect srcRect, tMatrixPixelsCoord dst_x, tMatrixPixelsCoord dst_y,
                                  const csMatrixBase& src) noexcept {
    // Clip source rectangle to source matrix bounds
    const csRect srcBounds = src.getRect();
    const csRect srcClipped = srcRect.intersect(srcBounds);
    if (srcClipped.empty()) {
        return;
    }

    // Copy pixels from source area to destination coordinates (overwrite mode)
    for (tMatrixPixelsSize y = 0; y < srcClipped.height; ++y) {
        for (tMatrixPixelsSize x = 0; x < srcClipped.width; ++x) {
            const csColorRGBA pixel = src.getPixel(srcClipped.x + static_cast<tMatrixPixelsCoord>(x),
                                                   srcClipped.y + static_cast<tMatrixPixelsCoord>(y));
            dst.setPixelRewrite(dst_x + static_cast<tMatrixPixelsCoord>(x),
                               dst_y + static_cast<tMatrixPixelsCoord>(y),
                               pixel);
        }
    }
}

// Fill rectangular area with color. Area is clipped to matrix bounds.
inline void fillArea(csMatrixBase& dst, csRect area, csColorRGBA color) noexcept {
    const csRect target = area.intersect(dst.getRect());
    if (target.empty()) {
        return;
    }
    const tMatrixPixelsCoord endX = target.x + to_coord(target.width);
    const tMatrixPixelsCoord endY = target.y + to_coord(target.height);
    for (tMatrixPixelsCoord y = target.y; y < endY; ++y) {
        for (tMatrixPixelsCoord x = target.x; x < endX; ++x) {
            dst.setPixel(x, y, color);
        }
    }
}

// Calculate average color of area. Fast.
// Uses two-level hierarchical averaging to avoid overflow in uint16_t accumulators.
// Max area: 65536 pixels (256x256). Larger areas return transparent black.
[[nodiscard]] inline csColorRGBA getAreaColor(const csMatrixBase& m, csRect area) noexcept {
    const csRect bounded = area.intersect(m.getRect());
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

    // Iterate through the bounded area using getPixel calls
    for (tMatrixPixelsSize iy = 0; iy < h; ++iy) {
        for (tMatrixPixelsSize ix = 0; ix < w; ++ix) {
            const csColorRGBA pixel = m.getPixel(bounded.x + static_cast<tMatrixPixelsCoord>(ix),
                                                 bounded.y + static_cast<tMatrixPixelsCoord>(iy));
            sum1 += pixel.sum(kZero);
            ++count1;

            if (count1 == kChunk) {
                sum2 += sum1.div(kChunk);
                ++count2;
                sum1 = csColorRGBA16{};
                count1 = 0;
            }
        }
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

// Scale and draw source matrix onto destination matrix using bilinear interpolation.
// 'srcRect' is the source rectangle in 'src' matrix, 'dstRect' is the destination rectangle in 'dst' matrix.
// Returns false if srcRect or dstRect is empty, or if srcRect is out of bounds of source matrix.
[[nodiscard]] inline bool drawMatrixScale(csMatrixBase& dst, csRect srcRect, csRect dstRect, const csMatrixBase& src) noexcept {
    // Validate input rectangles
    if (srcRect.empty() || dstRect.empty()) {
        return false;
    }

    // Check if source rectangle is within source matrix bounds
    const csRect sourceBounds = src.getRect();
    const csRect srcBounded = srcRect.intersect(sourceBounds);
    if (srcBounded.empty() || srcBounded.width != srcRect.width || srcBounded.height != srcRect.height) {
        return false;
    }

    // Clip destination rectangle to destination matrix bounds
    const csRect dstBounded = dstRect.intersect(dst.getRect());
    if (dstBounded.empty()) {
        return true; // Nothing to draw, but operation is valid
    }

    // Calculate scale factors based on original dst size (using fixed-point arithmetic for precision)
    // We use 16.16 fixed-point: multiply by 65536, then shift right by 16
    const int32_t scale_x_fp = (static_cast<int32_t>(srcRect.width) << 16) / static_cast<int32_t>(dstRect.width);
    const int32_t scale_y_fp = (static_cast<int32_t>(srcRect.height) << 16) / static_cast<int32_t>(dstRect.height);

    // Iterate over each pixel in clipped destination rectangle
    for (tMatrixPixelsSize dy = 0; dy < dstBounded.height; ++dy) {
        const tMatrixPixelsCoord dst_y = dstBounded.y + static_cast<tMatrixPixelsCoord>(dy);
        
        for (tMatrixPixelsSize dx = 0; dx < dstBounded.width; ++dx) {
            const tMatrixPixelsCoord dst_x = dstBounded.x + static_cast<tMatrixPixelsCoord>(dx);

            // Calculate corresponding position in source (fixed-point)
            // Map from dst coordinates to src coordinates, accounting for offset from dst origin
            const int32_t offset_x = dst_x - dstRect.x;
            const int32_t offset_y = dst_y - dstRect.y;
            const int32_t src_x_fp = (static_cast<int32_t>(srcRect.x) << 16) + 
                                     (offset_x * scale_x_fp);
            const int32_t src_y_fp = (static_cast<int32_t>(srcRect.y) << 16) + 
                                     (offset_y * scale_y_fp);

            // Extract integer and fractional parts
            const int32_t src_x_int = src_x_fp >> 16;
            const int32_t src_y_int = src_y_fp >> 16;
            const uint8_t fx = static_cast<uint8_t>((src_x_fp & 0xFFFF) >> 8); // 8-bit fractional part
            const uint8_t fy = static_cast<uint8_t>((src_y_fp & 0xFFFF) >> 8);

            // Get 4 neighboring pixels for bilinear interpolation
            const csColorRGBA p00 = src.getPixel(src_x_int, src_y_int);
            const csColorRGBA p10 = src.getPixel(src_x_int + 1, src_y_int);
            const csColorRGBA p01 = src.getPixel(src_x_int, src_y_int + 1);
            const csColorRGBA p11 = src.getPixel(src_x_int + 1, src_y_int + 1);

            // Bilinear interpolation: first interpolate horizontally (top and bottom)
            const csColorRGBA top = lerp(p00, p10, fx);
            const csColorRGBA bottom = lerp(p01, p11, fx);

            // Then interpolate vertically
            const csColorRGBA result = lerp(top, bottom, fy);

            // Draw the interpolated pixel with alpha blending
            dst.setPixel(dst_x, dst_y, result);
        }
    }

    return true;
}

// Compute blended color of matrix pixel over bgColor; does not modify matrix.
// Out-of-bounds returns bgColor unchanged.
[[nodiscard]] inline csColorRGBA getPixelBlend(const csMatrixBase& m, tMatrixPixelsCoord x, tMatrixPixelsCoord y, csColorRGBA bgColor) noexcept {
    const csColorRGBA pixel = m.getPixel(x, y);
    return csColorRGBA::sourceOverStraight(bgColor, pixel);
}

} // namespace matrix_utils
} // namespace amp
