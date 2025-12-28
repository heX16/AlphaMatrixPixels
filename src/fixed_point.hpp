#pragma once

#include <stdint.h>
#include <math.h>
#include "amp_constexpr.hpp"

// Fixed-point helpers are kept in a separate header to reduce compile time and keep math.hpp focused.
namespace amp {
namespace math {

// Fixed-point helpers:
// csFP16: signed 8.8 (int16_t)  -> scale = 256
// csFP32: signed 16.16 (fp_type=int32_t) -> scale = 65536

struct csFP16 {
public:
    using fp_type = int16_t;
    // Helper type for intermediate calculations (wider than fp_type).
    using fp_type2 = int32_t;

    fp_type raw{0};

    static constexpr int frac_bits = 8;
    static constexpr fp_type scale = static_cast<fp_type>(1 << frac_bits); // 256
    static constexpr fp_type min_raw = static_cast<fp_type>(-32768);
    static constexpr fp_type max_raw = static_cast<fp_type>(32767);

private:
    // Clamp helpers for intermediate math.
    static AMP_CONSTEXPR fp_type clamp_raw(fp_type2 v) noexcept {
        return v > static_cast<fp_type2>(max_raw) ? max_raw
             : v < static_cast<fp_type2>(min_raw) ? min_raw
             : static_cast<fp_type>(v);
    }

    static AMP_CONSTEXPR fp_type mul_raw(fp_type a, fp_type b) noexcept {
        return static_cast<fp_type>((static_cast<fp_type2>(a) * static_cast<fp_type2>(b)) >> frac_bits);
    }

    static AMP_CONSTEXPR fp_type div_raw(fp_type num, fp_type den) noexcept {
        return den == 0 ? static_cast<fp_type>(0)
                        : static_cast<fp_type>((static_cast<fp_type2>(num) << frac_bits) / den);
    }

    static AMP_CONSTEXPR int16_t saturate_raw(fp_type2 v) noexcept {
        return v > static_cast<fp_type2>(max_raw) ? static_cast<int16_t>(max_raw)
             : v < static_cast<fp_type2>(min_raw) ? static_cast<int16_t>(min_raw)
             : static_cast<int16_t>(v);
    }

    static AMP_CONSTEXPR fp_type float_to_raw_constexpr(float v) noexcept {
        return saturate_raw(static_cast<fp_type2>(
            (v * static_cast<float>(scale)) +
            ((v * static_cast<float>(scale)) >= 0.0f ? 0.5f : -0.5f))); // round to nearest, ties up
    }


    static float raw_to_float(fp_type v) noexcept {
        return static_cast<float>(v) / static_cast<float>(scale);
    }

    static inline fp_type2 round_raw_to_int(fp_type v) noexcept {
        const fp_type2 half = static_cast<fp_type2>(scale) / 2;
        const fp_type2 bias = (v >= 0) ? half : -half;
        return static_cast<fp_type2>(v + bias) >> frac_bits;
    }

public:
    csFP16() = default;
    explicit csFP16(float v) : raw(float_to_raw_constexpr(v)) {}

    static inline csFP16 from_raw(fp_type r) noexcept {
        csFP16 out{};
        out.raw = r;
        return out;
    }
    static inline csFP16 from_int(fp_type2 v) noexcept { return from_raw(clamp_raw(v << frac_bits)); }
    static csFP16 from_float(float v) noexcept { return csFP16(v); }
    static inline csFP16 float_const(float v) noexcept { return from_raw(float_to_raw_constexpr(v)); }
    static inline csFP16 from_ratio(fp_type2 numer, fp_type2 denom) noexcept {
        // round-to-nearest, ties up
        // Note: intermediate math uses fp_type2 (int32_t). Large numer/denom may overflow.
        const fp_type2 scaled = static_cast<fp_type2>(scale) * static_cast<fp_type2>(numer);
        const fp_type2 bias = (scaled >= 0) ? (static_cast<fp_type2>(denom) / 2) : -(static_cast<fp_type2>(denom) / 2);
        return from_raw(clamp_raw((scaled + bias) / denom));
    }

    [[nodiscard]] AMP_CONSTEXPR fp_type raw_value() const noexcept { return raw; }
    [[nodiscard]] float to_float() const noexcept { return raw_to_float(raw); }
    [[nodiscard]] AMP_CONSTEXPR fp_type2 int_trunc() const noexcept { return static_cast<fp_type2>(raw) >> frac_bits; }
    [[nodiscard]] AMP_CONSTEXPR uint8_t frac_raw() const noexcept { return static_cast<uint8_t>(raw & (scale - 1)); }

    [[nodiscard]] inline fp_type2 round_int() const noexcept { return round_raw_to_int(raw); }

    [[nodiscard]] inline fp_type2 floor_int() const noexcept {
        if (raw >= 0) {
            return static_cast<fp_type2>(raw) >> frac_bits;
        }
        const fp_type2 mask = static_cast<fp_type2>(scale - 1);
        const fp_type2 r = static_cast<fp_type2>(raw);
        return (r & mask) ? ((r >> frac_bits) - 1) : (r >> frac_bits);
    }

