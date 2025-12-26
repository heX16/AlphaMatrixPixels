#pragma once

#include <stdint.h>
#include <math.h>

#include "color_rgba.hpp"
#include "matrix_pixels.hpp"
#include "rect.hpp"
#include "math.hpp"
#include "matrix_render.hpp"
#include "fonts.h"
#include "matrix_boolean.hpp"




namespace amp {

// Base class for gradient effects with scale parameter.
class csRenderDynamic : public csRenderMatrixBase {
public:
    // Scale parameter for effect size (FP16, default 1.0).
    // Increasing value (scale > 1.0) → larger scale → effect stretches → fewer details visible (like "zooming out");
    // decreasing value (scale < 1.0) → smaller scale → effect compresses → more details visible (like "zooming in").
    math::csFP16 scale{1.0f};
    // Speed parameter for effect animation speed (FP16, default 1.0).
    math::csFP16 speed{1.0f};

    // NOTE: uint8_t getParamsCount() - count dont changed

    void getParamInfo(uint8_t paramNum, csParamInfo& info) override {
        csRenderMatrixBase::getParamInfo(paramNum, info);
        if (paramNum == paramScale) {
            info.ptr = &scale;
            info.disabled = false;
            return;
        }
        if (paramNum == paramSpeed) {
            info.ptr = &speed;
            info.disabled = false;
            return;
        }
    }
};

// Simple animated RGB gradient (float).
class csRenderGradientWaves : public csRenderDynamic {
public:
    void render(csRandGen& /*rand*/, uint16_t currTime) const override {
        if (disabled || !matrix) {
            return;
        }
        const float t = static_cast<float>(currTime) * 0.001f * speed.to_float();

        auto wave = [](float v) -> uint8_t {
            return static_cast<uint8_t>((sin(v) * 0.5f + 0.5f) * 255.0f);
        };

        const csRect target = rect.intersect(matrix->getRect());
        if (target.empty()) {
            return;
        }

        const tMatrixPixelsCoord endX = target.x + to_coord(target.width);
        const tMatrixPixelsCoord endY = target.y + to_coord(target.height);
        const float scaleF = scale.to_float();
        // Invert scale: divide by scale so larger values stretch the waves (bigger scale = more stretched).
        const float invScaleF = (scaleF > 0.0f) ? (1.0f / scaleF) : 1.0f;
        for (tMatrixPixelsCoord y = target.y; y < endY; ++y) {
            for (tMatrixPixelsCoord x = target.x; x < endX; ++x) {
                const float xf = static_cast<float>(x) * 0.4f * invScaleF;
                const float yf = static_cast<float>(y) * 0.4f * invScaleF;
                const uint8_t r = wave(t * 0.8f + xf);
                const uint8_t g = wave(t * 1.0f + yf);
                const uint8_t b = wave(t * 0.6f + xf + yf * 0.5f);
                matrix->setPixel(x, y, csColorRGBA{255, r, g, b});
            }
        }
    }
};

// Animated RGB gradient using fixed-point for time/phase.
class csRenderGradientWavesFP : public csRenderDynamic {
public:
    // Fixed-point "wave" mapping phase -> [0..255]. Not constexpr because fp32_sin() uses sin() internally.
    static uint8_t wave_fp(math::csFP32 phase) noexcept {
        using namespace math;
        static const csFP32 half = csFP32::float_const(0.5f);
        static const csFP32 scale255{255};

        const csFP32 s = fp32_sin(phase);
        const csFP32 norm = s * half + half;      // [-1..1] -> [0..1]
        const csFP32 scaled = norm * scale255;    // [0..255]
        int v = scaled.round_int();
        if (v < 0) v = 0;
        else if (v > 255) v = 255;
        return static_cast<uint8_t>(v);
    }

