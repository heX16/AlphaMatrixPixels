#pragma once

#if defined(ARDUINO)
#  include <Arduino.h>
#else
#  if defined(__has_include)
#    if __has_include(<cstdint>)
#      include <cstdint>
#    elif __has_include(<stdint.h>)
#      include <stdint.h>
#    endif
#  else
#    include <stdint.h>
#  endif
#endif

#include <cmath>

#include "color_rgba.hpp"
#include "matrix_pixels.hpp"
#include "rect.hpp"
#include "math.hpp"



namespace amp {

// Special random generator
class csRandGen {
public:

    // X(n+1) = (2053 * X(n)) + 13849)
    static constexpr auto RAND16_2053 = ((uint16_t)(2053));
    static constexpr auto RAND16_13849 = ((uint16_t)(13849));
    static constexpr auto RAND16_SEED = 1337;

    /// random number seed
    uint16_t rand_seed;

    csRandGen(uint16_t set_rand_seed = RAND16_SEED) {
        rand_seed = set_rand_seed;
    };

    /// Generate an 8-bit random number
    uint8_t rand()
    {
        // hard test
        //return 0;
        // test
        //return this->rand_seed++;

        rand_seed = (this->rand_seed * RAND16_2053) + RAND16_13849;
        // return first uint8_t (for more speed)
        return (uint8_t)(this->rand_seed & 0xFF);

        // return the sum of the high and low uint8_ts, for better
        //  mixing and non-sequential correlation
        //return (uint8_t)(((uint8_t)(this->rand_seed & 0xFF)) +
        //                 ((uint8_t)(this->rand_seed >> 8)));
    }

    /// Generate an 8-bit random number between 0 and lim
    /// @param lim the upper bound (not inclusive) for the result
    uint8_t rand(uint8_t lim)
    {
        uint8_t r = this->rand();
        r = (r*lim) >> 8;
        return r;
    }

    /// Generate an 8-bit random number in the given range
    /// @param min the lower bound for the random number
    /// @param max upper bound (inclusive) for the random number
    uint8_t randRange(uint8_t min, uint8_t max)
    {
        uint8_t delta = max - min + 1;
        uint8_t r = this->rand(delta) + min;
        return r;
    }

    /// Add entropy into the random number generator
    void addEntropy(uint16_t entropy)
    {
        this->rand_seed += entropy;
    }

};

class csParamsBase {
public:
    virtual ~csParamsBase() = default;

    virtual uint16_t count() const noexcept { return 0; }

    // return utf-8 constant string
    virtual const char* getParamName(uint8_t paramNum) const noexcept {
        (void)paramNum;
        return nullptr;
    }

    uint8_t findParamIdByName(const char* paramName) {
        (void)paramName;
        // TODO: WIP...
        return 0;
    }

    virtual bool getInt(uint8_t paramNum, uint32_t& value) const noexcept {
        (void)paramNum;
        (void)value;
        return false;
    }

    virtual void setInt(uint8_t paramNum, uint32_t value) noexcept {
        (void)paramNum;
        (void)value;
    }

    virtual bool getPtr(uint8_t paramNum, void*& pointer) const noexcept {
        (void)paramNum;
        pointer = nullptr;
        return false;
    }

    virtual void reset() noexcept {}
};

class csParamsRect : public csParamsBase {
public:
    csRect rect{};

    uint16_t count() const noexcept override { return 4; }

    const char* getParamName(uint8_t paramNum) const noexcept override {
        switch (paramNum) {
        case csParamsEnum::x: return "x";
        case csParamsEnum::y: return "y";
        case csParamsEnum::w: return "w";
        case csParamsEnum::h: return "h";
        default: return nullptr;
        }
    }

    bool getInt(uint8_t paramNum, uint32_t& value) const noexcept override {
        switch (paramNum) {
        case csParamsEnum::x: value = static_cast<uint32_t>(rect.x); return true;
        case csParamsEnum::y: value = static_cast<uint32_t>(rect.y); return true;
        case csParamsEnum::w: value = static_cast<uint32_t>(rect.width); return true;
        case csParamsEnum::h: value = static_cast<uint32_t>(rect.height); return true;
        default: return false;
        }
    }

    void setInt(uint8_t paramNum, uint32_t value) noexcept override {
        switch (paramNum) {
        case csParamsEnum::x: rect.x = to_coord(value); break;
        case csParamsEnum::y: rect.y = to_coord(value); break;
        case csParamsEnum::w: rect.width = to_size(value); break;
        case csParamsEnum::h: rect.height = to_size(value); break;
        default: break;
        }
    }

    void reset() noexcept override { rect = csRect{}; }
};

class csParamsEnum {
public:
    static constexpr auto x = 1;
    static constexpr auto y = 2;
    static constexpr auto w = 3;
    static constexpr auto h = 4; 

    static constexpr auto x2 = 5;
    static constexpr auto y2 = 6;
    static constexpr auto w2 = 7;
    static constexpr auto h2 = 8; 

    static constexpr auto speed = 10;   
    static constexpr auto dir = 11;

    // colors
    static constexpr auto color1 = 20;
    static constexpr auto color2 = 21;
    static constexpr auto color3 = 22;
    static constexpr auto color4 = 23;   

    // some special param
    static constexpr auto spec1 = 32;
    static constexpr auto spec2 = 33;
    static constexpr auto spec3 = 34;
    static constexpr auto spec4 = 35;
    static constexpr auto spec5 = 36;
    static constexpr auto spec6 = 37;
    static constexpr auto spec7 = 38;
    static constexpr auto spec8 = 39;
    static constexpr auto spec9 = 40;
    static constexpr auto spec10 = 41;
    static constexpr auto spec11 = 42;
    static constexpr auto spec12 = 43;
    static constexpr auto spec13 = 44;
    static constexpr auto spec14 = 45;
    static constexpr auto spec15 = 46;
    static constexpr auto spec16 = 47;     
};

class csEventBase {
    // Empty - WIP
};

class csRenderBase {
protected:
    csParamsBase* params_{nullptr};

