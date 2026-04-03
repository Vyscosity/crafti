#include "deathtask.h"

#include "texturetools.h"
#include "font.h"

#include "worldtask.h"
#include "starttask.h"

DeathTask death_task;

DeathTask::DeathTask()
{
}

DeathTask::~DeathTask()
{
}

void DeathTask::makeCurrent()
{
    death_selected_item = RESPAWN;

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

void DeathTask::render()
{
    drawBackground();

    // Darken background
    for(int i = 0; i < SCREEN_WIDTH * SCREEN_HEIGHT; ++i)
    {
        COLOR c = screen->bitmap[i];
        screen->bitmap[i] = (c & 0xF7DE) >> 1;
    }

    drawStringCenter("You Died", 0xFFFF, *screen, SCREEN_WIDTH / 2, 30);

    const char *items[DEATH_ITEM_MAX] = { "Respawn", "Quit to Title" };

    int start_y = 60;
    int button_w = 200;
    int button_h = 20;
    int button_x = (SCREEN_WIDTH - button_w) / 2;

    for(int i = 0; i < DEATH_ITEM_MAX; ++i)
    {
        int y = start_y + i * 26;
        COLOR label_color = 0xFFFF;

        if(i == death_selected_item)
        {
            fillRect(*screen, button_x, y, button_w, button_h, 0x7BEF);
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

void DeathTask::logic(GLFix /*dt*/)
{
    if(key_held_down)
        key_held_down = keyPressed(KEY_NSPIRE_ESC) || keyPressed(KEY_NSPIRE_MENU) || keyPressed(KEY_NSPIRE_UP) || keyPressed(KEY_NSPIRE_DOWN) || keyPressed(KEY_NSPIRE_2) || keyPressed(KEY_NSPIRE_8) || keyPressed(KEY_NSPIRE_5) || keyPressed(KEY_NSPIRE_CLICK) || keyPressed(KEY_NSPIRE_ENTER);
    else if(keyPressed(KEY_NSPIRE_8) || keyPressed(KEY_NSPIRE_UP))
    {
        --death_selected_item;
        if(death_selected_item < 0)
            death_selected_item = DEATH_ITEM_MAX - 1;
        key_held_down = true;
    }
    else if(keyPressed(KEY_NSPIRE_2) || keyPressed(KEY_NSPIRE_DOWN))
    {
        ++death_selected_item;
        if(death_selected_item == DEATH_ITEM_MAX)
            death_selected_item = 0;
        key_held_down = true;
    }
    else if(keyPressed(KEY_NSPIRE_5) || keyPressed(KEY_NSPIRE_CLICK) || keyPressed(KEY_NSPIRE_ENTER))
    {
        switch(death_selected_item)
        {
        case RESPAWN:
            world_task.respawnPlayer();
            world_task.makeCurrent();
            break;

        case QUIT_TO_TITLE:
            start_task.makeCurrent();
            break;

        default:
            world_task.respawnPlayer();
            world_task.makeCurrent();
            break;
        }

        key_held_down = true;
    }
    else if(keyPressed(KEY_NSPIRE_MENU) || keyPressed(KEY_NSPIRE_ESC))
    {
        // ESC acts like "Respawn" to mirror the feel of the in-game menu.
        world_task.respawnPlayer();
        world_task.makeCurrent();
        key_held_down = true;
    }
}

