#pragma once

#include <stdint.h>

#include "color_rgba.hpp"
#include "matrix_pixels.hpp"
#include "rect.hpp"
#include "fixed_point.hpp"
#include "matrix_types.hpp"

using namespace amp::math;

namespace amp {

// Render filled triangle in target rectangle
void fillTriangleSlow(const csRect& target, float x1, float y1, float x2, float y2, float x3, float y3, csMatrixPixels* matrix, const csColorRGBA& color) {
    if (!matrix || target.empty()) {
        return;
    }

    // Precompute edge vectors for efficiency
    const float dx12 = x2 - x1;
    const float dy12 = y2 - y1;
    const float dx23 = x3 - x2;
    const float dy23 = y3 - y2;
    const float dx31 = x1 - x3;
    const float dy31 = y1 - y3;

    const tMatrixPixelsCoord endX = target.x + to_coord(target.width);
    const tMatrixPixelsCoord endY = target.y + to_coord(target.height);

    for (tMatrixPixelsCoord y = target.y; y < endY; ++y) {
        const float py = static_cast<float>(y) + 0.5f;
        for (tMatrixPixelsCoord x = target.x; x < endX; ++x) {
            const float px = static_cast<float>(x) + 0.5f;

            // Edge function for each edge (cross product)
            // Edge 1: from vertex 1 to vertex 2
            const float edge1 = (px - x1) * dy12 - (py - y1) * dx12;
            // Edge 2: from vertex 2 to vertex 3
            const float edge2 = (px - x2) * dy23 - (py - y2) * dx23;
            // Edge 3: from vertex 3 to vertex 1
            const float edge3 = (px - x3) * dy31 - (py - y3) * dx31;

            // Point is inside triangle if all edge functions have the same sign
            const bool inside = (edge1 >= 0.0f && edge2 >= 0.0f && edge3 >= 0.0f) ||
                               (edge1 <= 0.0f && edge2 <= 0.0f && edge3 <= 0.0f);

            if (inside) {
                matrix->setPixel(x, y, color);
            }
        }
    }
}

// Render filled triangle using scanline algorithm with incremental approach (x += slope)
void fillTriangleScanlineFast(const csRect& target,
                          float xTop, float yTop,   // Top vertex
                          float xMid, float yMid,   // Middle vertex (by Y)
                          float xBot, float yBot, csMatrixPixels* matrix, const csColorRGBA& color) {
    if (!matrix || target.empty()) {
        return;
    }

    // Validate vertex ordering: yTop < yMid < yBot
    if (yTop >= yMid || yMid >= yBot) {
        return;
    }

    // Calculate edge slopes
    const float dy1 = yMid - yTop;
    const float dy2 = yBot - yTop;
    const float dy3 = yBot - yMid;

    // Handle division by zero for horizontal edges
    const float slope1 = (dy1 != 0.0f) ? ((xMid - xTop) / dy1) : 0.0f;
    const float slope2 = (dy2 != 0.0f) ? ((xBot - xTop) / dy2) : 0.0f;
    const float slope3 = (dy3 != 0.0f) ? ((xBot - xMid) / dy3) : 0.0f;

    const tMatrixPixelsCoord targetEndY = target.y + to_coord(target.height);
    const tMatrixPixelsCoord targetEndX = target.x + to_coord(target.width);

    // Top section: from yTop to yMid
    const tMatrixPixelsCoord yMidCoord = static_cast<tMatrixPixelsCoord>(yMid + 0.5f);
    const tMatrixPixelsCoord yTopStart = static_cast<tMatrixPixelsCoord>(yTop + 0.5f);
    const tMatrixPixelsCoord yTopClipped = (yTopStart < target.y) ? target.y : yTopStart;
    const tMatrixPixelsCoord yMidClipped = (yMidCoord > targetEndY) ? targetEndY : yMidCoord;

    // Initialize X positions for first scanline
    float xLeft = xTop;
    float xRight = xTop;
    if (yTopClipped > yTopStart) {
        // Adjust initial positions if we start below yTop due to clipping
        const float dyOffset = static_cast<float>(yTopClipped) + 0.5f - yTop;
        xLeft += slope1 * dyOffset;
        xRight += slope2 * dyOffset;
    }

    for (tMatrixPixelsCoord y = yTopClipped; y < yMidClipped; ++y) {
        // Update X positions incrementally
        if (y > yTopClipped) {
            xLeft += slope1;
            xRight += slope2;
        }

        // Determine left and right edges
        const float xMin = (xLeft < xRight) ? xLeft : xRight;
        const float xMax = (xLeft > xRight) ? xLeft : xRight;

        // Clip to target bounds
        const tMatrixPixelsCoord xStart = static_cast<tMatrixPixelsCoord>(xMin + 0.5f);
        const tMatrixPixelsCoord xEnd = static_cast<tMatrixPixelsCoord>(xMax + 0.5f);
        const tMatrixPixelsCoord xStartClipped = (xStart < target.x) ? target.x : xStart;
        const tMatrixPixelsCoord xEndClipped = (xEnd > targetEndX) ? targetEndX : xEnd;

        // Draw horizontal line
        for (tMatrixPixelsCoord x = xStartClipped; x < xEndClipped; ++x) {
            matrix->setPixel(x, y, color);
        }
    }

    // Bottom section: from yMid to yBot
    const tMatrixPixelsCoord yBotCoord = static_cast<tMatrixPixelsCoord>(yBot + 0.5f);
    const tMatrixPixelsCoord yMidStart = (yMidCoord < target.y) ? target.y : yMidCoord;
    const tMatrixPixelsCoord yBotClipped = (yBotCoord > targetEndY) ? targetEndY : yBotCoord;

    // Initialize X positions for first scanline of bottom section
    xLeft = xMid;
    // Calculate xRight at yMid for continuity
    const float dyFromTopToMid = yMid - yTop;
    xRight = xTop + slope2 * dyFromTopToMid;
    if (yMidStart > yMidCoord) {
        // Adjust initial positions if we start below yMid due to clipping
        const float dyOffset = static_cast<float>(yMidStart) + 0.5f - yMid;
        xLeft += slope3 * dyOffset;
        xRight += slope2 * dyOffset;
    }

    for (tMatrixPixelsCoord y = yMidStart; y < yBotClipped; ++y) {
        // Update X positions incrementally
        if (y > yMidStart) {
            xLeft += slope3;
            xRight += slope2;
        }

        // Determine left and right edges
        const float xMin = (xLeft < xRight) ? xLeft : xRight;
        const float xMax = (xLeft > xRight) ? xLeft : xRight;

        // Clip to target bounds
        const tMatrixPixelsCoord xStart = static_cast<tMatrixPixelsCoord>(xMin + 0.5f);
        const tMatrixPixelsCoord xEnd = static_cast<tMatrixPixelsCoord>(xMax + 0.5f);
        const tMatrixPixelsCoord xStartClipped = (xStart < target.x) ? target.x : xStart;
        const tMatrixPixelsCoord xEndClipped = (xEnd > targetEndX) ? targetEndX : xEnd;

        // Draw horizontal line
        for (tMatrixPixelsCoord x = xStartClipped; x < xEndClipped; ++x) {
            matrix->setPixel(x, y, color);
        }
    }
}

// Render filled triangle using scanline algorithm with direct X calculation on each scanline
void fillTriangleScanline(const csRect& target,
                                 float xTop, float yTop,   // Top vertex
                                 float xMid, float yMid,   // Middle vertex (by Y)
                                 float xBot, float yBot, csMatrixPixels* matrix, const csColorRGBA& color) {
    if (!matrix || target.empty()) {
        return;
    }

    // Validate vertex ordering: yTop < yMid < yBot
    if (yTop >= yMid || yMid >= yBot) {
        return;
    }

    const tMatrixPixelsCoord targetEndY = target.y + to_coord(target.height);
    const tMatrixPixelsCoord targetEndX = target.x + to_coord(target.width);

    // Top section: from yTop to yMid
    const tMatrixPixelsCoord yMidCoord = static_cast<tMatrixPixelsCoord>(yMid + 0.5f);
    const tMatrixPixelsCoord yTopStart = static_cast<tMatrixPixelsCoord>(yTop + 0.5f);
    const tMatrixPixelsCoord yTopClipped = (yTopStart < target.y) ? target.y : yTopStart;
    const tMatrixPixelsCoord yMidClipped = (yMidCoord > targetEndY) ? targetEndY : yMidCoord;

    const float dy1 = yMid - yTop;
    const float dy2 = yBot - yTop;

    for (tMatrixPixelsCoord y = yTopClipped; y < yMidClipped; ++y) {
        const float yFloat = static_cast<float>(y) + 0.5f;
        const float dyFromTop = yFloat - yTop;

        // Calculate X positions directly using linear equation
        float xLeft, xRight;
        if (dy1 != 0.0f) {
            xLeft = xTop + dyFromTop * (xMid - xTop) / dy1;
        } else {
            xLeft = xTop; // Horizontal edge
        }
        if (dy2 != 0.0f) {
            xRight = xTop + dyFromTop * (xBot - xTop) / dy2;
        } else {
            xRight = xTop; // Horizontal edge
        }

        // Determine left and right edges
        const float xMin = (xLeft < xRight) ? xLeft : xRight;
        const float xMax = (xLeft > xRight) ? xLeft : xRight;

        // Clip to target bounds
        const tMatrixPixelsCoord xStart = static_cast<tMatrixPixelsCoord>(xMin + 0.5f);
        const tMatrixPixelsCoord xEnd = static_cast<tMatrixPixelsCoord>(xMax + 0.5f);
        const tMatrixPixelsCoord xStartClipped = (xStart < target.x) ? target.x : xStart;
        const tMatrixPixelsCoord xEndClipped = (xEnd > targetEndX) ? targetEndX : xEnd;

        // Draw horizontal line
        for (tMatrixPixelsCoord x = xStartClipped; x < xEndClipped; ++x) {
            matrix->setPixel(x, y, color);
        }
    }

    // Bottom section: from yMid to yBot
    const tMatrixPixelsCoord yBotCoord = static_cast<tMatrixPixelsCoord>(yBot + 0.5f);
    const tMatrixPixelsCoord yMidStart = (yMidCoord < target.y) ? target.y : yMidCoord;
    const tMatrixPixelsCoord yBotClipped = (yBotCoord > targetEndY) ? targetEndY : yBotCoord;

    const float dy3 = yBot - yMid;

    for (tMatrixPixelsCoord y = yMidStart; y < yBotClipped; ++y) {
        const float yFloat = static_cast<float>(y) + 0.5f;
        const float dyFromMid = yFloat - yMid;
        const float dyFromTop = yFloat - yTop;

        // Calculate X positions directly using linear equation
        float xLeft, xRight;
        if (dy3 != 0.0f) {
            xLeft = xMid + dyFromMid * (xBot - xMid) / dy3;
        } else {
            xLeft = xMid; // Horizontal edge
        }
        if (dy2 != 0.0f) {
            xRight = xTop + dyFromTop * (xBot - xTop) / dy2;
        } else {
            xRight = xTop; // Horizontal edge
        }

        // Determine left and right edges
        const float xMin = (xLeft < xRight) ? xLeft : xRight;
        const float xMax = (xLeft > xRight) ? xLeft : xRight;

        // Clip to target bounds
        const tMatrixPixelsCoord xStart = static_cast<tMatrixPixelsCoord>(xMin + 0.5f);
        const tMatrixPixelsCoord xEnd = static_cast<tMatrixPixelsCoord>(xMax + 0.5f);
        const tMatrixPixelsCoord xStartClipped = (xStart < target.x) ? target.x : xStart;
        const tMatrixPixelsCoord xEndClipped = (xEnd > targetEndX) ? targetEndX : xEnd;

        // Draw horizontal line
        for (tMatrixPixelsCoord x = xStartClipped; x < xEndClipped; ++x) {
            matrix->setPixel(x, y, color);
        }
    }
}

// Render filled triangle in target rectangle (csFP32 version)
void fillTriangleSlowFP32(const csRect& target, csFP32 x1, csFP32 y1, csFP32 x2, csFP32 y2, csFP32 x3, csFP32 y3, csMatrixPixels* matrix, const csColorRGBA& color) {
    if (!matrix || target.empty()) {
        return;
    }

    // Precompute edge vectors for efficiency
    const csFP32 dx12 = x2 - x1;
    const csFP32 dy12 = y2 - y1;
    const csFP32 dx23 = x3 - x2;
    const csFP32 dy23 = y3 - y2;
    const csFP32 dx31 = x1 - x3;
    const csFP32 dy31 = y1 - y3;

    const tMatrixPixelsCoord endX = target.x + to_coord(target.width);
    const tMatrixPixelsCoord endY = target.y + to_coord(target.height);

    static const csFP32 half = csFP32::float_const(0.5f);
    static const csFP32 zero = csFP32::float_const(0.0f);

    for (tMatrixPixelsCoord y = target.y; y < endY; ++y) {
        const csFP32 py = csFP32::from_int(y) + half;
        for (tMatrixPixelsCoord x = target.x; x < endX; ++x) {
            const csFP32 px = csFP32::from_int(x) + half;

            // Edge function for each edge (cross product)
            // Edge 1: from vertex 1 to vertex 2
            const csFP32 edge1 = (px - x1) * dy12 - (py - y1) * dx12;
            // Edge 2: from vertex 2 to vertex 3
            const csFP32 edge2 = (px - x2) * dy23 - (py - y2) * dx23;
            // Edge 3: from vertex 3 to vertex 1
            const csFP32 edge3 = (px - x3) * dy31 - (py - y3) * dx31;

            // Point is inside triangle if all edge functions have the same sign
            const bool inside = (edge1 >= zero && edge2 >= zero && edge3 >= zero) ||
                               (edge1 <= zero && edge2 <= zero && edge3 <= zero);

            if (inside) {
                matrix->setPixel(x, y, color);
            }
        }
    }
}

// Render filled triangle using scanline algorithm with incremental approach (x += slope) (csFP32 version)
void fillTriangleScanlineFastFP32(const csRect& target,
                          csFP32 xTop, csFP32 yTop,   // Top vertex
                          csFP32 xMid, csFP32 yMid,   // Middle vertex (by Y)
                          csFP32 xBot, csFP32 yBot, csMatrixPixels* matrix, const csColorRGBA& color) {
    if (!matrix || target.empty()) {
        return;
    }

    // Validate vertex ordering: yTop < yMid < yBot
    static const csFP32 zero = csFP32::float_const(0.0f);
    if (yTop >= yMid || yMid >= yBot) {
        return;
    }

    // Calculate edge slopes
    const csFP32 dy1 = yMid - yTop;
    const csFP32 dy2 = yBot - yTop;
    const csFP32 dy3 = yBot - yMid;

    // Handle division by zero for horizontal edges
    const csFP32 slope1 = (dy1.raw != 0) ? ((xMid - xTop) / dy1) : zero;
    const csFP32 slope2 = (dy2.raw != 0) ? ((xBot - xTop) / dy2) : zero;
    const csFP32 slope3 = (dy3.raw != 0) ? ((xBot - xMid) / dy3) : zero;

    const tMatrixPixelsCoord targetEndY = target.y + to_coord(target.height);
    const tMatrixPixelsCoord targetEndX = target.x + to_coord(target.width);

    static const csFP32 half = csFP32::float_const(0.5f);

    // Top section: from yTop to yMid
    const tMatrixPixelsCoord yMidCoord = static_cast<tMatrixPixelsCoord>(yMid.to_float() + 0.5f);
    const tMatrixPixelsCoord yTopStart = static_cast<tMatrixPixelsCoord>(yTop.to_float() + 0.5f);
    const tMatrixPixelsCoord yTopClipped = (yTopStart < target.y) ? target.y : yTopStart;
    const tMatrixPixelsCoord yMidClipped = (yMidCoord > targetEndY) ? targetEndY : yMidCoord;

    // Initialize X positions for first scanline
    csFP32 xLeft = xTop;
    csFP32 xRight = xTop;
    if (yTopClipped > yTopStart) {
        // Adjust initial positions if we start below yTop due to clipping
        const csFP32 dyOffset = csFP32::from_int(yTopClipped) + half - yTop;
        xLeft += slope1 * dyOffset;
        xRight += slope2 * dyOffset;
    }

    for (tMatrixPixelsCoord y = yTopClipped; y < yMidClipped; ++y) {
        // Update X positions incrementally
        if (y > yTopClipped) {
            xLeft += slope1;
            xRight += slope2;
        }

        // Determine left and right edges
        const csFP32 xMin = (xLeft < xRight) ? xLeft : xRight;
        const csFP32 xMax = (xLeft > xRight) ? xLeft : xRight;

        // Clip to target bounds
        const tMatrixPixelsCoord xStart = static_cast<tMatrixPixelsCoord>(xMin.to_float() + 0.5f);
        const tMatrixPixelsCoord xEnd = static_cast<tMatrixPixelsCoord>(xMax.to_float() + 0.5f);
        const tMatrixPixelsCoord xStartClipped = (xStart < target.x) ? target.x : xStart;
        const tMatrixPixelsCoord xEndClipped = (xEnd > targetEndX) ? targetEndX : xEnd;

        // Draw horizontal line
        for (tMatrixPixelsCoord x = xStartClipped; x < xEndClipped; ++x) {
            matrix->setPixel(x, y, color);
        }
    }

    // Bottom section: from yMid to yBot
    const tMatrixPixelsCoord yBotCoord = static_cast<tMatrixPixelsCoord>(yBot.to_float() + 0.5f);
    const tMatrixPixelsCoord yMidStart = (yMidCoord < target.y) ? target.y : yMidCoord;
    const tMatrixPixelsCoord yBotClipped = (yBotCoord > targetEndY) ? targetEndY : yBotCoord;

    // Initialize X positions for first scanline of bottom section
    xLeft = xMid;
    // Calculate xRight at yMid for continuity
    const csFP32 dyFromTopToMid = yMid - yTop;
    xRight = xTop + slope2 * dyFromTopToMid;
    if (yMidStart > yMidCoord) {
        // Adjust initial positions if we start below yMid due to clipping
        const csFP32 dyOffset = csFP32::from_int(yMidStart) + half - yMid;
        xLeft += slope3 * dyOffset;
        xRight += slope2 * dyOffset;
    }

    for (tMatrixPixelsCoord y = yMidStart; y < yBotClipped; ++y) {
        // Update X positions incrementally
        if (y > yMidStart) {
            xLeft += slope3;
            xRight += slope2;
        }

        // Determine left and right edges
        const csFP32 xMin = (xLeft < xRight) ? xLeft : xRight;
        const csFP32 xMax = (xLeft > xRight) ? xLeft : xRight;

        // Clip to target bounds
        const tMatrixPixelsCoord xStart = static_cast<tMatrixPixelsCoord>(xMin.to_float() + 0.5f);
        const tMatrixPixelsCoord xEnd = static_cast<tMatrixPixelsCoord>(xMax.to_float() + 0.5f);
        const tMatrixPixelsCoord xStartClipped = (xStart < target.x) ? target.x : xStart;
        const tMatrixPixelsCoord xEndClipped = (xEnd > targetEndX) ? targetEndX : xEnd;

        // Draw horizontal line
        for (tMatrixPixelsCoord x = xStartClipped; x < xEndClipped; ++x) {
            matrix->setPixel(x, y, color);
        }
    }
}

// Render filled triangle using scanline algorithm with direct X calculation on each scanline (csFP32 version)
void fillTriangleScanlineFP32(const csRect& target,
                                 csFP32 xTop, csFP32 yTop,   // Top vertex
                                 csFP32 xMid, csFP32 yMid,   // Middle vertex (by Y)
                                 csFP32 xBot, csFP32 yBot, csMatrixPixels* matrix, const csColorRGBA& color) {
    if (!matrix || target.empty()) {
        return;
    }

    // Validate vertex ordering: yTop < yMid < yBot
    if (yTop >= yMid || yMid >= yBot) {
        return;
    }

    const tMatrixPixelsCoord targetEndY = target.y + to_coord(target.height);
    const tMatrixPixelsCoord targetEndX = target.x + to_coord(target.width);

    static const csFP32 half = csFP32::float_const(0.5f);
    static const csFP32 zero = csFP32::float_const(0.0f);

    // Top section: from yTop to yMid
    const tMatrixPixelsCoord yMidCoord = static_cast<tMatrixPixelsCoord>(yMid.to_float() + 0.5f);
    const tMatrixPixelsCoord yTopStart = static_cast<tMatrixPixelsCoord>(yTop.to_float() + 0.5f);
    const tMatrixPixelsCoord yTopClipped = (yTopStart < target.y) ? target.y : yTopStart;
    const tMatrixPixelsCoord yMidClipped = (yMidCoord > targetEndY) ? targetEndY : yMidCoord;

    const csFP32 dy1 = yMid - yTop;
    const csFP32 dy2 = yBot - yTop;

    for (tMatrixPixelsCoord y = yTopClipped; y < yMidClipped; ++y) {
        const csFP32 yFloat = csFP32::from_int(y) + half;
        const csFP32 dyFromTop = yFloat - yTop;

        // Calculate X positions directly using linear equation
        csFP32 xLeft, xRight;
        if (dy1.raw != 0) {
            xLeft = xTop + dyFromTop * (xMid - xTop) / dy1;
        } else {
            xLeft = xTop; // Horizontal edge
        }
        if (dy2.raw != 0) {
            xRight = xTop + dyFromTop * (xBot - xTop) / dy2;
        } else {
            xRight = xTop; // Horizontal edge
        }

        // Determine left and right edges
        const csFP32 xMin = (xLeft < xRight) ? xLeft : xRight;
        const csFP32 xMax = (xLeft > xRight) ? xLeft : xRight;

        // Clip to target bounds
        const tMatrixPixelsCoord xStart = static_cast<tMatrixPixelsCoord>(xMin.to_float() + 0.5f);
        const tMatrixPixelsCoord xEnd = static_cast<tMatrixPixelsCoord>(xMax.to_float() + 0.5f);
        const tMatrixPixelsCoord xStartClipped = (xStart < target.x) ? target.x : xStart;
        const tMatrixPixelsCoord xEndClipped = (xEnd > targetEndX) ? targetEndX : xEnd;

        // Draw horizontal line
        for (tMatrixPixelsCoord x = xStartClipped; x < xEndClipped; ++x) {
            matrix->setPixel(x, y, color);
        }
    }

    // Bottom section: from yMid to yBot
    const tMatrixPixelsCoord yBotCoord = static_cast<tMatrixPixelsCoord>(yBot.to_float() + 0.5f);
    const tMatrixPixelsCoord yMidStart = (yMidCoord < target.y) ? target.y : yMidCoord;
    const tMatrixPixelsCoord yBotClipped = (yBotCoord > targetEndY) ? targetEndY : yBotCoord;

    const csFP32 dy3 = yBot - yMid;

    for (tMatrixPixelsCoord y = yMidStart; y < yBotClipped; ++y) {
        const csFP32 yFloat = csFP32::from_int(y) + half;
        const csFP32 dyFromMid = yFloat - yMid;
        const csFP32 dyFromTop = yFloat - yTop;

        // Calculate X positions directly using linear equation
        csFP32 xLeft, xRight;
        if (dy3.raw != 0) {
            xLeft = xMid + dyFromMid * (xBot - xMid) / dy3;
        } else {
            xLeft = xMid; // Horizontal edge
        }
        if (dy2.raw != 0) {
            xRight = xTop + dyFromTop * (xBot - xTop) / dy2;
        } else {
            xRight = xTop; // Horizontal edge
        }

        // Determine left and right edges
        const csFP32 xMin = (xLeft < xRight) ? xLeft : xRight;
        const csFP32 xMax = (xLeft > xRight) ? xLeft : xRight;

        // Clip to target bounds
        const tMatrixPixelsCoord xStart = static_cast<tMatrixPixelsCoord>(xMin.to_float() + 0.5f);
        const tMatrixPixelsCoord xEnd = static_cast<tMatrixPixelsCoord>(xMax.to_float() + 0.5f);
        const tMatrixPixelsCoord xStartClipped = (xStart < target.x) ? target.x : xStart;
        const tMatrixPixelsCoord xEndClipped = (xEnd > targetEndX) ? targetEndX : xEnd;

        // Draw horizontal line
        for (tMatrixPixelsCoord x = xStartClipped; x < xEndClipped; ++x) {
            matrix->setPixel(x, y, color);
        }
    }
}

} // namespace amp
