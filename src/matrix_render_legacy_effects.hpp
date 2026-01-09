#pragma once

#include <stdint.h>
#include <math.h>

#include "color_rgba.hpp"
#include "matrix_pixels.hpp"
#include "matrix_render.hpp"
#include "matrix_render_efffects.hpp"
#include "rand_gen.hpp"

namespace amp {

// Helper function to generate random RGB color
inline csColorRGBA getRandomColor(csRandGen& rand) {
    return csColorRGBA{
        255,
        rand.rand(),
        rand.rand(),
        rand.rand()
    };
}

// Helper function to convert 1D index to 2D coordinates (row-major order)
inline void indexTo2D(uint32_t index, tMatrixPixelsSize width, tMatrixPixelsSize height,
                      tMatrixPixelsCoord& x, tMatrixPixelsCoord& y) {
    if (width == 0) {
        x = 0;
        y = 0;
        return;
    }
    y = to_coord(index / width);
    x = to_coord(index % width);
    if (y >= to_coord(height)) {
        y = to_coord(height - 1);
        x = to_coord(width - 1);
    }
}

// Helper function to convert 2D coordinates to 1D index (row-major order)
inline uint32_t coordsTo1D(tMatrixPixelsCoord x, tMatrixPixelsCoord y, tMatrixPixelsSize width) {
    if (x < 0 || y < 0) {
        return 0;
    }
    return static_cast<uint32_t>(y) * static_cast<uint32_t>(width) + static_cast<uint32_t>(x);
}

// Helper function to get total pixel count from matrix
inline uint32_t getMatrixPixelCount(csMatrixPixels* matrix) {
    if (!matrix) {
        return 0;
    }
    return static_cast<uint32_t>(matrix->width()) * static_cast<uint32_t>(matrix->height());
}

// Background color effect using direct colors
// Uses transparent color (0x00000000) to indicate transparent pixels
class csRenderBackgroundColorIndex : public csRenderMatrixBase {
public:
    static constexpr uint8_t base = csRenderMatrixBase::propLast;
    static constexpr uint8_t propUseRandomColor = base + 1;
    static constexpr uint8_t propLast = propUseRandomColor;
    
    // Local pixel colors
    csColorRGBA* pixels = nullptr;
    uint32_t pixelCount = 0;
    
    // true = random color for each pixel, false = use selectedColor
    bool useRandomColor = true;
    csColorRGBA selectedColor{255, 255, 255, 255};
    
    // Internal random generator
    csRandGen int_rand;
    
    ~csRenderBackgroundColorIndex() override {
        delete[] pixels;
    }
    
    void fillColor(csColorRGBA setColor) {
        if (!pixels) {
            return;
        }
        for (uint32_t i = 0; i < pixelCount; ++i) {
            pixels[i] = setColor;
        }
    }
    
    void init(csRandGen& rand) {
        int_rand.addEntropy(rand.rand());
        int_rand.addEntropy(rand.rand());
        
        if (!matrix) {
            return;
        }
        
        const uint32_t count = getMatrixPixelCount(matrix);
        if (count != pixelCount) {
            delete[] pixels;
            pixels = new csColorRGBA[count];
            pixelCount = count;
        }
        
        for (uint32_t i = 0; i < pixelCount; ++i) {
            pixels[i] = getNewColor(i);
        }
    }
    
    void reset() {
        fillColor(csColorRGBA{0, 0, 0, 0}); // Transparent
        useRandomColor = true;
    }
    
    csColorRGBA getNewColor(uint32_t /*pos*/) {
        if (useRandomColor) {
            return getRandomColor(int_rand);
        } else {
            return selectedColor;
        }
    }
    
    uint8_t getPropsCount() const override {
        return propLast;
    }
    
    void getPropInfo(uint8_t propNum, csPropInfo& info) override {
        csRenderMatrixBase::getPropInfo(propNum, info);
        switch (propNum) {
            case propUseRandomColor:
                info.valueType = PropType::Bool;
                info.name = "Use random color";
                info.valuePtr = &useRandomColor;
                info.readOnly = false;
                info.disabled = false;
                break;
            case propColor:
                info.valueType = PropType::Color;
                info.name = "Selected color";
                info.valuePtr = &selectedColor;
                info.readOnly = false;
                info.disabled = false;
                break;
        }
    }
    
    void propChanged(uint8_t propNum) override {
        csRenderMatrixBase::propChanged(propNum);
        if (propNum == propMatrixDest) {
            init(int_rand);
        }
    }
    
    void render(csRandGen& /*rand*/, tTime /*currTime*/) const override {
        if (disabled || !matrix || !pixels) {
            return;
        }
        
        const csRect target = rectDest.intersect(matrix->getRect());
        if (target.empty()) {
            return;
        }
        
        const tMatrixPixelsCoord endX = target.x + to_coord(target.width);
        const tMatrixPixelsCoord endY = target.y + to_coord(target.height);
        
        uint32_t pixelIdx = 0;
        for (tMatrixPixelsCoord y = target.y; y < endY; ++y) {
            for (tMatrixPixelsCoord x = target.x; x < endX; ++x) {
                if (pixelIdx < pixelCount && pixels[pixelIdx].value != 0) {
                    matrix->setPixel(x, y, pixels[pixelIdx]);
                }
                ++pixelIdx;
            }
        }
    }
};

// Background color effect using direct colors
class csRenderBackgroundColor : public csRenderMatrixBase {
public:
    static constexpr uint8_t base = csRenderMatrixBase::propLast;
    static constexpr uint8_t propUseRandomColor = base + 1;
    static constexpr uint8_t propLast = propUseRandomColor;
    
    // Local pixel colors
    csColorRGBA* pixels = nullptr;
    uint32_t pixelCount = 0;
    
