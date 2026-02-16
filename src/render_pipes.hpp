#pragma once

#include "render_base.hpp"
#include "matrix_base.hpp"
#include "matrix_types.hpp"
#include "amp_macros.hpp"
#include "fixed_point.hpp"
#include "matrix_utils.hpp"
// #include <stdint.h>

namespace amp {

// Base class for effects that use a source matrix.
class csRenderMatrixPipeBase : public csRenderMatrixBase {
public:
    // Source matrix pointer (nullptr means no source).
    csMatrixBase* matrixSource = nullptr;

    // Source rectangle (defines area to copy from source matrix).
    csRect rectSource;

    void getPropInfo(uint8_t propNum, csPropInfo& info) override {
        csRenderMatrixBase::getPropInfo(propNum, info);
        switch (propNum) {
            case propMatrixSource:
                info.valuePtr = &matrixSource;
                info.readOnly = false;
                info.disabled = false;
                break;
            case propRectSource:
                info.valuePtr = &rectSource;
                info.readOnly = false;
                info.disabled = false;
                break;
        }
    }

    void propChanged(uint8_t propNum) override {
        csRenderMatrixBase::propChanged(propNum);
        if (propNum == propMatrixSource) {
            if (renderRectAutosize && matrixSource) {
                rectDest = matrixSource->getRect();
            }
        }
    }

    // Class family identifier
    static constexpr PropType ClassFamilyId = PropType::EffectPipe;

    // Override to return pipe renderer family
    PropType getClassFamily() const override {
        return ClassFamilyId;
    }

    // Override to check for pipe renderer family
    void* queryClassFamily(PropType familyId) override {
        if (familyId == ClassFamilyId) {
            return this;
        }
        // Check base class (csRenderMatrixBase) family
        return csRenderMatrixBase::queryClassFamily(familyId);
    }
};

// Base class for post-frame effects.
// Post-frame effects work differently from regular effects: they don't render during the normal
// render() phase. Instead, they process the final frame in onFrameDone() after all other effects
// have been rendered. This allows them to apply transformations to the complete frame.
class csRenderPostFrame : public csRenderMatrixPipeBase {
public:
    // Class family identifier
    static constexpr PropType ClassFamilyId = PropType::EffectPostFrame;

    PropType getClassFamily() const override {
        return ClassFamilyId;
    }

    void* queryClassFamily(PropType familyId) override {
        if (familyId == ClassFamilyId) {
            return this;
        }
        return csRenderMatrixPipeBase::queryClassFamily(familyId);
    }

    void recalc(csRandGen& /*rand*/, tTime /*currTime*/) override {
        // Intentionally empty:
        // Post-frame effects don't use recalc() because their timing is driven from onFrameDone()
        // to avoid flickering artifacts when recalc() is called in a timing/order that doesn't
        // correspond to completed frames.
    }

    void render(csRandGen& /*rand*/, tTime /*currTime*/) const override {
        // Intentionally empty: rendering is handled in onFrameDone() after all effects are rendered.
    }

    void onFrameDone(csMatrixPixels& frame, csRandGen& rand, tTime currTime) override {
        if (disabled) {
            return;
        }
        // Derived classes should override this method to implement their specific post-frame logic.
        (void)frame;
        (void)rand;
        (void)currTime;
    }
};

// Effect: average area by computing average color of source area and filling destination area with it.
// Can average area of the destination matrix itself by setting matrixSource to the same matrix (`matrixSource = matrixDest`).
class csRenderAverageArea : public csRenderMatrixPipeBase {
public:
    void render(csRandGen& /*rand*/, tTime /*currTime*/) const override {
        if (disabled || !matrixDest || !matrixSource) {
            return;
        }

        // Get average color of source area and fill destination area with it
        const csColorRGBA areaColor = matrix_utils::getAreaColor(*matrixSource, rectSource);
        matrix_utils::fillArea(*matrixDest, rectDest, areaColor);
    }
};

// Effect: copy pixels from source matrix to destination matrix with blending.
class csRenderMatrixCopy : public csRenderMatrixPipeBase {
public:

