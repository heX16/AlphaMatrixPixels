#include <FastLED.h>
#include "matrix_render_efffects.hpp"
#include "color_rgba.hpp"
#include "led_config.hpp"
#include "wifi_ota.hpp"
#include "fastled_output.hpp"

// Output gamma correction (applied to FastLED output buffer right before show()).
// Set AMP_ENABLE_GAMMA to 0 to disable.
#ifndef AMP_ENABLE_GAMMA
#define AMP_ENABLE_GAMMA 1
#endif

// FastLED color correction (channel scaling), e.g. TypicalLEDStrip.
// This is NOT gamma correction; it can be enabled together with gamma.
#ifndef AMP_ENABLE_COLOR_CORRECTION
#define AMP_ENABLE_COLOR_CORRECTION 1
#endif

#ifndef AMP_COLOR_CORRECTION
#define AMP_COLOR_CORRECTION TypicalLEDStrip
#endif

constexpr uint8_t WIDTH = 8;
constexpr uint8_t HEIGHT = 8;
constexpr uint16_t NUM_LEDS = WIDTH * HEIGHT;

CRGB leds[NUM_LEDS];

amp::csMatrixPixels canvas(WIDTH, HEIGHT);
amp::csRenderPlasma plasma;
amp::csRenderGradientWaves gradientWaves;
amp::csRenderSnowfall snowfall;
amp::csRenderGlyph glyph;
amp::csRandGen rng;

void setup() {
    constexpr uint16_t cLedCount = NUM_LEDS;

#if defined(ARDUINO_ARCH_ESP8266)
    // ESP8266 LoLin/NodeMCU built-in LED is typically on D4 (GPIO2) and is active-low.
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, HIGH);
#endif

CLEDController* controller = nullptr;
#if (LED_INIT_MODE == 1)
    controller = &FastLED.addLeds<LED_CHIPSET, cDataPin, cClockPin, cLedRgbOrder>(leds, cLedCount);
#elif (LED_INIT_MODE == 2)
    controller = &FastLED.addLeds<LED_CHIPSET, cDataPin, cLedRgbOrder>(leds, cLedCount);
#elif (LED_INIT_MODE == 3)
    controller = &FastLED.addLeds<LED_CHIPSET, cLedRgbOrder>(leds, cLedCount);
#else
    #error "Invalid LED_INIT_MODE"
#endif

#if AMP_ENABLE_COLOR_CORRECTION
    controller->setCorrection(AMP_COLOR_CORRECTION);
#endif

    FastLED.setBrightness(180);
    FastLED.clear(true);

    plasma.setMatrix(canvas);
    plasma.scale = amp::math::csFP16(0.3f);
    plasma.speed = amp::math::csFP16(1.0f);

    gradientWaves.setMatrix(canvas);
    gradientWaves.scale = amp::math::csFP16(0.5f);
    gradientWaves.speed = amp::math::csFP16(1.0f);

    snowfall.setMatrix(canvas);
    snowfall.color = amp::csColorRGBA{255, 255, 255, 255};

    // Digit overlay configuration matches GUI_Test/main.cpp (csRenderGlyph defaults).
    /*
    glyph.setMatrix(canvas);
    glyph.color = amp::csColorRGBA{255, 255, 255, 255};
    glyph.backgroundColor = amp::csColorRGBA{128, 0, 0, 0};
    glyph.symbolIndex = 0;
    glyph.setFont(amp::font3x5Digits());
    glyph.renderRectAutosize = false;
    glyph.rect = amp::csRect{
        1,
        1,
        static_cast<amp::tMatrixPixelsSize>(amp::font3x5Digits().width()),
        static_cast<amp::tMatrixPixelsSize>(amp::font3x5Digits().height())
    };
    */

    amp::wifi_ota::setup();
}

void loop() {
    amp::wifi_ota::handle();

    // Blink built-in LED: (millis() / 500) % 2, no timers/state.
#if defined(ARDUINO_ARCH_ESP8266)
    digitalWrite(LED_BUILTIN, ((millis() / 500u) % 2u) == 0u ? LOW : HIGH);
#endif

    const uint8_t effectIndex = static_cast<uint8_t>((millis() / 20000u) % 3u);
    const uint16_t t = static_cast<uint16_t>(millis());

    canvas.clear();
    if (effectIndex == 0) {
        plasma.render(rng, t);
    } else if (effectIndex == 1) {
        gradientWaves.render(rng, t);
    } else {
        snowfall.recalc(rng, t);
        snowfall.render(rng, t);
    }

    /*
    // Show one digit (0..9), switching once per second (same rule as GUI_Test).
    glyph.symbolIndex = static_cast<uint8_t>((millis() / 1000u) % 10u);
    glyph.render(rng, t); // Overlay over plasma
    */

    amp::copyMatrixToFastLED(canvas, leds, NUM_LEDS);
    FastLED.show();
    delay(16); // ~60 FPS
}

