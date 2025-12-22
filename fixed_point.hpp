#pragma once

#include <stdint.h>
#include <math.h>

/*
Compile-time keyword hook for FUNCTIONS.

Default: AMP_CONSTEXPR = constexpr.

Why this exists:
 - Some older/embedded C++ toolchains (Arduino-like) have incomplete/buggy support for `constexpr`
   in functions (especially when the function body uses float math or float->int conversions).
 - On such compilers this may compile very slowly, produce strange errors, or fail to compile at all.

If you hit compilation problems, you can switch constexpr-off for functions by overriding the macro
 (e.g. via compiler flags or before including this header):
   #define AMP_CONSTEXPR inline

Note:
 - This macro must be used ONLY for functions (not for variables/constants).
 - Replacing constexpr with inline means the function is no longer a compile-time constant expression.
*/
#ifndef AMP_CONSTEXPR
#  if defined(ARDUINO)
// Many Arduino toolchains are stuck on older GCC versions where `constexpr` can be buggy/slow
// (especially with float math and some union/aggregate patterns). Default to `inline` there.
#    define AMP_CONSTEXPR inline
#  else
#    define AMP_CONSTEXPR constexpr
#  endif
#endif

// Fixed-point helpers are kept in a separate header to reduce compile time and keep math.hpp focused.
namespace amp {
namespace math {

// Fixed-point helpers:
// csFP16: signed 8.8 (int16_t)  -> scale = 256
// csFP32: signed 16.16 (int32_t) -> scale = 65536

struct csFP16 {
    using fp_type = int16_t;
    // Helper type for intermediate calculations (wider than fp_type).
    using fp_type_double = int32_t;

    fp_type raw{0};

    static constexpr int frac_bits = 8;
    static constexpr fp_type scale = static_cast<fp_type>(1 << frac_bits); // 256
    static constexpr fp_type min_raw = static_cast<fp_type>(-32768);
    static constexpr fp_type max_raw = static_cast<fp_type>(32767);

private:
    // Clamp helpers for intermediate math.
    static AMP_CONSTEXPR fp_type clamp_raw(fp_type_double v) noexcept {
        return v > static_cast<fp_type_double>(max_raw) ? max_raw
             : v < static_cast<fp_type_double>(min_raw) ? min_raw
             : static_cast<fp_type>(v);
    }

    static AMP_CONSTEXPR fp_type mul_raw(fp_type a, fp_type b) noexcept {
        return static_cast<fp_type>((static_cast<fp_type_double>(a) * static_cast<fp_type_double>(b)) >> frac_bits);
    }

    static AMP_CONSTEXPR fp_type div_raw(fp_type num, fp_type den) noexcept {
        return den == 0 ? static_cast<fp_type>(0)
                        : static_cast<fp_type>((static_cast<fp_type_double>(num) << frac_bits) / den);
    }

    static AMP_CONSTEXPR int16_t saturate_raw(fp_type_double v) noexcept {
        return v > static_cast<fp_type_double>(max_raw) ? static_cast<int16_t>(max_raw)
             : v < static_cast<fp_type_double>(min_raw) ? static_cast<int16_t>(min_raw)
             : static_cast<int16_t>(v);
    }

    static AMP_CONSTEXPR fp_type float_to_raw_constexpr(float v) noexcept {
        return saturate_raw(static_cast<int32_t>(
            (v * static_cast<float>(scale)) +
            ((v * static_cast<float>(scale)) >= 0.0f ? 0.5f : -0.5f))); // round to nearest, ties up
    }


    static float raw_to_float(fp_type v) noexcept {
        return static_cast<float>(v) / static_cast<float>(scale);
    }

    static inline int32_t round_raw_to_int(fp_type v) noexcept {
        const int32_t half = static_cast<int32_t>(scale) / 2;
        const int32_t bias = (v >= 0) ? half : -half;
        return static_cast<int32_t>(v + bias) >> frac_bits;
    }

public:
    csFP16() = default;
    explicit csFP16(float v) : raw(float_to_raw_constexpr(v)) {}

    static inline csFP16 from_raw(fp_type r) noexcept {
        csFP16 out{};
        out.raw = r;
        return out;
    }
    static inline csFP16 from_int(int32_t v) noexcept { return from_raw(clamp_raw(v << frac_bits)); }
    static csFP16 from_float(float v) noexcept { return csFP16(v); }
    static inline csFP16 float_const(float v) noexcept { return from_raw(float_to_raw_constexpr(v)); }
    static inline csFP16 from_ratio(int32_t numer, int32_t denom) noexcept {
        // round-to-nearest, ties up
        const int64_t scaled = static_cast<int64_t>(scale) * static_cast<int64_t>(numer);
        const int64_t bias = (scaled >= 0) ? (static_cast<int64_t>(denom) / 2) : -(static_cast<int64_t>(denom) / 2);
        return from_raw(clamp_raw(static_cast<int32_t>((scaled + bias) / denom)));
    }

