#pragma once

#include <stdint.h>
#include <math.h>

// Fixed-point helpers are kept in a separate header to reduce compile time and keep math.hpp focused.
namespace amp {
namespace math {

// Fixed-point helpers:
// csFP16: signed 8.8 (int16_t)  -> scale = 256
// csFP32: signed 16.16 (int32_t) -> scale = 65536

static constexpr int FP16_FRAC_BITS = 8;
static constexpr int FP32_FRAC_BITS = 16;
static constexpr int32_t FP16_SCALE = 1 << FP16_FRAC_BITS;   // 256
static constexpr int32_t FP32_SCALE = 1 << FP32_FRAC_BITS;   // 65536
static constexpr int32_t FP16_MIN_RAW = -32768;
static constexpr int32_t FP16_MAX_RAW = 32767;
static constexpr int32_t FP32_MIN_RAW = -2147483648; // INT32_MIN
static constexpr int32_t FP32_MAX_RAW = 2147483647;  // INT32_MAX

struct csFP16 {
    using raw_t = int16_t;
    union {
        raw_t raw{0};
        struct {
            int8_t int_part;
            uint8_t frac_part;
        };
    };

    static constexpr int frac_bits = FP16_FRAC_BITS;
    static constexpr raw_t scale = static_cast<raw_t>(FP16_SCALE);
    static constexpr raw_t min_raw = static_cast<raw_t>(FP16_MIN_RAW);
    static constexpr raw_t max_raw = static_cast<raw_t>(FP16_MAX_RAW);

private:
    // Clamp helpers for intermediate math.
    static constexpr raw_t clamp_raw(int32_t v) noexcept {
        return v > FP16_MAX_RAW ? static_cast<raw_t>(FP16_MAX_RAW)
             : v < FP16_MIN_RAW ? static_cast<raw_t>(FP16_MIN_RAW)
             : static_cast<raw_t>(v);
    }

    static constexpr raw_t mul_raw(raw_t a, raw_t b) noexcept {
        return static_cast<raw_t>((static_cast<int32_t>(a) * static_cast<int32_t>(b)) >> FP16_FRAC_BITS);
    }

    static constexpr raw_t div_raw(raw_t num, raw_t den) noexcept {
        return den == 0 ? static_cast<raw_t>(0)
                        : static_cast<raw_t>((static_cast<int32_t>(num) << FP16_FRAC_BITS) / den);
    }

    static constexpr int16_t saturate_raw(int32_t v) noexcept {
        return v > FP16_MAX_RAW ? static_cast<int16_t>(FP16_MAX_RAW)
             : v < FP16_MIN_RAW ? static_cast<int16_t>(FP16_MIN_RAW)
             : static_cast<int16_t>(v);
    }

    static inline raw_t float_to_raw_constexpr(float v) noexcept {
        const float scaled = v * static_cast<float>(FP16_SCALE);
        const float adj = (scaled >= 0.0f) ? 0.5f : -0.5f; // round to nearest, ties up
        const int32_t raw = static_cast<int32_t>(scaled + adj);
        return saturate_raw(raw);
    }

    static float raw_to_float(raw_t v) noexcept {
        return static_cast<float>(v) / static_cast<float>(FP16_SCALE);
    }

    static inline int32_t round_raw_to_int(raw_t v) noexcept {
        const int32_t bias = (v >= 0) ? (FP16_SCALE / 2) : -(FP16_SCALE / 2);
        return static_cast<int32_t>(v + bias) >> FP16_FRAC_BITS;
    }

public:
    csFP16() = default;
    explicit csFP16(int32_t v) : raw(clamp_raw(v << frac_bits)) {}
    explicit csFP16(float v) : raw(float_to_raw_constexpr(v)) {}

