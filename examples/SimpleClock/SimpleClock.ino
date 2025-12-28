#include <FastLED.h>
#include "AlphaMatrixPixels.h"
#include "effect_manager.hpp"
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
amp::csRenderDigitalClock clock;
amp::csRenderDigitalClockDigit digitGlyph;
amp::csRandGen rng;

// Effect manager
csEffectManager effectManager(canvas);

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

    // Get font dimensions for clock size calculation
    const auto& font = amp::getStaticFontTemplate<amp::csFont4x7DigitClock>();
    const amp::tMatrixPixelsSize fontWidth = static_cast<amp::tMatrixPixelsSize>(font.width());
    const amp::tMatrixPixelsSize fontHeight = static_cast<amp::tMatrixPixelsSize>(font.height());
    constexpr amp::tMatrixPixelsSize spacing = 1; // spacing between digits
    constexpr uint8_t digitCount = 4; // clock displays 4 digits
    
    // Calculate clock rect size: 4 digits + 3 spacings between them
    const amp::tMatrixPixelsSize clockWidth = digitCount * fontWidth + (digitCount - 1) * spacing;
    const amp::tMatrixPixelsSize clockHeight = fontHeight;
    
    // Configure glyph effect for rendering digits
    digitGlyph.setFont(font);
    digitGlyph.color = amp::csColorRGBA{255, 255, 255, 255};
    digitGlyph.backgroundColor = amp::csColorRGBA{255, 0, 0, 0};
    digitGlyph.renderRectAutosize = false;
    
    // Set renderDigit via paramRenderDigit parameter
    clock.renderDigit = &digitGlyph;
    
    // Notify clock that paramRenderDigit parameter changed
    // This will validate the glyph type and update its matrix if needed
    clock.paramChanged(amp::csRenderDigitalClock::paramRenderDigit);
    
    clock.renderRectAutosize = false;
    clock.rectDest = amp::csRect{2, 2, amp::to_size(clockWidth+1), amp::to_size(clockHeight+1)};
    
    // Add effects to manager (clock first, then digitGlyph)
    // Note: effectManager automatically binds matrix to effects via bindEffectMatrix()
    effectManager.set(0, &clock);
    effectManager.set(1, &digitGlyph);
}

void loop() {
    const uint32_t totalSeconds = millis() / 1000u;
    const uint8_t minutes = static_cast<uint8_t>((totalSeconds / 60u) % 60u);
    const uint8_t seconds = static_cast<uint8_t>(totalSeconds % 60u);
    
    // Format time as MMSS (4 digits)
    const uint32_t timeValue = static_cast<uint32_t>(minutes * 100u + seconds);

    canvas.clear();

    const uint16_t currTime = static_cast<uint16_t>(millis());
    
    // Update time for clock effect
    clock.time = timeValue;
    
    // Update and render all effects
    effectManager.updateAndRenderAll(rng, currTime);

    amp::copyMatrixToFastLED(canvas, leds, NUM_LEDS, amp::csMappingPattern::SerpentineHorizontalInverted);
    FastLED.show();
    delay(16); // ~60 FPS
}

