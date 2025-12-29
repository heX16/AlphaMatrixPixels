// Helper function for adding effect sets to effect manager
#ifndef EFFECT_SET_HELPER_HPP
#define EFFECT_SET_HELPER_HPP

#include <stdint.h>
#include "../../src/effect_manager.hpp"
#include "../../src/matrix_pixels.hpp"
#include "../../src/color_rgba.hpp"
#include "../../src/matrix_render.hpp"
#include "../../src/matrix_render_efffects.hpp"
#include "../../src/fixed_point.hpp"
#include "../../src/fonts.h"

using amp::csColorRGBA;
using amp::csMatrixPixels;
using amp::tMatrixPixelsSize;
using amp::csRenderGradientWaves;
using amp::csRenderGradientWavesFP;
using amp::csRenderPlasma;
using amp::csRenderGlyph;
using amp::csRenderCircleGradient;
using amp::csRenderSnowfall;
using amp::csRenderDigitalClock;
using amp::csRenderFill;
using amp::csRenderAverageArea;

// Abstract function: adds effects to the array based on effect ID
// effectManager: reference to effect manager for adding effects
// matrix: reference to matrix (used for some effects like AverageArea)
// effectId: ID of the effect to create
// isBaseEffect: true for base effects (eff1_base), false for secondary effects (eff2)
void addEffectSet(csEffectManager& effectManager, csMatrixPixels& matrix, uint8_t effectId, bool isBaseEffect) {
    if (effectId == 0) {
        return;
    }

    if (isBaseEffect) {
        // Create base effect based on number
        switch (effectId) {
            case 1:
                effectManager.add(new csRenderGradientWaves());
                break;
            case 2:
                effectManager.add(new csRenderGradientWavesFP());
                break;
            case 3:
                effectManager.add(new csRenderPlasma());
                break;
            case 4:
                effectManager.add(new csRenderSnowfall());
                break;
            default:
                ;
        }
    } else {
        // Create secondary effect based on number
        switch (effectId) {
            case 1: {
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
            case 2: {
                auto* circle = new csRenderCircleGradient();
                circle->color = csColorRGBA{255, 255, 255, 255};
                circle->backgroundColor = csColorRGBA{0, 0, 0, 0};
                circle->gradientOffset = 127;
                circle->renderRectAutosize = true; // использовать весь rect матрицы
                effectManager.add(circle);
                break;
            }
            case 3: {
                // Get font dimensions for clock size calculation
                const auto& font = amp::getStaticFontTemplate<amp::csFont4x7DigitClock>();
                const tMatrixPixelsSize fontWidth = static_cast<tMatrixPixelsSize>(font.width());
                const tMatrixPixelsSize fontHeight = static_cast<tMatrixPixelsSize>(font.height());
                constexpr tMatrixPixelsSize spacing = 1; // spacing between digits
                constexpr uint8_t digitCount = 4; // clock displays 4 digits
                
                // Calculate clock rect size: 4 digits + 3 spacings between them
                const tMatrixPixelsSize clockWidth = digitCount * fontWidth + (digitCount - 1) * spacing;
                const tMatrixPixelsSize clockHeight = fontHeight;
                
                // Create fill effect (background) - covers clock rect
                auto* fill = new csRenderFill();
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
                if (auto* digitalClock = dynamic_cast<amp::csRenderDigitalClock*>(clock)) {
                    digitalClock->propChanged(amp::csRenderDigitalClock::propRenderDigit);
                }
                
                clock->renderRectAutosize = false;
                clock->rectDest = amp::csRect{2, 2, amp::to_size(clockWidth+1), amp::to_size(clockHeight+1)};
                
                // Add effects to array (fill first, then clock on top, then digitGlyph for proper cleanup)
                effectManager.add(fill);
                effectManager.add(clock);
                effectManager.add(digitGlyph);
                break;
            }
            case 4: {
                auto* averageArea = new csRenderAverageArea();
                averageArea->matrix = &matrix;
                averageArea->matrixSource = &matrix;
                averageArea->renderRectAutosize = false;
                averageArea->rectSource = amp::csRect{1, 1, 4, 4};
                averageArea->rectDest = amp::csRect{1, 1, 4, 4};
                effectManager.add(averageArea);
                break;
            }
            case 255:
                ; // slip - remove "effect2"
            default:
                ;
        }
    }
}

#endif // EFFECT_SET_HELPER_HPP

