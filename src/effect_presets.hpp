// Helper function for adding effect presets to effect manager
#ifndef EFFECT_PRESETS_HPP
#define EFFECT_PRESETS_HPP

#include <stdint.h>
#include "effect_manager.hpp"
#include "matrix_pixels.hpp"
#include "color_rgba.hpp"
#include "render_efffects.hpp"
#include "render_pipes.hpp"
#include "fixed_point.hpp"
#include "amp_macros.hpp"
#include "fonts.h"

using namespace amp;
using namespace amp::math;

// Abstract function: adds effects to the array based on effect ID
// effectManager: reference to effect manager for adding effects (matrix is taken from effectManager.getMatrix())
// effectId: ID of the effect to create
// matrixSecondBuffer: optional second buffer for effects that need it (e.g., case 5)
inline void loadEffectPreset(
        csEffectManager& effectManager, 
        uint16_t effectId, 
        csMatrixPixels* matrixSecondBuffer = nullptr,
        amp::tProgmemStrPtr* eff_name = nullptr)
{
    // Optional effect name stored in PROGMEM (Flash). If pointer is provided, will be set to effect name.
    // If eff_name is nullptr, use a dummy local pointer as a placeholder
    amp::tProgmemStrPtr dummy_name;
    amp::tProgmemStrPtr* name_ptr = (eff_name != nullptr) ? eff_name : &dummy_name;

    if (effectId == 0) {
        return;
    }

    switch (effectId) {
        // AlphaMatrixPixelsBase effects (1-5)
        case 1:
            {
                *name_ptr = F("Plasma");
                auto* plasma = new csRenderPlasma();
                plasma->scale = csFP16(0.3f);
                plasma->speed = csFP16(1.0f);
                effectManager.add(plasma);
                break;
            }
        case 2:
            {
                *name_ptr = F("GradientWaves");
                auto* gradientWaves = new csRenderGradientWaves();
                gradientWaves->scale = csFP16(0.5f);
                gradientWaves->speed = csFP16(1.0f);
                effectManager.add(gradientWaves);
                break;
            }
        case 3:
            {
                *name_ptr = F("Snowfall");
                auto* snowfall = new csRenderSnowfall();
                snowfall->color = csColorRGBA{255, 255, 255, 255};
                snowfall->count = 5;
                snowfall->propChanged(csRenderSnowfall::propCount);
                // snowfall->speed = csFP16(1.0f / 4.0f);
                snowfall->smoothMovement = true; // Enable sub-pixel smooth movement
                effectManager.add(snowfall);
                break;
            }
        case 4:
            {
                *name_ptr = F("Glyph");
                auto* glyph = new csRenderGlyph();
                glyph->color = csColorRGBA{255, 255, 255, 255};
                glyph->backgroundColor = csColorRGBA{128, 0, 0, 0};
                glyph->symbolIndex = 0;
                glyph->setFont(amp::getStaticFontTemplate<amp::csFont3x5Digits>());
                glyph->renderRectAutosize = false;
                glyph->rectDest = amp::csRect{
                    1,
                    1,
                    glyph->fontWidth,
                    glyph->fontHeight
                };
                effectManager.add(glyph);
                break;
            }
        case 5:
            {
                *name_ptr = F("Snowfall (copy)");
                if (!matrixSecondBuffer) {
                    break; // canvasX2 not provided
                }
                
                // Create snowfall effect that renders to canvasX2
                auto* snowfall = new csRenderSnowfall();
                snowfall->color = csColorRGBA{255, 255, 255, 255};
                snowfall->count = 5;
                snowfall->speed = csFP16(1.0f / 4.0f);
                snowfall->smoothMovement = true; // Enable sub-pixel smooth movement
                snowfall->propChanged(csRenderSnowfall::propCount);
                // Add to manager first (it will set canvas), then override with canvasX2
                effectManager.add(snowfall);
                snowfall->setMatrix(matrixSecondBuffer);
                
                // Create pipe effect to copy from canvasX2 to canvas
                auto* pipe = new csRenderMatrixCopy();
                pipe->matrixSource = matrixSecondBuffer;
                pipe->rectSource = matrixSecondBuffer->getRect();
                if (effectManager.getMatrix()) {
                    pipe->rectDest = effectManager.getMatrix()->getRect();
                }
                effectManager.add(pipe);
                break;
            }
        
        // GUI_Test effects (101-110, 200)
        case 101:
            {
                *name_ptr = F("GradientWaves");
                effectManager.add(new csRenderGradientWaves());
                break;
            }
        case 102:
            {
                *name_ptr = F("GradientWavesFP");
                effectManager.add(new csRenderGradientWavesFP());
                break;
            }
        case 103:
            {
                *name_ptr = F("Plasma");
                effectManager.add(new csRenderPlasma());
                break;
            }
        case 104:
            {
                *name_ptr = F("Snowfall");
                auto* snowfall = new csRenderSnowfall();
                snowfall->smoothMovement = true; // Enable sub-pixel smooth movement
                effectManager.add(snowfall);
                break;
            }
        
        // Secondary effects (105-108)
        case 105:
            {
                *name_ptr = F("Glyph");
                auto* glyph = new csRenderGlyph();
                glyph->color = csColorRGBA{255, 255, 255, 255};
                glyph->backgroundColor = csColorRGBA{196, 0, 0, 0};
                glyph->symbolIndex = 0;
                glyph->setFont(amp::getStaticFontTemplate<amp::csFont4x7Digits>());
                glyph->renderRectAutosize = false;
                glyph->rectDest = amp::csRect{
                    2,
                    2,
                    amp::to_size(glyph->fontWidth + 2),
                    amp::to_size(glyph->fontHeight + 2)
                };
                effectManager.add(glyph);
                break;
            }
        case 106:
            {
            *name_ptr = F("Circle");
            auto* circle = new csRenderCircleGradient();
            circle->color = csColorRGBA{255, 255, 255, 255};
            circle->backgroundColor = csColorRGBA{0, 0, 0, 0};
            circle->gradientOffset = 127;
            circle->renderRectAutosize = true; // использовать весь rect матрицы
            effectManager.add(circle);
            break;
        }
        case 107:
            {
                *name_ptr = F("Clock");
                // Get font dimensions for clock size calculation
                const auto& font = amp::getStaticFontTemplate<amp::csFont4x7DigitalClock>();
                const tMatrixPixelsSize fontWidth = static_cast<tMatrixPixelsSize>(font.width());
                const tMatrixPixelsSize fontHeight = static_cast<tMatrixPixelsSize>(font.height());
                constexpr tMatrixPixelsSize spacing = 1; // spacing between digits
                constexpr uint8_t digitCount = 4; // clock displays 4 digits
                
                // Calculate clock rect size: 4 digits + 3 spacings between them
                const tMatrixPixelsSize clockWidth = digitCount * fontWidth + (digitCount - 1) * spacing;
                const tMatrixPixelsSize clockHeight = fontHeight;
                
                // Create fill effect (background) - covers clock rect
                auto* fill = new csRenderRectangle();
                fill->color = csColorRGBA{192, 0, 0, 0};
                fill->renderRectAutosize = false;
                fill->rectDest = amp::csRect{1, 1, amp::to_size(clockWidth+2), amp::to_size(clockHeight+2)};
                
                // Create clock effect
                auto* clock = new csRenderDigitalClock();
                
                // Create renderDigit effect for rendering digits
                auto* digitGlyph = clock->createRenderDigit();
                digitGlyph->setFont(font);
                digitGlyph->color = csColorRGBA{255, 255, 255, 255};
                digitGlyph->backgroundColor = csColorRGBA{255, 0, 0, 0};
                digitGlyph->renderRectAutosize = false;
                digitGlyph->disabled = true; // Disable direct rendering, only used by clock
                
                // Set renderDigit via propRenderDigit property
                clock->renderDigit = digitGlyph;
                
                // Notify clock that propRenderDigit property changed
                // This will validate the glyph type and update its matrix if needed
                clock->propChanged(amp::csRenderDigitalClock::propRenderDigit);
                
                clock->renderRectAutosize = false;
                clock->rectDest = amp::csRect{2, 2, amp::to_size(clockWidth+1), amp::to_size(clockHeight+1)};
                
                // Add effects to array (fill first, then clock on top, then digitGlyph for proper cleanup)
                effectManager.add(fill);
                effectManager.add(clock);
                effectManager.add(digitGlyph);
                break;
            }
        case 108:
            {
            *name_ptr = F("AverageArea");
            if (!effectManager.getMatrix()) {
                break;
            }
            auto* averageArea = new csRenderAverageArea();
            averageArea->matrixDest = effectManager.getMatrix();
            averageArea->matrixSource = effectManager.getMatrix();
            averageArea->renderRectAutosize = false;
            averageArea->rectSource = amp::csRect{1, 1, 4, 4};
            averageArea->rectDest = amp::csRect{1, 1, 4, 4};
            effectManager.add(averageArea);
            break;
        }
        case 109:
        case 110:
            {
                *name_ptr = (effectId == 109) ? F("Clock 3x5") : F("Clock 3x5 (no BG)");
                // Get font dimensions for clock size calculation
                const auto& font = amp::getStaticFontTemplate<amp::csFont3x5DigitalClock>();
                const tMatrixPixelsSize fontWidth = static_cast<tMatrixPixelsSize>(font.width());
                const tMatrixPixelsSize fontHeight = static_cast<tMatrixPixelsSize>(font.height());
                constexpr tMatrixPixelsSize spacing = 1; // spacing between digits
                constexpr uint8_t digitCount = 4; // clock displays 4 digits
                
                // Calculate clock rect size: 4 digits + 3 spacings between them
                const tMatrixPixelsSize clockWidth = digitCount * fontWidth + (digitCount - 1) * spacing;
                const tMatrixPixelsSize clockHeight = fontHeight;

                // Create clock effect
                auto* clock = new csRenderDigitalClock();
                clock->spacing = 0;
                
                // Create renderDigit effect for rendering digits
                auto* digitGlyph = clock->createRenderDigit();
                digitGlyph->setFont(font);
                digitGlyph->color = csColorRGBA{255, 255, 255, 255};
                if (effectId == 109)
                  digitGlyph->backgroundColor = csColorRGBA{255, 0, 0, 0};
                if (effectId == 110)
                  digitGlyph->backgroundColor = csColorRGBA{0, 0, 0, 0};
                digitGlyph->renderRectAutosize = false;
                digitGlyph->disabled = true; // Disable direct rendering, only used by clock
                
                // Set renderDigit via propRenderDigit property
                clock->renderDigit = digitGlyph;
                
                // Notify clock that propRenderDigit property changed
                // This will validate the glyph type and update its matrix if needed
                clock->propChanged(amp::csRenderDigitalClock::propRenderDigit);
                
                clock->renderRectAutosize = false;
                clock->rectDest = amp::csRect{0, 0, amp::to_size(clockWidth), amp::to_size(clockHeight)};
                
                // Add effects to array (fill first, then clock on top, then digitGlyph for proper cleanup)
                effectManager.add(clock);
                effectManager.add(digitGlyph);
                break;
            }
        case 111:
            {
                *name_ptr = F("SlowFadingBackground");
                auto* fade = new csRenderSlowFadingBackground();

                // Slightly slower fade by default (higher = slower).
                fade->fadeAlpha = 128;

                effectManager.add(fade);
                break;
            }
        case 112:
            {
                *name_ptr = F("7 horizontal lines");
                if (!effectManager.getMatrix()) {
                    break;
                }
                const tMatrixPixelsSize matrixWidth = effectManager.getMatrix()->width();
                const tMatrixPixelsSize matrixHeight = effectManager.getMatrix()->height();
                constexpr uint8_t lineCount = 7;
                
                // Calculate line height (at least 1 pixel)
                // const tMatrixPixelsSize lineHeight = (matrixHeight >= lineCount) ? (matrixHeight / lineCount) : 1;
                const tMatrixPixelsSize lineHeight = 1;
                
                // Define 7 different colors (rainbow colors)
                // csColorRGBA format: (a, r, g, b)
                const csColorRGBA colors[7] = {
                    csColorRGBA{255, 255, 0, 0},     // Red
                    csColorRGBA{255, 255, 165, 0},   // Orange
                    csColorRGBA{255, 255, 255, 0},   // Yellow
                    csColorRGBA{255, 0, 255, 0},     // Green
                    csColorRGBA{255, 0, 255, 255},   // Cyan
                    csColorRGBA{255, 0, 0, 255},     // Blue
                    csColorRGBA{255, 128, 0, 128}    // Purple
                };
                
                // Create 7 fill effects, one for each horizontal line
                for (uint8_t i = 0; i < lineCount; ++i) {
                    auto* fill = new csRenderRectangle();
                    fill->color = colors[i];
                    fill->renderRectAutosize = false;
                    fill->rectDest = amp::csRect{
                        0,
                        amp::to_coord(i * lineHeight),
                        amp::to_size(matrixWidth),
                        amp::to_size(lineHeight)
                    };
                    effectManager.add(fill);
                }
                break;
            }
        case 113:
            {
                *name_ptr = F("BouncingPixel");
                auto* bouncingPixel = new csRenderBouncingPixel();
                bouncingPixel->color = csColorRGBA{255, 255, 255, 0};
                bouncingPixel->speed = csFP16(0.5f);
                bouncingPixel->renderRectAutosize = true;
                effectManager.add(bouncingPixel);
                break;
            }
        case 114:
            {
                *name_ptr = F("SlowFadingOverlay");
                auto* fade = new csRenderSlowFadingOverlay();

                // Default fadeAlpha is already set to 240 in constructor.
                // Can be adjusted if needed:
                // fade->fadeAlpha = 240;

                effectManager.add(fade);
                break;
            }
        case 115:
            {
                *name_ptr = F("BouncingPixelDualTrail");
                auto* bouncingPixelDualTrail = new csRenderBouncingPixelDualTrail();
                bouncingPixelDualTrail->color = csColorRGBA{255, 255, 255, 0};
                bouncingPixelDualTrail->speed = csFP16(0.5f);
                bouncingPixelDualTrail->renderRectAutosize = true;
                effectManager.add(bouncingPixelDualTrail);
                break;
            }
        case 116:
            {
                *name_ptr = F("5 BouncingPixels");
                // Define 5 bright colors
                // csColorRGBA format: (a, r, g, b)
                const csColorRGBA colors[5] = {
                    csColorRGBA{255, 255, 0, 0},      // Red
                    csColorRGBA{255, 0, 255, 0},      // Green
                    csColorRGBA{255, 0, 0, 255},      // Blue
                    csColorRGBA{255, 255, 255, 0},    // Yellow
                    csColorRGBA{255, 0, 255, 255}     // Cyan
                };
                
                // Create 5 bouncing pixel effects, one for each color
                for (uint8_t i = 0; i < 5; ++i) {
                    auto* bouncingPixel = new csRenderBouncingPixel();
                    bouncingPixel->color = colors[i];
                    bouncingPixel->speed = csFP16(0.5f);
                    bouncingPixel->renderRectAutosize = true;
                    effectManager.add(bouncingPixel);
                }
                break;
            }
        case 117:
            {
                *name_ptr = F("RandomFlashPoint");
                auto* fade = new csRenderSlowFadingBackground();
                fade->fadeAlpha = 128;
                effectManager.add(fade);
                auto* flash = new csRenderRandomFlashPoint();
                flash->param = 120;
                flash->pauseMs = 300;
                flash->renderRectAutosize = true;
                effectManager.add(flash);
                break;
            }
        case 200:
            ; // slip - remove "effect2"
        
        // SimpleClock effects (201-205)
        case 201:
        case 202:
            {
                *name_ptr = (effectId == 201) ? F("Clock") : F("Clock negative");
                // Get font dimensions for clock size calculation
                const auto& font = amp::getStaticFontTemplate<amp::csFont3x5Digits>();
                const tMatrixPixelsSize fontWidth = static_cast<tMatrixPixelsSize>(font.width());
                const tMatrixPixelsSize fontHeight = static_cast<tMatrixPixelsSize>(font.height());
                constexpr tMatrixPixelsSize spacing = 0; // spacing between digits (no spacing for 12x5 matrix)
                constexpr uint8_t digitCount = 4; // clock displays 4 digits
                
                // Calculate clock rect size: 4 digits without spacing (12 = 3+3+3+3)
                const tMatrixPixelsSize clockWidth = digitCount * fontWidth + (digitCount - 1) * spacing;
                const tMatrixPixelsSize clockHeight = fontHeight;
                
                // Create digitGlyph effect for rendering digits
                auto* digitGlyph = new csRenderDigitalClockDigit();
                digitGlyph->setFont(font);
                digitGlyph->color = csColorRGBA{255, 255, 255, 255};
                digitGlyph->backgroundColor = csColorRGBA{255, 0, 0, 0};
                digitGlyph->renderRectAutosize = false;
                
                // Create clock effect
                auto* clock = new csRenderDigitalClock();
                
                // Set renderDigit via propRenderDigit property
                clock->renderDigit = digitGlyph;
                
                // Notify clock that propRenderDigit property changed
                // This will validate the glyph type and update its matrix if needed
                clock->propChanged(amp::csRenderDigitalClock::propRenderDigit);
                
                clock->renderRectAutosize = false;
                clock->rectDest = amp::csRect{0, 0, amp::to_size(clockWidth), amp::to_size(clockHeight)};
                clock->spacing = 0;

                if (effectId == 201) {
                    clock->renderDigit->color = csColorRGBA(255, 255, 255, 255);
                    clock->renderDigit->backgroundColor = csColorRGBA(0, 0, 0, 0);
                } else {
                    clock->renderDigit->color = csColorRGBA(0, 0, 0, 0);
                    clock->renderDigit->backgroundColor = csColorRGBA(255, 0, 0, 0);
                }
                
                // Add effects to manager (clock first, then digitGlyph for proper cleanup)
                effectManager.add(clock);
                effectManager.add(digitGlyph);
                break;
            }
        case 203:
            {
                *name_ptr = F("5 horizontal lines");
                if (!effectManager.getMatrix()) {
                    break;
                }
                const tMatrixPixelsSize matrixWidth = effectManager.getMatrix()->width();
                constexpr uint8_t lineCount = 5;
                constexpr tMatrixPixelsSize lineHeight = 1;
                
                // Define 5 different colors
                // csColorRGBA format: (a, r, g, b)
                const csColorRGBA colors[5] = {
                    csColorRGBA{255, 255, 0, 0},     // Red
                    csColorRGBA{255, 0, 255, 0},      // Green
                    csColorRGBA{255, 0, 0, 255},      // Blue
                    csColorRGBA{255, 255, 255, 0},   // Yellow
                    csColorRGBA{255, 255, 0, 255}    // Magenta
                };
                
                // Create 5 fill effects, one for each horizontal line
                for (uint8_t i = 0; i < lineCount; ++i) {
                    auto* fill = new csRenderRectangle();
                    fill->color = colors[i];
                    fill->renderRectAutosize = false;
                    fill->rectDest = amp::csRect{
                        0,
                        amp::to_coord(i * lineHeight),
                        amp::to_size(matrixWidth),
                        amp::to_size(lineHeight)
                    };
                    effectManager.add(fill);
                }
                break;
            }
        case 204:
            {
                *name_ptr = F("GradientWavesFP");
                effectManager.add(new csRenderGradientWavesFP());
                break;
            }
        case 205:
            {
                *name_ptr = F("Plasma");
                effectManager.add(new csRenderPlasma());
                break;
            }
        
        // SimpleClock_DBG effects (301-302)
        case 301:
            {
                *name_ptr = F("Clock");
                // Get font dimensions for clock size calculation
                const auto& font = amp::getStaticFontTemplate<amp::csFont3x5Digits>();
                const tMatrixPixelsSize fontWidth = static_cast<tMatrixPixelsSize>(font.width());
                const tMatrixPixelsSize fontHeight = static_cast<tMatrixPixelsSize>(font.height());
                constexpr tMatrixPixelsSize spacing = 0; // spacing between digits (no spacing for 12x5 matrix)
                constexpr uint8_t digitCount = 4; // clock displays 4 digits
                
                // Calculate clock rect size: 4 digits without spacing (12 = 3+3+3+3)
                const tMatrixPixelsSize clockWidth = digitCount * fontWidth + (digitCount - 1) * spacing;
                const tMatrixPixelsSize clockHeight = fontHeight;
                
                // Create digitGlyph effect for rendering digits
                auto* digitGlyph = new csRenderDigitalClockDigit();
                digitGlyph->setFont(font);
                digitGlyph->color = csColorRGBA{255, 255, 255, 255};
                digitGlyph->backgroundColor = csColorRGBA{255, 0, 0, 0};
                digitGlyph->renderRectAutosize = false;
                
                // Create clock effect
                auto* clock = new csRenderDigitalClock();
                
                // Set renderDigit via propRenderDigit property
                clock->renderDigit = digitGlyph;
                
                // Notify clock that propRenderDigit property changed
                // This will validate the glyph type and update its matrix if needed
                clock->propChanged(amp::csRenderDigitalClock::propRenderDigit);
                
                clock->renderRectAutosize = false;
                clock->rectDest = amp::csRect{0, 0, amp::to_size(clockWidth), amp::to_size(clockHeight)};
                clock->spacing = 0;
                
                // Add effects to manager (clock first, then digitGlyph for proper cleanup)
                effectManager.add(clock);
                effectManager.add(digitGlyph);
                break;
            }
        case 302:
            {
                *name_ptr = F("DigitGlyph");
                const auto& font = amp::getStaticFontTemplate<amp::csFont3x5Digits>();
                auto* digitGlyph = new csRenderDigitalClockDigit();
                digitGlyph->setFont(font);
                digitGlyph->color = csColorRGBA{255, 255, 255, 255};
                digitGlyph->backgroundColor = csColorRGBA{255, 0, 0, 0};
                digitGlyph->renderRectAutosize = false;
                effectManager.add(digitGlyph);
                break;
            }
        default:
            ;
    }
}

#endif // EFFECT_PRESETS_HPP