    static inline csFP16 from_raw(raw_t r) noexcept {
        csFP16 out{};
        out.raw = r;
        return out;
    }
    static inline csFP16 from_int(int32_t v) noexcept { return csFP16{v}; }
    static csFP16 from_float(float v) noexcept { return csFP16{v}; }
    static inline csFP16 float_const(float v) noexcept { return from_raw(float_to_raw_constexpr(v)); }
    static inline csFP16 from_ratio(int32_t numer, int32_t denom) noexcept {
        // round-to-nearest, ties up
        const int64_t scaled = static_cast<int64_t>(FP16_SCALE) * static_cast<int64_t>(numer);
        const int64_t bias = (scaled >= 0) ? (static_cast<int64_t>(denom) / 2) : -(static_cast<int64_t>(denom) / 2);
        return from_raw(clamp_raw(static_cast<int32_t>((scaled + bias) / denom)));
    }

    [[nodiscard]] constexpr raw_t raw_value() const noexcept { return raw; }
    [[nodiscard]] float to_float() const noexcept { return raw_to_float(raw); }
    [[nodiscard]] constexpr int32_t int_trunc() const noexcept { return static_cast<int32_t>(raw) >> frac_bits; }
    [[nodiscard]] constexpr uint8_t frac_raw() const noexcept { return static_cast<uint8_t>(raw & (scale - 1)); }

    [[nodiscard]] inline int32_t round_int() const noexcept { return round_raw_to_int(raw); }

    [[nodiscard]] inline int32_t floor_int() const noexcept {
        if (raw >= 0) {
            return static_cast<int32_t>(raw) >> frac_bits;
        }
        const int32_t mask = static_cast<int32_t>(scale - 1);
        const int32_t r = static_cast<int32_t>(raw);
        return (r & mask) ? ((r >> frac_bits) - 1) : (r >> frac_bits);
    }

    [[nodiscard]] inline int32_t ceil_int() const noexcept {
        if (raw >= 0) {
            const int32_t mask = static_cast<int32_t>(scale - 1);
            const int32_t r = static_cast<int32_t>(raw);
            return (r & mask) ? ((r >> frac_bits) + 1) : (r >> frac_bits);
        }
        return static_cast<int32_t>(raw) >> frac_bits;
    }

    [[nodiscard]] inline csFP16 absVal() const noexcept {
        return (raw >= 0) ? *this : from_raw(static_cast<raw_t>(raw == FP16_MIN_RAW ? FP16_MAX_RAW : -raw));
    }

    // Arithmetic
    [[nodiscard]] inline csFP16 operator+(csFP16 rhs) const noexcept {
        return from_raw(clamp_raw(static_cast<int32_t>(raw) + static_cast<int32_t>(rhs.raw)));
    }
    [[nodiscard]] inline csFP16 operator-(csFP16 rhs) const noexcept {
        return from_raw(clamp_raw(static_cast<int32_t>(raw) - static_cast<int32_t>(rhs.raw)));
    }
    [[nodiscard]] inline csFP16 operator*(csFP16 rhs) const noexcept {
        return from_raw(clamp_raw(mul_raw(raw, rhs.raw)));
    }
    [[nodiscard]] inline csFP16 operator/(csFP16 rhs) const noexcept {
        if (rhs.raw == 0) {
            return from_raw(static_cast<raw_t>(0));
        }
        return from_raw(clamp_raw(div_raw(raw, rhs.raw)));
    }

    csFP16& operator+=(csFP16 rhs) noexcept { return *this = *this + rhs; }
    csFP16& operator-=(csFP16 rhs) noexcept { return *this = *this - rhs; }
    csFP16& operator*=(csFP16 rhs) noexcept { return *this = *this * rhs; }
    csFP16& operator/=(csFP16 rhs) noexcept { return *this = *this / rhs; }

    // Comparisons
    [[nodiscard]] constexpr bool operator==(csFP16 rhs) const noexcept { return raw == rhs.raw; }
    [[nodiscard]] constexpr bool operator!=(csFP16 rhs) const noexcept { return raw != rhs.raw; }
    [[nodiscard]] constexpr bool operator<(csFP16 rhs) const noexcept { return raw < rhs.raw; }
    [[nodiscard]] constexpr bool operator<=(csFP16 rhs) const noexcept { return raw <= rhs.raw; }
    [[nodiscard]] constexpr bool operator>(csFP16 rhs) const noexcept { return raw > rhs.raw; }
    [[nodiscard]] constexpr bool operator>=(csFP16 rhs) const noexcept { return raw >= rhs.raw; }

