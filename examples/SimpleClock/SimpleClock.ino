#include <Wire.h>
#include <RTClib.h>
#include <FastLED.h>
#include "AlphaMatrixPixels.h"
#include "color_rgba.hpp"
#include "effect_manager.hpp"
#include "effect_presets.hpp"
#define LED_CFG 6
#include "led_config.hpp"
#include "matrix_render_pipes.hpp"
#include "amp_progmem.hpp"
#include "driver_serial.hpp"
#include "remap_config.hpp"

#define DIGIT_TEST_MODE
#define HORIZONTAL_LINE_DEBUG_MODE

// Debug mode: draw horizontal line in canvas matrix
// #define HORIZONTAL_LINE_DEBUG_MODE

// Enable clock rendering (set to 0 to disable clock digits)
#ifndef AMP_ENABLE_CLOCK
#define AMP_ENABLE_CLOCK 0
#endif

// Enable Serial debug output (set to 0 to disable)
#ifndef AMP_ENABLE_SERIAL_DEBUG
#define AMP_ENABLE_SERIAL_DEBUG 0
#endif

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

// Last time when debug output was printed (for 1 second interval)
unsigned long lastDebugPrintTime = 0;

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

    #if AMP_ENABLE_SERIAL_DEBUG
    // Initialize Serial for debug output
    Serial.begin(115200);
    
    // Wait for Serial to be ready (only on platforms with native USB)
    // This will block on boards without USB, so use timeout
        #if defined(ARDUINO_ARCH_AVR)
        // For AVR (Arduino Nano/Uno), Serial is usually ready immediately
        // But add small delay to ensure it's initialized
        delay(100);
        #elif defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_ESP32)
        // For ESP boards, wait for Serial with timeout
        unsigned long startTime = millis();
        while (!Serial && (millis() - startTime < 3000)) {
            delay(10);
        }
        #endif
    
    Serial.println("Started");
    #endif
}

void loop() {
    // Read time from RTC
    // TODO: время может отсутствовать - если модуль сбосился и не инициализирован
    DateTime now = rtc.now();

    canvas.clear();

    #ifdef HORIZONTAL_LINE_DEBUG_MODE
    // Draw horizontal line in canvas for debugging
    // Calculate line position based on seconds (moves down every second)
    uint8_t lineY = (now.second() % canvas.height());
    amp::csColorRGBA c(0xff0000);
    canvas.fillArea(amp::csRect{0, static_cast<amp::tMatrixPixelsCoord>(lineY), canvas.width(), 1}, c);
    #endif

    #if AMP_ENABLE_CLOCK
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
    #endif
    
    const amp::tTime currTime = static_cast<amp::tTime>(millis());
    
    // Check if second button is pressed - pixel test mode
    if (digitalRead(cButton1Pin) == LOW) {
        remapHelper.matrix1D.clear();

        // Fill entire 1D matrix with black
        remapHelper.matrix1D.fillArea(remapHelper.matrix1D.getRect(), amp::csColorRGBA(0, 0, 0));
        
        // Calculate current pixel index based on seconds
        uint32_t pixelIndex = (millis() / 1000) % cRemapDestMatrixLen;
        
        // Light up one pixel in white
        remapHelper.matrix1D.setPixelRewrite(static_cast<amp::tMatrixPixelsCoord>(pixelIndex), 0, amp::csColorRGBA(255, 255, 255));
    } else {
        // Normal mode: render effects
        #if AMP_ENABLE_CLOCK
        // Update and render all effects
        effectManager.updateAndRenderAll(rng, currTime);
        #endif
        
        // Remap 2D matrix to 1D matrix (after effects render)
        remapHelper.update(canvas, rng, currTime);
        
        // Check if first button is pressed - fill matrix with white
        if (digitalRead(cButton2Pin) == LOW) {
            canvas.fillArea(canvas.getRect(), amp::csColorRGBA(255, 255, 255));
            remapHelper.update(canvas, rng, currTime);
        }
    }

    amp::copyMatrixToFastLED(remapHelper.matrix1D, leds, 12 * 5, amp::csMappingPattern::SerpentineHorizontalInverted);
    FastLED.show();

    #if AMP_ENABLE_SERIAL_DEBUG
    // Print matrix to serial debug once per second
    unsigned long currentTime = millis();
    
    if (currentTime - lastDebugPrintTime >= 1000) {
        Serial.println("----");
        amp::printMatrixToSerialDebug(canvas);
        lastDebugPrintTime = currentTime;
    }
    #endif

    delay(16); // ~60 FPS
}

