#pragma once

#include <stdint.h>
#include <math.h>

#include "color_rgba.hpp"
#include "matrix_pixels.hpp"
#include "rect.hpp"
#include "math.hpp"
#include "matrix_render.hpp"
#include "font3x5.h"




namespace amp {


// Simple animated RGB gradient (float).
class csRenderGradient final : public csRenderMatrixBase {
public:
    void render(csRandGen& /*rand*/, uint16_t currTime) const override {
        if (!matrix) {
            return;
        }
        const float t = static_cast<float>(currTime) * 0.001f;

        auto wave = [](float v) -> uint8_t {
            return static_cast<uint8_t>((sin(v) * 0.5f + 0.5f) * 255.0f);
        };

        const csRect target = rect.intersect(matrix->getRect());
        if (target.empty()) {
            return;
        }

        const tMatrixPixelsCoord endX = target.x + to_coord(target.width);
        const tMatrixPixelsCoord endY = target.y + to_coord(target.height);
        for (tMatrixPixelsCoord y = target.y; y < endY; ++y) {
            for (tMatrixPixelsCoord x = target.x; x < endX; ++x) {
                const float xf = static_cast<float>(x) * 0.4f;
                const float yf = static_cast<float>(y) * 0.4f;
                const uint8_t r = wave(t * 0.8f + xf);
                const uint8_t g = wave(t * 1.0f + yf);
                const uint8_t b = wave(t * 0.6f + xf + yf * 0.5f);
                matrix->setPixel(x, y, csColorRGBA{255, r, g, b});
            }
        }
    }
};

// Animated RGB gradient using fixed-point for time/phase.
class csRenderGradientFP final : public csRenderMatrixBase {
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
        const csFP32 t = csFP32::from_raw(t_raw);

        // Fixed-point constants.
        static const csFP32 k08 = csFP32::float_const(0.7f);
        static const csFP32 k06 = csFP32::float_const(0.5f);
        static const csFP32 k04 = csFP32::float_const(0.3f);
        static const csFP32 k05 = csFP32::float_const(0.4f);
        const csRect target = rect.intersect(matrix->getRect());
        if (target.empty()) {
            return;
        }

        const tMatrixPixelsCoord endX = target.x + to_coord(target.width);
        const tMatrixPixelsCoord endY = target.y + to_coord(target.height);
        for (tMatrixPixelsCoord y = target.y; y < endY; ++y) {
            const csFP32 yf = csFP32::from_int(y);
            const csFP32 yf_scaled = yf * k04;
            for (tMatrixPixelsCoord x = target.x; x < endX; ++x) {
                const csFP32 xf = csFP32::from_int(x);
                const csFP32 xf_scaled = xf * k04;
                const uint8_t r = wave_fp(t * k08 + xf_scaled);
                const uint8_t g = wave_fp(t + yf_scaled);
                const uint8_t b = wave_fp(t * k06 + xf_scaled + yf_scaled * k05);
                matrix->setPixel(x, y, csColorRGBA{255, r, g, b});
            }
        }
    }
};

