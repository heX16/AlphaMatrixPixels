#include <iostream>
#include <string>
#include "matrix_pixels.hpp"

// Minimal self-contained test runner (no external frameworks).
struct TestStats {
    int passed{0};
    int failed{0};
};

#define EXPECT_TRUE(cond, msg)                                                                 \
    do {                                                                                       \
        if (cond) {                                                                            \
            ++stats.passed;                                                                    \
        } else {                                                                               \
            ++stats.failed;                                                                    \
            std::cerr << "FAIL [" << testName << "] " << msg << " (line " << __LINE__ << ")\n";\
        }                                                                                      \
    } while (0)

inline bool colorEq(const csColorRGBA& c, uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    return c.r == r && c.g == g && c.b == b && c.a == a;
}

// Reference SourceOver implementation (straight alpha) for expectations.
inline csColorRGBA refSourceOver(csColorRGBA dst, csColorRGBA src, uint8_t global_alpha = 255) {
    const uint8_t As = mul8(src.a, global_alpha);
    const uint8_t invAs = static_cast<uint8_t>(255u - As);
    const uint8_t Aout = static_cast<uint8_t>(As + mul8(dst.a, invAs));
    if (Aout == 0) {
        return csColorRGBA{0, 0, 0, 0};
    }
    const uint8_t Rout = csColorRGBA::blendChannel(src.r, dst.r, As, dst.a, invAs, Aout);
    const uint8_t Gout = csColorRGBA::blendChannel(src.g, dst.g, As, dst.a, invAs, Aout);
    const uint8_t Bout = csColorRGBA::blendChannel(src.b, dst.b, As, dst.a, invAs, Aout);
    return csColorRGBA{Rout, Gout, Bout, Aout};
}

void test_color_component_ctor(TestStats& stats) {
    const char* testName = "color_component_ctor";
    csColorRGBA c{10, 20, 30, 40};
    EXPECT_TRUE(colorEq(c, 10, 20, 30, 40), "component constructor keeps channels");
}

void test_color_packed_alpha_promote(TestStats& stats) {
    const char* testName = "color_packed_alpha_promote";
    // Packed as 0xRRGGBBAA; here AA==0 -> should be promoted to 0xFF.
    csColorRGBA c{0x01020300u};
    EXPECT_TRUE(colorEq(c, 0x01, 0x02, 0x03, 0xFF), "alpha promoted when zero");
}

void test_color_packed_preserve(TestStats& stats) {
    const char* testName = "color_packed_preserve";
    csColorRGBA c{0x0A0B0C7Fu};
    EXPECT_TRUE(colorEq(c, 0x0A, 0x0B, 0x0C, 0x7F), "packed alpha preserved when non-zero");
}

void test_color_divide(TestStats& stats) {
    const char* testName = "color_divide";
    csColorRGBA c{200, 100, 50, 255};
    c /= 2;
    EXPECT_TRUE(colorEq(c, 100, 50, 25, 127), "division applies to all channels");
}

void test_color_blend_ops(TestStats& stats) {
    const char* testName = "color_blend_ops";
    csColorRGBA dst{50, 60, 70, 80};
    csColorRGBA src{200, 10, 20, 255};
    auto sum = dst + src;
    EXPECT_TRUE(colorEq(sum, src.r, src.g, src.b, src.a), "operator+ with opaque src returns src");
    dst += src;
    EXPECT_TRUE(colorEq(dst, src.r, src.g, src.b, src.a), "operator+= mutates to src when opaque");
}

void test_color_source_over_global_alpha(TestStats& stats) {
    const char* testName = "color_source_over_global_alpha";
    const csColorRGBA dst{60, 80, 100, 120};
    const csColorRGBA src{200, 40, 20, 128};
    const uint8_t global = 128;
    const csColorRGBA expected = refSourceOver(dst, src, global);
    const csColorRGBA actual = csColorRGBA::sourceOverStraight(dst, src, global);
    EXPECT_TRUE(colorEq(actual, expected.r, expected.g, expected.b, expected.a), "sourceOver with global alpha matches reference");
}

