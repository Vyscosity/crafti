#include "itemicons.h"

#include "texturetools.h"

#include "textures/items.h"
#include "textures/items_texture.h"

namespace {
constexpr int item_atlas_cols = 16;
constexpr int item_tile_size = 16;
}

void drawItemIcon(BLOCK_WDATA block, TEXTURE &dest, int x, int y, int size)
{
    if(getBLOCK(block) != BLOCK_ITEM)
        return;

    if(size <= 0)
        return;

    const int item_index = static_cast<int>(getBLOCKDATA(block));
    const int src_x = (item_index % item_atlas_cols) * item_tile_size;
    const int src_y = (item_index / item_atlas_cols) * item_tile_size;

    if(src_x < 0 || src_y < 0 || src_x + item_tile_size > tex_items.width || src_y + item_tile_size > tex_items.height)
        return;

    // items_texture.h has no transparency metadata, so treat 0x0000 as transparent manually.
    for(int dy = 0; dy < size; ++dy)
    {
        const int dst_y = y + dy;
        if(dst_y < 0 || dst_y >= static_cast<int>(dest.height))
            continue;

        const int sy = src_y + (dy * item_tile_size) / size;
        for(int dx = 0; dx < size; ++dx)
        {
            const int dst_x = x + dx;
            if(dst_x < 0 || dst_x >= static_cast<int>(dest.width))
                continue;

            const int sx = src_x + (dx * item_tile_size) / size;
            const COLOR c = tex_items.bitmap[sy * tex_items.width + sx];
            if(c != 0x0000)
                dest.bitmap[dst_y * dest.width + dst_x] = c;
        }
    }
}

const char *getItemName(BLOCK_WDATA block)
{
    if(getBLOCK(block) != BLOCK_ITEM)
        return nullptr;

    switch(static_cast<ItemTexture>(getBLOCKDATA(block)))
    {
    case ItemTexture::STICK:
        return "Stick";
    default:
        return "Item";
    }
}