    void render(csRandGen& /*rand*/, uint16_t currTime) const override {
        if (disabled || !matrix) {
            return;
        }
        using namespace math;

        // Convert milliseconds to seconds in 16.16 fixed-point WITHOUT float:
        // t_raw = currTime * 65536 / 1000
        const int32_t t_raw = static_cast<int32_t>((static_cast<int64_t>(currTime) * csFP32::scale) / 1000);
        csFP32 t = csFP32::from_raw(t_raw);
        // Apply speed multiplier (convert speed from FP16 to FP32).
        const csFP32 speedFP32 = math::fp16_to_fp32(speed);
        t = t * speedFP32;

        // Fixed-point constants.
        static const csFP32 k08 = csFP32::float_const(0.7f);
        static const csFP32 k06 = csFP32::float_const(0.5f);
        static const csFP32 k04 = csFP32::float_const(0.3f);
        static const csFP32 k05 = csFP32::float_const(0.4f);
        // Convert scale from FP16 to FP32 and invert: divide by scale so larger values stretch the waves (bigger scale = more stretched).
        const csFP32 scaleFP32 = math::fp16_to_fp32(scale);
        static const csFP32 one = csFP32::float_const(1.0f);
        const csFP32 invScaleFP32 = (scaleFP32.raw > 0) ? (one / scaleFP32) : one;
        const csRect target = rect.intersect(matrix->getRect());
        if (target.empty()) {
            return;
        }

        const tMatrixPixelsCoord endX = target.x + to_coord(target.width);
        const tMatrixPixelsCoord endY = target.y + to_coord(target.height);
        for (tMatrixPixelsCoord y = target.y; y < endY; ++y) {
            const csFP32 yf = csFP32::from_int(y);
            const csFP32 yf_scaled = yf * k04 * invScaleFP32;
            for (tMatrixPixelsCoord x = target.x; x < endX; ++x) {
                const csFP32 xf = csFP32::from_int(x);
                const csFP32 xf_scaled = xf * k04 * invScaleFP32;
                const uint8_t r = wave_fp(t * k08 + xf_scaled);
                const uint8_t g = wave_fp(t + yf_scaled);
                const uint8_t b = wave_fp(t * k06 + xf_scaled + yf_scaled * k05);
                matrix->setPixel(x, y, csColorRGBA{255, r, g, b});
            }
        }
    }
};

// Simple sinusoidal plasma effect (float).
class csRenderPlasma : public csRenderDynamic {
public:
    void render(csRandGen& /*rand*/, uint16_t currTime) const override {
        if (disabled || !matrix) {
            return;
        }
        const float t = static_cast<float>(currTime) * 0.0025f * speed.to_float();
        const csRect target = rect.intersect(matrix->getRect());
        if (target.empty()) {
            return;
        }
        
        const float scaleF = scale.to_float();
        // Invert scale: divide by scale so larger values stretch the waves (bigger scale = more stretched).
        const float invScaleF = (scaleF > 0.0f) ? (1.0f / scaleF) : 1.0f;
        const tMatrixPixelsCoord endX = target.x + to_coord(target.width);
        const tMatrixPixelsCoord endY = target.y + to_coord(target.height);
        for (tMatrixPixelsCoord y = target.y; y < endY; ++y) {
            for (tMatrixPixelsCoord x = target.x; x < endX; ++x) {
                const float xf = static_cast<float>(x) * invScaleF;
                const float yf = static_cast<float>(y) * invScaleF;
                const float v = sin(xf * 0.35f + t) + sin(yf * 0.35f - t) + sin((xf + yf) * 0.25f + t * 0.5f);
                const float norm = (v + 3.0f) / 6.0f; // bring into [0..1]
                const uint8_t r = static_cast<uint8_t>(norm * 255.0f);
                const uint8_t g = static_cast<uint8_t>((1.0f - norm) * 255.0f);
                const uint8_t b = static_cast<uint8_t>((0.5f + 0.5f * sin(t + xf * 0.1f)) * 255.0f);
                matrix->setPixel(x, y, csColorRGBA{255, r, g, b});
            }
        }
    }
};

// Effect: draw a single digit glyph using the 3x5 font.
class csRenderGlyph : public csRenderMatrixBase {
public:
    static constexpr uint8_t base = csRenderMatrixBase::paramLast;
    static constexpr uint8_t paramSymbolIndex = base+1;
    static constexpr uint8_t paramFontWidth = base+2;
    static constexpr uint8_t paramFontHeight = base+3;
    static constexpr uint8_t paramLast = paramFontHeight;

    uint8_t symbolIndex = 0;
    csColorRGBA color{255, 255, 255, 255};
    csColorRGBA backgroundColor{255, 0, 0, 0};

    // Runtime font selection.
    // nullptr means "no font selected" (render will draw only background).
    const csFontBase* font = nullptr;

    // Cached font dimensions for UI/introspection.
    tMatrixPixelsSize fontWidth = 0;
    tMatrixPixelsSize fontHeight = 0;

    void setFont(const csFontBase& f) noexcept {
        font = &f;
        fontWidth = to_size(font->width());
        fontHeight = to_size(font->height());
        if (renderRectAutosize) {
            updateRenderRect();
        }
        if (symbolIndex >= font->count()) {
            symbolIndex = static_cast<uint8_t>((font->count() == 0) ? 0 : (font->count() - 1));
        }
    }

    uint8_t getParamsCount() const override {
        return paramLast;
    }

    void getParamInfo(uint8_t paramNum, csParamInfo& info) override {
        csRenderMatrixBase::getParamInfo(paramNum, info);
        switch (paramNum) {
            case paramRenderRectAutosize:
                info.disabled = true;
                break;
            case paramSymbolIndex:
                info.type = ParamType::UInt8;
                info.name = "Glyph index";
                info.ptr = &symbolIndex;
                info.readOnly = false;
                info.disabled = false;
                break;
            case paramColor:
                info.type = ParamType::Color;
                info.name = "Symbol color";
                info.ptr = &color;
                info.readOnly = false;
                info.disabled = false;
                break;
            case paramColorBackground:
                info.ptr = &backgroundColor;
                info.disabled = false;
                break;
            case paramFontWidth:
                info.type = ParamType::UInt16;
                info.name = "Font width";
                info.ptr = &fontWidth;
                info.readOnly = true;
                info.disabled = false;
                break;
            case paramFontHeight:
                info.type = ParamType::UInt16;
                info.name = "Font height";
                info.ptr = &fontHeight;
                info.readOnly = true;
                info.disabled = false;
                break;
        }
    }

    void paramChanged(uint8_t paramNum) override {
        csRenderMatrixBase::paramChanged(paramNum);
        if (!font) {
            return;
        }
        if (paramNum == paramSymbolIndex && symbolIndex >= font->count()) {
            symbolIndex = static_cast<uint8_t>((font->count() == 0) ? 0 : (font->count() - 1));
        }
    }

