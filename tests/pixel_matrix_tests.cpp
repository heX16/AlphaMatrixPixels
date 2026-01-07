#include <iostream>
#include <cmath>
#include "../src/matrix_pixels.hpp"

using amp::csColorRGBA;
using amp::csMatrixPixels;
using namespace amp::math;

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

inline void dumpFp(const char* name, csFP16 v) {
    std::cerr << "  " << name << ": raw=" << static_cast<int>(v.raw_value())
              << " float=" << v.to_float() << "\n";
}

inline void dumpFp(const char* name, csFP32 v) {
    std::cerr << "  " << name << ": raw=" << static_cast<int32_t>(v.raw_value())
              << " float=" << v.to_float() << "\n";
}

inline bool colorEq(const csColorRGBA& c, uint8_t a, uint8_t r, uint8_t g, uint8_t b) {
    return c.a == a && c.r == r && c.g == g && c.b == b;
}

inline bool colorRGBNear(const csColorRGBA& c1, const csColorRGBA& c2, uint8_t tolerance = 1) {
    return std::abs(static_cast<int>(c1.r) - static_cast<int>(c2.r)) <= tolerance &&
           std::abs(static_cast<int>(c1.g) - static_cast<int>(c2.g)) <= tolerance &&
           std::abs(static_cast<int>(c1.b) - static_cast<int>(c2.b)) <= tolerance;
}

inline void expect_colorRGBMatches(TestStats& stats, const char* testName, int line,
                                   const csColorRGBA& pixel, const csColorRGBA& expectedColor,
                                   uint8_t tolerance, const char* pixelName) {
    const bool isNear = std::abs(static_cast<int>(pixel.r) - static_cast<int>(expectedColor.r)) <= tolerance &&
                        std::abs(static_cast<int>(pixel.g) - static_cast<int>(expectedColor.g)) <= tolerance &&
                        std::abs(static_cast<int>(pixel.b) - static_cast<int>(expectedColor.b)) <= tolerance;
    if (isNear) {
        ++stats.passed;
        return;
    }
    
    ++stats.failed;
    std::cerr << "FAIL [" << testName << "] " << pixelName << " RGB does not match source color (line " << line << ")\n";
    std::cerr << "  pixel:    A=" << static_cast<int>(pixel.a) << " R=" << static_cast<int>(pixel.r) 
              << " G=" << static_cast<int>(pixel.g) << " B=" << static_cast<int>(pixel.b) << "\n";
    std::cerr << "  expected: R=" << static_cast<int>(expectedColor.r) 
              << " G=" << static_cast<int>(expectedColor.g) << " B=" << static_cast<int>(expectedColor.b) << "\n";
    std::cerr << "  diff:     R=" << std::abs(static_cast<int>(pixel.r) - static_cast<int>(expectedColor.r))
              << " G=" << std::abs(static_cast<int>(pixel.g) - static_cast<int>(expectedColor.g))
              << " B=" << std::abs(static_cast<int>(pixel.b) - static_cast<int>(expectedColor.b))
              << " (tolerance=" << static_cast<int>(tolerance) << ")\n";
}

void test_color_component_ctor(TestStats& stats) {
    const char* testName = "color_component_ctor";
    csColorRGBA c{40, 10, 20, 30};
    expect_true(stats, testName, __LINE__, colorEq(c, 40, 10, 20, 30), "component constructor keeps channels");
}

void test_color_rgb_ctor(TestStats& stats) {
    const char* testName = "color_rgb_ctor";
    csColorRGBA c{10, 20, 30}; // r,g,b
    expect_true(stats, testName, __LINE__, colorEq(c, 0xFF, 10, 20, 30), "RGB ctor forces alpha to 0xFF");
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
    m.setPixelRewrite(1, 1, c);
    expect_true(stats, testName, __LINE__, colorEq(m.getPixel(1, 1), 40, 10, 20, 30), "setPixelRewrite stores value");
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
    m.setPixelRewrite(0, 0, dst);
    m.setPixel(0, 0, src);
    const csColorRGBA expected = csColorRGBA::sourceOverStraight(dst, src);
    expect_true(stats, testName, __LINE__, colorEq(m.getPixel(0, 0), expected.a, expected.r, expected.g, expected.b), "setPixel matches SourceOver");
}

