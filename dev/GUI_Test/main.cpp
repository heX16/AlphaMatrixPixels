// Minimal SDL-based viewer for matrix_pixels
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <memory>
#include "../src/matrix_pixels.hpp"
#include "../src/math.hpp"

#include "../src/color_rgba.hpp"
#include "../src/matrix_render.hpp"
#include "../src/matrix_render_efffects.hpp"

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
using amp::csRenderDynamic;
using amp::csRenderSnowfall;
using amp::csRenderClock;
using amp::csRenderContainer;
using amp::csRenderFill;

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
    TTF_Font* font{nullptr};

    csMatrixPixels matrix{0, 0};
    csRandGen randGen{};
    csEffectBase* effect{nullptr};
    csEffectBase* effect2{nullptr};

    void bindEffectMatrix(csEffectBase* eff) {
        if (auto* m = dynamic_cast<amp::csRenderMatrixBase*>(eff)) {
            m->setMatrix(matrix);
            // If it's a container, also bind matrix to nested effects
            if (auto* container = dynamic_cast<csRenderContainer*>(eff)) {
                for (uint8_t i = 0; i < csRenderContainer::maxEffects; ++i) {
                    if (container->effects[i] != nullptr) {
                        bindEffectMatrix(container->effects[i]);
                    }
                }
            }
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

    csRenderContainer* createClock() const noexcept {
        // Get font dimensions for clock size calculation
        const auto& font = amp::font3x5Digits();
        const tMatrixPixelsSize fontWidth = static_cast<tMatrixPixelsSize>(font.width());
        const tMatrixPixelsSize fontHeight = static_cast<tMatrixPixelsSize>(font.height());
        constexpr tMatrixPixelsSize spacing = 1; // spacing between digits
        constexpr uint8_t digitCount = 4; // clock displays 4 digits
        
        // Calculate clock rect size: 4 digits + 3 spacings between them
        const tMatrixPixelsSize clockWidth = digitCount * fontWidth + (digitCount - 1) * spacing;
        const tMatrixPixelsSize clockHeight = fontHeight;
        
        // Create container
        auto* container = new csRenderContainer();
        
        // Create fill effect (background) - covers clock rect
        auto* fill = new csRenderFill();
        fill->color = csColorRGBA{192, 0, 0, 0};
        fill->renderRectAutosize = false;
        fill->rect = amp::csRect{1, 1, amp::to_size(clockWidth+2), amp::to_size(clockHeight+2)};
        
        // Create clock effect
        auto* clock = new csRenderClock();
        clock->glyph.color = csColorRGBA{255, 255, 255, 255};
        clock->glyph.backgroundColor = csColorRGBA{0, 0, 0, 0};
        clock->renderRectAutosize = false;
        clock->rect = amp::csRect{2, 2, amp::to_size(clockWidth+1), amp::to_size(clockHeight+1)};
        
        // Add effects to container (fill first, then clock on top)
        container->effects[0] = fill;
        container->effects[1] = clock;
        
        return container;
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
        
        // Initialize SDL_ttf
        if (TTF_Init() == -1) {
            std::printf("TTF_Init error: %s\n", TTF_GetError());
            // Continue without TTF, will use fallback rendering
        } else {
            // Try to load a system font
            // Common Windows font paths
            const char* fontPaths[] = {
                "C:/Windows/Fonts/arial.ttf",
                "C:/Windows/Fonts/calibri.ttf",
                "C:/Windows/Fonts/consola.ttf",
                nullptr
            };
            
            for (int i = 0; fontPaths[i] != nullptr; ++i) {
                font = TTF_OpenFont(fontPaths[i], 16);
                if (font) {
                    std::printf("Loaded font: %s\n", fontPaths[i]);
                    break;
                }
            }
            
            if (!font) {
                std::printf("Warning: Could not load any font, text will not be displayed\n");
            }
        }
        
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
                    } else if (event.key.keysym.sym == SDLK_1 || event.key.keysym.sym == SDLK_2 || event.key.keysym.sym == SDLK_3) {
                        tMatrixPixelsSize w = 0;
                        tMatrixPixelsSize h = 0;
                        if (event.key.keysym.sym == SDLK_1) {
                            w = 16;
                            h = 16;
                        } else if (event.key.keysym.sym == SDLK_2) {
                            w = 19;
                            h = 9;
                        } else if (event.key.keysym.sym == SDLK_3) {
                            w = 8;
                            h = 8;
                        }
                        recreateMatrix(w, h);
                        bindEffectMatrix(effect);
                        bindEffectMatrix(effect2);
                    } else if (event.key.keysym.sym == SDLK_w) {
                        delete effect;
                        effect = new csRenderGradientWaves();
                        bindEffectMatrix(effect);
                    } else if (event.key.keysym.sym == SDLK_e) {
                        delete effect;
                        effect = new csRenderGradientWavesFP();
                        bindEffectMatrix(effect);
                    } else if (event.key.keysym.sym == SDLK_q) {
                        delete effect;
                        effect = new csRenderPlasma();
                        bindEffectMatrix(effect);
                    } else if (event.key.keysym.sym == SDLK_s) {
                        delete effect;
                        effect = new csRenderSnowfall();
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
                    } else if (event.key.keysym.sym == SDLK_c) {
                        delete effect2;
                        effect2 = createClock();
                        bindEffectMatrix(effect2);
                    } else if (event.key.keysym.sym == SDLK_KP_PLUS || event.key.keysym.sym == SDLK_PLUS) {
                        // Increase scale for dynamic effects
                        if (auto* dynamicEffect = dynamic_cast<csRenderDynamic*>(effect)) {
                            dynamicEffect->scale = dynamicEffect->scale + amp::math::csFP16{0.1f};
                        }
                        if (auto* dynamicEffect2 = dynamic_cast<csRenderDynamic*>(effect2)) {
                            dynamicEffect2->scale = dynamicEffect2->scale + amp::math::csFP16{0.1f};
                        }
                    } else if (event.key.keysym.sym == SDLK_KP_MINUS || event.key.keysym.sym == SDLK_MINUS) {
                        // Decrease scale for dynamic effects
                        if (auto* dynamicEffect = dynamic_cast<csRenderDynamic*>(effect)) {
                            dynamicEffect->scale = dynamicEffect->scale - amp::math::csFP16{0.1f};
                        }
                        if (auto* dynamicEffect2 = dynamic_cast<csRenderDynamic*>(effect2)) {
                            dynamicEffect2->scale = dynamicEffect2->scale - amp::math::csFP16{0.1f};
                        }
                    }
                }
            }

            matrix.clear();

            if (effect) {
                const uint32_t ticks = SDL_GetTicks();
                if (auto* glyph = dynamic_cast<csRenderGlyph*>(effect)) {
                    glyph->symbolIndex = static_cast<uint8_t>((ticks / 1000u) % 10u);
                    glyph->render(randGen, static_cast<uint16_t>(ticks));
                } else if (auto* snowfall = dynamic_cast<csRenderSnowfall*>(effect)) {
                    snowfall->recalc(randGen, static_cast<uint16_t>(ticks));
                    snowfall->render(randGen, static_cast<uint16_t>(ticks));
                } else if (auto* clock = dynamic_cast<csRenderClock*>(effect)) {
                    // Update time from SDL ticks (convert to seconds)
                    clock->time = ticks / 1000u;
                    clock->render(randGen, static_cast<uint16_t>(ticks));
                } else {
                    effect->render(randGen, static_cast<uint16_t>(ticks));
                }
            }
            if (effect2) {
                const uint32_t ticks = SDL_GetTicks();
                if (auto* container = dynamic_cast<csRenderContainer*>(effect2)) {
                    // Update time for clock inside container
                    for (uint8_t i = 0; i < csRenderContainer::maxEffects; ++i) {
                        if (auto* clock = dynamic_cast<csRenderClock*>(container->effects[i])) {
                            clock->time = ticks / 1000u;
                            break;
                        }
                    }
                    container->render(randGen, static_cast<uint16_t>(ticks));
                } else if (auto* glyph = dynamic_cast<csRenderGlyph*>(effect2)) {
                    glyph->symbolIndex = static_cast<uint8_t>((ticks / 1000u) % 10u);
                    glyph->render(randGen, static_cast<uint16_t>(ticks));
                } else if (auto* snowfall = dynamic_cast<csRenderSnowfall*>(effect2)) {
                    snowfall->recalc(randGen, static_cast<uint16_t>(ticks));
                    snowfall->render(randGen, static_cast<uint16_t>(ticks));
                } else if (auto* clock = dynamic_cast<csRenderClock*>(effect2)) {
                    // Update time from SDL ticks (convert to seconds)
                    clock->time = ticks / 1000u;
                    clock->render(randGen, static_cast<uint16_t>(ticks));
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

    void drawNumber(int x, int y, float value) {
        // Format number as string
        char buf[32];
        std::snprintf(buf, sizeof(buf), "scale: %.2f", value);
        
        if (font) {
            // Use SDL_ttf for rendering
            SDL_Color color = {255, 255, 255, 255};
            SDL_Surface* textSurface = TTF_RenderText_Solid(font, buf, color);
            if (textSurface) {
                SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
                if (textTexture) {
                    SDL_Rect destRect = {x, y, textSurface->w, textSurface->h};
                    SDL_RenderCopy(renderer, textTexture, nullptr, &destRect);
                    SDL_DestroyTexture(textTexture);
                }
                SDL_FreeSurface(textSurface);
            }
        }
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

        // Draw scale parameter
        if (effect) {
            if (auto* dynamicEffect = dynamic_cast<csRenderDynamic*>(effect)) {
                const float scaleValue = dynamicEffect->scale.to_float();
                drawNumber(10, 10, scaleValue);
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
        if (font) {
            TTF_CloseFont(font);
        }
        TTF_Quit();
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