    void updateRenderRect() override {
        if (!renderRectAutosize || !font) {
            return;
        }

        rect = csRect{
            rect.x,
            rect.y,
            to_size(font->width()),
            to_size(font->height())
        };
    }

    void render(csRandGen& /*rand*/, uint16_t /*currTime*/) const override {
        if (disabled || !matrix || !font) {
            return;
        }

        const csRect target = rect.intersect(matrix->getRect());
        if (target.empty()) {
            return;
        }

        const tMatrixPixelsCoord endX = target.x + to_coord(target.width);
        const tMatrixPixelsCoord endY = target.y + to_coord(target.height);
        for (tMatrixPixelsCoord y = target.y; y < endY; ++y) {
            for (tMatrixPixelsCoord x = target.x; x < endX; ++x) {
                matrix->setPixel(x, y, backgroundColor);
            }
        }

        const tMatrixPixelsSize glyphWidth = math::min(rect.width, to_size(font->width()));
        const tMatrixPixelsSize glyphHeight = math::min(rect.height, to_size(font->height()));
        if (glyphWidth == 0 || glyphHeight == 0) {
            return;
        }

        const tMatrixPixelsCoord offsetX = rect.x + to_coord((rect.width - glyphWidth) / 2);
        const tMatrixPixelsCoord offsetY = rect.y + to_coord((rect.height - glyphHeight) / 2);

        const uint16_t safeIndex = (symbolIndex < font->count())
            ? static_cast<uint16_t>(symbolIndex)
            : static_cast<uint16_t>((font->count() == 0) ? 0 : (font->count() - 1));
        for (tMatrixPixelsSize row = 0; row < glyphHeight; ++row) {
            const uint32_t glyphRow = font->rowBits(safeIndex, static_cast<uint16_t>(row));
            for (tMatrixPixelsSize col = 0; col < glyphWidth; ++col) {
                const uint32_t mask = 1u << (31u - static_cast<uint32_t>(col));
                if ((glyphRow & mask) == 0u) {
                    continue;
                }
                const tMatrixPixelsCoord px = offsetX + to_coord(col);
                const tMatrixPixelsCoord py = offsetY + to_coord(row);
                matrix->setPixel(px, py, color);
            }
        }
    }
};

// Effect: draw a filled circle inscribed into rect.
class csRenderCircle : public csRenderMatrixBase {
public:
    static constexpr uint8_t base = csRenderMatrixBase::paramLast;
    static constexpr uint8_t paramSmoothEdges = base + 1;
    static constexpr uint8_t paramLast = paramSmoothEdges;

    csColorRGBA color{255, 255, 255, 255};
    csColorRGBA backgroundColor{0, 0, 0, 0};
    bool smoothEdges = true;

    uint8_t getParamsCount() const override {
        return paramLast;
    }

    void getParamInfo(uint8_t paramNum, csParamInfo& info) override {
        switch (paramNum) {
            case paramColor:
                info.type = ParamType::Color;
                info.name = "Circle color";
                info.ptr = &color;
                info.readOnly = false;
                info.disabled = false;
                break;
            case paramColorBackground:
                info.ptr = &backgroundColor;
                info.disabled = false;
                break;
            case paramSmoothEdges:
                info.type = ParamType::Bool;
                info.name = "Smooth edges";
                info.ptr = &smoothEdges;
                info.readOnly = false;
                info.disabled = false;
                break;
            default:
                csRenderMatrixBase::getParamInfo(paramNum, info);
                break;
        }
    }