    // true = random color for each pixel, false = use selectedColor
    bool useRandomColor = true;
    csColorRGBA selectedColor{255, 255, 255, 255};
    
    // Internal random generator
    csRandGen int_rand;
    
    ~csRenderBackgroundColor() override {
        delete[] pixels;
    }
    
    void fillColor(csColorRGBA setColor) {
        if (!pixels) {
            return;
        }
        for (uint32_t i = 0; i < pixelCount; ++i) {
            pixels[i] = setColor;
        }
    }
    
    void init(csRandGen& rand) {
        int_rand.addEntropy(rand.rand());
        int_rand.addEntropy(rand.rand());
        
        if (!matrix) {
            return;
        }
        
        const uint32_t count = getMatrixPixelCount(matrix);
        if (count != pixelCount) {
            delete[] pixels;
            pixels = new csColorRGBA[count];
            pixelCount = count;
        }
        
        for (uint32_t i = 0; i < pixelCount; ++i) {
            pixels[i] = getNewColor(i);
        }
    }
    
    void reset() {
        fillColor(csColorRGBA{0, 0, 0, 0}); // Transparent
        useRandomColor = true;
    }
    
    csColorRGBA getNewColor(uint32_t /*pos*/) {
        if (useRandomColor) {
            return getRandomColor(int_rand);
        } else {
            return selectedColor;
        }
    }
    
    uint8_t getPropsCount() const override {
        return propLast;
    }
    
    void getPropInfo(uint8_t propNum, csPropInfo& info) override {
        csRenderMatrixBase::getPropInfo(propNum, info);
        switch (propNum) {
            case propUseRandomColor:
                info.valueType = PropType::Bool;
                info.name = "Use random color";
                info.valuePtr = &useRandomColor;
                info.readOnly = false;
                info.disabled = false;
                break;
            case propColor:
                info.valueType = PropType::Color;
                info.name = "Selected color";
                info.valuePtr = &selectedColor;
                info.readOnly = false;
                info.disabled = false;
                break;
        }
    }
    
    void propChanged(uint8_t propNum) override {
        csRenderMatrixBase::propChanged(propNum);
        if (propNum == propMatrixDest) {
            init(int_rand);
        }
    }
    
    void render(csRandGen& /*rand*/, tTime /*currTime*/) const override {
        if (disabled || !matrix || !pixels) {
            return;
        }
        
        const csRect target = rectDest.intersect(matrix->getRect());
        if (target.empty()) {
            return;
        }
        
        const tMatrixPixelsCoord endX = target.x + to_coord(target.width);
        const tMatrixPixelsCoord endY = target.y + to_coord(target.height);
        
        uint32_t pixelIdx = 0;
        for (tMatrixPixelsCoord y = target.y; y < endY; ++y) {
            for (tMatrixPixelsCoord x = target.x; x < endX; ++x) {
                if (pixelIdx < pixelCount) {
                    const csColorRGBA& px = pixels[pixelIdx];
                    // Only draw if not transparent black
                    if (px.value != 0) {
                        matrix->setPixel(x, y, px);
                    }
                }
                ++pixelIdx;
            }
        }
    }
};

// Simple color fill effect
class csRenderColor : public csRenderMatrixBase {
public:
    csColorRGBA color{255, 255, 255, 255};
    
    void getPropInfo(uint8_t propNum, csPropInfo& info) override {
        csRenderMatrixBase::getPropInfo(propNum, info);
        switch (propNum) {
            case propColor:
                info.valueType = PropType::Color;
                info.name = "Color";
                info.valuePtr = &color;
                info.readOnly = false;
                info.disabled = false;
                break;
        }
    }
    
    void render(csRandGen& /*rand*/, tTime /*currTime*/) const override {
        if (disabled || !matrix) {
            return;
        }
        
        const csRect target = rectDest.intersect(matrix->getRect());
        if (target.empty()) {
            return;
        }
        
        const tMatrixPixelsCoord endX = target.x + to_coord(target.width);
        const tMatrixPixelsCoord endY = target.y + to_coord(target.height);
        
        for (tMatrixPixelsCoord y = target.y; y < endY; ++y) {
            for (tMatrixPixelsCoord x = target.x; x < endX; ++x) {
                matrix->setPixel(x, y, color);
            }
        }
    }
};

// Color spectrum effect - displays rainbow colors across the matrix
class csRenderPalette : public csRenderMatrixBase {
public:
    static constexpr uint8_t base = csRenderMatrixBase::propLast;
    static constexpr uint8_t propWide = base + 1;
    static constexpr uint8_t propLast = propWide;
    
    uint8_t wide = 1;
    
    uint8_t getPropsCount() const override {
        return propLast;
    }
    
    void getPropInfo(uint8_t propNum, csPropInfo& info) override {
        csRenderMatrixBase::getPropInfo(propNum, info);
        switch (propNum) {
            case propWide:
                info.valueType = PropType::UInt8;
                info.name = "Wide";
                info.valuePtr = &wide;
                info.readOnly = false;
                info.disabled = false;
                break;
        }
    }
    
    // Simple HSV to RGB conversion for rainbow
    static csColorRGBA hsvToRgb(uint8_t h, uint8_t s, uint8_t v) {
        uint8_t r, g, b;
        
        if (s == 0) {
            r = g = b = v;
        } else {
            const uint8_t region = h / 43;
            const uint8_t remainder = (h - (region * 43)) * 6;
            
            const uint8_t p = (v * (255 - s)) >> 8;
            const uint8_t q = (v * (255 - ((s * remainder) >> 8))) >> 8;
            const uint8_t t = (v * (255 - ((s * (255 - remainder)) >> 8))) >> 8;
            
            switch (region) {
                case 0:
                    r = v; g = t; b = p;
                    break;
                case 1:
                    r = q; g = v; b = p;
                    break;
                case 2:
                    r = p; g = v; b = t;
                    break;
                case 3:
                    r = p; g = q; b = v;
                    break;
                case 4:
                    r = t; g = p; b = v;
                    break;
                default:
                    r = v; g = p; b = q;
                    break;
            }
        }
        
        return csColorRGBA{255, r, g, b};
    }
    
