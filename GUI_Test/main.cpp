/*This source code copyrighted by Lazy Foo' Productions (2004-2019)
and may not be redistributed without written permission.*/

#include <SDL2/SDL.h>
#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "config.hpp"
#include "fastled_code.hpp"

//#define CLOCK_MODE

//fat_ver: #include <iostream> - too fat
//fat_ver: #include <stdexcept> - too fat

typedef uint8_t byte;

#define analogRead(x) (x)

#define PROGMEM

using namespace std;

// Screen dimension constants
constexpr auto screenWidth = 640;
constexpr auto screenHeight = 480;

constexpr auto cRenderTimeMs = 1000/30;


#define TIMER_GET_TIME SDL_GetTicks()
#include "small_timer.hpp"

#include "../src/led_core_header.hpp"
#define INT_T uint8_t
#include "../src/utility_funx.h"
#undef INT_T
#include "../src/shuffle.h"
#include "../src/led_core.hpp"
#include "../src/led_effect.hpp"
#include "../src/eff_matrix.hpp"
#include "../src/led_manager.hpp"
#include "../src/palette_progmem.hpp"
#include "effects_pack.hpp"

////////////////////////////////////////////////////////////////

//TODO: move to core

class csPixels1D_Mem: public csPixels1D {
public:
    TColor leds[cLedCount];

    csPixels1D_Mem() {
    };

    virtual const TCoord count() const override {
      return cLedCount;
    };

    inline bool validPos(TCoord x) const {
      return x < cLedCount;
    };

    virtual void setPixelA(TCoord x, TColor newColor, uint8_t alpha) override {
        if (validPos(x)) {
            TColor base = getPixel(x);
            TColor c = TColor{blend8(base.r, newColor.r, alpha), blend8(base.g, newColor.g, alpha), blend8(base.b, newColor.b, alpha)};
            setPixel(x,c);
        }
    };

    virtual void setPixel(TCoord x, TColor c) override {
      if (validPos(x))
        leds[x]=c;
    };

    virtual TColor getPixel(TCoord x) override {
      if (validPos(x))
        return leds[x]; else
        return TColor{0,0,0};
    };

    virtual void show() override {
      ;
    };

    virtual void clear() override {
      memset(leds, 0, sizeof(leds));
    };
};


class csPixels2D_Mem: public csPixels2D {
public:
    TColor matrix[cMatrixWidth][cMatrixHeight];

    virtual TCoord width() const override { return cMatrixWidth; };

    virtual TCoord height() const override { return cMatrixHeight; };

    inline bool validXY(TCoord x, TCoord y) const {
      return (x < cMatrixWidth) && (y < cMatrixHeight);
    };

    csPixels2D_Mem() {
    };

    virtual void setPixelA(TCoord x, TCoord y, TColor newColor, uint8_t alpha) override {
        if (validXY(x,y)) {
            TColor base = getPixel(x,y);
            TColor c = TColor{blend8(base.r, newColor.r, alpha), blend8(base.g, newColor.g, alpha), blend8(base.b, newColor.b, alpha)};
            setPixel(x,y,c);
        }
    };

    virtual void setPixel(TCoord x, TCoord y, TColor c) override {
      if (validXY(x,y))
        matrix[x][y]=c;
    };

    virtual TColor getPixel(TCoord x, TCoord y) override {
      if (validXY(x,y))
        return matrix[x][y]; else
        return TColor{0,0,0};
    };

    virtual void show() override {
      ;
    };

    virtual void clear() override {
      memset(matrix, 0, sizeof(matrix));
    };
};



class csMainProgramSDL {
public:
    static constexpr auto cPW = 60; // pixels step width - step beetwen pixel pos
    static constexpr auto cPS = 30; // pixel size


    bool quit = false;

    volatile bool renderInProgress = false;
    volatile bool waitProcessing = false;

    // The window we'll be rendering to
    SDL_Window* window;

    // The surface contained by the window
    //SDL_Surface* screenSurface;

    SDL_Renderer* renderer;

    SDL_Event event;

    SDL_TimerID timer_run;

    SDL_TimerID timer_wakeup;

    csPixels1D_Mem pixels1D;
    csPixels2D_Mem pixels2D;
    csLedManager2D_Simple LedManager;
    csEff1D_ExplodedSinusNoise eff_ExplodedSinusNoise;
    csEff1D_RandColors eff_RandColors;
    csPalette256_3color_R_G_B palette_RGB;
    csEff1D_Test eff1D_Test;