// Simple sinusoidal plasma effect (float).
class csRenderPlasma final : public csRenderMatrixBase {
public:
    void render(csRandGen& /*rand*/, uint16_t currTime) const override {
        if (!matrix) {
            return;
        }
        const float t = static_cast<float>(currTime) * 0.0025f;
        const csRect target = rect.intersect(matrix->getRect());
        if (target.empty()) {
            return;
        }

        const tMatrixPixelsCoord endX = target.x + to_coord(target.width);
        const tMatrixPixelsCoord endY = target.y + to_coord(target.height);
        for (tMatrixPixelsCoord y = target.y; y < endY; ++y) {
            for (tMatrixPixelsCoord x = target.x; x < endX; ++x) {
                const float xf = static_cast<float>(x);
                const float yf = static_cast<float>(y);
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
class csRenderGlyph final : public csRenderMatrixBase {
public:
    static constexpr uint8_t paramSymbolIndex = 4;
    static constexpr uint8_t paramSymbolColor = 5;
    static constexpr uint8_t paramBackgroundColor = 6;
    static constexpr uint8_t paramFontWidth = 7;
    static constexpr uint8_t paramFontHeight = 8;

    uint8_t symbolIndex = 0;
    csColorRGBA symbolColor{255, 255, 255, 255};
    csColorRGBA backgroundColor{255, 0, 0, 0};
    tMatrixPixelsSize fontWidth = static_cast<tMatrixPixelsSize>(FONT_WIDTH);
    tMatrixPixelsSize fontHeight = static_cast<tMatrixPixelsSize>(FONT_HEIGHT);

    uint8_t getParamsCount() const override {
        return paramFontHeight;
    }

    void getParamInfo(uint8_t paramNum, csParamInfo& info) override {
        info.readOnly = false;
        info.disabled = false;
        switch (paramNum) {
            case paramSymbolIndex:
                info.type = ParamType::UInt8;
                info.name = "Glyph index";
                info.ptr = &symbolIndex;
                break;
            case paramSymbolColor:
                info.type = ParamType::Color;
                info.name = "Symbol color";
                info.ptr = &symbolColor;
                break;
            case paramBackgroundColor:
                info.type = ParamType::Color;
                info.name = "Background color";
                info.ptr = &backgroundColor;
                break;
            case paramFontWidth:
                info.type = ParamType::UInt16;
                info.name = "Font width";
                info.ptr = &fontWidth;
                info.readOnly = true;
                break;
            case paramFontHeight:
                info.type = ParamType::UInt16;
                info.name = "Font height";
                info.ptr = &fontHeight;
                info.readOnly = true;
                break;
            default:
                csRenderMatrixBase::getParamInfo(paramNum, info);
                break;
        }
    }

    void paramChanged(uint8_t paramNum) override {
        csRenderMatrixBase::paramChanged(paramNum);
        if (paramNum == paramSymbolIndex && symbolIndex >= FONT_COUNT) {
            symbolIndex = static_cast<uint8_t>(FONT_COUNT - 1);
        }
    }

    void updateRenderRect() override {
        if (!matrix || !renderRectAutosize) {
            return;
        }
        const csRect mrect = matrix->getRect();
        rect = csRect{
            mrect.x,
            mrect.y,
            static_cast<tMatrixPixelsSize>(FONT_WIDTH),
            static_cast<tMatrixPixelsSize>(FONT_HEIGHT)
        };
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
        for (tMatrixPixelsCoord y = target.y; y < endY; ++y) {
            for (tMatrixPixelsCoord x = target.x; x < endX; ++x) {
                matrix->setPixel(x, y, backgroundColor);
            }
        }

        const tMatrixPixelsSize glyphWidth = math::min(rect.width, static_cast<tMatrixPixelsSize>(FONT_WIDTH));
        const tMatrixPixelsSize glyphHeight = math::min(rect.height, static_cast<tMatrixPixelsSize>(FONT_HEIGHT));
        if (glyphWidth == 0 || glyphHeight == 0) {
            return;
        }

        const tMatrixPixelsCoord offsetX = rect.x + to_coord((rect.width - glyphWidth) / 2);
        const tMatrixPixelsCoord offsetY = rect.y + to_coord((rect.height - glyphHeight) / 2);

        const uint8_t safeIndex = (symbolIndex < FONT_COUNT) ? symbolIndex : static_cast<uint8_t>(FONT_COUNT - 1);
        for (tMatrixPixelsSize row = 0; row < glyphHeight; ++row) {
            const TFontBitLine glyphRow = LedFont[safeIndex][row];
            for (tMatrixPixelsSize col = 0; col < glyphWidth; ++col) {
                const TFontBitLine mask = static_cast<TFontBitLine>(1 << (7u - static_cast<unsigned>(col)));
                if ((glyphRow & mask) == 0) {
                    continue;
                }
                const tMatrixPixelsCoord px = offsetX + to_coord(col);
                const tMatrixPixelsCoord py = offsetY + to_coord(row);
                matrix->setPixel(px, py, symbolColor);
            }
        }
    }
};

} // namespace amp
