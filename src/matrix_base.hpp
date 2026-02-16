#pragma once

#include "color_rgba.hpp"
#include "matrix_types.hpp"
#include "rect.hpp"

namespace amp {

// Base abstract matrix interface in terms of csColorRGBA.
// All matrix implementations provide safe out-of-bounds behavior:
// - Writes outside bounds are ignored.
// - Reads outside bounds return transparent black (0,0,0,0).
class csMatrixBase {
public:
    virtual ~csMatrixBase() = default;

    [[nodiscard]] virtual tMatrixPixelsSize width() const noexcept = 0;
    [[nodiscard]] virtual tMatrixPixelsSize height() const noexcept = 0;

    // Default rectangle helper: (0, 0, width, height).
    [[nodiscard]] inline csRect getRect() const noexcept {
        return csRect{0, 0, width(), height()};
    }

    // Read pixel as RGBA.
    [[nodiscard]] virtual csColorRGBA getPixel(tMatrixPixelsCoord x, tMatrixPixelsCoord y) const noexcept = 0;

    // Write pixel without blending.
    virtual void setPixelRewrite(tMatrixPixelsCoord x, tMatrixPixelsCoord y, csColorRGBA color) noexcept = 0;

    // Write pixel with blending (semantics depend on implementation).
    virtual void setPixel(tMatrixPixelsCoord x, tMatrixPixelsCoord y, csColorRGBA color) noexcept = 0;
};

} // namespace amp