    [[nodiscard]] inline fp_type2 ceil_int() const noexcept {
        if (raw >= 0) {
            const fp_type2 mask = static_cast<fp_type2>(scale - 1);
            const fp_type2 r = static_cast<fp_type2>(raw);
            return (r & mask) ? ((r >> frac_bits) + 1) : (r >> frac_bits);
        }
        return static_cast<fp_type2>(raw) >> frac_bits;
    }

    [[nodiscard]] inline csFP16 absVal() const noexcept {
        return (raw >= 0) ? *this : from_raw(static_cast<fp_type>(raw == min_raw ? max_raw : -raw));
    }

    // Arithmetic
    [[nodiscard]] inline csFP16 operator+(csFP16 rhs) const noexcept {
        return from_raw(clamp_raw(static_cast<fp_type2>(raw) + static_cast<fp_type2>(rhs.raw)));
    }
    [[nodiscard]] inline csFP16 operator-(csFP16 rhs) const noexcept {
        return from_raw(clamp_raw(static_cast<fp_type2>(raw) - static_cast<fp_type2>(rhs.raw)));
    }
    [[nodiscard]] inline csFP16 operator*(csFP16 rhs) const noexcept {
        return from_raw(clamp_raw(mul_raw(raw, rhs.raw)));
    }
    [[nodiscard]] inline csFP16 operator/(csFP16 rhs) const noexcept {
        if (rhs.raw == 0) {
            return from_raw(static_cast<fp_type>(0));
        }
        return from_raw(clamp_raw(div_raw(raw, rhs.raw)));
    }

    csFP16& operator+=(csFP16 rhs) noexcept { return *this = *this + rhs; }
    csFP16& operator-=(csFP16 rhs) noexcept { return *this = *this - rhs; }
    csFP16& operator*=(csFP16 rhs) noexcept { return *this = *this * rhs; }
    csFP16& operator/=(csFP16 rhs) noexcept { return *this = *this / rhs; }

    // Comparisons
    [[nodiscard]] AMP_CONSTEXPR bool operator==(csFP16 rhs) const noexcept { return raw == rhs.raw; }
    [[nodiscard]] AMP_CONSTEXPR bool operator!=(csFP16 rhs) const noexcept { return raw != rhs.raw; }
    [[nodiscard]] AMP_CONSTEXPR bool operator<(csFP16 rhs) const noexcept { return raw < rhs.raw; }
    [[nodiscard]] AMP_CONSTEXPR bool operator<=(csFP16 rhs) const noexcept { return raw <= rhs.raw; }
    [[nodiscard]] AMP_CONSTEXPR bool operator>(csFP16 rhs) const noexcept { return raw > rhs.raw; }
    [[nodiscard]] AMP_CONSTEXPR bool operator>=(csFP16 rhs) const noexcept { return raw >= rhs.raw; }

    explicit operator float() const noexcept { return to_float(); }
};




struct csFP32 {
public:
    using fp_type = int32_t;
    // Helper type for intermediate calculations (wider than fp_type).
    using fp_type2 = int64_t;

    fp_type raw{0};

    static constexpr int frac_bits = 16;
    static constexpr fp_type scale = static_cast<fp_type>(1UL << frac_bits); // 65536
    static constexpr fp_type min_raw = static_cast<fp_type>(-2147483647 - 1);
    static constexpr fp_type max_raw = static_cast<fp_type>(2147483647);

private:
    // Clamp helpers for intermediate math.
    static AMP_CONSTEXPR fp_type clamp_raw(fp_type2 v) noexcept {
        return v > static_cast<fp_type2>(max_raw) ? max_raw
             : v < static_cast<fp_type2>(min_raw) ? min_raw
             : static_cast<fp_type>(v);
    }

    static AMP_CONSTEXPR fp_type mul_raw(fp_type a, fp_type b) noexcept {
        return static_cast<fp_type>((static_cast<fp_type2>(a) * static_cast<fp_type2>(b)) >> frac_bits);
    }

    static AMP_CONSTEXPR fp_type div_raw(fp_type num, fp_type den) noexcept {
        return den == 0 ? static_cast<fp_type>(0)
                        : static_cast<fp_type>((static_cast<fp_type2>(num) << frac_bits) / den);
    }

    static AMP_CONSTEXPR fp_type saturate_raw(fp_type2 v) noexcept {
        return v > static_cast<fp_type2>(max_raw) ? max_raw
             : v < static_cast<fp_type2>(min_raw) ? min_raw
             : static_cast<fp_type>(v);
    }

    static inline fp_type float_to_raw_constexpr(float v) noexcept {
        const float scaled = v * static_cast<float>(scale);
        const float adj = (scaled >= 0.0f) ? 0.5f : -0.5f; // round to nearest, ties up
        const fp_type2 raw = static_cast<fp_type2>(scaled + adj);
        return saturate_raw(raw);
    }

    static float raw_to_float(fp_type v) noexcept {
        return static_cast<float>(v) / static_cast<float>(scale);
    }

