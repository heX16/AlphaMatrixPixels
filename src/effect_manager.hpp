// Effect manager for managing array of effects
#ifndef EFFECT_MANAGER_HPP
#define EFFECT_MANAGER_HPP

#include <cstddef>
#include <cstdint>
#include "matrix_pixels.hpp"
#include "matrix_render.hpp"

class csEffectManager {
public:
    static constexpr size_t maxEffects = 10;

    explicit csEffectManager(amp::csMatrixPixels& matrix) : matrix(matrix) {}

    ~csEffectManager() {
        clearAll();
    }

    // Add effect (returns index or maxEffects if array is full)
    size_t add(amp::csEffectBase* eff) {
        if (!eff) {
            return maxEffects;
        }
        for (size_t i = 0; i < maxEffects; ++i) {
            if (effects[i] == nullptr) {
                effects[i] = eff;
                bindEffectMatrix(eff);
                return i;
            }
        }
        return maxEffects; // Array is full
    }

    // Remove effect by index
    void remove(size_t index) {
        if (index >= maxEffects) {
            return;
        }
        deleteEffect(effects[index]);
        effects[index] = nullptr;
    }

    // Clear all effects
    void clearAll() {
        for (size_t i = 0; i < maxEffects; ++i) {
            deleteEffect(effects[i]);
            effects[i] = nullptr;
        }
    }

    // Bind matrix to all effects
    void bindMatrix() {
        for (size_t i = 0; i < maxEffects; ++i) {
            if (effects[i] != nullptr) {
                bindEffectMatrix(effects[i]);
            }
        }
    }

    // Update and render all effects
    void updateAndRenderAll(amp::csRandGen& randGen, uint32_t ticks, uint16_t currTime) {
        for (size_t i = 0; i < maxEffects; ++i) {
            if (effects[i] != nullptr) {
                effects[i]->recalc(randGen, currTime);
                effects[i]->render(randGen, currTime);
            }
        }
    }

    // Access effects
    amp::csEffectBase* get(size_t index) {
        return (index < maxEffects) ? effects[index] : nullptr;
    }

    const amp::csEffectBase* get(size_t index) const {
        return (index < maxEffects) ? effects[index] : nullptr;
    }

    amp::csEffectBase* operator[](size_t index) {
        return get(index);
    }

    const amp::csEffectBase* operator[](size_t index) const {
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
        if (auto* m = dynamic_cast<amp::csRenderMatrixBase*>(eff)) {
            m->setMatrix(matrix);
        }
    }

    void deleteEffect(amp::csEffectBase* eff) {
        if (!eff) {
            return;
        }
        delete eff;
    }
};

#endif // EFFECT_MANAGER_HPP

