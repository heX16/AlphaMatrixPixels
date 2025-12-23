#pragma once

#include <stdint.h>
#include <math.h>

#include "color_rgba.hpp"
#include "matrix_pixels.hpp"
#include "rect.hpp"
#include "math.hpp"



namespace amp {

// Generic parameter pointer type for render parameter introspection (WIP).
using paramPtr = void*;

// Parameter type for render parameter introspection (WIP).
enum class ParamType : uint8_t {
    None = 0,
    UInt8 = 1,
    UInt16 = 2,
    UInt32 = 3,
    Int8 = 4,
    Int16 = 5,
    Int32 = 6,
    FP16 = 7,
    FP32 = 8,
    Str = 9,
    Bool = 10,
    Ptr = 11,
    Matrix = 12,
    Rect = 13
};

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


class csEventBase {
    // Empty - WIP
};

class csRenderBase {
public:

    virtual ~csRenderBase() = default;

    virtual uint8_t getParamsCount() const {
        return 0;
    }

    virtual void getParamInfo(uint8_t paramNum,
                              ParamType& paramType,
                              const char*& paramName,
                              paramPtr& ptr) {
        (void)paramNum;
        paramType = ParamType::None;
        paramName = nullptr;
        ptr = nullptr;
    }

    virtual void recalc(csRandGen& rand, uint16_t currTime) {
        (void)rand;
        (void)currTime;
    }

    virtual void render(csRandGen& rand, uint16_t currTime) const {
        (void)rand;
        (void)currTime;
    }

    // generate one event to external object
    virtual bool getEvent(csRandGen& rand,
                          uint16_t currTime,
                          csEventBase& event,
                          uint16_t eventNum) const {
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

class csRenderMatrixBase : public csRenderBase {
protected:
    csMatrixPixels* matrix = nullptr;

    csRect rect;
public:
    virtual ~csRenderMatrixBase() = default;

    void setMatrix(csMatrixPixels* m) noexcept { matrix = m; }
    void setMatrix(csMatrixPixels& m) noexcept { matrix = &m; }

    static constexpr uint8_t paramRenderRect = 1;
    static constexpr uint8_t paramMatrixDest = 2;

    uint8_t getParamsCount() const override {
        return 2;
    }

    void getParamInfo(uint8_t paramNum,
                      ParamType& paramType,
                      const char*& paramName,
                      paramPtr& ptr) override {
        switch (paramNum) {
            case 1:
                paramType = ParamType::Rect;
                paramName = "Render rect";
                ptr = &rect;
                break;
            case 2:
                paramType = ParamType::Matrix;
                paramName = "Matrix dest";
                ptr = &matrix;
                break;
            default:
                paramType = ParamType::None;
                paramName = nullptr;
                ptr = nullptr;
                break;
        }
    }
};


class csRenderGradient final : public csRenderMatrixBase {
public:
    void render(csRandGen& /*rand*/, uint16_t currTime) const override {
        if (!matrix) {
            return;
        }
        const float t = static_cast<float>(currTime) * 0.001f;

        auto wave = [](float v) -> uint8_t {
            return static_cast<uint8_t>((sin(v) * 0.5f + 0.5f) * 255.0f);
        };

        const auto width = matrix->width();
        const auto height = matrix->height();

        for (tMatrixPixelsSize y = 0; y < height; ++y) {
            for (tMatrixPixelsSize x = 0; x < width; ++x) {
                const float xf = static_cast<float>(x) * 0.4f;
                const float yf = static_cast<float>(y) * 0.4f;
                const uint8_t r = wave(t * 0.8f + xf);
                const uint8_t g = wave(t * 1.0f + yf);
                const uint8_t b = wave(t * 0.6f + xf + yf * 0.5f);
                matrix->setPixel(x, y, csColorRGBA{255, r, g, b});
            }
        }
    }
};

class csRenderGradientFP final : public csRenderMatrixBase {
public:
    // Fixed-point "wave" mapping phase -> [0..255]. Not constexpr because fp32_sin() uses sin() internally.
    static uint8_t wave_fp(math::csFP32 phase) noexcept {
        using namespace math;
        static const csFP32 half = csFP32::float_const(0.5f);
        static const csFP32 scale255{255};

        const csFP32 s = fp32_sin(phase);
        const csFP32 norm = s * half + half;      // [-1..1] -> [0..1]
        const csFP32 scaled = norm * scale255;    // [0..255]
        int v = scaled.round_int();
        if (v < 0) v = 0;
        else if (v > 255) v = 255;
        return static_cast<uint8_t>(v);
    }

    void render(csRandGen& /*rand*/, uint16_t currTime) const override {
        if (!matrix) {
            return;
        }
        using namespace math;

        // Convert milliseconds to seconds in 16.16 fixed-point WITHOUT float:
        // t_raw = currTime * 65536 / 1000
        const int32_t t_raw = static_cast<int32_t>((static_cast<int64_t>(currTime) * csFP32::scale) / 1000);
        const csFP32 t = csFP32::from_raw(t_raw);

        // Fixed-point constants.
        static const csFP32 k08 = csFP32::float_const(0.7f);
        static const csFP32 k06 = csFP32::float_const(0.5f);
        static const csFP32 k04 = csFP32::float_const(0.3f);
        static const csFP32 k05 = csFP32::float_const(0.4f);
        const auto width = matrix->width();
        const auto height = matrix->height();

        for (tMatrixPixelsSize y = 0; y < height; ++y) {
            const csFP32 yf = csFP32::from_int(y);
            const csFP32 yf_scaled = yf * k04;
            for (tMatrixPixelsSize x = 0; x < width; ++x) {
                const csFP32 xf = csFP32::from_int(x);
                const csFP32 xf_scaled = xf * k04;
                const uint8_t r = wave_fp(t * k08 + xf_scaled);
                const uint8_t g = wave_fp(t + yf_scaled);
                const uint8_t b = wave_fp(t * k06 + xf_scaled + yf_scaled * k05);
                matrix->setPixel(x, y, csColorRGBA{255, r, g, b});
            }
        }
    }
};

class csRenderPlasma final : public csRenderMatrixBase {
public:
    void render(csRandGen& /*rand*/, uint16_t currTime) const override {
        if (!matrix) {
            return;
        }
        const float t = static_cast<float>(currTime) * 0.0025f;
        const auto width = matrix->width();
        const auto height = matrix->height();

        for (tMatrixPixelsSize y = 0; y < height; ++y) {
            for (tMatrixPixelsSize x = 0; x < width; ++x) {
                const float xf = static_cast<float>(x);
                const float yf = static_cast<float>(y);
                const float v = sin(xf * 0.35f + t) + sin(yf * 0.35f - t) + sin((xf + yf) * 0.25f + t * 0.5f);
                const float norm = (v + 3.0f) / 6.0f; // bring into [0..1]
                const uint8_t r = static_cast<uint8_t>(norm * 255.0f);
                const uint8_t g = static_cast<uint8_t>((1.0f - norm) * 255.0f);
                const uint8_t b = static_cast<uint8_t>((0.5f + 0.5f * sin(t + xf * 0.1f)) * 255.0f);
                matrix->setPixel(x, y, csColorRGBA{255, r, g, b});
            }
        }
    }
};

} // namespace amp

