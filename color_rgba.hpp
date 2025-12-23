#pragma once

#include <stdint.h>

namespace amp {

using ::uint8_t;
using ::uint16_t;
using ::uint32_t;

// Forward declaration
struct csColorRGBA;

struct csColorRGBA16 {
    uint16_t a;
    uint16_t r;
    uint16_t g;
    uint16_t b;

    csColorRGBA16() : a(0), r(0), g(0), b(0) {}
    csColorRGBA16(uint16_t a_, uint16_t r_, uint16_t g_, uint16_t b_) : a(a_), r(r_), g(g_), b(b_) {}

    csColorRGBA16& operator+=(const csColorRGBA16& other) noexcept {
        a += other.a;
        r += other.r;
        g += other.g;
        b += other.b;
        return *this;
    }

    // Divide all channels by constant with rounding to nearest.
    [[nodiscard]] inline csColorRGBA16 div(uint16_t d) const noexcept {
        const uint16_t half = d / 2u;
        return csColorRGBA16{
            static_cast<uint16_t>((a + half) / d),
            static_cast<uint16_t>((r + half) / d),
            static_cast<uint16_t>((g + half) / d),
            static_cast<uint16_t>((b + half) / d)
        };
    }

    // Convert back to 8-bit color by dividing and rounding.
    [[nodiscard]] inline csColorRGBA toColor8(uint16_t divisor) const noexcept;
};

// 8-bit multiply scaled by 1/255 with rounding-to-nearest.
// Example: mul8(128, 128) ~= round(128*128/255) = 64.
inline constexpr uint8_t mul8(uint8_t a, uint8_t b) noexcept {
    return static_cast<uint8_t>((static_cast<uint16_t>(a) * static_cast<uint16_t>(b) + 127u) / 255u);
}

// Divide premultiplied value by alpha with rounding to nearest.
inline constexpr uint8_t div255(uint16_t p, uint8_t A) noexcept {
    return A == 0 ? 0 : static_cast<uint8_t>((static_cast<uint32_t>(p) * 255u + static_cast<uint32_t>(A) / 2u) / static_cast<uint32_t>(A));
}

#if defined(__GNUC__) || defined(__clang__)
#  define CS_PACKED [[gnu::packed]]
#else
#  define CS_PACKED
#endif

#if defined(_MSC_VER)
#  pragma pack(push, 1)
#endif



// Packed ARGB color: 0xAARRGGBB (A in the most significant byte). Packed to 4 bytes.
struct CS_PACKED csColorRGBA {
    union {
        struct {
            uint8_t a;
            uint8_t r;
            uint8_t g;
            uint8_t b;
        };
        uint8_t raw[4];
        // Note: keep read/write consistent. If you read through `value`, write back through `value`
        // (not via fields) and vice versa, to avoid surprises on exotic ABIs/packing.
        // Also avoid writing raw `value` inside constructors that apply extra rules (see below).
        uint32_t value;
    };

    // Ensure deterministic zero-init for default ctor.
    csColorRGBA() : value{0} {}
    // Initialize explicit channels in declared order to avoid reorder warnings.
    // Argument order: a, r, g, b to match internal layout.
    csColorRGBA(uint8_t a_, uint8_t r_, uint8_t g_, uint8_t b_) : a(a_), r(r_), g(g_), b(b_) {}

    // Construct opaque RGB (alpha forced to 0xFF).
    // Argument order: r, g, b.
    csColorRGBA(uint8_t r_, uint8_t g_, uint8_t b_) : a(0xFF), r(r_), g(g_), b(b_) {}

    // Construct from packed 0xAARRGGBB or 0xRRGGBB.
    // WARN: If alpha is zero, treat input as 0xRRGGBB and force A=0xFF.
    // This is an "opaque RGB" mode; it is not a way to create transparent ARGB with A=0.
    // Example:
    //   csColorRGBA c1{0x00010203}; // becomes 0xFF010203 (opaque RGB)
    //   csColorRGBA c2{0x7F000000}; // stays 0x7F000000 (alpha-only preserved)
    explicit csColorRGBA(uint32_t packed) : value{0} {
        const uint8_t Aa = static_cast<uint8_t>((packed >> 24) & 0xFF);
        const uint8_t Rr = static_cast<uint8_t>((packed >> 16) & 0xFF);
        const uint8_t Gg = static_cast<uint8_t>((packed >> 8) & 0xFF);
        const uint8_t Bb = static_cast<uint8_t>(packed & 0xFF);
        a = (Aa == 0) ? 0xFF : Aa;
        r = Rr;
        g = Gg;
        b = Bb;
    }

    csColorRGBA& operator=(const csColorRGBA& rhs) = default;

    // Divide each channel by constant (integer division), e.g.:
    // Example: `csColorRGBA c{200,100,50,255}; c /= 2; // -> {100,50,25,127}`
    csColorRGBA& operator/=(uint8_t d) noexcept {
        r = static_cast<uint8_t>(r / d);
        g = static_cast<uint8_t>(g / d);
        b = static_cast<uint8_t>(b / d);
        a = static_cast<uint8_t>(a / d);
        return *this;
    }

    // Blend src over this color without modifying this instance.
    // Example: `csColorRGBA background{100, 50, 25, 255};`
    //          `csColorRGBA foreground{255, 0, 0, 128};`
    //          `auto blended = background + foreground;`
    [[nodiscard]] csColorRGBA operator+(const csColorRGBA& src) const noexcept {
        return sourceOverStraight(*this, src);
    }

