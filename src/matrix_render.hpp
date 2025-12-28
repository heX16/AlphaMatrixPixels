#pragma once

#include <stdint.h>
#include <math.h>

#include "color_rgba.hpp"
#include "matrix_pixels.hpp"
#include "matrix_types.hpp"
#include "rect.hpp"
#include "math.hpp"
#include "rand_gen.hpp"



namespace amp {

// Generic property pointer type for render property introspection (WIP).
using propPtr = void*;

// Property type for render property introspection (WIP).
enum class PropType : uint8_t {
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
    LinkToEffectProp = 16,
    // `PropType`
    LinkToEffectPropType = 17,

    EffectBase = 32,
    EffectMatrixDest = 33,
    EffectPipe = 34,
    EffectGlyph = 35,
    EffectUserArea = 64,
};

struct csPropInfo {
    PropType type = PropType::None;
    propPtr ptr = nullptr;
    const char* name = nullptr;
    bool readOnly = false;
    bool disabled = false;
};

class csEventBase {
    // Empty - WIP
};

// Base class for all render/effect implementations
// Contains standard property types without the actual fields - only their property ID.
// All these properties are disabled by default - derived classes can enable them.
// This reserves a small pool of the most commonly used properties.
class csEffectBase {
public:

    virtual ~csEffectBase() = default;

    // Standard property constants
    static constexpr uint8_t propMatrixDest = 1;
    static constexpr uint8_t propRectDest = 2;
    static constexpr uint8_t propRenderRectAutosize = 3;
    static constexpr uint8_t propDisabled = 4;
    // Scale property: 
    // increasing value (scale > 1.0) → larger scale → effect stretches → fewer details visible (like "zooming out");
    // decreasing value (scale < 1.0) → smaller scale → effect compresses → more details visible (like "zooming in").
    static constexpr uint8_t propScale = 5;
    static constexpr uint8_t propSpeed = 6;
    static constexpr uint8_t propAlpha = 7;
    static constexpr uint8_t propColor = 8;
    static constexpr uint8_t propColor2 = 9;
    static constexpr uint8_t propColor3 = 10;
    static constexpr uint8_t propColorBackground = 11;

    // `propLast` - Special "property" - the last one in the list. 
    // Shadowed in each derived class.
    // Example of use:
    // ```
    //     static constexpr uint8_t base = _prev_class_::propLast;
    //     static constexpr uint8_t propNew = base+1;
    //     static constexpr uint8_t propLast = propNew;
    // ```
    static constexpr uint8_t propLast = propColorBackground;

    // Property introspection: returns number of exposed properties
    // NOTE: property indices start at 1 (not 0).
    // If `getPropsCount()==1`, call `getPropInfo(1, ...)`.
    virtual uint8_t getPropsCount() const {
        return propLast;
    }

    // Property introspection: returns property metadata (type/name/pointer)
    // NOTE: property indices start at 1 (not 0).
    virtual void getPropInfo(uint8_t propNum, csPropInfo& info) {
        info.readOnly = false;
        info.disabled = true;  // All properties are disabled by default
        info.ptr = nullptr;    // No fields in this class

        switch (propNum) {
            case propMatrixDest:
                info.type = PropType::Matrix;
                info.name = "Matrix dest";
                break;
            case propRectDest:
                info.type = PropType::Rect;
                info.name = "Rect dest";
                break;
            case propRenderRectAutosize:
                info.type = PropType::Bool;
                info.name = "Render rect autosize";
                break;
            case propDisabled:
                info.type = PropType::Bool;
                info.name = "Disabled";
                break;
            case propScale:
                info.type = PropType::FP16;
                info.name = "Scale";
                break;
            case propSpeed:
                info.type = PropType::FP16;
                info.name = "Speed";
                break;
            case propAlpha:
                info.type = PropType::UInt8;
                info.name = "Alpha";
                break;
            case propColor:
                info.type = PropType::Color;
                info.name = "Color";
                break;
            case propColor2:
                info.type = PropType::Color;
                info.name = "Color 2";
                break;
            case propColor3:
                info.type = PropType::Color;
                info.name = "Color 3";
                break;
            case propColorBackground:
                info.type = PropType::Color;
                info.name = "Background color";
                break;
            default:
                info = csPropInfo{};
                break;
        }
    }

    // Notification hook.
    // Must be called when a property value changes
    virtual void propChanged(uint8_t propNum) {
        (void)propNum;
    }

    // Precomputation step
    virtual void recalc(csRandGen& rand, tTime currTime) {
        (void)rand;
        (void)currTime;
    }

    // Render one frame
    virtual void render(csRandGen& rand, tTime currTime) const {
        (void)rand;
        (void)currTime;
    }

    // Event generation hook
    // generate one event to external object
    virtual bool getEvent(csRandGen& rand,
                          tTime currTime,
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
    // is a csRenderGlyph* when setting up csRenderDigitalClock::renderDigit property).
    //
    // HOW IT WORKS:
    // Each class family (e.g., EffectMatrixDest, EffectGlyph, EffectPipe) has a unique PropType identifier.
    // Derived classes override getClassFamily() to return their family ID, and queryClassFamily() to
    // check if the object belongs to a requested family (including base class families via chain of calls).
    //
    // USAGE EXAMPLE:
    //   // Instead of: if (auto* m = dynamic_cast<csRenderMatrixBase*>(eff))
    //   if (auto* m = static_cast<csRenderMatrixBase*>(
    //       eff->queryClassFamily(PropType::EffectMatrixDest)
    //   )) {
    //       m->setMatrix(matrix);
    //   }
    //
    // Get class family identifier
    // Returns PropType value that identifies the class family
    // Base implementation returns EffectBase
    virtual PropType getClassFamily() const {
        return PropType::EffectBase;
    }

    // Query if object belongs to a specific class family
    // Returns pointer to this object if it belongs to requested family, nullptr otherwise
    // Works without RTTI by checking class family hierarchy
    // Derived classes should override to check their own family and call base class implementation
    virtual void* queryClassFamily(PropType familyId) {
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
        propChanged(propMatrixDest);
    }
    void setMatrix(csMatrixPixels& m) noexcept {
        matrix = &m;
        propChanged(propMatrixDest);
    }

    // Class family identifier
    static constexpr PropType ClassFamilyId = PropType::EffectMatrixDest;

    // Override to return matrix-based renderer family
    PropType getClassFamily() const override {
        return ClassFamilyId;
    }

    // Override to check for matrix-based renderer family
    void* queryClassFamily(PropType familyId) override {
        if (familyId == ClassFamilyId) {
            return this;
        }
        // Check base class families
        return csEffectBase::queryClassFamily(familyId);
    }

    // NOTE: uint8_t getPropsCount() - count dont changed

    void getPropInfo(uint8_t propNum, csPropInfo& info) override {
        csEffectBase::getPropInfo(propNum, info);

        switch (propNum) {
            case propRenderRectAutosize:
                info.ptr = &renderRectAutosize;
                info.disabled = false;
                break;
            case propRectDest:
                info.ptr = &rectDest;
                info.disabled = false;
                break;
            case propMatrixDest:
                info.ptr = &matrix;
                info.disabled = false;
                break;
            case propDisabled:
                info.ptr = &disabled;
                info.disabled = false;
                break;
            default:
                break;
        }
    }

    void propChanged(uint8_t propNum) override {
        switch (propNum) {
            case propRenderRectAutosize:
            case propMatrixDest:
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

