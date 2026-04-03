#include "blocklisttask.h"

#include <algorithm>

#include "blockrenderer.h"
#include "font.h"
#include "inventory.h"
#include "itemicons.h"
#include "terrain.h"
#include "texturetools.h"
#include "worldtask.h"
#include "textures/items.h"

BlockListTask block_list_task;

static const BLOCK_WDATA user_selectable[] = {
    BLOCK_STONE,
    BLOCK_COBBLESTONE,
    BLOCK_DIRT,
    BLOCK_GRASS,
    BLOCK_SAND,
    BLOCK_WOOD,
    BLOCK_LEAVES,
    BLOCK_PLANKS_NORMAL,
    BLOCK_PLANKS_DARK,
    BLOCK_PLANKS_BRIGHT,
    BLOCK_WALL,
    BLOCK_GLASS,
    BLOCK_DOOR,
    BLOCK_COAL_ORE,
    BLOCK_GOLD_ORE,
    BLOCK_IRON_ORE,
    BLOCK_DIAMOND_ORE,
    BLOCK_REDSTONE_ORE,
    BLOCK_IRON,
    BLOCK_GOLD,
    BLOCK_DIAMOND,
    BLOCK_GLOWSTONE,
    BLOCK_NETHERRACK,
    BLOCK_TNT,
    BLOCK_SPONGE,
    BLOCK_FURNACE,
    BLOCK_CRAFTING_TABLE,
    BLOCK_BOOKSHELF,
    BLOCK_PUMPKIN,
    getBLOCKWDATA(BLOCK_WATER, RANGE_WATER),
    getBLOCKWDATA(BLOCK_LAVA, RANGE_LAVA),
    getBLOCKWDATA(BLOCK_FLOWER, 0),
    getBLOCKWDATA(BLOCK_FLOWER, 1),
    getBLOCKWDATA(BLOCK_MUSHROOM, 0),
    getBLOCKWDATA(BLOCK_MUSHROOM, 1),
    getBLOCKWDATA(BLOCK_WHEAT, 0),
    BLOCK_SPIDERWEB,
    BLOCK_TORCH,
    BLOCK_CAKE,
    BLOCK_REDSTONE_LAMP,
    BLOCK_REDSTONE_SWITCH,
    BLOCK_PRESSURE_PLATE,
    BLOCK_REDSTONE_WIRE,
    BLOCK_REDSTONE_TORCH
};

static const BLOCK_WDATA user_items_page_1[] = {
    getBLOCKWDATA(BLOCK_ITEM, static_cast<uint8_t>(ItemTexture::STICK)),
    getBLOCKWDATA(BLOCK_ITEM, static_cast<uint8_t>(ItemTexture::COAL)),
    getBLOCKWDATA(BLOCK_ITEM, static_cast<uint8_t>(ItemTexture::CHARCOAL)),
    getBLOCKWDATA(BLOCK_ITEM, static_cast<uint8_t>(ItemTexture::REDSTONE_DUST)),
    getBLOCKWDATA(BLOCK_ITEM, static_cast<uint8_t>(ItemTexture::BREAD)),
    getBLOCKWDATA(BLOCK_ITEM, static_cast<uint8_t>(ItemTexture::BUCKET)),
    getBLOCKWDATA(BLOCK_ITEM, static_cast<uint8_t>(ItemTexture::BOWL)),
    getBLOCKWDATA(BLOCK_ITEM, static_cast<uint8_t>(ItemTexture::WOODEN_PICKAXE)),
    getBLOCKWDATA(BLOCK_ITEM, static_cast<uint8_t>(ItemTexture::STONE_PICKAXE)),
    getBLOCKWDATA(BLOCK_ITEM, static_cast<uint8_t>(ItemTexture::IRON_PICKAXE)),
    getBLOCKWDATA(BLOCK_ITEM, static_cast<uint8_t>(ItemTexture::DIAMOND_PICKAXE)),
    getBLOCKWDATA(BLOCK_ITEM, static_cast<uint8_t>(ItemTexture::GOLDEN_PICKAXE)),
    getBLOCKWDATA(BLOCK_ITEM, static_cast<uint8_t>(ItemTexture::WOODEN_AXE)),
    getBLOCKWDATA(BLOCK_ITEM, static_cast<uint8_t>(ItemTexture::STONE_AXE)),
    getBLOCKWDATA(BLOCK_ITEM, static_cast<uint8_t>(ItemTexture::IRON_AXE)),
    getBLOCKWDATA(BLOCK_ITEM, static_cast<uint8_t>(ItemTexture::DIAMOND_AXE)),
    getBLOCKWDATA(BLOCK_ITEM, static_cast<uint8_t>(ItemTexture::GOLDEN_AXE)),
    getBLOCKWDATA(BLOCK_ITEM, static_cast<uint8_t>(ItemTexture::WOODEN_SHOVEL)),
    getBLOCKWDATA(BLOCK_ITEM, static_cast<uint8_t>(ItemTexture::STONE_SHOVEL)),
    getBLOCKWDATA(BLOCK_ITEM, static_cast<uint8_t>(ItemTexture::IRON_SHOVEL)),
    getBLOCKWDATA(BLOCK_ITEM, static_cast<uint8_t>(ItemTexture::DIAMOND_SHOVEL)),
    getBLOCKWDATA(BLOCK_ITEM, static_cast<uint8_t>(ItemTexture::GOLDEN_SHOVEL)),
    getBLOCKWDATA(BLOCK_ITEM, static_cast<uint8_t>(ItemTexture::WOODEN_HOE)),
    getBLOCKWDATA(BLOCK_ITEM, static_cast<uint8_t>(ItemTexture::STONE_HOE)),
    getBLOCKWDATA(BLOCK_ITEM, static_cast<uint8_t>(ItemTexture::IRON_HOE)),
    getBLOCKWDATA(BLOCK_ITEM, static_cast<uint8_t>(ItemTexture::DIAMOND_HOE)),
    getBLOCKWDATA(BLOCK_ITEM, static_cast<uint8_t>(ItemTexture::GOLDEN_HOE)),
    getBLOCKWDATA(BLOCK_ITEM, static_cast<uint8_t>(ItemTexture::WOODEN_SWORD)),
    getBLOCKWDATA(BLOCK_ITEM, static_cast<uint8_t>(ItemTexture::STONE_SWORD)),
    getBLOCKWDATA(BLOCK_ITEM, static_cast<uint8_t>(ItemTexture::IRON_SWORD)),
    getBLOCKWDATA(BLOCK_ITEM, static_cast<uint8_t>(ItemTexture::DIAMOND_SWORD)),
    getBLOCKWDATA(BLOCK_ITEM, static_cast<uint8_t>(ItemTexture::GOLDEN_SWORD)),
};