void test_matrix_setPixelBlend_with_global(TestStats& stats) {
    const char* testName = "matrix_setPixelBlend_with_global";
    csMatrixPixels m{1, 1};
    const csColorRGBA dst{200, 0, 50, 100};
    const csColorRGBA src{128, 255, 0, 0};
    m.setPixelRewrite(0, 0, dst);
    m.setPixel(0, 0, src, 128);
    const csColorRGBA expected = csColorRGBA::sourceOverStraight(dst, src, 128);
    expect_true(stats, testName, __LINE__, colorEq(m.getPixel(0, 0), expected.a, expected.r, expected.g, expected.b), "setPixel with global alpha matches SourceOver");
}

void test_matrix_getPixelBlend(TestStats& stats) {
    const char* testName = "matrix_getPixelBlend";
    csMatrixPixels m{1, 1};
    const csColorRGBA dst{255, 0, 0, 255};
    const csColorRGBA fg{128, 255, 0, 0};
    m.setPixelRewrite(0, 0, fg);
    const csColorRGBA blended = m.getPixelBlend(0, 0, dst);
    const csColorRGBA expected = csColorRGBA::sourceOverStraight(dst, fg);
    expect_true(stats, testName, __LINE__, colorEq(blended, expected.a, expected.r, expected.g, expected.b), "getPixelBlend returns SourceOver without mutating");
    expect_true(stats, testName, __LINE__, colorEq(m.getPixel(0, 0), fg.a, fg.r, fg.g, fg.b), "getPixelBlend does not modify matrix");
}

void test_matrix_drawMatrix_clip(TestStats& stats) {
    const char* testName = "matrix_drawMatrix_clip";
    csMatrixPixels dst{3, 3};
    csMatrixPixels src{2, 2};
    src.setPixelRewrite(0, 0, csColorRGBA{255, 255, 0, 0});     // not drawn (clipped)
    src.setPixelRewrite(1, 1, csColorRGBA{255, 0, 255, 0});     // drawn to (0,0)
    dst.drawMatrix(-1, -1, src, 128);
    const csColorRGBA expected = csColorRGBA::sourceOverStraight(csColorRGBA{0, 0, 0, 0}, src.getPixel(1, 1), 128);
    expect_true(stats, testName, __LINE__, colorEq(dst.getPixel(0, 0), expected.a, expected.r, expected.g, expected.b), "clipped draw writes only overlapping pixel");
    expect_true(stats, testName, __LINE__, colorEq(dst.getPixel(1, 1), 0, 0, 0, 0), "non-overlapping stays clear");
}

void test_matrix_drawMatrix_basic(TestStats& stats) {
    const char* testName = "matrix_drawMatrix_basic";
    csMatrixPixels dst{2, 2};
    csMatrixPixels src{2, 2};
    src.setPixelRewrite(0, 0, csColorRGBA{128, 0, 0, 255});
    src.setPixelRewrite(1, 0, csColorRGBA{255, 255, 0, 0});
    dst.setPixelRewrite(0, 0, csColorRGBA{255, 0, 255, 0});
    dst.drawMatrix(0, 0, src, 200);
    csColorRGBA expected00 = csColorRGBA::sourceOverStraight(csColorRGBA{255, 0, 255, 0}, src.getPixel(0, 0), 200);
    csColorRGBA expected10 = csColorRGBA::sourceOverStraight(csColorRGBA{0, 0, 0, 0}, src.getPixel(1, 0), 200);
    expect_true(stats, testName, __LINE__, colorEq(dst.getPixel(0, 0), expected00.a, expected00.r, expected00.g, expected00.b), "drawMatrix blends with dst");
    expect_true(stats, testName, __LINE__, colorEq(dst.getPixel(1, 0), expected10.a, expected10.r, expected10.g, expected10.b), "drawMatrix fills empty dst");
}

