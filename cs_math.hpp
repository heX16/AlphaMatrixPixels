#pragma once

#include <cstdint>
#include <cmath>

#ifdef max
#  undef max
#endif
#ifdef min
#  undef min
#endif

// Lightweight math helpers for Arduino-friendly builds (no <algorithm> dependency).
namespace matrix_pixels_math {

inline constexpr int32_t max(int32_t a, int32_t b) noexcept {
    return (a > b) ? a : b;
}

inline constexpr int32_t min(int32_t a, int32_t b) noexcept {
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

inline constexpr float max(float a, float b) noexcept {
    return (a > b) ? a : b;
}

inline constexpr float min(float a, float b) noexcept {
    return (a < b) ? a : b;
}

// Fixed-point helpers:
// fp16: signed 8.8 (int16_t)  -> scale = 256
// fp32: signed 16.16 (int32_t) -> scale = 65536
using fp16 = int16_t;
using fp32 = int32_t;

static constexpr int FP16_FRAC_BITS = 8;
static constexpr int FP32_FRAC_BITS = 16;
static constexpr int32_t FP16_SCALE = 1 << FP16_FRAC_BITS;   // 256
static constexpr int32_t FP32_SCALE = 1 << FP32_FRAC_BITS;   // 65536
static constexpr int32_t FP16_MIN_RAW = -32768;
static constexpr int32_t FP16_MAX_RAW = 32767;
static constexpr int32_t FP32_MIN_RAW = -2147483648; // INT32_MIN
static constexpr int32_t FP32_MAX_RAW = 2147483647;  // INT32_MAX

// Convert int32 to fixed-point (exact shift, no saturation).
inline constexpr fp32 int32_to_fp32(int32_t v) noexcept {
    return static_cast<fp32>(v) << FP32_FRAC_BITS;
}

inline constexpr int16_t saturate_to_fp16(int32_t v) noexcept {
    return v > FP16_MAX_RAW ? static_cast<int16_t>(FP16_MAX_RAW)
         : v < FP16_MIN_RAW ? static_cast<int16_t>(FP16_MIN_RAW)
         : static_cast<int16_t>(v);
}

inline constexpr int32_t saturate_to_fp32(int64_t v) noexcept {
    return v > FP32_MAX_RAW ? FP32_MAX_RAW
         : v < static_cast<int64_t>(FP32_MIN_RAW) ? FP32_MIN_RAW
         : static_cast<int32_t>(v);
}

// fp16 <-> float
inline int16_t float_to_fp16(float v) noexcept {
    const float scaled = v * static_cast<float>(FP16_SCALE);
    const float adj = (scaled >= 0.0f) ? 0.5f : -0.5f; // round to nearest, ties up
    const int32_t raw = static_cast<int32_t>(scaled + adj);
    return saturate_to_fp16(raw);
}

inline float fp16_to_float(int16_t v) noexcept {
    return static_cast<float>(v) / static_cast<float>(FP16_SCALE);
}

// fp32 <-> float
inline int32_t float_to_fp32(float v) noexcept {
    const float scaled = v * static_cast<float>(FP32_SCALE);
    const float adj = (scaled >= 0.0f) ? 0.5f : -0.5f; // round to nearest, ties up
    const int64_t raw = static_cast<int64_t>(scaled + adj);
    return saturate_to_fp32(raw);
}

inline float fp32_to_float(int32_t v) noexcept {
    return static_cast<float>(v) / static_cast<float>(FP32_SCALE);
}

// fp16 <-> fp32
inline int32_t fp16_to_fp32(int16_t v) noexcept {
    return static_cast<int32_t>(v) << (FP32_FRAC_BITS - FP16_FRAC_BITS);
}

inline int16_t fp32_to_fp16(int32_t v) noexcept {
    constexpr int shift = FP32_FRAC_BITS - FP16_FRAC_BITS; // 8
    const int32_t bias = (v >= 0) ? (1 << (shift - 1)) : -(1 << (shift - 1)); // rounding bias
    const int32_t raw = (v + bias) >> shift;
    return saturate_to_fp16(raw);
}

// Round fixed-point to nearest integer (ties up)
inline constexpr int32_t fp16_round(int16_t v) noexcept {
    const int32_t bias = (v >= 0) ? (FP16_SCALE / 2) : -(FP16_SCALE / 2);
    return static_cast<int32_t>(v + bias) >> FP16_FRAC_BITS;
}

inline constexpr int32_t fp32_round(int32_t v) noexcept {
    const int32_t bias = (v >= 0) ? (FP32_SCALE / 2) : -(FP32_SCALE / 2);
    return (v + bias) >> FP32_FRAC_BITS;
}

// Multiply fp32 (16.16) with trunc toward zero.
inline constexpr fp32 fp32_mul(fp32 a, fp32 b) noexcept {
    return static_cast<fp32>((static_cast<int64_t>(a) * static_cast<int64_t>(b)) >> FP32_FRAC_BITS);
}

// Multiply fp16 (8.8) with trunc toward zero.
inline constexpr fp16 fp16_mul(fp16 a, fp16 b) noexcept {
    return static_cast<fp16>((static_cast<int32_t>(a) * static_cast<int32_t>(b)) >> FP16_FRAC_BITS);
}

// Divide fp32 (16.16) by fp32 -> fp32, trunc toward zero. Return 0 on div-by-zero.
inline constexpr fp32 fp32_div(fp32 num, fp32 den) noexcept {
    if (den == 0) {
        return 0;
    }
    return static_cast<fp32>((static_cast<int64_t>(num) << FP32_FRAC_BITS) / den);
}

// Divide fp16 (8.8) by fp16 -> fp16, trunc toward zero. Return 0 on div-by-zero.
inline constexpr fp16 fp16_div(fp16 num, fp16 den) noexcept {
    if (den == 0) {
        return 0;
    }
    return static_cast<fp16>((static_cast<int32_t>(num) << FP16_FRAC_BITS) / den);
}

// Trigonometry on fixed-point (argument in radians, fixed-point), result fixed-point.
inline fp16 fp16_sin(fp16 angle) noexcept {
    return float_to_fp16(std::sin(fp16_to_float(angle)));
}

inline fp16 fp16_cos(fp16 angle) noexcept {
    return float_to_fp16(std::cos(fp16_to_float(angle)));
}

inline fp32 fp32_sin(fp32 angle) noexcept {
    return float_to_fp32(std::sin(fp32_to_float(angle)));
}

inline fp32 fp32_cos(fp32 angle) noexcept {
    return float_to_fp32(std::cos(fp32_to_float(angle)));
}

} // namespace matrix_pixels_math


