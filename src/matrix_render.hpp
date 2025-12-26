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
    Rect = 13,
    Color
};

struct csParamInfo {
    ParamType type = ParamType::None;
    paramPtr ptr = nullptr;
    const char* name = nullptr;
    bool readOnly = false;
    bool disabled = false;
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

class csEventBase {
    // Empty - WIP
};

// Base class for all render/effect implementations
class csEffectBase {
public:

    virtual ~csEffectBase() = default;

    // Parameter introspection: returns number of exposed parameters
    // NOTE: parameter indices start at 1 (not 0).
    // If `getParamsCount()==1`, call `getParamInfo(1, ...)`.
    virtual uint8_t getParamsCount() const {
        return 0;
    }

    // Parameter introspection: returns parameter metadata (type/name/pointer)
    // NOTE: parameter indices start at 1 (not 0).
    virtual void getParamInfo(uint8_t paramNum, csParamInfo& info) {
        (void)paramNum;
        info = csParamInfo{};
    }

    // Notification hook.
    // Must be called when a parameter value changes
    virtual void paramChanged(uint8_t paramNum) {
        (void)paramNum;
    }

    // Precomputation step
    virtual void recalc(csRandGen& rand, uint16_t currTime) {
        (void)rand;
        (void)currTime;
    }

    // Render one frame
    virtual void render(csRandGen& rand, uint16_t currTime) const {
        (void)rand;
        (void)currTime;
    }

    // Event generation hook
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

    // Event receive hook
    // receive one event from external object
    virtual void receiveEvent(const csEventBase& event) {
        (void)event;
    }
};

// Contains standard parameter types without the actual fields - only their parameter ID.
// All these parameters are disabled by default - derived classes can enable them.
// This reserves a small pool of the most commonly used properties.
class csEffectBaseStdParams : public csEffectBase {
public:

    // Standard parameter constants
    static constexpr uint8_t paramMatrixDest = 1;
    static constexpr uint8_t paramRenderRect = 2;
    static constexpr uint8_t paramRenderRectAutosize = 3;
    static constexpr uint8_t paramDisabled = 4;
    // Scale parameter: 
    // increasing value (scale > 1.0) → larger scale → effect stretches → fewer details visible (like "zooming out");
    // decreasing value (scale < 1.0) → smaller scale → effect compresses → more details visible (like "zooming in").
    static constexpr uint8_t paramScale = 5;
    static constexpr uint8_t paramSpeed = 6;
    static constexpr uint8_t paramAlpha = 7;
    static constexpr uint8_t paramColor = 8;
    static constexpr uint8_t paramColor2 = 9;
    static constexpr uint8_t paramColor3 = 10;
    static constexpr uint8_t paramColorBackground = 11;

    // Special "parameter" - the last one in the list. Shadowed in each derived class.
    // Example:
    // ```
    //     static constexpr uint8_t base = _prev_class_::paramLast;
    //     static constexpr uint8_t paramNew = base+1;
    //     static constexpr uint8_t paramLast = paramNew;
    // ```
    static constexpr uint8_t paramLast = paramColorBackground;
    
    
    uint8_t getParamsCount() const override {
        return paramLast;
    }

    void getParamInfo(uint8_t paramNum, csParamInfo& info) override {
        info.readOnly = false;
        info.disabled = true;  // All parameters are disabled by default
        info.ptr = nullptr;    // No fields in this class

        switch (paramNum) {
            case paramMatrixDest:
                info.type = ParamType::Matrix;
                info.name = "Matrix dest";
                break;
            case paramRenderRect:
                info.type = ParamType::Rect;
                info.name = "Render rect";
                break;
            case paramRenderRectAutosize:
                info.type = ParamType::Bool;
                info.name = "Render rect autosize";
                break;
            case paramDisabled:
                info.type = ParamType::Bool;
                info.name = "Disabled";
                break;
            case paramScale:
                info.type = ParamType::FP16;
                info.name = "Scale";
                break;
            case paramSpeed:
                info.type = ParamType::FP16;
                info.name = "Speed";
                break;
            case paramAlpha:
                info.type = ParamType::UInt8;
                info.name = "Alpha";
                break;
            case paramColor:
                info.type = ParamType::Color;
                info.name = "Color";
                break;
            case paramColor2:
                info.type = ParamType::Color;
                info.name = "Color 2";
                break;
            case paramColor3:
                info.type = ParamType::Color;
                info.name = "Color 3";
                break;
            case paramColorBackground:
                info.type = ParamType::Color;
                info.name = "Background color";
                break;
            default:
                csEffectBase::getParamInfo(paramNum, info);
                break;
        }
    }

};

// Base class for matrix-based renderers.
// All fields are public by design for direct access/performance.
class csRenderMatrixBase : public csEffectBaseStdParams {
public:
    csMatrixPixels* matrix = nullptr;

    csRect rect;

    bool renderRectAutosize = true;

    bool disabled = false;

    virtual ~csRenderMatrixBase() = default;

    void setMatrix(csMatrixPixels* m) noexcept {
        matrix = m;
        paramChanged(paramMatrixDest);
        paramChanged(paramRenderRect);
    }
    void setMatrix(csMatrixPixels& m) noexcept {
        matrix = &m;
        paramChanged(paramMatrixDest);
        paramChanged(paramRenderRect);
    }

    // NOTE: uint8_t getParamsCount() - count dont changed

    void getParamInfo(uint8_t paramNum, csParamInfo& info) override {
        csEffectBaseStdParams::getParamInfo(paramNum, info);

        switch (paramNum) {
            case paramRenderRectAutosize:
                info.ptr = &renderRectAutosize;
                info.disabled = false;
                break;
            case paramRenderRect:
                info.ptr = &rect;
                info.disabled = false;
                break;
            case paramMatrixDest:
                info.ptr = &matrix;
                info.disabled = false;
                break;
            case paramDisabled:
                info.ptr = &disabled;
                info.disabled = false;
                break;
            default:
                break;
        }
    }

    void paramChanged(uint8_t paramNum) override {
        switch (paramNum) {
            case paramRenderRectAutosize:
            case paramMatrixDest:
                if (renderRectAutosize) {
                    updateRenderRect();
                }
                break;
            default:
                break;
        }
    }

protected:
    virtual void updateRenderRect() {
        if (!matrix) {
            return;
        }

        if (renderRectAutosize) {
            rect = matrix->getRect();
        }
    }
};

} // namespace amp