void test_color_source_over_no_global(TestStats& stats) {
    const char* testName = "color_source_over_no_global";
    const csColorRGBA dst{0, 100, 200, 255};
    const csColorRGBA src{255, 0, 0, 128};
    const csColorRGBA expected = refSourceOver(dst, src);
    const csColorRGBA actual = csColorRGBA::sourceOverStraight(dst, src);
    EXPECT_TRUE(colorEq(actual, expected.r, expected.g, expected.b, expected.a), "sourceOver without global alpha matches reference");
}

void test_matrix_ctor_and_clear(TestStats& stats) {
    const char* testName = "matrix_ctor_and_clear";
    csMatrixPixels m{3, 2};
    EXPECT_TRUE(m.width() == 3 && m.height() == 2, "width/height match ctor");
    EXPECT_TRUE(colorEq(m.getPixel(0, 0), 0, 0, 0, 0), "pixels start transparent");
    EXPECT_TRUE(colorEq(m.getPixel(2, 1), 0, 0, 0, 0), "last pixel transparent");
}

void test_matrix_set_get_in_bounds(TestStats& stats) {
    const char* testName = "matrix_set_get_in_bounds";
    csMatrixPixels m{2, 2};
    csColorRGBA c{10, 20, 30, 40};
    m.setPixel(1, 1, c);
    EXPECT_TRUE(colorEq(m.getPixel(1, 1), 10, 20, 30, 40), "setPixel stores value");
}

void test_matrix_out_of_bounds(TestStats& stats) {
    const char* testName = "matrix_out_of_bounds";
    csMatrixPixels m{2, 2};
    csColorRGBA c{1, 2, 3, 4};
    m.setPixel(-1, 0, c);
    m.setPixel(5, 5, c);
    EXPECT_TRUE(colorEq(m.getPixel(-1, 0), 0, 0, 0, 0), "getPixel negative returns transparent");
    EXPECT_TRUE(colorEq(m.getPixel(2, 0), 0, 0, 0, 0), "getPixel beyond width transparent");
    EXPECT_TRUE(colorEq(m.getPixel(0, 0), 0, 0, 0, 0), "in-bounds untouched");
}

void test_matrix_setPixelBlend(TestStats& stats) {
    const char* testName = "matrix_setPixelBlend";
    csMatrixPixels m{1, 1};
    const csColorRGBA dst{10, 20, 30, 40};
    const csColorRGBA src{200, 100, 50, 128};
    m.setPixel(0, 0, dst);
    m.setPixelBlend(0, 0, src);
    const csColorRGBA expected = csColorRGBA::sourceOverStraight(dst, src);
    EXPECT_TRUE(colorEq(m.getPixel(0, 0), expected.r, expected.g, expected.b, expected.a), "setPixelBlend matches SourceOver");
}

void test_matrix_setPixelBlend_with_global(TestStats& stats) {
    const char* testName = "matrix_setPixelBlend_with_global";
    csMatrixPixels m{1, 1};
    const csColorRGBA dst{0, 50, 100, 200};
    const csColorRGBA src{255, 0, 0, 128};
    m.setPixel(0, 0, dst);
    m.setPixelBlend(0, 0, src, 128);
    const csColorRGBA expected = csColorRGBA::sourceOverStraight(dst, src, 128);
    EXPECT_TRUE(colorEq(m.getPixel(0, 0), expected.r, expected.g, expected.b, expected.a), "setPixelBlend with global alpha matches SourceOver");
}

