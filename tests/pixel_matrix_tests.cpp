#include <iostream>
#include <cmath>
#include <string>
#include "matrix_pixels.hpp"
#include "cs_math.hpp"

// Minimal self-contained test runner (no external frameworks).
struct TestStats {
    int passed{0};
    int failed{0};
};

inline void expect_true(TestStats& stats, const char* testName, int line, bool cond, const char* msg) {
    if (cond) {
        ++stats.passed;
        return;
    }

    ++stats.failed;
    std::cerr << "FAIL [" << testName << "] " << msg << " (line " << line << ")\n";
}

inline void expect_eq_int(TestStats& stats, const char* testName, int line, long long actual, long long expected, const char* msg) {
    if (actual == expected) {
        ++stats.passed;
        return;
    }

    ++stats.failed;
    std::cerr << "FAIL [" << testName << "] " << msg << " (line " << line << ")\n";
    std::cerr << "  expected: " << expected << "\n";
    std::cerr << "  actual:   " << actual << "\n";
}

inline void expect_near_float(TestStats& stats, const char* testName, int line, float actual, float expected, float eps, const char* msg) {
    const float d = std::fabs(actual - expected);
    if (d <= eps) {
        ++stats.passed;
        return;
    }

    ++stats.failed;
    std::cerr << "FAIL [" << testName << "] " << msg << " (line " << line << ")\n";
    std::cerr << "  expected: " << expected << "\n";
    std::cerr << "  actual:   " << actual << "\n";
    std::cerr << "  diff:     " << d << "\n";
}

inline void dumpFp(const char* name, matrix_pixels_math::fp16_t v) {
    std::cerr << "  " << name << ": raw=" << static_cast<int>(v.raw_value())
              << " float=" << v.to_float() << "\n";
}

inline void dumpFp(const char* name, matrix_pixels_math::fp32_t v) {
    std::cerr << "  " << name << ": raw=" << static_cast<int32_t>(v.raw_value())
              << " float=" << v.to_float() << "\n";
}

inline bool colorEq(const csColorRGBA& c, uint8_t a, uint8_t r, uint8_t g, uint8_t b) {
    return c.a == a && c.r == r && c.g == g && c.b == b;
}

void test_color_component_ctor(TestStats& stats) {
    const char* testName = "color_component_ctor";
    csColorRGBA c{40, 10, 20, 30};
    expect_true(stats, testName, __LINE__, colorEq(c, 40, 10, 20, 30), "component constructor keeps channels");
}

void test_color_packed_alpha_promote(TestStats& stats) {
    const char* testName = "color_packed_alpha_promote";
    // Packed as 0xAARRGGBB; here AA==0 -> should be promoted to 0xFF.
    csColorRGBA c{0x00010203u}; // A=0x00, R=0x01, G=0x02, B=0x03
    expect_true(stats, testName, __LINE__, colorEq(c, 0xFF, 0x01, 0x02, 0x03), "alpha promoted when zero (ARGB)");
}

void test_color_packed_preserve(TestStats& stats) {
    const char* testName = "color_packed_preserve";
    csColorRGBA c{0x80010203u}; // A=0x80, R=0x01, G=0x02, B=0x03
    expect_true(stats, testName, __LINE__, colorEq(c, 0x80, 0x01, 0x02, 0x03), "packed alpha preserved when non-zero (ARGB)");
}

void test_color_divide(TestStats& stats) {
    const char* testName = "color_divide";
    csColorRGBA c{255, 200, 100, 50};
    c /= 2;
    expect_true(stats, testName, __LINE__, colorEq(c, 127, 100, 50, 25), "division applies to all channels");
}

void test_color_blend_ops(TestStats& stats) {
    const char* testName = "color_blend_ops";
    csColorRGBA dst{80, 50, 60, 70};
    csColorRGBA src{255, 200, 10, 20};
    auto sum = dst + src;
    expect_true(stats, testName, __LINE__, colorEq(sum, src.a, src.r, src.g, src.b), "operator+ with opaque src returns src");
    dst += src;
    expect_true(stats, testName, __LINE__, colorEq(dst, src.a, src.r, src.g, src.b), "operator+= mutates to src when opaque");
}

void test_color_source_over_global_alpha(TestStats& stats) {
    const char* testName = "color_source_over_global_alpha";
    const csColorRGBA dst{120, 60, 80, 100};
    const csColorRGBA src{128, 200, 40, 20};
    const uint8_t global = 128;
    // Expected manually computed: A=154, R=118, G=63, B=66.
    const csColorRGBA expected{154, 118, 63, 66};
    const csColorRGBA actual = csColorRGBA::sourceOverStraight(dst, src, global);
    expect_true(stats, testName, __LINE__, colorEq(actual, expected.a, expected.r, expected.g, expected.b), "sourceOver with global alpha matches reference");
}