void test_fp16_basic(TestStats& stats) {
    using namespace amp::math;
    const char* testName = "fp16_basic";
    // Note: csFP16 now uses 4 fractional bits (scale=16), so precision is lower
    const csFP16 a{1.5f};
    const csFP16 b{-0.25f};
    expect_near_float(stats, testName, __LINE__, a.to_float(), 1.5f, 0.1f, "fp16 to_float close to 1.5");
    expect_near_float(stats, testName, __LINE__, b.to_float(), -0.25f, 0.1f, "fp16 to_float close to -0.25");
    const csFP16 c = a + b; // 1.25
    expect_near_float(stats, testName, __LINE__, c.to_float(), 1.25f, 0.15f, "fp16 add works");
    const csFP16 d = a * b; // -0.375
    expect_near_float(stats, testName, __LINE__, d.to_float(), -0.375f, 0.15f, "fp16 mul works");
    const csFP16 e = csFP16::from_int(2) / csFP16::from_int(4); // 0.5
    expect_near_float(stats, testName, __LINE__, e.to_float(), 0.5f, 0.15f, "fp16 div works");
}

void test_fp32_basic(TestStats& stats) {
    using namespace amp::math;
    const char* testName = "fp32_basic";
    const csFP32 a{3.25f};
    const csFP32 b{0.5f};
    expect_near_float(stats, testName, __LINE__, a.to_float(), 3.25f, 0.001f, "fp32 to_float close to 3.25");
    const csFP32 c = a - b; // 2.75
    expect_near_float(stats, testName, __LINE__, c.to_float(), 2.75f, 0.001f, "fp32 sub works");
    const csFP32 d = a * b; // 1.625
    expect_near_float(stats, testName, __LINE__, d.to_float(), 1.625f, 0.001f, "fp32 mul works");
    const csFP32 e = csFP32::from_int(1) / csFP32::from_int(2); // 0.5
    expect_near_float(stats, testName, __LINE__, e.to_float(), 0.5f, 0.001f, "fp32 div works");
}

void test_fp_trig(TestStats& stats) {
    using namespace amp::math;
    const char* testName = "fp_trig";
    const csFP32 zero{0};
    const csFP32 pi_over2{1.57079632679f}; // ~pi/2
    const csFP32 one{1};
    const csFP32 s0 = fp32_sin(zero);
    const csFP32 c0 = fp32_cos(zero);
    const csFP32 s1 = fp32_sin(pi_over2);
    const csFP32 c1 = fp32_cos(pi_over2);
    expect_near_float(stats, testName, __LINE__, s0.to_float(), 0.0f, 0.001f, "sin(0) ~ 0");
    expect_near_float(stats, testName, __LINE__, c0.to_float(), 1.0f, 0.001f, "cos(0) ~ 1");
    expect_near_float(stats, testName, __LINE__, s1.to_float(), 1.0f, 0.01f, "sin(pi/2) ~ 1");
    expect_near_float(stats, testName, __LINE__, c1.to_float(), 0.0f, 0.05f, "cos(pi/2) ~ 0");
    expect_eq_int(
        stats, testName, __LINE__,
        static_cast<long long>(csFP32::from_int(1).raw_value()),
        static_cast<long long>(one.raw_value()),
        "fp32 comparison works (raw)"
    );
}

