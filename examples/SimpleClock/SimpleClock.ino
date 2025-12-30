#include <Wire.h>
#include <RTClib.h>
#include <FastLED.h>
#include "AlphaMatrixPixels.h"
#include "color_rgba.hpp"
#include "effect_manager.hpp"
#include "effect_presets.hpp"
#include "matrix_render_pipes.hpp"
#include "amp_progmem.hpp"
#include "driver_serial.hpp"
#include "rect.hpp"
#include "remap_config.hpp"

#define LED_CFG 6
#include "led_config.hpp"


amp::csMatrixPixels canvas(cSrcWidth, cSrcHeight);
amp::csRandGen rng;

// Real-time clock
RTC_DS3231 rtc;

// Effect manager
csEffectManager effectManager(canvas);

// Remap helper: remaps 2D matrix to 1D matrix using custom mapping array
// Based on copy_line_index_helper.hpp from GUI_Test
class csRemap1DHelper {
public:
    // 1D destination matrix (height=1)
    amp::csMatrixPixels matrix1D{cRemapDestMatrixLen, 1};

    // Remap effect
    amp::csRenderRemap1DByConstArray remapEffect;

    csRemap1DHelper() {
        // Configure remap effect
        remapEffect.matrix = &matrix1D;
        remapEffect.renderRectAutosize = false;
        remapEffect.remapArray = cRemapSrcArray;
        remapEffect.remapWidth = cSrcWidth;
        remapEffect.remapHeight = cSrcHeight;
        remapEffect.rewrite = true;
        remapEffect.rectSource.x = 0;
        remapEffect.rectSource.y = 0;
        remapEffect.rectSource.width = cSrcWidth;
        remapEffect.rectSource.height = cSrcHeight;
        remapEffect.rectDest = amp::csRect{0, 0, cRemapDestMatrixLen, 1};
    }

    // Update remap source and render to fill matrix1D
    // Call AFTER effects render into source matrix
    void update(amp::csMatrixPixels& sourceMatrix, amp::csRandGen& randGen, amp::tTime currTime) {
        remapEffect.matrixSource = &sourceMatrix;
        remapEffect.render(randGen, currTime);
    }
};

csRemap1DHelper remapHelper;

CRGB leds[cRemapDestMatrixLen];

void setup() {
    CLEDController* controller = nullptr;
    
    #if (LED_INIT_MODE == 1)
    controller = &FastLED.addLeds<LED_CHIPSET, cDataPin, cClockPin, cLedRgbOrder>(leds, cRemapDestMatrixLen);
    #elif (LED_INIT_MODE == 2)
    controller = &FastLED.addLeds<LED_CHIPSET, cDataPin, cLedRgbOrder>(leds, cRemapDestMatrixLen);
    #elif (LED_INIT_MODE == 3)
    controller = &FastLED.addLeds<LED_CHIPSET, cLedRgbOrder>(leds, cRemapDestMatrixLen);
    #else
        #error "Invalid LED_INIT_MODE"
    #endif

    #if AMP_ENABLE_COLOR_CORRECTION
    controller->setCorrection(AMP_COLOR_CORRECTION);
    #endif

    FastLED.setBrightness(180);
    FastLED.clear(true);

    // Initialize RTC
    Wire.begin();
    rtc.begin();

    // For manual time setting (uncomment if needed):
    // rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));

    // Load clock effect preset (creates clock and digitGlyph, adds them to manager)
    loadEffectPreset(effectManager, canvas, 3);
    loadEffectPreset(effectManager, canvas, 2);
}

void loop() {
    // Read time from RTC
    DateTime now = rtc.now();

    canvas.clear();

    // Update time for clock effect
    // Search through all effects to find csRenderDigitalClock
    for (auto* effect : effectManager) {
        if (effect == nullptr) {
            continue;
        }
        if (auto* clock = static_cast<amp::csRenderDigitalClock*>(
            effect->queryClassFamily(amp::PropType::EffectDigitalClock)
        )) {
            // Normal mode: display hours and minutes
            clock->time = static_cast<uint32_t>(now.hour() * 100u + now.minute());
            //clock->time = 1234;
            break; // Found clock, no need to continue
        }
    }
    
    const amp::tTime currTime = static_cast<amp::tTime>(millis());
    
    // Normal mode: render effects
    // Update and render all effects
    effectManager.updateAndRenderAll(rng, currTime);
    

    //#define HORIZONTAL_LINE_DEBUG_MODE
    #ifdef HORIZONTAL_LINE_DEBUG_MODE
    // Draw horizontal line in canvas for debugging
    // Calculate line position based on seconds (moves down every second)
    uint8_t lineY = (now.second() % canvas.height());
    canvas.fillArea(
        amp::csRect{0, 
        static_cast<amp::tMatrixPixelsCoord>(lineY), canvas.width(), 1}, 
        amp::csColorRGBA(0x888800));
    #endif

    // Remap 2D matrix to 1D matrix (after effects render)
    remapHelper.update(canvas, rng, currTime);

    amp::copyMatrixToFastLED(remapHelper.matrix1D, 
        leds, cRemapDestMatrixLen,
        amp::csMappingPattern::SerpentineHorizontalInverted);

    FastLED.show();

    delay(16); // ~60 FPS
}

