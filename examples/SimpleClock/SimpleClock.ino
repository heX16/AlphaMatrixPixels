#include <Wire.h>
#include "RTClib.h"
#include <FastLED.h>
#include <cstdint>
#include "AlphaMatrixPixels.h"
#include "effect_manager.hpp"
#include "effect_presets.hpp"
#define LED_CFG 6
#include "led_config.hpp"
#include "matrix_render_pipes.hpp"
#include "amp_progmem.hpp"

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

constexpr uint8_t WIDTH = 19;
constexpr uint8_t HEIGHT = 7;
constexpr uint16_t NUM_LEDS = WIDTH * HEIGHT;

amp::csMatrixPixels canvas(WIDTH, HEIGHT);
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
    static constexpr amp::tMatrixPixelsSize RemapDestMatrixLen = 56;
    amp::csMatrixPixels matrix1D{RemapDestMatrixLen, 1};

    // Remap effect
    amp::csRenderRemap1DByConstArray remapEffect;

    // Remap array: src(x,y) -> dst x (row-major order, 1-based indexing)
    // Stored in PROGMEM (Flash) to save RAM on Arduino platforms.
    static constexpr amp::tMatrixPixelsSize RemapIndexLen = 133; // 19x7
    static const amp::tMatrixPixelsCoord remapArray[RemapIndexLen] PROGMEM;

    // Remap array dimensions
    static constexpr amp::tMatrixPixelsSize remapWidth = 19;
    static constexpr amp::tMatrixPixelsSize remapHeight = 7;

    csRemap1DHelper() {
        // Configure remap effect
        remapEffect.matrix = &matrix1D;
        remapEffect.renderRectAutosize = false;
        remapEffect.remapArray = remapArray;
        remapEffect.remapWidth = remapWidth;
        remapEffect.remapHeight = remapHeight;
        remapEffect.rewrite = true;
        remapEffect.rectSource.x = 2;
        remapEffect.rectSource.y = 2;
        remapEffect.rectSource.width = remapWidth;
        remapEffect.rectSource.height = remapHeight;
        remapEffect.rectDest = amp::csRect{0, 0, RemapDestMatrixLen, 1};
    }

    // Update remap source and render to fill matrix1D
    // Call AFTER effects render into source matrix
    void update(amp::csMatrixPixels& sourceMatrix, amp::csRandGen& randGen, amp::tTime currTime) {
        remapEffect.matrixSource = &sourceMatrix;
        remapEffect.render(randGen, currTime);
    }
};

// Out-of-class definition for PROGMEM array (required for C++11/Arduino IDE 1.8.18)
const amp::tMatrixPixelsCoord csRemap1DHelper::remapArray[csRemap1DHelper::RemapIndexLen] PROGMEM = {
    0, 6, 5, 0, 0, 0, 20, 19, 0, 0, 0, 34, 33, 0, 0, 0, 48, 47, 0,
    7, 0, 0, 4, 0, 21, 0, 0, 18, 0, 35, 0, 0, 32, 0, 49, 0, 0, 46,
    8, 0, 0, 3, 0, 22, 0, 0, 17, 0, 36, 0, 0, 31, 0, 50, 0, 0, 45,
    0, 1, 2, 0, 0, 0, 15, 16, 0, 0, 0, 29, 30, 0, 0, 0, 43, 44, 0,
    9, 0, 0, 14, 0, 23, 0, 0, 28, 0, 37, 0, 0, 42, 0, 51, 0, 0, 56,
    10, 0, 0, 13, 0, 24, 0, 0, 27, 0, 38, 0, 0, 41, 0, 52, 0, 0, 55,
    0, 11, 12, 0, 0, 0, 25, 26, 0, 0, 0, 39, 40, 0, 0, 0, 53, 54, 0,
};

csRemap1DHelper remapHelper;

CRGB leds[csRemap1DHelper::RemapDestMatrixLen];

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

    // Initialize RTC
    Wire.begin();
    rtc.begin();

    // For manual time setting (uncomment if needed):
    // rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));

    // Load clock effect preset (creates clock and digitGlyph, adds them to manager)
    loadEffectPreset(effectManager, canvas, 1);
}

void loop() {
    // Read time from RTC
    DateTime now = rtc.now();

    canvas.clear();

    // Update time for clock effect
    if (auto* clock = dynamic_cast<amp::csRenderDigitalClock*>(effectManager[0])) {
        clock->time = static_cast<uint32_t>(now.hour() * 100u + now.minute());
    }
    
    const amp::tTime currTime = static_cast<amp::tTime>(millis());
    
    // Update and render all effects
    effectManager.updateAndRenderAll(rng, currTime);

    // Remap 2D matrix to 1D matrix (after effects render)
    remapHelper.update(canvas, rng, currTime);

    amp::copyMatrixToFastLED(remapHelper.matrix1D, leds, NUM_LEDS, amp::csMappingPattern::SerpentineHorizontalInverted);
    FastLED.show();

    delay(16); // ~60 FPS
}

