// Minimal SDL-based viewer for matrix_pixels
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <cstdint>
#include <cstdio>
#include "../../src/matrix_pixels.hpp"

#include "../../src/color_rgba.hpp"
#include "../../src/render_base.hpp"
#include "../../src/render_efffects.hpp"
#include "../../src/render_pipes.hpp"
#include "../../src/driver_sdl2.hpp"
#include "../../src/fixed_point.hpp"
#include "../../src/effect_manager.hpp"
#include "copy_line_index_helper.hpp"
#include "../../src/effect_presets.hpp"

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
using amp::csRenderRectangle;
using amp::csRenderAverageArea;
using amp::csRenderRemap1DByConstArray;
using amp::csRenderBouncingPixel;

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

    amp::csMatrixSFXSystem sfxSystem;
    
    // Helper for csRenderRemapByIndexMatrix functionality
    csCopyLineIndexHelper copyLineIndexHelper;

    // Preset ID ranges for cycling via keyboard.
    // NOTE: These ranges are intentionally continuous to allow +/- 1 wrap-around cycling.
    static constexpr uint16_t cEff1BaseMin = 101;
    static constexpr uint16_t cEff1BaseMax = 112;
    static constexpr uint16_t cEff2Min = 105;
    static constexpr uint16_t cEff2Max = 115;

    // Active preset IDs:
    // - eff1_base: base effect (usually 101-110 in GUI_Test presets)
    // - eff2: secondary overlay effect (usually 105-109; 200 is used as "skip" by an existing hotkey)
    uint16_t eff1_base = cEff1BaseMin; // GradientWaves (was 1)
    uint16_t eff2 = cEff2Min; // Glyph (was 5)

    static uint16_t cyclePresetId(uint16_t current, uint16_t minId, uint16_t maxId, int delta) {
        if (minId == 0 || maxId == 0 || minId > maxId) {
            return current;
        }

        // If current is outside of the supported range, normalize it first.
        if (current < minId || current > maxId) {
            current = minId;
        }

        if (delta > 0) {
            return (current >= maxId) ? minId : static_cast<uint16_t>(current + 1u);
        }
        if (delta < 0) {
            return (current <= minId) ? maxId : static_cast<uint16_t>(current - 1u);
        }
        return current;
    }

    void cycleEff1Base(int delta) {
        const uint16_t next = cyclePresetId(eff1_base, cEff1BaseMin, cEff1BaseMax, delta);
        createEffectBundleDouble(next, 0);
    }

    void cycleEff2(int delta) {
        const uint16_t next = cyclePresetId(eff2, cEff2Min, cEff2Max, delta);
        createEffectBundleDouble(0, next);
    }

    void handleKeyPress(SDL_Keysym keysym) {
        switch (keysym.sym) {
            case SDLK_ESCAPE:
                quit = true;
                break;
            case SDLK_1:
                recreateMatrix(16, 16);
                createEffectBundleDouble(0, 0);
                break;
            case SDLK_2:
                recreateMatrix(23, 11);
                createEffectBundleDouble(0, 0);
                break;
            case SDLK_3:
                recreateMatrix(8, 8);
                createEffectBundleDouble(0, 0);
                break;
            case SDLK_4:
                recreateMatrix(19, 7);
                createEffectBundleDouble(0, 0);
                break;
            case SDLK_w:
                createEffectBundleDouble(101, 0); // GradientWaves
                break;
            case SDLK_e:
                createEffectBundleDouble(102, 0); // GradientWavesFP
                break;
            case SDLK_q:
                createEffectBundleDouble(103, 0); // Plasma
                break;
            case SDLK_s:
                createEffectBundleDouble(104, 0); // Snowfall
                break;
            case SDLK_r:
                createEffectBundleDouble(0, 105); // Glyph
                break;
            case SDLK_t:
                createEffectBundleDouble(0, 106); // Circle
                break;
            case SDLK_c:
                createEffectBundleDouble(0, 107); // Clock
                break;
            case SDLK_b:
                createEffectBundleDouble(0, 108); // AverageArea
                break;
            case SDLK_v:
                createEffectBundleDouble(0, 109); // Clock (3x5 font)
                break;
            case SDLK_m:
                createEffectBundleDouble(111, 0); // 7 horizontal lines
                break;
            case SDLK_p:
                createEffectBundleDouble(112, 0); // BouncingPixel (eff1_base)
                break;
            case SDLK_a:
                createEffectBundleDouble(0, 110); // SlowFadingBackground (eff2)
                break;
            case SDLK_f:
                createEffectBundleDouble(0, 114); // SlowFadingOverlay (eff2)
                break;
            case SDLK_n:
                createEffectBundleDouble(0, 200); // skip
                break;
            case SDLK_LEFTBRACKET:
                cycleEff1Base(-1);
                break;
            case SDLK_RIGHTBRACKET:
                cycleEff1Base(+1);
                break;
            // '<' and '>' are typically Shift+Comma/Period, but keysyms can be either:
            // - SDLK_COMMA / SDLK_PERIOD (physical key)
            // - SDLK_LESS / SDLK_GREATER (character)
            case SDLK_COMMA:
            case SDLK_LESS:
                cycleEff2(-1);
                break;
            case SDLK_PERIOD:
            case SDLK_GREATER:
                cycleEff2(+1);
                break;
            case SDLK_8:
                // Toggle debug mode for 2D->1D remapping visualization
                copyLineIndexHelper.isActive = !copyLineIndexHelper.isActive;
                break;
            case SDLK_KP_PLUS:
            case SDLK_PLUS:
                // Increase scale for dynamic effects
                adjustEffectScale((*sfxSystem.effectManager)[0], 0.1f);
                adjustEffectScale((*sfxSystem.effectManager)[1], 0.1f);
                break;
            case SDLK_KP_MINUS:
            case SDLK_MINUS:
                // Decrease scale for dynamic effects
                adjustEffectScale((*sfxSystem.effectManager)[0], -0.1f);
                adjustEffectScale((*sfxSystem.effectManager)[1], -0.1f);
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
            dynamicEffect->scale = dynamicEffect->scale + csFP16{delta};
        }
    }

    void createEffectBundleDouble(uint16_t a_eff1_base, uint16_t a_eff2) {
        if (a_eff1_base != 0) {
            eff1_base = a_eff1_base;
        }
        if (a_eff2 != 0) {
            eff2 = a_eff2;
        }

        // Clear all effects
        sfxSystem.effectManager->clearAll();

        // Default order: base first, then overlay.
        if (eff1_base != 0) {
            loadEffectPreset(*sfxSystem.effectManager, eff1_base);
        }
        if (eff2 != 0) {
            loadEffectPreset(*sfxSystem.effectManager, eff2);
        }
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
        createEffectBundleDouble(101, 0); // GradientWaves
        copyLineIndexHelper.configureRemapEffect(sfxSystem.matrix);
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

            sfxSystem.matrix->clear();

            const uint32_t ticks = SDL_GetTicks();
            const amp::tTime currTime = static_cast<amp::tTime>(ticks);

            for (auto* eff : *sfxSystem.effectManager) {
                if (!eff) {
                    continue;
                }
                if (auto* glyph = dynamic_cast<csRenderGlyph*>(eff)) {
                    glyph->symbolIndex = static_cast<uint8_t>((ticks / 1000u) % 10u);
                } else if (auto* clock = dynamic_cast<csRenderDigitalClock*>(eff)) {
                    clock->time = ticks / 1000u;
                }
            }
            sfxSystem.recalcAndRender(currTime);
            copyLineIndexHelper.updateCopyLineIndexSource(sfxSystem.randGen, currTime);
            renderProc();
            SDL_Delay(16); // ~60 FPS
            //SDL_Delay(20); // 50 FPS
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
        amp::renderMatrixToSDL(*sfxSystem.matrix, renderer, screenWidth, screenHeight,
                                true,  // clearBeforeRender
                                false  // presentAfterRender - we'll call it after drawing scale
                               );

        // Draw scale property
        if ((*sfxSystem.effectManager)[0]) {
            if (auto* dynamicEffect = dynamic_cast<csRenderDynamic*>((*sfxSystem.effectManager)[0])) {
                const float scaleValue = dynamicEffect->scale.to_float();
                drawNumber(10, 10, scaleValue, "scale: %.2f");
            }
        }

        // Draw effect preset IDs
        drawNumber(10, 30, static_cast<float>(eff1_base), "eff1_base: %.0f");
        drawNumber(10, 50, static_cast<float>(eff2), "eff2: %.0f");

        // Render 1D matrix overlay if debug mode is active
        copyLineIndexHelper.render(renderer, screenWidth, screenHeight);

        SDL_RenderPresent(renderer);
    }

    void recreateMatrix(tMatrixPixelsSize w, tMatrixPixelsSize h) {
        if (w == 0 || h == 0) {
            return;
        }
        // Delete old matrix
        sfxSystem.deleteMatrix();
        // Create new matrix with new size
        sfxSystem.setMatrix(sfxSystem.createMatrix(w, h));
        copyLineIndexHelper.configureRemapEffect(sfxSystem.matrix);
    }

    void done() {
        sfxSystem.effectManager->clearAll();
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