    explicit constexpr operator raw_t() const noexcept { return raw; }
    explicit operator float() const noexcept { return to_float(); }
};

struct csFP32 {
    using raw_t = int32_t;
    union {
        raw_t raw{0};
        struct {
            int16_t int_part;
            uint16_t frac_part;
        };
    };

    static constexpr int frac_bits = FP32_FRAC_BITS;
    static constexpr raw_t scale = static_cast<raw_t>(FP32_SCALE);
    static constexpr raw_t min_raw = static_cast<raw_t>(FP32_MIN_RAW);
    static constexpr raw_t max_raw = static_cast<raw_t>(FP32_MAX_RAW);

private:
    // Clamp helpers for intermediate math.
    static constexpr raw_t clamp_raw(int64_t v) noexcept {
        return v > FP32_MAX_RAW ? static_cast<raw_t>(FP32_MAX_RAW)
             : v < static_cast<int64_t>(FP32_MIN_RAW) ? static_cast<raw_t>(FP32_MIN_RAW)
             : static_cast<raw_t>(v);
    }

    static constexpr raw_t mul_raw(raw_t a, raw_t b) noexcept {
        return static_cast<raw_t>((static_cast<int64_t>(a) * static_cast<int64_t>(b)) >> FP32_FRAC_BITS);
    }

    static constexpr raw_t div_raw(raw_t num, raw_t den) noexcept {
        return den == 0 ? static_cast<raw_t>(0)
                        : static_cast<raw_t>((static_cast<int64_t>(num) << FP32_FRAC_BITS) / den);
    }

    static constexpr int32_t saturate_raw(int64_t v) noexcept {
        return v > FP32_MAX_RAW ? FP32_MAX_RAW
             : v < static_cast<int64_t>(FP32_MIN_RAW) ? FP32_MIN_RAW
             : static_cast<int32_t>(v);
    }

    static inline raw_t float_to_raw_constexpr(float v) noexcept {
        const float scaled = v * static_cast<float>(FP32_SCALE);
        const float adj = (scaled >= 0.0f) ? 0.5f : -0.5f; // round to nearest, ties up
        const int64_t raw = static_cast<int64_t>(scaled + adj);
        return saturate_raw(raw);
    }

    static float raw_to_float(raw_t v) noexcept {
        return static_cast<float>(v) / static_cast<float>(FP32_SCALE);
    }

    static inline int32_t round_raw_to_int(raw_t v) noexcept {
        const int32_t bias = (v >= 0) ? (FP32_SCALE / 2) : -(FP32_SCALE / 2);
        return (v + bias) >> FP32_FRAC_BITS;
    }

public:
    csFP32() = default;
    explicit csFP32(int32_t v) : raw(clamp_raw(static_cast<int64_t>(v) << frac_bits)) {}
    explicit csFP32(float v) : raw(float_to_raw_constexpr(v)) {}

    static inline csFP32 from_raw(raw_t r) noexcept {
        csFP32 out{};
        out.raw = r;
        return out;
    }
    static inline csFP32 from_int(int32_t v) noexcept { return csFP32(v); }
    static csFP32 from_float(float v) noexcept { return csFP32(v); }
    static inline csFP32 float_const(float v) noexcept { return from_raw(float_to_raw_constexpr(v)); }
    static inline csFP32 from_ratio(int32_t numer, int32_t denom) noexcept {
        // round-to-nearest, ties up
        const int64_t scaled = static_cast<int64_t>(FP32_SCALE) * static_cast<int64_t>(numer);
        const int64_t bias = (scaled >= 0) ? (static_cast<int64_t>(denom) / 2) : -(static_cast<int64_t>(denom) / 2);
        return from_raw(clamp_raw((scaled + bias) / denom));
    }

    [[nodiscard]] constexpr raw_t raw_value() const noexcept { return raw; }
    [[nodiscard]] float to_float() const noexcept { return raw_to_float(raw); }
    [[nodiscard]] constexpr int32_t int_trunc() const noexcept { return static_cast<int32_t>(raw) >> frac_bits; }
    [[nodiscard]] constexpr uint16_t frac_raw() const noexcept { return static_cast<uint16_t>(raw & (scale - 1)); }