    void render(csRandGen& /*rand*/, uint16_t /*currTime*/) const override {
        if (disabled || !matrix) {
            return;
        }

        const csRect target = rect.intersect(matrix->getRect());
        if (target.empty()) {
            return;
        }

        const tMatrixPixelsCoord endX = target.x + to_coord(target.width);
        const tMatrixPixelsCoord endY = target.y + to_coord(target.height);

        const float cx = static_cast<float>(target.x) + static_cast<float>(target.width) * 0.5f;
        const float cy = static_cast<float>(target.y) + static_cast<float>(target.height) * 0.5f;
        const float radius = static_cast<float>(math::min(target.width, target.height)) * 0.5f;
        const float radiusSq = radius * radius;
        const float aaHalfPixel = 0.5f; // simple 1px-wide anti-aliasing ramp

        for (tMatrixPixelsCoord y = target.y; y < endY; ++y) {
            const float py = static_cast<float>(y) + 0.5f;
            const float dy = py - cy;
            const float dySq = dy * dy;
            for (tMatrixPixelsCoord x = target.x; x < endX; ++x) {
                const float px = static_cast<float>(x) + 0.5f;
                const float dx = px - cx;
                const float distSq = dx * dx + dySq;
                if (!smoothEdges) {
                    const csColorRGBA color = (distSq <= radiusSq) ? color : backgroundColor;
                    matrix->setPixel(x, y, color);
                    continue;
                }

                // Smooth: compute coverage in [0..1] using linear ramp over ~1px border.
                const float dist = sqrtf(distSq);
                float coverage = radius + aaHalfPixel - dist;
                if (coverage <= 0.0f) {
                    coverage = 0.0f;
                } else if (coverage >= 1.0f) {
                    coverage = 1.0f;
                }

                // First lay down background (blended over existing content), then blend circle with coverage-scaled alpha.
                matrix->setPixel(x, y, backgroundColor);
                if (coverage > 0.0f) {
                    const uint8_t coverageAlpha = static_cast<uint8_t>(coverage * 255.0f + 0.5f);
                    matrix->setPixel(x, y, color, coverageAlpha);
                }
            }
        }
    }
};

// Optimized circle renderer: per-scanline chord computation, optional AA on edges.
class csRenderCircleFast : public csRenderCircle {
public:
    void render(csRandGen& /*rand*/, uint16_t /*currTime*/) const override {
        if (disabled || !matrix) {
            return;
        }

        const csRect target = rect.intersect(matrix->getRect());
        if (target.empty()) {
            return;
        }

        const float cx = static_cast<float>(target.x) + static_cast<float>(target.width) * 0.5f;
        const float cy = static_cast<float>(target.y) + static_cast<float>(target.height) * 0.5f;
        const float radius = static_cast<float>(math::min(target.width, target.height)) * 0.5f;
        if (radius <= 0.0f) {
            return;
        }

        const float radiusSq = radius * radius;
        const float aaWidth = 1.0f; // ~1px anti-aliased ramp
        const tMatrixPixelsCoord endY = target.y + to_coord(target.height);
        const tMatrixPixelsCoord endX = target.x + to_coord(target.width);

        for (tMatrixPixelsCoord y = target.y; y < endY; ++y) {
            const float py = static_cast<float>(y) + 0.5f;
            const float dy = py - cy;
            const float dySq = dy * dy;

            // If the entire scanline is outside the AA band, fill with background and continue.
            if (dySq > radiusSq + aaWidth * aaWidth) {
                for (tMatrixPixelsCoord x = target.x; x < endX; ++x) {
                    matrix->setPixel(x, y, backgroundColor);
                }
                continue;
            }

            float chord = radiusSq - dySq;
            if (chord < 0.0f) {
                chord = 0.0f;
            }
            const float dx = sqrtf(chord);

            const float leftF = cx - dx;
            const float rightF = cx + dx;
            const tMatrixPixelsCoord xLeft = static_cast<tMatrixPixelsCoord>(floorf(leftF));
            const tMatrixPixelsCoord xRight = static_cast<tMatrixPixelsCoord>(floorf(rightF));

            // Lay down background first (alpha-aware).
            for (tMatrixPixelsCoord x = target.x; x < endX; ++x) {
                matrix->setPixel(x, y, backgroundColor);
            }

            // Solid interior (no AA).
            for (tMatrixPixelsCoord x = xLeft + 1; x < xRight; ++x) {
                if (x < target.x || x >= endX) {
                    continue;
                }
                matrix->setPixel(x, y, color);
            }

            if (!smoothEdges) {
                // Hard edges: fill boundary pixels if inside bounds.
                if (xLeft >= target.x && xLeft < endX) {
                    matrix->setPixel(xLeft, y, color);
                }
                if (xRight >= target.x && xRight < endX && xRight != xLeft) {
                    matrix->setPixel(xRight, y, color);
                }
                continue;
            }

            // Anti-aliased edges on the two boundary columns.
            auto blendEdge = [&](tMatrixPixelsCoord x, float distToEdge) {
                if (x < target.x || x >= endX) {
                    return;
                }
                float coverage = aaWidth - distToEdge;
                if (coverage <= 0.0f) {
                    return;
                }
                if (coverage > 1.0f) {
                    coverage = 1.0f;
                }
                const uint8_t coverageAlpha = static_cast<uint8_t>(coverage * 255.0f + 0.5f);
                matrix->setPixel(x, y, color, coverageAlpha);
            };

            const float leftPixelCenter = static_cast<float>(xLeft) + 0.5f;
            const float rightPixelCenter = static_cast<float>(xRight) + 0.5f;

            blendEdge(xLeft, fabsf(leftPixelCenter - leftF));
            blendEdge(xRight, fabsf(rightF - rightPixelCenter));
        }
    }
};

// Radial gradient circle: interpolates from color (center) to backgroundColor (edge).
class csRenderCircleGradient : public csRenderCircle {
public:
    static constexpr uint8_t base = csRenderMatrixBase::paramLast;
    static constexpr uint8_t paramGradientOffset = base+1;
    static constexpr uint8_t paramLast = paramGradientOffset;

    // Offset of gradient start from center, normalized: 0..255 -> 0..1 of radius.
    uint8_t gradientOffset = 0;

    uint8_t getParamsCount() const override {
        return paramLast;
    }

    void getParamInfo(uint8_t paramNum, csParamInfo& info) override {
        if (paramNum == paramGradientOffset) {
            info.type = ParamType::UInt8;
            info.name = "Gradient offset";
            info.ptr = &gradientOffset;
            info.readOnly = false;
            info.disabled = false;
            return;
        }
        csRenderCircle::getParamInfo(paramNum, info);
    }

