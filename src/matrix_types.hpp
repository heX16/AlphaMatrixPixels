#pragma once

#include <stdint.h>

namespace amp {

using ::int32_t;
using ::uint16_t;

using tMatrixPixelsCoord = int32_t;
using tMatrixPixelsSize = uint16_t;

template <typename T>
constexpr tMatrixPixelsCoord to_coord(T v) noexcept {
    return static_cast<tMatrixPixelsCoord>(v);
}

template <typename T>
constexpr tMatrixPixelsSize to_size(T v) noexcept {
    return static_cast<tMatrixPixelsSize>(v);
}

} // namespace amp


