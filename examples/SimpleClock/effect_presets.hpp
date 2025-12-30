// Helper function for adding effect presets to effect manager
#ifndef EFFECT_PRESETS_HPP
#define EFFECT_PRESETS_HPP

#include <stdint.h>
#include "../src/effect_manager.hpp"
#include "../src/matrix_pixels.hpp"
#include "../src/color_rgba.hpp"
#include "../src/matrix_render.hpp"
#include "../src/matrix_render_efffects.hpp"
#include "../src/fonts.h"

using amp::csColorRGBA;
using amp::csMatrixPixels;
using amp::tMatrixPixelsSize;
using amp::csRenderDigitalClock;
using amp::csRenderDigitalClockDigit;
using amp::csRenderFill;

// Abstract function: adds effects to the array based on effect ID
// effectManager: reference to effect manager for adding effects
// matrix: reference to matrix (used for some effects)
// effectId: ID of the effect to create
void loadEffectPreset(csEffectManager& effectManager, csMatrixPixels& matrix, uint8_t effectId) {
    if (effectId == 0) {
        return;
    }

    switch (effectId) {
        case 1: // Clock
        case 2: // Clock negative
            {
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

                if (effectId == 0) {
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
        case 3: // 5 horizontal lines with different colors
            {
                const tMatrixPixelsSize matrixWidth = matrix.width();
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
                    auto* fill = new csRenderFill();
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
        default:
            ;
    }
}

#endif // EFFECT_PRESETS_HPP

