#pragma once

#include <stdint.h>
#include <math.h>

#include "color_rgba.hpp"
#include "matrix_pixels.hpp"
#include "matrix_types.hpp"
#include "rect.hpp"
#include "math.hpp"
#include "rand_gen.hpp"
#include "amp_class_base.hpp"

namespace amp {

class csEffectBase;

// Type alias for a pointer to a member function of csEffectBase with the same signature as receiveEvent
using tReceiverEventHandlerPtr = void (csEffectBase::*)(const csEventHandlerArgs&);

// WIP...
// see: enum `PropType.LinkToEffectProp`
struct csPropLinkToProp {
    csEffectBase * effect;
    // Target property number
    uint8_t propNum;
    // READ ONLY. Valid type of the target property.
    PropType validTypeRO;
};

// see: enum `PropType.EventEmitterLinkToRecv`
struct csEventEmitterLinkToRecv {
    csEffectBase * eventReceiver;
    uint8_t eventNum;
    // READ ONLY. Valid class of the receiver.
    PropType validReceiverClass;
};

// Base class for all render/effect implementations
// Contains standard property types without the actual fields - only their property ID.
// All these properties are disabled by default - derived classes can enable them.
// This reserves a small pool of the most commonly used properties.
class csEffectBase : public csBase {
public:

    virtual ~csEffectBase() = default;

    // Standard property constants:
    static constexpr uint8_t base = csBase::propLast;

    static constexpr uint8_t propMatrixDest = base + 1;
    static constexpr uint8_t propRectDest = base + 2;
    static constexpr uint8_t propRenderRectAutosize = base + 3;
    static constexpr uint8_t propDisabled = base + 4;
    // Scale property: 
    // increasing value (scale > 1.0) → larger scale → effect stretches → fewer details visible (like "zooming out");
    // decreasing value (scale < 1.0) → smaller scale → effect compresses → more details visible (like "zooming in").
    static constexpr uint8_t propScale = base + 5;
    static constexpr uint8_t propSpeed = base + 6;
    static constexpr uint8_t propAlpha = base + 7;
    static constexpr uint8_t propColor = base + 8;
    static constexpr uint8_t propColor2 = base + 9;
    static constexpr uint8_t propColor3 = base + 10;
    static constexpr uint8_t propColorBackground = base + 11;
    static constexpr uint8_t propMatrixSource = base + 12;
    static constexpr uint8_t propRectSource = base + 13;
    static constexpr uint8_t propRewrite = base + 14;

    // `propLast` - Special "property" - the last one in the list. 
    // Shadowed in each derived class.
    // Example of use:
    // ```
    //     static constexpr uint8_t base = _prev_class_::propLast;
    //     static constexpr uint8_t propNew = base+1;
    //     static constexpr uint8_t propLast = propNew;
    // ```
    static constexpr uint8_t propLast = propRewrite;

    // Property introspection: returns number of exposed properties
    // NOTE: property indices start at 1 (not 0).
    // If `getPropsCount()==1`, call `getPropInfo(1, ...)`.
    virtual uint8_t getPropsCount() const override {
        return propLast;
    }

    // Property introspection: returns property metadata (type/name/pointer)
    // NOTE: property indices start at 1 (not 0).
    virtual void getPropInfo(uint8_t propNum, csPropInfo& info) override {
        info.readOnly = false;
        info.disabled = true;  // All properties are disabled by default
        info.valuePtr = nullptr;    // No fields in this class
        info.desc = nullptr;

        switch (propNum) {
            case propMatrixDest:
                info.valueType = PropType::Matrix;
                info.name = "Matrix dest";
                break;
            case propRectDest:
                info.valueType = PropType::Rect;
                info.name = "Rect dest";
                break;
            case propRenderRectAutosize:
                info.valueType = PropType::Bool;
                info.name = "Render rect autosize";
                break;
            case propDisabled:
                info.valueType = PropType::Bool;
                info.name = "Disabled";
                break;
            case propScale:
                info.valueType = PropType::FP16;
                info.name = "Scale";
                break;
            case propSpeed:
                info.valueType = PropType::FP16;
                info.name = "Speed";
                break;
            case propAlpha:
                info.valueType = PropType::UInt8;
                info.name = "Alpha";
                break;
            case propColor:
                info.valueType = PropType::Color;
                info.name = "Color";
                break;
            case propColor2:
                info.valueType = PropType::Color;
                info.name = "Color 2";
                break;
            case propColor3:
                info.valueType = PropType::Color;
                info.name = "Color 3";
                break;
            case propColorBackground:
                info.valueType = PropType::Color;
                info.name = "Background color";
                break;
            case propMatrixSource:
                info.valueType = PropType::Matrix;
                info.name = "Matrix source";
                break;
            case propRectSource:
                info.valueType = PropType::Rect;
                info.name = "Rect source";
                break;
            case propRewrite:
                info.valueType = PropType::Bool;
                info.name = "Rewrite";
                break;
            default:
                csBase::getPropInfo(propNum, info);
                break;
        }
    }

    // Notification hook.
    // Must be called when a property value changes
    virtual void propChanged(uint8_t propNum) override {
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

    // Called after the entire frame is rendered; gives the effect access to the final matrix.
    // TODO: MOVE!!!
    virtual void onFrameDone(csMatrixPixels& frame, csRandGen& rand, tTime currTime) {
        (void)frame;
        (void)rand;
        (void)currTime;
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
    //       m->setMatrix(matrixDest);
    //   }
    //
    // Get class family identifier
    // Returns PropType value that identifies the class family
    // Base implementation returns EffectBase
    virtual PropType getClassFamily() const override {
        return PropType::EffectBase;
    }

    // Query if object belongs to a specific class family
    // Returns pointer to this object if it belongs to requested family, nullptr otherwise
    // Works without RTTI by checking class family hierarchy
    // Derived classes should override to check their own family and call base class implementation
    virtual void* queryClassFamily(PropType familyId) override {
        if (familyId == getClassFamily()) {
            return this;
        }
        // Check base class families
        return csBase::queryClassFamily(familyId);
    }

};

// Base class for matrix-based renderers.
// All fields are public by design for direct access/performance.
class csRenderMatrixBase : public csEffectBase {
public:
    csMatrixPixels* matrixDest = nullptr;

    csRect rectDest;

    bool renderRectAutosize = true;

    bool disabled = false;

    virtual ~csRenderMatrixBase() = default;

    void setMatrix(csMatrixPixels* m) noexcept {
        matrixDest = m;
        propChanged(propMatrixDest);
    }
    void setMatrix(csMatrixPixels& m) noexcept {
        matrixDest = &m;
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
                info.valuePtr = &renderRectAutosize;
                info.disabled = false;
                break;
            case propRectDest:
                info.valuePtr = &rectDest;
                info.disabled = false;
                break;
            case propMatrixDest:
                info.valuePtr = &matrixDest;
                info.disabled = false;
                break;
            case propDisabled:
                info.valuePtr = &disabled;
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
        if (!matrixDest) {
            return;
        }

        if (renderRectAutosize) {
            rectDest = matrixDest->getRect();
        }
    }
};

} // namespace amp

