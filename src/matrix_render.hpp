#pragma once

#include <stdint.h>
#include <math.h>

#include "color_rgba.hpp"
#include "matrix_pixels.hpp"
#include "rect.hpp"
#include "math.hpp"
#include "rand_gen.hpp"



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
    Bool = 9,
    Ptr = 10,
    StrConst = 11,
    Str = 12,
    // `csMatrixPixels`
    Matrix = 13,
    // `csRect`
    Rect = 14,
    // `csColorRGBA`
    Color = 15,
    // WIP ...
    LinkToEffectParam = 16,
    // `ParamType`
    LinkToEffectParamType = 17,

    EffectBase = 32,
    EffectMatrixDest = 33,
    EffectPipe = 34,
    EffectGlyph = 35,
    EffectUserArea = 64,
};

struct csParamInfo {
    ParamType type = ParamType::None;
    paramPtr ptr = nullptr;
    const char* name = nullptr;
    bool readOnly = false;
    bool disabled = false;
};

class csEventBase {
    // Empty - WIP
};

// Base class for all render/effect implementations
// Contains standard parameter types without the actual fields - only their parameter ID.
// All these parameters are disabled by default - derived classes can enable them.
// This reserves a small pool of the most commonly used properties.
class csEffectBase {
public:

    virtual ~csEffectBase() = default;

    // Standard parameter constants
    static constexpr uint8_t paramMatrixDest = 1;
    static constexpr uint8_t paramRectDest = 2;
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

    // `paramLast` - Special "parameter" - the last one in the list. 
    // Shadowed in each derived class.
    // Example of use:
    // ```
    //     static constexpr uint8_t base = _prev_class_::paramLast;
    //     static constexpr uint8_t paramNew = base+1;
    //     static constexpr uint8_t paramLast = paramNew;
    // ```
    static constexpr uint8_t paramLast = paramColorBackground;

    // Parameter introspection: returns number of exposed parameters
    // NOTE: parameter indices start at 1 (not 0).
    // If `getParamsCount()==1`, call `getParamInfo(1, ...)`.
    virtual uint8_t getParamsCount() const {
        return paramLast;
    }

    // Parameter introspection: returns parameter metadata (type/name/pointer)
    // NOTE: parameter indices start at 1 (not 0).
    virtual void getParamInfo(uint8_t paramNum, csParamInfo& info) {
        info.readOnly = false;
        info.disabled = true;  // All parameters are disabled by default
        info.ptr = nullptr;    // No fields in this class

        switch (paramNum) {
            case paramMatrixDest:
                info.type = ParamType::Matrix;
                info.name = "Matrix dest";
                break;
            case paramRectDest:
                info.type = ParamType::Rect;
                info.name = "Rect dest";
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
                info = csParamInfo{};
                break;
        }
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

    // Class family identification system (replacement for dynamic_cast without RTTI)
    //
    // PURPOSE:
    // This system allows type checking and safe downcasting without RTTI (Runtime Type Information).
    // Arduino compilers use -fno-rtti flag which disables RTTI to save memory, making dynamic_cast unavailable.
    // This mechanism provides type-safe casting for effect relationships (e.g., checking if an effect
    // is a csRenderGlyph* when setting up csRenderDigitalClock::renderDigit parameter).
    //
    // HOW IT WORKS:
    // Each class family (e.g., EffectMatrixDest, EffectGlyph, EffectPipe) has a unique ParamType identifier.
    // Derived classes override getClassFamily() to return their family ID, and queryClassFamily() to
    // check if the object belongs to a requested family (including base class families via chain of calls).
    //
    // USAGE EXAMPLE:
    //   // Instead of: if (auto* m = dynamic_cast<csRenderMatrixBase*>(eff))
    //   if (auto* m = static_cast<csRenderMatrixBase*>(
    //       eff->queryClassFamily(ParamType::EffectMatrixDest)
    //   )) {
    //       m->setMatrix(matrix);
    //   }
    //
    // Get class family identifier
    // Returns ParamType value that identifies the class family
    // Base implementation returns EffectBase
    virtual ParamType getClassFamily() const {
        return ParamType::EffectBase;
    }

    // Query if object belongs to a specific class family
    // Returns pointer to this object if it belongs to requested family, nullptr otherwise
    // Works without RTTI by checking class family hierarchy
    // Derived classes should override to check their own family and call base class implementation
    virtual void* queryClassFamily(ParamType familyId) {
        if (familyId == getClassFamily()) {
            return this;
        }
        return nullptr; // Base class doesn't belong to any other family
    }

};

// Base class for matrix-based renderers.
// All fields are public by design for direct access/performance.
class csRenderMatrixBase : public csEffectBase {
public:
    csMatrixPixels* matrix = nullptr;

    csRect rectDest;

    bool renderRectAutosize = true;

    bool disabled = false;

    virtual ~csRenderMatrixBase() = default;

    void setMatrix(csMatrixPixels* m) noexcept {
        matrix = m;
        paramChanged(paramMatrixDest);
    }
    void setMatrix(csMatrixPixels& m) noexcept {
        matrix = &m;
        paramChanged(paramMatrixDest);
    }

    // Class family identifier
    static constexpr ParamType ClassFamilyId = ParamType::EffectMatrixDest;

    // Override to return matrix-based renderer family
    ParamType getClassFamily() const override {
        return ClassFamilyId;
    }

    // Override to check for matrix-based renderer family
    void* queryClassFamily(ParamType familyId) override {
        if (familyId == ClassFamilyId) {
            return this;
        }
        // Check base class families
        return csEffectBase::queryClassFamily(familyId);
    }

    // NOTE: uint8_t getParamsCount() - count dont changed

    void getParamInfo(uint8_t paramNum, csParamInfo& info) override {
        csEffectBase::getParamInfo(paramNum, info);

        switch (paramNum) {
            case paramRenderRectAutosize:
                info.ptr = &renderRectAutosize;
                info.disabled = false;
                break;
            case paramRectDest:
                info.ptr = &rectDest;
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
            rectDest = matrix->getRect();
        }
    }
};

} // namespace amp