    static inline fp_type round_raw_to_int(fp_type v) noexcept {
        const fp_type half = scale / 2;
        const fp_type bias = (v >= 0) ? half : -half;
        return (v + bias) >> frac_bits;
    }

public:
    csFP32() = default;
    explicit csFP32(float v) : raw(float_to_raw_constexpr(v)) {}

    static inline csFP32 from_raw(fp_type r) noexcept {
        csFP32 out{};
        out.raw = r;
        return out;
    }
    static inline csFP32 from_int(fp_type v) noexcept { return from_raw(clamp_raw(static_cast<fp_type2>(v) << frac_bits)); }
    static csFP32 from_float(float v) noexcept { return csFP32(v); }
    static inline csFP32 float_const(float v) noexcept { return from_raw(float_to_raw_constexpr(v)); }
    static inline csFP32 from_ratio(fp_type numer, fp_type denom) noexcept {
        // round-to-nearest, ties up
        const fp_type2 scaled = static_cast<fp_type2>(scale) * static_cast<fp_type2>(numer);
        const fp_type2 bias = (scaled >= 0) ? (static_cast<fp_type2>(denom) / 2) : -(static_cast<fp_type2>(denom) / 2);
        return from_raw(clamp_raw((scaled + bias) / denom));
    }

    [[nodiscard]] AMP_CONSTEXPR fp_type raw_value() const noexcept { return raw; }
    [[nodiscard]] float to_float() const noexcept { return raw_to_float(raw); }
    [[nodiscard]] AMP_CONSTEXPR fp_type int_trunc() const noexcept { return static_cast<fp_type>(raw >> frac_bits); }
    [[nodiscard]] AMP_CONSTEXPR uint16_t frac_raw() const noexcept { return static_cast<uint16_t>(raw & (scale - 1)); }

    [[nodiscard]] inline fp_type round_int() const noexcept { return round_raw_to_int(raw); }

    [[nodiscard]] inline fp_type floor_int() const noexcept {
        if (raw >= 0) {
            return static_cast<fp_type>(raw >> frac_bits);
        }
        const fp_type mask = static_cast<fp_type>(scale - 1);
        const fp_type r = raw;
        return (r & mask) ? ((r >> frac_bits) - 1) : (r >> frac_bits);
    }

    [[nodiscard]] inline fp_type ceil_int() const noexcept {
        if (raw >= 0) {
            const fp_type mask = static_cast<fp_type>(scale - 1);
            const fp_type r = raw;
            return (r & mask) ? ((r >> frac_bits) + 1) : (r >> frac_bits);
        }
        return static_cast<fp_type>(raw >> frac_bits);
    }

    [[nodiscard]] inline csFP32 absVal() const noexcept {
        return (raw >= 0) ? *this : from_raw(raw == min_raw ? max_raw : -raw);
    }

    // Arithmetic
    [[nodiscard]] inline csFP32 operator+(csFP32 rhs) const noexcept {
        return from_raw(clamp_raw(static_cast<fp_type2>(raw) + static_cast<fp_type2>(rhs.raw)));
    }
    [[nodiscard]] inline csFP32 operator-(csFP32 rhs) const noexcept {
        return from_raw(clamp_raw(static_cast<fp_type2>(raw) - static_cast<fp_type2>(rhs.raw)));
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
    [[nodiscard]] AMP_CONSTEXPR bool operator==(csFP32 rhs) const noexcept { return raw == rhs.raw; }
    [[nodiscard]] AMP_CONSTEXPR bool operator!=(csFP32 rhs) const noexcept { return raw != rhs.raw; }
    [[nodiscard]] AMP_CONSTEXPR bool operator<(csFP32 rhs) const noexcept { return raw < rhs.raw; }
    [[nodiscard]] AMP_CONSTEXPR bool operator<=(csFP32 rhs) const noexcept { return raw <= rhs.raw; }
    [[nodiscard]] AMP_CONSTEXPR bool operator>(csFP32 rhs) const noexcept { return raw > rhs.raw; }
    [[nodiscard]] AMP_CONSTEXPR bool operator>=(csFP32 rhs) const noexcept { return raw >= rhs.raw; }

    explicit operator float() const noexcept { return to_float(); }
};

// Conversion between fixed-point types.
inline csFP32 fp16_to_fp32(csFP16 fp16) noexcept {
    // Convert FP16 (8.8) to FP32 (16.16) by shifting raw value left by 8 bits.
    return csFP32::from_raw(static_cast<csFP32::fp_type>(fp16.raw_value()) << 8);
}

inline csFP16 fp32_to_fp16(csFP32 fp32) noexcept {
    // Convert FP32 (16.16) to FP16 (8.8) by shifting raw value right by 8 bits with rounding.
    // Round to nearest: add 128 (half of 256) before shifting.
    const int32_t raw32 = fp32.raw_value();
    const int32_t rounded = (raw32 >= 0) 
        ? (raw32 + 128) >> 8
        : (raw32 - 128) >> 8;
    // Clamp to FP16 range and convert.
    const int16_t clamped = (rounded > 32767) ? 32767
                          : (rounded < -32768) ? -32768
                          : static_cast<int16_t>(rounded);
    return csFP16::from_raw(clamped);
}

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