    void render(csRandGen& /*rand*/, tTime /*currTime*/) const override {
        if (disabled || !matrixDest || !matrixSource) {
            return;
        }

        if (rectDest.intersect(rectSource).empty()) {
            return;
        }

        // If sizes match, use simple drawMatrix (faster)
        if (rectDest.width == rectSource.width && rectDest.height == rectSource.height) {
            matrix_utils::drawMatrixArea(*matrixDest, rectSource, rectDest.x, rectDest.y, *matrixSource);
        } else {
            // Use drawMatrixScale for different sizes
            (void)matrix_utils::drawMatrixScale(*matrixDest, rectSource, rectDest, *matrixSource);
        }
    }
};

// Effect: copy pixels by some remap function from source matrix to destination matrix.
class csRenderRemapBase : public csRenderMatrixPipeBase {
public:
    bool rewrite = false;

    void getPropInfo(uint8_t propNum, csPropInfo& info) override {
        csRenderMatrixPipeBase::getPropInfo(propNum, info);
        switch (propNum) {
            case propRewrite:
                info.valuePtr = &rewrite;
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

    void render(csRandGen& /*rand*/, tTime /*currTime*/) const override {
        if (disabled || !matrixDest || !matrixSource) {
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
                    matrixDest->setPixelRewrite(rectDest.x + dst_x, rectDest.x + dst_y, sourcePixel);
                else
                    matrixDest->setPixel(rectDest.x + dst_x, rectDest.x + dst_y, sourcePixel);
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

    void propChanged(uint8_t propNum) override {
        // Validate matrix size when matrix or matrixSource changes
        if (propNum == csRenderMatrixBase::propMatrixDest ||
            propNum == csRenderMatrixPipeBase::propMatrixSource) {
            if (matrixDest != nullptr && matrixSource != nullptr) {
                // If size is incorrect, disable effect by setting matrix to nullptr
                if ((matrixDest->height() != 1) ||
                    (matrixDest->width() != matrixSource->height() * matrixSource->width())) {
                    matrixDest = nullptr;
                }
            }
        }

        csRenderRemapBase::propChanged(propNum);
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
    static constexpr uint8_t base = csRenderMatrixPipeBase::propLast;
    static constexpr uint8_t propMatrixIndex = base + 1;
    static constexpr uint8_t propLast = propMatrixIndex;

    // Index matrix pointer (nullptr means skip remapping).
    csMatrixPixels* matrixIndex = nullptr;

    uint8_t getPropsCount() const override {
        return propLast;
    }

    void getPropInfo(uint8_t propNum, csPropInfo& info) override {
        csRenderRemapBase::getPropInfo(propNum, info);
        switch (propNum) {
            case propMatrixIndex:
                info.valueType = PropType::Matrix;
                info.name = "Matrix index";
                info.valuePtr = &matrixIndex;
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



// Effect: remap 1D matrix (height=1) to 2D matrix.
// Base class for 1D->2D remapping effects. Source matrix must have height=1.
// Provides virtual function to calculate required source pixel count and auto-update source matrix size.
class csRenderMatrix1DTo2DBase : public csRenderMatrixPipeBase {
public:
    static constexpr uint8_t base = csRenderMatrixPipeBase::propLast;
    static constexpr uint8_t propAutoUpdateSourceMatrixSize = base + 1;
    static constexpr uint8_t propLast = propAutoUpdateSourceMatrixSize;

    // Auto-update source matrix size when pixel count changes
    bool autoUpdateSourceMatrixSize = true;

    uint8_t getPropsCount() const override {
        return propLast;
    }

    void getPropInfo(uint8_t propNum, csPropInfo& info) override {
        csRenderMatrixPipeBase::getPropInfo(propNum, info);
        switch (propNum) {
            case propAutoUpdateSourceMatrixSize:
                info.valueType = PropType::Bool;
                info.name = "Auto update source matrix size";
                info.valuePtr = &autoUpdateSourceMatrixSize;
                info.readOnly = false;
                info.disabled = false;
                break;
        }
    }

    // Calculate required number of pixels in source matrix (1D, so this is the width).
    // Default implementation returns rectDest.width (simple example: one line maps to destination width).
    virtual tMatrixPixelsSize calcSourceMatrixPixelsCount() const {
        return rectDest.width;
    }

    // Map 1D source index to 2D destination coordinates.
    // src_x is the 1D index (y is implicitly 0 for 1D source matrix).
    // Returns false to skip mapping this pixel.
    // Simple example implementation: x passthrough, y fixed to 1.
    virtual bool mapIndexToDest(tMatrixPixelsCoord src_x, tMatrixPixelsCoord& dst_x, tMatrixPixelsCoord& dst_y) const {
        dst_x = src_x;
        dst_y = 1; // Fixed y coordinate as per example
        return true;
    }

    void propChanged(uint8_t propNum) override {
        csRenderMatrixPipeBase::propChanged(propNum);
        
        // Trigger source matrix size update on relevant property changes
        if (propNum == csRenderMatrixBase::propMatrixDest ||
            propNum == csRenderMatrixBase::propRectDest ||
            propNum == csRenderMatrixBase::propRenderRectAutosize ||
            propNum == csRenderMatrixPipeBase::propMatrixSource ||
            propNum == propAutoUpdateSourceMatrixSize) {
            updateSourceMatrixSizeIfNeeded();
        }
    }

    void render(csRandGen& rand, tTime currTime) const override {
        (void)rand;
        (void)currTime;

        if (disabled || !matrixDest || !matrixSource) {
            return;
        }

        // Source must be 1D (height=1)
        if (matrixSource->height() != 1) {
            return;
        }

        const tMatrixPixelsSize sourceWidth = matrixSource->width();
        
        // Iterate over source width only (1D matrix: height=1, so y is always 0)
        for (tMatrixPixelsSize x = 0; x < sourceWidth; ++x) {
            tMatrixPixelsCoord dst_x = 0;
            tMatrixPixelsCoord dst_y = 0;
            
            // Map 1D index to 2D destination coordinates
            if (!mapIndexToDest(to_coord(x), dst_x, dst_y)) {
                continue; // Skip if mapping function returns false
            }

            // Source coordinates: x from loop, y is always 0 for 1D matrix
            const csColorRGBA sourcePixel = matrixSource->getPixel(to_coord(x), 0);

            // Write to destination with blending (always use setPixel, no rewrite option)
            matrixDest->setPixel(rectDest.x + dst_x, rectDest.y + dst_y, sourcePixel);
        }
    }

protected:
    // Update source matrix size if auto-update is enabled and size mismatch detected.
    void updateSourceMatrixSizeIfNeeded() {
        if (!autoUpdateSourceMatrixSize || !matrixSource) {
            return;
        }

        const tMatrixPixelsSize neededWidth = calcSourceMatrixPixelsCount();
        
        // Ensure source is 1D (height=1)
        if (matrixSource->height() != 1) {
            matrixSource->resize(neededWidth, 1);
        }
        // Ensure source width matches required pixel count
        else if (matrixSource->width() != neededWidth) {
            matrixSource->resize(neededWidth, 1);
        }

        // Update rectSource to cover full source line
        rectSource = csRect{0, 0, neededWidth, 1};
    }
};

// Effect: fill destination rectangle with 1D source strip (height=1) by drawing repeated stripes.
// The fill direction is controlled by the `angle` property:
// - angle = 0  -> horizontal mode: pixel strip is drawn horizontally. Each pixel is stretched (copied) along the rect height. The loop runs over `x`.
// - angle = 90 -> vertical mode: the pixel strip is drawn vertically. Each pixel is stretched (copied) along the rect length. The loop runs over `y`.
// Only angle values 0 and 90 are supported and meaningful.
class csRenderMatrix1DTo2DRect : public csRenderMatrix1DTo2DBase {
public:
    static constexpr uint8_t base = csRenderMatrix1DTo2DBase::propLast;
    static constexpr uint8_t propAngle = base + 1;
    static constexpr uint8_t propLast = propAngle;

    math::csFP16 angle{0.0f};

    uint8_t getPropsCount() const override {
        return propLast;
    }

    void getPropInfo(uint8_t propNum, csPropInfo& info) override {
        csRenderMatrix1DTo2DBase::getPropInfo(propNum, info);
        switch (propNum) {
            case propAngle:
                info.valueType = PropType::FP16;
                info.name = "Angle";
                info.valuePtr = &angle;
                info.readOnly = false;
                info.disabled = false;
                break;
        }
    }

    // Calculate required number of pixels in source matrix based on fill angle.
    // Horizontal mode (angle=0): need rectDest.width pixels (one per column).
    // Vertical mode (angle=90): need rectDest.height pixels (one per row).
    tMatrixPixelsSize calcSourceMatrixPixelsCount() const override {
        // Check if angle is 90 degrees (vertical mode)
        // Only 0 and 90 are supported, so we check if rounded angle equals 90
        if (angle.round_int() == 90) {
            return rectDest.height;
        }
        // Otherwise, use horizontal mode (angle=0 or any other value)
        return rectDest.width;
    }

    void propChanged(uint8_t propNum) override {
        csRenderMatrix1DTo2DBase::propChanged(propNum);
        
        // Trigger source matrix size update when angle changes
        if (propNum == propAngle) {
            updateSourceMatrixSizeIfNeeded();
        }
    }

    void render(csRandGen& /*rand*/, tTime /*currTime*/) const override {
        if (disabled || !matrixDest || !matrixSource) {
            return;
        }

        // Source must be 1D (height=1)
        if (matrixSource->height() != 1) {
            return;
        }

        const tMatrixPixelsSize sourceWidth = matrixSource->width();
        
        // Determine fill mode based on angle (only 0 and 90 are supported)
        const bool isVertical = (angle.round_int() == 90);

        if (isVertical) {
            // Vertical mode: loop over y, draw horizontal stripes
            // Each source pixel source[y] is stretched into a horizontal stripe of width rectDest.width
            const tMatrixPixelsSize effectiveRows = (rectDest.height < sourceWidth) ? rectDest.height : sourceWidth;
            
            for (tMatrixPixelsSize y = 0; y < effectiveRows; ++y) {
                const csColorRGBA sourcePixel = matrixSource->getPixel(to_coord(y), 0);
                
                // Draw horizontal stripe: fill entire row with this pixel
                for (tMatrixPixelsSize x = 0; x < rectDest.width; ++x) {
                    matrixDest->setPixel(rectDest.x + to_coord(x), rectDest.y + to_coord(y), sourcePixel);
                }
            }
        } else {
            // Horizontal mode: loop over x, draw vertical stripes
            // Each source pixel source[x] is stretched into a vertical stripe of height rectDest.height
            const tMatrixPixelsSize effectiveCols = (rectDest.width < sourceWidth) ? rectDest.width : sourceWidth;
            
            for (tMatrixPixelsSize x = 0; x < effectiveCols; ++x) {
                const csColorRGBA sourcePixel = matrixSource->getPixel(to_coord(x), 0);
                
                // Draw vertical stripe: fill entire column with this pixel
                for (tMatrixPixelsSize y = 0; y < rectDest.height; ++y) {
                    matrixDest->setPixel(rectDest.x + to_coord(x), rectDest.y + to_coord(y), sourcePixel);
                }
            }
        }
    }
};

// Effect: fill destination rectangle perimeter with 1D source strip (height=1).
// The source strip is mapped to the perimeter of rectDest in clockwise order,
// starting from the top-left corner and going right along the top edge first.
// Each corner pixel is used exactly once (no duplication).
class csRenderMatrix1DTo2DRectFrame : public csRenderMatrix1DTo2DBase {
public:
    // Calculate required number of pixels in source matrix based on perimeter length.
    // Perimeter calculation:
    // - If width==0 || height==0: 0
    // - If width==1 && height==1: 1
    // - If height==1: width (single row)
    // - If width==1: height (single column)
    // - Else: 2*(width + height) - 4 (unique corners)
    tMatrixPixelsSize calcSourceMatrixPixelsCount() const override {
        const tMatrixPixelsSize w = rectDest.width;
        const tMatrixPixelsSize h = rectDest.height;
        
        if (w == 0 || h == 0) {
            return 0;
        }
        if (w == 1 && h == 1) {
            return 1;
        }
        if (h == 1) {
            return w; // Single row
        }
        if (w == 1) {
            return h; // Single column
        }
        // Normal rectangle: perimeter = 2*(w+h) - 4 (corners counted once)
        return 2 * (w + h) - 4;
    }

    // Map 1D source index to 2D destination coordinates on the perimeter.
    // Traversal order: clockwise, starting at top-left, going right first.
    // Each corner pixel is used exactly once.
    bool mapIndexToDest(tMatrixPixelsCoord src_x, tMatrixPixelsCoord& dst_x, tMatrixPixelsCoord& dst_y) const override {
        const tMatrixPixelsSize w = rectDest.width;
        const tMatrixPixelsSize h = rectDest.height;
        const tMatrixPixelsCoord i = src_x;
        
        // Edge cases
        if (w == 0 || h == 0) {
            return false;
        }
        if (w == 1 && h == 1) {
            if (i == 0) {
                dst_x = 0;
                dst_y = 0;
                return true;
            }
            return false;
        }
        if (h == 1) {
            // Single row: all pixels are on the perimeter
            if (i < static_cast<tMatrixPixelsCoord>(w)) {
                dst_x = i;
                dst_y = 0;
                return true;
            }
            return false;
        }
        if (w == 1) {
            // Single column: all pixels are on the perimeter
            if (i < static_cast<tMatrixPixelsCoord>(h)) {
                dst_x = 0;
                dst_y = i;
                return true;
            }
            return false;
        }
        
        // Normal rectangle: perimeter = 2*(w+h) - 4
        const tMatrixPixelsSize perimeter = 2 * (w + h) - 4;
        if (i >= static_cast<tMatrixPixelsCoord>(perimeter)) {
            return false;
        }
        
        // Top edge: indices [0 .. w-1]
        if (i < static_cast<tMatrixPixelsCoord>(w)) {
            dst_x = i;
            dst_y = 0;
            return true;
        }
        
        // Right edge (excluding top-right corner): indices [w .. w+h-2]
        const tMatrixPixelsCoord rightStart = static_cast<tMatrixPixelsCoord>(w);
        const tMatrixPixelsCoord rightEnd = rightStart + static_cast<tMatrixPixelsCoord>(h) - 1;
        if (i < rightEnd) {
            dst_x = static_cast<tMatrixPixelsCoord>(w) - 1;
            dst_y = 1 + (i - rightStart);
            return true;
        }
        
        // Bottom edge (excluding bottom-right corner): indices [w+h-1 .. w+h+w-3]
        const tMatrixPixelsCoord bottomStart = rightEnd;
        const tMatrixPixelsCoord bottomEnd = bottomStart + static_cast<tMatrixPixelsCoord>(w) - 1;
        if (i < bottomEnd) {
            const tMatrixPixelsCoord bottomIdx = i - bottomStart;
            dst_x = static_cast<tMatrixPixelsCoord>(w) - 2 - bottomIdx;
            dst_y = static_cast<tMatrixPixelsCoord>(h) - 1;
            return true;
        }
        
        // Left edge (excluding both left corners): indices [w+h+w-2 .. perimeter-1]
        const tMatrixPixelsCoord leftStart = bottomEnd;
        const tMatrixPixelsCoord leftIdx = i - leftStart;
        dst_x = 0;
        dst_y = static_cast<tMatrixPixelsCoord>(h) - 2 - leftIdx;
        return true;
    }
};

/*
TODO: 

future:
- csRenderMatrix1DTo2DRectSpiral
- csRenderMatrix1DTo2DRectZigzag
- csRenderMatrix1DTo2DRectAngle
- csRenderMatrix1DTo2DLine
- csRenderMatrix1DTo2DRadial

*/




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
        // Read from PROGMEM using pgm_read_dword (tMatrixPixelsCoord is int32_t)
        dst_x = static_cast<tMatrixPixelsCoord>(pgm_read_dword(&remapArray[src_y * remapWidth + src_x]));
        if (dst_x == 0)
            return false;
        dst_x -= 1;
        dst_y = 0;
        return true;
    }

    void propChanged(uint8_t propNum) override {
        // Validate matrix size when matrix or matrixSource changes
        if (propNum == csRenderMatrixBase::propMatrixDest ||
            propNum == csRenderMatrixPipeBase::propMatrixSource) {
            if (matrixDest != nullptr) {
                // Matrix must have height == 1 for 1D remapping
                if (matrixDest->height() != 1) {
                    matrixDest = nullptr;
                }
            }
        }

        csRenderRemapBase::propChanged(propNum);
    }
};

// Base class for slow fading effects that accumulate frames in a buffer.
// Derived classes only need to override composePixel() to specify the blending order.
class csRenderSlowFadingBase : public csRenderPostFrame {
public:
    /*
    // TODO: `buffer` - улучшить качество логики.
     `buffer` - можно сделать его как свойство, которое может буть null - и тогда оно само создает буффер.
     а если оно имеет указатель на матрицу, тогда в `buffer` записывается этот указатель,
     но при этом csRender больше не может удалять матрицу (поскольку он не владелец).

     но csRender может менять размер матрицы `buffer`, и без разницы - владеет от этой матрицей или нет.

     получается что у класса будет 3 матрицы:
     1. source
     2. dest
     3. buffer

     это интересно тем что если буфер будет доступен, тогда можно будет делать более сложные эффекты.
    */

    static constexpr uint8_t base = csRenderMatrixPipeBase::propLast;
    static constexpr uint8_t propFadeAlpha = base + 1;
    static constexpr uint8_t propLast = propFadeAlpha;

    static constexpr uint16_t cFadeIntervalMs = 32; // Hz

    // Internal buffer that stores the accumulated image.
    csMatrixPixels* buffer = nullptr;

    // User-facing trail strength (0-255): higher = slower fade.
    // Note: internally we apply a non-linear curve so the lower half of the range
    // is much more usable (small values no longer disappear "instantly").
    uint8_t fadeAlpha = 224;

    // Timestamp of last fade step (wrap-safe, tTime is uint16_t).
    // Note: 0 is a valid timestamp (wrap-around), so we track initialization separately.
    uint16_t lastFadeTime = 0;
    bool lastFadeTimeValid = false;

    uint8_t getPropsCount() const override {
        return propLast;
    }

    void getPropInfo(uint8_t propNum, csPropInfo& info) override {
        csRenderPostFrame::getPropInfo(propNum, info);
        if (propNum == propFadeAlpha) {
            info.valueType = PropType::UInt8;
            info.name = "Fade alpha";
            info.valuePtr = &fadeAlpha;
            info.readOnly = false;
            info.disabled = false;
        }
    }

    ~csRenderSlowFadingBase() override {
        delete buffer;
    }

    void propChanged(uint8_t propNum) override {
        csRenderPostFrame::propChanged(propNum);
        if (propNum == propRectSource) {
            updateBuffer();
        }
    }

    void onFrameDone(csMatrixPixels& frame, csRandGen& /*rand*/, tTime currTime) override {
        // Base class handles disabled check, but we need to check it here too since
        // base method returns early and we can't detect that with void return type.
        if (disabled) {
            return;
        }

        rectSource = frame.getRect();
        rectDest = rectSource;
        updateBuffer();
        if (!buffer) {
            return;
        }

        // Fade accumulated trail with a stable rate, based on elapsed time intervals.
        // If multiple intervals passed (e.g., low FPS), apply fade multiple times.
        // If no interval passed, skip fading for this frame (avoids jitter/flicker).
        if (!lastFadeTimeValid) {
            lastFadeTime = currTime;
            lastFadeTimeValid = true;
        } else {
            const uint16_t delta = static_cast<uint16_t>(currTime - lastFadeTime);
            const uint16_t steps = static_cast<uint16_t>(delta / cFadeIntervalMs);
            if (steps != 0) {
                // Prevent pathological long loops after long pauses.
                const uint16_t cappedSteps = (steps > 32) ? 32 : steps;
                for (uint16_t i = 0; i < cappedSteps; ++i) {
                    fadeBuffer();
                }
                lastFadeTime = static_cast<uint16_t>(lastFadeTime + cappedSteps * cFadeIntervalMs);
            }
        }

        const tMatrixPixelsSize height = rectSource.height;
        const tMatrixPixelsSize width = rectSource.width;

        for (tMatrixPixelsSize y = 0; y < height; ++y) {
            for (tMatrixPixelsSize x = 0; x < width; ++x) {
                const tMatrixPixelsCoord fx = rectSource.x + to_coord(x);
                const tMatrixPixelsCoord fy = rectSource.y + to_coord(y);
                const csColorRGBA cur = frame.getPixel(fx, fy);
                const csColorRGBA trail = buffer->getPixel(x, y);

                // Derived classes can override processPixel() to customize how the buffer/frame are updated.
                processPixel(frame, fx, fy, x, y, cur, trail);
            }
        }
    }

protected:
    // Virtual method to compose current frame and trail.
    // Derived classes override this to specify the blending order.
    virtual csColorRGBA composePixel(csColorRGBA cur, csColorRGBA trail) const = 0;

    // Pixel processing hook.
    // Default implementation keeps the original behavior: compute a single composite color
    // and write it both to the internal buffer and to the output frame.
    virtual void processPixel(csMatrixPixels& frame,
                              tMatrixPixelsCoord fx,
                              tMatrixPixelsCoord fy,
                              tMatrixPixelsSize x,
                              tMatrixPixelsSize y,
                              csColorRGBA cur,
                              csColorRGBA trail) {
        const csColorRGBA composite = composePixel(cur, trail);
        buffer->setPixelRewrite(x, y, composite);
        frame.setPixelRewrite(fx, fy, composite);
    }

    // Protected methods for derived classes to use
    void updateBuffer() {
        if (rectSource.empty()) {
            delete buffer;
            buffer = nullptr;
            return;
        }

        const tMatrixPixelsSize width = rectSource.width;
        const tMatrixPixelsSize height = rectSource.height;
        if (width == 0 || height == 0) {
            delete buffer;
            buffer = nullptr;
            return;
        }

        if (buffer && buffer->width() == width && buffer->height() == height) {
            return;
        }

        // TODO: тут ошибка! удалять объект buffer не нужно. достаточно вызывать `resize`.
        delete buffer;
        buffer = new csMatrixPixels(width, height);
        buffer->clear();
    }

    void fadeBuffer() {
        if (!buffer) {
            return;
        }

        const tMatrixPixelsSize height = buffer->height();
        const tMatrixPixelsSize width = buffer->width();
        for (tMatrixPixelsSize y = 0; y < height; ++y) {
            for (tMatrixPixelsSize x = 0; x < width; ++x) {
                uint8_t newAlpha;
                csColorRGBA pixel = buffer->getPixel(x, y);
                
                if (pixel.a < 4) {
                    if (pixel.a == 0) {
                        continue;
                    } else {
                        newAlpha = 0;
                    }
                } else {
                    newAlpha = mul8(pixel.a, getFadeMul(fadeAlpha));
                }

                if (newAlpha != pixel.a) {
                    pixel.a = newAlpha;
                    buffer->setPixelRewrite(x, y, pixel);
                }
            }
        }
    }

private:
    // Convert user-facing fadeAlpha (0..255) to an actual fade multiplier (0..255).
    //
    // We intentionally apply a non-linear curve to expand the "useful" range:
    // small fadeAlpha values were previously fading too aggressively (exponential decay),
    // making trails effectively invisible. By squaring the "decay amount", we make the
    // lower half of the range much softer while keeping the upper range similar.
    //
    // Mapping:
    // - fadeAlpha = 255 -> fadeMul = 255 (no fading)
    // - fadeAlpha = 0   -> fadeMul = 0   (instant clear)
    static inline uint8_t getFadeMul(uint8_t fadeAlpha) noexcept {
        const uint8_t decay = static_cast<uint8_t>(255u - fadeAlpha);
        const uint8_t decay2 = mul8(decay, decay); // non-linear: square (0..255)
        return static_cast<uint8_t>(255u - decay2);
    }
};

// Effect: slow fade trail from source matrix to destination.
// The effect keeps an internal buffer that accumulates the source
// and gradually reduces its alpha each recalc tick (max 16Hz).
// Current frame is drawn over the trail (trail = background).
class csRenderSlowFadingBackground : public csRenderSlowFadingBase {
protected:
    csColorRGBA composePixel(csColorRGBA cur, csColorRGBA trail) const override {
        // Current frame over trail (trail = background)
        return csColorRGBA::sourceOverStraight(trail, cur);
    }
};

// Effect: slow fading overlay ("slow motion") that draws the accumulated trail over the current frame.
//
// Key idea:
// - We keep an internal buffer that accumulates pixels slowly (per-frame), then fades it over time.
// - The displayed frame is computed from the accumulated buffer + (optionally) a direct contribution
//   of the current frame.
//
// Why this exists / transparent background gotcha:
// - If the current frame contains a fully opaque pixel on a fully transparent background
//   (typical "particles" / "snowflakes" style effect), showing `cur` directly makes that pixel
//   appear instantly, even if the buffer is accumulating slowly.
// - To achieve a slow reveal, we must avoid showing `cur` as-is and instead drive visibility mainly
//   from the accumulated buffer.
//
// Output model:
// - First we accumulate:     accumulated = sourceOverStraight(trail, cur, accumulationAlpha)
// - Then we display:         out = sourceOverStraight(accumulated, cur, directAlpha)
//
// Tuning:
// - `directAlpha = 0`   -> pure slow reveal (no instant "pop" on transparent background)
// - `directAlpha = 255` -> instant visibility of current frame (buffer mainly adds fading trail)
class csRenderSlowFadingOverlay : public csRenderSlowFadingBase {
public:
    static constexpr uint8_t base = csRenderSlowFadingBase::propLast;
    static constexpr uint8_t propDirectAlpha = base + 1;
    static constexpr uint8_t propLast = propDirectAlpha;

    // Default is higher than SlowFadingBackground for stronger blur effect.
    csRenderSlowFadingOverlay() {
        fadeAlpha = 240;
    }

    // Property introspection
    uint8_t getPropsCount() const override {
        return propLast;
    }

    void getPropInfo(uint8_t propNum, csPropInfo& info) override {
        csRenderSlowFadingBase::getPropInfo(propNum, info);
        if (propNum == propDirectAlpha) {
            info.valueType = PropType::UInt8;
            info.name = "Direct alpha";
            info.valuePtr = &directAlpha;
            info.readOnly = false;
            info.disabled = false;
        }
    }

protected:
    // How much of the current frame is directly visible (0..255).
    // 0   -> fully driven by accumulated buffer (slow reveal)
    // 255 -> current frame appears instantly (buffer mostly affects the fade-out trail)
    uint8_t directAlpha = 0;

    csColorRGBA composePixel(csColorRGBA cur, csColorRGBA trail) const override {
        // Trail over current frame (trail = foreground)
        return csColorRGBA::sourceOverStraight(cur, trail);
    }

    void processPixel(csMatrixPixels& frame,
                      tMatrixPixelsCoord fx,
                      tMatrixPixelsCoord fy,
                      tMatrixPixelsSize x,
                      tMatrixPixelsSize y,
                      csColorRGBA cur,
                      csColorRGBA trail) override {
        // Accumulate: current frame over existing trail in buffer (slowly).
        // NOTE: accumulationAlpha is currently fixed to 16.
        // TODO: `accumulationAlpha=16` - make a property.
        const csColorRGBA accumulated = csColorRGBA::sourceOverStraight(trail, cur, 16);
        buffer->setPixelRewrite(x, y, accumulated);

        // Display: accumulated buffer, plus optional direct feed-through of the current frame.
        const csColorRGBA composite = csColorRGBA::sourceOverStraight(accumulated, cur, directAlpha);
        frame.setPixelRewrite(fx, fy, composite);
    }
};

} // namespace amp