    // Blend src over this color and store the result here.
    csColorRGBA& operator+=(const csColorRGBA& src) noexcept {
        *this = sourceOverStraight(*this, src);
        return *this;
    }

    // Blend single channel (SourceOver, straight alpha):
    // Cs/Cd - source/dest channel; As/Ad - source/dest alphas; `invAs = 255 - As`; Aout - resulting alpha.
    [[nodiscard]] static inline uint8_t blendChannel(uint8_t Cs, uint8_t Cd, uint8_t As, uint8_t Ad, uint8_t invAs, uint8_t Aout) noexcept {
        const uint16_t src_p = mul8(Cs, As);
        const uint16_t dst_p = mul8(Cd, Ad);
        const uint16_t out_p = static_cast<uint16_t>(src_p + mul8(static_cast<uint8_t>(dst_p), invAs));
        return div255(out_p, Aout);
    }

    // Porter-Duff SourceOver with straight alpha; applies extra global alpha to source.
    [[nodiscard]] static inline csColorRGBA sourceOverStraight(csColorRGBA dst, csColorRGBA src, uint8_t global_alpha) noexcept {
        const uint8_t As = mul8(src.a, global_alpha);
        const uint8_t invAs = static_cast<uint8_t>(255u - As);
        const uint8_t Aout = static_cast<uint8_t>(As + mul8(dst.a, invAs));

        if (Aout == 0) {
            return csColorRGBA{0, 0, 0, 0};
        }

        const uint8_t Rout = blendChannel(src.r, dst.r, As, dst.a, invAs, Aout);
        const uint8_t Gout = blendChannel(src.g, dst.g, As, dst.a, invAs, Aout);
        const uint8_t Bout = blendChannel(src.b, dst.b, As, dst.a, invAs, Aout);

        return csColorRGBA{Aout, Rout, Gout, Bout};
    }

    // Porter-Duff SourceOver with straight alpha; uses source alpha only.
    [[nodiscard]] static inline csColorRGBA sourceOverStraight(csColorRGBA dst, csColorRGBA src) noexcept {
        // no global alpha multiplier
        const uint8_t invAs = static_cast<uint8_t>(255u - src.a);
        const uint8_t Aout = static_cast<uint8_t>(src.a + mul8(dst.a, invAs));

        if (Aout == 0) {
            return csColorRGBA{0, 0, 0, 0};
        }

        const uint8_t Rout = blendChannel(src.r, dst.r, src.a, dst.a, invAs, Aout);
        const uint8_t Gout = blendChannel(src.g, dst.g, src.a, dst.a, invAs, Aout);
        const uint8_t Bout = blendChannel(src.b, dst.b, src.a, dst.a, invAs, Aout);

        return csColorRGBA{Aout, Rout, Gout, Bout};
    }

    // Add channels of this color to 'other' and return 16-bit result (no saturation).
    [[nodiscard]] inline csColorRGBA16 sum(csColorRGBA other) const noexcept {
        return csColorRGBA16{
            static_cast<uint16_t>(static_cast<uint16_t>(a) + static_cast<uint16_t>(other.a)),
            static_cast<uint16_t>(static_cast<uint16_t>(r) + static_cast<uint16_t>(other.r)),
            static_cast<uint16_t>(static_cast<uint16_t>(g) + static_cast<uint16_t>(other.g)),
            static_cast<uint16_t>(static_cast<uint16_t>(b) + static_cast<uint16_t>(other.b))
        };
    }
};

#if defined(_MSC_VER)
#  pragma pack(pop)
#endif

// csColorRGBA16::toColor8 implementation (defined after csColorRGBA is complete)
inline csColorRGBA csColorRGBA16::toColor8(uint16_t divisor) const noexcept {
    const uint16_t half = divisor / 2u;
    return csColorRGBA{
        static_cast<uint8_t>((a + half) / divisor),
        static_cast<uint8_t>((r + half) / divisor),
        static_cast<uint8_t>((g + half) / divisor),
        static_cast<uint8_t>((b + half) / divisor)
    };
}

// TODO: OPTIMIZE!!! - `uint8_t`
// Linear interpolation of two channels with t in [0..255]; t=0 -> a, t=255 -> b.
inline constexpr uint8_t lerp8(uint8_t a, uint8_t b, uint8_t t) noexcept {
    const int32_t delta = static_cast<int32_t>(b) - static_cast<int32_t>(a);
    const int32_t scaled = (delta * static_cast<int32_t>(t) + 127) / 255;
    return static_cast<uint8_t>(static_cast<int32_t>(a) + scaled);
}

// Linear interpolation of two colors; t in [0..255] (fixed-point 8.8 style).
// t=0 -> left, t=255 -> right.
inline csColorRGBA lerp(csColorRGBA left, csColorRGBA right, uint8_t t) noexcept {
    return csColorRGBA{
        lerp8(left.a, right.a, t),
        lerp8(left.r, right.r, t),
        lerp8(left.g, right.g, t),
        lerp8(left.b, right.b, t)
    };
}

} // namespace amp

static_assert(sizeof(amp::csColorRGBA) == 4, "csColorRGBA must be tightly packed to 4 bytes");

