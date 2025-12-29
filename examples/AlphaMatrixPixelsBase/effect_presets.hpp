// Helper function for adding effect presets to effect manager
#ifndef EFFECT_PRESETS_HPP
#define EFFECT_PRESETS_HPP

#include <stdint.h>
#include "../src/effect_manager.hpp"
#include "../src/matrix_pixels.hpp"
#include "../src/color_rgba.hpp"
#include "../src/matrix_render.hpp"
#include "../src/matrix_render_efffects.hpp"
#include "../src/fixed_point.hpp"
#include "../src/fonts.h"

using amp::csColorRGBA;
using amp::csMatrixPixels;
using amp::csRenderPlasma;
using amp::csRenderGradientWaves;
using amp::csRenderSnowfall;
using amp::csRenderGlyph;

// Abstract function: adds effects to the array based on effect ID
// effectManager: reference to effect manager for adding effects
// matrix: reference to matrix (used for some effects)
// effectId: ID of the effect to create
void loadEffectPreset(csEffectManager& effectManager, csMatrixPixels& matrix, uint8_t effectId) {
    if (effectId == 0) {
        return;
    }

    switch (effectId) {
        case 1: // Plasma
            {
                auto* plasma = new csRenderPlasma();
                plasma->scale = amp::math::csFP16(0.3f);
                plasma->speed = amp::math::csFP16(1.0f);
                effectManager.add(plasma);
                break;
            }
        case 2: // GradientWaves
            {
                auto* gradientWaves = new csRenderGradientWaves();
                gradientWaves->scale = amp::math::csFP16(0.5f);
                gradientWaves->speed = amp::math::csFP16(1.0f);
                effectManager.add(gradientWaves);
                break;
            }
        case 3: // Snowfall
            {
                auto* snowfall = new csRenderSnowfall();
                snowfall->color = csColorRGBA{255, 255, 255, 255};
                effectManager.add(snowfall);
                break;
            }
        case 4: // Glyph
            {
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
        default:
            ;
    }
}

#endif // EFFECT_PRESETS_HPP