    [[nodiscard]] inline int32_t round_int() const noexcept { return round_raw_to_int(raw); }

    [[nodiscard]] inline int32_t floor_int() const noexcept {
        if (raw >= 0) {
            return static_cast<int32_t>(raw) >> frac_bits;
        }
        const int32_t mask = static_cast<int32_t>(scale - 1);
        const int32_t r = static_cast<int32_t>(raw);
        return (r & mask) ? ((r >> frac_bits) - 1) : (r >> frac_bits);
    }

    [[nodiscard]] inline int32_t ceil_int() const noexcept {
        if (raw >= 0) {
            const int32_t mask = static_cast<int32_t>(scale - 1);
            const int32_t r = static_cast<int32_t>(raw);
            return (r & mask) ? ((r >> frac_bits) + 1) : (r >> frac_bits);
        }
        return static_cast<int32_t>(raw) >> frac_bits;
    }

    [[nodiscard]] inline csFP32 absVal() const noexcept {
        return (raw >= 0) ? *this : from_raw(raw == FP32_MIN_RAW ? FP32_MAX_RAW : -raw);
    }

    // Arithmetic
    [[nodiscard]] inline csFP32 operator+(csFP32 rhs) const noexcept {
        return from_raw(clamp_raw(static_cast<int64_t>(raw) + static_cast<int64_t>(rhs.raw)));
    }
    [[nodiscard]] inline csFP32 operator-(csFP32 rhs) const noexcept {
        return from_raw(clamp_raw(static_cast<int64_t>(raw) - static_cast<int64_t>(rhs.raw)));
    }
    [[nodiscard]] inline csFP32 operator*(csFP32 rhs) const noexcept {
        return from_raw(clamp_raw(mul_raw(raw, rhs.raw)));
    }
    [[nodiscard]] inline csFP32 operator/(csFP32 rhs) const noexcept {
        return rhs.raw == 0 ? from_raw(0) : from_raw(clamp_raw(div_raw(raw, rhs.raw)));
    }

    csFP32& operator+=(csFP32 rhs) noexcept { return *this = *this + rhs; }
    csFP32& operator-=(csFP32 rhs) noexcept { return *this = *this - rhs; }
    csFP32& operator*=(csFP32 rhs) noexcept { return *this = *this * rhs; }
    csFP32& operator/=(csFP32 rhs) noexcept { return *this = *this / rhs; }

    // Comparisons
    [[nodiscard]] constexpr bool operator==(csFP32 rhs) const noexcept { return raw == rhs.raw; }
    [[nodiscard]] constexpr bool operator!=(csFP32 rhs) const noexcept { return raw != rhs.raw; }
    [[nodiscard]] constexpr bool operator<(csFP32 rhs) const noexcept { return raw < rhs.raw; }
    [[nodiscard]] constexpr bool operator<=(csFP32 rhs) const noexcept { return raw <= rhs.raw; }
    [[nodiscard]] constexpr bool operator>(csFP32 rhs) const noexcept { return raw > rhs.raw; }
    [[nodiscard]] constexpr bool operator>=(csFP32 rhs) const noexcept { return raw >= rhs.raw; }

    explicit constexpr operator raw_t() const noexcept { return raw; }
    explicit operator float() const noexcept { return to_float(); }
};

// Trigonometry on fixed-point types (argument in radians, result fixed-point).
inline csFP16 fp16_sin(csFP16 angle) noexcept {
    return csFP16(static_cast<float>(sin(angle.to_float())));
}

inline csFP16 fp16_cos(csFP16 angle) noexcept {
    return csFP16(static_cast<float>(cos(angle.to_float())));
}

inline csFP32 fp32_sin(csFP32 angle) noexcept {
    return csFP32(static_cast<float>(sin(angle.to_float())));
}

inline csFP32 fp32_cos(csFP32 angle) noexcept {
    return csFP32(static_cast<float>(cos(angle.to_float())));
}

} // namespace math
} // namespace amp


