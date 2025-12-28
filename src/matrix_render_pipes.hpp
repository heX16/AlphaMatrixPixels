#pragma once

#include "matrix_render.hpp"
#include "matrix_types.hpp"

namespace amp {

// Base class for effects that use a source matrix.
class csRenderMatrixPipeBase : public csRenderMatrixBase {
public:
    static constexpr uint8_t base = csRenderMatrixBase::paramLast;
    static constexpr uint8_t paramMatrixSource = base + 1;
    static constexpr uint8_t paramRectSource = base + 2;
    static constexpr uint8_t paramLast = paramRectSource;

    // Source matrix pointer (nullptr means no source).
    csMatrixPixels* matrixSource = nullptr;

    // Source rectangle (defines area to copy from source matrix).
    csRect rectSource;

    uint8_t getParamsCount() const override {
        return paramLast;
    }

    void getParamInfo(uint8_t paramNum, csParamInfo& info) override {
        csRenderMatrixBase::getParamInfo(paramNum, info);
        switch (paramNum) {
            case paramMatrixSource:
                info.type = ParamType::Matrix;
                info.name = "Matrix source";
                info.ptr = &matrixSource;
                info.readOnly = false;
                info.disabled = false;
                break;
            case paramRectSource:
                info.type = ParamType::Rect;
                info.name = "Rect source";
                info.ptr = &rectSource;
                info.readOnly = false;
                info.disabled = false;
                break;
        }
    }

    void paramChanged(uint8_t paramNum) override {
        csRenderMatrixBase::paramChanged(paramNum);
        if (paramNum == paramMatrixSource) {
            if (renderRectAutosize && matrixSource) {
                rectDest = matrixSource->getRect();
            }
        }
    }
};

// Effect: average area by computing average color of source area and filling destination area with it.
// Can average area of the destination matrix itself by setting matrixSource to the same matrix (`matrixSource = matrix`).
class csRenderAverageArea : public csRenderMatrixPipeBase {
public:
    void render(csRandGen& /*rand*/, uint16_t /*currTime*/) const override {
        if (disabled || !matrix || !matrixSource) {
            return;
        }

        // Get average color of source area and fill destination area with it
        const csColorRGBA areaColor = matrixSource->getAreaColor(rectSource);
        matrix->fillArea(rectDest, areaColor);
    }
};

// Effect: copy pixels from source matrix to destination matrix with blending.
class csRenderMatrixCopy : public csRenderMatrixPipeBase {
public:

    void render(csRandGen& /*rand*/, uint16_t /*currTime*/) const override {
        if (disabled || !matrix || !matrixSource) {
            return;
        }

        if (rectDest.intersect(rectSource).empty()) {
            return;
        }

        // If sizes match, use simple drawMatrix (faster)
        if (rectDest.width == rectSource.width && rectDest.height == rectSource.height) {
            matrix->drawMatrixArea(rectSource, rectDest.x, rectDest.y, *matrixSource);
        } else {
            // Use drawMatrixScale for different sizes
            matrix->drawMatrixScale(rectSource, rectDest, *matrixSource);
        }
    }
};

// Effect: copy pixels by some remap function from source matrix to destination matrix.
class csRenderRemapBase : public csRenderMatrixPipeBase {
public:
    static constexpr uint8_t base = csRenderMatrixPipeBase::paramLast;
    static constexpr uint8_t paramRewrite = base + 1;
    static constexpr uint8_t paramLast = paramRewrite;

    bool rewrite = false;

    uint8_t getParamsCount() const override {
        return paramLast;
    }

    void getParamInfo(uint8_t paramNum, csParamInfo& info) override {
        csRenderMatrixPipeBase::getParamInfo(paramNum, info);
        switch (paramNum) {
            case paramRewrite:
                info.type = ParamType::Bool;
                info.name = "Rewrite";
                info.ptr = &rewrite;
                info.readOnly = false;
                info.disabled = false;
                break;
        }
    }

    virtual bool getPixelRemap(tMatrixPixelsCoord src_x, tMatrixPixelsCoord src_y, tMatrixPixelsCoord & dst_x, tMatrixPixelsCoord & dst_y) const {
        (void)src_x;
        (void)src_y;
        (void)dst_x;
        (void)dst_y;
        return false;
    }

