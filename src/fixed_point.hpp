#pragma once

#include <stdint.h>
#include <math.h>
#include "amp_constexpr.hpp"

// Fixed-point helpers are kept in a separate header to reduce compile time and keep math.hpp focused.
namespace amp {
namespace math {

// Fixed-point helpers:
// csFP16: signed 12.4 (int16_t)  -> scale = 16
// csFP32: signed 16.16 (fp_type=int32_t) -> scale = 65536

// Traits class that contains all type-specific information
template<typename IntType>
struct fp_traits;

// Specialization for int8_t
template<>
struct fp_traits<int8_t> {
    using fp_type = int8_t;
    using fp_type2 = int16_t;
    static constexpr fp_type min_raw = static_cast<fp_type>(-128);
    static constexpr fp_type max_raw = static_cast<fp_type>(127);
};

// Specialization for int16_t
template<>
struct fp_traits<int16_t> {
    using fp_type = int16_t;
    using fp_type2 = int32_t;
    static constexpr fp_type min_raw = static_cast<fp_type>(-32768);
    static constexpr fp_type max_raw = static_cast<fp_type>(32767);
};

// Specialization for int32_t
template<>
struct fp_traits<int32_t> {
    using fp_type = int32_t;
    using fp_type2 = int64_t;
    static constexpr fp_type min_raw = static_cast<fp_type>(-2147483647 - 1);
    static constexpr fp_type max_raw = static_cast<fp_type>(2147483647);
};

// Template fixed-point class
template<typename IntType, int FracBits>
struct csFP {
public:
    using traits = fp_traits<IntType>;
    using fp_type = typename traits::fp_type;
    // Helper type for intermediate calculations (wider than fp_type).
    using fp_type2 = typename traits::fp_type2;

    fp_type raw{0};

    static constexpr int frac_bits = FracBits;
    static constexpr fp_type scale = static_cast<fp_type>(1UL << frac_bits);
    static constexpr fp_type min_raw = traits::min_raw;
    static constexpr fp_type max_raw = traits::max_raw;

    static constexpr fp_type one_raw = scale; // Raw value for 1.0
    static constexpr fp_type minimal_value_raw = static_cast<fp_type>(1); // Minimal positive raw value (1/scale)

    // Basic mathematical constants (defined below for csFP16 and csFP32)
    static const csFP pi;
    static const csFP piHalf;
    static const csFP pi2;
    static const csFP degToRad;
    static const csFP half;
    static const csFP zero;
    static const csFP one;

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

    static AMP_CONSTEXPR fp_type float_to_raw_constexpr(float v) noexcept {
        // For smaller types (int8_t, int16_t), use simpler calculation
        if constexpr (sizeof(fp_type) <= 2) {
            return saturate_raw(static_cast<fp_type2>(
                (v * static_cast<float>(scale)) +
                ((v * static_cast<float>(scale)) >= 0.0f ? 0.5f : -0.5f))); // round to nearest, ties up
        } else {
            // For int32_t, use the more precise calculation
            const float scaled = v * static_cast<float>(scale);
            const float adj = (scaled >= 0.0f) ? 0.5f : -0.5f; // round to nearest, ties up
            const fp_type2 raw = static_cast<fp_type2>(scaled + adj);
            return saturate_raw(raw);
        }
    }

    static float raw_to_float(fp_type v) noexcept {
        return static_cast<float>(v) / static_cast<float>(scale);
    }

    // round_raw_to_int: returns fp_type (integer part after rounding)
    static inline fp_type round_raw_to_int(fp_type v) noexcept {
        if constexpr (sizeof(fp_type) == 2) {
            const fp_type2 half = static_cast<fp_type2>(scale) / 2;
            const fp_type2 bias = (v >= 0) ? half : -half;
            return static_cast<fp_type>(static_cast<fp_type2>(v + bias) >> frac_bits);
        } else {
            const fp_type half = scale / 2;
            const fp_type bias = (v >= 0) ? half : -half;
            return (v + bias) >> frac_bits;
        }
    }

public:
    csFP() = default;
    explicit csFP(float v) : raw(float_to_raw_constexpr(v)) {}

    static inline csFP from_raw(fp_type r) noexcept {
        csFP out{};
        out.raw = r;
        return out;
    }
    
    // from_int: for int16_t uses fp_type2, for int32_t uses fp_type
    // Use overloads instead of std::conditional for Arduino compatibility
    static inline csFP from_int(fp_type2 v) noexcept {
        return from_raw(clamp_raw(static_cast<fp_type2>(v) << frac_bits));
    }
    static inline csFP from_int(fp_type v) noexcept {
        return from_raw(clamp_raw(static_cast<fp_type2>(v) << frac_bits));
    }
    