void test_color_source_over_no_global(TestStats& stats) {
    const char* testName = "color_source_over_no_global";
    const csColorRGBA dst{255, 0, 100, 200};
    const csColorRGBA src{128, 255, 0, 0};
    // Expected manually computed: A=255, R=128, G=50, B=100.
    const csColorRGBA expected{255, 128, 50, 100};
    const csColorRGBA actual = csColorRGBA::sourceOverStraight(dst, src);
    expect_true(stats, testName, __LINE__, colorEq(actual, expected.a, expected.r, expected.g, expected.b), "sourceOver without global alpha matches reference");
}

void test_matrix_ctor_and_clear(TestStats& stats) {
    const char* testName = "matrix_ctor_and_clear";
    csMatrixPixels m{3, 2};
    expect_true(stats, testName, __LINE__, m.width() == 3 && m.height() == 2, "width/height match ctor");
    expect_true(stats, testName, __LINE__, colorEq(m.getPixel(0, 0), 0, 0, 0, 0), "pixels start transparent");
    expect_true(stats, testName, __LINE__, colorEq(m.getPixel(2, 1), 0, 0, 0, 0), "last pixel transparent");
}

void test_matrix_set_get_in_bounds(TestStats& stats) {
    const char* testName = "matrix_set_get_in_bounds";
    csMatrixPixels m{2, 2};
    csColorRGBA c{40, 10, 20, 30};
    m.setPixel(1, 1, c);
    expect_true(stats, testName, __LINE__, colorEq(m.getPixel(1, 1), 40, 10, 20, 30), "setPixel stores value");
}

void test_matrix_out_of_bounds(TestStats& stats) {
    const char* testName = "matrix_out_of_bounds";
    csMatrixPixels m{2, 2};
    csColorRGBA c{4, 1, 2, 3};
    m.setPixel(-1, 0, c);
    m.setPixel(5, 5, c);
    expect_true(stats, testName, __LINE__, colorEq(m.getPixel(-1, 0), 0, 0, 0, 0), "getPixel negative returns transparent");
    expect_true(stats, testName, __LINE__, colorEq(m.getPixel(2, 0), 0, 0, 0, 0), "getPixel beyond width transparent");
    expect_true(stats, testName, __LINE__, colorEq(m.getPixel(0, 0), 0, 0, 0, 0), "in-bounds untouched");
}

void test_matrix_setPixelBlend(TestStats& stats) {
    const char* testName = "matrix_setPixelBlend";
    csMatrixPixels m{1, 1};
    const csColorRGBA dst{40, 10, 20, 30};
    const csColorRGBA src{128, 200, 100, 50};
    m.setPixel(0, 0, dst);
    m.setPixelBlend(0, 0, src);
    const csColorRGBA expected = csColorRGBA::sourceOverStraight(dst, src);
    expect_true(stats, testName, __LINE__, colorEq(m.getPixel(0, 0), expected.a, expected.r, expected.g, expected.b), "setPixelBlend matches SourceOver");
}

void test_matrix_setPixelBlend_with_global(TestStats& stats) {
    const char* testName = "matrix_setPixelBlend_with_global";
    csMatrixPixels m{1, 1};
    const csColorRGBA dst{200, 0, 50, 100};
    const csColorRGBA src{128, 255, 0, 0};
    m.setPixel(0, 0, dst);
    m.setPixelBlend(0, 0, src, 128);
    const csColorRGBA expected = csColorRGBA::sourceOverStraight(dst, src, 128);
    expect_true(stats, testName, __LINE__, colorEq(m.getPixel(0, 0), expected.a, expected.r, expected.g, expected.b), "setPixelBlend with global alpha matches SourceOver");
}

void test_matrix_getPixelBlend(TestStats& stats) {
    const char* testName = "matrix_getPixelBlend";
    csMatrixPixels m{1, 1};
    const csColorRGBA dst{255, 0, 0, 255};
    const csColorRGBA fg{128, 255, 0, 0};
    m.setPixel(0, 0, fg);
    const csColorRGBA blended = m.getPixelBlend(0, 0, dst);
    const csColorRGBA expected = csColorRGBA::sourceOverStraight(dst, fg);
    expect_true(stats, testName, __LINE__, colorEq(blended, expected.a, expected.r, expected.g, expected.b), "getPixelBlend returns SourceOver without mutating");
    expect_true(stats, testName, __LINE__, colorEq(m.getPixel(0, 0), fg.a, fg.r, fg.g, fg.b), "getPixelBlend does not modify matrix");
}

void test_matrix_drawMatrix_clip(TestStats& stats) {
    const char* testName = "matrix_drawMatrix_clip";
    csMatrixPixels dst{3, 3};
    csMatrixPixels src{2, 2};
    src.setPixel(0, 0, csColorRGBA{255, 255, 0, 0});     // not drawn (clipped)
    src.setPixel(1, 1, csColorRGBA{255, 0, 255, 0});     // drawn to (0,0)
    dst.drawMatrix(-1, -1, src, 128);
    const csColorRGBA expected = csColorRGBA::sourceOverStraight(csColorRGBA{0, 0, 0, 0}, src.getPixel(1, 1), 128);
    expect_true(stats, testName, __LINE__, colorEq(dst.getPixel(0, 0), expected.a, expected.r, expected.g, expected.b), "clipped draw writes only overlapping pixel");
    expect_true(stats, testName, __LINE__, colorEq(dst.getPixel(1, 1), 0, 0, 0, 0), "non-overlapping stays clear");
}

