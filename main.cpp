#include <libndls.h>
#include <unistd.h>
#include <time.h>

#ifndef _TINSPIRE
#define SDL_MAIN_HANDLED
#include <SDL/SDL.h>
#undef main
#endif

#include "gl.h"
#include "terrain.h"
#include "worldtask.h"
#include "starttask.h"

#include "textures/loading.h"

static uint32_t nowMs()
{
#ifdef _TINSPIRE
    return static_cast<uint32_t>((clock() * 1000) / CLOCKS_PER_SEC);
#else
    return SDL_GetTicks();
#endif
}

int main(int argc, char *argv[])
{
    #ifdef _TINSPIRE
        //Sometimes there's a clock on screen, switch that off
        __asm__ volatile("mrs r0, cpsr;"
                        "orr r0, r0, #0x80;"
                        "msr cpsr_c, r0;" ::: "r0");
    #endif

    nglInit();
    srand(static_cast<unsigned int>(time(nullptr)));
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
    {
        world_task.setMessage("World loaded.");
        start_task.setHasSavedWorld(true);
    }
    else
    {
        world_task.setMessage("World failed to load.");
        start_task.setHasSavedWorld(false);
    }

    //Start with StartTask as current task so we can choose flat/terrain.
    start_task.makeCurrent();

    #ifdef _TINSPIRE
    constexpr uint32_t tick_ms = 300; // Calculator fixed simulation tick
    #else
    constexpr uint32_t tick_ms = 33; // Fixed simulation tick (~30 Hz)
    #endif
    // dt = frame_time / tick_ms so one "unit" matches the old single logic() call per tick.
    constexpr uint32_t max_frame_ms = 500; // Clamp wall-clock gap (pause / debugger) so one frame does not simulate many seconds.
    uint32_t prev_ticks = nowMs();

    while(Task::running)
    {
        const uint32_t now = nowMs();
        uint32_t frame_time = now - prev_ticks;
        prev_ticks = now;
        if(frame_time > max_frame_ms)
            frame_time = max_frame_ms;

        GLFix dt = GLFix(static_cast<int>(frame_time)) / GLFix(static_cast<int>(tick_ms));
        if(dt > GLFix(4))
            dt = GLFix(4);
        if(dt < GLFix(0.001f))
            dt = GLFix(0.001f);

        Task::current_task->logic(dt);

        //Reset "loading" message
        drawLoadingtext(-1);

        Task::current_task->render();

        nglDisplay();

#ifndef _TINSPIRE
        // Yield a tiny slice to avoid pegging a core; still allows very high visual FPS.
        SDL_Delay(1);
#endif
    }

#ifndef _TINSPIRE
    // 1. Clear world first (may use textures/graphics for loading text)
    world.clear();

    // 2. Clear terrain resources
    terrainUninit();

    // 3. Uninit graphics engine
    nglUninit();

    // 4. Deinitialize UI globals at the very end
    Task::deinitializeGlobals();
#endif

    return 0;
}