static const BLOCK_WDATA user_items_page_2[] = {
    getBLOCKWDATA(BLOCK_ITEM, static_cast<uint8_t>(ItemTexture::IRON_HELMET)),
    getBLOCKWDATA(BLOCK_ITEM, static_cast<uint8_t>(ItemTexture::IRON_CHESTPLATE)),
    getBLOCKWDATA(BLOCK_ITEM, static_cast<uint8_t>(ItemTexture::IRON_LEGGINGS)),
    getBLOCKWDATA(BLOCK_ITEM, static_cast<uint8_t>(ItemTexture::IRON_BOOTS)),
    getBLOCKWDATA(BLOCK_ITEM, static_cast<uint8_t>(ItemTexture::GOLDEN_HELMET)),
    getBLOCKWDATA(BLOCK_ITEM, static_cast<uint8_t>(ItemTexture::GOLDEN_CHESTPLATE)),
    getBLOCKWDATA(BLOCK_ITEM, static_cast<uint8_t>(ItemTexture::GOLDEN_LEGGINGS)),
    getBLOCKWDATA(BLOCK_ITEM, static_cast<uint8_t>(ItemTexture::GOLDEN_BOOTS)),
    getBLOCKWDATA(BLOCK_ITEM, static_cast<uint8_t>(ItemTexture::DIAMOND_HELMET)),
    getBLOCKWDATA(BLOCK_ITEM, static_cast<uint8_t>(ItemTexture::DIAMOND_CHESTPLATE)),
    getBLOCKWDATA(BLOCK_ITEM, static_cast<uint8_t>(ItemTexture::DIAMOND_LEGGINGS)),
    getBLOCKWDATA(BLOCK_ITEM, static_cast<uint8_t>(ItemTexture::DIAMOND_BOOTS)),
    getBLOCKWDATA(BLOCK_ITEM, static_cast<uint8_t>(ItemTexture::ARROW)),
    getBLOCKWDATA(BLOCK_ITEM, static_cast<uint8_t>(ItemTexture::APPLE)),
    getBLOCKWDATA(BLOCK_ITEM, static_cast<uint8_t>(ItemTexture::GOLDEN_APPLE)),
    getBLOCKWDATA(BLOCK_ITEM, static_cast<uint8_t>(ItemTexture::MAP)),
    getBLOCKWDATA(BLOCK_ITEM, static_cast<uint8_t>(ItemTexture::BOOK)),
    getBLOCKWDATA(BLOCK_ITEM, static_cast<uint8_t>(ItemTexture::PAPER)),
    getBLOCKWDATA(BLOCK_ITEM, static_cast<uint8_t>(ItemTexture::COMPASS)),
    getBLOCKWDATA(BLOCK_ITEM, static_cast<uint8_t>(ItemTexture::CLOCK)),
    getBLOCKWDATA(BLOCK_ITEM, static_cast<uint8_t>(ItemTexture::OAK_DOOR)),
    getBLOCKWDATA(BLOCK_ITEM, static_cast<uint8_t>(ItemTexture::IRON_DOOR)),
    getBLOCKWDATA(BLOCK_ITEM, static_cast<uint8_t>(ItemTexture::MELON_SLICE)),
    getBLOCKWDATA(BLOCK_ITEM, static_cast<uint8_t>(ItemTexture::PUMPKIN_PIE)),
    getBLOCKWDATA(BLOCK_ITEM, static_cast<uint8_t>(ItemTexture::COOKIE)),
    getBLOCKWDATA(BLOCK_ITEM, static_cast<uint8_t>(ItemTexture::RAW_BEEF)),
    getBLOCKWDATA(BLOCK_ITEM, static_cast<uint8_t>(ItemTexture::COOKED_BEEF)),
    getBLOCKWDATA(BLOCK_ITEM, static_cast<uint8_t>(ItemTexture::RAW_CHICKEN)),
    getBLOCKWDATA(BLOCK_ITEM, static_cast<uint8_t>(ItemTexture::COOKED_CHICKEN)),
    getBLOCKWDATA(BLOCK_ITEM, static_cast<uint8_t>(ItemTexture::ROTTEN_FLESH)),
};

