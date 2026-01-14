// Effect manager for managing array of effects
#ifndef EFFECT_MANAGER_HPP
#define EFFECT_MANAGER_HPP

// Use C headers for compatibility with older Arduino IDE versions (AVR)
#include <stddef.h>
#include <stdint.h>
#include <limits.h>
#include "matrix_pixels.hpp"
#include "render_base.hpp"
#include "rand_gen.hpp"

namespace amp {

class csEffectManager {
public:
    static constexpr uint8_t maxEffects = 10;
    static constexpr uint8_t notFound = UINT8_MAX;

    csEffectManager() = default;

    // Virtual destructor: safe deletion through base pointer in polymorphic class.
    virtual ~csEffectManager() {
        clearAll();
    }

    // Add effect (returns index or notFound if array is full or effect is null)
    uint8_t add(csEffectBase* eff) {
        if (!eff) {
            return notFound;
        }
        // Check if we need to grow the array
        if (effectsCount >= effectsCapacity) {
            uint8_t newCapacity = (effectsCapacity == 0) ? 4 : effectsCapacity * 2;
            if (newCapacity > maxEffects) {
                newCapacity = maxEffects;
            }
            if (newCapacity <= effectsCapacity) {
                return notFound; // Array is full (reached maxEffects)
            }
            resizeArray(newCapacity);
        }
        // Add to the end
        effects[effectsCount] = eff;
        bindEffectMatrix(eff);
        return effectsCount++;
    }

    // Remove effect by index
    void remove(uint8_t index) {
        if (index >= effectsCount) {
            return;
        }
        deleteEffect(effects[index]);
        
        // Shift elements left to maintain compact array
        for (uint8_t i = index; i < effectsCount - 1; ++i) {
            effects[i] = effects[i + 1];
        }
        effectsCount--;
        
        // Shrink array if more than 25% is empty
        // Empty space = effectsCapacity - effectsCount
        // 25% empty means: (effectsCapacity - effectsCount) >= effectsCapacity * 0.25
        // Which is equivalent to: effectsCount <= effectsCapacity * 0.75
        if (effectsCapacity > 0 && effectsCount > 0) {
            uint8_t threshold = (effectsCapacity * 3) / 4; // 75% of capacity
            if (effectsCount <= threshold) {
                uint8_t newCapacity = effectsCount * 2;
                if (newCapacity < 4) {
                    newCapacity = (effectsCount > 0) ? 4 : 0;
                }
                resizeArray(newCapacity);
            }
        } else if (effectsCount == 0 && effectsCapacity > 0) {
            // Free array if empty
            delete[] effects;
            effects = nullptr;
            effectsCapacity = 0;
        }
    }

