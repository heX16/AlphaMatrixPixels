// Minimal SDL-based viewer for matrix_pixels
#include <SDL2/SDL.h>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <memory>
#include "../matrix_pixels.hpp"
#include "../math.hpp"

#include "../color_rgba.hpp"
#include "../matrix_render.hpp"
#include "../matrix_render_efffects.hpp"

using amp::csColorRGBA;
using amp::csMatrixPixels;
using amp::csEffectBase;
using amp::csRandGen;
using amp::tMatrixPixelsSize;
using amp::math::max;
using amp::math::min;
using amp::csRenderGradientWaves;
using amp::csRenderGradientWavesFP;
using amp::csRenderPlasma;
using amp::csRenderGlyph;
using amp::csRenderCircle;
using amp::csRenderCircleGradient;

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
    csRandGen randGen{};
    csEffectBase* effect{nullptr};
    csEffectBase* effect2{nullptr};

    void bindEffectMatrix(csEffectBase* eff) {
        if (auto* m = dynamic_cast<amp::csRenderMatrixBase*>(eff)) {
            m->setMatrix(matrix);
        }
    }

    void initGlyphDefaults(csRenderGlyph& glyph) const noexcept {
        glyph.color = csColorRGBA{255, 255, 255, 255};
        glyph.backgroundColor = csColorRGBA{196, 0, 0, 0};
        glyph.symbolIndex = 0;
        glyph.setFont(amp::font3x5Digits());
        glyph.renderRectAutosize = false;
        glyph.rect = amp::csRect{
            2,
            2,
            static_cast<amp::tMatrixPixelsSize>(amp::font3x5Digits().width() + 2),
            static_cast<amp::tMatrixPixelsSize>(amp::font3x5Digits().height() + 2)
        };
    }

    void initCircleDefaults(csRenderCircleGradient& circle) const noexcept {
        // circle.color = csColorRGBA{255, 0, 0, 255};
        circle.color = csColorRGBA{255, 255, 255, 255};
        circle.backgroundColor = csColorRGBA{0, 0, 0, 0};
        circle.gradientOffset = 127;
        circle.renderRectAutosize = true; // использовать весь rect матрицы
    }

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
        delete effect;
        effect = new csRenderGradientWaves();
        bindEffectMatrix(effect);
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
                        bindEffectMatrix(effect);
                        bindEffectMatrix(effect2);
                    } else if (event.key.keysym.sym == SDLK_q) {
                        delete effect;
                        effect = new csRenderGradientWaves();
                        bindEffectMatrix(effect);
                    } else if (event.key.keysym.sym == SDLK_e) {
                        delete effect;
                        effect = new csRenderGradientWavesFP();
                        bindEffectMatrix(effect);
                    } else if (event.key.keysym.sym == SDLK_w) {
                        delete effect;
                        effect = new csRenderPlasma();
                        bindEffectMatrix(effect);
                    } else if (event.key.keysym.sym == SDLK_r) {
                        delete effect2;
                        auto* glyph = new csRenderGlyph();
                        initGlyphDefaults(*glyph);
                        effect2 = glyph;
                        bindEffectMatrix(effect2);
                    } else if (event.key.keysym.sym == SDLK_t) {
                        delete effect2;
                        // auto* circle = new csRenderCircle();
                        auto* circle = new csRenderCircleGradient();
                        initCircleDefaults(*circle);
                        effect2 = circle;
                        bindEffectMatrix(effect2);
                    }
                }
            }

            if (effect) {
                const uint32_t ticks = SDL_GetTicks();
                if (auto* glyph = dynamic_cast<csRenderGlyph*>(effect)) {
                    glyph->symbolIndex = static_cast<uint8_t>((ticks / 1000u) % 10u);
                    glyph->render(randGen, static_cast<uint16_t>(ticks));
                } else {
                    effect->render(randGen, static_cast<uint16_t>(ticks));
                }
            }
            if (effect2) {
                const uint32_t ticks = SDL_GetTicks();
                if (auto* glyph = dynamic_cast<csRenderGlyph*>(effect2)) {
                    glyph->symbolIndex = static_cast<uint8_t>((ticks / 1000u) % 10u);
                    glyph->render(randGen, static_cast<uint16_t>(ticks));
                } else {
                    effect2->render(randGen, static_cast<uint16_t>(ticks));
                }
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

        const int availW = max(screenWidth - padding * 2, 1);
        const int availH = max(screenHeight - padding * 2, 1);

        const int step = max(1, min(availW / matrixW, availH / matrixH));
        const int fill = max(1, step - max(2, step / 4)); // leave a border

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
        delete effect;
        delete effect2;
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