    void render(csRandGen& /*rand*/, uint16_t /*currTime*/) const override {
        if (disabled || !matrix) {
            return;
        }

        const csRect target = rect.intersect(matrix->getRect());
        if (target.empty()) {
            return;
        }

        const float cx = static_cast<float>(target.x) + static_cast<float>(target.width) * 0.5f;
        const float cy = static_cast<float>(target.y) + static_cast<float>(target.height) * 0.5f;
        const float radius = static_cast<float>(math::min(target.width, target.height)) * 0.5f;
        if (radius <= 0.0f) {
            return;
        }

        const float radiusSq = radius * radius;
        const float offsetNorm = static_cast<float>(gradientOffset) / 255.0f;
        const float startR = radius * offsetNorm; // inner radius where gradient starts (t=0)
        const float span = radius - startR;

        const tMatrixPixelsCoord endX = target.x + to_coord(target.width);
        const tMatrixPixelsCoord endY = target.y + to_coord(target.height);
        for (tMatrixPixelsCoord y = target.y; y < endY; ++y) {
            const float py = static_cast<float>(y) + 0.5f;
            const float dy = py - cy;
            const float dySq = dy * dy;
            for (tMatrixPixelsCoord x = target.x; x < endX; ++x) {
                const float px = static_cast<float>(x) + 0.5f;
                const float dx = px - cx;
                const float distSq = dx * dx + dySq;

                // Outside circle: just background.
                if (distSq > radiusSq) {
                    matrix->setPixel(x, y, backgroundColor);
                    continue;
                }

                const float dist = sqrtf(distSq);
                float t = 0.0f;
                if (span > 0.0f) {
                    // Clamp: t=0 inside startR; t=1 at radius.
                    if (dist > startR) {
                        t = math::max(0.0f, math::min(1.0f, (dist - startR) / span));
                    }
                }

                // Linear interpolation between color (center) and backgroundColor (edge).
                const uint8_t t8 = static_cast<uint8_t>(t * 255.0f + 0.5f);
                const csColorRGBA gradColor = lerp(color, backgroundColor, t8);

                matrix->setPixel(x, y, gradColor);
            }
        }
    }
};

// Snowfall effect: single snowflake falls down, accumulates at bottom, restarts at specified fill percentage.
class csRenderSnowfall : public csRenderDynamic {
public:
    static constexpr uint8_t base = csRenderMatrixBase::paramLast;
    static constexpr uint8_t paramSnowflake_WIP = base + 1;
    static constexpr uint8_t paramLast = paramSnowflake_WIP;
    static constexpr uint8_t compactSnowInterval = 10; // Call compactSnow every N snowfalls
    static constexpr uint8_t restartFillPercent = 80; // Restart when fill reaches this percentage (0-100)

    csColorRGBA color{255, 255, 255, 255};

    // State fields (not mutable, changed in recalc())
    tMatrixPixelsCoord currentX = 0;
    tMatrixPixelsCoord currentY = 0;
    bool hasActiveSnowflake = false;
    uint16_t filledPixelsCount = 0;
    uint16_t lastUpdateTime = 0;
    uint8_t snowfallCount = 0; // Counter for compactSnow calls
    bool lastDirectionWasLeft = false; // Alternating priority for moveDownSide directions
    csMatrixBoolean* bitmap = nullptr;
    uint16_t clearingIterations = 0; // Clearing mode counter: >0 means active, decreases to 0

    csRenderSnowfall() = default;

    ~csRenderSnowfall() {
        delete bitmap;
    }

    uint8_t getParamsCount() const override {
        return paramLast;
    }

    void getParamInfo(uint8_t paramNum, csParamInfo& info) override {
        csRenderDynamic::getParamInfo(paramNum, info);
        switch (paramNum) {
            case paramRenderRectAutosize:
                info.disabled = true;
                break;
            case paramColor:
                info.type = ParamType::Color;
                info.name = "Snowflake color";
                info.ptr = &color;
                info.readOnly = false;
                info.disabled = false;
                break;
            case paramSnowflake_WIP:
                // WIP
                info.disabled = true;
                break;
    }
    }

    void paramChanged(uint8_t paramNum) override {
        csRenderMatrixBase::paramChanged(paramNum);
        if (paramNum == paramRenderRect) {
            updateBitmap();
        }
    }

    void recalc(csRandGen& rand, uint16_t currTime) override {
        if (disabled) {
            return;
        }
        // Algorithm works independently of matrix, only needs rect and bitmap
        if (!bitmap || rect.width == 0 || rect.height == 0) {
            return;
        }

        const uint32_t totalPixels = static_cast<uint32_t>(rect.width) * static_cast<uint32_t>(rect.height);

        // Check for clearing mode activation at specified fill percentage
        if (clearingIterations == 0 && filledPixelsCount >= (totalPixels * restartFillPercent) / 100) {
            clearingIterations = rect.height;
            filledPixelsCount = 0;
        }

        // Calculate time step based on speed
        const uint16_t timeStep = static_cast<uint16_t>(50.0f / speed.to_float());
        if (timeStep == 0) {
            return;
        }

        // Check if it's time to update position
        const uint16_t timeDelta = currTime - lastUpdateTime;
        if (timeDelta < timeStep && hasActiveSnowflake) {
            return;
        }

        lastUpdateTime = currTime;

        // Create new snowflake if none active
        if (!hasActiveSnowflake) {
            // Random X position in top row (local coordinates: 0..width-1)
            currentX = to_coord(rand.rand(static_cast<uint8_t>(rect.width)));
            currentY = 0;
            hasActiveSnowflake = true;
            return;
        }

        // Try to move snowflake down
        const tMatrixPixelsCoord nextY = currentY + 1;

        // Common fix logic
        auto fixSnowflakeAtCurrent = [&]() {
            // Logic with outOfBoundsValue = true:
            // - If snowflake is inside bounds and position is empty: getPixel returns false,
            //   we set the pixel and increment counter.
            // - If snowflake is inside bounds and position already has snowflake: getPixel returns true,
            //   we do nothing.
            // - If snowflake is out of bounds: getPixel returns true (due to outOfBoundsValue = true),
            //   we do nothing, counter is not incremented.
            // No boundary check needed: getPixel with outOfBoundsValue = true already handles out-of-bounds.
            if (!bitmap->getPixel(to_size(currentX), to_size(currentY))) {
                bitmap->setPixel(to_size(currentX), to_size(currentY), true);
                ++filledPixelsCount;
            }

            hasActiveSnowflake = false;
            
            // Compact snow pile: make snowflakes settle down and to the left
            // Call compactSnow only once every N snowfalls
            ++snowfallCount;
            if (snowfallCount >= compactSnowInterval) {
                compactSnow();
                snowfallCount = 0;
            }

            // Handle clearing mode
            if (clearingIterations > 0) {
                shiftBitmapDown();
                --clearingIterations;
            }
        };

        // Check collision with existing snowflake or boundary
        // outOfBoundsValue = true means getPixel returns true for out-of-bounds
        if (bitmap->getPixel(to_size(currentX), to_size(nextY))) {
            fixSnowflakeAtCurrent();
            return;
        }

        // Move down
        currentY = nextY;
    }

