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

// TODO: `csMatrixSFXSystem` rename `csRenderSubMatrix`

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
    // Construct matrix system with empty matrix (0x0).
    // Creates matrix and effect manager via virtual factory methods, and binds matrix to manager.
    csMatrixSFXSystem()
        : csRenderMatrixBase()  // matrix will be nullptr (external matrix not used yet)
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
        : csRenderMatrixBase()  // matrix will be nullptr (external matrix not used yet)
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
    // Base class field 'matrix' is used for external matrix reference (not used yet).
    csMatrixPixels* internalMatrix = nullptr;
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

    // Delete current internal matrix. Effect manager reference is not updated (caller should handle this).
    void deleteMatrix() {
        if (internalMatrix) {
            delete internalMatrix;
            internalMatrix = nullptr;
        }
    }

    // Set external matrix pointer (base class field 'matrix').
    // This sets the external matrix reference, not the internal matrix.
    void setMatrix(csMatrixPixels* m) {
        matrix = m;  // Base class field - external matrix
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
