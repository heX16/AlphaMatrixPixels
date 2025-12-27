#pragma once

#include "../../src/matrix_pixels.hpp"
#include "../../src/matrix_render_pipes"
#include "../../src/color_rgba.hpp"

using amp::csRenderMatrixCopyLineIndex;
using amp::csMatrixPixels;
using amp::csColorRGBA;
using amp::tMatrixPixelsSize;

// Helper class for csRenderMatrixCopyLineIndex functionality
class csCopyLineIndexHelper {
public:
    csMatrixPixels matrixIndexCopyLine{0, 0};

    // Initialize csRenderMatrixCopyLineIndex with default settings
    template<typename RecreateMatrixFunc>
    void initCopyLineIndexDefaults(csRenderMatrixCopyLineIndex& copyLineIndex, 
                                    csMatrixPixels& matrix,
                                    RecreateMatrixFunc recreateMatrix) {
        // Get current matrix size (source will be the main matrix after rendering)
        // We'll use the matrix size after effect2 renders into it
        constexpr tMatrixPixelsSize defaultSize = 8;
        tMatrixPixelsSize srcWidth = matrix.width();
        tMatrixPixelsSize srcHeight = matrix.height();
        
        if (srcWidth == 0 || srcHeight == 0) {
            // Default to 8x8 if matrix is empty
            srcWidth = defaultSize;
            srcHeight = defaultSize;
            recreateMatrix(srcWidth, srcHeight);
        }
        
        // Create index matrix (same size as source matrix)
        matrixIndexCopyLine = csMatrixPixels{srcWidth, srcHeight};
        
        // Fill index matrix with row-major order indices (starting from 1)
        // Store index as packed value in color (0xAARRGGBB format)
        for (tMatrixPixelsSize y = 0; y < srcHeight; ++y) {
            for (tMatrixPixelsSize x = 0; x < srcWidth; ++x) {
                const uint32_t linearIndex = (y * srcWidth + x) + 1;
                // Create color and set value directly to store index
                csColorRGBA indexColor{0, 0, 0, 0};
                indexColor.value = linearIndex;
                matrixIndexCopyLine.setPixel(x, y, indexColor);
            }
        }
        
        // Configure copyLineIndex
        // matrixSource will be set to &matrix in the render loop after effect2 renders
        copyLineIndex.matrix = &matrix;
        copyLineIndex.matrixSource = &matrix; // Will be updated in render loop to current matrix
        copyLineIndex.matrixIndex = &matrixIndexCopyLine;
        copyLineIndex.renderRectAutosize = true;
        copyLineIndex.rectSource = matrix.getRect();
    }

    // Update matrixSource for csRenderMatrixCopyLineIndex in render loop
    void updateCopyLineIndexSource(csRenderMatrixCopyLineIndex* copyLineIndexEffect, 
                                   csMatrixPixels& matrix) {
        if (copyLineIndexEffect) {
            // Update matrixSource to point to matrix (which now contains effect2 result)
            copyLineIndexEffect->matrixSource = &matrix;
            copyLineIndexEffect->rectSource = matrix.getRect();
        }
    }
};