struct SelectablePage {
    const char *name;
    const BLOCK_WDATA *entries;
    int count;
};

constexpr int user_selectable_count = sizeof(user_selectable)/sizeof(*user_selectable);
constexpr int user_items_page_1_count = sizeof(user_items_page_1)/sizeof(*user_items_page_1);
constexpr int user_items_page_2_count = sizeof(user_items_page_2)/sizeof(*user_items_page_2);

static const SelectablePage selectable_pages[] = {
    {"Blocks", user_selectable, user_selectable_count},
    {"Items 1", user_items_page_1, user_items_page_1_count},
    {"Items 2", user_items_page_2, user_items_page_2_count},
};

constexpr int selectable_page_count = sizeof(selectable_pages) / sizeof(*selectable_pages);
//The values have to stay somewhere
unsigned int BlockListTask::blocklist_top;
//Black texture as background
TEXTURE *BlockListTask::blocklist_background;

BlockListTask::BlockListTask()
{
    blocklist_top = (SCREEN_HEIGHT - blocklist_height - current_inventory.height()) / 2;

    static_assert(field_width * fields_x <= SCREEN_WIDTH, "fields_x too high");
    static_assert(fields_x * fields_y >= 1, "Not enough fields");
    if(blocklist_height + current_inventory.height() > SCREEN_WIDTH)
        printf("fields_y too high\n");

    blocklist_background = newTexture(blocklist_width, blocklist_height, 0, false);
}

BlockListTask::~BlockListTask()
{
    deleteTexture(blocklist_background);
}

void BlockListTask::makeCurrent()
{
    if(!background_saved)
        saveBackground();

    Task::makeCurrent();
}

void BlockListTask::render()
{
    drawBackground();

    drawTextureOverlay(*blocklist_background, 0, 0, *screen, blocklist_left, blocklist_top, blocklist_background->width, blocklist_background->height);

    const SelectablePage &page = selectable_pages[current_page];

    int block_nr = 0;
    int screen_x, screen_y = blocklist_top + pad_y;
    for(int y = 0; y < fields_y; y++, screen_y += field_height)
    {
        screen_x = blocklist_left + pad_x;
        for(int x = 0; x < fields_x; x++, screen_x += field_width)
        {
            if(block_nr >= page.count)
                goto end;

            const BLOCK_WDATA entry = page.entries[block_nr];

            if(getBLOCK(entry) == BLOCK_ITEM)
                drawItemIcon(entry, *screen, screen_x + pad_x, screen_y + pad_y, 24);
            //BLOCK_DOOR is twice as high, so center it manually
            else if(getBLOCK(entry) == BLOCK_DOOR)
                global_block_renderer.drawPreview(page.entries[block_nr], *screen, screen_x + pad_x, screen_y + pad_y_door);
            else
                global_block_renderer.drawPreview(page.entries[block_nr], *screen, screen_x + pad_y, screen_y + pad_y);

            block_nr++;
        }
    }

    end:

    //Draw the selection indicator
    screen_x = blocklist_left + pad_x + field_width * (current_selection % fields_x);
    screen_y = blocklist_top + pad_y + field_height * (current_selection / fields_x);
    drawTexture(*inv_selection_p, *screen, 0, 0, inv_selection_p->width, inv_selection_p->height, screen_x + pad_x - 11, screen_y + pad_y - 10, inv_selection_p->width, inv_selection_p->height);

    current_inventory.draw(*screen);
    const int page_title_y = static_cast<int>(blocklist_top) - static_cast<int>(fontHeight());
    drawStringCenter(page.name, 0xFFFF, *screen, SCREEN_WIDTH / 2, std::max(0, page_title_y));
    drawStringCenter("7/9 Change Page", 0xFFFF, *screen, SCREEN_WIDTH / 2, SCREEN_HEIGHT - current_inventory.height() - fontHeight() * 2);
    if(page.count > 0)
        drawStringCenter(global_block_renderer.getName(page.entries[current_selection]), 0xFFFF, *screen, SCREEN_WIDTH / 2, SCREEN_HEIGHT - current_inventory.height() - fontHeight());
}

