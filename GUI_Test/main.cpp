// Minimal SDL-based viewer for matrix_pixels
#include <SDL2/SDL.h>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <memory>
#include "../matrix_pixels.hpp"
#include "../color_rgba.hpp"
#include "../matrix_effects.hpp"

// Screen dimension constants
constexpr int screenWidth  = 640;
constexpr int screenHeight = 480;

struct RenderLayout {
    int step;   // distance between cells (grid pitch)
    int fill;   // size of the filled square
    int offsX;  // top-left offset to center grid horizontally
    int offsY;  // top-left offset to center grid vertically
};

// Simple SDL wrapper to draw matrix_pixels with a time-based gradient
class csMainProgramSDL {
public:
    bool quit = false;

    SDL_Window* window{};
    SDL_Renderer* renderer{};
    SDL_Event event{};

    csMatrixPixels matrix{0, 0};
    std::unique_ptr<IMatrixEffect> effect{};

    bool initSDL() {
        if (SDL_Init(SDL_INIT_VIDEO) < 0) {
            return false;
        }

        window = SDL_CreateWindow("MatrixPixels SDL test",
                                  SDL_WINDOWPOS_UNDEFINED,
                                  SDL_WINDOWPOS_UNDEFINED,
                                  screenWidth,
                                  screenHeight,
                                  SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
        if (window == nullptr) {
            return false;
        }

        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
        if (renderer == nullptr) {
            return false;
        }
        SDL_RenderSetLogicalSize(renderer, screenWidth, screenHeight);
        recreateMatrix(16, 16);
        effect = std::make_unique<GradientEffect>();
        return true;
    }

    void mainLoop() {
        while (!quit) {
            while (SDL_PollEvent(&event)) {
                if (event.type == SDL_QUIT || event.type == SDL_APP_TERMINATING) {
                    quit = true;
                } else if (event.type == SDL_KEYDOWN) {
                    if (event.key.keysym.sym == SDLK_ESCAPE) {
                        quit = true;
                    } else if (event.key.keysym.sym == SDLK_1 || event.key.keysym.sym == SDLK_2) {
                        tMatrixPixelsSize w = 0;
                        tMatrixPixelsSize h = 0;
                        if (event.key.keysym.sym == SDLK_1) {
                            w = 16;
                            h = 16;
                        } else if (event.key.keysym.sym == SDLK_2) {
                            w = 15;
                            h = 5;
                        }
                        recreateMatrix(w, h);
                    } else if (event.key.keysym.sym == SDLK_q) {
                        effect = std::make_unique<GradientEffect>();
                    } else if (event.key.keysym.sym == SDLK_w) {
                        effect = std::make_unique<PlasmaEffect>();
                    }
                }
            }

            if (effect) {
                effect->render(matrix, SDL_GetTicks());
            }
            renderProc();
            SDL_Delay(16); // ~60 FPS
        }
    }

    [[nodiscard]] RenderLayout makeLayout() const noexcept {
        // Keep a small padding; fit grid into logical renderer size.
        constexpr int padding = 10;
        const int matrixW = static_cast<int>(matrix.width());
        const int matrixH = static_cast<int>(matrix.height());

        const int availW = max_c(screenWidth - padding * 2, 1);
        const int availH = max_c(screenHeight - padding * 2, 1);

        const int step = max_c(1, min_c(availW / matrixW, availH / matrixH));
        const int fill = max_c(1, step - max_c(2, step / 4)); // leave a border

        const int totalW = step * matrixW;
        const int totalH = step * matrixH;
        const int offsX = (screenWidth - totalW) / 2;
        const int offsY = (screenHeight - totalH) / 2;

        return RenderLayout{step, fill, offsX, offsY};
    }

    void drawPixel(int x, int y, int step, int fill, csColorRGBA c) {
        SDL_SetRenderDrawColor(renderer, 128, 128, 128, SDL_ALPHA_OPAQUE);
        SDL_Rect rectBorder = {x, y, step - 1, step - 1};
        SDL_RenderDrawRect(renderer, &rectBorder);

        SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, SDL_ALPHA_OPAQUE);
        SDL_Rect rectFill = {x + (step - fill) / 2,
                             y + (step - fill) / 2,
                             fill,
                             fill};
        SDL_RenderFillRect(renderer, &rectFill);
    }

    void renderProc() {
        const RenderLayout layout = makeLayout();

        SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, SDL_ALPHA_OPAQUE);
        SDL_RenderClear(renderer);

        const int matrixW = static_cast<int>(matrix.width());
        const int matrixH = static_cast<int>(matrix.height());
        for (int x = 0; x < matrixW; ++x) {
            for (int y = 0; y < matrixH; ++y) {
                const csColorRGBA c = matrix.getPixel(x, y);
                drawPixel(layout.offsX + x * layout.step,
                          layout.offsY + y * layout.step,
                          layout.step,
                          layout.fill,
                          c);
            }
        }

        SDL_RenderPresent(renderer);
    }

    void recreateMatrix(tMatrixPixelsSize w, tMatrixPixelsSize h) {
        if (w == 0 || h == 0) {
            return;
        }
        matrix = csMatrixPixels{w, h};
    }

    void done() {
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
    }
};

csMainProgramSDL prog;

int main(int /*argc*/, char* /*args*/[]) {
    if (!prog.initSDL()) {
        std::printf("Error: %s\n", SDL_GetError());
        return 1;
    }
    prog.mainLoop();
    prog.done();
    return 0;
}
