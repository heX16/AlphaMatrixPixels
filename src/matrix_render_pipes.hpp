#pragma once

#include "matrix_render.hpp"
#include "matrix_types.hpp"
#include "amp_macros.hpp"
// #include <stdint.h>

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
                info.valueType = PropType::Matrix;
                info.name = "Matrix source";
                info.valuePtr = &matrixSource;
                info.readOnly = false;
                info.disabled = false;
                break;
            case propRectSource:
                info.valueType = PropType::Rect;
                info.name = "Rect source";
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
                info.valueType = PropType::Bool;
                info.name = "Rewrite";
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

// Base class for slow fading effects that accumulate frames in a buffer.
// Derived classes only need to override composePixel() to specify the blending order.
class csRenderSlowFadingBase : public csRenderPostFrame {
public:
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
        const csColorRGBA accumulated = csColorRGBA::sourceOverStraight(trail, cur, 16);
        buffer->setPixelRewrite(x, y, accumulated);

        // Display: accumulated buffer, plus optional direct feed-through of the current frame.
        const csColorRGBA composite = csColorRGBA::sourceOverStraight(accumulated, cur, directAlpha);
        frame.setPixelRewrite(fx, fy, composite);
    }
};

} // namespace amp
