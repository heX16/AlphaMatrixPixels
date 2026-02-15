#pragma once

#include <stdint.h>

#ifdef max
#  undef max
#endif
#ifdef min
#  undef min
#endif

// Lightweight math helpers for Arduino-friendly builds (no <algorithm> dependency).
namespace amp {
namespace math {

inline constexpr uint8_t max(uint8_t a, uint8_t b) noexcept {
    return (a > b) ? a : b;
}

inline constexpr uint8_t min(uint8_t a, uint8_t b) noexcept {
    return (a < b) ? a : b;
}

inline constexpr int32_t max(int32_t a, int32_t b) noexcept {
    return (a > b) ? a : b;
}

inline constexpr int32_t min(int32_t a, int32_t b) noexcept {
    return (a < b) ? a : b;
}

inline constexpr uint32_t max(uint32_t a, uint32_t b) noexcept {
    return (a > b) ? a : b;
}

inline constexpr uint32_t min(uint32_t a, uint32_t b) noexcept {
    return (a < b) ? a : b;
}

inline constexpr int16_t max(int16_t a, int16_t b) noexcept {
    return (a > b) ? a : b;
}

inline constexpr int16_t min(int16_t a, int16_t b) noexcept {
    return (a < b) ? a : b;
}

inline constexpr int8_t max(int8_t a, int8_t b) noexcept {
    return (a > b) ? a : b;
}

inline constexpr int8_t min(int8_t a, int8_t b) noexcept {
    return (a < b) ? a : b;
}

inline constexpr uint16_t max(uint16_t a, uint16_t b) noexcept {
    return (a > b) ? a : b;
}

inline constexpr uint16_t min(uint16_t a, uint16_t b) noexcept {
    return (a < b) ? a : b;
}

inline constexpr int64_t max(int64_t a, int64_t b) noexcept {
    return (a > b) ? a : b;
}

inline constexpr int64_t min(int64_t a, int64_t b) noexcept {
    return (a < b) ? a : b;
}

inline constexpr uint64_t max(uint64_t a, uint64_t b) noexcept {
    return (a > b) ? a : b;
}

inline constexpr uint64_t min(uint64_t a, uint64_t b) noexcept {
    return (a < b) ? a : b;
}

inline constexpr float max(float a, float b) noexcept {
    return (a > b) ? a : b;
}

inline constexpr float min(float a, float b) noexcept {
    return (a < b) ? a : b;
}

} // namespace math
} // namespace amp


