#include <FastLED.h>
#include "matrix_render.hpp"

constexpr uint8_t WIDTH = 8;
constexpr uint8_t HEIGHT = 8;
constexpr uint16_t NUM_LEDS = WIDTH * HEIGHT;

// LED strip configuration via macros.
//
// Configure by selecting a preset number in one macro:
//   #define LED_CFG 3
// Also select initialization mode (which FastLED.addLeds overload to use):
//   #define LED_INIT_MODE 2
// Or define LED_TYPE/DATA_PIN/CLOCK_PIN/LED_RGB_TYPE directly.
//
// Notes:
// - LED_INIT_MODE = 1: LED_TYPE + DATA_PIN + CLOCK_PIN + LED_RGB_TYPE
// - LED_INIT_MODE = 2: LED_TYPE + DATA_PIN + LED_RGB_TYPE
// - LED_INIT_MODE = 3: LED_TYPE + LED_RGB_TYPE
//
// Presets:
//   1: LPD6803 (2-wire)  - pins must be provided
//   2: SM16716 (2-wire?) - pins must be provided (adjust to your hardware)
//   3: WS2812  (1-wire)  - DATA_PIN=6, GRB
//   4: LPD6803 (2-wire)  - DATA_PIN=D1, CLOCK_PIN=D2
//   5: APA102  (2-wire)  - DATA_PIN=11, CLOCK_PIN=13, BRG

// Default preset if nothing is selected AND LED_TYPE is not provided explicitly.
#ifndef LED_CFG
  #if !defined(LED_TYPE)
    #define LED_CFG 3
  #endif
#endif

// Map preset number to macro values (only if LED_TYPE is not provided explicitly).
#if defined(LED_CFG) && !defined(LED_TYPE)
  #if (LED_CFG < 1) || (LED_CFG > 5)
    #error "LED_CFG must be in range [1..5]"
  #endif

  #if (LED_CFG == 1)
    #define LED_TYPE LPD6803
    #define LED_RGB_TYPE RGB
    #define LED_INIT_MODE 1
    #ifndef DATA_PIN
      #error "LED_CFG=1 requires DATA_PIN"
    #endif
    #ifndef CLOCK_PIN
      #error "LED_CFG=1 requires CLOCK_PIN"
    #endif
  #elif (LED_CFG == 2)
    #define LED_TYPE SM16716
    #define LED_RGB_TYPE RGB
    #define LED_INIT_MODE 1
    #ifndef DATA_PIN
      #error "LED_CFG=2 requires DATA_PIN (adjust wiring if needed)"
    #endif
    #ifndef CLOCK_PIN
      #error "LED_CFG=2 requires CLOCK_PIN (adjust wiring if needed)"
    #endif
  #elif (LED_CFG == 3)
    #define LED_TYPE WS2812
    #define DATA_PIN 6
    #define LED_RGB_TYPE GRB
    #define LED_INIT_MODE 2
  #elif (LED_CFG == 4)
    #define LED_TYPE LPD6803
    #define DATA_PIN D1
    #define CLOCK_PIN D2
    #define LED_RGB_TYPE RGB
    #define LED_INIT_MODE 1
  #elif (LED_CFG == 5)
    #define LED_TYPE APA102
    #define DATA_PIN 11
    #define CLOCK_PIN 13
    #define LED_RGB_TYPE BRG
    #define LED_INIT_MODE 1
  #endif
#endif

// Minimal fallbacks when configuring without presets / without preset mapping.
#ifndef LED_RGB_TYPE
#define LED_RGB_TYPE GRB
#endif

// If user configures manually (without LED_CFG mapping), they must also provide LED_INIT_MODE.
#ifndef LED_INIT_MODE
  #if defined(LED_TYPE)
    #error "Define LED_INIT_MODE (1..3) when configuring LED_TYPE manually (without LED_CFG mapping)"
  #endif
#endif

#if defined(LED_INIT_MODE) && ((LED_INIT_MODE < 1) || (LED_INIT_MODE > 3))
#error "LED_INIT_MODE must be in range [1..3]"
#endif

CRGB leds[NUM_LEDS];

amp::csMatrixPixels canvas(WIDTH, HEIGHT);
amp::csRenderPlasma plasma;
amp::csRandGen rng;

// Map 2D coordinates into serpentine 1D index.
uint16_t xyToIndex(uint8_t x, uint8_t y) {
    return (y & 1u) ? static_cast<uint16_t>(y) * WIDTH + (WIDTH - 1u - x)
                    : static_cast<uint16_t>(y) * WIDTH + x;
}

void setup() {
    constexpr uint16_t cLedCount = NUM_LEDS;

#if (LED_INIT_MODE == 1)
    #ifndef DATA_PIN
      #error "LED_INIT_MODE=1 requires DATA_PIN"
    #endif
    #ifndef CLOCK_PIN
      #error "LED_INIT_MODE=1 requires CLOCK_PIN"
    #endif
    FastLED.addLeds<LED_TYPE, DATA_PIN, CLOCK_PIN, LED_RGB_TYPE>(leds, cLedCount);
#elif (LED_INIT_MODE == 2)
    #ifndef DATA_PIN
      #error "LED_INIT_MODE=2 requires DATA_PIN"
    #endif
    FastLED.addLeds<LED_TYPE, DATA_PIN, LED_RGB_TYPE>(leds, cLedCount);
#elif (LED_INIT_MODE == 3)
    FastLED.addLeds<LED_TYPE, LED_RGB_TYPE>(leds, cLedCount);
#else
    #error "Invalid LED_INIT_MODE"
#endif

    FastLED.setBrightness(180);
    FastLED.clear(true);
}

static void copyCanvasToLeds() {
    for (uint8_t y = 0; y < HEIGHT; ++y) {
        for (uint8_t x = 0; x < WIDTH; ++x) {
            const amp::csColorRGBA c = canvas.getPixel(x, y);
            leds[xyToIndex(x, y)] = CRGB(c.r, c.g, c.b);
        }
    }
}

void loop() {
    const uint16_t t = static_cast<uint16_t>(millis());
    plasma.render(canvas, rng, t);
    copyCanvasToLeds();
    FastLED.show();
    delay(16); // ~60 FPS
}