    // Set effect at specific index (removes existing effect if present)
    // NOTE: This is the ONLY way to write/set effects. operator[] is read-only.
    void set(uint8_t index, csEffectBase* eff) {
        if (index >= effectsCount) {
            return; // Index out of range
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
        for (uint8_t i = 0; i < effectsCount; ++i) {
            deleteEffect(effects[i]);
        }
        if (effects != nullptr) {
            delete[] effects;
            effects = nullptr;
        }
        effectsCount = 0;
        effectsCapacity = 0;
    }

    // Delete effect with safety: clears all references to it from other effects' properties
    // Before deletion, scans all effects and their properties, and if any property
    // of type Effect points to the object being deleted, sets it to nullptr and calls propChanged
    void deleteSlowAndSafety(uint8_t index) {
        if (index >= effectsCount) {
            return;
        }

        csEffectBase* eff = effects[index];
        if (!eff) {
            return;
        }

        // Scan all effects and their properties
        for (uint8_t i = 0; i < effectsCount; ++i) {
            if (i == index) {
                continue; // Skip the effect being deleted
            }

            csEffectBase* currentEff = effects[i];
            const uint8_t propCount = currentEff->getPropsCount();

            // Enumerate all properties
            for (uint8_t propNum = 1; propNum <= propCount; ++propNum) {
                csPropInfo info;
                currentEff->getPropInfo(propNum, info);

                // Check if property is of type Effect (pointer to effect)
                // All effect family types are >= EffectBase ("Effect*")
                // Using >= ensures we catch all current and future effect family types
                if ((info.valueType >= PropType::EffectBase) && (info.valuePtr != nullptr)) {
                    // Compare pointer value with the effect being deleted
                    csEffectBase** effectPtr = static_cast<csEffectBase**>(info.valuePtr);
                    if (*effectPtr == eff) {
                        // Found reference to the effect being deleted
                        *effectPtr = nullptr;
                        currentEff->propChanged(propNum);
                    }
                }
            }
        }

        // Shift elements left to maintain compact array
        for (uint8_t i = index; i < effectsCount - 1; ++i) {
            effects[i] = effects[i + 1];
        }
        effectsCount--;

        // Now safe to delete the effect
        deleteEffect(eff);

        // Shrink array if more than 25% is empty
        // Empty space = effectsCapacity - effectsCount
        // 25% empty means: (effectsCapacity - effectsCount) >= effectsCapacity * 0.25
        // Which is equivalent to: effectsCount <= effectsCapacity * 0.75
        if (effectsCapacity > 0 && effectsCount > 0) {
            uint8_t threshold = (effectsCapacity * 3) / 4; // 75% of capacity
            if (effectsCount <= threshold) {
                uint8_t newCapacity = effectsCount * 2;
                if (newCapacity < 4) {
                    newCapacity = (effectsCount > 0) ? 4 : 0;
                }
                resizeArray(newCapacity);
            }
        } else if (effectsCount == 0 && effectsCapacity > 0) {
            // Free array if empty
            delete[] effects;
            effects = nullptr;
            effectsCapacity = 0;
        }
    }

    // Set matrix and bind to all effects
    void setMatrix(csMatrixPixels& m) {
        matrix = &m;
        bindMatrix();
    }

    // Get matrix pointer
    csMatrixPixels* getMatrix() {
        return matrix;
    }

    const csMatrixPixels* getMatrix() const {
        return matrix;
    }

    // Bind matrix to all effects
    void bindMatrix() {
        if (!matrix) {
            return;
        }
        for (uint8_t i = 0; i < effectsCount; ++i) {
            bindEffectMatrix(effects[i]);
        }
    }

    // Recalc all effects (update internal state, no rendering)
    void recalc(csRandGen& randGen, tTime currTime) {
        for (uint8_t i = 0; i < effectsCount; ++i) {
            effects[i]->recalc(randGen, currTime);
        }
    }

    // Render all effects and call onFrameDone for post-frame effects
    void render(csRandGen& randGen, tTime currTime) {
        auto isPostFrame = [](csEffectBase* eff) -> bool {
            return eff->queryClassFamily(PropType::EffectPostFrame) != nullptr;
        };

        for (uint8_t i = 0; i < effectsCount; ++i) {
            effects[i]->render(randGen, currTime);
        }

        if (!matrix) {
            return;
        }

        for (uint8_t i = 0; i < effectsCount; ++i) {
            if (isPostFrame(effects[i])) {
                effects[i]->onFrameDone(*matrix, randGen, currTime);
            }
        }
    }

    // Find first free slot (returns index or notFound if array is full)
    // NOTE: Not used in current implementation, kept for future use
    virtual uint8_t findFreeSlot() const {
        for (uint8_t i = 0; i < maxEffects; ++i) {
            if (i >= effectsCount || effects[i] == nullptr) {
                return i;
            }
        }
        return notFound; // Array is full
    }

    // Get current number of effects
    uint8_t size() const {
        return effectsCount;
    }

    // Access effects
    csEffectBase* get(uint8_t index) {
        return (index < effectsCount) ? effects[index] : nullptr;
    }

    const csEffectBase* get(uint8_t index) const {
        return (index < effectsCount) ? effects[index] : nullptr;
    }

    // Read-only access via operator[] (for reading only, use set() for writing)
    csEffectBase* operator[](uint8_t index) {
        return get(index);
    }

    // Read-only access via operator[] (for reading only, use set() for writing)
    const csEffectBase* operator[](uint8_t index) const {
        return get(index);
    }

    // Iterators for range-based for
    csEffectBase** begin() {
        return effects; // May be nullptr if array is empty
    }

    csEffectBase** end() {
        return effects + effectsCount; // Safe even if effects is nullptr (effectsCount is 0)
    }

    const csEffectBase* const* begin() const {
        return effects; // May be nullptr if array is empty
    }

    const csEffectBase* const* end() const {
        return effects + effectsCount; // Safe even if effects is nullptr (effectsCount is 0)
    }

private:
    csMatrixPixels* matrix = nullptr;
    csEffectBase** effects = nullptr;
    uint8_t effectsCount = 0;
    uint8_t effectsCapacity = 0;

    void bindEffectMatrix(csEffectBase* eff) {
        if (!eff || !matrix) {
            return;
        }
        if (auto* m = static_cast<csRenderMatrixBase*>(
            eff->queryClassFamily(PropType::EffectMatrixDest)
        )) {
            m->setMatrix(matrix);
        }
    }

    // Resize the effects array to new capacity
    void resizeArray(uint8_t newCapacity) {
        if (newCapacity == 0) {
            if (effects != nullptr) {
                delete[] effects;
                effects = nullptr;
            }
            effectsCapacity = 0;
            return;
        }

        csEffectBase** newEffects = new csEffectBase*[newCapacity];
        if (newEffects == nullptr) {
            return; // Allocation failed
        }

        // Copy existing elements
        uint8_t copyCount = (effectsCount < newCapacity) ? effectsCount : newCapacity;
        for (uint8_t i = 0; i < copyCount; ++i) {
            newEffects[i] = effects[i];
        }

        // Initialize remaining elements to nullptr
        for (uint8_t i = copyCount; i < newCapacity; ++i) {
            newEffects[i] = nullptr;
        }

        // Free old array and update pointers
        if (effects != nullptr) {
            delete[] effects;
        }
        effects = newEffects;
        effectsCapacity = newCapacity;
        if (effectsCount > newCapacity) {
            effectsCount = newCapacity;
        }
    }

    // incapsulation of `delete`
    void deleteEffect(csEffectBase* eff) {
        if (!eff) {
            return;
        }
        delete eff;
    }
};

} // namespace amp


#endif // EFFECT_MANAGER_HPP

