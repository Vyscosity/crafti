#include "menutask.h"

#include "texturetools.h"
#include "worldtask.h"
#include "helptask.h"
#include "settingstask.h"
#include "starttask.h"
#include "font.h"

MenuTask menu_task;

MenuTask::MenuTask()
{
}

MenuTask::~MenuTask()
{
}

void MenuTask::makeCurrent()
{
    menu_selected_item = RESUME;

    if(!background_saved)
        saveBackground();

    Task::makeCurrent();
}

static void fillRect(TEXTURE &tex, int x, int y, int w, int h, COLOR c)
{
    if(x >= (int)tex.width || y >= (int)tex.height || w <= 0 || h <= 0)
        return;

    if(x < 0)
    {
        w += x;
        x = 0;
    }
    if(y < 0)
    {
        h += y;
        y = 0;
    }
    if(x + w > (int)tex.width)
        w = tex.width - x;
    if(y + h > (int)tex.height)
        h = tex.height - y;

    for(int yy = 0; yy < h; ++yy)
    {
        COLOR *line = tex.bitmap + (y + yy) * tex.width + x;
        for(int xx = 0; xx < w; ++xx)
            line[xx] = c;
    }
}

void MenuTask::render()
{
    drawBackground();

    // Darken background
    for(int i = 0; i < SCREEN_WIDTH * SCREEN_HEIGHT; ++i)
    {
        COLOR c = screen->bitmap[i];
        // Simple RGB565 darkening: (c & 0xF7DE) >> 1
        screen->bitmap[i] = (c & 0xF7DE) >> 1;
    }

    drawStringCenter("Game Menu", 0xFFFF, *screen, SCREEN_WIDTH / 2, 30);

    const char *items[MENU_ITEM_MAX] = { "Back to Game", "Settings", "Help", "Save World", "Quit to Title" };

    int start_y = 60;
    int button_w = 200;
    int button_h = 20;
    int button_x = (SCREEN_WIDTH - button_w) / 2;

    for(int i = 0; i < MENU_ITEM_MAX; ++i)
    {
        int y = start_y + i * 26;
        COLOR label_color = 0xFFFF;

        if(i == menu_selected_item)
        {
            fillRect(*screen, button_x, y, button_w, button_h, 0x7BEF); // Minecraft-like selection color
            drawRectangle(*screen, button_x, y, button_w, button_h, 0xFFFF);
            label_color = 0x0000;
        }
        else
        {
            fillRect(*screen, button_x, y, button_w, button_h, 0x4208);
            drawRectangle(*screen, button_x, y, button_w, button_h, 0x8410);
        }

        drawStringCenter(items[i], label_color, *screen, SCREEN_WIDTH / 2, y + 4);
    }
}

void MenuTask::logic()
{
    if(key_held_down)
        key_held_down = keyPressed(KEY_NSPIRE_ESC) || keyPressed(KEY_NSPIRE_MENU) || keyPressed(KEY_NSPIRE_UP) || keyPressed(KEY_NSPIRE_DOWN) || keyPressed(KEY_NSPIRE_2) || keyPressed(KEY_NSPIRE_8) || keyPressed(KEY_NSPIRE_5) || keyPressed(KEY_NSPIRE_CLICK);
    else if(keyPressed(KEY_NSPIRE_8) || keyPressed(KEY_NSPIRE_UP))
    {
        --menu_selected_item;
        if(menu_selected_item < 0)
            menu_selected_item = MENU_ITEM_MAX - 1;

        key_held_down = true;
    }
    else if(keyPressed(KEY_NSPIRE_2) || keyPressed(KEY_NSPIRE_DOWN))
    {
        ++menu_selected_item;
        if(menu_selected_item == MENU_ITEM_MAX)
            menu_selected_item = 0;

        key_held_down = true;
    }
    else if(keyPressed(KEY_NSPIRE_5) || keyPressed(KEY_NSPIRE_CLICK))
    {
        switch(menu_selected_item)
        {
        case RESUME:
            world_task.makeCurrent();
            break;

        case SETTINGS:
            settings_task.makeCurrent();
            break;

        case HELP:
            help_task.makeCurrent();
            break;

        case SAVE_WORLD:
            if(save())
                world_task.setMessage("World saved.");
            else
                world_task.setMessage("Failed to save world.");
            world_task.makeCurrent();
            break;

        case QUIT_TO_TITLE:
            #ifndef _TINSPIRE
                save();
            #endif
            start_task.makeCurrent();
            break;

        default:
            world_task.makeCurrent();
            break;
        }

        key_held_down = true;
    }
    else if(keyPressed(KEY_NSPIRE_MENU) || keyPressed(KEY_NSPIRE_ESC))
    {
        world_task.makeCurrent();
        key_held_down = true;
    }
}
