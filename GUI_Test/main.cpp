// Minimal SDL-based viewer for matrix_pixels
#include <SDL2/SDL.h>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include "../matrix_pixels.hpp"
#include "../color_rgba.hpp"

// Screen dimension constants
constexpr int screenWidth  = 640;
constexpr int screenHeight = 480;

// Logical matrix size for the demo
constexpr int cMatrixWidth  = 16;
constexpr int cMatrixHeight = 16;

// Pixel step on screen and size of the drawn block
constexpr int cPW = 32; // distance between cells
constexpr int cPS = 22; // size of the filled square

// Simple SDL wrapper to draw matrix_pixels with a time-based gradient
class csMainProgramSDL {
public:
    bool quit = false;

    SDL_Window* window{};
    SDL_Renderer* renderer{};
    SDL_Event event{};

    csMatrixPixels matrix{static_cast<tMatrixPixelsSize>(cMatrixWidth),
                          static_cast<tMatrixPixelsSize>(cMatrixHeight)};

    bool initSDL() {
        if (SDL_Init(SDL_INIT_VIDEO) < 0) {
            return false;
        }

        window = SDL_CreateWindow("MatrixPixels SDL test",
                                  SDL_WINDOWPOS_UNDEFINED,
                                  SDL_WINDOWPOS_UNDEFINED,
                                  screenWidth,
                                  screenHeight,
                                  SDL_WINDOW_SHOWN);
        if (window == nullptr) {
            return false;
        }

        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
        if (renderer == nullptr) {
            return false;
        }
        return true;
    }

    void updateGradient() {
        // Time-dependent gradient; hue-like drift on X/Y axes.
        const float t = static_cast<float>(SDL_GetTicks()) * 0.001f;
        auto wave = [](float v) -> uint8_t {
            return static_cast<uint8_t>((std::sin(v) * 0.5f + 0.5f) * 255.0f);
        };

        for (int y = 0; y < cMatrixHeight; ++y) {
            for (int x = 0; x < cMatrixWidth; ++x) {
                const float xf = static_cast<float>(x) * 0.4f;
                const float yf = static_cast<float>(y) * 0.4f;
                const uint8_t r = wave(t * 0.8f + xf);
                const uint8_t g = wave(t * 1.0f + yf);
                const uint8_t b = wave(t * 0.6f + xf + yf * 0.5f);
                matrix.setPixel(x, y, csColorRGBA{255, r, g, b});
            }
        }
    }

    void mainLoop() {
        while (!quit) {
            while (SDL_PollEvent(&event)) {
                if (event.type == SDL_QUIT || event.type == SDL_APP_TERMINATING) {
                    quit = true;
                } else if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE) {
                    quit = true;
                }
            }

            updateGradient();
            renderProc();
            SDL_Delay(16); // ~60 FPS
        }
    }

    void drawPixel(int x, int y, csColorRGBA c) {
        SDL_SetRenderDrawColor(renderer, 128, 128, 128, SDL_ALPHA_OPAQUE);
        SDL_Rect rectBorder = {x, y, cPW - 1, cPW - 1};
        SDL_RenderDrawRect(renderer, &rectBorder);

        SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, SDL_ALPHA_OPAQUE);
        SDL_Rect rectFill = {x + (cPW - cPS) / 2,
                             y + (cPW - cPS) / 2,
                             cPS,
                             cPS};
        SDL_RenderFillRect(renderer, &rectFill);
    }

    void renderProc() {
        SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, SDL_ALPHA_OPAQUE);
        SDL_RenderClear(renderer);

        const int offsX = 20;
        const int offsY = 20;
        for (int x = 0; x < cMatrixWidth; ++x) {
            for (int y = 0; y < cMatrixHeight; ++y) {
                const csColorRGBA c = matrix.getPixel(x, y);
                drawPixel(offsX + x * cPW, offsY + y * cPW, c);
            }
        }

        SDL_RenderPresent(renderer);
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