    void render(csRandGen& /*rand*/, uint16_t /*currTime*/) const override {
        if (disabled || !matrix || !bitmap) {
            return;
        }

        const csRect target = rect.intersect(matrix->getRect());
        if (target.empty()) {
            return;
        }

        const tMatrixPixelsCoord endX = target.x + to_coord(target.width);
        const tMatrixPixelsCoord endY = target.y + to_coord(target.height);

        // Draw fixed snowflakes from bitmap and clear background
        for (tMatrixPixelsCoord y = target.y; y < endY; ++y) {
            for (tMatrixPixelsCoord x = target.x; x < endX; ++x) {
                const tMatrixPixelsCoord localX = x - rect.x;
                const tMatrixPixelsCoord localY = y - rect.y;
                if (bitmap->getPixel(to_size(localX), to_size(localY))) {
                    matrix->setPixel(x, y, color);
                }
            }
        }

        // Draw current falling snowflake
        // Convert local coordinates to global coordinates
        if (hasActiveSnowflake) {
            const tMatrixPixelsCoord globalX = rect.x + currentX;
            const tMatrixPixelsCoord globalY = rect.y + currentY;
            // Check if snowflake is within target area (intersection of rect and matrix)
            if (globalX >= target.x && globalX < endX && 
                globalY >= target.y && globalY < endY) {
                matrix->setPixel(globalX, globalY, color);
            }
        }
    }

private:
    void updateBitmap() {
        delete bitmap;
        bitmap = nullptr;

        if (rect.empty()) {
            return;
        }

        bitmap = new csMatrixBoolean(rect.width, rect.height, true);
        // Reset state when bitmap is recreated
        hasActiveSnowflake = false;
        filledPixelsCount = 0;
        snowfallCount = 0;
        clearingIterations = 0;
        lastDirectionWasLeft = false;
        lastUpdateTime = 0;
    }

    // Try to move snowflake down. Returns true if moved.
    bool moveDown(tMatrixPixelsSize x, tMatrixPixelsSize y) {
        if (!bitmap->getPixel(x, y + 1)) {
            // Move down
            bitmap->setPixel(x, y, false);
            bitmap->setPixel(x, y + 1, true);
            return true;
        }
        return false;
    }

    // Try to move snowflake down-left or down-right. Returns true if moved.
    // direction: -1 for left, +1 for right
    // Updates lastDirectionWasLeft on successful movement
    bool moveDownSide(tMatrixPixelsSize x, tMatrixPixelsSize y, int direction, bool& lastDirectionWasLeft) {
        const int newXInt = static_cast<int>(x) + direction;
        const tMatrixPixelsSize newX = static_cast<tMatrixPixelsSize>(newXInt);
        if (!bitmap->getPixel(newX, y + 1)) {
            // Move down-side
            bitmap->setPixel(x, y, false);
            bitmap->setPixel(newX, y + 1, true);
            lastDirectionWasLeft = !lastDirectionWasLeft;
            return true;
        }
        return false;
    }

    // Compact snow pile: make snowflakes settle down and to the left
    void compactSnow() {
        if (!bitmap) {
            return;
        }

        const tMatrixPixelsSize w = bitmap->width();
        const tMatrixPixelsSize h = bitmap->height();

        // Process from bottom to top, right to left
        // This ensures snowflakes fall correctly without glitches
        for (tMatrixPixelsSize y = h; y > 0; --y) {
            const tMatrixPixelsSize py = y - 1;
            for (tMatrixPixelsSize x = w; x > 0; --x) {
                const tMatrixPixelsSize px = x - 1;

                // Check if there's a snowflake at this position
                if (!bitmap->getPixel(px, py)) {
                    continue;
                }

                // Try to move down first
                if (moveDown(px, py)) {
                    continue;
                }

                // Try diagonal movement with alternating priority
                // Alternate between left (-1) and right (+1) directions
                if (lastDirectionWasLeft) {
                    // Last was left, try right first this time
                    moveDownSide(px, py, +1, lastDirectionWasLeft) || moveDownSide(px, py, -1, lastDirectionWasLeft);
                } else {
                    // Last was right, try left first this time
                    moveDownSide(px, py, -1, lastDirectionWasLeft) || moveDownSide(px, py, +1, lastDirectionWasLeft);
                }
            }
        }
    }

