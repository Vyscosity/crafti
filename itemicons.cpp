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

TextureAtlasEntry itemIconAtlasUV(BLOCK_WDATA block)
{
    TextureAtlasEntry e{};
    if(getBLOCK(block) != BLOCK_ITEM)
        return e;

    const int item_index = static_cast<int>(getBLOCKDATA(block));
    const int src_x = (item_index % item_atlas_cols) * item_tile_size;
    const int src_y = (item_index / item_atlas_cols) * item_tile_size;

    if(src_x < 0 || src_y < 0 || src_x + item_tile_size > tex_items.width || src_y + item_tile_size > tex_items.height)
        return e;

    e.left = static_cast<unsigned>(src_x);
    e.right = static_cast<unsigned>(src_x + item_tile_size);
    e.bottom = static_cast<unsigned>(src_y);
    e.top = static_cast<unsigned>(src_y + item_tile_size);
    return e;
}

const TEXTURE *itemsTextureAtlas()
{
    return &tex_items;
}

const char *getItemName(BLOCK_WDATA block)
{
    if(getBLOCK(block) != BLOCK_ITEM)
        return nullptr;

    switch(static_cast<ItemTexture>(getBLOCKDATA(block)))
    {
    case ItemTexture::COAL:
        return "Coal";
    case ItemTexture::CHARCOAL:
        return "Charcoal";
    case ItemTexture::STICK:
        return "Stick";
    case ItemTexture::REDSTONE_DUST:
        return "Redstone Dust";
    case ItemTexture::BREAD:
        return "Bread";
    case ItemTexture::BUCKET:
        return "Bucket";
    case ItemTexture::BOWL:
        return "Bowl";
    case ItemTexture::WOODEN_SWORD:
        return "Wooden Sword";
    case ItemTexture::STONE_SWORD:
        return "Stone Sword";
    case ItemTexture::IRON_SWORD:
        return "Iron Sword";
    case ItemTexture::DIAMOND_SWORD:
        return "Diamond Sword";
    case ItemTexture::GOLDEN_SWORD:
        return "Golden Sword";
    case ItemTexture::WOODEN_PICKAXE:
        return "Wooden Pickaxe";
    case ItemTexture::STONE_PICKAXE:
        return "Stone Pickaxe";
    case ItemTexture::IRON_PICKAXE:
        return "Iron Pickaxe";
    case ItemTexture::DIAMOND_PICKAXE:
        return "Diamond Pickaxe";
    case ItemTexture::GOLDEN_PICKAXE:
        return "Golden Pickaxe";
    case ItemTexture::WOODEN_AXE:
        return "Wooden Axe";
    case ItemTexture::STONE_AXE:
        return "Stone Axe";
    case ItemTexture::IRON_AXE:
        return "Iron Axe";
    case ItemTexture::DIAMOND_AXE:
        return "Diamond Axe";
    case ItemTexture::GOLDEN_AXE:
        return "Golden Axe";
    case ItemTexture::WOODEN_SHOVEL:
        return "Wooden Shovel";
    case ItemTexture::STONE_SHOVEL:
        return "Stone Shovel";
    case ItemTexture::IRON_SHOVEL:
        return "Iron Shovel";
    case ItemTexture::DIAMOND_SHOVEL:
        return "Diamond Shovel";
    case ItemTexture::GOLDEN_SHOVEL:
        return "Golden Shovel";
    case ItemTexture::WOODEN_HOE:
        return "Wooden Hoe";
    case ItemTexture::STONE_HOE:
        return "Stone Hoe";
    case ItemTexture::IRON_HOE:
        return "Iron Hoe";
    case ItemTexture::DIAMOND_HOE:
        return "Diamond Hoe";
    case ItemTexture::GOLDEN_HOE:
        return "Golden Hoe";
    case ItemTexture::IRON_HELMET:
        return "Iron Helmet";
    case ItemTexture::IRON_CHESTPLATE:
        return "Iron Chestplate";
    case ItemTexture::IRON_LEGGINGS:
        return "Iron Leggings";
    case ItemTexture::IRON_BOOTS:
        return "Iron Boots";
    case ItemTexture::GOLDEN_HELMET:
        return "Golden Helmet";
    case ItemTexture::GOLDEN_CHESTPLATE:
        return "Golden Chestplate";
    case ItemTexture::GOLDEN_LEGGINGS:
        return "Golden Leggings";
    case ItemTexture::GOLDEN_BOOTS:
        return "Golden Boots";
    case ItemTexture::DIAMOND_HELMET:
        return "Diamond Helmet";
    case ItemTexture::DIAMOND_CHESTPLATE:
        return "Diamond Chestplate";
    case ItemTexture::DIAMOND_LEGGINGS:
        return "Diamond Leggings";
    case ItemTexture::DIAMOND_BOOTS:
        return "Diamond Boots";
    case ItemTexture::RAW_CHICKEN:
        return "Raw Chicken";
    case ItemTexture::COOKED_CHICKEN:
        return "Cooked Chicken";
    case ItemTexture::IRON_INGOT:
        return "Iron Ingot";
    case ItemTexture::GOLD_INGOT:
        return "Gold Ingot";
    case ItemTexture::DIAMOND:
        return "Diamond";
    case ItemTexture::RAW_BEEF:
        return "Raw Beef";
    case ItemTexture::COOKED_BEEF:
        return "Cooked Beef";
    case ItemTexture::RAW_PORKCHOP:
        return "Raw Porkchop";
    case ItemTexture::COOKED_PORKCHOP:
        return "Cooked Porkchop";
    case ItemTexture::RAW_COD:
        return "Raw Cod";
    case ItemTexture::COOKED_COD:
        return "Cooked Cod";
    case ItemTexture::RAW_SALMON:
        return "Raw Salmon";
    case ItemTexture::COOKED_SALMON:
        return "Cooked Salmon";
    case ItemTexture::POTATO:
        return "Potato";
    case ItemTexture::BAKED_POTATO:
        return "Baked Potato";
    case ItemTexture::COD:
        return "Cod";
    case ItemTexture::SALMON:
        return "Salmon";
    case ItemTexture::TROPICAL_FISH:
        return "Tropical Fish";
    case ItemTexture::PUFFERFISH:
        return "Pufferfish";
    case ItemTexture::ROTTEN_FLESH:
        return "Rotten Flesh";
    case ItemTexture::GUNPOWDER:
        return "Gunpowder";
    default:
        return "Item";
    }
}