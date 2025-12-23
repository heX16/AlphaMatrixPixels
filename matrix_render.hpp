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
class csRenderBase {
public:

    virtual ~csRenderBase() = default;

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

// Base class for matrix-based renderers.
// All fields are public by design for direct access/performance.
class csRenderMatrixBase : public csRenderBase {
public:
    csMatrixPixels* matrix = nullptr;

    csRect rect;

    bool renderRectAutosize = true;

    virtual ~csRenderMatrixBase() = default;

    void setMatrix(csMatrixPixels* m) noexcept {
        matrix = m;
        updateRenderRect();
    }
    void setMatrix(csMatrixPixels& m) noexcept {
        matrix = &m;
        updateRenderRect();
    }

    static constexpr uint8_t paramRenderRectAutosize = 3;
    static constexpr uint8_t paramRenderRect = 1;
    static constexpr uint8_t paramMatrixDest = 2;

    uint8_t getParamsCount() const override {
        return 3;
    }

    void getParamInfo(uint8_t paramNum, csParamInfo& info) override {
        info.readOnly = false;
        info.disabled = false;
        switch (paramNum) {
            case paramRenderRectAutosize:
                info.type = ParamType::Bool;
                info.name = "Render rect autosize";
                info.ptr = &renderRectAutosize;
                break;
            case paramRenderRect:
                info.type = ParamType::Rect;
                info.name = "Render rect";
                info.ptr = &rect;
                break;
            case paramMatrixDest:
                info.type = ParamType::Matrix;
                info.name = "Matrix dest";
                info.ptr = &matrix;
                break;
            default:
                info.type = ParamType::None;
                info.name = nullptr;
                info.ptr = nullptr;
                break;
        }
    }

    virtual void paramChanged(uint8_t paramNum) {
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

    void updateRenderRect() {
        if (!matrix) {
            return;
        }

        if (renderRectAutosize) {
            rect = matrix->getRect();
        }
    }
};

} // namespace amp

