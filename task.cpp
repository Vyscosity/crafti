#include "task.h"
#include <zlib.h>

#ifndef _TINSPIRE
#include <SDL/SDL.h>
#endif

#include "texturetools.h"
#include "blocklisttask.h"
#include "worldtask.h"
#include "settingstask.h"
#include "inventory.h"

//The values have to stay somewhere
Task *Task::current_task;
bool Task::key_held_down, Task::running, Task::background_saved, Task::has_touchpad, Task::keys_inverted;
TEXTURE *Task::screen, *Task::background;
const char *Task::savefile;

void Task::makeCurrent()
{
    current_task = this;
}

bool Task::keyPressed(const t_key &key)
{
    #ifdef _TINSPIRE
        if(has_touchpad)
        {
            if(key.tpad_arrow != TPAD_ARROW_NONE)
                return touchpad_arrow_pressed(key.tpad_arrow);
            else
                return !(*reinterpret_cast<volatile uint16_t*>(0x900E0000 + key.tpad_row) & key.tpad_col) == keys_inverted;
        }
        else
            return (*reinterpret_cast<volatile uint16_t*>(0x900E0000 + key.row) & key.col) == 0;
    #else
        static bool quit_requested = false;

        SDL_PumpEvents();
        SDL_Event event;
        while(SDL_PollEvent(&event))
        {
            if(event.type == SDL_QUIT)
            {
                quit_requested = true;
                running = false;
            }
        }

        if(quit_requested)
            return key.row == KEY_NSPIRE_ESC.row && key.col == KEY_NSPIRE_ESC.col;

        const Uint8 *keys = SDL_GetKeyState(nullptr);
        const Uint8 mouse = SDL_GetMouseState(nullptr, nullptr);

        const auto is_down = [keys](SDLKey sdl_key) {
            return keys[sdl_key] != 0;
        };

        if(key.row == KEY_NSPIRE_ESC.row && key.col == KEY_NSPIRE_ESC.col)
            return is_down(SDLK_ESCAPE);

        if(key.row == KEY_NSPIRE_8.row && key.col == KEY_NSPIRE_8.col)
            return is_down(SDLK_w) || is_down(SDLK_UP);
        if(key.row == KEY_NSPIRE_2.row && key.col == KEY_NSPIRE_2.col)
            return is_down(SDLK_s) || is_down(SDLK_DOWN);
        if(key.row == KEY_NSPIRE_4.row && key.col == KEY_NSPIRE_4.col)
            return is_down(SDLK_a) || is_down(SDLK_LEFT);
        if(key.row == KEY_NSPIRE_6.row && key.col == KEY_NSPIRE_6.col)
            return is_down(SDLK_d) || is_down(SDLK_RIGHT);

        if(key.row == KEY_NSPIRE_5.row && key.col == KEY_NSPIRE_5.col)
            return is_down(SDLK_SPACE) || is_down(SDLK_RETURN);

        if(key.row == KEY_NSPIRE_7.row && key.col == KEY_NSPIRE_7.col)
            return (mouse & SDL_BUTTON(SDL_BUTTON_LEFT)) != 0 || is_down(SDLK_e);
        if(key.row == KEY_NSPIRE_9.row && key.col == KEY_NSPIRE_9.col)
            return (mouse & SDL_BUTTON(SDL_BUTTON_RIGHT)) != 0 || is_down(SDLK_q);

        if(key.row == KEY_NSPIRE_1.row && key.col == KEY_NSPIRE_1.col)
            return is_down(SDLK_1) || is_down(SDLK_LEFTBRACKET);
        if(key.row == KEY_NSPIRE_3.row && key.col == KEY_NSPIRE_3.col)
            return is_down(SDLK_3) || is_down(SDLK_RIGHTBRACKET);

        if(key.row == KEY_NSPIRE_PERIOD.row && key.col == KEY_NSPIRE_PERIOD.col)
            return is_down(SDLK_r) || is_down(SDLK_PERIOD);
        if(key.row == KEY_NSPIRE_MENU.row && key.col == KEY_NSPIRE_MENU.col)
            return is_down(SDLK_TAB);

        if(key.row == KEY_NSPIRE_UP.row && key.col == KEY_NSPIRE_UP.col)
            return is_down(SDLK_UP);
        if(key.row == KEY_NSPIRE_DOWN.row && key.col == KEY_NSPIRE_DOWN.col)
            return is_down(SDLK_DOWN);
        if(key.row == KEY_NSPIRE_LEFT.row && key.col == KEY_NSPIRE_LEFT.col)
            return is_down(SDLK_LEFT);
        if(key.row == KEY_NSPIRE_RIGHT.row && key.col == KEY_NSPIRE_RIGHT.col)
            return is_down(SDLK_RIGHT);

        if(key.row == KEY_NSPIRE_PLUS.row && key.col == KEY_NSPIRE_PLUS.col)
            return is_down(SDLK_EQUALS) || is_down(SDLK_PLUS);
        if(key.row == KEY_NSPIRE_MINUS.row && key.col == KEY_NSPIRE_MINUS.col)
            return is_down(SDLK_MINUS);
        if(key.row == KEY_NSPIRE_CTRL.row && key.col == KEY_NSPIRE_CTRL.col)
            return is_down(SDLK_LCTRL) || is_down(SDLK_RCTRL) || is_down(SDLK_LMETA) || is_down(SDLK_RMETA);

        return false;
    #endif
}

