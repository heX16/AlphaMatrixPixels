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

inline void loadEffectPresetLocal(csEffectManager& effectManager, uint16_t effectId, csMatrixPixels* matrixSecondBuffer = nullptr) {
    // Only effects listed here are compiled into binary
    switch (effectId) {
        EFFECT_CASE_LIST(1, 2, 3, 5, 116, 117, 118, 119, 120)
        default:
            break;
    }
}

#define EFFECT_PRESETS_LOCAL_MACROS_UNDEF
#include "effect_presets_local_macros.hpp"
#undef EFFECT_PRESETS_LOCAL_MACROS_UNDEF

#endif // EFFECT_PRESETS_LOCAL_HPP
