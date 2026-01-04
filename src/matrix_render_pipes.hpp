#pragma once

#include "matrix_render.hpp"
#include "matrix_types.hpp"
#include "amp_progmem.hpp"

namespace amp {

// Base class for effects that use a source matrix.
class csRenderMatrixPipeBase : public csRenderMatrixBase {
public:
    static constexpr uint8_t base = csRenderMatrixBase::propLast;
    static constexpr uint8_t propMatrixSource = base + 1;
    static constexpr uint8_t propRectSource = base + 2;
    static constexpr uint8_t propLast = propRectSource;

    // Source matrix pointer (nullptr means no source).
    csMatrixPixels* matrixSource = nullptr;

    // Source rectangle (defines area to copy from source matrix).
    csRect rectSource;

    uint8_t getPropsCount() const override {
        return propLast;
    }

    void getPropInfo(uint8_t propNum, csPropInfo& info) override {
        csRenderMatrixBase::getPropInfo(propNum, info);
        switch (propNum) {
            case propMatrixSource:
                info.type = PropType::Matrix;
                info.name = "Matrix source";
                info.ptr = &matrixSource;
                info.readOnly = false;
                info.disabled = false;
                break;
            case propRectSource:
                info.type = PropType::Rect;
                info.name = "Rect source";
                info.ptr = &rectSource;
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

// Effect: average area by computing average color of source area and filling destination area with it.
// Can average area of the destination matrix itself by setting matrixSource to the same matrix (`matrixSource = matrix`).
class csRenderAverageArea : public csRenderMatrixPipeBase {
public:
    void render(csRandGen& /*rand*/, tTime /*currTime*/) const override {
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

    void render(csRandGen& /*rand*/, tTime /*currTime*/) const override {
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
    static constexpr uint8_t base = csRenderMatrixPipeBase::propLast;
    static constexpr uint8_t propRewrite = base + 1;
    static constexpr uint8_t propLast = propRewrite;

    bool rewrite = false;

    uint8_t getPropsCount() const override {
        return propLast;
    }

    void getPropInfo(uint8_t propNum, csPropInfo& info) override {
        csRenderMatrixPipeBase::getPropInfo(propNum, info);
        switch (propNum) {
            case propRewrite:
                info.type = PropType::Bool;
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

    void render(csRandGen& /*rand*/, tTime /*currTime*/) const override {
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

    void propChanged(uint8_t propNum) override {
        // Validate matrix size when matrix or matrixSource changes
        if (propNum == csRenderMatrixBase::propMatrixDest ||
            propNum == csRenderMatrixPipeBase::propMatrixSource) {
            if (matrix != nullptr && matrixSource != nullptr) {
                // If size is incorrect, disable effect by setting matrix to nullptr
                if ((matrix->height() != 1) ||
                    (matrix->width() != matrixSource->height() * matrixSource->width())) {
                    matrix = nullptr;
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
                info.type = PropType::Matrix;
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
            if (matrix != nullptr) {
                // Matrix must have height == 1 for 1D remapping
                if (matrix->height() != 1) {
                    matrix = nullptr;
                }
            }
        }

        csRenderRemapBase::propChanged(propNum);
    }
};

// Effect: slow fade trail from source matrix to destination.
// The effect keeps an internal buffer that accumulates the source
// and gradually reduces its alpha each recalc tick (max 16Hz).
class csRenderSlowFadingBackground : public csRenderMatrixPipeBase {
public:
    static constexpr uint8_t base = csRenderMatrixPipeBase::propLast;
    static constexpr uint8_t propFadeAlpha = base + 1;
    static constexpr uint8_t propLast = propFadeAlpha;

    // Internal buffer that stores the accumulated image.
    csMatrixPixels* buffer = nullptr;

    // Fade multiplier applied to each pixel alpha on recalc (0-255).
    uint8_t fadeAlpha = 224;

    // Simple timer to cap recalcs to ~16Hz.
    uint16_t lastRecalcTime = 0;

    uint8_t getPropsCount() const override {
        return propLast;
    }

    void getPropInfo(uint8_t propNum, csPropInfo& info) override {
        csRenderMatrixPipeBase::getPropInfo(propNum, info);
        if (propNum == propFadeAlpha) {
            info.type = PropType::UInt8;
            info.name = "Fade alpha";
            info.ptr = &fadeAlpha;
            info.readOnly = false;
            info.disabled = false;
        }
    }

    ~csRenderSlowFadingBackground() override {
        delete buffer;
    }

    void propChanged(uint8_t propNum) override {
        csRenderMatrixPipeBase::propChanged(propNum);
        if (propNum == propRectSource) {
            updateBuffer();
        }
    }

    void recalc(csRandGen& /*rand*/, tTime currTime) override {
        if (disabled) {
            return;
        }

        constexpr uint16_t kRecalcIntervalMs = 63; // ≈16Hz
        const uint16_t delta = static_cast<uint16_t>(currTime - lastRecalcTime);
        if (lastRecalcTime != 0 && delta < kRecalcIntervalMs) {
            return;
        }
        lastRecalcTime = currTime;

        fadeBuffer();
    }

    void render(csRandGen& /*rand*/, tTime /*currTime*/) const override {
        // Intentionally empty: rendering is handled in onFrameDone.
    }

    void onFrameDone(csMatrixPixels& frame, csRandGen& /*rand*/, tTime /*currTime*/) override {
        if (disabled) {
            return;
        }

        rectSource = frame.getRect();
        rectDest = rectSource;
        updateBuffer();
        if (!buffer) {
            return;
        }

        const tMatrixPixelsSize height = rectSource.height;
        const tMatrixPixelsSize width = rectSource.width;

        for (tMatrixPixelsSize y = 0; y < height; ++y) {
            for (tMatrixPixelsSize x = 0; x < width; ++x) {
                const tMatrixPixelsCoord fx = rectSource.x + to_coord(x);
                const tMatrixPixelsCoord fy = rectSource.y + to_coord(y);
                const csColorRGBA cur = frame.getPixel(fx, fy);
                const csColorRGBA trail = buffer->getPixel(x, y);
                const csColorRGBA composite = csColorRGBA::sourceOverStraight(trail, cur);
                buffer->setPixelRewrite(x, y, composite);
                frame.setPixelRewrite(fx, fy, composite);
            }
        }
    }

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

private:
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
                /*
                if (pixel.a < 4) {
                    if (pixel.a == 0) {
                        continue;
                    } else {
                        newAlpha = 0;
                    }
                } else {
                    newAlpha = mul8(pixel.a, fadeAlpha);
                }
                newAlpha = mul8(pixel.a, fadeAlpha);
                */
                
                if (pixel.a < 16) {
                    newAlpha = 0;
                } else {
                    newAlpha = pixel.a - 16;
                }
                if (newAlpha != pixel.a) {
                    pixel.a = newAlpha;
                    buffer->setPixelRewrite(x, y, pixel);
                }
            }
        }
    }
};

} // namespace amp
