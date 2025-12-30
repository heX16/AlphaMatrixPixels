// LED configuration presets for SimpleClock Arduino sketch.
// All comments in English (project convention).
#pragma once

#include <FastLED.h>

// Button pins
constexpr int cButton1Pin = 7;
constexpr int cButton2Pin = 8;


// Select a preset by number:
//   #define LED_CFG 2
//
// Each preset defines:
// - LED_CHIPSET (FastLED chipset selector for addLeds templates)
// - cDataPin / cClockPin (if required)
// - cLedRgbOrder (channel order)
// - LED_INIT_MODE (which FastLED.addLeds overload to use)
//
// Notes:
// - LED_INIT_MODE = 1: LED_TYPE + DATA_PIN + CLOCK_PIN + LED_RGB_TYPE
// - LED_INIT_MODE = 2: LED_TYPE + DATA_PIN + LED_RGB_TYPE
// - LED_INIT_MODE = 3: LED_TYPE + LED_RGB_TYPE
//
// This is a minimal configuration for Arduino Nano (no ESP/WiFi support).

// Output gamma correction (applied to FastLED output buffer right before show()).
// Set AMP_ENABLE_GAMMA to 0 to disable.
#define AMP_ENABLE_GAMMA 1

// FastLED color correction (channel scaling), e.g. TypicalLEDStrip.
// This is NOT gamma correction; it can be enabled together with gamma.
#define AMP_ENABLE_COLOR_CORRECTION 1
#define AMP_COLOR_CORRECTION TypicalLEDStrip

// If you prefer to keep this file untouched, override LED_CFG in your sketch
// BEFORE including this header.
#ifndef LED_CFG
#define LED_CFG 6
#endif

#if (LED_CFG == 1)
// Preset name: "елка"
#define LED_CHIPSET LPD6803
constexpr uint8_t cDataPin = 11;
constexpr uint8_t cClockPin = 13;
constexpr EOrder cLedRgbOrder = RGB;
#define LED_INIT_MODE 1

#elif (LED_CFG == 2)
// Preset name: "матрица на окне", controller: Arduino Nano
#define LED_CHIPSET SM16716
constexpr uint8_t cDataPin = 11;
constexpr uint8_t cClockPin = 13;
constexpr EOrder cLedRgbOrder = RGB;
#define LED_INIT_MODE 1

#elif (LED_CFG == 3)
// Preset name: "лента у брата"
#define LED_CHIPSET WS2812
constexpr uint8_t cDataPin = 6;
constexpr EOrder cLedRgbOrder = GRB;
#define LED_INIT_MODE 2

#elif (LED_CFG == 5)
// Preset name: "часы"
#define LED_CHIPSET APA102
constexpr uint8_t cDataPin = 11;
constexpr uint8_t cClockPin = 13;
constexpr EOrder cLedRgbOrder = BRG;
#define LED_INIT_MODE 1

#elif (LED_CFG == 6)
// Preset name: "rgb_clock_2025" (RTC clock with APA102)
#define LED_CHIPSET APA102
constexpr uint8_t cDataPin = 3;
constexpr uint8_t cClockPin = 4;
constexpr EOrder cLedRgbOrder = BGR;
#define LED_INIT_MODE 1

#else
#error "LED_CFG invalid config number"
#endif

