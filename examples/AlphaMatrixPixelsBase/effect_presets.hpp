// Helper function for adding effect presets to effect manager
#ifndef EFFECT_PRESETS_HPP
#define EFFECT_PRESETS_HPP

#include <stdint.h>
#include "../src/effect_manager.hpp"
#include "../src/matrix_pixels.hpp"
#include "../src/color_rgba.hpp"
#include "../src/matrix_render.hpp"
#include "../src/matrix_render_efffects.hpp"
#include "../src/matrix_render_pipes.hpp"
#include "../src/fixed_point.hpp"
#include "../src/fonts.h"

using amp::csColorRGBA;
using amp::csMatrixPixels;
using amp::csRenderPlasma;
using amp::csRenderGradientWaves;
using amp::csRenderSnowfall;
using amp::csRenderGlyph;
using amp::csRenderMatrixCopy;

// Abstract function: adds effects to the array based on effect ID
// effectManager: reference to effect manager for adding effects
// matrix: reference to matrix (used for some effects)
// effectId: ID of the effect to create
// matrixX2: optional reference to 2x matrix (used for case 5)
void loadEffectPreset(csEffectManager& effectManager, csMatrixPixels& matrix, uint8_t effectId, csMatrixPixels* matrixX2 = nullptr) {
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
                snowfall->count = 5;
                snowfall->speed = amp::math::csFP16(1.0f / 4.0f);
                snowfall->propChanged(csRenderSnowfall::propCount);
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
        case 5: // Snowfall (copy) - renders to canvasX2, then copies to canvas via pipe
            {
                if (!matrixX2) {
                    break; // canvasX2 not provided
                }
                
                // Create snowfall effect that renders to canvasX2
                auto* snowfall = new csRenderSnowfall();
                snowfall->color = csColorRGBA{255, 255, 255, 255};
                snowfall->count = 5;
                snowfall->speed = amp::math::csFP16(1.0f / 4.0f);
                snowfall->propChanged(csRenderSnowfall::propCount);
                // Add to manager first (it will set canvas), then override with canvasX2
                effectManager.add(snowfall);
                snowfall->setMatrix(matrixX2);
                
                // Create pipe effect to copy from canvasX2 to canvas
                auto* pipe = new csRenderMatrixCopy();
                pipe->matrixSource = matrixX2;
                pipe->rectSource = matrixX2->getRect();
                pipe->rectDest = matrix.getRect();
                effectManager.add(pipe);
                break;
            }
        default:
            ;
    }
}

#endif // EFFECT_PRESETS_HPP