    void render(csRandGen& /*rand*/, tTime /*currTime*/) const override {
        if (disabled || !matrix) {
            return;
        }
        
        const csRect target = rectDest.intersect(matrix->getRect());
        if (target.empty()) {
            return;
        }
        
        const tMatrixPixelsCoord endX = target.x + to_coord(target.width);
        const tMatrixPixelsCoord endY = target.y + to_coord(target.height);
        
        uint32_t pixelIdx = 0;
        for (tMatrixPixelsCoord y = target.y; y < endY; ++y) {
            for (tMatrixPixelsCoord x = target.x; x < endX; ++x) {
                // Create rainbow spectrum: hue cycles through 0-255
                const uint8_t hue = static_cast<uint8_t>((pixelIdx / wide) % 256);
                const csColorRGBA color = hsvToRgb(hue, 255, 255);
                matrix->setPixel(x, y, color);
                ++pixelIdx;
            }
        }
    }
};

// Random colors effect - generates random RGB colors
class csRenderRandColors : public csRenderDynamic {
public:
    void render(csRandGen& rand, tTime /*currTime*/) const override {
        if (disabled || !matrix) {
            return;
        }
        
        const csRect target = rectDest.intersect(matrix->getRect());
        if (target.empty()) {
            return;
        }
        
        const tMatrixPixelsCoord endX = target.x + to_coord(target.width);
        const tMatrixPixelsCoord endY = target.y + to_coord(target.height);
        
        for (tMatrixPixelsCoord y = target.y; y < endY; ++y) {
            for (tMatrixPixelsCoord x = target.x; x < endX; ++x) {
                matrix->setPixel(x, y, getRandomColor(rand));
            }
        }
    }
};

// Random drop effect
class csRenderColorRandDrop : public csRenderDynamic {
public:
    static constexpr uint8_t base = csRenderMatrixBase::propLast;
    static constexpr uint8_t propLevel = base + 1;
    static constexpr uint8_t propPercent = base + 2;
    static constexpr uint8_t propLast = propPercent;
    
    uint8_t level = 255;
    uint8_t percent = 127; // 255 = 100%, 127 = 50%, 0 = 0%
    uint8_t rand_seed = 0;
    
    uint8_t getPropsCount() const override {
        return propLast;
    }
    
    void getPropInfo(uint8_t propNum, csPropInfo& info) override {
        csRenderDynamic::getPropInfo(propNum, info);
        switch (propNum) {
            case propLevel:
                info.valueType = PropType::UInt8;
                info.name = "Level";
                info.valuePtr = &level;
                info.readOnly = false;
                info.disabled = false;
                break;
            case propPercent:
                info.valueType = PropType::UInt8;
                info.name = "Percent";
                info.valuePtr = &percent;
                info.readOnly = false;
                info.disabled = false;
                break;
        }
    }
    
    void recalc(csRandGen& rand, tTime /*currTime*/) override {
        rand_seed = rand.rand();
    }
    
    void render(csRandGen& /*rand*/, tTime /*currTime*/) const override {
        if (disabled || !matrix) {
            return;
        }
        
        const csRect target = rectDest.intersect(matrix->getRect());
        if (target.empty()) {
            return;
        }
        
        const tMatrixPixelsCoord endX = target.x + to_coord(target.width);
        const tMatrixPixelsCoord endY = target.y + to_coord(target.height);
        
        csRandGen local_rand(rand_seed);
        for (tMatrixPixelsCoord y = target.y; y < endY; ++y) {
            for (tMatrixPixelsCoord x = target.x; x < endX; ++x) {
                if (local_rand.rand() <= percent) {
                    matrix->setPixel(x, y, csColorRGBA{255, level, level, level});
                }
            }
        }
    }
};

// Single pixel moving effect
class csRenderPixel : public csRenderDynamic {
public:
    static constexpr uint8_t base = csRenderMatrixBase::propLast;
    static constexpr uint8_t propPos = base + 1;
    static constexpr uint8_t propDir = base + 2;
    static constexpr uint8_t propLast = propDir;
    
    tMatrixPixelsCoord offset = 0;
    tMatrixPixelsCoord pos = 0;
    uint8_t speed = 4; // multiplied by 10
    int8_t dir = 1; // -1 = move backward, +1 = move forward
    
    uint8_t getPropsCount() const override {
        return propLast;
    }
    
    void getPropInfo(uint8_t propNum, csPropInfo& info) override {
        csRenderDynamic::getPropInfo(propNum, info);
        switch (propNum) {
            case propSpeed:
                info.valuePtr = &speed;
                info.disabled = false;
                break;
            case propPos:
                info.valueType = PropType::Int32;
                info.name = "Position";
                info.valuePtr = &pos;
                info.readOnly = false;
                info.disabled = false;
                break;
            case propDir:
                info.valueType = PropType::Int8;
                info.name = "Direction";
                info.valuePtr = &dir;
                info.readOnly = false;
                info.disabled = false;
                break;
        }
    }
    
    void reset() {
        pos = 0;
        speed = 4;
        dir = 1;
    }
    