void Task::initializeGlobals(const char *savefile)
{
    running = true;

    screen = newTexture(SCREEN_WIDTH, SCREEN_HEIGHT);
    nglSetBuffer(screen->bitmap);

    has_touchpad = is_touchpad;
    keys_inverted = is_classic;

    background = newTexture(SCREEN_WIDTH, SCREEN_HEIGHT);
    background_saved = false;

    Task::savefile = savefile;
}

void Task::deinitializeGlobals()
{
    deleteTexture(screen);
    deleteTexture(background);
}

void Task::saveBackground()
{
    copyTexture(*screen, *background);

    background_saved = true;
}

void Task::drawBackground()
{
    copyTexture(*background, *screen);
}

/* Version 2: First in git
 * Version 3 (31d5ee3a): Blocks are stored as 16bit BLOCK_WDATA
 * Version 4 (1ebc685a): Add inventory
 * Version 5 (710e7269): Add settings
 * Version 6 (d52f3992): BLOCK_SIZE changed from 120 to 128,
 *                       gzip compression introduced shortly afterwards
 * Version 7: Inventory stacks now store per-slot item counts
 * Version 8: Inventory expanded to 36 slots (9 hotbar + 27 storage)
 */
static constexpr int savefile_version = 8;

#define LOAD_FROM_FILE(var) if(gzfread(&var, sizeof(var), 1, file) != 1) { gzclose(file); return false; }
#define SAVE_TO_FILE(var) if(gzfwrite(&var, sizeof(var), 1, file) != 1) { gzclose(file); return false; }

bool Task::load()
{
    // Versions before 6 (and 6 for a short time) were uncompressed,
    // but gzopen detects and handles uncompressed files transparently.
    // Previous versions read the gzip magic as savefile version and bail out.
    gzFile file = gzopen(savefile, "rb");
    if(!file)
        return false;

    int version;
    LOAD_FROM_FILE(version);

    static_assert(savefile_version == 8, "Adjust loading code for backward compatibility");

    if(version < 4 || version > 8)
    {
        printf("Save file version %d not supported!\n", version);
        gzclose(file);
        return false;
    }

    if(!settings_task.loadFromFile(file, version))
    {
        gzclose(file);
        return false;
    }

    if(version >= 8)
    {
        LOAD_FROM_FILE(current_inventory.entries)
        LOAD_FROM_FILE(current_inventory.counts)
    }
    else
    {
        BLOCK_WDATA old_entries[5] = {};
        unsigned int old_counts[5] = {};

        if(gzfread(old_entries, sizeof(old_entries), 1, file) != 1)
        {
            gzclose(file);
            return false;
        }

        if(version >= 7)
        {
            if(gzfread(old_counts, sizeof(old_counts), 1, file) != 1)
            {
                gzclose(file);
                return false;
            }
        }

        for(int i = 0; i < Inventory::slot_count; ++i)
            current_inventory.setSlot(i, BLOCK_AIR, 0);

        for(int i = 0; i < 5; ++i)
        {
            const unsigned int count = (version >= 7) ? old_counts[i] : 1;
            current_inventory.setSlot(i, old_entries[i], count);
        }
    }

    current_inventory.importLegacyCounts();

    LOAD_FROM_FILE(world_task.xr)
    LOAD_FROM_FILE(world_task.yr)
    LOAD_FROM_FILE(world_task.x)
    LOAD_FROM_FILE(world_task.y)
    LOAD_FROM_FILE(world_task.z)
    // Previous versions used BLOCK_SIZE 120
    if(version < 6)
    {
        world_task.x = world_task.x * BLOCK_SIZE / 120;
        world_task.y = world_task.y * BLOCK_SIZE / 120;
        world_task.z = world_task.z * BLOCK_SIZE / 120;
    }

    LOAD_FROM_FILE(current_inventory.current_slot)

    LOAD_FROM_FILE(block_list_task.current_selection)

    const bool ret = world.loadFromFile(file);

    gzclose(file);

    world.setPosition(world_task.x, world_task.y, world_task.z);

    return ret;
}

bool Task::save()
{
    gzFile file = gzopen(savefile, "wb");
    if(!file)
        return false;

    SAVE_TO_FILE(savefile_version)
    if(!settings_task.saveToFile(file))
    {
        gzclose(file);
        return false;
    }
    SAVE_TO_FILE(current_inventory.entries)
    SAVE_TO_FILE(current_inventory.counts)
    SAVE_TO_FILE(world_task.xr)
    SAVE_TO_FILE(world_task.yr)
    SAVE_TO_FILE(world_task.x)
    SAVE_TO_FILE(world_task.y)
    SAVE_TO_FILE(world_task.z)
    SAVE_TO_FILE(current_inventory.current_slot)
    SAVE_TO_FILE(block_list_task.current_selection)

    const bool ret = world.saveToFile(file);

    gzclose(file);

    return ret;
}
