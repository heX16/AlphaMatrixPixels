#pragma once

#include <SDL2/SDL.h>
#include "matrix_pixels.hpp"
#include "color_rgba.hpp"
#include "math.hpp"

namespace amp {

// Layout structure for calculating pixel grid positioning on screen.
struct RenderLayout {
    int step;   // distance between cells (grid pitch)
    int fill;   // size of the filled square
    int offsX;  // top-left offset to center grid horizontally
    int offsY;  // top-left offset to center grid vertically
};

// Calculate layout for rendering matrix on screen with centering and padding.
// 
// Parameters:
//   matrix - source pixel matrix
//   screenWidth - width of the screen/logical renderer size
//   screenHeight - height of the screen/logical renderer size
//
// Returns RenderLayout with calculated step, fill, and offsets for centering.
[[nodiscard]] inline RenderLayout calculateLayout(
    const csMatrixPixels& matrix,
    int screenWidth,
    int screenHeight
) noexcept {
    // Keep a small padding; fit grid into logical renderer size.
    constexpr int padding = 10;
    const int matrixW = static_cast<int>(matrix.width());
    const int matrixH = static_cast<int>(matrix.height());

    const int availW = math::max(screenWidth - padding * 2, 1);
    const int availH = math::max(screenHeight - padding * 2, 1);

    const int step = math::max(1, math::min(availW / matrixW, availH / matrixH));
    const int fill = math::max(1, step - math::max(2, step / 4)); // leave a border

    const int totalW = step * matrixW;
    const int totalH = step * matrixH;
    const int offsX = (screenWidth - totalW) / 2;
    const int offsY = (screenHeight - totalH) / 2;

    return RenderLayout{step, fill, offsX, offsY};
}

// Draw a single pixel cell on SDL renderer with border and fill.
//
// Parameters:
//   renderer - SDL renderer (must be valid)
//   x, y - screen coordinates for top-left corner of the cell
//   step - size of the cell (including border)
//   fill - size of the filled square inside the cell
//   c - color to fill the cell with
inline void drawPixel(
    SDL_Renderer* renderer,
    int x,
    int y,
    int step,
    int fill,
    csColorRGBA c
) noexcept {
    if (!renderer) {
        return;
    }

    // Draw gray border
    SDL_SetRenderDrawColor(renderer, 128, 128, 128, SDL_ALPHA_OPAQUE);
    SDL_Rect rectBorder = {x, y, step - 1, step - 1};
    SDL_RenderDrawRect(renderer, &rectBorder);

    // Draw colored fill
    SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, SDL_ALPHA_OPAQUE);
    SDL_Rect rectFill = {x + (step - fill) / 2,
                         y + (step - fill) / 2,
                         fill,
                         fill};
    SDL_RenderFillRect(renderer, &rectFill);
}

// Render matrix pixels to SDL renderer with automatic layout calculation and centering.
//
// Parameters:
//   matrix - source pixel matrix
//   renderer - SDL renderer (must be initialized)
//   screenWidth - width of the screen/logical renderer size
//   screenHeight - height of the screen/logical renderer size
//   clearBeforeRender - if true, clear the screen with black before rendering (default: true)
//   presentAfterRender - if true, call SDL_RenderPresent after rendering (default: false)
//
// Note: renderer must be valid and initialized. This function does not initialize SDL.
inline void renderMatrixToSDL(
    const csMatrixPixels& matrix,
    SDL_Renderer* renderer,
    int screenWidth,
    int screenHeight,
    bool clearBeforeRender = true,
    bool presentAfterRender = false
) noexcept {
    if (!renderer) {
        return;
    }

    // Calculate layout for centering and scaling
    const RenderLayout layout = calculateLayout(matrix, screenWidth, screenHeight);

    // Clear screen if requested
    if (clearBeforeRender) {
        SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, SDL_ALPHA_OPAQUE);
        SDL_RenderClear(renderer);
    }

    // Render all pixels
    const int matrixW = static_cast<int>(matrix.width());
    const int matrixH = static_cast<int>(matrix.height());
    for (int x = 0; x < matrixW; ++x) {
        for (int y = 0; y < matrixH; ++y) {
            const csColorRGBA c = matrix.getPixel(x, y);
            drawPixel(renderer,
                      layout.offsX + x * layout.step,
                      layout.offsY + y * layout.step,
                      layout.step,
                      layout.fill,
                      c);
        }
    }

    // Present renderer if requested
    if (presentAfterRender) {
        SDL_RenderPresent(renderer);
    }
}

} // namespace amp

