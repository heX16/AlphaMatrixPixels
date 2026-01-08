/*

Why: The main loadEffectPreset() function contains all effects (30+), which increases
binary size. By limiting the switch to only used effects, the compiler can optimize away
unused code, significantly reducing firmware size on memory-constrained Arduino devices.

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

// Macro definitions for effect presets local wrapper
// These macros generate switch case statements for effect IDs
#ifndef EFFECT_PRESETS_LOCAL_MACROS_HPP
#define EFFECT_PRESETS_LOCAL_MACROS_HPP

// Macro generates proxy case statement calling `loadEffectPreset`
#define EFFECT_CASE(id) \
    case id: \
        loadEffectPreset(effectManager, id, matrixSecondBuffer); \
        break;
        
// Variadic macro for generating multiple effect cases from a list
// Supports 1-10 effects (extend EFFECT_CASE_LIST_N macros as needed)
#define EFFECT_CASE_LIST_1(id1) \
    EFFECT_CASE(id1)

#define EFFECT_CASE_LIST_2(id1, id2) \
    EFFECT_CASE(id1) \
    EFFECT_CASE(id2)

#define EFFECT_CASE_LIST_3(id1, id2, id3) \
    EFFECT_CASE(id1) \
    EFFECT_CASE(id2) \
    EFFECT_CASE(id3)

#define EFFECT_CASE_LIST_4(id1, id2, id3, id4) \
    EFFECT_CASE(id1) \
    EFFECT_CASE(id2) \
    EFFECT_CASE(id3) \
    EFFECT_CASE(id4)

#define EFFECT_CASE_LIST_5(id1, id2, id3, id4, id5) \
    EFFECT_CASE(id1) \
    EFFECT_CASE(id2) \
    EFFECT_CASE(id3) \
    EFFECT_CASE(id4) \
    EFFECT_CASE(id5)

#define EFFECT_CASE_LIST_6(id1, id2, id3, id4, id5, id6) \
    EFFECT_CASE(id1) \
    EFFECT_CASE(id2) \
    EFFECT_CASE(id3) \
    EFFECT_CASE(id4) \
    EFFECT_CASE(id5) \
    EFFECT_CASE(id6)

#define EFFECT_CASE_LIST_7(id1, id2, id3, id4, id5, id6, id7) \
    EFFECT_CASE(id1) \
    EFFECT_CASE(id2) \
    EFFECT_CASE(id3) \
    EFFECT_CASE(id4) \
    EFFECT_CASE(id5) \
    EFFECT_CASE(id6) \
    EFFECT_CASE(id7)

#define EFFECT_CASE_LIST_8(id1, id2, id3, id4, id5, id6, id7, id8) \
    EFFECT_CASE(id1) \
    EFFECT_CASE(id2) \
    EFFECT_CASE(id3) \
    EFFECT_CASE(id4) \
    EFFECT_CASE(id5) \
    EFFECT_CASE(id6) \
    EFFECT_CASE(id7) \
    EFFECT_CASE(id8)

#define EFFECT_CASE_LIST_9(id1, id2, id3, id4, id5, id6, id7, id8, id9) \
    EFFECT_CASE(id1) \
    EFFECT_CASE(id2) \
    EFFECT_CASE(id3) \
    EFFECT_CASE(id4) \
    EFFECT_CASE(id5) \
    EFFECT_CASE(id6) \
    EFFECT_CASE(id7) \
    EFFECT_CASE(id8) \
    EFFECT_CASE(id9)

#define EFFECT_CASE_LIST_10(id1, id2, id3, id4, id5, id6, id7, id8, id9, id10) \
    EFFECT_CASE(id1) \
    EFFECT_CASE(id2) \
    EFFECT_CASE(id3) \
    EFFECT_CASE(id4) \
    EFFECT_CASE(id5) \
    EFFECT_CASE(id6) \
    EFFECT_CASE(id7) \
    EFFECT_CASE(id8) \
    EFFECT_CASE(id9) \
    EFFECT_CASE(id10)

// Helper macro to select correct variant based on argument count
#define EFFECT_CASE_LIST(...) \
    EFFECT_CASE_LIST_SELECT(__VA_ARGS__, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1)(__VA_ARGS__)

#define EFFECT_CASE_LIST_SELECT(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, N, ...) \
    EFFECT_CASE_LIST_##N

#endif // EFFECT_PRESETS_LOCAL_MACROS_HPP

#ifdef EFFECT_PRESETS_LOCAL_MACROS_UNDEF
#undef EFFECT_CASE
#undef EFFECT_CASE_LIST
#undef EFFECT_CASE_LIST_SELECT
#undef EFFECT_CASE_LIST_1
#undef EFFECT_CASE_LIST_2
#undef EFFECT_CASE_LIST_3
#undef EFFECT_CASE_LIST_4
#undef EFFECT_CASE_LIST_5
#undef EFFECT_CASE_LIST_6
#undef EFFECT_CASE_LIST_7
#undef EFFECT_CASE_LIST_8
#undef EFFECT_CASE_LIST_9
#undef EFFECT_CASE_LIST_10
#endif // EFFECT_PRESETS_LOCAL_MACROS_UNDEF
