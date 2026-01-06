/* 
 * Local wrapper for effect presets - reduces binary size by including only used effects.
 * 
 * Why: The main loadEffectPreset() function contains all effects (30+), which increases
 * binary size. By limiting the switch to only used effects, the compiler can optimize away
 * unused code, significantly reducing firmware size on memory-constrained Arduino devices.
 * 
 * Usage:
 *   1. Include: #include "effect_presets_local.hpp"
 *   2. Call: loadEffectPresetLocal(*sfxSystem.effectManager, 202);
 *   3. To specify effect list: edit `switch (effectId) {` in `loadEffectPresetLocal()`,
 *      add `EFFECT_CASE(id)` lines for each effect you need
 * 
 * Example:
 *   loadEffectPresetLocal(*sfxSystem.effectManager, 205); // Plasma
 *   loadEffectPresetLocal(*sfxSystem.effectManager, 202); // Clock negative
 */

#ifndef EFFECT_PRESETS_LOCAL_HPP
#define EFFECT_PRESETS_LOCAL_HPP

#include "effect_presets.hpp"

// Macro generates proxy case statement calling `loadEffectPreset`
#define EFFECT_CASE(id) \
    case id: \
        loadEffectPreset(effectManager, id, matrixSecondBuffer); \
        break;

inline void loadEffectPresetLocal(csEffectManager& effectManager, uint16_t effectId, csMatrixPixels* matrixSecondBuffer = nullptr) {
    if (effectId == 0) {
        return;
    }
    
    // Only effects listed here are compiled into binary
    switch (effectId) {
        EFFECT_CASE(202)
        EFFECT_CASE(205)
        default:
            break;
    }
}

#undef EFFECT_CASE

#endif // EFFECT_PRESETS_LOCAL_HPP
