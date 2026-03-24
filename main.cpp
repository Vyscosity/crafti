#include <libndls.h>
#include <unistd.h>

#ifndef _TINSPIRE
#define SDL_MAIN_HANDLED
#include <SDL/SDL.h>
#undef main
#endif

#include "gl.h"
#include "terrain.h"
#include "worldtask.h"

#include "textures/loading.h"

int main(int argc, char *argv[])
{
    #ifdef _TINSPIRE
        //Sometimes there's a clock on screen, switch that off
        __asm__ volatile("mrs r0, cpsr;"
                        "orr r0, r0, #0x80;"
                        "msr cpsr_c, r0;" ::: "r0");
    #endif

    nglInit();
    if(lcd_type() == SCR_320x240_4)
        greyscaleTexture(loading);
    // The loading bitmap is 320x240, but the desktop renderer uses 640x480
    // (SCREEN_WIDTH/HEIGHT). Provide the correct buffer size so nglDisplay
    // doesn't read past the loading bitmap.
    nglSetBuffer(loading.bitmap, 320, 240);
    nglDisplay();

    //Early exit #1
    //(Task::keyPressed can only be used after initializeGlobals)
    if(isKeyPressed(KEY_NSPIRE_ESC))
    {
        nglUninit();
        return 0;
    }

#ifndef _TINSPIRE
    // Desktop builds can load a PNG terrain atlas directly from the repo.
    // If loading fails, terrainInit will fall back to the embedded terrain3 texture (or built-in terrain2).
    terrainInit("textures/terrain3.png");
#else
    terrainInit("/documents/ndless/crafti.ppm.tns");
#endif
    glBindTexture(terrain_current);

    glLoadIdentity();

    //Early exit #2
    if(isKeyPressed(KEY_NSPIRE_ESC))
    {
        terrainUninit();
        nglUninit();

        return 0;
    }

    //If crafti has been started by the file extension association, use the first argument as savefile path
    Task::initializeGlobals(argc > 1 ? argv[1] : "/documents/ndless/crafti.map.tns");

    if(Task::load())
        world_task.setMessage("World loaded.");
    else
        world_task.setMessage("World failed to load.");

    //Start with WorldTask as current task
    world_task.makeCurrent();

#ifndef _TINSPIRE
    constexpr Uint32 tick_ms = 33; // Fixed simulation tick (~30 Hz)
    constexpr Uint32 max_frame_ms = 250; // Clamp to avoid huge catch-up after pauses
    Uint32 prev_ticks = SDL_GetTicks();
    Uint32 accumulator = 0;
#endif

    while(Task::running)
    {
#ifndef _TINSPIRE
        const Uint32 now = SDL_GetTicks();
        Uint32 frame_time = now - prev_ticks;
        prev_ticks = now;
        if(frame_time > max_frame_ms)
            frame_time = max_frame_ms;
        accumulator += frame_time;

        // Run simulation at a fixed rate independent of rendering frequency.
        unsigned int steps = 0;
        while(accumulator >= tick_ms && Task::running)
        {
            Task::current_task->logic();
            accumulator -= tick_ms;

            // Safety guard against spending forever catching up.
            if(++steps >= 8)
            {
                accumulator = 0;
                break;
            }
        }
#endif

#ifdef _TINSPIRE
        Task::current_task->logic();
#endif

        //Reset "loading" message
        drawLoadingtext(-1);

        Task::current_task->render();

        nglDisplay();

#ifndef _TINSPIRE
        // Yield a tiny slice to avoid pegging a core; still allows very high visual FPS.
        SDL_Delay(1);
#endif
    }

    Task::deinitializeGlobals();

    // Must clear the world before shutting down graphics, otherwise
    // the global world object's destructor will try to delete chunks
    // after graphics resources have been freed
    world.clear();

    nglUninit();

    terrainUninit();

    return 0;
}
