#include <FastLED.h>
#include "AlphaMatrixPixels.h"
#include "led_config.hpp"

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
amp::csRenderGlyph glyph;
amp::csRandGen rng;

void setup() {
    constexpr uint16_t cLedCount = NUM_LEDS;

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

    // Configure glyph renderer for clock display
    glyph.setMatrix(canvas);
    glyph.color = amp::csColorRGBA{255, 255, 255, 255};
    glyph.backgroundColor = amp::csColorRGBA{0, 0, 0, 0};
    glyph.setFont(amp::getStaticFontTemplate<amp::csFont3x5Digits>());
    glyph.renderRectAutosize = false;
    glyph.rectDest = amp::csRect{
        1,
        1,
        glyph.fontWidth,
        glyph.fontHeight
    };
}

void loop() {
    const uint32_t totalSeconds = millis() / 1000u;
    const uint8_t minutes = static_cast<uint8_t>((totalSeconds / 60u) % 60u);
    const uint8_t seconds = static_cast<uint8_t>(totalSeconds % 60u);

    canvas.clear();

    // Display minutes (left digit) at position (0, 1)
    glyph.symbolIndex = static_cast<uint8_t>(minutes / 10u);
    glyph.rectDest = amp::csRect{0, 1, glyph.fontWidth, glyph.fontHeight};
    glyph.render(rng, static_cast<uint16_t>(millis()));

    // Display minutes (right digit) at position (4, 1)
    glyph.symbolIndex = static_cast<uint8_t>(minutes % 10u);
    glyph.rectDest = amp::csRect{4, 1, glyph.fontWidth, glyph.fontHeight};
    glyph.render(rng, static_cast<uint16_t>(millis()));

    // Note: For 8x8 matrix with 3x5 font, we can display MM (two digits)
    // For MM:SS format, you would need a larger matrix (at least 10x8)

    amp::copyMatrixToFastLED(canvas, leds, NUM_LEDS, amp::csMappingPattern::SerpentineHorizontalInverted);
    FastLED.show();
    delay(16); // ~60 FPS
}

