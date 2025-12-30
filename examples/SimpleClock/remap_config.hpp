#pragma once

#include "../../src/matrix_types.hpp"

using amp::tMatrixPixelsSize;
using amp::tMatrixPixelsCoord;

//////////////////////////////////////////////

static constexpr tMatrixPixelsSize cRemapSrcArrayLen = 60; // 12x5
static constexpr tMatrixPixelsCoord cRemapSrcArray[60] = {
    0, 3, 0, 0, 9, 0, 0, 17, 0, 0, 24, 0,
    4, 0, 2, 10, 0, 0/*x*/, 18, 0, 16, 25, 0, 23,
    0, 1, 0, 0, 8, 0, 0, 15, 0, 0, 22, 0,
    5, 0, 7, 11, 0, 13, 19, 0, 21, 26, 0, 28,
    0, 6, 0, 0, 12, 0, 0, 20, 0, 0, 27, 0,
};

static constexpr tMatrixPixelsSize cRemapDestMatrixLen = 28;
static constexpr tMatrixPixelsSize cSrcWidth = 12;
static constexpr tMatrixPixelsSize cSrcHeight = 5;