    static csFP from_float(float v) noexcept { return csFP(v); }
    static inline csFP float_const(float v) noexcept { return from_raw(float_to_raw_constexpr(v)); }
    static inline csFP from_ratio(fp_type2 numer, fp_type2 denom) noexcept {
        // round-to-nearest, ties up
        // Note: intermediate math uses fp_type2. Large numer/denom may overflow.
        const fp_type2 scaled = static_cast<fp_type2>(scale) * static_cast<fp_type2>(numer);
        const fp_type2 bias = (scaled >= 0) ? (static_cast<fp_type2>(denom) / 2) : -(static_cast<fp_type2>(denom) / 2);
        return from_raw(clamp_raw((scaled + bias) / denom));
    }

    [[nodiscard]] AMP_CONSTEXPR fp_type raw_value() const noexcept { return raw; }
    [[nodiscard]] float to_float() const noexcept { return raw_to_float(raw); }
    
    // Truncates towards zero. Returns integer part only.
    // Example: `csFP16(3.75f).int_trunc() == 3, csFP16(-3.75f).int_trunc() == -3`
    // Returns: fp_type2 for int16_t, fp_type for int32_t
    [[nodiscard]] AMP_CONSTEXPR auto int_trunc() const noexcept -> decltype(static_cast<fp_type2>(raw) >> frac_bits) {
        if constexpr (sizeof(fp_type) == 2) {
            return static_cast<fp_type2>(raw) >> frac_bits;
        } else {
            return static_cast<fp_type>(raw >> frac_bits);
        }
    }
    
    // Returns absolute fractional part in raw format (0 to scale-1).
    // Always returns positive value by using absVal() internally.
    // Example: 
    //   `csFP16(3.75f).frac_abs_raw() == 12 (0.75 * 16)`
    //   `csFP16(-3.75f).frac_abs_raw() == 12` (correct, not 4)
    [[nodiscard]] AMP_CONSTEXPR fp_type frac_abs_raw() const noexcept {
        return absVal().raw & (scale - 1);
    }

    // Returns signed fractional part in raw format (-(scale-1) to (scale-1)).
    // This corresponds to the fractional part after truncation towards zero.
    // Examples:
    //   `csFP16(3.75f).frac_raw_signed() == 12`
    //   `csFP16(-3.75f).frac_raw_signed() == -12`
    //   `csFP16(-3.25f).frac_raw_signed() == -4`
    [[nodiscard]] AMP_CONSTEXPR fp_type frac_raw_signed() const noexcept {
        return (raw < 0) ? -frac_abs_raw() : frac_abs_raw();
    }
    
    

    // Rounds to nearest integer (ties up).
    // Example: `csFP16(3.5f).round_int() == 4, csFP16(3.4f).round_int() == 3`
    // Returns: fp_type (int16_t for csFP16, int32_t for csFP32)
    [[nodiscard]] inline fp_type round_int() const noexcept {
        return round_raw_to_int(raw);
    }

    // Floors towards negative infinity.
    // Example: `csFP16(3.75f).floor_int() == 3, csFP16(-3.25f).floor_int() == -4`
    // Returns: fp_type2 for int16_t, fp_type for int32_t
    [[nodiscard]] inline auto floor_int() const noexcept -> decltype(static_cast<fp_type2>(raw) >> frac_bits) {
        if constexpr (sizeof(fp_type) == 2) {
            return static_cast<fp_type2>(raw) >> frac_bits;
        } else {
            return raw >> frac_bits;
        }
    }

    // Ceils towards positive infinity.
    // Example: `csFP16(3.25f).ceil_int() == 4, csFP16(-3.75f).ceil_int() == -3`
    // Returns: fp_type2 for int16_t, fp_type for int32_t
    [[nodiscard]] inline auto ceil_int() const noexcept -> decltype(static_cast<fp_type2>(raw) >> frac_bits) {
        if constexpr (sizeof(fp_type) == 2) {
            if (raw >= 0) {
                const fp_type2 mask = static_cast<fp_type2>(scale - 1);
                const fp_type2 r = static_cast<fp_type2>(raw);
                return (r & mask) ? ((r >> frac_bits) + 1) : (r >> frac_bits);
            }
            return static_cast<fp_type2>(raw) >> frac_bits;
        } else {
            if (raw >= 0) {
                const fp_type mask = static_cast<fp_type>(scale - 1);
                const fp_type r = raw;
                return (r & mask) ? ((r >> frac_bits) + 1) : (r >> frac_bits);
            }
            return static_cast<fp_type>(raw >> frac_bits);
        }
    }

    // Returns absolute value. Handles min_raw edge case.
    // Example: `csFP16(-3.5f).absVal() == csFP16(3.5f)`
    [[nodiscard]] inline csFP absVal() const noexcept {
        return (raw >= 0) ? *this : from_raw(static_cast<fp_type>(raw == min_raw ? max_raw : -raw));
    }

