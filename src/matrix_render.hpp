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

class csEffectBase;
class csEventHandlerArgs;

// Generic property pointer type for render property introspection (WIP).
using tPropPtr = void*;

// Type alias for a pointer to a member function of csEffectBase with the same signature as receiveEvent
using tReceiverEventHandlerPtr = void (csEffectBase::*)(const csEventHandlerArgs&);

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
    // t_float = ??? - wip
    Bool = 9,
    Ptr = 10,
    // PROGMEM const
    StrConst = 11,
    Str = 12,
    // `csMatrixPixels`
    Matrix = 13,
    // `csRect`
    Rect = 14,
    // `csColorRGBA`
    Color = 15,

    // WIP ...
    /*

    Name -  описывает что контретно делает этот link (как он меняет prop).
    Указывает на структуру,
    структура содержит:
    - `csEffect * ptrEffect` - указатель на целевой эффект.
    - `propNum` - номер свойства целевого эффекта куда нужно писать (prop ID).
       смотри `getPropInfo(>> propNum << ...);`.
    - `PropType validTypeRO` - допустимый тип для указателя на csEffect. READ ONLY.
    например если это свойство имеет значение `PropType::UInt16`,
    то это значит что LinkToEffectProp может указывать только на свойства имеющие такой тип.
    на самом деле это не совсем так. точнее совсем не так.
    алгоритмы связывания делают умную работу,
    например все числа можно связывать между друг другом,
    просто в момент записи будет сделанно приведение типа.
    с EffectBase и подобными - там еще сложнее,
    учитывается реультат функции queryClassFamily.
    */
    LinkToEffectProp = 28,

    // WIP ...
    /*
    Генератор событий.
    **event emitter** -> event receiver.
    Имя - это текстовое описание события ().
    Указатель на `csEffect`.
    Он содержит указатель получателя события (или `nullptr`).
    которая содержит указатель на csEffect, и номер "EventRecvPoint",
    csEventBase.EventRecvPoint - это аргумент который передается в функцию
    которая получает события.
    EventRecvPoint передается в объекте csEventBase.
    */
    EventEmitterLinkToRecv = 29,
    // event emitter --(**Num**)--> event receiver.
    // Генератор событий - номер обработчика в приемнике.
    // Имя - должно отсутствовать, потомучто это свойство связанное с `EventEmitterLinkToRecv`.
    EventEmitterHandlerNum = 30,
    // event emitter -> **event receiver**
    // Read Only. Value type: UInt8.
    // Это номер приемника события.
    // Это особое свойство.
    // Множество этих свойств создает список список "EventRecvPoint".
    // числа которая содержиться в Value - это номер в `csEventBase.EventRecvPoint`,
    // этот номер является индификатором события.
    EventReceiverHandlerNum = 31,

    // Special:
    EffectBase = 32,
    EffectMatrixDest = 33,
    EffectPipe = 34,
    EffectPostFrame = 35,
    EffectGlyph = 36,
    EffectDigitalClock = 37,
    EffectUserArea = 64,
};

struct csPropInfo {
    // Type of the property value
    PropType valueType = PropType::None;
    // Pointer to the property value
    tPropPtr valuePtr = nullptr;
    // Name of the property
    const char* name = nullptr;
    // Detailed description
    const char* desc = nullptr;
    // Is the property read only?
    bool readOnly = false;
    // Is the property disabled?
    // `true` - property must be hidden from the user GUI.
    bool disabled = false;
};

// WIP...
// see: enum `PropType.LinkToEffectProp`
struct csPropLinkToProp {
    csEffectBase * effect;
    // Target property number
    uint8_t propNum;
    // READ ONLY. Valid type of the target property.
    PropType validTypeRO;
};

// WIP...
// see: enum `PropType.EventEmitterLinkToRecv`
class csEventHandlerArgs {
public:
    // WIP...

    // "Event number" in the `receiveEvent` function
    uint8_t eventNum;
    // random source
    csRandGen * rand;
    // time
    tTime currTime;
};

// see: enum `PropType.EventEmitterLinkToRecv`
struct csEventEmitterLinkToRecv {
    csEffectBase * eventReceiver;
    uint8_t eventNum;
    // READ ONLY. Valid class of the receiver.
    PropType validReceiverClass;
}

// Base class for all render/effect implementations
// Contains standard property types without the actual fields - only their property ID.
// All these properties are disabled by default - derived classes can enable them.
// This reserves a small pool of the most commonly used properties.
class csEffectBase {
public:

    virtual ~csEffectBase() = default;

    // Standard property constants:

    static constexpr uint8_t propClassName = 1;
    static constexpr uint8_t propMatrixDest = 2;
    static constexpr uint8_t propRectDest = 3;
    static constexpr uint8_t propRenderRectAutosize = 4;
    static constexpr uint8_t propDisabled = 5;
    // Scale property: 
    // increasing value (scale > 1.0) → larger scale → effect stretches → fewer details visible (like "zooming out");
    // decreasing value (scale < 1.0) → smaller scale → effect compresses → more details visible (like "zooming in").
    static constexpr uint8_t propScale = 6;
    static constexpr uint8_t propSpeed = 7;
    static constexpr uint8_t propAlpha = 8;
    static constexpr uint8_t propColor = 9;
    static constexpr uint8_t propColor2 = 10;
    static constexpr uint8_t propColor3 = 11;
    static constexpr uint8_t propColorBackground = 12;

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
        info.valuePtr = nullptr;    // No fields in this class
        info.desc = nullptr;

        switch (propNum) {
            case propClassName:
                info.valueType = PropType::StrConst;
                info.valuePtr = (tPropPtr)getClassName();
                info.name = "Class name";
                info.desc = "Name of the class";
                break;
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

    // Called after the entire frame is rendered; gives the effect access to the final matrix.
    // TODO: MOVE!!!
    virtual void onFrameDone(csMatrixPixels& frame, csRandGen& rand, tTime currTime) {
        (void)frame;
        (void)rand;
        (void)currTime;
    }

    // generate one event to external object
    bool sendEvent(csEventHandlerArgs& event) {
        (void)event;
        return false;
    }

    // Event receive hook
    // receive one event from external object
    virtual void receiveEvent(const csEventHandlerArgs& event) {
        (void)event;
    }

    // Get class name
    // see also: `propClassName`
    virtual const char* getClassName() const { return "csEffectBase"; }

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
                info.valuePtr = &renderRectAutosize;
                info.disabled = false;
                break;
            case propRectDest:
                info.valuePtr = &rectDest;
                info.disabled = false;
                break;
            case propMatrixDest:
                info.valuePtr = &matrix;
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
        if (!matrix) {
            return;
        }

        if (renderRectAutosize) {
            rectDest = matrix->getRect();
        }
    }
};

} // namespace amp