void test_matrix_getPixelBlend(TestStats& stats) {
    const char* testName = "matrix_getPixelBlend";
    csMatrixPixels m{1, 1};
    const csColorRGBA dst{0, 0, 255, 255};
    const csColorRGBA fg{255, 0, 0, 128};
    m.setPixel(0, 0, fg);
    const csColorRGBA blended = m.getPixelBlend(0, 0, dst);
    const csColorRGBA expected = csColorRGBA::sourceOverStraight(dst, fg);
    EXPECT_TRUE(colorEq(blended, expected.r, expected.g, expected.b, expected.a), "getPixelBlend returns SourceOver without mutating");
    EXPECT_TRUE(colorEq(m.getPixel(0, 0), fg.r, fg.g, fg.b, fg.a), "getPixelBlend does not modify matrix");
}

void test_matrix_drawMatrix_clip(TestStats& stats) {
    const char* testName = "matrix_drawMatrix_clip";
    csMatrixPixels dst{3, 3};
    csMatrixPixels src{2, 2};
    src.setPixel(0, 0, csColorRGBA{255, 0, 0, 255});     // not drawn (clipped)
    src.setPixel(1, 1, csColorRGBA{0, 255, 0, 255});     // drawn to (0,0)
    dst.drawMatrix(-1, -1, src, 128);
    const csColorRGBA expected = csColorRGBA::sourceOverStraight(csColorRGBA{0, 0, 0, 0}, src.getPixel(1, 1), 128);
    EXPECT_TRUE(colorEq(dst.getPixel(0, 0), expected.r, expected.g, expected.b, expected.a), "clipped draw writes only overlapping pixel");
    EXPECT_TRUE(colorEq(dst.getPixel(1, 1), 0, 0, 0, 0), "non-overlapping stays clear");
}

void test_matrix_drawMatrix_basic(TestStats& stats) {
    const char* testName = "matrix_drawMatrix_basic";
    csMatrixPixels dst{2, 2};
    csMatrixPixels src{2, 2};
    src.setPixel(0, 0, csColorRGBA{0, 0, 255, 128});
    src.setPixel(1, 0, csColorRGBA{255, 0, 0, 255});
    dst.setPixel(0, 0, csColorRGBA{0, 255, 0, 255});
    dst.drawMatrix(0, 0, src, 200);
    csColorRGBA expected00 = csColorRGBA::sourceOverStraight(csColorRGBA{0, 255, 0, 255}, src.getPixel(0, 0), 200);
    csColorRGBA expected10 = csColorRGBA::sourceOverStraight(csColorRGBA{0, 0, 0, 0}, src.getPixel(1, 0), 200);
    EXPECT_TRUE(colorEq(dst.getPixel(0, 0), expected00.r, expected00.g, expected00.b, expected00.a), "drawMatrix blends with dst");
    EXPECT_TRUE(colorEq(dst.getPixel(1, 0), expected10.r, expected10.g, expected10.b, expected10.a), "drawMatrix fills empty dst");
}

int main() {
    TestStats stats;
    test_color_component_ctor(stats);
    test_color_packed_alpha_promote(stats);
    test_color_packed_preserve(stats);
    test_color_divide(stats);
    test_color_blend_ops(stats);
    test_color_source_over_global_alpha(stats);
    test_color_source_over_no_global(stats);

    test_matrix_ctor_and_clear(stats);
    test_matrix_set_get_in_bounds(stats);
    test_matrix_out_of_bounds(stats);
    test_matrix_setPixelBlend(stats);
    test_matrix_setPixelBlend_with_global(stats);
    test_matrix_getPixelBlend(stats);
    test_matrix_drawMatrix_clip(stats);
    test_matrix_drawMatrix_basic(stats);

    std::cout << "Passed: " << stats.passed << ", Failed: " << stats.failed << '\n';
    if (stats.failed != 0) {
        std::cerr << "Some tests failed\n";
    }
    // Example build: g++ -std=c++17 -O2 -Wall -Wextra -o pixel_matrix_tests tests/pixel_matrix_tests.cpp
    return stats.failed == 0 ? 0 : 1;
}