    bool initSDL() {
        void * ptr = this;
        auto init_sdl = SDL_Init(SDL_INIT_VIDEO);
        if( init_sdl < 0 ) {
            return false;
            //fat_ver: throw std::runtime_error(string("SDL could not initialize! SDL_Error: ") + string(SDL_GetError()));
        }

        // Create window
        window = SDL_CreateWindow("HXLED RGB Clock test (SDL)", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, screenWidth, screenHeight, SDL_WINDOW_SHOWN);
        if( window == NULL ) {
            return false;
                //fat_ver: throw std::runtime_error(string("Window could not be created! SDL_Error: ") + string(SDL_GetError()));
        }

        // Get window surface and render
        /*screenSurface = SDL_GetWindowSurface(window);
        if( screenSurface == NULL ) {
            return false;
                //fat_ver: throw std::runtime_error(string("Window could not be created! SDL_Error: ") + string(SDL_GetError()));
        }*/
        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
        if( renderer == NULL ) {
            return false;
                //fat_ver: throw std::runtime_error(string("Window could not be created! SDL_Error: ") + string(SDL_GetError()));
        }
        timer_run    = SDL_AddTimer(10, timer_do_LedManager_run, ptr);
        timer_wakeup = SDL_AddTimer(cRenderTimeMs, timer_do_wakeup_renderSDL, ptr);
        return true;
    }

    void init() {
        LedManager.init();
        LedManager.setup(&pixels1D, &pixels2D);

        LedManager.addEffect(&eff_ExplodedSinusNoise, TColor{0,255,0});
        //eff_RandColors.setUpPalette(&palette_RGB);
        //LedManager.addEffect(&eff_RandColors);
        //LedManager.addEffect(&eff1D_Test);

        //currentSFX = LoadEffect(16, LedManager);
    }

    static uint32_t timer_do_wakeup_renderSDL(uint32_t interval, void * param)
    {
        csMainProgramSDL * self = static_cast<csMainProgramSDL *>(param);
        if (self->renderInProgress) {
            return cRenderTimeMs/2;
        }

        if (self->waitProcessing) {
            return cRenderTimeMs;
        }

        SDL_Event event;
        SDL_UserEvent userevent;

        /* callback pushes an SDL_USEREVENT eventinto the queue */
        userevent.type = SDL_USEREVENT;
        userevent.code = 1;
        userevent.data1 = NULL;
        userevent.data2 = NULL;

        event.type = SDL_USEREVENT;
        event.user = userevent;

        SDL_PushEvent(&event);

        self->waitProcessing = true;

        return cRenderTimeMs;
    }

    static uint32_t timer_do_LedManager_run(uint32_t interval, void * param)
    {
        if ( ! static_cast<csMainProgramSDL *>(param)->renderInProgress)
            static_cast<csMainProgramSDL *>(param)->LedManager.run();
        return interval;
    }

    const char* getLastSDLError() {
        return SDL_GetError();
    }

    void arduinoLoop() {
        LedManager.run();
    }