    [[nodiscard]] AMP_CONSTEXPR fp_type raw_value() const noexcept { return raw; }
    [[nodiscard]] float to_float() const noexcept { return raw_to_float(raw); }
    [[nodiscard]] AMP_CONSTEXPR int32_t int_trunc() const noexcept { return static_cast<int32_t>(raw) >> frac_bits; }
    [[nodiscard]] AMP_CONSTEXPR uint8_t frac_raw() const noexcept { return static_cast<uint8_t>(raw & (scale - 1)); }

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
        return (raw >= 0) ? *this : from_raw(static_cast<fp_type>(raw == min_raw ? max_raw : -raw));
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
    using fp_type = int32_t;
    // Helper type for intermediate calculations (wider than fp_type).
    using fp_type_double = int64_t;

    fp_type raw{0};

    static constexpr int frac_bits = 16;
    static constexpr fp_type scale = static_cast<fp_type>(1 << frac_bits); // 65536
    static constexpr fp_type min_raw = static_cast<fp_type>(-2147483647 - 1);
    static constexpr fp_type max_raw = static_cast<fp_type>(2147483647);

private:
    // Clamp helpers for intermediate math.
    static AMP_CONSTEXPR fp_type clamp_raw(fp_type_double v) noexcept {
        return v > static_cast<fp_type_double>(max_raw) ? max_raw
             : v < static_cast<fp_type_double>(min_raw) ? min_raw
             : static_cast<fp_type>(v);
    }

    static AMP_CONSTEXPR fp_type mul_raw(fp_type a, fp_type b) noexcept {
        return static_cast<fp_type>((static_cast<fp_type_double>(a) * static_cast<fp_type_double>(b)) >> frac_bits);
    }

    static AMP_CONSTEXPR fp_type div_raw(fp_type num, fp_type den) noexcept {
        return den == 0 ? static_cast<fp_type>(0)
                        : static_cast<fp_type>((static_cast<fp_type_double>(num) << frac_bits) / den);
    }

    static AMP_CONSTEXPR int32_t saturate_raw(fp_type_double v) noexcept {
        return v > static_cast<fp_type_double>(max_raw) ? max_raw
             : v < static_cast<fp_type_double>(min_raw) ? min_raw
             : static_cast<int32_t>(v);
    }

    static inline fp_type float_to_raw_constexpr(float v) noexcept {
        const float scaled = v * static_cast<float>(scale);
        const float adj = (scaled >= 0.0f) ? 0.5f : -0.5f; // round to nearest, ties up
        const fp_type_double raw = static_cast<fp_type_double>(scaled + adj);
        return saturate_raw(raw);
    }

    static float raw_to_float(fp_type v) noexcept {
        return static_cast<float>(v) / static_cast<float>(scale);
    }

    static inline int32_t round_raw_to_int(fp_type v) noexcept {
        const int32_t half = scale / 2;
        const int32_t bias = (v >= 0) ? half : -half;
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
    static inline csFP32 from_int(int32_t v) noexcept { return from_raw(clamp_raw(static_cast<int64_t>(v) << frac_bits)); }
    static csFP32 from_float(float v) noexcept { return csFP32(v); }
    static inline csFP32 float_const(float v) noexcept { return from_raw(float_to_raw_constexpr(v)); }
    static inline csFP32 from_ratio(int32_t numer, int32_t denom) noexcept {
        // round-to-nearest, ties up
        const int64_t scaled = static_cast<int64_t>(scale) * static_cast<int64_t>(numer);
        const int64_t bias = (scaled >= 0) ? (static_cast<int64_t>(denom) / 2) : -(static_cast<int64_t>(denom) / 2);
        return from_raw(clamp_raw((scaled + bias) / denom));
    }

    [[nodiscard]] AMP_CONSTEXPR fp_type raw_value() const noexcept { return raw; }
    [[nodiscard]] float to_float() const noexcept { return raw_to_float(raw); }
    [[nodiscard]] AMP_CONSTEXPR int32_t int_trunc() const noexcept { return static_cast<int32_t>(raw) >> frac_bits; }
    [[nodiscard]] AMP_CONSTEXPR uint16_t frac_raw() const noexcept { return static_cast<uint16_t>(raw & (scale - 1)); }

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
        return (raw >= 0) ? *this : from_raw(raw == min_raw ? max_raw : -raw);
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
    [[nodiscard]] AMP_CONSTEXPR bool operator==(csFP32 rhs) const noexcept { return raw == rhs.raw; }
    [[nodiscard]] AMP_CONSTEXPR bool operator!=(csFP32 rhs) const noexcept { return raw != rhs.raw; }
    [[nodiscard]] AMP_CONSTEXPR bool operator<(csFP32 rhs) const noexcept { return raw < rhs.raw; }
    [[nodiscard]] AMP_CONSTEXPR bool operator<=(csFP32 rhs) const noexcept { return raw <= rhs.raw; }
    [[nodiscard]] AMP_CONSTEXPR bool operator>(csFP32 rhs) const noexcept { return raw > rhs.raw; }
    [[nodiscard]] AMP_CONSTEXPR bool operator>=(csFP32 rhs) const noexcept { return raw >= rhs.raw; }

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


