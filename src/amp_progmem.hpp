#pragma once

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