    // Shift bitmap down: remove bottom row, shift all rows down, clear top row
    void shiftBitmapDown() {
        if (!bitmap) {
            return;
        }

        const tMatrixPixelsSize w = bitmap->width();
        const tMatrixPixelsSize h = bitmap->height();
        if (h == 0) {
            return;
        }

        // Shift all rows down (from bottom to top to avoid overwriting)
        for (tMatrixPixelsSize y = h - 1; y > 0; --y) {
            for (tMatrixPixelsSize x = 0; x < w; ++x) {
                const bool value = bitmap->getPixel(x, y - 1);
                bitmap->setPixel(x, y, value);
            }
        }

        // Clear top row
        for (tMatrixPixelsSize x = 0; x < w; ++x) {
            bitmap->setPixel(x, 0, false);
        }
    }

};

// Effect: clear matrix to transparent black.
class csRenderClear : public csRenderMatrixBase {
public:
    void render(csRandGen& /*rand*/, uint16_t /*currTime*/) const override {
        if (disabled || !matrix) {
            return;
        }
        matrix->clear();
    }
};

// Effect: fill rectangular area with solid color.
class csRenderFill : public csRenderMatrixBase {
public:
    csColorRGBA color{255, 255, 255, 255};

    void getParamInfo(uint8_t paramNum, csParamInfo& info) override {
        csRenderMatrixBase::getParamInfo(paramNum, info);
        switch (paramNum) {
            case paramColor:
                info.type = ParamType::Color;
                info.name = "Fill color";
                info.ptr = &color;
                info.readOnly = false;
                info.disabled = false;
                break;
        }
    }