    virtual csParamsBase* createParams() const { return new csParamsRect(); }

public:
    csRenderBase() : params_(createParams()) {}
    virtual ~csRenderBase() { delete params_; }

    inline csParamsBase* getParams() const noexcept { return params_; }

    virtual void recalc(const csMatrixPixels& led, csRandGen& rand, uint16_t currTime) {
        (void)led;
        (void)rand;
        (void)currTime;
    }

    virtual void render(csMatrixPixels& led, csRandGen& rand, uint16_t currTime) const {
        (void)led;
        (void)rand;
        (void)currTime;
    }

    // generate one event to external object
    virtual bool getEvent(const csMatrixPixels& led,
                          csRandGen& rand,
                          uint16_t currTime,
                          csEventBase& event,
                          uint16_t eventNum) const {
        (void)led;
        (void)rand;
        (void)currTime;
        (void)event;
        (void)eventNum;
        return false;
    }

    // receive one event from external object
    virtual void receiveEvent(const csEventBase& event) {
        (void)event;
    }
};

class csRenderGradient final : public csRenderBase {
public:
    void render(csMatrixPixels& matrix, csRandGen& /*rand*/, uint16_t currTime) const override {
        const float t = static_cast<float>(currTime) * 0.001f;

        auto wave = [](float v) -> uint8_t {
            return static_cast<uint8_t>((std::sin(v) * 0.5f + 0.5f) * 255.0f);
        };

        const int width = static_cast<int>(matrix.width());
        const int height = static_cast<int>(matrix.height());

        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                const float xf = static_cast<float>(x) * 0.4f;
                const float yf = static_cast<float>(y) * 0.4f;
                const uint8_t r = wave(t * 0.8f + xf);
                const uint8_t g = wave(t * 1.0f + yf);
                const uint8_t b = wave(t * 0.6f + xf + yf * 0.5f);
                matrix.setPixel(x, y, csColorRGBA{255, r, g, b});
            }
        }
    }
};

class csRenderGradientFP final : public csRenderBase {
public:
    // Fixed-point "wave" mapping phase -> [0..255]. Not constexpr because fp32_sin() uses std::sin internally.
    static uint8_t wave_fp(math::csFP32 phase) noexcept {
        using namespace math;
        static constexpr csFP32 half = csFP32::float_const(0.5f);
        static constexpr csFP32 scale255{255};

        const csFP32 s = fp32_sin(phase);
        const csFP32 norm = s * half + half;      // [-1..1] -> [0..1]
        const csFP32 scaled = norm * scale255;    // [0..255]
        int v = scaled.round_int();
        if (v < 0) v = 0;
        else if (v > 255) v = 255;
        return static_cast<uint8_t>(v);
    }

    void render(csMatrixPixels& matrix, csRandGen& /*rand*/, uint16_t currTime) const override {
        using namespace math;

        // Convert milliseconds to seconds in 16.16 fixed-point WITHOUT float:
        // t_raw = currTime * 65536 / 1000
        const int32_t t_raw = static_cast<int32_t>((static_cast<int64_t>(currTime) * FP32_SCALE) / 1000);
        const csFP32 t = csFP32::from_raw(t_raw);

        // Constants as constexpr fixed-point from float literals (compile-time).
        static constexpr csFP32 k08 = csFP32::float_const(0.7f);
        static constexpr csFP32 k06 = csFP32::float_const(0.5f);
        static constexpr csFP32 k04 = csFP32::float_const(0.3f);
        static constexpr csFP32 k05 = csFP32::float_const(0.4f);
        const int width = static_cast<int>(matrix.width());
        const int height = static_cast<int>(matrix.height());

        for (int y = 0; y < height; ++y) {
            const csFP32 yf{y};
            const csFP32 yf_scaled = yf * k04;
            for (int x = 0; x < width; ++x) {
                const csFP32 xf{x};
                const csFP32 xf_scaled = xf * k04;
                const uint8_t r = wave_fp(t * k08 + xf_scaled);
                const uint8_t g = wave_fp(t + yf_scaled);
                const uint8_t b = wave_fp(t * k06 + xf_scaled + yf_scaled * k05);
                matrix.setPixel(x, y, csColorRGBA{255, r, g, b});
            }
        }
    }
};

class csRenderPlasma final : public csRenderBase {
public:
    void render(csMatrixPixels& matrix, csRandGen& /*rand*/, uint16_t currTime) const override {
        const float t = static_cast<float>(currTime) * 0.0025f;
        const int width = static_cast<int>(matrix.width());
        const int height = static_cast<int>(matrix.height());

        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                const float xf = static_cast<float>(x);
                const float yf = static_cast<float>(y);
                const float v = std::sin(xf * 0.35f + t) + std::sin(yf * 0.35f - t) + std::sin((xf + yf) * 0.25f + t * 0.5f);
                const float norm = (v + 3.0f) / 6.0f; // bring into [0..1]
                const uint8_t r = static_cast<uint8_t>(norm * 255.0f);
                const uint8_t g = static_cast<uint8_t>((1.0f - norm) * 255.0f);
                const uint8_t b = static_cast<uint8_t>((0.5f + 0.5f * std::sin(t + xf * 0.1f)) * 255.0f);
                matrix.setPixel(x, y, csColorRGBA{255, r, g, b});
            }
        }
    }
};

} // namespace amp