    void render(csRandGen& /*rand*/, uint16_t /*currTime*/) const override {
        if (disabled || !matrix || !matrixSource) {
            return;
        }

        if (rectSource.empty()) {
            return;
        }

        // Iterate over all pixels in source rectangle
        for (tMatrixPixelsSize y = 0; y < rectSource.height; ++y) {
            for (tMatrixPixelsSize x = 0; x < rectSource.width; ++x) {
                // Get remapped destination coordinates
                tMatrixPixelsCoord dst_x = 0;
                tMatrixPixelsCoord dst_y = 0;
                // Use `x,y` - because the remap array is zero based
                if (!getPixelRemap(x, y, dst_x, dst_y)) {
                    continue; // Skip if remap function returns false
                }

                const tMatrixPixelsCoord src_x = rectSource.x + to_coord(x);
                const tMatrixPixelsCoord src_y = rectSource.y + to_coord(y);

                // Get source pixel and write to destination with blending
                const csColorRGBA sourcePixel = matrixSource->getPixel(src_x, src_y);
                if (rewrite)
                    matrix->setPixelRewrite(rectDest.x + dst_x, rectDest.x + dst_y, sourcePixel);
                else
                    matrix->setPixel(rectDest.x + dst_x, rectDest.x + dst_y, sourcePixel);
            }
        }
    }

};

// Effect: convert 2D matrix to linear 1D matrix
//
// linear 1D matrix size:  `height=1`, `width=src.height*src.width`.
// Index starts from 1. (not 0!)
// If `index==0` - skip the pixel.
class csRenderMatrix2DTo1D : public csRenderRemapBase {
public:
    bool getPixelRemap(tMatrixPixelsCoord src_x, tMatrixPixelsCoord src_y, tMatrixPixelsCoord & dst_x, tMatrixPixelsCoord & dst_y) const override {
        if (!matrixSource) {
            return false;
        }

        // Calculate linear index from 2D coordinates (row-major order), index starts from 1
        const uint32_t linearIndex = (src_y * matrixSource->width() + src_x) + 1;
        if (linearIndex == 0) {
            return false; // Skip pixel
        }
        dst_x = to_coord(linearIndex - 1);
        dst_y = 0;
        return true;
    }

    void paramChanged(uint8_t paramNum) override {
        // Validate matrix size when matrix or matrixSource changes
        if (paramNum == csRenderMatrixBase::paramMatrixDest ||
            paramNum == csRenderMatrixPipeBase::paramMatrixSource) {
            if (matrix != nullptr && matrixSource != nullptr) {
                // If size is incorrect, disable effect by setting matrix to nullptr
                if ((matrix->height() != 1) ||
                    (matrix->width() != matrixSource->height() * matrixSource->width())) {
                    matrix = nullptr;
                }
            }
        }

        csRenderRemapBase::paramChanged(paramNum);
    }
};

// Effect: remap 2D matrix to 2D matrix using custom index mapping via matrixIndex.
//
// `csRenderRemapByIndexMatrix` contains the matrix `matrixIndex`.
// It is desirable that its size matches `matrixSource`.
// But this is not required.
//
// `csRenderRemapByIndexMatrix` copies pixels from `matrixSource` to `matrixDest`.
// For this, it takes the pixel `(x, y)`, then takes the color from `matrixIndex`,
// and extracts destination coordinates from color components:
// - dst_x from g, b components (green, blue)
// - dst_y from a, r components (alpha, red)
// Thus, the matrix `matrixIndex` is a table where each color encodes
// the destination coordinates where the corresponding pixel should be placed.
class csRenderRemapByIndexMatrix : public csRenderRemapBase {
public:
    static constexpr uint8_t base = csRenderMatrixPipeBase::paramLast;
    static constexpr uint8_t paramMatrixIndex = base + 1;
    static constexpr uint8_t paramLast = paramMatrixIndex;

    // Index matrix pointer (nullptr means skip remapping).
    csMatrixPixels* matrixIndex = nullptr;

    uint8_t getParamsCount() const override {
        return paramLast;
    }

    void getParamInfo(uint8_t paramNum, csParamInfo& info) override {
        csRenderRemapBase::getParamInfo(paramNum, info);
        switch (paramNum) {
            case paramMatrixIndex:
                info.type = ParamType::Matrix;
                info.name = "Matrix index";
                info.ptr = &matrixIndex;
                info.readOnly = false;
                info.disabled = false;
                break;
        }
    }

    bool getPixelRemap(tMatrixPixelsCoord src_x, tMatrixPixelsCoord src_y, tMatrixPixelsCoord & dst_x, tMatrixPixelsCoord & dst_y) const override {
        if (!matrixSource || !matrixIndex) {
            return false;
        }

        const csColorRGBA indexColor = matrixIndex->getPixel(src_x, src_y);

        // Extract dst_x from g, b components (16-bit value: g in high byte, b in low byte)
        dst_x = to_coord((static_cast<uint16_t>(indexColor.g) << 8) | indexColor.b);

        // Extract dst_y from a, r components (16-bit value: a in high byte, r in low byte)
        dst_y = to_coord((static_cast<uint16_t>(indexColor.a) << 8) | indexColor.r);

        return true;
    }
};

// Effect: remap 2D matrix to 2D matrix using custom index mapping via 2D array.
//
// `csRenderRemap` contains a pointer to a const 2D array of remap coordinates.
// Each element in the array contains destination coordinates (x, y) for the corresponding source pixel.
// The array dimensions are specified by `remapWidth` and `remapHeight` fields.
class csRenderRemapByConstArray : public csRenderRemapBase {
public:
    // Structure for storing remap coordinates
    struct RemapCoord {
        tMatrixPixelsCoord x;
        tMatrixPixelsCoord y;
    };

