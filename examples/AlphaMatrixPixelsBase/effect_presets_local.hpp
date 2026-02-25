/* 
Local wrapper for effect presets - reduces binary size by including only used effects.

Why: The main loadEffectPreset() function contains all effects (30+), which increases
binary size. By limiting the switch to only used effects, the compiler can optimize away
unused code, significantly reducing firmware size on memory-constrained Arduino devices.

Usage:
  1. Include: #include "effect_presets_local.hpp"
  2. Call: loadEffectPresetLocal(*system.effectManager, 202);
  3. To specify effect list: edit `EFFECT_CASE_LIST(...)` in `loadEffectPresetLocal()`,
     add effect IDs as arguments: `EFFECT_CASE_LIST(202, 205, 201)`

Example:
  loadEffectPresetLocal(*system.effectManager, 205); // Plasma
  loadEffectPresetLocal(*system.effectManager, 202); // Clock negative

Example:
```
    inline void loadEffectPresetLocal(csEffectManager& effectManager, uint16_t effectId, csMatrixPixels* matrixSecondBuffer = nullptr) {
        // Only effects listed here are compiled into binary
        switch (effectId) {
            EFFECT_CASE_LIST(111, 112, 113, 114)
            EFFECT_CASE_LIST(201, 202, 203, 204, 205)
            default:
                break;
        }
    }
```
 */

#ifndef EFFECT_PRESETS_LOCAL_HPP
#define EFFECT_PRESETS_LOCAL_HPP

#include "effect_presets.hpp"
#include "effect_presets_local_macros.hpp"

/*
 * Conditional build:
 * - Default: full set of effects (cEffectsCount = 8, loadEffectByIndexLocal switches 1..8).
 * - Define ALPHAMATRIX_SINGLE_EFFECT_FLAME before including this file for minimal build:
 *   cEffectsCount = 1, loadEffectByIndexLocal always loads preset 120 (Flame).
 */
#ifdef ALPHAMATRIX_SINGLE_EFFECT_FLAME
constexpr uint8_t cEffectsCount = 1;
#else
constexpr uint8_t cEffectsCount = 8;
#endif

inline void loadEffectPresetLocal(csEffectManager& effectManager, uint16_t effectId, csMatrixPixels* matrixSecondBuffer = nullptr) {
    // Only effects listed here are compiled into binary
    switch (effectId) {
        EFFECT_CASE_LIST(1, 2, 3, 5, 116, 117, 118, 119, 120)
        default:
            break;
    }
}

#ifdef ALPHAMATRIX_SINGLE_EFFECT_FLAME
inline void loadEffectByIndexLocal(csEffectManager& effectManager, uint8_t /* effectIndex */) {
    loadEffectPresetLocal(effectManager, 120); // Flame
}
#else
inline void loadEffectByIndexLocal(csEffectManager& effectManager, uint8_t effectIndex) {
    switch (effectIndex) {
        case 1:
            loadEffectPresetLocal(effectManager, 3); // Snowfall
            break;
        case 2:
            loadEffectPresetLocal(effectManager, 2); // GradientWaves
            break;
        case 3:
            loadEffectPresetLocal(effectManager, 1); // Plasma
            break;
        case 4:
            loadEffectPresetLocal(effectManager, 116); // 5 BouncingPixels with different bright colors
            break;
        case 5:
            loadEffectPresetLocal(effectManager, 117); // RandomFlashPoint
            break;
        case 6:
            loadEffectPresetLocal(effectManager, 118); // RandomFlashPointOverlay
            break;
        case 7:
            loadEffectPresetLocal(effectManager, 119); // 5 BouncingPixels Fading
            break;
        case 8:
            loadEffectPresetLocal(effectManager, 120); // Flame
            break;
        default:
            break;
    }
}
#endif

#define EFFECT_PRESETS_LOCAL_MACROS_UNDEF
#include "effect_presets_local_macros.hpp"
#undef EFFECT_PRESETS_LOCAL_MACROS_UNDEF

#endif // EFFECT_PRESETS_LOCAL_HPP
