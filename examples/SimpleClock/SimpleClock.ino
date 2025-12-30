#include <Wire.h>
#include <RTClib.h>
#include <FastLED.h>
#include "AlphaMatrixPixels.h"
#include "effect_manager.hpp"
#include "effect_presets.hpp"
#define LED_CFG 6
#include "led_config.hpp"
#include "matrix_render_pipes.hpp"
#include "amp_progmem.hpp"

#define DIGIT_TEST_MODE

// Button pins
constexpr int cButton1Pin = 7;
constexpr int cButton2Pin = 8;

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

amp::csMatrixPixels canvas(12, 5);
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
    static constexpr amp::tMatrixPixelsSize RemapDestMatrixLen = 28;
    amp::csMatrixPixels matrix1D{RemapDestMatrixLen, 1};

    // Remap effect
    amp::csRenderRemap1DByConstArray remapEffect;

    // Remap array: src(x,y) -> dst x (row-major order, 1-based indexing)
    // Stored in PROGMEM (Flash) to save RAM on Arduino platforms.
    static constexpr amp::tMatrixPixelsSize RemapIndexLen = 60; // 12x5
    static const amp::tMatrixPixelsCoord remapArray[RemapIndexLen] PROGMEM;

    // Remap array dimensions
    static constexpr amp::tMatrixPixelsSize remapWidth = 12;
    static constexpr amp::tMatrixPixelsSize remapHeight = 5;

    csRemap1DHelper() {
        // Configure remap effect
        remapEffect.matrix = &matrix1D;
        remapEffect.renderRectAutosize = false;
        remapEffect.remapArray = remapArray;
        remapEffect.remapWidth = remapWidth;
        remapEffect.remapHeight = remapHeight;
        remapEffect.rewrite = true;
        remapEffect.rectSource.x = 0;
        remapEffect.rectSource.y = 0;
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
    0, 3, 0, 0, 9, 0, 0, 17, 0, 0, 24, 0,
    4, 0, 2, 10, 0, 0/*x*/, 18, 0, 16, 25, 0, 23,
    0, 1, 0, 0, 8, 0, 0, 15, 0, 0, 22, 0,
    5, 0, 7, 11, 0, 13, 19, 0, 21, 26, 0, 28,
    0, 6, 0, 0, 12, 0, 0, 20, 0, 0, 27, 0,

};

csRemap1DHelper remapHelper;

CRGB leds[csRemap1DHelper::RemapDestMatrixLen];

void setup() {
    constexpr uint16_t cLedCount = 12 * 5;

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

    // Initialize button pins
    pinMode(cButton1Pin, INPUT_PULLUP);
    pinMode(cButton2Pin, INPUT_PULLUP);

    // Load clock effect preset (creates clock and digitGlyph, adds them to manager)
    loadEffectPreset(effectManager, canvas, 1);
}

void loop() {
    // Read time from RTC
    // TODO: время может отсутствовать - если модуль сбосился и не инициализирован
    DateTime now = rtc.now();

    canvas.clear();

    // Update time for clock effect
    if (effectManager[0] != nullptr) {
        auto* clock = static_cast<amp::csRenderDigitalClock*>(effectManager[0]);
#ifdef DIGIT_TEST_MODE
        // Test mode: display last digit of seconds on all 4 positions
        //uint8_t lastSecondDigit = (millis() * 1000) % 10;
        uint8_t lastSecondDigit = now.second() % 10;
        //clock->time = lastSecondDigit * 1111u;
        //clock->time = lastSecondDigit * 0100u;
        clock->time = 1811;
#else
        // Normal mode: display hours and minutes
        clock->time = static_cast<uint32_t>(now.hour() * 100u + now.minute());
#endif
    }
    
    const amp::tTime currTime = static_cast<amp::tTime>(millis());
    
    // Update and render all effects
    effectManager.updateAndRenderAll(rng, currTime);

    // Remap 2D matrix to 1D matrix (after effects render)
    remapHelper.update(canvas, rng, currTime);

    // Check if first button is pressed - fill matrix with white
    if (digitalRead(cButton1Pin) == LOW) {
        canvas.fillArea(canvas.getRect(), amp::csColorRGBA(255, 255, 255));
        remapHelper.update(canvas, rng, currTime);
    }

    amp::copyMatrixToFastLED(remapHelper.matrix1D, leds, 12 * 5, amp::csMappingPattern::SerpentineHorizontalInverted);
    FastLED.show();

    delay(16); // ~60 FPS
}