    void recalc(csRandGen& /*rand*/, tTime currTime) override {
        if (!matrix) {
            return;
        }
        
        const uint32_t totalPixels = getMatrixPixelCount(matrix);
        if (totalPixels == 0) {
            return;
        }
        
        const uint16_t timeStep = speed * 10;
        static uint16_t lastTime = 0;
        const uint16_t timeDelta = currTime - lastTime;
        
        if (timeDelta < timeStep) {
            return;
        }
        
        lastTime = currTime;
        
        const int32_t newpos = pos + (1 * dir);
        
        if (newpos >= static_cast<int32_t>(totalPixels)) {
            pos = 0;
        } else if (newpos < 0) {
            pos = static_cast<tMatrixPixelsCoord>(totalPixels - 1);
        } else {
            pos = static_cast<tMatrixPixelsCoord>(newpos);
        }
    }
    
    void render(csRandGen& /*rand*/, tTime /*currTime*/) const override {
        if (disabled || !matrix) {
            return;
        }
        
        const csRect target = rectDest.intersect(matrix->getRect());
        if (target.empty()) {
            return;
        }
        
        const tMatrixPixelsSize matrixWidth = matrix->width();
        tMatrixPixelsCoord x, y;
        indexTo2D(static_cast<uint32_t>(pos), matrixWidth, matrix->height(), x, y);
        
        if (x >= target.x && x < target.x + to_coord(target.width) &&
            y >= target.y && y < target.y + to_coord(target.height)) {
            matrix->setPixel(x, y, csColorRGBA{255, 255, 255, 255});
        }
    }
};

// Line moving effect
class csRenderLine : public csRenderDynamic {
public:
    static constexpr uint8_t base = csRenderMatrixBase::propLast;
    static constexpr uint8_t propPos = base + 1;
    static constexpr uint8_t propLen = base + 2;
    static constexpr uint8_t propDir = base + 3;
    static constexpr uint8_t propTrimTail = base + 4;
    static constexpr uint8_t propLast = propTrimTail;
    
    tMatrixPixelsCoord offset = 0;
    tMatrixPixelsCoord pos = 0;
    uint8_t len = 16;
    uint8_t speed = 4; // multiplied by 10
    int8_t dir = 1; // -1 = move backward, +1 = move forward
    uint8_t trim_tail = 0;
    
    uint8_t getPropsCount() const override {
        return propLast;
    }
    
    void getPropInfo(uint8_t propNum, csPropInfo& info) override {
        csRenderDynamic::getPropInfo(propNum, info);
        switch (propNum) {
            case propSpeed:
                info.valuePtr = &speed;
                info.disabled = false;
                break;
            case propPos:
                info.valueType = PropType::Int32;
                info.name = "Position";
                info.valuePtr = &pos;
                info.readOnly = false;
                info.disabled = false;
                break;
            case propLen:
                info.valueType = PropType::UInt8;
                info.name = "Length";
                info.valuePtr = &len;
                info.readOnly = false;
                info.disabled = false;
                break;
            case propDir:
                info.valueType = PropType::Int8;
                info.name = "Direction";
                info.valuePtr = &dir;
                info.readOnly = false;
                info.disabled = false;
                break;
            case propTrimTail:
                info.valueType = PropType::UInt8;
                info.name = "Trim tail";
                info.valuePtr = &trim_tail;
                info.readOnly = false;
                info.disabled = false;
                break;
        }
    }
    
    void reset() {
        len = 16;
        pos = 0;
        speed = 4;
        dir = 1;
        trim_tail = len;
    }
    
    void recalc(csRandGen& /*rand*/, tTime currTime) override {
        if (!matrix) {
            return;
        }
        
        const uint32_t totalPixels = getMatrixPixelCount(matrix);
        if (totalPixels == 0) {
            return;
        }
        
        const uint16_t timeStep = speed * 10;
        static uint16_t lastTime = 0;
        const uint16_t timeDelta = currTime - lastTime;
        
        if (timeDelta < timeStep) {
            return;
        }
        
        lastTime = currTime;
        
        if (trim_tail > 0) {
            const int16_t s_trim_tail = static_cast<int16_t>(trim_tail) - 1;
            if (s_trim_tail < 0) {
                trim_tail = 0;
            } else {
                trim_tail = static_cast<uint8_t>(s_trim_tail);
            }
        }
        
        const int32_t newpos = pos + (1 * dir);
        
        if (newpos > static_cast<int32_t>(totalPixels + len)) {
            pos = 0;
            trim_tail = len;
        } else if (newpos < -static_cast<int32_t>(len)) {
            pos = static_cast<tMatrixPixelsCoord>(totalPixels - 1);
            trim_tail = len;
        } else {
            pos = static_cast<tMatrixPixelsCoord>(newpos);
        }
    }
    
    void render(csRandGen& /*rand*/, tTime /*currTime*/) const override {
        if (disabled || !matrix) {
            return;
        }
        
        const csRect target = rectDest.intersect(matrix->getRect());
        if (target.empty()) {
            return;
        }
        
        const tMatrixPixelsSize matrixWidth = matrix->width();
        
        for (uint8_t i = trim_tail; i < len; ++i) {
            const int32_t linePos = pos - (static_cast<int32_t>(i) * dir);
            if (linePos < 0) {
                continue;
            }
            
            tMatrixPixelsCoord x, y;
            indexTo2D(static_cast<uint32_t>(linePos), matrixWidth, matrix->height(), x, y);
            
            if (x >= target.x && x < target.x + to_coord(target.width) &&
                y >= target.y && y < target.y + to_coord(target.height)) {
                const uint8_t brightness = 255 - (i * (256 / len));
                matrix->setPixel(x, y, csColorRGBA{255, brightness, brightness, brightness});
            }
        }
    }
};

// White noise effect
class csRenderWhiteNoise : public csRenderDynamic {
public:
    static constexpr uint8_t base = csRenderMatrixBase::propLast;
    static constexpr uint8_t propLevel = base + 1;
    static constexpr uint8_t propLast = propLevel;
    
