// Minimal SDL-based viewer for matrix_pixels
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <cstdint>
#include <cstdio>
#include "../../src/matrix_pixels.hpp"

#include "../../src/color_rgba.hpp"
#include "../../src/matrix_render.hpp"
#include "../../src/matrix_render_efffects.hpp"
#include "../../src/matrix_render_pipes"
#include "../../src/driver_sdl2.hpp"
#include "../../src/fixed_point.hpp"
#include "SDL2/SDL_stdinc.h"
#include "copy_line_index_helper.hpp"

using amp::csColorRGBA;
using amp::csMatrixPixels;
using amp::csEffectBase;
using amp::csRandGen;
using amp::tMatrixPixelsSize;
using amp::csRenderGradientWaves;
using amp::csRenderGradientWavesFP;
using amp::csRenderPlasma;
using amp::csRenderGlyph;
using amp::csRenderCircle;
using amp::csRenderCircleGradient;
using amp::csRenderDynamic;
using amp::csRenderSnowfall;
using amp::csRenderDigitalClock;
using amp::csRenderFill;
using amp::csRenderAverageArea;
using amp::csRenderRemap1DByConstArray;

// Screen dimension constants
constexpr int screenWidth  = 640;
constexpr int screenHeight = 480;

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
    static constexpr size_t maxEffects = 10;
    csEffectBase* effects[maxEffects] = {};
    
    // Helper for csRenderRemapByIndexMatrix functionality
    csCopyLineIndexHelper copyLineIndexHelper;

    void bindEffectMatrix(csEffectBase* eff) {
        if (auto* m = dynamic_cast<amp::csRenderMatrixBase*>(eff)) {
            m->setMatrix(matrix);
        }
    }

    void deleteEffect(csEffectBase* eff) {
        if (!eff) {
            return;
        }
        // Delete the effect itself
        delete eff;
    }




    void handleKeyPress(SDL_Keysym keysym) {
        switch (keysym.sym) {
            case SDLK_ESCAPE:
                quit = true;
                break;
            case SDLK_1:
                recreateMatrix(16, 16);
                break;
            case SDLK_2:
                recreateMatrix(23, 11);
                break;
            case SDLK_3:
                recreateMatrix(8, 8);
                break;
            case SDLK_w:
                createEffectBundle(1, 0);
                break;
            case SDLK_e:
                createEffectBundle(2, 0);
                break;
            case SDLK_q:
                createEffectBundle(3, 0);
                break;
            case SDLK_s:
                createEffectBundle(4, 0);
                break;
            case SDLK_r:
                createEffectBundle(0, 1);
                break;
            case SDLK_t:
                createEffectBundle(0, 2);
                break;
            case SDLK_c:
                createEffectBundle(0, 3);
                break;
            case SDLK_b:
                createEffectBundle(0, 4);
                break;
            case SDLK_8:
                // Toggle debug mode for 2D->1D remapping visualization
                copyLineIndexHelper.isActive = !copyLineIndexHelper.isActive;
                break;
            case SDLK_KP_PLUS:
            case SDLK_PLUS:
                // Increase scale for dynamic effects
                adjustEffectScale(effects[0], 0.1f);
                adjustEffectScale(effects[1], 0.1f);
                break;
            case SDLK_KP_MINUS:
            case SDLK_MINUS:
                // Decrease scale for dynamic effects
                adjustEffectScale(effects[0], -0.1f);
                adjustEffectScale(effects[1], -0.1f);
                break;
            default:
                break;
        }
    }

    void adjustEffectScale(csEffectBase* eff, float delta) {
        if (!eff) {
            return;
        }
        if (auto* dynamicEffect = dynamic_cast<csRenderDynamic*>(eff)) {
            dynamicEffect->scale = dynamicEffect->scale + amp::math::csFP16{delta};
        }
    }

    uint8_t eff1_base = 1;
    uint8_t eff2 = 1;

    void createEffectBundle(uint8_t a_eff1_base, uint8_t a_eff2) {
        if (a_eff1_base != 0) {
            eff1_base = a_eff1_base;
        }
        if (a_eff2 != 0) {
            eff2 = a_eff2;
        }

        // Clear all effects
        for (auto*& eff : effects) {
            deleteEffect(eff);
            eff = nullptr;
        }

        // Create base effect based on number
        switch (eff1_base) {
            case 1:
                effects[0] = new csRenderGradientWaves();
                break;
            case 2:
                effects[0] = new csRenderGradientWavesFP();
                break;
            case 3:
                effects[0] = new csRenderPlasma();
                break;
            case 4:
                effects[0] = new csRenderSnowfall();
                break;
            default:
                ;
        }

        // Create effect based on number
        switch (eff2) {
            case 1: {
                auto* glyph = new csRenderGlyph();
                glyph->color = csColorRGBA{255, 255, 255, 255};
                glyph->backgroundColor = csColorRGBA{196, 0, 0, 0};
                glyph->symbolIndex = 0;
                glyph->setFont(amp::getStaticFontTemplate<amp::csFont4x7Digits>());
                glyph->renderRectAutosize = false;
                glyph->rectDest = amp::csRect{
                    2,
                    2,
                    amp::to_size(glyph->fontWidth + 2),
                    amp::to_size(glyph->fontHeight + 2)
                };
                effects[1] = glyph;
                break;
            }
            case 2: {
                // auto* circle = new csRenderCircle();
                auto* circle = new csRenderCircleGradient();
                // circle.color = csColorRGBA{255, 0, 0, 255};
                circle->color = csColorRGBA{255, 255, 255, 255};
                circle->backgroundColor = csColorRGBA{0, 0, 0, 0};
                circle->gradientOffset = 127;
                circle->renderRectAutosize = true; // использовать весь rect матрицы
                effects[1] = circle;
                break;
            }
            case 3: {
                // Get font dimensions for clock size calculation
                const auto& font = amp::getStaticFontTemplate<amp::csFont4x7DigitClock>();
                const tMatrixPixelsSize fontWidth = static_cast<tMatrixPixelsSize>(font.width());
                const tMatrixPixelsSize fontHeight = static_cast<tMatrixPixelsSize>(font.height());
                constexpr tMatrixPixelsSize spacing = 1; // spacing between digits
                constexpr uint8_t digitCount = 4; // clock displays 4 digits
                
                // Calculate clock rect size: 4 digits + 3 spacings between them
                const tMatrixPixelsSize clockWidth = digitCount * fontWidth + (digitCount - 1) * spacing;
                const tMatrixPixelsSize clockHeight = fontHeight;
                
                // Create fill effect (background) - covers clock rect
                auto* fill = new csRenderFill();
                fill->color = csColorRGBA{192, 0, 0, 0};
                fill->renderRectAutosize = false;
                fill->rectDest = amp::csRect{1, 1, amp::to_size(clockWidth+2), amp::to_size(clockHeight+2)};
                
                // Create clock effect
                auto* clock = new csRenderDigitalClock();
                
                // Create glyph effect for rendering digits
                // TODO: leak memory!!!
                auto* digitGlyph = clock->createGlyph();
                digitGlyph->setFont(font);
                digitGlyph->color = csColorRGBA{255, 255, 255, 255};
                digitGlyph->backgroundColor = csColorRGBA{255, 0, 0, 0};
                digitGlyph->renderRectAutosize = false;
                
                // Set glyph via paramRenderDigit parameter
                clock->glyph = digitGlyph;
                
                // Notify clock that paramRenderDigit parameter changed
                // This will validate the glyph type and update its matrix if needed
                if (auto* digitalClock = dynamic_cast<amp::csRenderDigitalClock*>(clock)) {
                    digitalClock->paramChanged(amp::csRenderDigitalClock::paramRenderDigit);
                }
                
                clock->renderRectAutosize = false;
                clock->rectDest = amp::csRect{2, 2, amp::to_size(clockWidth+1), amp::to_size(clockHeight+1)};
                
                // Add effects to array (fill first, then clock on top)
                effects[1] = fill;
                effects[2] = clock;
                break;
            }
            case 4: {
                auto* averageArea = new csRenderAverageArea();
                averageArea->matrix = &matrix;
                averageArea->matrixSource = &matrix;
                averageArea->renderRectAutosize = false;
                averageArea->rectSource = amp::csRect{1, 1, 4, 4};
                averageArea->rectDest = amp::csRect{1, 1, 4, 4};
                effects[1] = averageArea;
                break;
            }
            default:
                ;
        }

        // Bind matrix to the created effects
        for (auto* eff : effects) {
            bindEffectMatrix(eff);
        }
    }

    void updateAndRenderEffect(csEffectBase* eff, uint32_t ticks, uint16_t currTime) {
        if (!eff) {
            return;
        }

        // Update effect-specific parameters
        if (auto* glyph = dynamic_cast<csRenderGlyph*>(eff)) {
            glyph->symbolIndex = static_cast<uint8_t>((ticks / 1000u) % 10u);
        } else if (auto* clock = dynamic_cast<csRenderDigitalClock*>(eff)) {
            // Update time from SDL ticks (convert to seconds)
            clock->time = ticks / 1000u;
        }

        // Always call recalc for all effects
        eff->recalc(randGen, currTime);

        // Render the effect
        eff->render(randGen, currTime);
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
        createEffectBundle(1, 0);
        return true;
    }

    void mainLoop() {
        while (!quit) {
            while (SDL_PollEvent(&event)) {
                if (event.type == SDL_QUIT || event.type == SDL_APP_TERMINATING) {
                    quit = true;
                } else if (event.type == SDL_KEYDOWN) {
                    handleKeyPress(event.key.keysym);
                }
            }

            matrix.clear();

            const uint32_t ticks = SDL_GetTicks();
            const uint16_t currTime = static_cast<uint16_t>(ticks);

            for (auto* eff : effects) {
                updateAndRenderEffect(eff, ticks, currTime);
            }
            copyLineIndexHelper.updateCopyLineIndexSource(matrix, randGen, currTime);
            renderProc();
            SDL_Delay(16); // ~60 FPS
        }
    }


    void drawNumber(int x, int y, float value, const char* format = "%.2f") {
        // Format number as string
        char buf[32];
        std::snprintf(buf, sizeof(buf), format, value);
        
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
        // Render matrix using driver_sdl2 module
        amp::renderMatrixToSDL(matrix, renderer, screenWidth, screenHeight,
                                true,  // clearBeforeRender
                                false  // presentAfterRender - we'll call it after drawing scale
                               );

        // Draw scale parameter
        if (effects[0]) {
            if (auto* dynamicEffect = dynamic_cast<csRenderDynamic*>(effects[0])) {
                const float scaleValue = dynamicEffect->scale.to_float();
                drawNumber(10, 10, scaleValue, "scale: %.2f");
            }
        }

        // Render 1D matrix overlay if debug mode is active
        copyLineIndexHelper.render(renderer, screenWidth, screenHeight);

        SDL_RenderPresent(renderer);
    }

    void recreateMatrix(tMatrixPixelsSize w, tMatrixPixelsSize h) {
        if (w == 0 || h == 0) {
            return;
        }
        matrix = csMatrixPixels{w, h};
        for (auto* eff : effects) {
            bindEffectMatrix(eff);
        }
    }

    void done() {
        for (auto* eff : effects) {
            deleteEffect(eff);
        }
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
