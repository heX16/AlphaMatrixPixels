// Effect manager for managing array of effects
#ifndef EFFECT_MANAGER_HPP
#define EFFECT_MANAGER_HPP

// Use C headers for compatibility with older Arduino IDE versions (AVR)
#include <stddef.h>
#include <stdint.h>
#include <limits.h>
#include "matrix_pixels.hpp"
#include "matrix_render.hpp"
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
    void set(uint8_t index, csEffectBase* eff) {
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

    // Delete effect with safety: clears all references to it from other effects' properties
    // Before deletion, scans all effects and their properties, and if any property
    // of type Effect points to the object being deleted, sets it to nullptr and calls propChanged
    void deleteSlowAndSafety(uint8_t index) {
        if (index >= maxEffects) {
            return;
        }

        csEffectBase* eff = effects[index];
        if (!eff) {
            return;
        }

        // Scan all effects and their properties
        for (uint8_t i = 0; i < maxEffects; ++i) {
            if (effects[i] == nullptr || i == index) {
                continue; // Skip null or the effect being deleted
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
                if ((info.type >= PropType::EffectBase) && (info.ptr != nullptr)) {
                    // Compare pointer value with the effect being deleted
                    csEffectBase** effectPtr = static_cast<csEffectBase**>(info.ptr);
                    if (*effectPtr == eff) {
                        // Found reference to the effect being deleted
                        *effectPtr = nullptr;
                        currentEff->propChanged(propNum);
                    }
                }
            }
        }

        // Clear from array (before deletion)
        effects[index] = nullptr;

        // Now safe to delete the effect
        deleteEffect(eff);
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
        for (uint8_t i = 0; i < maxEffects; ++i) {
            if (effects[i] != nullptr) {
                bindEffectMatrix(effects[i]);
            }
        }
    }

    // Recalc all effects (update internal state, no rendering)
    void recalc(csRandGen& randGen, tTime currTime) {
        for (uint8_t i = 0; i < maxEffects; ++i) {
            if (effects[i] != nullptr) {
                effects[i]->recalc(randGen, currTime);
            }
        }
    }

    // Render all effects and call onFrameDone for post-frame effects
    void render(csRandGen& randGen, tTime currTime) {
        auto isPostFrame = [](csEffectBase* eff) -> bool {
            return eff->queryClassFamily(PropType::EffectPostFrame) != nullptr;
        };

        for (uint8_t i = 0; i < maxEffects; ++i) {
            if (effects[i] != nullptr) {
                effects[i]->render(randGen, currTime);
            }
        }

        if (!matrix) {
            return;
        }

        for (uint8_t i = 0; i < maxEffects; ++i) {
            if (effects[i] != nullptr && isPostFrame(effects[i])) {
                effects[i]->onFrameDone(*matrix, randGen, currTime);
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
    csEffectBase* get(uint8_t index) {
        return (index < maxEffects) ? effects[index] : nullptr;
    }

    const csEffectBase* get(uint8_t index) const {
        return (index < maxEffects) ? effects[index] : nullptr;
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
        return effects;
    }

    csEffectBase** end() {
        return effects + maxEffects;
    }

    const csEffectBase* const* begin() const {
        return effects;
    }

    const csEffectBase* const* end() const {
        return effects + maxEffects;
    }

private:
    csMatrixPixels* matrix = nullptr;
    csEffectBase* effects[maxEffects] = {};

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

    // incapsulation of `delete`
    void deleteEffect(csEffectBase* eff) {
        if (!eff) {
            return;
        }
        delete eff;
    }
};

// Container class that manages csMatrixPixels and csEffectManager lifecycle.
// Creates both objects in constructor via virtual factory methods and destroys them in destructor.
class csMatrixSFXSystem {
public:
    // Delete copy constructor and assignment operator to prevent shallow copy of pointers
    csMatrixSFXSystem(const csMatrixSFXSystem&) = delete;
    csMatrixSFXSystem& operator=(const csMatrixSFXSystem&) = delete;
    
    // Allow move constructor and move assignment
    csMatrixSFXSystem(csMatrixSFXSystem&& other) noexcept
        : matrix(other.matrix)
        , effectManager(other.effectManager)
        , randGen(other.randGen) {
        other.matrix = nullptr;
        other.effectManager = nullptr;
    }
    
    csMatrixSFXSystem& operator=(csMatrixSFXSystem&& other) noexcept {
        if (this != &other) {
            // Delete existing objects
            if (effectManager) {
                delete effectManager;
            }
            if (matrix) {
                delete matrix;
            }
            // Move from other
            matrix = other.matrix;
            effectManager = other.effectManager;
            randGen = other.randGen;
            // Clear other
            other.matrix = nullptr;
            other.effectManager = nullptr;
        }
        return *this;
    }
    
public:
    // Construct matrix system with empty matrix (0x0).
    // Creates matrix and effect manager via virtual factory methods, and binds matrix to manager.
    csMatrixSFXSystem()
        : matrix(createMatrix(0, 0))
        , effectManager(createEffectManager())
        , randGen(csRandGen::RAND16_SEED) {
        if (effectManager) {
            effectManager->setMatrix(*matrix);
        }
    }

    // Construct matrix system with given matrix size.
    // Creates matrix and effect manager via virtual factory methods, and binds matrix to manager.
    csMatrixSFXSystem(tMatrixPixelsSize width, tMatrixPixelsSize height)
        : matrix(createMatrix(width, height))
        , effectManager(createEffectManager())
        , randGen(csRandGen::RAND16_SEED) {
        if (effectManager) {
            effectManager->setMatrix(*matrix);
        }
    }

    // Virtual destructor: destroys both objects.
    virtual ~csMatrixSFXSystem() {
        if (effectManager) {
            delete effectManager;
            effectManager = nullptr;
        }
        if (matrix) {
            delete matrix;
            matrix = nullptr;
        }
    }

    // Public fields: direct access to matrix and effect manager pointers.
    csMatrixPixels* matrix;
    csEffectManager* effectManager;
    csRandGen randGen;

    // Recalc all effects (update internal state, no rendering).
    void recalc(tTime currTime) {
        if (effectManager) {
            effectManager->recalc(randGen, currTime);
        }
    }

    // Render all effects and call onFrameDone for post-frame effects.
    void render(tTime currTime) {
        if (effectManager) {
            effectManager->render(randGen, currTime);
        }
    }

    // Convenience method: recalc and render all effects in one call.
    void recalcAndRender(tTime currTime) {
        recalc(currTime);
        render(currTime);
    }

    // Delete current matrix. Effect manager reference is not updated (caller should handle this).
    void deleteMatrix() {
        if (matrix) {
            delete matrix;
            matrix = nullptr;
        }
    }

    // Set matrix pointer and update effect manager (similar to effectManager->setMatrix).
    void setMatrix(csMatrixPixels* m) {
        matrix = m;
        if (effectManager && matrix) {
            effectManager->setMatrix(*matrix);
        }
    }

    // Virtual factory method for creating matrix. Override to customize matrix creation.
    virtual csMatrixPixels* createMatrix(tMatrixPixelsSize width, tMatrixPixelsSize height) {
        return new csMatrixPixels(width, height);
    }

    // Virtual factory method for creating effect manager. Override to customize manager creation.
    virtual csEffectManager* createEffectManager() {
        return new csEffectManager();
    }

    //TODO: add `update(currTime)` method to update all effects and remap the matrix.
    //  и этот метод должен вызываться в loop(), и использовать таймеры для определения частоты вызова.
};

} // namespace amp


#endif // EFFECT_MANAGER_HPP