    uint8_t level = 8;
    
    uint8_t getPropsCount() const override {
        return propLast;
    }
    
    void getPropInfo(uint8_t propNum, csPropInfo& info) override {
        csRenderDynamic::getPropInfo(propNum, info);
        switch (propNum) {
            case propLevel:
                info.valueType = PropType::UInt8;
                info.name = "Level";
                info.valuePtr = &level;
                info.readOnly = false;
                info.disabled = false;
                break;
        }
    }
    
    void render(csRandGen& rand, tTime /*currTime*/) const override {
        if (disabled || !matrix) {
            return;
        }
        
        const csRect target = rectDest.intersect(matrix->getRect());
        if (target.empty()) {
            return;
        }
        
        const tMatrixPixelsCoord endX = target.x + to_coord(target.width);
        const tMatrixPixelsCoord endY = target.y + to_coord(target.height);
        
        for (tMatrixPixelsCoord y = target.y; y < endY; ++y) {
            for (tMatrixPixelsCoord x = target.x; x < endX; ++x) {
                const uint8_t brightness = rand.randRange(0, level);
                matrix->setPixel(x, y, csColorRGBA{255, brightness, brightness, brightness});
            }
        }
    }
};

// Gradient effect
class csRenderGradient : public csRenderMatrixBase {
public:
    void render(csRandGen& /*rand*/, tTime /*currTime*/) const override {
        if (disabled || !matrix) {
            return;
        }
        
        const csRect target = rectDest.intersect(matrix->getRect());
        if (target.empty()) {
            return;
        }
        
        const tMatrixPixelsCoord endX = target.x + to_coord(target.width);
        const tMatrixPixelsCoord endY = target.y + to_coord(target.height);
        const uint32_t totalPixels = getMatrixPixelCount(matrix);
        
        if (totalPixels == 0) {
            return;
        }
        
        const uint8_t dec = static_cast<uint8_t>((256 / totalPixels) + 1);
        int16_t b = 255;
        uint32_t pixelIdx = 0;
        
        for (tMatrixPixelsCoord y = target.y; y < endY; ++y) {
            for (tMatrixPixelsCoord x = target.x; x < endX; ++x) {
                if (pixelIdx < totalPixels - 1) {
                    const int16_t clampedB = (b < 0) ? 0 : ((b > 255) ? 255 : b);
                    const uint8_t brightness = static_cast<uint8_t>(clampedB);
                    matrix->setPixel(x, y, csColorRGBA{255, brightness, brightness, brightness});
                    b -= dec;
                    if (b < 0) {
                        break;
                    }
                }
                ++pixelIdx;
            }
            if (b < 0) {
                break;
            }
        }
    }
};

// Sinus wave effect
class csRenderSinusWave : public csRenderDynamic {
public:
    static constexpr uint8_t base = csRenderMatrixBase::propLast;
    static constexpr uint8_t propLen = base + 1;
    static constexpr uint8_t propPos = base + 2;
    static constexpr uint8_t propDir = base + 3;
    static constexpr uint8_t propLast = propDir;
    
    uint8_t len = 16;
    uint8_t pos = 0;
    uint8_t offset = 0;
    uint8_t speed = 64;
    int8_t dir = 1;
    
    uint8_t getPropsCount() const override {
        return propLast;
    }
    
    void getPropInfo(uint8_t propNum, csPropInfo& info) override {
        csRenderDynamic::getPropInfo(propNum, info);
        switch (propNum) {
            case propSpeed:
                info.valuePtr = &speed;
                info.disabled = false;
                break;
            case propLen:
                info.valueType = PropType::UInt8;
                info.name = "Length";
                info.valuePtr = &len;
                info.readOnly = false;
                info.disabled = false;
                break;
            case propPos:
                info.valueType = PropType::UInt8;
                info.name = "Position";
                info.valuePtr = &pos;
                info.readOnly = false;
                info.disabled = false;
                break;
            case propDir:
                info.valueType = PropType::Int8;
                info.name = "Direction";
                info.valuePtr = &dir;
                info.readOnly = false;
                info.disabled = false;
                break;
        }
    }
    
    void recalc(csRandGen& /*rand*/, tTime currTime) override {
        if (!matrix) {
            return;
        }
        
        offset = static_cast<uint8_t>(currTime / speed);
        if (dir == 0) {
            const uint32_t totalPixels = getMatrixPixelCount(matrix);
            offset = static_cast<uint8_t>(totalPixels - 1 - offset);
        }
        offset = static_cast<uint8_t>(offset + pos);
    }
    