void test_setPixelFloat_exact_center(TestStats& stats) {
    const char* testName = "setPixelFloat_exact_center";
    csMatrixPixels m{5, 5};
    const csColorRGBA color{255, 100, 200, 50};
    // Exact center at (2, 2) - should draw single pixel with full alpha
    m.setPixelFloat(csFP16::from_int(2), csFP16::from_int(2), color);
    expect_true(stats, testName, __LINE__, colorEq(m.getPixel(2, 2), 255, 100, 200, 50), "exact center draws single pixel with full alpha");
    expect_true(stats, testName, __LINE__, colorEq(m.getPixel(2, 1), 0, 0, 0, 0), "neighbor pixel stays clear");
    expect_true(stats, testName, __LINE__, colorEq(m.getPixel(2, 3), 0, 0, 0, 0), "neighbor pixel stays clear");
}

void test_setPixelFloat_offset_vertical_down(TestStats& stats) {
    const char* testName = "setPixelFloat_offset_vertical_down";
    csMatrixPixels m{5, 5};
    const csColorRGBA color{255, 100, 200, 50};
    // Offset (0, +0.5) - should split between (2,2) and (2,3)
    // max_offset_raw = 8 (0.5 * 16), weight = (8 * 255 + 8) / 16 = 128
    m.setPixelFloat(csFP16::from_int(2), csFP16{2.5f}, color);
    const csColorRGBA center = m.getPixel(2, 2);
    const csColorRGBA secondary = m.getPixel(2, 3);
    expect_true(stats, testName, __LINE__, center.a > 0 && secondary.a > 0, "both pixels have alpha");
    expect_true(stats, testName, __LINE__, center.a + secondary.a == 255, "alpha sums to full");
    // Both pixels should have RGB close to source color (may differ slightly due to blending)
    expect_colorRGBMatches(stats, testName, __LINE__, center, color, 2, "center pixel");
    expect_colorRGBMatches(stats, testName, __LINE__, secondary, color, 2, "secondary pixel");
}

void test_setPixelFloat_offset_vertical_up(TestStats& stats) {
    const char* testName = "setPixelFloat_offset_vertical_up";
    csMatrixPixels m{5, 5};
    const csColorRGBA color{255, 100, 200, 50};
    // Offset (0, -0.5) - should split between (2,2) and (2,1)
    m.setPixelFloat(csFP16::from_int(2), csFP16{1.5f}, color);
    const csColorRGBA center = m.getPixel(2, 2);
    const csColorRGBA secondary = m.getPixel(2, 1);
    expect_true(stats, testName, __LINE__, center.a > 0 && secondary.a > 0, "both pixels have alpha");
    expect_true(stats, testName, __LINE__, center.a + secondary.a == 255, "alpha sums to full");
    // Both pixels should have RGB close to source color (may differ slightly due to blending)
    expect_colorRGBMatches(stats, testName, __LINE__, center, color, 2, "center pixel");
    expect_colorRGBMatches(stats, testName, __LINE__, secondary, color, 2, "secondary pixel");
}

void test_setPixelFloat_offset_diagonal(TestStats& stats) {
    const char* testName = "setPixelFloat_offset_diagonal";
    csMatrixPixels m{5, 5};
    const csColorRGBA color{255, 100, 200, 50};
    // Offset (+0.5, +0.5) - equal components, should use diagonal direction
    m.setPixelFloat(csFP16{2.5f}, csFP16{2.5f}, color);
    const csColorRGBA center = m.getPixel(2, 2);
    const csColorRGBA secondary = m.getPixel(3, 3);
    expect_true(stats, testName, __LINE__, center.a > 0 && secondary.a > 0, "both pixels have alpha");
    expect_true(stats, testName, __LINE__, center.a + secondary.a == 255, "alpha sums to full");
    // Both pixels should have RGB close to source color (may differ slightly due to blending)
    expect_colorRGBMatches(stats, testName, __LINE__, center, color, 2, "center pixel");
    expect_colorRGBMatches(stats, testName, __LINE__, secondary, color, 2, "secondary pixel");
}

