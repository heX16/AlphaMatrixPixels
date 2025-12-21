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
// fp16_t: signed 8.8 (int16_t)  -> scale = 256
// fp32_t: signed 16.16 (int32_t) -> scale = 65536

static constexpr int FP16_FRAC_BITS = 8;
static constexpr int FP32_FRAC_BITS = 16;
static constexpr int32_t FP16_SCALE = 1 << FP16_FRAC_BITS;   // 256
static constexpr int32_t FP32_SCALE = 1 << FP32_FRAC_BITS;   // 65536
static constexpr int32_t FP16_MIN_RAW = -32768;
static constexpr int32_t FP16_MAX_RAW = 32767;
static constexpr int32_t FP32_MIN_RAW = -2147483648; // INT32_MIN
static constexpr int32_t FP32_MAX_RAW = 2147483647;  // INT32_MAX

struct fp16_t {
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

    static constexpr raw_t float_to_raw_constexpr(float v) noexcept {
        const float scaled = v * static_cast<float>(FP16_SCALE);
        const float adj = (scaled >= 0.0f) ? 0.5f : -0.5f; // round to nearest, ties up
        const int32_t raw = static_cast<int32_t>(scaled + adj);
        return saturate_raw(raw);
    }

    static float raw_to_float(raw_t v) noexcept {
        return static_cast<float>(v) / static_cast<float>(FP16_SCALE);
    }

    static constexpr int32_t round_raw_to_int(raw_t v) noexcept {
        const int32_t bias = (v >= 0) ? (FP16_SCALE / 2) : -(FP16_SCALE / 2);
        return static_cast<int32_t>(v + bias) >> FP16_FRAC_BITS;
    }

public:
    constexpr fp16_t() = default;
    explicit constexpr fp16_t(int32_t v) : raw(clamp_raw(v << frac_bits)) {}
    explicit fp16_t(float v) : raw(float_to_raw_constexpr(v)) {}

    static constexpr fp16_t from_raw(raw_t r) noexcept {
        fp16_t out{};
        out.raw = r;
        return out;
    }
    static constexpr fp16_t from_int(int32_t v) noexcept { return fp16_t{v}; }
    static fp16_t from_float(float v) noexcept { return fp16_t{v}; }
    static constexpr fp16_t float_const(float v) noexcept { return from_raw(float_to_raw_constexpr(v)); }
    static constexpr fp16_t from_ratio(int32_t numer, int32_t denom) noexcept {
        // round-to-nearest, ties up
        const int64_t scaled = static_cast<int64_t>(FP16_SCALE) * static_cast<int64_t>(numer);
        const int64_t bias = (scaled >= 0) ? (static_cast<int64_t>(denom) / 2) : -(static_cast<int64_t>(denom) / 2);
        return from_raw(clamp_raw(static_cast<int32_t>((scaled + bias) / denom)));
    }

    [[nodiscard]] constexpr raw_t raw_value() const noexcept { return raw; }
    [[nodiscard]] float to_float() const noexcept { return raw_to_float(raw); }
    [[nodiscard]] constexpr int32_t int_trunc() const noexcept { return static_cast<int32_t>(raw) >> frac_bits; }
    [[nodiscard]] constexpr uint8_t frac_raw() const noexcept { return static_cast<uint8_t>(raw & (scale - 1)); }

    [[nodiscard]] constexpr int32_t round_int() const noexcept { return round_raw_to_int(raw); }

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
        return (raw >= 0) ? *this : from_raw(static_cast<raw_t>(raw == FP16_MIN_RAW ? FP16_MAX_RAW : -raw));
    }

    // Arithmetic
    [[nodiscard]] constexpr fp16_t operator+(fp16_t rhs) const noexcept {
        return from_raw(clamp_raw(static_cast<int32_t>(raw) + static_cast<int32_t>(rhs.raw)));
    }
    [[nodiscard]] constexpr fp16_t operator-(fp16_t rhs) const noexcept {
        return from_raw(clamp_raw(static_cast<int32_t>(raw) - static_cast<int32_t>(rhs.raw)));
    }
    [[nodiscard]] constexpr fp16_t operator*(fp16_t rhs) const noexcept {
        return from_raw(clamp_raw(mul_raw(raw, rhs.raw)));
    }
    [[nodiscard]] constexpr fp16_t operator/(fp16_t rhs) const noexcept {
        if (rhs.raw == 0) {
            return from_raw(static_cast<raw_t>(0));
        }
        return from_raw(clamp_raw(div_raw(raw, rhs.raw)));
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

    explicit constexpr operator raw_t() const noexcept { return raw; }
    explicit operator float() const noexcept { return to_float(); }
};

