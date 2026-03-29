#include "starttask.h"

#include "font.h"
#include "worldtask.h"
#include "world.h"
#include "blockrenderer.h"
#include "terrain.h"
#include "texturetools.h"
#include "textures/selection.h"

extern unsigned char font_dat[];

StartTask start_task;

StartTask::StartTask()
{
}

StartTask::~StartTask()
{
}

void StartTask::makeCurrent()
{
    selected_item = NEW_TERRAIN;
    Task::makeCurrent();
}

static unsigned int measureTextWidth(const char *str)
{
    unsigned int w = 0;
    while(*str)
        w += font_dat[17 + static_cast<unsigned char>(*str++)];
    return w;
}

static void drawStringBigCenter(const char *str, COLOR color, TEXTURE &dest, int center_x, int center_y, int scale)
{
    if(scale <= 1)
    {
        drawStringCenter(str, color, dest, center_x, center_y);
        return;
    }

    unsigned int w = measureTextWidth(str);
    unsigned int h = fontHeight();
    if(w == 0 || h == 0)
    {
        drawStringCenter(str, color, dest, center_x, center_y);
        return;
    }

    TEXTURE *tmp = newTexture(w, h, 0, true, 0);
    drawString(str, color, *tmp, 0, 0);

    int big_w = w * scale;
    int big_h = h * scale;
    int dest_x = center_x - big_w / 2;
    int dest_y = center_y - big_h / 2;

    drawTexture(*tmp, dest, 0, 0, w, h, dest_x, dest_y, big_w, big_h);
    deleteTexture(tmp);
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

void StartTask::render()
{
    // Tile real dirt tile from terrain atlas for the background (16x16 block layout, each block zoomed 4x => 96px).
    const auto &dirt = terrain_atlas[2][0].resized; // [2][0] is dirt block texture in atlas.
    const int tile_size = 96; // 24 * 4 zoom
    const int cells_x = 16;
    const int cells_y = 16;
    const int total_width = cells_x * tile_size;
    const int total_height = cells_y * tile_size;
    const int offset_x = std::max(0, (SCREEN_WIDTH - total_width) / 2);
    const int offset_y = std::max(0, (SCREEN_HEIGHT - total_height) / 2);

    for(int cy = 0; cy < cells_y; ++cy)
    {
        for(int cx = 0; cx < cells_x; ++cx)
        {
            int px = offset_x + cx * tile_size;
            int py = offset_y + cy * tile_size;
            // Scale each dirt tile to 2x by setting destination width/height = tile_size.
            drawTexture(*terrain_resized,
                        *screen,
                        dirt.left, dirt.top,
                        dirt.right - dirt.left, dirt.bottom - dirt.top,
                        px, py,
                        tile_size, tile_size);
        }
    }

    // Title
    drawStringCenter("CRAFTI", 0xFFFF, *screen, SCREEN_WIDTH / 2, 12);

    // Subtitle
    drawStringCenter("Select world type", 0xFFFF, *screen, SCREEN_WIDTH / 2, 28);

    const char *items[START_ITEM_MAX] = { "Continue Saved World", "New Flat World", "New Terrain World", "Exit" };

    int menu_height = START_ITEM_MAX * 24 - 4;
    int start_y = (SCREEN_HEIGHT / 2) - menu_height / 2;

    for(int i = 0; i < START_ITEM_MAX; ++i)
    {
        int y = start_y + i * 24;
        int button_w = 180;
        int button_h = 20;
        int button_x = (SCREEN_WIDTH - button_w) / 2;

        // Label color
        COLOR label_color = 0xFFFF;

        if(i == selected_item)
        {
            // selected button background
            fillRect(*screen, button_x, y, button_w, button_h, 0x7BEF);
            drawRectangle(*screen, button_x, y, button_w, button_h, 0xFFFF);
            label_color = 0x0000;
        }
        else
        {
            drawRectangle(*screen, button_x, y, button_w, button_h, 0xFFFF);
        }

        drawStringCenter(items[i], label_color, *screen, SCREEN_WIDTH / 2, y + 4);
    }

    drawStringCenter("Use 8/2 or Up/Down to move; 5 or Return to select", 0xFFFF, *screen, SCREEN_WIDTH / 2, SCREEN_HEIGHT - 16);
}

void StartTask::logic()
{
    if(key_held_down)
        key_held_down = keyPressed(KEY_NSPIRE_ESC) || keyPressed(KEY_NSPIRE_UP) || keyPressed(KEY_NSPIRE_DOWN) || keyPressed(KEY_NSPIRE_2) || keyPressed(KEY_NSPIRE_8);
    else if(keyPressed(KEY_NSPIRE_UP) || keyPressed(KEY_NSPIRE_8))
    {
        --selected_item;
        if(selected_item < 0)
            selected_item = START_ITEM_MAX - 1;
        key_held_down = true;
    }
    else if(keyPressed(KEY_NSPIRE_DOWN) || keyPressed(KEY_NSPIRE_2))
    {
        ++selected_item;
        if(selected_item == START_ITEM_MAX)
            selected_item = 0;
        key_held_down = true;
    }
    else if(keyPressed(KEY_NSPIRE_5))
    {
        switch(selected_item)
        {
        case CONTINUE:
            if(has_saved_world)
                world_task.makeCurrent();
            break;
        case NEW_FLAT:
            world.setWorldType(World::WorldType::Flat);
            world_task.resetWorld();
            world_task.makeCurrent();
            break;
        case NEW_TERRAIN:
            world.setWorldType(World::WorldType::Terrain);
            world_task.resetWorld();
            world_task.makeCurrent();
            break;
        case EXIT:
            running = false;
            break;
        }

        key_held_down = true;
    }
    else if(keyPressed(KEY_NSPIRE_ESC))
    {
        if(has_saved_world)
            world_task.makeCurrent();
        else
            running = false;

        key_held_down = true;
    }
}
