#pragma once

#include <stdint.h>
#include <math.h>

#include "color_rgba.hpp"
#include "matrix_pixels.hpp"
#include "rect.hpp"
#include "math.hpp"
#include "fixed_point.hpp"
#include "matrix_render.hpp"
#include "fonts.h"
#include "matrix_boolean.hpp"


using namespace amp::math;

namespace amp {

// Base class for gradient effects with scale property.
class csRenderDynamic : public csRenderMatrixBase {
public:
    // Scale property for effect size (FP16, default 1.0).
    // Increasing value (scale > 1.0) → larger scale → effect stretches → fewer details visible (like "zooming out");
    // decreasing value (scale < 1.0) → smaller scale → effect compresses → more details visible (like "zooming in").
    csFP16 scale{1.0f};
    // Speed property for effect animation speed (FP16, default 1.0).
    csFP16 speed{1.0f};

    // NOTE: uint8_t getPropsCount() - count dont changed

    void getPropInfo(uint8_t propNum, csPropInfo& info) override {
        csRenderMatrixBase::getPropInfo(propNum, info);
        if (propNum == propScale) {
            info.ptr = &scale;
            info.disabled = false;
            return;
        }
        if (propNum == propSpeed) {
            info.ptr = &speed;
            info.disabled = false;
            return;
        }
    }
};

// Simple animated RGB gradient (float).
class csRenderGradientWaves : public csRenderDynamic {
public:
    void render(csRandGen& /*rand*/, tTime currTime) const override {
        if (disabled || !matrix) {
            return;
        }
        const float t = static_cast<float>(currTime) * 0.001f * speed.to_float();

        auto wave = [](float v) -> uint8_t {
            return static_cast<uint8_t>((sin(v) * 0.5f + 0.5f) * 255.0f);
        };

        const csRect target = rectDest.intersect(matrix->getRect());
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
    static uint8_t wave_fp(csFP32 phase) noexcept {
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

    void render(csRandGen& /*rand*/, tTime currTime) const override {
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
        const csRect target = rectDest.intersect(matrix->getRect());
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
    void render(csRandGen& /*rand*/, tTime currTime) const override {
        if (disabled || !matrix) {
            return;
        }
        const float t = static_cast<float>(currTime) * 0.0025f * speed.to_float();
        const csRect target = rectDest.intersect(matrix->getRect());
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
    static constexpr uint8_t base = csRenderMatrixBase::propLast;
    static constexpr uint8_t propSymbolIndex = base+1;
    static constexpr uint8_t propFontWidth = base+2;
    static constexpr uint8_t propFontHeight = base+3;
    static constexpr uint8_t propLast = propFontHeight;

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

    // Class family identifier
    static constexpr PropType ClassFamilyId = PropType::EffectGlyph;

    // Override to return glyph renderer family
    PropType getClassFamily() const override {
        return ClassFamilyId;
    }

    // Override to check for glyph renderer family
    void* queryClassFamily(PropType familyId) override {
        if (familyId == ClassFamilyId) {
            return this;
        }
        // Check base class (csRenderMatrixBase) family
        return csRenderMatrixBase::queryClassFamily(familyId);
    }

    uint8_t getPropsCount() const override {
        return propLast;
    }

    void getPropInfo(uint8_t propNum, csPropInfo& info) override {
        csRenderMatrixBase::getPropInfo(propNum, info);
        switch (propNum) {
            case propRenderRectAutosize:
                info.disabled = true;
                break;
            case propSymbolIndex:
                info.type = PropType::UInt8;
                info.name = "Glyph index";
                info.ptr = &symbolIndex;
                info.readOnly = false;
                info.disabled = false;
                break;
            case propColor:
                info.type = PropType::Color;
                info.name = "Symbol color";
                info.ptr = &color;
                info.readOnly = false;
                info.disabled = false;
                break;
            case propColorBackground:
                info.ptr = &backgroundColor;
                info.disabled = false;
                break;
            case propFontWidth:
                info.type = PropType::UInt16;
                info.name = "Font width";
                info.ptr = &fontWidth;
                info.readOnly = true;
                info.disabled = false;
                break;
            case propFontHeight:
                info.type = PropType::UInt16;
                info.name = "Font height";
                info.ptr = &fontHeight;
                info.readOnly = true;
                info.disabled = false;
                break;
        }
    }

    void propChanged(uint8_t propNum) override {
        csRenderMatrixBase::propChanged(propNum);
        if (!font) {
            return;
        }
        if (propNum == propSymbolIndex && symbolIndex >= font->count()) {
            symbolIndex = static_cast<uint8_t>((font->count() == 0) ? 0 : (font->count() - 1));
        }
    }

    void updateRenderRect() override {
        if (!renderRectAutosize || !font) {
            return;
        }

        rectDest = csRect{
            rectDest.x,
            rectDest.y,
            to_size(font->width()),
            to_size(font->height())
        };
    }

    void render(csRandGen& /*rand*/, uint16_t /*currTime*/) const override {
        if (disabled || !matrix || !font) {
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
                matrix->setPixel(x, y, backgroundColor);
            }
        }

        const tMatrixPixelsSize glyphWidth = math::min(rectDest.width, to_size(font->width()));
        const tMatrixPixelsSize glyphHeight = math::min(rectDest.height, to_size(font->height()));
        if (glyphWidth == 0 || glyphHeight == 0) {
            return;
        }

        const tMatrixPixelsCoord offsetX = rectDest.x + to_coord((rectDest.width - glyphWidth) / 2);
        const tMatrixPixelsCoord offsetY = rectDest.y + to_coord((rectDest.height - glyphHeight) / 2);

        if (symbolIndex >= font->count()) {
            return;
        }
            
        for (tMatrixPixelsSize row = 0; row < glyphHeight; ++row) {
            const uint32_t glyphRow = font->getRowBits(static_cast<uint16_t>(symbolIndex), static_cast<uint16_t>(row));
            for (tMatrixPixelsSize col = 0; col < glyphWidth; ++col) {
                if (csFontBase::getColBit(glyphRow, static_cast<uint16_t>(col))) {
                    const tMatrixPixelsCoord px = offsetX + to_coord(col);
                    const tMatrixPixelsCoord py = offsetY + to_coord(row);
                    matrix->setPixel(px, py, color);
                } 
            }
        }
    }
};

// Effect: draw a single digit glyph
class csRenderDigitalClockDigit : public csRenderGlyph {
public:
    csRenderDigitalClockDigit() {
        setFont(getStaticFontTemplate<csFont4x7DigitalClock>());
    }

    void render(csRandGen& /*rand*/, uint16_t /*currTime*/) const override {
        if (disabled || !matrix || !font) {
            return;
        }

        const csRect target = rectDest.intersect(matrix->getRect());
        if (target.empty()) {
            return;
        }

        const tMatrixPixelsSize glyphWidth = math::min(rectDest.width, to_size(font->width()));
        const tMatrixPixelsSize glyphHeight = math::min(rectDest.height, to_size(font->height()));
        if (glyphWidth == 0 || glyphHeight == 0) {
            return;
        }

        const tMatrixPixelsCoord offsetX = rectDest.x + to_coord((rectDest.width - glyphWidth) / 2);
        const tMatrixPixelsCoord offsetY = rectDest.y + to_coord((rectDest.height - glyphHeight) / 2);

        if (symbolIndex >= font->count()) {
            return;
        }
            
        for (tMatrixPixelsSize row = 0; row < glyphHeight; ++row) {
            const uint32_t glyphRow = font->getRowBits(static_cast<uint16_t>(symbolIndex), static_cast<uint16_t>(row));
            // get the row bits for all segments (its a number "8")
            const uint32_t glyphRowAllSeg = font->getRowBits(8, static_cast<uint16_t>(row));

            for (tMatrixPixelsSize col = 0; col < glyphWidth; ++col) {
                const tMatrixPixelsCoord px = offsetX + to_coord(col);
                const tMatrixPixelsCoord py = offsetY + to_coord(row);

                //TODO:!!!
                /*if (csFontBase::getColBit(glyphRowAllSeg, static_cast<uint16_t>(col))) {
                    matrix->setPixel(px, py, csColorRGBA(255, 0,0));
                }*/

                if (csFontBase::getColBit(glyphRow, static_cast<uint16_t>(col))) {
                    matrix->setPixel(px, py, color);
                } else {
                    if (csFontBase::getColBit(glyphRowAllSeg, static_cast<uint16_t>(col))) {
                        matrix->setPixel(px, py, backgroundColor);
                    }
                }
            }
        }
    }
};

// Effect: draw a filled circle inscribed into rect.
class csRenderCircle : public csRenderMatrixBase {
public:
    static constexpr uint8_t base = csRenderMatrixBase::propLast;
    static constexpr uint8_t propSmoothEdges = base + 1;
    static constexpr uint8_t propLast = propSmoothEdges;

    csColorRGBA color{255, 255, 255, 255};
    csColorRGBA backgroundColor{0, 0, 0, 0};
    bool smoothEdges = true;

    uint8_t getPropsCount() const override {
        return propLast;
    }

    void getPropInfo(uint8_t propNum, csPropInfo& info) override {
        switch (propNum) {
            case propColor:
                info.type = PropType::Color;
                info.name = "Circle color";
                info.ptr = &color;
                info.readOnly = false;
                info.disabled = false;
                break;
            case propColorBackground:
                info.ptr = &backgroundColor;
                info.disabled = false;
                break;
            case propSmoothEdges:
                info.type = PropType::Bool;
                info.name = "Smooth edges";
                info.ptr = &smoothEdges;
                info.readOnly = false;
                info.disabled = false;
                break;
            default:
                csRenderMatrixBase::getPropInfo(propNum, info);
                break;
        }
    }

    void render(csRandGen& /*rand*/, uint16_t /*currTime*/) const override {
        if (disabled || !matrix) {
            return;
        }

        const csRect target = rectDest.intersect(matrix->getRect());
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
                    const csColorRGBA c = (distSq <= radiusSq) ? color : backgroundColor;
                    matrix->setPixel(x, y, c);
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

        const csRect target = rectDest.intersect(matrix->getRect());
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
    static constexpr uint8_t base = csRenderMatrixBase::propLast;
    static constexpr uint8_t propGradientOffset = base+1;
    static constexpr uint8_t propLast = propGradientOffset;

    // Offset of gradient start from center, normalized: 0..255 -> 0..1 of radius.
    uint8_t gradientOffset = 0;

    uint8_t getPropsCount() const override {
        return propLast;
    }

    void getPropInfo(uint8_t propNum, csPropInfo& info) override {
        if (propNum == propGradientOffset) {
            info.type = PropType::UInt8;
            info.name = "Gradient offset";
            info.ptr = &gradientOffset;
            info.readOnly = false;
            info.disabled = false;
            return;
        }
        csRenderCircle::getPropInfo(propNum, info);
    }

    void render(csRandGen& /*rand*/, uint16_t /*currTime*/) const override {
        if (disabled || !matrix) {
            return;
        }

        const csRect target = rectDest.intersect(matrix->getRect());
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
private:
    static constexpr tMatrixPixelsCoord cSpawnFlagForceInit = -1; // special flag for instant init in `recalc`

public:
    static constexpr uint8_t base = csRenderMatrixBase::propLast;
    static constexpr uint8_t propCount = base + 1;
    static constexpr uint8_t propRestartFillPercent = base + 2;
    static constexpr uint8_t propSmoothMovement = base + 3;
    static constexpr uint8_t propLast = propSmoothMovement;
    
    static constexpr uint8_t compactSnowInterval = 10; // Call compactSnow every N snowfalls

    // Snowflake array configuration
    static constexpr int8_t cSpawnDelayMin = -5; // Minimum spawn delay value (negative)
    static constexpr int8_t cSpawnDelayMax = -1; // Maximum spawn delay value (negative, must be less than 0)

    // Snowflake structure
    struct Snowflake {
        csFP16 x{0.0f};
        csFP16 y{0.0f};
    };

    csColorRGBA color{255, 255, 255, 255};
    uint16_t count = 4; // Number of snowflakes (dynamic)
    uint8_t restartFillPercent = 80; // Restart when fill reaches this percentage (0-100)
    bool smoothMovement = true; // Enable sub-pixel smooth movement (default: enabled)

    // State fields (not mutable, changed in recalc())
    Snowflake* snowflakes = nullptr;
    uint16_t snowflakesAllocatedCount = 0; // Track allocated array size
    uint16_t filledPixelsCount = 0;
    uint16_t lastUpdateTime = 0;
    uint8_t snowfallCount = 0; // Counter for compactSnow calls
    bool lastDirectionWasLeft = false; // Alternating priority for moveDownSide directions
    csMatrixBoolean* bitmap = nullptr;
    uint16_t clearingIterations = 0; // Clearing mode counter: >0 means active, decreases to 0

    csRenderSnowfall() {
        resizeOrInitSnowflakesArray();
    }

    ~csRenderSnowfall() override {
        delete[] snowflakes;
        delete bitmap;
    }

    uint8_t getPropsCount() const override {
        return propLast;
    }

    void getPropInfo(uint8_t propNum, csPropInfo& info) override {
        csRenderDynamic::getPropInfo(propNum, info);
        switch (propNum) {
            case propRenderRectAutosize:
                info.disabled = true;
                break;
            case propCount:
                info.type = PropType::UInt16;
                info.name = "Snowflake count";
                info.ptr = &count;
                info.readOnly = false;
                info.disabled = false;
                break;
            case propColor:
                info.type = PropType::Color;
                info.name = "Snowflake color";
                info.ptr = &color;
                info.readOnly = false;
                info.disabled = false;
                break;
            case propRestartFillPercent:
                info.type = PropType::UInt8;
                info.name = "Restart fill percent";
                info.ptr = &restartFillPercent;
                info.readOnly = false;
                info.disabled = false;
                break;
            case propSmoothMovement:
                info.type = PropType::Bool;
                info.name = "Smooth movement";
                info.ptr = &smoothMovement;
                info.readOnly = false;
                info.disabled = false;
                break;
    }
    }

    void propChanged(uint8_t propNum) override {
        csRenderMatrixBase::propChanged(propNum);
        // After base class updates rectDest for propMatrixDest, we need to update bitmap
        if (propNum == propMatrixDest || propNum == propRectDest) {
            updateBitmap();
        } else if (propNum == propCount) {
            resizeOrInitSnowflakesArray();
        }
    }

    // Initialize one snowflake with random values
    void randOneSnowflake(Snowflake& snowflake, csRandGen& rand) {
        if (rectDest.width == 0) {
            return;
        }
        
        // Random X position in top row (local coordinates: 0..width-1)
        snowflake.x = csFP16::from_int(to_coord(rand.rand(static_cast<uint8_t>(rectDest.width))));
        // Random negative y value for spawn delay (between cSpawnDelayMin and cSpawnDelayMax)
        snowflake.y = csFP16::from_int(to_coord(cSpawnDelayMin + 
            rand.randRange(0, cSpawnDelayMax - cSpawnDelayMin)));
    }
    
    void recalc(csRandGen& rand, tTime currTime) override {
        if (disabled) {
            return;
        }

        // Algorithm works independently of matrix, only needs rectDest and bitmap
        if (!bitmap || rectDest.width == 0 || rectDest.height == 0) {
            return;
        }

        const uint32_t totalPixels = static_cast<uint32_t>(rectDest.width) * static_cast<uint32_t>(rectDest.height);

        // Check for clearing mode activation at specified fill percentage
        if (clearingIterations == 0 && filledPixelsCount >= (totalPixels * restartFillPercent) / 100) {
            clearingIterations = rectDest.height;
            filledPixelsCount = 0;
        }

        // Calculate time step based on speed
        const uint16_t timeStep = static_cast<uint16_t>(50.0f / speed.to_float());
        if (timeStep == 0) {
            return;
        }

        // Check if it's time to update position
        const uint16_t timeDelta = currTime - lastUpdateTime;
        if (timeDelta < timeStep) {
            return;
        }

        lastUpdateTime = currTime;

        // Process all snowflakes in the array
        if (!snowflakes || count == 0) {
            return; // Early exit if no array
        }
        for (uint16_t i = 0; i < count; ++i) {
            auto& snowflake = snowflakes[i];
            // Handle spawn delay phase: snowflake is moving toward visible area
            if (snowflake.y < csFP16::from_int(0)) {
                snowflake.y = snowflake.y + csFP16::from_int(1);
                continue;
            }
            if (snowflake.x == csFP16::from_int(cSpawnFlagForceInit)) {
                randOneSnowflake(snowflake, rand);
                continue;
            }

            // Normal falling logic for visible snowflakes (y >= 0)
            // Calculate movement delta based on speed (slower movement with sub-pixel precision)
            const csFP16 moveDelta = speed * csFP16::float_const(0.1f);
            const csFP16 nextY = snowflake.y + moveDelta;

            // Common fix logic (takes snowflake reference)
            auto fixSnowflakeAtCurrent = [&](Snowflake& flake) {
                // Logic with outOfBoundsValue = true:
                // - If snowflake is inside bounds and position is empty: getPixel returns false,
                //   we set the pixel and increment counter.
                // - If snowflake is inside bounds and position already has snowflake: getPixel returns true,
                //   we do nothing.
                // - If snowflake is out of bounds: getPixel returns true (due to outOfBoundsValue = true),
                //   we do nothing, counter is not incremented.
                // No boundary check needed: getPixel with outOfBoundsValue = true already handles out-of-bounds.
                // Convert fixed-point coordinates to integer for bitmap operations
                const tMatrixPixelsSize flakeX = to_size(static_cast<tMatrixPixelsCoord>(flake.x.round_int()));
                const tMatrixPixelsSize flakeY = to_size(static_cast<tMatrixPixelsCoord>(flake.y.round_int()));
                if (!bitmap->getPixel(flakeX, flakeY)) {
                    bitmap->setPixel(flakeX, flakeY, true);
                    ++filledPixelsCount;
                }

                randOneSnowflake(flake, rand);

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
            // Convert fixed-point coordinates to integer for bitmap collision check
            const tMatrixPixelsSize snowflakeX = to_size(static_cast<tMatrixPixelsCoord>(snowflake.x.round_int()));
            const tMatrixPixelsSize nextYInt = to_size(static_cast<tMatrixPixelsCoord>(nextY.round_int()));
            if (bitmap->getPixel(snowflakeX, nextYInt)) {
                fixSnowflakeAtCurrent(snowflake);
                continue;
            }

            // Move down
            snowflake.y = nextY;
        }
    }

    void render(csRandGen& /*rand*/, uint16_t /*currTime*/) const override {
        if (disabled || !matrix || !bitmap) {
            return;
        }

        const csRect target = rectDest.intersect(matrix->getRect());
        if (target.empty()) {
            return;
        }

        const tMatrixPixelsCoord endX = target.x + to_coord(target.width);
        const tMatrixPixelsCoord endY = target.y + to_coord(target.height);

        // Draw fixed snowflakes from bitmap and clear background
        for (tMatrixPixelsCoord y = target.y; y < endY; ++y) {
            for (tMatrixPixelsCoord x = target.x; x < endX; ++x) {
                const tMatrixPixelsCoord localX = x - rectDest.x;
                const tMatrixPixelsCoord localY = y - rectDest.y;
                if (bitmap->getPixel(to_size(localX), to_size(localY))) {
                    matrix->setPixel(x, y, color);
                }
            }
        }

        // Draw all falling snowflakes from array (only visible ones, y >= 0)
        // Convert local fixed-point coordinates to global fixed-point coordinates
        if (!snowflakes || count == 0) {
            return; // Early exit if no array
        }
        for (uint16_t i = 0; i < count; ++i) {
            const auto& snowflake = snowflakes[i];
            // Skip snowflakes in spawn delay phase (negative y)
            if (snowflake.y < csFP16::from_int(0)) {
                continue;
            }

            // Convert to global fixed-point coordinates
            const csFP16 globalX = csFP16::from_int(rectDest.x) + snowflake.x;
            const csFP16 globalY = csFP16::from_int(rectDest.y) + snowflake.y;
            // Check if snowflake is within target area (intersection of rect and matrix)
            // Convert to integer for bounds check
            const tMatrixPixelsCoord globalXInt = static_cast<tMatrixPixelsCoord>(globalX.round_int());
            const tMatrixPixelsCoord globalYInt = static_cast<tMatrixPixelsCoord>(globalY.round_int());
            if (globalXInt >= target.x && globalXInt < endX &&
                globalYInt >= target.y && globalYInt < endY) {
                if (smoothMovement) {
                    matrix->setPixelFloat(globalX, globalY, color);
                } else {
                    matrix->setPixel(globalXInt, globalYInt, color);
                }
            }
        }
    }

private:
    void updateBitmap() {
        delete bitmap;
        bitmap = nullptr;

        if (rectDest.empty()) {
            return;
        }

        bitmap = new csMatrixBoolean(rectDest.width, rectDest.height, true);
        // Reset state when bitmap is recreated
        filledPixelsCount = 0;
        snowfallCount = 0;
        clearingIterations = 0;
        lastDirectionWasLeft = false;
        lastUpdateTime = 0;
    }

    // Initialize snowflakes array with random values
    void initSnowflakesRandom(csRandGen& rand) {
        if (!snowflakes || count == 0 || rectDest.width == 0) {
            return;
        }

        for (uint16_t i = 0; i < count; ++i) {
            randOneSnowflake(snowflakes[i], rand);
        }
    }

    // Resize snowflakes array and initialize with random values
    void resizeOrInitSnowflakesArray() {
        // Free old array
        if (snowflakes != nullptr)
            delete[] snowflakes;

        snowflakes = nullptr;

        if (count == 0) {
            return;
        }

        // Allocate new array
        snowflakes = new Snowflake[count];

        if (snowflakes == nullptr)
        {
            count = 0;
            return;
        }
        for (uint16_t i = 0; i < count; ++i) {
            snowflakes[i].x = csFP16::from_int(cSpawnFlagForceInit);
        }
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

    void getPropInfo(uint8_t propNum, csPropInfo& info) override {
        csRenderMatrixBase::getPropInfo(propNum, info);
        switch (propNum) {
            case propColor:
                info.type = PropType::Color;
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
        matrix->fillArea(rectDest, color);
    }
};

// Effect: container that holds and calls up to 5 nested effects.
// Container does NOT create or destroy nested effects - only stores pointers.
class csRenderContainer : public csRenderMatrixBase {
public:
    static constexpr uint8_t maxEffects = 5;
    static constexpr uint8_t base = csRenderMatrixBase::propLast;
    static constexpr uint8_t propEffect1 = base + 1;
    static constexpr uint8_t propEffect2 = base + 2;
    static constexpr uint8_t propEffect3 = base + 3;
    static constexpr uint8_t propEffect4 = base + 4;
    static constexpr uint8_t propEffect5 = base + 5;
    static constexpr uint8_t propLast = propEffect5;

    // Array of pointers to nested effects (nullptr means slot is empty).
    // Container does NOT manage memory - effects must be created/destroyed externally.
    csEffectBase* effects[maxEffects] = {};

    uint8_t getPropsCount() const override {
        return propLast;
    }

    void getPropInfo(uint8_t propNum, csPropInfo& info) override {
        csRenderMatrixBase::getPropInfo(propNum, info);
        switch (propNum) {
            case propRectDest:
                info.disabled = true;
                break;
            case propRenderRectAutosize:
                info.disabled = true;
                break;
            case propEffect1:
                info.type = PropType::EffectBase;
                info.name = "Effect 1";
                info.ptr = &effects[0];
                info.readOnly = false;
                info.disabled = false;
                break;
            case propEffect2:
                info.type = PropType::EffectBase;
                info.name = "Effect 2";
                info.ptr = &effects[1];
                info.readOnly = false;
                info.disabled = false;
                break;
            case propEffect3:
                info.type = PropType::EffectBase;
                info.name = "Effect 3";
                info.ptr = &effects[2];
                info.readOnly = false;
                info.disabled = false;
                break;
            case propEffect4:
                info.type = PropType::EffectBase;
                info.name = "Effect 4";
                info.ptr = &effects[3];
                info.readOnly = false;
                info.disabled = false;
                break;
            case propEffect5:
                info.type = PropType::EffectBase;
                info.name = "Effect 5";
                info.ptr = &effects[4];
                info.readOnly = false;
                info.disabled = false;
                break;
        }
    }

    void propChanged(uint8_t propNum) override {
        // Ignore propRectDest and propRenderRectAutosize - they are disabled
        if (propNum == propRectDest || propNum == propRenderRectAutosize) {
            return;
        }
        // Call base implementation for other properties
        csRenderMatrixBase::propChanged(propNum);
        
        // If matrix destination changed, bind matrix to all nested effects
        if (propNum == csRenderMatrixBase::propMatrixDest) {
            for (uint8_t i = 0; i < maxEffects; ++i) {
                if (effects[i] != nullptr) {
                    if (auto* m = static_cast<csRenderMatrixBase*>(
                        effects[i]->queryClassFamily(PropType::EffectMatrixDest)
                    )) {
                        m->setMatrix(matrix);
                    }
                }
            }
        }
    }

    void recalc(csRandGen& rand, tTime currTime) override {
        if (disabled) {
            return;
        }
        for (uint8_t i = 0; i < maxEffects; ++i) {
            if (effects[i] != nullptr) {
                effects[i]->recalc(rand, currTime);
            }
        }
    }

    void render(csRandGen& rand, tTime currTime) const override {
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
// Time value is passed via 'time' property (int32_t).
// Extracts last 4 decimal digits from time property.
// Does not use system time - time must be set externally.
class csRenderDigitalClock : public csRenderMatrixBase {
public:
    static constexpr uint8_t base = csRenderMatrixBase::propLast;
    static constexpr uint8_t propTime = base + 1;
    static constexpr uint8_t propRenderDigit = base + 2;
    static constexpr uint8_t propSpacing = base + 3;
    static constexpr uint8_t propLast = propSpacing;

    // Time property (uint32_t): stores time value for clock display.
    //
    // Examples:
    //   - time = 1234 → extracts [1, 2, 3, 4] → displays "1234"
    //   - time = 5    → extracts [0, 0, 0, 5] → displays "0005"
    //
    // Note:
    //   This effect does NOT read system time. The time value must be set
    //   externally by the application.
    uint32_t time = 0;

    // Spacing between digits (in pixels)
    tMatrixPixelsSize spacing = 1;

    static constexpr uint8_t digitCount = 4;

    // Single glyph renderer reused for all 4 digits
    // Set via propRenderDigit property (external memory management)
    // Mutable pointer because render() modifies it but doesn't change logical state
    // Note: The associated effect (renderDigit) will always be disabled after rendering.
    //       The render() method temporarily enables it during rendering and then
    //       disables it again, so the renderDigit should not be rendered directly in
    //       the effects loop.
    mutable csRenderGlyph* renderDigit = nullptr;

    // Virtual factory method to create renderDigit renderer
    // Derived classes can override to use custom glyph types
    // Public method for external use - caller owns the returned object
    virtual csRenderGlyph* createRenderDigit() const {
        return new csRenderDigitalClockDigit();
    }

    csRenderDigitalClock() {
        // renderDigit is set externally via propRenderDigit property
    }

    ~csRenderDigitalClock() override {
        // renderDigit is managed externally, do not delete
    }

    // Class family identifier
    static constexpr PropType ClassFamilyId = PropType::EffectDigitalClock;

    // Override to return digital clock renderer family
    PropType getClassFamily() const override {
        return ClassFamilyId;
    }

    // Override to check for digital clock renderer family
    void* queryClassFamily(PropType familyId) override {
        if (familyId == ClassFamilyId) {
            return this;
        }
        // Check base class (csRenderMatrixBase) family
        return csRenderMatrixBase::queryClassFamily(familyId);
    }

    void propChanged(uint8_t propNum) override {
        csRenderMatrixBase::propChanged(propNum);
        if (propNum == propRenderDigit) {
            // Validate renderDigit type - must be csRenderGlyph*
            if (renderDigit) {
                csRenderGlyph* validRenderDigit = static_cast<csRenderGlyph*>(
                    renderDigit->queryClassFamily(PropType::EffectGlyph)
                );
                if (validRenderDigit) {
                    // Update renderDigit matrix when renderDigit is set
                    if (matrix) {
                        validRenderDigit->setMatrix(matrix);
                    }
                } else {
                    // Invalid type - ignore
                    renderDigit = nullptr;
                }
            }
        } else if (propNum == propMatrixDest) {
            // Update renderDigit matrix when matrix destination changes
            if (matrix && renderDigit) {
                renderDigit->setMatrix(matrix);
            }
        }
    }

    uint8_t getPropsCount() const override {
        return propLast;
    }

    void getPropInfo(uint8_t propNum, csPropInfo& info) override {
        csRenderMatrixBase::getPropInfo(propNum, info);
        switch (propNum) {
            case propRenderRectAutosize:
                info.disabled = true;
                break;
            case propTime:
                // Time property: uint32_t value containing time data.
                // Extracts last 4 decimal digits (rightmost) from the value.
                // Digits are extracted right-to-left and displayed left-to-right.
                // Examples: time=1234 → "4321", time=567 → "7650", time=99 → "9900"
                info.type = PropType::UInt32;
                info.name = "Time";
                info.ptr = &time;
                info.readOnly = false;
                info.disabled = false;
                break;
            case propRenderDigit:
                // Render digit property: csEffectBase* pointer to csRenderGlyph effect.
                // Used for rendering individual digits. Must be csRenderGlyph* or derived.
                // Memory is managed externally - object does not own the renderDigit.
                info.type = PropType::EffectGlyph;
                info.name = "Render digit";
                info.ptr = reinterpret_cast<void*>(&renderDigit);
                info.readOnly = false;
                info.disabled = false;
                break;
            case propSpacing:
                info.type = PropType::UInt16;
                info.name = "Spacing";
                info.ptr = &spacing;
                info.readOnly = false;
                info.disabled = false;
                break;
        }
    }

    void render(csRandGen& rand, tTime currTime) const override {
        if (disabled || !matrix || !renderDigit) {
            return;
        }

        // Temporarily enable renderDigit for rendering
        renderDigit->disabled = false;

        // Extract last 4 decimal digits from time property
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

        // Get font dimensions directly from renderDigit
        if (!renderDigit->font) {
            // Disable renderDigit before returning
            renderDigit->disabled = true;
            return;
        }

        const tMatrixPixelsSize fontWidth = to_size(renderDigit->font->width());
        const tMatrixPixelsSize fontHeight = to_size(renderDigit->font->height());

        // Calculate positions for 4 digits horizontally
        // Position them within rectDest bounds
        const tMatrixPixelsCoord startX = rectDest.x;
        const tMatrixPixelsCoord startY = rectDest.y;

        // Render each digit by updating and rendering the single renderDigit
        for (uint8_t i = 0; i < digitCount; ++i) {
            renderDigit->symbolIndex = digits[i];
            renderDigit->rectDest = csRect{
                startX + to_coord((fontWidth + spacing) * i),
                startY,
                fontWidth,
                fontHeight
            };
            renderDigit->render(rand, currTime);
        }

        // Disable renderDigit after rendering
        renderDigit->disabled = true;
    }
};

// Effect: single pixel bouncing between the walls of rectDest.
class csRenderBouncingPixel : public csRenderDynamic {
public:
    static constexpr uint8_t base = csRenderDynamic::propLast;
    static constexpr uint8_t propSmoothMovement = base + 1;
    static constexpr uint8_t propLast = propSmoothMovement;

    csColorRGBA color{255, 255, 255, 255};
    bool smoothMovement = true; // Enable sub-pixel smooth movement (default: enabled)

    uint8_t getPropsCount() const override {
        return propLast;
    }

    void getPropInfo(uint8_t propNum, csPropInfo& info) override {
        csRenderDynamic::getPropInfo(propNum, info);
        switch (propNum) {
            case propColor:
                info.type = PropType::Color;
                info.name = "Pixel color";
                info.ptr = &color;
                info.readOnly = false;
                info.disabled = false;
                break;
            case propSmoothMovement:
                info.type = PropType::Bool;
                info.name = "Smooth movement";
                info.ptr = &smoothMovement;
                info.readOnly = false;
                info.disabled = false;
                break;
        }
    }

    void propChanged(uint8_t propNum) override {
        csRenderDynamic::propChanged(propNum);
        if (propNum == propMatrixDest || propNum == propRectDest) {
            needsReset = true;
        }
    }

    void recalc(csRandGen& rand, tTime currTime) override {
        if (disabled || !matrix || rectDest.empty()) {
            return;
        }

        if (needsReset) {
            if (!initialize(rand, currTime)) {
                return;
            }
        }

        const uint16_t timeDelta = currTime - lastUpdateTime;
        if (timeDelta == 0) {
            return;
        }

        lastUpdateTime = currTime;
        const csFP16 dt = csFP16::from_int(timeDelta);
        const csFP16 motionScale = speed * kBaseSpeed * dt;
        posX += velX * motionScale;
        posY += velY * motionScale;

        handleBoundaryCollisions(rand);
    }

    void render(csRandGen& /*rand*/, tTime /*currTime*/) const override {
        if (disabled || !matrix || rectDest.empty()) {
            return;
        }

        const csRect target = rectDest.intersect(matrix->getRect());
        if (target.empty()) {
            return;
        }

        // Check bounds using rounded integer coordinates
        const tMatrixPixelsCoord px = static_cast<tMatrixPixelsCoord>(posX.round_int());
        const tMatrixPixelsCoord py = static_cast<tMatrixPixelsCoord>(posY.round_int());
        if (px < target.x || px >= target.x + to_coord(target.width) ||
            py < target.y || py >= target.y + to_coord(target.height)) {
            return;
        }

        if (smoothMovement) {
            matrix->setPixelFloat(posX, posY, color);
        } else {
            matrix->setPixel(px, py, color);
        }
    }

private:
    static const csFP16 kBaseSpeed;

    csFP16 posX{0.0f};
    csFP16 posY{0.0f};
    csFP16 velX{0.0f};
    csFP16 velY{0.0f};
    uint16_t lastUpdateTime = 0;
    bool needsReset = true;

    bool initialize(csRandGen& rand, tTime currTime) {
        if (rectDest.width == 0 || rectDest.height == 0) {
            return false;
        }

        // posX = rectDest.x + rectDest.width * 0.5
        posX = csFP16::from_int(rectDest.x) + csFP16::from_int(rectDest.width) * csFP16::float_const(0.5f);
        // posY = rectDest.y + rectDest.height * 0.5
        posY = csFP16::from_int(rectDest.y) + csFP16::from_int(rectDest.height) * csFP16::float_const(0.5f);
        const csFP16 angle = randomAngle(rand);
        velX = math::fp16_cos(angle);
        velY = math::fp16_sin(angle);
        normalizeVelocity();

        lastUpdateTime = currTime;
        needsReset = false;
        return true;
    }

    void handleBoundaryCollisions(csRandGen& rand) {
        const csFP16 minX = csFP16::from_int(rectDest.x);
        const csFP16 minY = csFP16::from_int(rectDest.y);
        // maxX = minX + rectDest.width - 1
        const csFP16 maxX = minX + csFP16::from_int(rectDest.width) - csFP16::float_const(1.0f);
        // maxY = minY + rectDest.height - 1
        const csFP16 maxY = minY + csFP16::from_int(rectDest.height) - csFP16::float_const(1.0f);

        bool collidedX = false;
        bool collidedY = false;

        if (posX < minX) {
            posX = minX;
            collidedX = true;
        } else if (posX > maxX) {
            posX = maxX;
            collidedX = true;
        }

        if (posY < minY) {
            posY = minY;
            collidedY = true;
        } else if (posY > maxY) {
            posY = maxY;
            collidedY = true;
        }

        if (collidedX || collidedY) {
            reflect(rand, collidedX, collidedY);
        }
    }

    void reflect(csRandGen& rand, bool reflectX, bool reflectY) {
        if (reflectX) {
            velX = csFP16::float_const(0.0f) - velX;
        }
        if (reflectY) {
            velY = csFP16::float_const(0.0f) - velY;
        }
        applyRandomSpread(rand);
        normalizeVelocity();
    }

    void applyRandomSpread(csRandGen& rand) {
        const csFP16 spreadDeg = randomSpread(rand);
        // angleRad = spreadDeg * kDegToRad
        const csFP16 angleRad = spreadDeg * csFP16::kDegToRad;
        const csFP16 cosA = math::fp16_cos(angleRad);
        const csFP16 sinA = math::fp16_sin(angleRad);
        // newX = velX * cosA - velY * sinA
        const csFP16 newX = velX * cosA - velY * sinA;
        // newY = velX * sinA + velY * cosA
        const csFP16 newY = velX * sinA + velY * cosA;
        velX = newX;
        velY = newY;
    }

    void normalizeVelocity() {
        // mag = sqrt(velX^2 + velY^2)
        const csFP16 magSq = velX * velX + velY * velY;
        const csFP16 mag = csFP16{sqrtf(magSq.to_float())};
        if (mag == csFP16::float_const(0.0f)) {
            velX = csFP16::float_const(1.0f);
            velY = csFP16::float_const(0.0f);
            return;
        }
        velX = velX / mag;
        velY = velY / mag;
    }

    static csFP16 randomAngle(csRandGen& rand) {
        const uint8_t raw = rand.rand();
        // fraction = raw / 256
        const csFP16 fraction = csFP16::from_ratio(raw, 256);
        // angle = fraction * kTwoPi
        return fraction * csFP16::kTwoPi;
    }

    static csFP16 randomSpread(csRandGen& rand) {
        const uint8_t spread = rand.randRange(15, 30);
        const int8_t sign = (rand.rand() & 0x1) ? 1 : -1;
        // spreadDeg = spread * sign
        return csFP16::from_int(static_cast<int16_t>(spread) * sign);
    }
};

// Fixed-point constants for csRenderBouncingPixel
const csFP16 csRenderBouncingPixel::kBaseSpeed = csFP16::float_const(0.05f);

} // namespace amp
