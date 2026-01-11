#pragma once

#include <stdint.h>
#include "matrix_types.hpp"

// Forward declarations for types that depend on these base types
namespace amp {
    class csEffectBase;
    class csRandGen;
}

namespace amp {

// Generic property pointer type for render property introspection (WIP).
using tPropPtr = void*;

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
    PtrConst = 10,
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
    ClassBase = 25,
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

// Base class for all classes in the system
// Contains fundamental introspection, class family, and event methods
class csBase {
public:
    virtual ~csBase() = default;

    // Standard property constants:
    static constexpr uint8_t propClassName = 1;
    static constexpr uint8_t propLast = propClassName;

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
                info.disabled = false;
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

    // Get class name
    // see also: `propClassName`
    virtual const char* getClassName() const { return "csBase"; }

    // Class family identification system (replacement for dynamic_cast without RTTI)
    //
    // PURPOSE:
    // This system allows type checking and safe downcasting without RTTI (Runtime Type Information).
    // Arduino compilers use -fno-rtti flag which disables RTTI to save memory, making dynamic_cast unavailable.
    //
    // Get class family identifier
    // Returns PropType value that identifies the class family
    // Base implementation returns ClassBase
    virtual PropType getClassFamily() const {
        return PropType::ClassBase;
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
};

} // namespace amp