    // Pointer to const 2D array of remap coordinates (nullptr means skip remapping).
    // Array is indexed as: remapArray[y * remapWidth + x]
    const RemapCoord* remapArray = nullptr;

    // Dimensions of the remap array
    tMatrixPixelsSize remapWidth = 0;
    tMatrixPixelsSize remapHeight = 0;

    // Default constructor
    csRenderRemapByConstArray() = default;

    // Constructor with remap array and dimensions
    csRenderRemapByConstArray(const RemapCoord* array, tMatrixPixelsSize width, tMatrixPixelsSize height)
        : remapArray(array), remapWidth(width), remapHeight(height) {}

    bool getPixelRemap(tMatrixPixelsCoord src_x, tMatrixPixelsCoord src_y, tMatrixPixelsCoord & dst_x, tMatrixPixelsCoord & dst_y) const override {
        if (!matrixSource || !remapArray) {
            return false;
        }

        // Check bounds
        if (src_x < 0 || src_y < 0 ||
            src_x >= static_cast<tMatrixPixelsCoord>(remapWidth) ||
            src_y >= static_cast<tMatrixPixelsCoord>(remapHeight)) {
            return false;
        }

        // Get remap coordinates from array
        const RemapCoord& coord = remapArray[src_y * remapWidth + src_x];
        dst_x = coord.x;
        dst_y = coord.y;
        return true;
    }
};

/*
Effect: remap 2D matrix to 1D matrix (height=1) using custom index mapping via 2D array.

`csRenderRemap1DByConstArray` contains a pointer to a const 2D array of x coordinates.
Each element in the array contains destination x coordinate for the corresponding source pixel.
The y coordinate is always 0 (1D matrix).
The array dimensions are specified by `remapWidth` and `remapHeight` fields.
This class is optimized for 2D->1D remapping, saving memory compared to storing both x and y.

Array index number based at 1! (not 0!)

Example:
   2x2 remap array that transforms:
   `Remap = {1,2,3,4}`
    src(x 0,y 0) -> dst x 0
    src(x 1,y 0) -> dst x 1
    src(x 0,y 1) -> dst x 2
    src(x 1,y 1) -> dst x 3
 Result: 2x2 source matrix becomes 1x4 (height=1) destination matrix.
*/
class csRenderRemap1DByConstArray : public csRenderRemapBase {
public:
    // Pointer to const 2D array of x coordinates (nullptr means skip remapping).
    // Array is indexed as: `remapArray[y * remapWidth + x]`
    const tMatrixPixelsCoord* remapArray = nullptr;

    // Dimensions of the remap array
    tMatrixPixelsSize remapWidth = 0;
    tMatrixPixelsSize remapHeight = 0;

    // Default constructor
    csRenderRemap1DByConstArray() = default;

    // Constructor with remap array and dimensions
    csRenderRemap1DByConstArray(const tMatrixPixelsCoord* array, tMatrixPixelsSize width, tMatrixPixelsSize height)
        : remapArray(array), remapWidth(width), remapHeight(height) {}

    bool getPixelRemap(tMatrixPixelsCoord src_x, tMatrixPixelsCoord src_y, tMatrixPixelsCoord & dst_x, tMatrixPixelsCoord & dst_y) const override {
        if (!matrixSource || !remapArray) {
            return false;
        }

        // Check bounds
        if (src_x < 0 || src_y < 0 ||
            src_x >= static_cast<tMatrixPixelsCoord>(remapWidth) ||
            src_y >= static_cast<tMatrixPixelsCoord>(remapHeight)) {
            return false;
        }

        // Get remap x coordinate from array, y is always 0 for 1D matrix
        dst_x = remapArray[src_y * remapWidth + src_x];
        if (dst_x == 0)
            return false;
        dst_x -= 1;
        dst_y = 0;
        return true;
    }

    void paramChanged(uint8_t paramNum) override {
        // Validate matrix size when matrix or matrixSource changes
        if (paramNum == csRenderMatrixBase::paramMatrixDest ||
            paramNum == csRenderMatrixPipeBase::paramMatrixSource) {
            if (matrix != nullptr) {
                // Matrix must have height == 1 for 1D remapping
                if (matrix->height() != 1) {
                    matrix = nullptr;
                }
            }
        }

        csRenderRemapBase::paramChanged(paramNum);
    }
};

} // namespace amp
