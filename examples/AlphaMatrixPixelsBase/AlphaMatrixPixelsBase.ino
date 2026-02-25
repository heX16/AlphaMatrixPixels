#include <FastLED.h>
#include "AlphaMatrixPixels.h"
#include "effect_manager.hpp"
#include "matrix_sfx_system.hpp"
#include "effect_presets.hpp"

#define ALPHAMATRIX_SINGLE_EFFECT_FLAME
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
uint8_t effectIndex = 1;

#ifdef ALPHAMATRIX_SINGLE_EFFECT_FLAME
// 10 s timer: switch flame strength via sparking (strong / medium / weak)
csTimerDef<10 * 1000> tFlameSparkingSwitch;
// Sparking levels: higher = more active flame (cooling stays 40, set by preset)
static const uint8_t cFlameSparkingLevels[3] = { 150, 70, 30 };  // strong, medium, weak
static const uint8_t cFlameCoolingLevels[3] = { 60, 60, 60 };  // strong, medium, weak
#endif

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
#ifdef ALPHAMATRIX_SINGLE_EFFECT_FLAME
    tFlameSparkingSwitch.start();  // Start 10 s flame sparking switch timer
#endif
    loadEffectByIndexLocal(*sfxSystem.effectManager, effectIndex);
}

void loop() {
    handleOTA();

    // Blink built-in LED: (millis() / 500) % 2, no timers/state.
    #if defined(ARDUINO_ARCH_ESP8266)
        digitalWrite(LED_BUILTIN, ((millis() / 500u) % 2u) == 0u ? LOW : HIGH);
    #endif

    #ifdef ALPHAMATRIX_SINGLE_EFFECT_FLAME
    #else
    // Check if timer has fired and switch to next effect
    if (tEffectSwitch.run()) {
        effectIndex = (effectIndex % cEffectsCount) + 1;
        tEffectSwitch.start();  // Restart timer with default time

        // Clear all effects and add needed ones based on effectIndex (1..cEffectsCount)
        sfxSystem.effectManager->clearAll();
        loadEffectByIndexLocal(*sfxSystem.effectManager, effectIndex);
    }
    #endif
    
    // Check if render timer has fired
    if (tRender.run()) {
        const amp::tTime currTime = static_cast<amp::tTime>(millis());

        canvasX2.clear();
        sfxSystem.internalMatrix->clear();

#ifdef ALPHAMATRIX_SINGLE_EFFECT_FLAME
        // Every 10 s pick random flame strength via sparking (strong / medium / weak)
        if (tFlameSparkingSwitch.run()) {
            amp::csEffectBase* eff = sfxSystem.effectManager->get(0);
            auto randValue = random(0, 3);
            if (eff) {
                auto* flame = static_cast<amp::csRenderFlame*>(eff->queryClassFamily(amp::PropType::EffectFlame));
                if (flame != nullptr) {
                    flame->sparking = cFlameSparkingLevels[randValue];
                    flame->cooling = cFlameCoolingLevels[randValue];
                }
            }
            tFlameSparkingSwitch.start();
        }
#endif

        // Update and render all effects
        sfxSystem.recalcAndRender(currTime);

        amp::copyMatrixToFastLED(*sfxSystem.internalMatrix, leds, cNumLeds, amp::csMappingPattern::SerpentineHorizontalInverted);
        FastLED.show();
        
        tRender.start();  // Restart timer with default time
    }
}

