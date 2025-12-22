#include <FastLED.h>
#include "matrix_render.hpp"

constexpr uint8_t WIDTH = 8;
constexpr uint8_t HEIGHT = 8;
constexpr uint16_t NUM_LEDS = WIDTH * HEIGHT;
constexpr uint8_t DATA_PIN = 6;
constexpr EOrder COLOR_ORDER = GRB;

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
    FastLED.addLeds<WS2812B, DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS);
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