    // Arithmetic
    [[nodiscard]] inline csFP operator+(csFP rhs) const noexcept {
        return from_raw(clamp_raw(static_cast<fp_type2>(raw) + static_cast<fp_type2>(rhs.raw)));
    }
    [[nodiscard]] inline csFP operator-(csFP rhs) const noexcept {
        return from_raw(clamp_raw(static_cast<fp_type2>(raw) - static_cast<fp_type2>(rhs.raw)));
    }
    [[nodiscard]] inline csFP operator*(csFP rhs) const noexcept {
        return from_raw(clamp_raw(mul_raw(raw, rhs.raw)));
    }
    [[nodiscard]] inline csFP operator/(csFP rhs) const noexcept {
        if (rhs.raw == 0) {
            return from_raw(static_cast<fp_type>(0));
        }
        return from_raw(clamp_raw(div_raw(raw, rhs.raw)));
    }

    csFP& operator+=(csFP rhs) noexcept { return *this = *this + rhs; }
    csFP& operator-=(csFP rhs) noexcept { return *this = *this - rhs; }
    csFP& operator*=(csFP rhs) noexcept { return *this = *this * rhs; }
    csFP& operator/=(csFP rhs) noexcept { return *this = *this / rhs; }

    // Comparisons
    [[nodiscard]] AMP_CONSTEXPR bool operator==(csFP rhs) const noexcept { return raw == rhs.raw; }
    [[nodiscard]] AMP_CONSTEXPR bool operator!=(csFP rhs) const noexcept { return raw != rhs.raw; }
    [[nodiscard]] AMP_CONSTEXPR bool operator<(csFP rhs) const noexcept { return raw < rhs.raw; }
    [[nodiscard]] AMP_CONSTEXPR bool operator<=(csFP rhs) const noexcept { return raw <= rhs.raw; }
    [[nodiscard]] AMP_CONSTEXPR bool operator>(csFP rhs) const noexcept { return raw > rhs.raw; }
    [[nodiscard]] AMP_CONSTEXPR bool operator>=(csFP rhs) const noexcept { return raw >= rhs.raw; }

    explicit operator float() const noexcept { return to_float(); }
};

// Type aliases for backward compatibility
using csFP16 = csFP<int16_t, 4>;
using csFP32 = csFP<int32_t, 16>;

// Definitions of mathematical constants for csFP16
template<>
inline const csFP16 csFP<int16_t, 4>::pi = csFP16::float_const(3.141592653589793f);
template<>
inline const csFP16 csFP<int16_t, 4>::piHalf = csFP16::float_const(1.570796326794897f);
template<>
inline const csFP16 csFP<int16_t, 4>::pi2 = csFP16::float_const(6.283185307179586f);
template<>
inline const csFP16 csFP<int16_t, 4>::degToRad = csFP16::float_const(0.017453292519943295f);
template<>
inline const csFP16 csFP<int16_t, 4>::half = csFP16::float_const(0.5f);
template<>
inline const csFP16 csFP<int16_t, 4>::zero = csFP16::float_const(0.0f);
template<>
inline const csFP16 csFP<int16_t, 4>::one = csFP16::float_const(1.0f);

// Definitions of mathematical constants for csFP32
template<>
inline const csFP32 csFP<int32_t, 16>::pi = csFP32::float_const(3.141592653589793f);
template<>
inline const csFP32 csFP<int32_t, 16>::piHalf = csFP32::float_const(1.570796326794897f);
template<>
inline const csFP32 csFP<int32_t, 16>::pi2 = csFP32::float_const(6.283185307179586f);
template<>
inline const csFP32 csFP<int32_t, 16>::degToRad = csFP32::float_const(0.017453292519943295f);
template<>
inline const csFP32 csFP<int32_t, 16>::half = csFP32::float_const(0.5f);
template<>
inline const csFP32 csFP<int32_t, 16>::zero = csFP32::float_const(0.0f);
template<>
inline const csFP32 csFP<int32_t, 16>::one = csFP32::float_const(1.0f);

// Conversion between fixed-point types.
inline csFP32 fp16_to_fp32(csFP16 fp16) noexcept {
    // Convert FP16 (12.4) to FP32 (16.16) by shifting raw value left by 12 bits.
    return csFP32::from_raw(static_cast<csFP32::fp_type>(fp16.raw_value()) << (csFP32::frac_bits - csFP16::frac_bits));
}

inline csFP16 fp32_to_fp16(csFP32 fp32) noexcept {
    // Convert FP32 (16.16) to FP16 (12.4) by shifting raw value right by 12 bits with rounding.
    // Round to nearest: add half of FP16 scale before shifting.
    const csFP32::fp_type raw32 = fp32.raw_value();
    const csFP32::fp_type half_scale = csFP16::scale / 2; // half of FP16 scale
    const csFP32::fp_type rounded = (raw32 >= 0) 
        ? (raw32 + half_scale) >> (csFP32::frac_bits - csFP16::frac_bits)
        : (raw32 - half_scale) >> (csFP32::frac_bits - csFP16::frac_bits);
    // Clamp to FP16 range and convert.
    const csFP16::fp_type clamped = (rounded > csFP16::max_raw) ? csFP16::max_raw
                          : (rounded < csFP16::min_raw) ? csFP16::min_raw
                          : static_cast<csFP16::fp_type>(rounded);
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