    void render(csRandGen& /*rand*/, tTime /*currTime*/) const override {
        if (disabled || !matrix) {
            return;
        }
        
        const csRect target = rectDest.intersect(matrix->getRect());
        if (target.empty()) {
            return;
        }
        
        const tMatrixPixelsCoord endX = target.x + to_coord(target.width);
        const tMatrixPixelsCoord endY = target.y + to_coord(target.height);
        
        uint32_t pixelIdx = 0;
        for (tMatrixPixelsCoord y = target.y; y < endY; ++y) {
            for (tMatrixPixelsCoord x = target.x; x < endX; ++x) {
                // Simple sin approximation: sin8-like function
                const uint8_t phase = static_cast<uint8_t>((offset + pixelIdx) * len);
                const float sinVal = sinf(static_cast<float>(phase) * 3.14159f / 128.0f);
                const uint8_t brightness = static_cast<uint8_t>((sinVal + 1.0f) * 127.5f);
                matrix->setPixel(x, y, csColorRGBA{255, brightness, brightness, brightness});
                ++pixelIdx;
            }
        }
    }
};

// Saturating subtract: result = max(0, a - b)
inline uint8_t qsub8(uint8_t a, uint8_t b) {
    const int16_t result = static_cast<int16_t>(a) - static_cast<int16_t>(b);
    return (result < 0) ? 0 : static_cast<uint8_t>(result);
}

// Saturating subtract with multiplier
inline uint8_t qsub8_mult(uint8_t a, uint8_t b, uint8_t mult) {
    const int16_t result = static_cast<int16_t>(a) - (static_cast<int16_t>(b) * static_cast<int16_t>(mult));
    return (result < 0) ? 0 : static_cast<uint8_t>(result);
}

// Simple 8-bit sine approximation (0-255 input, 0-255 output)
inline uint8_t sin8(uint8_t x) {
    // Simple approximation: use sin() function normalized to 0-255
    const float sinVal = sinf(static_cast<float>(x) * 3.14159f / 128.0f);
    return static_cast<uint8_t>((sinVal + 1.0f) * 127.5f);
}

// Simple shuffle function for pseudo-randomization
inline uint32_t shuffle32(uint32_t x) {
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    return x;
}

// Background color index decay effect (decreases brightness over time)
class csRenderBackgroundColorIndexDec : public csRenderDynamic {
public:
    static constexpr uint8_t base = csRenderMatrixBase::propLast;
    static constexpr uint8_t propTarget = base + 1;
    static constexpr uint8_t propLast = propTarget;
    
    csRenderBackgroundColorIndex* target = nullptr;
    uint8_t speed = 20;
    
    uint8_t getPropsCount() const override {
        return propLast;
    }
    
    void getPropInfo(uint8_t propNum, csPropInfo& info) override {
        csRenderDynamic::getPropInfo(propNum, info);
        switch (propNum) {
            case propSpeed:
                info.valuePtr = &speed;
                info.disabled = false;
                break;
            case propTarget:
                info.valueType = PropType::Ptr;
                info.name = "Target effect";
                info.valuePtr = &target;
                info.readOnly = false;
                info.disabled = false;
                break;
        }
    }
    
    void reset() {
        speed = 20;
    }
    
    void recalc(csRandGen& /*rand*/, tTime /*currTime*/) override {
        if (!target || !target->pixels) {
            return;
        }
        
        // Decrease brightness of each color channel
        for (uint32_t i = 0; i < target->pixelCount; ++i) {
            csColorRGBA& color = target->pixels[i];
            if (color.value != 0) { // Only process non-transparent pixels
                color.r = qsub8(color.r, 1);
                color.g = qsub8(color.g, 1);
                color.b = qsub8(color.b, 1);
                // If all channels become zero, make it transparent
                if (color.r == 0 && color.g == 0 && color.b == 0) {
                    color.value = 0;
                }
            }
        }
    }
};

// Splashes noise effect
class csRenderSplashesNoise : public csRenderDynamic {
public:
    static constexpr uint8_t base = csRenderMatrixBase::propLast;
    static constexpr uint8_t propLevel = base + 1;
    static constexpr uint8_t propLast = propLevel;
    
    static constexpr uint32_t splashesCountMax = 100; // Maximum number of splashes
    
    uint8_t speed = 16;
    uint8_t level = 255;
    
    struct Splash {
        tMatrixPixelsCoord pos;
        uint8_t level;
    };
    
    Splash* splashes = nullptr;
    uint32_t splashesCount = 0;
    bool initialized = false;
    
    ~csRenderSplashesNoise() override {
        delete[] splashes;
    }
    
    uint8_t getPropsCount() const override {
        return propLast;
    }
    
    void getPropInfo(uint8_t propNum, csPropInfo& info) override {
        csRenderDynamic::getPropInfo(propNum, info);
        switch (propNum) {
            case propSpeed:
                info.valuePtr = &speed;
                info.disabled = false;
                break;
            case propLevel:
                info.valueType = PropType::UInt8;
                info.name = "Level";
                info.valuePtr = &level;
                info.readOnly = false;
                info.disabled = false;
                break;
        }
    }
    
    void reset() {
        speed = 16;
        level = 255;
        initialized = false;
    }
    
    void init(csRandGen& rand) {
        if (!matrix) {
            return;
        }
        
        const uint32_t totalPixels = getMatrixPixelCount(matrix);
        if (totalPixels == 0) {
            return;
        }
        
        const uint32_t calculatedCount = totalPixels / 5;
        splashesCount = (calculatedCount < splashesCountMax) ? calculatedCount : splashesCountMax;
        
        if (splashesCount != 0 && !splashes) {
            splashes = new Splash[splashesCount];
        }
        
        for (uint32_t i = 0; i < splashesCount; ++i) {
            splashes[i].level = rand.randRange(128, 255);
            splashes[i].pos = static_cast<tMatrixPixelsCoord>(rand.rand(static_cast<uint8_t>(totalPixels)));
        }
        
        initialized = true;
    }
    
    void recalc(csRandGen& rand, tTime /*currTime*/) override {
        if (!matrix || !splashes) {
            return;
        }
        
        if (!initialized) {
            init(rand);
            return;
        }
        
        const uint32_t totalPixels = getMatrixPixelCount(matrix);
        
        for (uint32_t i = 0; i < splashesCount; ++i) {
            if (splashes[i].level == 0) {
                // New splash
                splashes[i].level = rand.randRange(level - (level / 4), level);
                splashes[i].pos = static_cast<tMatrixPixelsCoord>(rand.rand(static_cast<uint8_t>(totalPixels)));
            } else {
                splashes[i].level = qsub8(splashes[i].level, speed);
            }
        }
    }
    
