#pragma once

#include <cmath>
#include <cstdint>

#include "color_rgba.hpp"
#include "matrix_pixels.hpp"
#include "cs_math.hpp"



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

    virtual bool getInt(uint8_t paramNum, uint32_t& value) const noexcept {
        (void)paramNum;
        (void)value;
        return false;
    }

    virtual void setInt(uint8_t paramNum, uint32_t value) noexcept {
        (void)paramNum;
        (void)value;
    }

    virtual void reset() noexcept {}
};

class csMatrixEvent {
    // Empty - WIP
};

class csMatrixRenderBase {
protected:
    csParamsBase* params_{nullptr};

    virtual csParamsBase* createParams() const { return nullptr; }

public:
    csMatrixRenderBase() : params_(createParams()) {}
    virtual ~csMatrixRenderBase() { delete params_; }

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
                          csMatrixEvent& event,
                          uint16_t eventNum) const {
        (void)led;
        (void)rand;
        (void)currTime;
        (void)event;
        (void)eventNum;
        return false;
    }

    // receive one event from external object
    virtual void receiveEvent(const csMatrixEvent& event) {
        (void)event;
    }
};

class GradientEffect final : public csMatrixRenderBase {
public:
    void render(csMatrixPixels& matrix, csRandGen& /*rand*/, uint16_t currTime) const override {
        using matrix_pixels_math::fp32;
        using matrix_pixels_math::FP32_FRAC_BITS;
        using matrix_pixels_math::float_to_fp32;
        using matrix_pixels_math::fp32_sin;
        using matrix_pixels_math::fp32_to_float;
        using matrix_pixels_math::int32_to_fp32;
        using matrix_pixels_math::fp32_mul;

        auto wave = [](fp32 phase) -> uint8_t {
            const float s = fp32_to_float(fp32_sin(phase));
            const float norm = s * 0.5f + 0.5f;
            const int val = static_cast<int>(norm * 255.0f + 0.5f);
            return static_cast<uint8_t>(val < 0 ? 0 : (val > 255 ? 255 : val));
        };

        const fp32 t = float_to_fp32(static_cast<float>(currTime) * 0.001f);
        const fp32 k08 = float_to_fp32(0.8f);
        const fp32 k06 = float_to_fp32(0.6f);
        const fp32 k04 = float_to_fp32(0.4f);
        const fp32 k05 = float_to_fp32(0.5f);

        const int width = static_cast<int>(matrix.width());
        const int height = static_cast<int>(matrix.height());

        for (int y = 0; y < height; ++y) {
            const fp32 yf = fp32_mul(int32_to_fp32(y), k04);
            for (int x = 0; x < width; ++x) {
                const fp32 xf = fp32_mul(int32_to_fp32(x), k04);
                const uint8_t r = wave(fp32_mul(t, k08) + xf);
                const uint8_t g = wave(t + yf);
                const uint8_t b = wave(fp32_mul(t, k06) + xf + fp32_mul(yf, k05));
                matrix.setPixel(x, y, csColorRGBA{255, r, g, b});
            }
        }
    }
};

class PlasmaEffect final : public csMatrixRenderBase {
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