void BlockListTask::logic(GLFix /*dt*/)
{
    if(key_held_down)
        key_held_down = keyPressed(KEY_NSPIRE_ESC) || keyPressed(KEY_NSPIRE_PERIOD) || keyPressed(KEY_NSPIRE_2) || keyPressed(KEY_NSPIRE_8) || keyPressed(KEY_NSPIRE_4) || keyPressed(KEY_NSPIRE_6) || keyPressed(KEY_NSPIRE_7) || keyPressed(KEY_NSPIRE_9) || keyPressed(KEY_NSPIRE_1) || keyPressed(KEY_NSPIRE_3) || keyPressed(KEY_NSPIRE_5) || keyPressed(KEY_NSPIRE_UP) || keyPressed(KEY_NSPIRE_DOWN) || keyPressed(KEY_NSPIRE_LEFT) || keyPressed(KEY_NSPIRE_RIGHT)  || keyPressed(KEY_NSPIRE_CLICK);
    else if(keyPressed(KEY_NSPIRE_ESC) || keyPressed(KEY_NSPIRE_PERIOD))
    {
        world_task.makeCurrent();

        key_held_down = true;
    }
    else if(keyPressed(KEY_NSPIRE_7))
    {
        const int old_page = current_page;
        current_page--;
        if(current_page < 0)
            current_page = selectable_page_count - 1;

        const int page_count = selectable_pages[current_page].count;
        if(current_page != old_page)
            current_selection = std::min(current_selection, std::max(0, page_count - 1));

        key_held_down = true;
    }
    else if(keyPressed(KEY_NSPIRE_9))
    {
        const int old_page = current_page;
        current_page = (current_page + 1) % selectable_page_count;

        const int page_count = selectable_pages[current_page].count;
        if(current_page != old_page)
            current_selection = std::min(current_selection, std::max(0, page_count - 1));

        key_held_down = true;
    }
    else if(keyPressed(KEY_NSPIRE_2) || keyPressed(KEY_NSPIRE_DOWN))
    {
        const int page_count = selectable_pages[current_page].count;
        current_selection += fields_x;
        if(current_selection >= page_count)
            current_selection %= fields_x;

        key_held_down = true;
    }
    else if(keyPressed(KEY_NSPIRE_8) || keyPressed(KEY_NSPIRE_UP))
    {
        const int page_count = selectable_pages[current_page].count;
        if(current_selection >= fields_x)
            current_selection -= fields_x;
        else
        {
            current_selection = ((page_count - 1) / fields_x) * fields_x + (current_selection % fields_x);
            if(current_selection >= page_count)
                current_selection -= fields_x;
        }

        key_held_down = true;
    }
    else if(keyPressed(KEY_NSPIRE_4) || keyPressed(KEY_NSPIRE_LEFT))
    {
        const int page_count = selectable_pages[current_page].count;
        if(current_selection % fields_x == 0)
        {
            current_selection += fields_x - 1;
            if(current_selection >= page_count)
                current_selection = page_count - 1;
        }
        else
            current_selection--;

        key_held_down = true;
    }
    else if(keyPressed(KEY_NSPIRE_6) || keyPressed(KEY_NSPIRE_RIGHT))
    {
        const int page_count = selectable_pages[current_page].count;
        if(current_selection % fields_x != fields_x-1 && current_selection < page_count - 1)
            current_selection++;
        else
            current_selection -= current_selection % fields_x;

        key_held_down = true;
    }
    else if(keyPressed(KEY_NSPIRE_1)) //Switch inventory slot
    {
        current_inventory.previousSlot();

        key_held_down = true;
    }
    else if(keyPressed(KEY_NSPIRE_3))
    {
        current_inventory.nextSlot();

        key_held_down = true;
    }
    else if(keyPressed(KEY_NSPIRE_5) || keyPressed(KEY_NSPIRE_CLICK))
    {
        const SelectablePage &page = selectable_pages[current_page];
        if(page.count > 0)
            current_inventory.setCurrentSlot(page.entries[current_selection], 64);

        key_held_down = true;
    }
}
