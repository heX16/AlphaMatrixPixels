#pragma once

#include <stdint.h>
#include <math.h>

#include "color_rgba.hpp"
#include "matrix_pixels.hpp"
#include "rect.hpp"
#include "math.hpp"
#include "matrix_render.hpp"
#include "font3x5.h"
#include "bit_matrix.hpp"




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
        if (!matrix) {
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
        if (!matrix) {
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
        if (!matrix) {
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
        if (!matrix || !renderRectAutosize || !font) {
            return;
        }
        const csRect mrect = matrix->getRect();
        rect = csRect{
            mrect.x,
            mrect.y,
            to_size(font->width()),
            to_size(font->height())
        };
    }

    void render(csRandGen& /*rand*/, uint16_t /*currTime*/) const override {
        if (!matrix || !font) {
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
        if (!matrix) {
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
        if (!matrix) {
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
        if (!matrix) {
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

// Snowfall effect: single snowflake falls down, accumulates at bottom, restarts at 50% fill.
class csRenderSnowfall : public csRenderDynamic {
public:
    static constexpr uint8_t base = csRenderMatrixBase::paramLast;
    static constexpr uint8_t paramSnowflakeColor = base + 1;
    static constexpr uint8_t paramLast = paramSnowflakeColor;

    csColorRGBA snowflakeColor{255, 255, 255, 255};
    csColorRGBA backgroundColor{0, 0, 0, 0};

    // State fields (not mutable, changed in recalc())
    tMatrixPixelsCoord currentX = 0;
    tMatrixPixelsCoord currentY = 0;
    bool hasActiveSnowflake = false;
    uint16_t filledPixelsCount = 0;
    uint16_t lastUpdateTime = 0;
    csBitMatrix* bitmap = nullptr;

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
            case paramSnowflakeColor:
                info.type = ParamType::Color;
                info.name = "Snowflake color";
                info.ptr = &snowflakeColor;
                info.readOnly = false;
                info.disabled = false;
                break;
            case paramColorBackground:
                info.ptr = &backgroundColor;
                info.disabled = false;
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
        if (!matrix) {
            return;
        }

        if (rect.empty() || !bitmap) {
            return;
        }

        const uint32_t totalPixels = static_cast<uint32_t>(rect.width) * static_cast<uint32_t>(rect.height);

        // Check for restart at 50% fill
        if (filledPixelsCount >= totalPixels / 2) {
            bitmap->clear();
            filledPixelsCount = 0;
            hasActiveSnowflake = false;
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
            // Random X position in top row
            currentX = rect.x + to_coord(rand.rand(static_cast<uint8_t>(rect.width)));
            currentY = rect.y;
            hasActiveSnowflake = true;
            return;
        }

        // Try to move snowflake down
        const tMatrixPixelsCoord nextY = currentY + 1;

        // Common fix logic
        auto fixSnowflakeAtCurrent = [&]() {
            const tMatrixPixelsCoord localX = currentX - rect.x;
            const tMatrixPixelsCoord localY = currentY - rect.y;

            if (localX >= 0 && localX < to_coord(rect.width) &&
                localY >= 0 && localY < to_coord(rect.height)) {

                if (!bitmap->getPixel(to_size(localX), to_size(localY))) {
                    bitmap->setPixel(to_size(localX), to_size(localY), true);
                    ++filledPixelsCount;
                }
            }

            hasActiveSnowflake = false;
        };

        // Reached bottom
        if (nextY >= rect.y + to_coord(rect.height)) {
            fixSnowflakeAtCurrent();
            return;
        }

        // Collision with existing snowflake
        const tMatrixPixelsCoord localX = currentX - rect.x;
        const tMatrixPixelsCoord localY = nextY - rect.y;

        if (localX >= 0 && localX < to_coord(rect.width) &&
            localY >= 0 && localY < to_coord(rect.height)) {

            if (bitmap->getPixel(to_size(localX), to_size(localY))) {
                fixSnowflakeAtCurrent();
                return;
            }
        }

        // Move down
        currentY = nextY;
    }

    void render(csRandGen& /*rand*/, uint16_t /*currTime*/) const override {
        if (!matrix || !bitmap) {
            return;
        }

        const csRect target = rect.intersect(matrix->getRect());
        if (target.empty()) {
            return;
        }

        const tMatrixPixelsCoord endX = target.x + to_coord(target.width);
        const tMatrixPixelsCoord endY = target.y + to_coord(target.height);

        // Clear background
        for (tMatrixPixelsCoord y = target.y; y < endY; ++y) {
            for (tMatrixPixelsCoord x = target.x; x < endX; ++x) {
                matrix->setPixel(x, y, backgroundColor);
            }
        }

        // Draw fixed snowflakes from bitmap
        for (tMatrixPixelsCoord y = target.y; y < endY; ++y) {
            for (tMatrixPixelsCoord x = target.x; x < endX; ++x) {
                const tMatrixPixelsCoord localX = x - rect.x;
                const tMatrixPixelsCoord localY = y - rect.y;
                if (bitmap->getPixel(to_size(localX), to_size(localY))) {
                    matrix->setPixel(x, y, snowflakeColor);
                }
            }
        }

        // Draw current falling snowflake
        if (hasActiveSnowflake) {
            if (currentX >= target.x && currentX < endX && 
                currentY >= target.y && currentY < endY) {
                matrix->setPixel(currentX, currentY, snowflakeColor);
            }
        }
    }

private:
    void updateBitmap() {
        delete bitmap;
        bitmap = nullptr;

        if (rect.empty() || rect.width == 0 || rect.height == 0) {
            return;
        }

        bitmap = new csBitMatrix(rect.width, rect.height);
        // Reset state when bitmap is recreated
        hasActiveSnowflake = false;
        filledPixelsCount = 0;
    }
};

} // namespace amp
