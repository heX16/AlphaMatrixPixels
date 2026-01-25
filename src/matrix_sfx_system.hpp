// Matrix SFX System - container class that manages csMatrixPixels and csEffectManager lifecycle
#ifndef MATRIX_SFX_SYSTEM_HPP
#define MATRIX_SFX_SYSTEM_HPP

#include "matrix_pixels.hpp"
#include "effect_manager.hpp"
#include "rand_gen.hpp"
#include "matrix_types.hpp"
#include "render_base.hpp"

namespace amp {

// TODO: `csMatrixSFXSystem` as `csRender` - WIP... !

// TODO: `csMatrixSFXSystem` as `csRender` - add the `virtual_matrix`.
// режим когда матрица создается перед отрисовкой, и затем удаляется, чтобы не занимать место
// впрочем имеет смысл сделать особый режим у самой `Matrix` - чтобы она освобождала буффер `pixels_` по запросу,
// и затем повторно его создавала когда нужно.

// TODO: `csMatrixSFXSystem` rename `csRenderSubMatrix`
// TODO: `matrix_sfx_system.hpp` rename `render_sub_matrix.hpp`

// Container class that manages csMatrixPixels and csEffectManager lifecycle.
// Creates both objects in constructor via virtual factory methods and destroys them in destructor.
class csMatrixSFXSystem : public csRenderMatrixBase {
public:
    // Delete copy constructor and assignment operator to prevent shallow copy of pointers
    csMatrixSFXSystem(const csMatrixSFXSystem&) = delete;
    csMatrixSFXSystem& operator=(const csMatrixSFXSystem&) = delete;

    // Delete move constructor and move assignment
    csMatrixSFXSystem(csMatrixSFXSystem&& other) = delete;
    csMatrixSFXSystem& operator=(csMatrixSFXSystem&& other) = delete;

public:
    /*
    // TODO:
    подумать какие тут имеет смысл добавить или активировать Property.

    Нужно добавить property:
    - `virtual_matrix`.
    - `clean_matrix`

    */

    // Construct matrix system with empty matrix (0x0).
    // Creates matrix and effect manager via virtual factory methods, and binds matrix to manager.
    csMatrixSFXSystem()
        : csRenderMatrixBase()  // matrixDest will be nullptr (external matrix not used yet)
        , effectManager(createEffectManager())
        , randGen(csRandGen::RAND16_SEED) {
        // Create internal matrix
        internalMatrix = createMatrix(0, 0);

        // effectManager works with internalMatrix directly
        if (effectManager && internalMatrix) {
            effectManager->setMatrix(*internalMatrix);
        }
    }

    // Construct matrix system with given matrix size.
    // Creates matrix and effect manager via virtual factory methods, and binds matrix to manager.
    csMatrixSFXSystem(tMatrixPixelsSize width, tMatrixPixelsSize height)
        : csRenderMatrixBase()  // matrixDest will be nullptr (external matrix not used yet)
        , effectManager(createEffectManager())
        , randGen(csRandGen::RAND16_SEED) {
        // Create internal matrix
        internalMatrix = createMatrix(width, height);

        // effectManager works with internalMatrix directly
        if (effectManager && internalMatrix) {
            effectManager->setMatrix(*internalMatrix);
        }
    }

    // Virtual destructor: destroys both objects.
    virtual ~csMatrixSFXSystem() {
        if (effectManager) {
            delete effectManager;
            effectManager = nullptr;
        }
        if (internalMatrix) {
            delete internalMatrix;
            internalMatrix = nullptr;
        }
    }

    // Public fields: direct access to internal matrix and effect manager pointers.
    // Base class field 'matrixDest' is used for external matrix reference (not used yet).
    csMatrixPixels* internalMatrix = nullptr;
    mutable csEffectManager* effectManager;  // mutable: render() is const but needs to call non-const effectManager->render()

    /*
    // TODO: для работы recalc и render нужно использовать эту переменную
    но при этом `recalc(csRandGen& rand`
    этот входящий rand тоже может пригодиться - как источник случайности для нашего генератора.
    тут нужно подумать...
    */
    mutable csRandGen randGen;

    // Override csEffectBase::recalc - recalc all effects (update internal state, no rendering).
    void recalc(csRandGen& rand, tTime currTime) override {
        if (effectManager) {
            effectManager->recalc(randGen, currTime);
        }
    }

    // Override csEffectBase::render - render all effects and call onFrameDone for post-frame effects.
    void render(csRandGen& rand, tTime currTime) const override {
        if (effectManager) {
            effectManager->render(randGen, currTime);
        }
    }

    // Convenience method: recalc and render all effects in one call.
    void recalcAndRender(tTime currTime) {
        recalc(randGen, currTime);
        render(randGen, currTime);
    }

    // Delete current internal matrix. Effect manager reference is not updated (caller should handle this).
    void deleteMatrix() {
        if (internalMatrix) {
            delete internalMatrix;
            internalMatrix = nullptr;
        }
    }

    // Set external matrix pointer (base class field 'matrixDest').
    // This sets the external matrix reference, not the internal matrix.
    void setMatrix(csMatrixPixels* m) {
        matrixDest = m;  // Base class field - external matrix
        propChanged(propMatrixDest);  // Call base class propChanged
    }

    // Virtual factory method for creating matrix. Override to customize matrix creation.
    virtual csMatrixPixels* createMatrix(tMatrixPixelsSize width, tMatrixPixelsSize height) {
        if (width == 0 || height == 0) {
            return nullptr;
        }
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

#endif // MATRIX_SFX_SYSTEM_HPP
