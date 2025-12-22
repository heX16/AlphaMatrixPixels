#include <FastLED.h>
#include "matrix_render.hpp"

constexpr uint8_t WIDTH = 8;
constexpr uint8_t HEIGHT = 8;
constexpr uint16_t NUM_LEDS = WIDTH * HEIGHT;

// LED strip configuration via macros.
//
// Select a preset by number:
//   #define LED_CFG 3
//
// The preset defines:
// - LedChip (FastLED chipset type)
// - cDataPin / cClockPin (if required)
// - cLedRgbOrder (channel order)
// - LED_INIT_MODE (which FastLED.addLeds overload to use)
//
// Notes:
// - LED_INIT_MODE = 1: LED_TYPE + DATA_PIN + CLOCK_PIN + LED_RGB_TYPE
// - LED_INIT_MODE = 2: LED_TYPE + DATA_PIN + LED_RGB_TYPE
// - LED_INIT_MODE = 3: LED_TYPE + LED_RGB_TYPE


#define LED_CFG 3


  #if (LED_CFG == 1)
    using LedChip = LPD6803;
    constexpr uint8_t cDataPin = 11;
    constexpr uint8_t cClockPin = 13;
    constexpr EOrder cLedRgbOrder = RGB;
    #define LED_INIT_MODE 1
  #elif (LED_CFG == 2)
    using LedChip = SM16716;
    constexpr uint8_t cDataPin = 11;
    constexpr uint8_t cClockPin = 13;
    constexpr EOrder cLedRgbOrder = RGB;
    #define LED_INIT_MODE 1
  #elif (LED_CFG == 3)
    using LedChip = WS2812;
    constexpr uint8_t cDataPin = 6;
    constexpr EOrder cLedRgbOrder = GRB;
    #define LED_INIT_MODE 2
  #elif (LED_CFG == 4)
    using LedChip = LPD6803;
    constexpr uint8_t cDataPin = D1;
    constexpr uint8_t cClockPin = D2;
    constexpr EOrder cLedRgbOrder = RGB;
    #define LED_INIT_MODE 1
  #elif (LED_CFG == 5)
    using LedChip = APA102;
    constexpr uint8_t cDataPin = 11;
    constexpr uint8_t cClockPin = 13;
    constexpr EOrder cLedRgbOrder = BRG;
    #define LED_INIT_MODE 1
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
    FastLED.addLeds<LedChip, cDataPin, cClockPin, cLedRgbOrder>(leds, cLedCount);
#elif (LED_INIT_MODE == 2)
    FastLED.addLeds<LedChip, cDataPin, cLedRgbOrder>(leds, cLedCount);
#elif (LED_INIT_MODE == 3)
    FastLED.addLeds<LedChip, cLedRgbOrder>(leds, cLedCount);
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