    void render(csRandGen& /*rand*/, uint16_t /*currTime*/) const override {
        if (disabled || !matrix) {
            return;
        }
        const csRect target = rect.intersect(matrix->getRect());
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

// Effect: container that holds and calls up to 5 nested effects.
// Container does NOT create or destroy nested effects - only stores pointers.
class csRenderContainer : public csRenderMatrixBase {
public:
    static constexpr uint8_t maxEffects = 5;
    static constexpr uint8_t base = csRenderMatrixBase::paramLast;
    static constexpr uint8_t paramEffect1 = base + 1;
    static constexpr uint8_t paramEffect2 = base + 2;
    static constexpr uint8_t paramEffect3 = base + 3;
    static constexpr uint8_t paramEffect4 = base + 4;
    static constexpr uint8_t paramEffect5 = base + 5;
    static constexpr uint8_t paramLast = paramEffect5;

    // Array of pointers to nested effects (nullptr means slot is empty).
    // Container does NOT manage memory - effects must be created/destroyed externally.
    csEffectBase* effects[maxEffects] = {};

    uint8_t getParamsCount() const override {
        return paramLast;
    }

    void getParamInfo(uint8_t paramNum, csParamInfo& info) override {
        csRenderMatrixBase::getParamInfo(paramNum, info);
        switch (paramNum) {
            case paramRenderRect:
                info.disabled = true;
                break;
            case paramRenderRectAutosize:
                info.disabled = true;
                break;
            case paramEffect1:
                info.type = ParamType::Effect;
                info.name = "Effect 1";
                info.ptr = &effects[0];
                info.readOnly = false;
                info.disabled = false;
                break;
            case paramEffect2:
                info.type = ParamType::Effect;
                info.name = "Effect 2";
                info.ptr = &effects[1];
                info.readOnly = false;
                info.disabled = false;
                break;
            case paramEffect3:
                info.type = ParamType::Effect;
                info.name = "Effect 3";
                info.ptr = &effects[2];
                info.readOnly = false;
                info.disabled = false;
                break;
            case paramEffect4:
                info.type = ParamType::Effect;
                info.name = "Effect 4";
                info.ptr = &effects[3];
                info.readOnly = false;
                info.disabled = false;
                break;
            case paramEffect5:
                info.type = ParamType::Effect;
                info.name = "Effect 5";
                info.ptr = &effects[4];
                info.readOnly = false;
                info.disabled = false;
                break;
        }
    }

    void paramChanged(uint8_t paramNum) override {
        // Ignore paramRenderRect and paramRenderRectAutosize - they are disabled
        if (paramNum == paramRenderRect || paramNum == paramRenderRectAutosize) {
            return;
        }
        // Call base implementation for other parameters
        csRenderMatrixBase::paramChanged(paramNum);
    }

    void recalc(csRandGen& rand, uint16_t currTime) override {
        if (disabled) {
            return;
        }
        for (uint8_t i = 0; i < maxEffects; ++i) {
            if (effects[i] != nullptr) {
                effects[i]->recalc(rand, currTime);
            }
        }
    }

    void render(csRandGen& rand, uint16_t currTime) const override {
        if (disabled) {
            return;
        }
        for (uint8_t i = 0; i < maxEffects; ++i) {
            if (effects[i] != nullptr) {
                effects[i]->render(rand, currTime);
            }
        }
    }
};

// Effect: display 4 digits clock using glyph renderer.
// Time value is passed via 'time' parameter (int32_t).
// Extracts last 4 decimal digits from time parameter.
// Does not use system time - time must be set externally.
class csRenderClock : public csRenderMatrixBase {
public:
    static constexpr uint8_t base = csRenderMatrixBase::paramLast;
    static constexpr uint8_t paramTime = base + 1;
    static constexpr uint8_t paramLast = paramTime;

    // Time parameter (uint32_t): stores time value for clock display.
    //
    // Examples:
    //   - time = 1234 → extracts [1, 2, 3, 4] → displays "1234"
    //   - time = 5    → extracts [0, 0, 0, 5] → displays "0005"
    //
    // Note:
    //   This effect does NOT read system time. The time value must be set
    //   externally by the application.
    uint32_t time = 0;

    // Single glyph renderer reused for all 4 digits
    // Mutable because render() modifies it but doesn't change logical state
    static constexpr uint8_t digitCount = 4;
    mutable csRenderGlyph glyph;

    csRenderClock() {
        // Initialize glyph with font
        const csFontBase& font = font3x5Digits();
        glyph.setFont(font);
        glyph.renderRectAutosize = false;
    }

    void paramChanged(uint8_t paramNum) override {
        csRenderMatrixBase::paramChanged(paramNum);
        if (paramNum == paramMatrixDest) {
            // Update glyph matrix when matrix destination changes
            if (matrix) {
                glyph.setMatrix(matrix);
            }
        }
    }

    uint8_t getParamsCount() const override {
        return paramLast;
    }

    void getParamInfo(uint8_t paramNum, csParamInfo& info) override {
        csRenderMatrixBase::getParamInfo(paramNum, info);
        switch (paramNum) {
            case paramRenderRectAutosize:
                info.disabled = true;
                break;
            case paramTime:
                // Time parameter: uint32_t value containing time data.
                // Extracts last 4 decimal digits (rightmost) from the value.
                // Digits are extracted right-to-left and displayed left-to-right.
                // Examples: time=1234 → "4321", time=567 → "7650", time=99 → "9900"
                info.type = ParamType::UInt32;
                info.name = "Time";
                info.ptr = &time;
                info.readOnly = false;
                info.disabled = false;
                break;
        }
    }

    void render(csRandGen& rand, uint16_t currTime) const override {
        if (disabled || !matrix) {
            return;
        }

        // Extract last 4 decimal digits from time parameter
        // Extract from left to right (most significant first)
        uint8_t digits[digitCount];

        uint32_t divisor = 1;
        // cycle [3..0]
        for (int i = digitCount - 1; i >= 0; --i) {
            // digits[3] = (1234 / 1) → 1234 % 10 → 4
            // digits[2] = (1234 / 10) → 123 % 10 → 3
            // digits[1] = (1234 / 100) → 12 % 10 → 2
            // digits[0] = (1234 / 1000) → 1 % 10 → 1
            digits[i] = static_cast<uint8_t>( (time / divisor) % 10 );
            // 1 → 10 → 100 → 1000
            divisor = divisor * 10;
        }

        // Get font dimensions
        const csFontBase* font = glyph.font;
        if (!font) {
            return;
        }

        const tMatrixPixelsSize fontWidth = to_size(font->width());
        const tMatrixPixelsSize fontHeight = to_size(font->height());

        // Calculate positions for 4 digits horizontally
        // Position them within rect bounds
        const tMatrixPixelsCoord startX = rect.x;
        const tMatrixPixelsCoord startY = rect.y;
        const tMatrixPixelsSize spacing = 1; // 1 pixel spacing between digits

        // Render each digit by updating and rendering the single glyph
        for (uint8_t i = 0; i < digitCount; ++i) {
            glyph.symbolIndex = digits[i];
            glyph.rect = csRect{
                startX + to_coord((fontWidth + spacing) * i),
                startY,
                fontWidth,
                fontHeight
            };
            glyph.render(rand, currTime);
        }
    }
};

// Effect: copy pixels from source matrix to destination matrix with blending.
class csRenderMatrixCopy : public csRenderMatrixBase {
public:
    static constexpr uint8_t base = csRenderMatrixBase::paramLast;
    static constexpr uint8_t paramMatrixSource = base + 1;
    static constexpr uint8_t paramLast = paramMatrixSource;

    // Source matrix pointer (nullptr means no source).
    csMatrixPixels* matrixSource = nullptr;

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
        }
    }

    void paramChanged(uint8_t paramNum) override {
        csRenderMatrixBase::paramChanged(paramNum);
        if (paramNum == paramMatrixSource) {
            if (renderRectAutosize && matrixSource) {
                rect = matrixSource->getRect();
            }
        }
    }

    void render(csRandGen& /*rand*/, uint16_t /*currTime*/) const override {
        if (disabled || !matrix || !matrixSource) {
            return;
        }

        const csRect target = rect.intersect(matrix->getRect());
        if (target.empty()) {
            return;
        }

        const tMatrixPixelsCoord endX = target.x + to_coord(target.width);
        const tMatrixPixelsCoord endY = target.y + to_coord(target.height);
        for (tMatrixPixelsCoord y = target.y; y < endY; ++y) {
            for (tMatrixPixelsCoord x = target.x; x < endX; ++x) {
                // Calculate local coordinates in source matrix (relative to rect.x and rect.y)
                const tMatrixPixelsCoord srcX = x - rect.x;
                const tMatrixPixelsCoord srcY = y - rect.y;
                const csColorRGBA srcColor = matrixSource->getPixel(srcX, srcY);
                matrix->setPixel(x, y, srcColor);
            }
        }
    }
};

} // namespace amp
