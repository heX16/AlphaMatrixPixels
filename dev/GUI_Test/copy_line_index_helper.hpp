#pragma once

#include <SDL2/SDL.h>
#include "../../src/matrix_pixels.hpp"
#include "../../src/render_pipes.hpp"
#include "../../src/render_base.hpp"
#include "../../src/driver_sdl2.hpp"
#include "remap_config.hpp"

using amp::csRenderRemap1DByConstArray;
using amp::csMatrixPixels;
using amp::tMatrixPixelsSize;
using amp::tMatrixPixelsCoord;
using amp::csRandGen;
using amp::csColorRGBA;
using amp::RenderLayout;

// Debug visualization for 2D->1D matrix remapping.
// Displays 1D matrix overlay at bottom when isActive=true.
//
// Design: Main program always calls render() - helper checks isActive internally.
// This keeps main code simple and independent of debug mode.
//
// How it works:
//   - Internal 1D matrix stores remapped result from external 2D matrix via remap effect
//   - Remap effect reads from source matrix and writes to matrix1D according to remap array
//   - When active, 1D matrix is rendered as overlay at bottom of screen
//
// Important:
//   - Call order: updateCopyLineIndexSource() AFTER effects render, BEFORE render()
//   - Only reads pixels from rectSource area (by default entire matrix, but remap array limits the area)
//   - Not thread-safe
class csCopyLineIndexHelper {
public:
    // Debug mode flag. Toggle to show/hide 1D overlay.
    bool isActive = false;

    // 1D matrix storing remapped result. Created in constructor, updated every frame when active.
    csMatrixPixels matrix1D{0, 0};

    // Remap effect: reads from external 2D matrix (set via configureRemapEffect), writes to matrix1D.
    csRenderRemap1DByConstArray* remapEffect = nullptr;

    // Source matrix pointer (set via configureRemapEffect)
    csMatrixPixels* matrixSource_ = nullptr;

        // Creates 1D matrix and configures remap effect. Effect ready but inactive until isActive=true.
    csCopyLineIndexHelper() {

        // Create 1D destination matrix
        matrix1D = csMatrixPixels{cRemapDestMatrixLen, 1};

        // Create remap effect
        remapEffect = new csRenderRemap1DByConstArray();

        // Configure remap - matrixSource must be set via configureRemapEffect() after construction
    }

    ~csCopyLineIndexHelper() {
        if (remapEffect) {
            delete remapEffect;
            remapEffect = nullptr;
        }
    }

    // Update remap source and render to fill matrix1D.
    // Call AFTER effects render into source matrix, BEFORE render().
    // Only reads from rectSource area of source matrix (by default entire matrix bounds).
    void updateCopyLineIndexSource(csRandGen& randGen, amp::tTime currTime) {
        if (!isActive || !remapEffect || !matrixSource_) {
            return;
        }
        // Update matrixSource to point to stored matrix
        remapEffect->matrixSource = matrixSource_;
        // Render remap effect to fill matrix1D
        remapEffect->render(randGen, currTime);
    }

    // Render 1D overlay at bottom. Always call every frame - checks isActive internally.
    // Does not clear screen or call SDL_RenderPresent (overlay rendering).
    void render(SDL_Renderer* renderer, int screenWidth, int screenHeight) const {
        if (!isActive || !renderer || matrix1D.width() == 0 || matrix1D.height() == 0) {
            return;
        }

        // Calculate layout using calculateLayout for step, fill, and offsX
        RenderLayout layout = amp::calculateLayout(matrix1D, screenWidth, screenHeight);

        // Override offsY to position matrix at bottom
        constexpr int padding = 10;
        const int totalH = layout.step * static_cast<int>(matrix1D.height());
        layout.offsY = screenHeight - (totalH + padding);

        // Render using renderMatrixToSDL with custom layout
        amp::renderMatrixToSDL(matrix1D, renderer, screenWidth, screenHeight, false, false, &layout);
    }

    // Configure remap effect settings
    // matrixSource will be stored internally and used in updateCopyLineIndexSource
    void configureRemapEffect(csMatrixPixels* matrixSource) {
        if (!remapEffect) {
            return;
        }
        remapEffect->matrixDest = &matrix1D;
        remapEffect->renderRectAutosize = false;
        remapEffect->remapArray = cRemapSrcArray;
        remapEffect->remapWidth = cSrcWidth;
        remapEffect->remapHeight = cSrcHeight;
        remapEffect->rewrite = true;
        //remapEffect->rectSource.x = 2;
        //remapEffect->rectSource.y = 2;
        // TODO: WIP!!!
        remapEffect->rectSource.x = 0;
        remapEffect->rectSource.y = 0;
        remapEffect->rectSource.height = cSrcHeight;
        remapEffect->rectSource.width = cSrcWidth;
        // Store matrixSource internally
        matrixSource_ = matrixSource;
        remapEffect->matrixSource = matrixSource;
    }
};

