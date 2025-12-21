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

// Clamp helpers for intermediate math.
inline constexpr fp16 clamp_fp16(int32_t v) noexcept {
    return v > FP16_MAX_RAW ? static_cast<fp16>(FP16_MAX_RAW)
         : v < FP16_MIN_RAW ? static_cast<fp16>(FP16_MIN_RAW)
         : static_cast<fp16>(v);
}

inline constexpr fp32 clamp_fp32(int64_t v) noexcept {
    return v > FP32_MAX_RAW ? static_cast<fp32>(FP32_MAX_RAW)
         : v < static_cast<int64_t>(FP32_MIN_RAW) ? static_cast<fp32>(FP32_MIN_RAW)
         : static_cast<fp32>(v);
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

struct fp16_t {
    union {
        fp16 raw{0};
        struct {
            int8_t int_part;
            uint8_t frac_part;
        };
    };

    static constexpr int frac_bits = FP16_FRAC_BITS;
    static constexpr fp16 scale = static_cast<fp16>(FP16_SCALE);
    static constexpr fp16 min_raw = static_cast<fp16>(FP16_MIN_RAW);
    static constexpr fp16 max_raw = static_cast<fp16>(FP16_MAX_RAW);

    constexpr fp16_t() = default;
    explicit constexpr fp16_t(fp16 r) : raw(r) {}
    explicit constexpr fp16_t(int32_t v) : raw(clamp_fp16(v << frac_bits)) {}
    explicit fp16_t(float v) : raw(float_to_fp16(v)) {}

    static constexpr fp16_t from_raw(fp16 r) noexcept { return fp16_t{r}; }
    static constexpr fp16_t from_int(int32_t v) noexcept { return fp16_t{clamp_fp16(v << frac_bits)}; }
    static fp16_t from_float(float v) noexcept { return fp16_t{float_to_fp16(v)}; }

    [[nodiscard]] constexpr fp16 raw_value() const noexcept { return raw; }
    [[nodiscard]] float to_float() const noexcept { return fp16_to_float(raw); }
    [[nodiscard]] constexpr int32_t int_trunc() const noexcept { return static_cast<int32_t>(raw) >> frac_bits; }
    [[nodiscard]] constexpr uint8_t frac_raw() const noexcept { return static_cast<uint8_t>(raw & (scale - 1)); }

    [[nodiscard]] constexpr int32_t round_int() const noexcept { return fp16_round(raw); }

    [[nodiscard]] constexpr int32_t floor_int() const noexcept {
        if (raw >= 0) {
            return static_cast<int32_t>(raw) >> frac_bits;
        }
        const int32_t mask = static_cast<int32_t>(scale - 1);
        const int32_t r = static_cast<int32_t>(raw);
        return (r & mask) ? ((r >> frac_bits) - 1) : (r >> frac_bits);
    }

    [[nodiscard]] constexpr int32_t ceil_int() const noexcept {
        if (raw >= 0) {
            const int32_t mask = static_cast<int32_t>(scale - 1);
            const int32_t r = static_cast<int32_t>(raw);
            return (r & mask) ? ((r >> frac_bits) + 1) : (r >> frac_bits);
        }
        return static_cast<int32_t>(raw) >> frac_bits;
    }

    [[nodiscard]] constexpr fp16_t abs() const noexcept {
        return (raw >= 0) ? *this : fp16_t{static_cast<fp16>(raw == FP16_MIN_RAW ? FP16_MAX_RAW : -raw)};
    }

    // Arithmetic
    [[nodiscard]] constexpr fp16_t operator+(fp16_t rhs) const noexcept {
        return fp16_t{clamp_fp16(static_cast<int32_t>(raw) + static_cast<int32_t>(rhs.raw))};
    }
    [[nodiscard]] constexpr fp16_t operator-(fp16_t rhs) const noexcept {
        return fp16_t{clamp_fp16(static_cast<int32_t>(raw) - static_cast<int32_t>(rhs.raw))};
    }
    [[nodiscard]] constexpr fp16_t operator*(fp16_t rhs) const noexcept {
        return fp16_t{clamp_fp16(fp16_mul(raw, rhs.raw))};
    }
    [[nodiscard]] constexpr fp16_t operator/(fp16_t rhs) const noexcept {
        return fp16_t{rhs.raw == 0 ? 0 : clamp_fp16(fp16_div(raw, rhs.raw))};
    }

    fp16_t& operator+=(fp16_t rhs) noexcept { return *this = *this + rhs; }
    fp16_t& operator-=(fp16_t rhs) noexcept { return *this = *this - rhs; }
    fp16_t& operator*=(fp16_t rhs) noexcept { return *this = *this * rhs; }
    fp16_t& operator/=(fp16_t rhs) noexcept { return *this = *this / rhs; }

    // Comparisons
    [[nodiscard]] constexpr bool operator==(fp16_t rhs) const noexcept { return raw == rhs.raw; }
    [[nodiscard]] constexpr bool operator!=(fp16_t rhs) const noexcept { return raw != rhs.raw; }
    [[nodiscard]] constexpr bool operator<(fp16_t rhs) const noexcept { return raw < rhs.raw; }
    [[nodiscard]] constexpr bool operator<=(fp16_t rhs) const noexcept { return raw <= rhs.raw; }
    [[nodiscard]] constexpr bool operator>(fp16_t rhs) const noexcept { return raw > rhs.raw; }
    [[nodiscard]] constexpr bool operator>=(fp16_t rhs) const noexcept { return raw >= rhs.raw; }

    explicit constexpr operator fp16() const noexcept { return raw; }
    explicit operator float() const noexcept { return to_float(); }
};

struct fp32_t {
    union {
        fp32 raw{0};
        struct {
            int16_t int_part;
            uint16_t frac_part;
        };
    };

    static constexpr int frac_bits = FP32_FRAC_BITS;
    static constexpr fp32 scale = static_cast<fp32>(FP32_SCALE);
    static constexpr fp32 min_raw = static_cast<fp32>(FP32_MIN_RAW);
    static constexpr fp32 max_raw = static_cast<fp32>(FP32_MAX_RAW);

    constexpr fp32_t() = default;
    explicit constexpr fp32_t(int32_t v) : raw(clamp_fp32(static_cast<int64_t>(v) << frac_bits)) {}
    explicit fp32_t(float v) : raw(float_to_fp32(v)) {}

    static constexpr fp32_t from_raw(fp32 r) noexcept { return fp32_t{r}; }
    static constexpr fp32_t from_int(int32_t v) noexcept { return fp32_t{clamp_fp32(static_cast<int64_t>(v) << frac_bits)}; }
    static fp32_t from_float(float v) noexcept { return fp32_t{float_to_fp32(v)}; }

    [[nodiscard]] constexpr fp32 raw_value() const noexcept { return raw; }
    [[nodiscard]] float to_float() const noexcept { return fp32_to_float(raw); }
    [[nodiscard]] constexpr int32_t int_trunc() const noexcept { return static_cast<int32_t>(raw) >> frac_bits; }
    [[nodiscard]] constexpr uint16_t frac_raw() const noexcept { return static_cast<uint16_t>(raw & (scale - 1)); }

    [[nodiscard]] constexpr int32_t round_int() const noexcept { return fp32_round(raw); }

    [[nodiscard]] constexpr int32_t floor_int() const noexcept {
        if (raw >= 0) {
            return static_cast<int32_t>(raw) >> frac_bits;
        }
        const int32_t mask = static_cast<int32_t>(scale - 1);
        const int32_t r = static_cast<int32_t>(raw);
        return (r & mask) ? ((r >> frac_bits) - 1) : (r >> frac_bits);
    }

    [[nodiscard]] constexpr int32_t ceil_int() const noexcept {
        if (raw >= 0) {
            const int32_t mask = static_cast<int32_t>(scale - 1);
            const int32_t r = static_cast<int32_t>(raw);
            return (r & mask) ? ((r >> frac_bits) + 1) : (r >> frac_bits);
        }
        return static_cast<int32_t>(raw) >> frac_bits;
    }

    [[nodiscard]] constexpr fp32_t abs() const noexcept {
        return (raw >= 0) ? *this : fp32_t{raw == FP32_MIN_RAW ? FP32_MAX_RAW : -raw};
    }

    // Arithmetic
    [[nodiscard]] constexpr fp32_t operator+(fp32_t rhs) const noexcept {
        return fp32_t{clamp_fp32(static_cast<int64_t>(raw) + static_cast<int64_t>(rhs.raw))};
    }
    [[nodiscard]] constexpr fp32_t operator-(fp32_t rhs) const noexcept {
        return fp32_t{clamp_fp32(static_cast<int64_t>(raw) - static_cast<int64_t>(rhs.raw))};
    }
    [[nodiscard]] constexpr fp32_t operator*(fp32_t rhs) const noexcept {
        return fp32_t{clamp_fp32(fp32_mul(raw, rhs.raw))};
    }
    [[nodiscard]] constexpr fp32_t operator/(fp32_t rhs) const noexcept {
        return fp32_t{rhs.raw == 0 ? 0 : clamp_fp32(fp32_div(raw, rhs.raw))};
    }

    fp32_t& operator+=(fp32_t rhs) noexcept { return *this = *this + rhs; }
    fp32_t& operator-=(fp32_t rhs) noexcept { return *this = *this - rhs; }
    fp32_t& operator*=(fp32_t rhs) noexcept { return *this = *this * rhs; }
    fp32_t& operator/=(fp32_t rhs) noexcept { return *this = *this / rhs; }

    // Comparisons
    [[nodiscard]] constexpr bool operator==(fp32_t rhs) const noexcept { return raw == rhs.raw; }
    [[nodiscard]] constexpr bool operator!=(fp32_t rhs) const noexcept { return raw != rhs.raw; }
    [[nodiscard]] constexpr bool operator<(fp32_t rhs) const noexcept { return raw < rhs.raw; }
    [[nodiscard]] constexpr bool operator<=(fp32_t rhs) const noexcept { return raw <= rhs.raw; }
    [[nodiscard]] constexpr bool operator>(fp32_t rhs) const noexcept { return raw > rhs.raw; }
    [[nodiscard]] constexpr bool operator>=(fp32_t rhs) const noexcept { return raw >= rhs.raw; }

    explicit constexpr operator fp32() const noexcept { return raw; }
    explicit operator float() const noexcept { return to_float(); }
};

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

inline fp16_t fp16_sin(fp16_t angle) noexcept {
    return fp16_t::from_raw(fp16_sin(angle.raw_value()));
}

inline fp16_t fp16_cos(fp16_t angle) noexcept {
    return fp16_t::from_raw(fp16_cos(angle.raw_value()));
}

inline fp32_t fp32_sin(fp32_t angle) noexcept {
    return fp32_t::from_raw(fp32_sin(angle.raw_value()));
}

inline fp32_t fp32_cos(fp32_t angle) noexcept {
    return fp32_t::from_raw(fp32_cos(angle.raw_value()));
}

} // namespace matrix_pixels_math


