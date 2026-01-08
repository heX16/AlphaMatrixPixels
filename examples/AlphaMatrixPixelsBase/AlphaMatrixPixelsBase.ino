#include <FastLED.h>
#include "AlphaMatrixPixels.h"
#include "effect_manager.hpp"
#include "effect_presets.hpp"
#include "led_config.hpp"
#include "wifi_ota.hpp"
#include <small_timer.h>

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

constexpr uint8_t cWidth = 8;
constexpr uint8_t cHeight = 8;
constexpr uint16_t cNumLeds = cWidth * cHeight;

CRGB leds[cNumLeds];

// Matrix sfxSystem: manages matrix and effect manager
amp::csMatrixSFXSystem sfxSystem(cWidth, cHeight);

// TODO: WIP...
amp::csMatrixPixels canvasX2(cWidth*2, cHeight*2);

csTimerDef<20 * 1000> tEffectSwitch;  // 20 seconds - default time in template
csTimerShortDef<1000 / 60> tRender;   // ~60 FPS (16 ms) - default time in template
uint8_t effectIndex = 0;

void setup() {
    constexpr uint16_t cLedCount = cNumLeds;

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

    amp::wifi_ota::setup();
    
    tEffectSwitch.start();  // Start timer with default time (20 seconds)
    tRender.start();        // Start render timer with default time (~60 FPS)
    loadEffectPreset(*sfxSystem.effectManager, 5, &canvasX2); // Snowfall
}

void loop() {
    amp::wifi_ota::handle();

    // Blink built-in LED: (millis() / 500) % 2, no timers/state.
#if defined(ARDUINO_ARCH_ESP8266)
    digitalWrite(LED_BUILTIN, ((millis() / 500u) % 2u) == 0u ? LOW : HIGH);
#endif

    // Check if timer has fired and switch to next effect
    if (tEffectSwitch.run()) {
        effectIndex = (effectIndex + 1) % 4;
        tEffectSwitch.start();  // Restart timer with default time

        // Clear all effects and add needed ones based on effectIndex
        sfxSystem.effectManager->clearAll();
        if (effectIndex == 0) {
            //loadEffectPreset(*sfxSystem.effectManager, 2); // GradientWaves
            loadEffectPreset(*sfxSystem.effectManager, 3); // Snowfall
        } else if (effectIndex == 1) {
            loadEffectPreset(*sfxSystem.effectManager, 2); // GradientWaves
        } else if (effectIndex == 2) {
            loadEffectPreset(*sfxSystem.effectManager, 1); // Plasma
        } else {
            loadEffectPreset(*sfxSystem.effectManager, 116); // 5 BouncingPixels with different bright colors
        }
    }
    
    // Check if render timer has fired
    if (tRender.run()) {
        const amp::tTime currTime = static_cast<amp::tTime>(millis());

        canvasX2.clear();
        sfxSystem.matrix->clear();
        
        // Update and render all effects
        sfxSystem.recalcAndRender(currTime);

        amp::copyMatrixToFastLED(*sfxSystem.matrix, leds, cNumLeds, amp::csMappingPattern::SerpentineHorizontalInverted);
        FastLED.show();
        
        tRender.start();  // Restart timer with default time
    }
}

