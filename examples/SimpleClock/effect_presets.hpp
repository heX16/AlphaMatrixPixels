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
                
                // Add effects to manager (clock first, then digitGlyph for proper cleanup)
                effectManager.add(clock);
                effectManager.add(digitGlyph);
                break;
            }
        case 2: // DigitGlyph (standalone, if needed)
            {
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