void test_matrix_drawMatrix_basic(TestStats& stats) {
    const char* testName = "matrix_drawMatrix_basic";
    csMatrixPixels dst{2, 2};
    csMatrixPixels src{2, 2};
    src.setPixel(0, 0, csColorRGBA{128, 0, 0, 255});
    src.setPixel(1, 0, csColorRGBA{255, 255, 0, 0});
    dst.setPixel(0, 0, csColorRGBA{255, 0, 255, 0});
    dst.drawMatrix(0, 0, src, 200);
    csColorRGBA expected00 = csColorRGBA::sourceOverStraight(csColorRGBA{255, 0, 255, 0}, src.getPixel(0, 0), 200);
    csColorRGBA expected10 = csColorRGBA::sourceOverStraight(csColorRGBA{0, 0, 0, 0}, src.getPixel(1, 0), 200);
    expect_true(stats, testName, __LINE__, colorEq(dst.getPixel(0, 0), expected00.a, expected00.r, expected00.g, expected00.b), "drawMatrix blends with dst");
    expect_true(stats, testName, __LINE__, colorEq(dst.getPixel(1, 0), expected10.a, expected10.r, expected10.g, expected10.b), "drawMatrix fills empty dst");
}

void test_fp16_basic(TestStats& stats) {
    using namespace matrix_pixels_math;
    const char* testName = "fp16_basic";
    const fp16_t a{1.5f};
    const fp16_t b{-0.25f};
    expect_near_float(stats, testName, __LINE__, a.to_float(), 1.5f, 0.01f, "fp16 to_float close to 1.5");
    expect_near_float(stats, testName, __LINE__, b.to_float(), -0.25f, 0.01f, "fp16 to_float close to -0.25");
    const fp16_t c = a + b; // 1.25
    expect_near_float(stats, testName, __LINE__, c.to_float(), 1.25f, 0.02f, "fp16 add works");
    const fp16_t d = a * b; // -0.375
    expect_near_float(stats, testName, __LINE__, d.to_float(), -0.375f, 0.02f, "fp16 mul works");
    const fp16_t e = fp16_t{2} / fp16_t{4}; // 0.5
    expect_near_float(stats, testName, __LINE__, e.to_float(), 0.5f, 0.02f, "fp16 div works");
}

void test_fp32_basic(TestStats& stats) {
    using namespace matrix_pixels_math;
    const char* testName = "fp32_basic";
    const fp32_t a{3.25f};
    const fp32_t b{0.5f};
    expect_near_float(stats, testName, __LINE__, a.to_float(), 3.25f, 0.001f, "fp32 to_float close to 3.25");
    const fp32_t c = a - b; // 2.75
    expect_near_float(stats, testName, __LINE__, c.to_float(), 2.75f, 0.001f, "fp32 sub works");
    const fp32_t d = a * b; // 1.625
    expect_near_float(stats, testName, __LINE__, d.to_float(), 1.625f, 0.001f, "fp32 mul works");
    const fp32_t e = fp32_t{1} / fp32_t{2}; // 0.5
    expect_near_float(stats, testName, __LINE__, e.to_float(), 0.5f, 0.001f, "fp32 div works");
}

void test_fp_trig(TestStats& stats) {
    using namespace matrix_pixels_math;
    const char* testName = "fp_trig";
    const fp32_t zero{0};
    const fp32_t pi_over2{1.57079632679f}; // ~pi/2
    const fp32_t one{1};
    const fp32_t s0 = fp32_sin(zero);
    const fp32_t c0 = fp32_cos(zero);
    const fp32_t s1 = fp32_sin(pi_over2);
    const fp32_t c1 = fp32_cos(pi_over2);
    expect_near_float(stats, testName, __LINE__, s0.to_float(), 0.0f, 0.001f, "sin(0) ~ 0");
    expect_near_float(stats, testName, __LINE__, c0.to_float(), 1.0f, 0.001f, "cos(0) ~ 1");
    expect_near_float(stats, testName, __LINE__, s1.to_float(), 1.0f, 0.01f, "sin(pi/2) ~ 1");
    expect_near_float(stats, testName, __LINE__, c1.to_float(), 0.0f, 0.05f, "cos(pi/2) ~ 0");
    expect_eq_int(
        stats, testName, __LINE__,
        static_cast<long long>(fp32_t{1}.raw_value()),
        static_cast<long long>(one.raw_value()),
        "fp32 comparison works (raw)"
    );
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

    test_fp16_basic(stats);
    test_fp32_basic(stats);
    test_fp_trig(stats);

    std::cout << "Passed: " << stats.passed << ", Failed: " << stats.failed << '\n';
    if (stats.failed != 0) {
        std::cerr << "Some tests failed\n";
    }
    // Example build: g++ -std=c++17 -O2 -Wall -Wextra -o pixel_matrix_tests tests/pixel_matrix_tests.cpp
    return stats.failed == 0 ? 0 : 1;
}

