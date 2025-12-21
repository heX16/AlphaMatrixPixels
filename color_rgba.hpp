#pragma once

#include <cstdint>

using std::uint8_t;
using std::uint16_t;
using std::uint32_t;

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
    constexpr csColorRGBA() : value{0} {}
    // Initialize explicit channels in declared order to avoid reorder warnings.
    // Argument order: a, r, g, b to match internal layout.
    constexpr csColorRGBA(uint8_t a_, uint8_t r_, uint8_t g_, uint8_t b_) : a(a_), r(r_), g(g_), b(b_) {}

    // Construct from packed 0xAARRGGBB or 0xRRGGBB.
    // WARN: If alpha is zero, treat input as 0xRRGGBB and force A=0xFF.
    // This is an "opaque RGB" mode; it is not a way to create transparent ARGB with A=0.
    // Example:
    //   csColorRGBA c1{0x00010203}; // becomes 0xFF010203 (opaque RGB)
    //   csColorRGBA c2{0x7F000000}; // stays 0x7F000000 (alpha-only preserved)
    explicit constexpr csColorRGBA(uint32_t packed) : value{0} {
        const uint8_t Aa = static_cast<uint8_t>((packed >> 24) & 0xFF);
        const uint8_t Rr = static_cast<uint8_t>((packed >> 16) & 0xFF);
        const uint8_t Gg = static_cast<uint8_t>((packed >> 8) & 0xFF);
        const uint8_t Bb = static_cast<uint8_t>(packed & 0xFF);
        a = (Aa == 0) ? 0xFF : Aa;
        r = Rr;
        g = Gg;
        b = Bb;
    }

    constexpr csColorRGBA& operator=(const csColorRGBA& rhs) = default;

    // Divide each channel by constant (integer division), e.g.:
    // Example: `csColorRGBA c{200,100,50,255}; c /= 2; // -> {100,50,25,127}`
    constexpr csColorRGBA& operator/=(uint8_t d) noexcept {
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
    [[nodiscard]] constexpr csColorRGBA operator+(const csColorRGBA& src) const noexcept {
        return sourceOverStraight(*this, src);
    }

    // Blend src over this color and store the result here.
    constexpr csColorRGBA& operator+=(const csColorRGBA& src) noexcept {
        *this = sourceOverStraight(*this, src);
        return *this;
    }

    // Blend single channel (SourceOver, straight alpha):
    // Cs/Cd - source/dest channel; As/Ad - source/dest alphas; `invAs = 255 - As`; Aout - resulting alpha.
    [[nodiscard]] static constexpr uint8_t blendChannel(uint8_t Cs, uint8_t Cd, uint8_t As, uint8_t Ad, uint8_t invAs, uint8_t Aout) noexcept {
        const uint16_t src_p = mul8(Cs, As);
        const uint16_t dst_p = mul8(Cd, Ad);
        const uint16_t out_p = static_cast<uint16_t>(src_p + mul8(static_cast<uint8_t>(dst_p), invAs));
        return div255(out_p, Aout);
    }

    // Porter-Duff SourceOver with straight alpha; applies extra global alpha to source.
    [[nodiscard]] static constexpr csColorRGBA sourceOverStraight(csColorRGBA dst, csColorRGBA src, uint8_t global_alpha) noexcept {
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
    [[nodiscard]] static constexpr csColorRGBA sourceOverStraight(csColorRGBA dst, csColorRGBA src) noexcept {
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
};

#if defined(_MSC_VER)
#  pragma pack(pop)
#endif

static_assert(sizeof(csColorRGBA) == 4, "csColorRGBA must be tightly packed to 4 bytes");