    void render(csRandGen& /*rand*/, tTime /*currTime*/) const override {
        if (disabled || !matrix || !splashes) {
            return;
        }
        
        const csRect target = rectDest.intersect(matrix->getRect());
        if (target.empty()) {
            return;
        }
        
        const tMatrixPixelsSize matrixWidth = matrix->width();
        
        for (uint32_t i = 0; i < splashesCount; ++i) {
            if (splashes[i].level > 0) {
                tMatrixPixelsCoord x, y;
                indexTo2D(static_cast<uint32_t>(splashes[i].pos), matrixWidth, matrix->height(), x, y);
                
                if (x >= target.x && x < target.x + to_coord(target.width) &&
                    y >= target.y && y < target.y + to_coord(target.height)) {
                    matrix->setPixel(x, y, csColorRGBA{255, splashes[i].level, splashes[i].level, splashes[i].level});
                }
            }
        }
    }
};

// Rainbow wave effect (HSV color wheel)
class csRenderRainbowWave : public csRenderDynamic {
public:
    static constexpr uint8_t base = csRenderMatrixBase::propLast;
    static constexpr uint8_t propLen = base + 1;
    static constexpr uint8_t propPos = base + 2;
    static constexpr uint8_t propDir = base + 3;
    static constexpr uint8_t propLast = propDir;
    
    uint8_t len = 64;
    uint8_t pos = 0;
    uint16_t offset = 0;
    uint8_t speed = 64;
    int8_t dir = 1;
    
    uint8_t getPropsCount() const override {
        return propLast;
    }
    
    void getPropInfo(uint8_t propNum, csPropInfo& info) override {
        csRenderDynamic::getPropInfo(propNum, info);
        switch (propNum) {
            case propSpeed:
                info.valuePtr = &speed;
                info.disabled = false;
                break;
            case propLen:
                info.valueType = PropType::UInt8;
                info.name = "Length";
                info.valuePtr = &len;
                info.readOnly = false;
                info.disabled = false;
                break;
            case propPos:
                info.valueType = PropType::UInt8;
                info.name = "Position";
                info.valuePtr = &pos;
                info.readOnly = false;
                info.disabled = false;
                break;
            case propDir:
                info.valueType = PropType::Int8;
                info.name = "Direction";
                info.valuePtr = &dir;
                info.readOnly = false;
                info.disabled = false;
                break;
        }
    }
    
    void recalc(csRandGen& /*rand*/, tTime currTime) override {
        if (!matrix) {
            return;
        }
        
        offset = static_cast<uint16_t>(currTime / speed);
        if (dir == 0) {
            const uint32_t totalPixels = getMatrixPixelCount(matrix);
            offset = static_cast<uint16_t>(totalPixels - 1 - offset);
        }
        offset = static_cast<uint16_t>(offset + pos);
    }
    
    // Simple HSV to RGB conversion
    static csColorRGBA hsvToRgb(uint8_t h, uint8_t s, uint8_t v) {
        uint8_t r, g, b;
        
        if (s == 0) {
            r = g = b = v;
        } else {
            const uint8_t region = h / 43;
            const uint8_t remainder = (h - (region * 43)) * 6;
            
            const uint8_t p = (v * (255 - s)) >> 8;
            const uint8_t q = (v * (255 - ((s * remainder) >> 8))) >> 8;
            const uint8_t t = (v * (255 - ((s * (255 - remainder)) >> 8))) >> 8;
            
            switch (region) {
                case 0:
                    r = v; g = t; b = p;
                    break;
                case 1:
                    r = q; g = v; b = p;
                    break;
                case 2:
                    r = p; g = v; b = t;
                    break;
                case 3:
                    r = p; g = q; b = v;
                    break;
                case 4:
                    r = t; g = p; b = v;
                    break;
                default:
                    r = v; g = p; b = q;
                    break;
            }
        }
        
        return csColorRGBA{255, r, g, b};
    }
    
    void render(csRandGen& /*rand*/, tTime /*currTime*/) const override {
        if (disabled || !matrix) {
            return;
        }
        
        const csRect target = rectDest.intersect(matrix->getRect());
        if (target.empty()) {
            return;
        }
        
        const tMatrixPixelsCoord endX = target.x + to_coord(target.width);
        const tMatrixPixelsCoord endY = target.y + to_coord(target.height);
        
        uint32_t pixelIdx = 0;
        for (tMatrixPixelsCoord y = target.y; y < endY; ++y) {
            for (tMatrixPixelsCoord x = target.x; x < endX; ++x) {
                const uint8_t h = static_cast<uint8_t>(((pixelIdx + offset) * (256 / len)) % 256);
                const csColorRGBA color = hsvToRgb(h, 255, 255);
                matrix->setPixel(x, y, color);
                ++pixelIdx;
            }
        }
    }
};

// Exploded sinus noise effect
class csRenderExplodedSinusNoise : public csRenderDynamic {
public:
    static constexpr uint8_t base = csRenderMatrixBase::propLast;
    static constexpr uint8_t propPos = base + 1;
    static constexpr uint8_t propLast = propPos;
    
    uint8_t speed = 4;
    uint8_t pos = 0;
    uint8_t offset = 0;
    
    uint8_t getPropsCount() const override {
        return propLast;
    }
    
    void getPropInfo(uint8_t propNum, csPropInfo& info) override {
        csRenderDynamic::getPropInfo(propNum, info);
        switch (propNum) {
            case propSpeed:
                info.valuePtr = &speed;
                info.disabled = false;
                break;
            case propPos:
                info.valueType = PropType::UInt8;
                info.name = "Position";
                info.valuePtr = &pos;
                info.readOnly = false;
                info.disabled = false;
                break;
        }
    }
    
