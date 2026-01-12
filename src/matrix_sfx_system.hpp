// Matrix SFX System - container class that manages csMatrixPixels and csEffectManager lifecycle
#ifndef MATRIX_SFX_SYSTEM_HPP
#define MATRIX_SFX_SYSTEM_HPP

#include "matrix_pixels.hpp"
#include "effect_manager.hpp"
#include "rand_gen.hpp"
#include "matrix_types.hpp"

namespace amp {

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
