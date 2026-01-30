#include <FastLED.h>
#include "AlphaMatrixPixels.h"
#include "effect_manager.hpp"
#include "matrix_sfx_system.hpp"
#include "effect_presets.hpp"
#include "effect_presets_local.hpp"
#include "led_config.hpp"
#include "wifi_ota.hpp"
#include <small_timer.h>

constexpr uint16_t cNumLeds = cWidth * cHeight;

CRGB leds[cNumLeds];

// Matrix sfxSystem: manages matrix and effect manager
amp::csMatrixSFXSystem sfxSystem(cWidth, cHeight);

// TODO: WIP...
amp::csMatrixPixels canvasX2(cWidth*2, cHeight*2);

csTimerDef<20 * 1000> tEffectSwitch;  // 20 seconds - default time in template
csTimerShortDef<1000 / 60> tRender;   // ~60 FPS (16 ms) - default time in template
constexpr uint8_t cEffectsCount = 6;
uint8_t effectIndex = 1;

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

    setupOTA();
    
    tEffectSwitch.start();  // Start timer with default time (20 seconds)
    tRender.start();        // Start render timer with default time (~60 FPS)
    loadEffectPresetLocal(*sfxSystem.effectManager, 5, &canvasX2); // Snowfall
}

void loop() {
    handleOTA();

    // Blink built-in LED: (millis() / 500) % 2, no timers/state.
    #if defined(ARDUINO_ARCH_ESP8266)
        digitalWrite(LED_BUILTIN, ((millis() / 500u) % 2u) == 0u ? LOW : HIGH);
    #endif

    // Check if timer has fired and switch to next effect
    if (tEffectSwitch.run()) {
        effectIndex = (effectIndex % cEffectsCount) + 1;
        tEffectSwitch.start();  // Restart timer with default time

        // Clear all effects and add needed ones based on effectIndex (1..cEffectsCount)
        sfxSystem.effectManager->clearAll();
        switch (effectIndex) {
            case 1:
                loadEffectPresetLocal(*sfxSystem.effectManager, 3); // Snowfall
                break;
            case 2:
                loadEffectPresetLocal(*sfxSystem.effectManager, 2); // GradientWaves
                break;
            case 3:
                loadEffectPresetLocal(*sfxSystem.effectManager, 1); // Plasma
                break;
            case 4:
                loadEffectPresetLocal(*sfxSystem.effectManager, 116); // 5 BouncingPixels with different bright colors
                break;
            case 5:
                loadEffectPresetLocal(*sfxSystem.effectManager, 117); // RandomFlashPoint
                break;
            case 6:
                loadEffectPresetLocal(*sfxSystem.effectManager, 118); // RandomFlashPointOverlay
                break;
        }
    }
    
    // Check if render timer has fired
    if (tRender.run()) {
        const amp::tTime currTime = static_cast<amp::tTime>(millis());

        canvasX2.clear();
        sfxSystem.internalMatrix->clear();
        
        // Update and render all effects
        sfxSystem.recalcAndRender(currTime);

        amp::copyMatrixToFastLED(*sfxSystem.internalMatrix, leds, cNumLeds, amp::csMappingPattern::SerpentineHorizontalInverted);
        FastLED.show();
        
        tRender.start();  // Restart timer with default time
    }
}

