#pragma once

#include "matrix_pixels.hpp"
#include "math.hpp"

namespace amp {

using math::max;
using math::min;

// Axis-aligned rectangle with integer coordinates and size.
class csRect {
public:
    tMatrixPixelsCoord x{0};
    tMatrixPixelsCoord y{0};
    tMatrixPixelsSize width{0};
    tMatrixPixelsSize height{0};

    constexpr csRect() = default;

    constexpr csRect(tMatrixPixelsCoord x_, tMatrixPixelsCoord y_, tMatrixPixelsSize w_, tMatrixPixelsSize h_)
        : x{x_}, y{y_}, width{w_}, height{h_} {}

    [[nodiscard]] constexpr bool empty() const noexcept {
        return width == 0 || height == 0;
    }

    // Return intersection of two rectangles; empty rectangle when they do not overlap.
    [[nodiscard]] inline csRect intersect(const csRect& other) const noexcept {
        const auto nx = max(x, other.x);
        const auto ny = max(y, other.y);
        const auto rx = min(x + to_coord(width),
                            other.x + to_coord(other.width));
        const auto ry = min(y + to_coord(height),
                            other.y + to_coord(other.height));

        const auto w = rx - nx;
        const auto h = ry - ny;

        if (w <= 0 || h <= 0) {
            return csRect{};
        }

        return csRect{nx, ny, to_size(w), to_size(h)};
    }
};


} // namespace amp