void test_setPixelFloat_offset_horizontal(TestStats& stats) {
    const char* testName = "setPixelFloat_offset_horizontal";
    csMatrixPixels m{5, 5};
    const csColorRGBA color{255, 100, 200, 50};
    // Offset (+0.5, 0) - should split between (2,2) and (3,2)
    m.setPixelFloat(csFP16{2.5f}, csFP16::from_int(2), color);
    const csColorRGBA center = m.getPixel(2, 2);
    const csColorRGBA secondary = m.getPixel(3, 2);
    expect_true(stats, testName, __LINE__, center.a > 0 && secondary.a > 0, "both pixels have alpha");
    expect_true(stats, testName, __LINE__, center.a + secondary.a == 255, "alpha sums to full");
    // Both pixels should have RGB close to source color (may differ slightly due to blending)
    expect_colorRGBMatches(stats, testName, __LINE__, center, color, 2, "center pixel");
    expect_colorRGBMatches(stats, testName, __LINE__, secondary, color, 2, "secondary pixel");
}

void test_setPixelFloat_large_offset(TestStats& stats) {
    const char* testName = "setPixelFloat_large_offset";
    csMatrixPixels m{5, 5};
    const csColorRGBA color{255, 100, 200, 50};
    // Large offset (+0.75, +0.25) - should favor horizontal direction (dx > dy)
    m.setPixelFloat(csFP16{2.75f}, csFP16{2.25f}, color);
    const csColorRGBA center = m.getPixel(2, 2);
    const csColorRGBA secondary = m.getPixel(3, 2); // horizontal, not vertical
    expect_true(stats, testName, __LINE__, center.a > 0 && secondary.a > 0, "both pixels have alpha");
    expect_true(stats, testName, __LINE__, center.a + secondary.a == 255, "alpha sums to full");
    expect_true(stats, testName, __LINE__, colorEq(m.getPixel(2, 3), 0, 0, 0, 0), "vertical pixel stays clear (horizontal chosen)");
    // Both pixels should have RGB close to source color (may differ slightly due to blending)
    if (center.a > 0) {
        expect_colorRGBMatches(stats, testName, __LINE__, center, color, 2, "center pixel");
    }
    if (secondary.a > 0) {
        expect_colorRGBMatches(stats, testName, __LINE__, secondary, color, 2, "secondary pixel");
    }
}

void test_setPixelFloat_out_of_bounds(TestStats& stats) {
    const char* testName = "setPixelFloat_out_of_bounds";
    csMatrixPixels m{3, 3};
    const csColorRGBA color{255, 100, 200, 50};
    // Try to draw outside bounds - should be silently ignored
    m.setPixelFloat(csFP16{-0.5f}, csFP16{1.5f}, color);
    m.setPixelFloat(csFP16{5.5f}, csFP16{1.5f}, color);
    expect_true(stats, testName, __LINE__, colorEq(m.getPixel(0, 0), 0, 0, 0, 0), "out of bounds pixel stays clear");
    expect_true(stats, testName, __LINE__, colorEq(m.getPixel(2, 1), 0, 0, 0, 0), "out of bounds pixel stays clear");
}

int main() {
    TestStats stats;
    test_color_component_ctor(stats);
    test_color_rgb_ctor(stats);
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

    test_setPixelFloat_exact_center(stats);
    test_setPixelFloat_offset_vertical_down(stats);
    test_setPixelFloat_offset_vertical_up(stats);
    test_setPixelFloat_offset_diagonal(stats);
    test_setPixelFloat_offset_horizontal(stats);
    test_setPixelFloat_large_offset(stats);
    test_setPixelFloat_out_of_bounds(stats);

    std::cout << "Passed: " << stats.passed << ", Failed: " << stats.failed << '\n';
    if (stats.failed != 0) {
        std::cerr << "Some tests failed\n";
    }
    // Example build: g++ -std=c++17 -O2 -Wall -Wextra -o pixel_matrix_tests tests/pixel_matrix_tests.cpp
    return stats.failed == 0 ? 0 : 1;
}

