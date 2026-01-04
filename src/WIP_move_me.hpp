// HSV to RGB conversion - Optimized integer-only algorithm.
// Converts Hue (0-254), Saturation (0-254), Value/Brightness (0-255) to RGB (0-255)
// Uses 6 color regions instead of floating point calculations for maximum efficiency
//
// Algorithm:
// 1. Divide hue into 6 regions (0-5), each 43 units wide (255/6 ≈ 42.5)
// 2. Calculate remainder within region (0-252) and scale to 0-255 range (*6/43)
// 3. Compute p,q,t values using fixed-point arithmetic (>>8 = divide by 256)
// 4. Select RGB values based on region using standard HSV->RGB mapping
void hsvToRgb(uint8_t h, uint8_t s, uint8_t v, uint8_t *r, uint8_t *g, uint8_t *b) {
    // Determine which of 6 color regions the hue falls into (0-5)
    uint8_t region = h / 43;  // 255/6 ≈ 42.5, so regions are 0-42, 43-85, etc.

    // Calculate position within region (0-42) and scale to 0-255 range
    uint8_t remainder = (h - (region * 43)) * 6;

    // Calculate intermediate brightness values using fixed-point multiplication.
    // `p = v * (1-s) = v * (255-s) / 255`, using >>8 for /256 approximation.
    // In HSV->RGB algorithm, these represent different brightness levels for color transitions:
    // `p = value * (1-saturation)` - darkest point (minimum brightness)
    // `q = value * (1-saturation * fraction)` - medium brightness for upward transitions
    // `t = value * (1-saturation * (1-fraction))` - medium brightness for downward transitions
    // Using >>8 for /256 approximation instead of floating point division
    uint8_t p = (v * (255 - s)) >> 8;
    uint8_t q = (v * (255 - ((s * remainder) >> 8))) >> 8;
    uint8_t t = (v * (255 - ((s * (255 - remainder)) >> 8))) >> 8;

    // Map to RGB based on color region (following HSV color wheel)
    switch (region) {
        case 0: *r = v; *g = t; *b = p; break;  // Red to Yellow
        case 1: *r = q; *g = v; *b = p; break;  // Yellow to Green
        case 2: *r = p; *g = v; *b = t; break;  // Green to Cyan
        case 3: *r = p; *g = q; *b = v; break;  // Cyan to Blue
        case 4: *r = t; *g = p; *b = v; break;  // Blue to Magenta
        default: *r = v; *g = p; *b = q; break; // Magenta to Red
    }
}
