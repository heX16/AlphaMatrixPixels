#include <FastLED.h>
#include "matrix_render.hpp"
#include "led_config.hpp"

constexpr uint8_t WIDTH = 8;
constexpr uint8_t HEIGHT = 8;
constexpr uint16_t NUM_LEDS = WIDTH * HEIGHT;

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
    FastLED.addLeds<LED_CHIPSET, cDataPin, cClockPin, cLedRgbOrder>(leds, cLedCount);
#elif (LED_INIT_MODE == 2)
    FastLED.addLeds<LED_CHIPSET, cDataPin, cLedRgbOrder>(leds, cLedCount);
#elif (LED_INIT_MODE == 3)
    FastLED.addLeds<LED_CHIPSET, cLedRgbOrder>(leds, cLedCount);
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

