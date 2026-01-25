#pragma once



///////////////////////////////////////////////////////

/*
Compile-time keyword hook for FUNCTIONS.

Default: AMP_CONSTEXPR = constexpr.

Why this exists:
 - Some older/embedded C++ toolchains (Arduino-like) have incomplete/buggy support for `constexpr`
   in functions (especially when the function body uses float math or float->int conversions).
 - On such compilers this may compile very slowly, produce strange errors, or fail to compile at all.

If you hit compilation problems, you can switch constexpr-off for functions by overriding the macro
 (e.g. via compiler flags or before including this header):
   #define AMP_CONSTEXPR inline

Note:
 - This macro must be used ONLY for functions (not for variables/constants).
 - Replacing constexpr with inline means the function is no longer a compile-time constant expression.
*/
#ifndef AMP_CONSTEXPR

// Many Arduino toolchains are stuck on older GCC versions where `constexpr` can be buggy/slow
// (especially with float math and some union/aggregate patterns). Default to `inline` there.
#if defined(ARDUINO)
  // C++ 11
  #define AMP_CONSTEXPR inline
  // Keyword for constexpr - empty
  #define AMP_CONSTEXPR_KW
  #warning AMP_CONSTEXPR: inline
#else
  // C++ 17+
  #define AMP_CONSTEXPR constexpr
  // Keyword for constexpr
  #define AMP_CONSTEXPR_KW constexpr
#endif

#endif

//TODO: WIP
//#if __cpp_constexpr >= 201603L


// Arduino PROGMEM support (AVR/ESP8266/ESP32).
// Platform-specific header inclusion for PROGMEM support.
#if defined(__AVR__)
    // AVR-based boards (Arduino Nano, Uno, etc.)
    #include <avr/pgmspace.h>
#elif defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_ESP32)
    // ESP8266/ESP32 boards
    #include <pgmspace.h>
#elif defined(ARDUINO)
    // Other Arduino platforms: Arduino.h should provide PROGMEM
    #include <Arduino.h>
#else
    // Non-Arduino context: PROGMEM may not be available
    // Define PROGMEM as empty and pgm_read functions as direct access for non-Arduino builds
    #define PROGMEM
    #define pgm_read_byte(addr) (*(const uint8_t *)(addr))
    #define pgm_read_word(addr) (*(const uint16_t *)(addr))
    #define pgm_read_dword(addr) (*(const uint32_t *)(addr))
#endif

// Arduino flash-string helper macro compatibility.
// In Arduino, `F("text")` stores the string in flash/PROGMEM and returns a special pointer type.
// For non-Arduino builds we define `F(x)` as a no-op so the same code compiles.
#ifndef F
    #if defined(ARDUINO)
        // Arduino core normally defines `F()` in Arduino.h/WString.h.
        // If some toolchain doesn't provide it, define a compatible fallback.
        class __FlashStringHelper;
        #ifndef PSTR
            #define F(str) (str)
        #else
            #define F(str) (reinterpret_cast<const __FlashStringHelper*>(PSTR(str)))
        #endif
    #else
        #define F(str) (str)
    #endif
#endif

#ifdef __cpp_static_assert
    // C++ 17+
    #define AMP_STATIC_ASSERT static_assert
#else
    // C++ 11
    #include <cassert>
    #define AMP_STATIC_ASSERT(condition, message) static_assert(condition, message)
#endif

// PROGMEM string pointer type (Flash/ROM string storage).
// Use this type for pointers to strings stored in flash.
// - Arduino: matches the type returned by `F("...")`.
// - Non-Arduino builds: falls back to a normal `const char*`.
namespace amp {
#if defined(ARDUINO)
    class __FlashStringHelper;
    using tProgmemStrPtr = const __FlashStringHelper*;
#else
    using tProgmemStrPtr = const char*;
#endif
} // namespace amp

