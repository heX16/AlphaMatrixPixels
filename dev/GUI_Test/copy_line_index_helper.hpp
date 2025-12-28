#pragma once

#include <SDL2/SDL.h>
#include "../../src/matrix_pixels.hpp"
#include "../../src/matrix_render_pipes"
#include "../../src/matrix_render.hpp"
#include "../../src/driver_sdl2.hpp"

using amp::csRenderRemap1DByConstArray;
using amp::csMatrixPixels;
using amp::tMatrixPixelsSize;
using amp::tMatrixPixelsCoord;
using amp::csRandGen;
using amp::csColorRGBA;
using amp::RenderLayout;

// Helper class for csRenderRemap1DByConstArray functionality
class csCopyLineIndexHelper {
public:
    // Debug mode state
    bool isActive = false;
    
    // 1D matrix for displaying remap result
    csMatrixPixels matrix1D{0, 0};
    
    // Remap effect pointer
    csRenderRemap1DByConstArray* remapEffect = nullptr;
    // 2x2 remap array: src(x,y) -> dst x coordinate (1D matrix, y is always 0)
    // Layout (row-major order):
    //   src(0,0) -> dst x=0
    //   src(1,0) -> dst x=1
    //   src(0,1) -> dst x=2
    //   src(1,1) -> dst x=3
    static constexpr tMatrixPixelsCoord remapArray2x2[4] = {
        0,  // src(0,0) -> dst x=0
        1,  // src(1,0) -> dst x=1
        2,  // src(0,1) -> dst x=2
        3   // src(1,1) -> dst x=3
    };

    // Constructor: initialize 1D matrix and remap effect
    csCopyLineIndexHelper() {
        // Configure remap with 2x2 array
        constexpr tMatrixPixelsSize remapWidth = 2;
        constexpr tMatrixPixelsSize remapHeight = 2;
        const tMatrixPixelsSize dstWidth = remapWidth * remapHeight; // 4 for 2x2
        
        // Create 1D destination matrix
        matrix1D = csMatrixPixels{dstWidth, 1};
        
        // Create remap effect
        remapEffect = new csRenderRemap1DByConstArray();
        
        // Configure remap
        // matrixSource will be set to external 2D matrix in updateCopyLineIndexSource
        remapEffect->matrix = &matrix1D;
        remapEffect->remapArray = remapArray2x2;
        remapEffect->remapWidth = remapWidth;
        remapEffect->remapHeight = remapHeight;
        remapEffect->renderRectAutosize = true;
    }
    
    // Destructor: cleanup remap effect
    ~csCopyLineIndexHelper() {
        if (remapEffect) {
            delete remapEffect;
            remapEffect = nullptr;
        }
    }

    // Update matrixSource for csRenderRemap1DByConstArray in render loop
    // Also calls render to fill matrix1D
    void updateCopyLineIndexSource(csMatrixPixels& matrix, csRandGen& randGen, uint16_t currTime) {
        if (!isActive || !remapEffect) {
            return;
        }
        
        // Update matrixSource to point to matrix (which now contains effect2 result)
        remapEffect->matrixSource = &matrix;
        remapEffect->rectSource = matrix.getRect();
        
        // Render remap effect to fill matrix1D
        remapEffect->render(randGen, currTime);
    }
    
    // Render 1D matrix at the bottom of the screen (overlay)
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