    void mainLoop() {
        bool ok;

        while (!quit) {
            // Now we wait for an event to arrive
            ok = SDL_WaitEvent(&event);
            if (ok) {
                do {
                    // Events processing
                    waitProcessing = false;
                    if (false) {
                        // empty
                    } else if (event.type == SDL_QUIT || event.type == SDL_APP_TERMINATING) {
                        quit = true;
                    } else if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE) {
                        quit = true;
                    } else if (event.type == SDL_MOUSEBUTTONUP) {
                        // ...
                    }

                    // Next event
                    ok = SDL_PollEvent(&event);
                } while (ok);
            }

            arduinoLoop();

            // procedure timer_do_wakeup_renderSDL send events - and we wakeup and render
            renderProc();
        }
    }

    void drawPixel(int x, int y, TColor c) {
        SDL_SetRenderDrawColor(renderer, 128, 128, 128, SDL_ALPHA_OPAQUE);
        SDL_Rect rect = {x, y, cPW-1, cPW-1};
        SDL_RenderDrawRect(renderer, &rect);

        SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, SDL_ALPHA_OPAQUE);
        rect = {x+(cPS/2)+(cPS/4), y+(cPS/2), cPS-(cPS/4*2), cPS};
        SDL_RenderFillRect(renderer, &rect);
        rect = {x+(cPS/2), y+(cPS/2)+(cPS/4), cPS, cPS-(cPS/4*2)};
        SDL_RenderFillRect(renderer, &rect);
    }

    void drawRectLine(int x1, int y1, int w, int h, TColor line, TColor bg) {
        SDL_Rect rect = SDL_Rect{x1, y1, w, h};
        SDL_SetRenderDrawColor(renderer, bg.r, bg.g, bg.b, SDL_ALPHA_OPAQUE);
        SDL_RenderFillRect(renderer, &rect);
        SDL_SetRenderDrawColor(renderer, line.r, line.g, line.b, SDL_ALPHA_OPAQUE);
        SDL_RenderDrawRect(renderer, &rect);
    }

    void drawDigit(int x, int y, int d) {
        // 128x256
        //  1
        // 2 3
        //  4
        // 5 6
        //  7
        drawRectLine(x +32, y +0, +64, +32, RGBToColor(128,128,128), RGBToColor(0,192,0));
        drawRectLine(x +0, y +32, +32, +64, RGBToColor(128,128,128), RGBToColor(0,192,0));
        drawRectLine(x +32+64, y +32, +32, +64, RGBToColor(128,128,128), RGBToColor(0,192,0));
        drawRectLine(x +32, y +64+32, +64, +32, RGBToColor(128,128,128), RGBToColor(0,192,0));
        drawRectLine(x +0, y +32+64+32, +32, +64, RGBToColor(128,128,128), RGBToColor(0,192,0));
        drawRectLine(x +32+64, y +32+64+32, +32, +64, RGBToColor(128,128,128), RGBToColor(0,192,0));
        drawRectLine(x +32, y +64+32+64+32, +64, +32, RGBToColor(128,128,128), RGBToColor(0,192,0));
    }

    void renderProc() {
        renderInProgress = true;

        // Initialize renderer color white for the background
        SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, SDL_ALPHA_OPAQUE);

        // Clear screen
        SDL_RenderClear(renderer);

        /*{
            SDL_Rect rect = SDL_Rect{10,10,100,100};

            // Set renderer color red to draw the square
            SDL_SetRenderDrawColor(renderer, 0xFF, 0x00, 0x00, SDL_ALPHA_OPAQUE);

            // Draw filled square
            SDL_RenderFillRect(renderer, &rect);

            // Update screen
            SDL_RenderPresent(renderer);

            renderInProgress = false;
            return;
        }*/

        auto offsX = 0;
        auto offsY = cPW + 10;
        for(auto x=0; x<cMatrixWidth; x++) {
            for(auto y=0; y<cMatrixHeight; y++) {
                TColor c = LedManager.pixels2D->getPixel(x,y);
                drawPixel(offsX+x*cPW, offsY+y*cPW, c);
            }
        }

        auto offsX_line = 0;
        auto offsY_line = 0;
        for(auto i=0; i<cLedCount; i++) {
            TColor c = LedManager.pixels1D->getPixel(i);
            drawPixel(offsX_line+i*cPW, offsY_line*cPW, c);

            // Дублирование рисования квадратиков размером в 10 пикселей
            SDL_Rect smallRect = {offsX_line+i*10, offsY_line, 10, 10};
            SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, SDL_ALPHA_OPAQUE);
            SDL_RenderFillRect(renderer, &smallRect);
        }

        #ifdef CLOCK_MODE
        drawRectLine(32 -16, 50 -16, +144*4+16+16, +256, RGBToColor(128,128,128), RGBToColor(0,0,0));
        drawDigit(32+144*0, 50, 8);
        drawDigit(32+144*1, 50, 8);
        drawDigit(32+16+144*2, 50, 8);
        drawDigit(32+16+144*3, 50, 8);
        #endif // CLOCK_MODE

        // Update screen
        SDL_RenderPresent(renderer);

        renderInProgress = false;
    }

    void done() {
            SDL_RemoveTimer(timer_run);
            SDL_RemoveTimer(timer_wakeup);
            SDL_DestroyRenderer(renderer);
            SDL_DestroyWindow(window);
            //Quit SDL subsystems
            SDL_Quit();
    }

};

csMainProgramSDL prog;


int main( int argc, char* args[] )
{
    if (! prog.initSDL()) {
        //fat_ver: count << prog.getLastSDLError();
        printf("Error: %s\n", prog.getLastSDLError());
        return 0;
    }
    prog.init();
    prog.mainLoop();
    prog.done();
	return 0;
}