struct fp32_t {
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

    static constexpr raw_t float_to_raw_constexpr(float v) noexcept {
        const float scaled = v * static_cast<float>(FP32_SCALE);
        const float adj = (scaled >= 0.0f) ? 0.5f : -0.5f; // round to nearest, ties up
        const int64_t raw = static_cast<int64_t>(scaled + adj);
        return saturate_raw(raw);
    }

    static float raw_to_float(raw_t v) noexcept {
        return static_cast<float>(v) / static_cast<float>(FP32_SCALE);
    }

    static constexpr int32_t round_raw_to_int(raw_t v) noexcept {
        const int32_t bias = (v >= 0) ? (FP32_SCALE / 2) : -(FP32_SCALE / 2);
        return (v + bias) >> FP32_FRAC_BITS;
    }

public:
    constexpr fp32_t() = default;
    explicit constexpr fp32_t(int32_t v) : raw(clamp_raw(static_cast<int64_t>(v) << frac_bits)) {}
    explicit fp32_t(float v) : raw(float_to_raw_constexpr(v)) {}

    static constexpr fp32_t from_raw(raw_t r) noexcept {
        fp32_t out{};
        out.raw = r;
        return out;
    }
    static constexpr fp32_t from_int(int32_t v) noexcept { return fp32_t{v}; }
    static fp32_t from_float(float v) noexcept { return fp32_t{v}; }
    static constexpr fp32_t float_const(float v) noexcept { return from_raw(float_to_raw_constexpr(v)); }
    static constexpr fp32_t from_ratio(int32_t numer, int32_t denom) noexcept {
        // round-to-nearest, ties up
        const int64_t scaled = static_cast<int64_t>(FP32_SCALE) * static_cast<int64_t>(numer);
        const int64_t bias = (scaled >= 0) ? (static_cast<int64_t>(denom) / 2) : -(static_cast<int64_t>(denom) / 2);
        return from_raw(clamp_raw((scaled + bias) / denom));
    }

    [[nodiscard]] constexpr raw_t raw_value() const noexcept { return raw; }
    [[nodiscard]] float to_float() const noexcept { return raw_to_float(raw); }
    [[nodiscard]] constexpr int32_t int_trunc() const noexcept { return static_cast<int32_t>(raw) >> frac_bits; }
    [[nodiscard]] constexpr uint16_t frac_raw() const noexcept { return static_cast<uint16_t>(raw & (scale - 1)); }

    [[nodiscard]] constexpr int32_t round_int() const noexcept { return round_raw_to_int(raw); }

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
        return (raw >= 0) ? *this : from_raw(raw == FP32_MIN_RAW ? FP32_MAX_RAW : -raw);
    }

    // Arithmetic
    [[nodiscard]] constexpr fp32_t operator+(fp32_t rhs) const noexcept {
        return from_raw(clamp_raw(static_cast<int64_t>(raw) + static_cast<int64_t>(rhs.raw)));
    }
    [[nodiscard]] constexpr fp32_t operator-(fp32_t rhs) const noexcept {
        return from_raw(clamp_raw(static_cast<int64_t>(raw) - static_cast<int64_t>(rhs.raw)));
    }
    [[nodiscard]] constexpr fp32_t operator*(fp32_t rhs) const noexcept {
        return from_raw(clamp_raw(mul_raw(raw, rhs.raw)));
    }
    [[nodiscard]] constexpr fp32_t operator/(fp32_t rhs) const noexcept {
        return rhs.raw == 0 ? from_raw(0) : from_raw(clamp_raw(div_raw(raw, rhs.raw)));
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

    explicit constexpr operator raw_t() const noexcept { return raw; }
    explicit operator float() const noexcept { return to_float(); }
};

// Trigonometry on fixed-point types (argument in radians, result fixed-point).
inline fp16_t fp16_sin(fp16_t angle) noexcept {
    return fp16_t{std::sin(angle.to_float())};
}

inline fp16_t fp16_cos(fp16_t angle) noexcept {
    return fp16_t{std::cos(angle.to_float())};
}

inline fp32_t fp32_sin(fp32_t angle) noexcept {
    return fp32_t{std::sin(angle.to_float())};
}

inline fp32_t fp32_cos(fp32_t angle) noexcept {
    return fp32_t{std::cos(angle.to_float())};
}

} // namespace matrix_pixels_math