    void recalc(csRandGen& /*rand*/, tTime currTime) override {
        offset = static_cast<uint8_t>(pos + currTime / speed);
    }
    
    void render(csRandGen& /*rand*/, tTime /*currTime*/) const override {
        if (disabled || !matrix) {
            return;
        }
        
        const csRect target = rectDest.intersect(matrix->getRect());
        if (target.empty()) {
            return;
        }
        
        const tMatrixPixelsCoord endX = target.x + to_coord(target.width);
        const tMatrixPixelsCoord endY = target.y + to_coord(target.height);
        const tMatrixPixelsSize matrixWidth = matrix->width();
        
        for (tMatrixPixelsCoord y = target.y; y < endY; ++y) {
            for (tMatrixPixelsCoord x = target.x; x < endX; ++x) {
                const uint32_t shuffled = shuffle32(static_cast<uint32_t>(x + y * matrixWidth));
                const uint8_t brightness = sin8(static_cast<uint8_t>(shuffled * 8 + offset));
                matrix->setPixel(x, y, csColorRGBA{255, brightness, brightness, brightness});
            }
        }
    }
};

// Magic slow glowing effect
class csRenderMagicSlowGlowing : public csRenderDynamic {
public:
    static constexpr uint8_t base = csRenderMatrixBase::propLast;
    static constexpr uint8_t propPercent = base + 1;
    static constexpr uint8_t propLast = propPercent;
    
    static constexpr uint32_t splashesCount = 3;
    
    uint8_t speed = 4;
    uint8_t percent = 20;
    
    struct Glow {
        tMatrixPixelsCoord pos;
        int8_t level; // -126..127: negative = damping, positive = increasing, 127 = glowing
    };
    
    Glow glows[splashesCount];
    bool initialized = false;
    
    uint8_t getPropsCount() const override {
        return propLast;
    }
    
    void getPropInfo(uint8_t propNum, csPropInfo& info) override {
        csRenderDynamic::getPropInfo(propNum, info);
        switch (propNum) {
            case propSpeed:
                info.valuePtr = &speed;
                info.disabled = false;
                break;
            case propPercent:
                info.valueType = PropType::UInt8;
                info.name = "Percent";
                info.valuePtr = &percent;
                info.readOnly = false;
                info.disabled = false;
                break;
        }
    }
    
    void init(csRandGen& rand) {
        if (!matrix) {
            return;
        }
        
        const uint32_t totalPixels = getMatrixPixelCount(matrix);
        if (totalPixels == 0) {
            return;
        }
        
        for (uint32_t i = 0; i < splashesCount; ++i) {
            glows[i].level = 0;
            glows[i].pos = 0;
        }
        
        glows[0].level = 1;
        glows[0].pos = static_cast<tMatrixPixelsCoord>(rand.rand(static_cast<uint8_t>(totalPixels)));
        
        initialized = true;
    }
    
    void recalc(csRandGen& rand, tTime /*currTime*/) override {
        if (!matrix) {
            return;
        }
        
        if (!initialized) {
            init(rand);
            return;
        }
        
        const uint32_t totalPixels = getMatrixPixelCount(matrix);
        
        for (uint32_t i = 0; i < splashesCount; ++i) {
            if (glows[i].level == 0) {
                // Zero - try to create new glow
                if (rand.rand() <= (255 / percent)) {
                    glows[i].level = 1;
                    glows[i].pos = static_cast<tMatrixPixelsCoord>(rand.rand(static_cast<uint8_t>(totalPixels)));
                }
            } else if (glows[i].level == 127) {
                // Glowing - try to start damping
                if (rand.rand() <= (255 / percent * 2)) {
                    glows[i].level = -126;
                }
            } else if (glows[i].level < 0) {
                // Damping
                const int16_t newLevel = static_cast<int16_t>(glows[i].level) + static_cast<int16_t>(speed);
                if (newLevel >= 0) {
                    glows[i].level = 0;
                } else {
                    glows[i].level = static_cast<int8_t>(newLevel);
                }
            } else if (glows[i].level > 0) {
                // Increasing
                const int16_t newLevel = static_cast<int16_t>(glows[i].level) + static_cast<int16_t>(speed);
                if (newLevel >= 127) {
                    glows[i].level = 127;
                } else {
                    glows[i].level = static_cast<int8_t>(newLevel);
                }
            }
        }
    }
    
    void render(csRandGen& /*rand*/, tTime /*currTime*/) const override {
        if (disabled || !matrix) {
            return;
        }
        
        const csRect target = rectDest.intersect(matrix->getRect());
        if (target.empty()) {
            return;
        }
        
        const tMatrixPixelsSize matrixWidth = matrix->width();
        
        for (uint32_t i = 0; i < splashesCount; ++i) {
            if (glows[i].level != 0) {
                tMatrixPixelsCoord x, y;
                indexTo2D(static_cast<uint32_t>(glows[i].pos), matrixWidth, matrix->height(), x, y);
                
                if (x >= target.x && x < target.x + to_coord(target.width) &&
                    y >= target.y && y < target.y + to_coord(target.height)) {
                    const int16_t absLevel = (glows[i].level < 0) ? -static_cast<int16_t>(glows[i].level) : static_cast<int16_t>(glows[i].level);
                    const uint8_t brightness = static_cast<uint8_t>(absLevel * 2);
                    matrix->setPixel(x, y, csColorRGBA{255, brightness, brightness, brightness});
                }
            }
        }
    }
};

// Color explode effect (similar to rainbow wave)
class csRenderColorExplode1 : public csRenderRainbowWave {
public:
    // Inherits all functionality from csRenderRainbowWave
};

} // namespace amp

