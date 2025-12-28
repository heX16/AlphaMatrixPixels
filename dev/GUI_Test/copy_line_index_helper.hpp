#pragma once

#include "../../src/matrix_pixels.hpp"
#include "../../src/matrix_render_pipes"

using amp::csRenderRemapByConstArray;
using amp::csMatrixPixels;
using amp::tMatrixPixelsSize;

// Helper class for csRenderRemapByConstArray functionality
class csCopyLineIndexHelper {
public:
    // 2x2 remap array: src(x,y) -> dst coordinates
    // Layout:
    //   (0, 0) (0, 1)
    //   (1, 0) (1, 1)
    static constexpr csRenderRemapByConstArray::RemapCoord remapArray2x2[4] = {
        {0, 0},  // src(0,0) -> dst(0,0)
        {0, 1},  // src(1,0) -> dst(0,1)
        {1, 0},  // src(0,1) -> dst(1,0)
        {1, 1}   // src(1,1) -> dst(1,1)
    };

    // Initialize csRenderRemapByConstArray with default settings
    template<typename RecreateMatrixFunc>
    void initCopyLineIndexDefaults(csRenderRemapByConstArray& remap, 
                                    csMatrixPixels& matrix,
                                    RecreateMatrixFunc recreateMatrix) {
        // Get current matrix size (source will be the main matrix after rendering)
        // We'll use the matrix size after effect2 renders into it
        constexpr tMatrixPixelsSize defaultSize = 2;
        tMatrixPixelsSize srcWidth = matrix.width();
        tMatrixPixelsSize srcHeight = matrix.height();
        
        if (srcWidth == 0 || srcHeight == 0) {
            // Default to 2x2 if matrix is empty
            srcWidth = defaultSize;
            srcHeight = defaultSize;
            recreateMatrix(srcWidth, srcHeight);
        }
        
        // Configure remap
        // matrixSource will be set to &matrix in the render loop after effect2 renders
        remap.matrix = &matrix;
        remap.matrixSource = &matrix; // Will be updated in render loop to current matrix
        remap.remapArray = remapArray2x2;
        remap.remapWidth = 2;
        remap.remapHeight = 2;
        remap.renderRectAutosize = true;
        remap.rectSource = matrix.getRect();
    }

    // Update matrixSource for csRenderRemapByConstArray in render loop
    void updateCopyLineIndexSource(csRenderRemapByConstArray* remapEffect, 
                                   csMatrixPixels& matrix) {
        if (remapEffect) {
            // Update matrixSource to point to matrix (which now contains effect2 result)
            remapEffect->matrixSource = &matrix;
            remapEffect->rectSource = matrix.getRect();
        }
    }
};

