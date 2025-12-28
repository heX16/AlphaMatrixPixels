#pragma once

#include <SDL2/SDL.h>
#include "../../src/matrix_pixels.hpp"
#include "../../src/matrix_render_pipes.hpp"
#include "../../src/matrix_render.hpp"
#include "../../src/driver_sdl2.hpp"

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

    // Remap effect: reads from external 2D matrix (set via updateCopyLineIndexSource), writes to matrix1D.
    csRenderRemap1DByConstArray* remapEffect = nullptr;
    // Remap array: src(x,y) -> dst x (row-major order)
    static constexpr tMatrixPixelsSize RemapIndexLen = 133; // 19x7
    static constexpr tMatrixPixelsCoord remapArray[133] = {
        0, 6, 5, 0, 0, 0, 20, 19, 0, 0, 0, 34, 33, 0, 0, 0, 48, 47, 0,
        7, 0, 0, 4, 0, 21, 0, 0, 18, 0, 35, 0, 0, 32, 0, 49, 0, 0, 46,
        8, 0, 0, 3, 0, 22, 0, 0, 17, 0, 36, 0, 0, 31, 0, 50, 0, 0, 45,
        0, 1, 2, 0, 0, 0, 15, 16, 0, 0, 0, 29, 30, 0, 0, 0, 43, 44, 0,
        9, 0, 0, 14, 0, 23, 0, 0, 28, 0, 37, 0, 0, 42, 0, 51, 0, 0, 56,
        10, 0, 0, 13, 0, 24, 0, 0, 27, 0, 38, 0, 0, 41, 0, 52, 0, 0, 55,
        0, 11, 12, 0, 0, 0, 25, 26, 0, 0, 0, 39, 40, 0, 0, 0, 53, 54, 0,
    };

    static constexpr tMatrixPixelsSize RemapDestMatrixLen = 56;

    // Configure remap array dimensions
    static constexpr tMatrixPixelsSize remapWidth = 19;
    static constexpr tMatrixPixelsSize remapHeight = 7;

        // Creates 1D matrix and configures remap effect. Effect ready but inactive until isActive=true.
    csCopyLineIndexHelper() {

        // Create 1D destination matrix
        matrix1D = csMatrixPixels{RemapDestMatrixLen, 1};

        // Create remap effect
        remapEffect = new csRenderRemap1DByConstArray();

        // Configure remap
        // matrixSource will be set to external 2D matrix in updateCopyLineIndexSource
        remapEffect->matrix = &matrix1D;
        remapEffect->renderRectAutosize = false;
        remapEffect->remapArray = remapArray;
        remapEffect->remapWidth = remapWidth;
        remapEffect->remapHeight = remapHeight;
        remapEffect->rewrite = true;
        remapEffect->rectSource.x = 2;
        remapEffect->rectSource.y = 2;
        remapEffect->rectSource.height = remapHeight;
        remapEffect->rectSource.width = remapWidth;
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
    void updateCopyLineIndexSource(csMatrixPixels& matrix, csRandGen& randGen, amp::tTime currTime) {
        if (!isActive || !remapEffect) {
            return;
        }
        // Update matrixSource to point to matrix
        remapEffect->matrixSource = &matrix;
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
};

