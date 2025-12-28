// Effect manager for managing array of effects
#ifndef EFFECT_MANAGER_HPP
#define EFFECT_MANAGER_HPP

// Use C headers for compatibility with older Arduino IDE versions (AVR)
#include <stddef.h>
#include <stdint.h>
#include <limits.h>
#include "matrix_pixels.hpp"
#include "matrix_render.hpp"

class csEffectManager {
public:
    static constexpr uint8_t maxEffects = 10;
    static constexpr uint8_t notFound = UINT8_MAX;

    explicit csEffectManager(amp::csMatrixPixels& matrix) : matrix(matrix) {}

    ~csEffectManager() {
        clearAll();
    }

    // Add effect (returns index or notFound if array is full or effect is null)
    uint8_t add(amp::csEffectBase* eff) {
        if (!eff) {
            return notFound;
        }
        uint8_t freeIndex = findFreeSlot();
        if (freeIndex != notFound) {
            effects[freeIndex] = eff;
            bindEffectMatrix(eff);
            return freeIndex;
        }
        return notFound; // Array is full
    }

    // Remove effect by index
    void remove(uint8_t index) {
        if (index >= maxEffects) {
            return;
        }
        deleteEffect(effects[index]);
        effects[index] = nullptr;
    }

    // Set effect at specific index (removes existing effect if present)
    // NOTE: This is the ONLY way to write/set effects. operator[] is read-only.
    void set(uint8_t index, amp::csEffectBase* eff) {
        if (index >= maxEffects) {
            return;
        }
        if (eff == nullptr) {
            remove(index);
            return;
        }
        // Remove existing effect at this index
        deleteEffect(effects[index]);
        // Set new effect
        effects[index] = eff;
        bindEffectMatrix(eff);
    }

    // Clear all effects
    void clearAll() {
        for (uint8_t i = 0; i < maxEffects; ++i) {
            deleteEffect(effects[i]);
            effects[i] = nullptr;
        }
    }

    // Delete effect with safety: clears all references to it from other effects' parameters
    // Before deletion, scans all effects and their parameters, and if any parameter
    // of type Effect points to the object being deleted, sets it to nullptr and calls paramChanged
    void deleteSlowAndSafety(uint8_t index) {
        if (index >= maxEffects) {
            return;
        }

        amp::csEffectBase* eff = effects[index];
        if (!eff) {
            return;
        }

        // Scan all effects and their parameters
        for (uint8_t i = 0; i < maxEffects; ++i) {
            if (effects[i] == nullptr || i == index) {
                continue; // Skip null or the effect being deleted
            }

            amp::csEffectBase* currentEff = effects[i];
            const uint8_t paramCount = currentEff->getParamsCount();

            // Enumerate all parameters
            for (uint8_t paramNum = 1; paramNum <= paramCount; ++paramNum) {
                amp::csParamInfo info;
                currentEff->getParamInfo(paramNum, info);

                // Check if parameter is of type Effect (pointer to effect)
                // All effect family types are >= EffectBase ("Effect*")
                // Using >= ensures we catch all current and future effect family types
                if ((info.type >= amp::ParamType::EffectBase) && (info.ptr != nullptr)) {
                    // Compare pointer value with the effect being deleted
                    amp::csEffectBase** effectPtr = static_cast<amp::csEffectBase**>(info.ptr);
                    if (*effectPtr == eff) {
                        // Found reference to the effect being deleted
                        *effectPtr = nullptr;
                        currentEff->paramChanged(paramNum);
                    }
                }
            }
        }

        // Clear from array (before deletion)
        effects[index] = nullptr;

        // Now safe to delete the effect
        deleteEffect(eff);
    }

    // Bind matrix to all effects
    void bindMatrix() {
        for (uint8_t i = 0; i < maxEffects; ++i) {
            if (effects[i] != nullptr) {
                bindEffectMatrix(effects[i]);
            }
        }
    }

    // Update and render all effects
    void updateAndRenderAll(amp::csRandGen& randGen, uint16_t currTime) {
        for (uint8_t i = 0; i < maxEffects; ++i) {
            if (effects[i] != nullptr) {
                effects[i]->recalc(randGen, currTime);
                effects[i]->render(randGen, currTime);
            }
        }
    }

    // Find first free slot (returns index or notFound if array is full)
    virtual uint8_t findFreeSlot() const {
        for (uint8_t i = 0; i < maxEffects; ++i) {
            if (effects[i] == nullptr) {
                return i;
            }
        }
        return notFound; // Array is full
    }

    // Access effects
    amp::csEffectBase* get(uint8_t index) {
        return (index < maxEffects) ? effects[index] : nullptr;
    }

    const amp::csEffectBase* get(uint8_t index) const {
        return (index < maxEffects) ? effects[index] : nullptr;
    }

    // Read-only access via operator[] (for reading only, use set() for writing)
    amp::csEffectBase* operator[](uint8_t index) {
        return get(index);
    }

    // Read-only access via operator[] (for reading only, use set() for writing)
    const amp::csEffectBase* operator[](uint8_t index) const {
        return get(index);
    }

    // Iterators for range-based for
    amp::csEffectBase** begin() {
        return effects;
    }

    amp::csEffectBase** end() {
        return effects + maxEffects;
    }

    const amp::csEffectBase* const* begin() const {
        return effects;
    }

    const amp::csEffectBase* const* end() const {
        return effects + maxEffects;
    }

private:
    amp::csMatrixPixels& matrix;
    amp::csEffectBase* effects[maxEffects] = {};

    void bindEffectMatrix(amp::csEffectBase* eff) {
        if (!eff) {
            return;
        }
        if (auto* m = static_cast<amp::csRenderMatrixBase*>(
            eff->queryClassFamily(amp::ParamType::EffectMatrixDest)
        )) {
            m->setMatrix(&matrix);
        }
    }

    // incapsulation of `delete`
    void deleteEffect(amp::csEffectBase* eff) {
        if (!eff) {
            return;
        }
        delete eff;
    }
};

#endif // EFFECT_MANAGER_HPP

