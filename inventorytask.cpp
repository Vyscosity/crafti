#include "inventorytask.h"

#ifndef _TINSPIRE
#include <SDL/SDL.h>
#endif

#include <algorithm>
#include <cstdio>

#include "blockrenderer.h"
#include "font.h"
#include "inventory.h"
#include "itemicons.h"
#include "world.h"
#include "worldtask.h"

#include "textures/crafting_table.h"
#include "textures/furnace.h"
#include "textures/items.h"
#include "textures/inventory2.h"

InventoryTask inventory_task;

namespace {
constexpr int inv_src_slot_size = 16;
constexpr int inv_src_slot_gap = 2;
constexpr int inv_src_pitch = inv_src_slot_size + inv_src_slot_gap;
#ifdef _TINSPIRE
constexpr int inv_draw_scale = 1;
#else
constexpr int inv_draw_scale = 2;
#endif
constexpr int inv_draw_slot_size = inv_src_slot_size * inv_draw_scale;
constexpr int inv_draw_slot_gap = inv_src_slot_gap * inv_draw_scale;
constexpr int inv_draw_pitch = inv_draw_slot_size + inv_draw_slot_gap;
constexpr int inv_draw_slot_inset = 1 * inv_draw_scale;
constexpr int hotbar_src_x = 8;
constexpr int hotbar_src_y = 142;
constexpr int storage_src_x = 8;
constexpr int storage_src_y = 84;
constexpr int storage_cols = 9;
constexpr int storage_rows = 3;

// Crafting table panel coordinates in textures/crafting_table.png
constexpr int table_input_x0 = 58;
constexpr int table_input_y0 = 32;
constexpr int table_input_x1 = 165;
constexpr int table_input_y1 = 139;
constexpr int table_output_x0 = 238;
constexpr int table_output_y0 = 60;
constexpr int table_output_x1 = 288;
constexpr int table_output_y1 = 110;
#ifdef _TINSPIRE
constexpr int table_panel_offset_y = 6;
#else
constexpr int table_panel_offset_y = 26; // Previously 6; moved down by 20px.
#endif

// Crafting area coordinates in inventory2.png
constexpr int crafting_src_x = 87;
constexpr int crafting_src_y = 25;
constexpr int crafting_output_src_x = 143;
constexpr int crafting_output_src_y = 35;

// Crafting grid is 2x2
constexpr int crafting_cols = 2;
constexpr int crafting_rows = 2;
constexpr int crafting_slot_width = 18;  // (122-87+1) / 2 = ~18
constexpr int crafting_slot_height = 18; // (60-25+1) / 2 = ~18

constexpr int inventory_center_offset_x = 0;
#ifdef _TINSPIRE
constexpr int inventory_center_offset_y = 0;
#else
constexpr int inventory_center_offset_y = 40;
#endif

// The usable slot layout is not centered inside inventory2.png, so center using this region.
constexpr int inventory_layout_left = storage_src_x;
constexpr int inventory_layout_right = hotbar_src_x + (Inventory::hotbar_slot_count - 1) * inv_src_pitch + inv_src_slot_size;
constexpr int inventory_layout_top = storage_src_y;
constexpr int inventory_layout_bottom = hotbar_src_y + inv_src_slot_size;

int inventoryOriginX()
{
    return (SCREEN_WIDTH - (inventory_layout_left + inventory_layout_right) * inv_draw_scale) / 2 + inventory_center_offset_x;
}

int inventoryOriginY()
{
    return (SCREEN_HEIGHT - (inventory_layout_top + inventory_layout_bottom) * inv_draw_scale) / 2 + inventory_center_offset_y;
}

TEXTURE *craftingTableTexture()
{
    return &crafting_table;
}

bool slotOccupied(BLOCK_WDATA block, unsigned int count)
{
    return getBLOCK(block) != BLOCK_AIR && count > 0;
}

void craftingTablePanelRect(const TEXTURE &table_tex, int &panel_x, int &panel_y, int &panel_w, int &panel_h)
{
    const int max_w = std::max(1, SCREEN_WIDTH - 8);
    const int inventory_w = static_cast<int>(inventory2.width) * inv_draw_scale;
    const int preferred_w = std::min(max_w, inventory_w);
    const int tw = static_cast<int>(table_tex.width);
    const int th = static_cast<int>(table_tex.height);

    panel_w = std::min(tw, preferred_w);
    if(panel_w < 1)
        panel_w = 1;

    panel_h = (panel_w * th) / tw;
#ifdef _TINSPIRE
    // 320x240: uncapped panel is ~126px tall and crowds the inventory below.
    constexpr int nspire_table_max_h = 74;
    if(panel_h > nspire_table_max_h)
    {
        panel_h = nspire_table_max_h;
        panel_w = (panel_h * tw) / th;
        if(panel_w < 1)
            panel_w = 1;
    }
#endif
    if(panel_h < 1)
        panel_h = 1;

    panel_x = (SCREEN_WIDTH - panel_w) / 2;
    panel_y = table_panel_offset_y;
}

// Vanilla GuiContainer uses 176×166; GuiFurnace draws only UV (0,0)–(176,166). Columns x≥176 in furnace.png are
// flame / smelting progress sprites (drawTexturedModalRect at 176,12 and 176,14), not part of the static background.
constexpr int furnace_gui_src_w = 176;
constexpr int furnace_gui_src_h = 166;

/** Fit the 176×166 furnace panel to the screen (correct aspect; matches ContainerFurnace slot coordinates). */
void furnaceGuiPanelRect(int &panel_x, int &panel_y, int &panel_w, int &panel_h)
{
    const int margin = 8;
    const int max_w = std::max(1, SCREEN_WIDTH - margin);
    const int max_h = std::max(1, SCREEN_HEIGHT - margin);

    int w = max_w;
    int h = (w * furnace_gui_src_h) / furnace_gui_src_w;
    if(h > max_h)
    {
        h = max_h;
        w = (h * furnace_gui_src_w) / furnace_gui_src_h;
        if(w < 1)
            w = 1;
    }
    // ~20% smaller than max-fit (less dominant on screen)
    constexpr int furnace_scale_pct = 80;
    w = (w * furnace_scale_pct) / 100;
    h = (h * furnace_scale_pct) / 100;
    if(w < 1)
        w = 1;
    if(h < 1)
        h = 1;
    panel_w = w;
    panel_h = h;
    panel_x = (SCREEN_WIDTH - panel_w) / 2;
    panel_y = (SCREEN_HEIGHT - panel_h) / 2;
}

enum class RecipeMat : uint8_t {
    Empty,
    Planks,
    WoodLog,
    Stone,
    Cobble,
    StickItem,
    CoalItem,
    RedstoneDustItem,
    RedstoneTorchBlock,
    WheatCrop,
};

enum class RecipeOutputKind : uint8_t {
    Block,
    Item,
};

struct RecipeDef {
    int cols;
    int rows;
    RecipeMat cells[9];
    RecipeOutputKind out_kind;
    uint16_t out_id;
    unsigned int out_count;
};

struct RecipeMatch {
    bool matched = false;
    BLOCK_WDATA output = BLOCK_AIR;
    unsigned int output_count = 0;
    int slots[9] = {};
    int slot_count = 0;
};

bool isPlanks(BLOCK b)
{
    return b == BLOCK_PLANKS_NORMAL || b == BLOCK_PLANKS_DARK || b == BLOCK_PLANKS_BRIGHT;
}

bool blockMatchesRecipeMat(BLOCK_WDATA block, unsigned int count, RecipeMat mat)
{
    if(mat == RecipeMat::Empty)
        return !slotOccupied(block, count);

    if(!slotOccupied(block, count))
        return false;

    const BLOCK b = getBLOCK(block);
    switch(mat)
    {
    case RecipeMat::Planks:
        return isPlanks(b);
    case RecipeMat::WoodLog:
        return b == BLOCK_WOOD;
    case RecipeMat::Stone:
        return b == BLOCK_STONE;
    case RecipeMat::Cobble:
        return b == BLOCK_COBBLESTONE;
    case RecipeMat::StickItem:
        return b == BLOCK_ITEM && getBLOCKDATA(block) == static_cast<uint8_t>(ItemTexture::STICK);
    case RecipeMat::CoalItem:
        if(b != BLOCK_ITEM)
            return false;
        return getBLOCKDATA(block) == static_cast<uint8_t>(ItemTexture::COAL) ||
               getBLOCKDATA(block) == static_cast<uint8_t>(ItemTexture::CHARCOAL);
    case RecipeMat::RedstoneDustItem:
        return b == BLOCK_ITEM && getBLOCKDATA(block) == static_cast<uint8_t>(ItemTexture::REDSTONE_DUST);
    case RecipeMat::RedstoneTorchBlock:
        return b == BLOCK_REDSTONE_TORCH;
    case RecipeMat::WheatCrop:
        return b == BLOCK_WHEAT;
    case RecipeMat::Empty:
    default:
        return false;
    }
}

BLOCK_WDATA recipeOutput(const RecipeDef &recipe)
{
    if(recipe.out_kind == RecipeOutputKind::Block)
        return getBLOCKWDATA(static_cast<BLOCK>(recipe.out_id), 0);

    return getBLOCKWDATA(BLOCK_ITEM, static_cast<uint8_t>(recipe.out_id));
}

static const RecipeDef recipes[] = {
    // 1 log -> 4 planks
    {1, 1,
     {RecipeMat::WoodLog, RecipeMat::Empty, RecipeMat::Empty,
      RecipeMat::Empty, RecipeMat::Empty, RecipeMat::Empty,
      RecipeMat::Empty, RecipeMat::Empty, RecipeMat::Empty},
     RecipeOutputKind::Block, BLOCK_PLANKS_NORMAL, 4},

    // 2 planks vertical -> 4 sticks
    {1, 2,
     {RecipeMat::Planks, RecipeMat::Empty, RecipeMat::Empty,
      RecipeMat::Planks, RecipeMat::Empty, RecipeMat::Empty,
      RecipeMat::Empty, RecipeMat::Empty, RecipeMat::Empty},
     RecipeOutputKind::Item, static_cast<uint16_t>(ItemTexture::STICK), 4},

    // 2x2 planks -> crafting table
    {2, 2,
     {RecipeMat::Planks, RecipeMat::Planks, RecipeMat::Empty,
      RecipeMat::Planks, RecipeMat::Planks, RecipeMat::Empty,
      RecipeMat::Empty, RecipeMat::Empty, RecipeMat::Empty},
     RecipeOutputKind::Block, BLOCK_CRAFTING_TABLE, 1},

    // 3x3 cobble ring -> furnace
    {3, 3,
     {RecipeMat::Cobble, RecipeMat::Cobble, RecipeMat::Cobble,
      RecipeMat::Cobble, RecipeMat::Empty, RecipeMat::Cobble,
      RecipeMat::Cobble, RecipeMat::Cobble, RecipeMat::Cobble},
     RecipeOutputKind::Block, BLOCK_FURNACE, 1},

    // coal over stick -> 4 torches
    {1, 2,
     {RecipeMat::CoalItem, RecipeMat::Empty, RecipeMat::Empty,
      RecipeMat::StickItem, RecipeMat::Empty, RecipeMat::Empty,
      RecipeMat::Empty, RecipeMat::Empty, RecipeMat::Empty},
     RecipeOutputKind::Block, BLOCK_TORCH, 4},

    // redstone dust over stick -> redstone torch
    {1, 2,
     {RecipeMat::RedstoneDustItem, RecipeMat::Empty, RecipeMat::Empty,
      RecipeMat::StickItem, RecipeMat::Empty, RecipeMat::Empty,
      RecipeMat::Empty, RecipeMat::Empty, RecipeMat::Empty},
     RecipeOutputKind::Block, BLOCK_REDSTONE_TORCH, 1},

    // 2 planks horizontal -> pressure plate
    {2, 1,
     {RecipeMat::Planks, RecipeMat::Planks, RecipeMat::Empty,
      RecipeMat::Empty, RecipeMat::Empty, RecipeMat::Empty,
      RecipeMat::Empty, RecipeMat::Empty, RecipeMat::Empty},
     RecipeOutputKind::Block, BLOCK_PRESSURE_PLATE, 1},

    // 3 wheat horizontal -> bread
    {3, 1,
     {RecipeMat::WheatCrop, RecipeMat::WheatCrop, RecipeMat::WheatCrop,
      RecipeMat::Empty, RecipeMat::Empty, RecipeMat::Empty,
      RecipeMat::Empty, RecipeMat::Empty, RecipeMat::Empty},
     RecipeOutputKind::Item, static_cast<uint16_t>(ItemTexture::BREAD), 1},

    // 2x3 planks -> 3 doors
    {2, 3,
     {RecipeMat::Planks, RecipeMat::Planks, RecipeMat::Empty,
      RecipeMat::Planks, RecipeMat::Planks, RecipeMat::Empty,
      RecipeMat::Planks, RecipeMat::Planks, RecipeMat::Empty},
     RecipeOutputKind::Block, BLOCK_DOOR, 3},

    // bowl
    {3, 2,
     {RecipeMat::Planks, RecipeMat::Empty, RecipeMat::Planks,
      RecipeMat::Empty, RecipeMat::Planks, RecipeMat::Empty,
      RecipeMat::Empty, RecipeMat::Empty, RecipeMat::Empty},
     RecipeOutputKind::Item, static_cast<uint16_t>(ItemTexture::BOWL), 1},

    // Wooden tools
    {3, 3,
     {RecipeMat::Planks, RecipeMat::Planks, RecipeMat::Planks,
      RecipeMat::Empty, RecipeMat::StickItem, RecipeMat::Empty,
      RecipeMat::Empty, RecipeMat::StickItem, RecipeMat::Empty},
     RecipeOutputKind::Item, static_cast<uint16_t>(ItemTexture::WOODEN_PICKAXE), 1},
    {3, 3,
     {RecipeMat::Planks, RecipeMat::Planks, RecipeMat::Empty,
      RecipeMat::Planks, RecipeMat::StickItem, RecipeMat::Empty,
      RecipeMat::Empty, RecipeMat::StickItem, RecipeMat::Empty},
     RecipeOutputKind::Item, static_cast<uint16_t>(ItemTexture::WOODEN_AXE), 1},
    {3, 3,
     {RecipeMat::Empty, RecipeMat::Planks, RecipeMat::Planks,
      RecipeMat::Empty, RecipeMat::StickItem, RecipeMat::Planks,
      RecipeMat::Empty, RecipeMat::StickItem, RecipeMat::Empty},
     RecipeOutputKind::Item, static_cast<uint16_t>(ItemTexture::WOODEN_AXE), 1},
    {3, 3,
     {RecipeMat::Planks, RecipeMat::Planks, RecipeMat::Empty,
      RecipeMat::Empty, RecipeMat::StickItem, RecipeMat::Empty,
      RecipeMat::Empty, RecipeMat::StickItem, RecipeMat::Empty},
     RecipeOutputKind::Item, static_cast<uint16_t>(ItemTexture::WOODEN_HOE), 1},
    {3, 3,
     {RecipeMat::Empty, RecipeMat::Planks, RecipeMat::Planks,
      RecipeMat::Empty, RecipeMat::StickItem, RecipeMat::Empty,
      RecipeMat::Empty, RecipeMat::StickItem, RecipeMat::Empty},
     RecipeOutputKind::Item, static_cast<uint16_t>(ItemTexture::WOODEN_HOE), 1},
    {3, 3,
     {RecipeMat::Empty, RecipeMat::Planks, RecipeMat::Empty,
      RecipeMat::Empty, RecipeMat::StickItem, RecipeMat::Empty,
      RecipeMat::Empty, RecipeMat::StickItem, RecipeMat::Empty},
     RecipeOutputKind::Item, static_cast<uint16_t>(ItemTexture::WOODEN_SHOVEL), 1},
    {3, 3,
     {RecipeMat::Empty, RecipeMat::Planks, RecipeMat::Empty,
      RecipeMat::Empty, RecipeMat::Planks, RecipeMat::Empty,
      RecipeMat::Empty, RecipeMat::StickItem, RecipeMat::Empty},
     RecipeOutputKind::Item, static_cast<uint16_t>(ItemTexture::WOODEN_SWORD), 1},

    // Stone tools
    {3, 3,
     {RecipeMat::Cobble, RecipeMat::Cobble, RecipeMat::Cobble,
      RecipeMat::Empty, RecipeMat::StickItem, RecipeMat::Empty,
      RecipeMat::Empty, RecipeMat::StickItem, RecipeMat::Empty},
     RecipeOutputKind::Item, static_cast<uint16_t>(ItemTexture::STONE_PICKAXE), 1},
    {3, 3,
     {RecipeMat::Cobble, RecipeMat::Cobble, RecipeMat::Empty,
      RecipeMat::Cobble, RecipeMat::StickItem, RecipeMat::Empty,
      RecipeMat::Empty, RecipeMat::StickItem, RecipeMat::Empty},
     RecipeOutputKind::Item, static_cast<uint16_t>(ItemTexture::STONE_AXE), 1},
    {3, 3,
     {RecipeMat::Empty, RecipeMat::Cobble, RecipeMat::Cobble,
      RecipeMat::Empty, RecipeMat::StickItem, RecipeMat::Cobble,
      RecipeMat::Empty, RecipeMat::StickItem, RecipeMat::Empty},
     RecipeOutputKind::Item, static_cast<uint16_t>(ItemTexture::STONE_AXE), 1},
    {3, 3,
     {RecipeMat::Cobble, RecipeMat::Cobble, RecipeMat::Empty,
      RecipeMat::Empty, RecipeMat::StickItem, RecipeMat::Empty,
      RecipeMat::Empty, RecipeMat::StickItem, RecipeMat::Empty},
     RecipeOutputKind::Item, static_cast<uint16_t>(ItemTexture::STONE_HOE), 1},
    {3, 3,
     {RecipeMat::Empty, RecipeMat::Cobble, RecipeMat::Cobble,
      RecipeMat::Empty, RecipeMat::StickItem, RecipeMat::Empty,
      RecipeMat::Empty, RecipeMat::StickItem, RecipeMat::Empty},
     RecipeOutputKind::Item, static_cast<uint16_t>(ItemTexture::STONE_HOE), 1},
    {3, 3,
     {RecipeMat::Empty, RecipeMat::Cobble, RecipeMat::Empty,
      RecipeMat::Empty, RecipeMat::StickItem, RecipeMat::Empty,
      RecipeMat::Empty, RecipeMat::StickItem, RecipeMat::Empty},
     RecipeOutputKind::Item, static_cast<uint16_t>(ItemTexture::STONE_SHOVEL), 1},
    {3, 3,
     {RecipeMat::Empty, RecipeMat::Cobble, RecipeMat::Empty,
      RecipeMat::Empty, RecipeMat::Cobble, RecipeMat::Empty,
      RecipeMat::Empty, RecipeMat::StickItem, RecipeMat::Empty},
     RecipeOutputKind::Item, static_cast<uint16_t>(ItemTexture::STONE_SWORD), 1},

    // Repeater: torch, redstone, torch over three stone
    {3, 2,
     {RecipeMat::RedstoneTorchBlock, RecipeMat::RedstoneDustItem, RecipeMat::RedstoneTorchBlock,
      RecipeMat::Stone, RecipeMat::Stone, RecipeMat::Stone,
      RecipeMat::Empty, RecipeMat::Empty, RecipeMat::Empty},
     RecipeOutputKind::Item, static_cast<uint16_t>(ItemTexture::REDSTONE_REPEATER), 1},
};

bool matchRecipeAt(const RecipeDef &recipe,
                   const BLOCK_WDATA input[], const unsigned int counts[],
                   int cols, int rows,
                   int offset_col, int offset_row,
                   RecipeMatch &out_match)
{
    out_match.matched = false;
    out_match.slot_count = 0;

    for(int row = 0; row < rows; ++row)
    {
        for(int col = 0; col < cols; ++col)
        {
            RecipeMat expected = RecipeMat::Empty;
            if(col >= offset_col && col < offset_col + recipe.cols &&
               row >= offset_row && row < offset_row + recipe.rows)
            {
                const int rcol = col - offset_col;
                const int rrow = row - offset_row;
                expected = recipe.cells[rrow * 3 + rcol];
            }

            const int slot = row * cols + col;
            if(!blockMatchesRecipeMat(input[slot], counts[slot], expected))
                return false;

            if(expected != RecipeMat::Empty)
                out_match.slots[out_match.slot_count++] = slot;
        }
    }

    out_match.matched = true;
    out_match.output = recipeOutput(recipe);
    out_match.output_count = recipe.out_count;
    return true;
}

RecipeMatch findMatchingRecipe(const BLOCK_WDATA input[], const unsigned int counts[], int cols, int rows)
{
    RecipeMatch match;

    for(const RecipeDef &recipe : recipes)
    {
        if(recipe.cols > cols || recipe.rows > rows)
            continue;

        for(int row_off = 0; row_off <= rows - recipe.rows; ++row_off)
            for(int col_off = 0; col_off <= cols - recipe.cols; ++col_off)
                if(matchRecipeAt(recipe, input, counts, cols, rows, col_off, row_off, match))
                    return match;
    }

    return match;
}
}

namespace {

bool furnaceStackEmpty(const std::pair<BLOCK_WDATA, unsigned> &s)
{
    return getBLOCK(s.first) == BLOCK_AIR || s.second == 0;
}

BLOCK_WDATA furnaceSmeltingResult(BLOCK_WDATA in)
{
    const BLOCK b = getBLOCK(in);
    if(b == BLOCK_COBBLESTONE)
        return BLOCK_STONE;
    if(b == BLOCK_IRON_ORE)
        return getBLOCKWDATA(BLOCK_ITEM, static_cast<uint8_t>(ItemTexture::IRON_INGOT));
    if(b == BLOCK_GOLD_ORE)
        return getBLOCKWDATA(BLOCK_ITEM, static_cast<uint8_t>(ItemTexture::GOLD_INGOT));
    if(b == BLOCK_DIAMOND_ORE)
        return getBLOCKWDATA(BLOCK_ITEM, static_cast<uint8_t>(ItemTexture::DIAMOND));
    if(b == BLOCK_COAL_ORE)
        return getBLOCKWDATA(BLOCK_ITEM, static_cast<uint8_t>(ItemTexture::COAL));
    if(b == BLOCK_REDSTONE_ORE)
        return getBLOCKWDATA(BLOCK_ITEM, static_cast<uint8_t>(ItemTexture::REDSTONE_DUST));
    if(b == BLOCK_SAND)
        return BLOCK_GLASS;
    if(b == BLOCK_WOOD)
        return getBLOCKWDATA(BLOCK_ITEM, static_cast<uint8_t>(ItemTexture::CHARCOAL));

    if(b != BLOCK_ITEM)
        return BLOCK_AIR;

    switch(static_cast<ItemTexture>(getBLOCKDATA(in)))
    {
    case ItemTexture::RAW_PORKCHOP:
        return getBLOCKWDATA(BLOCK_ITEM, static_cast<uint8_t>(ItemTexture::COOKED_PORKCHOP));
    case ItemTexture::RAW_BEEF:
        return getBLOCKWDATA(BLOCK_ITEM, static_cast<uint8_t>(ItemTexture::COOKED_BEEF));
    case ItemTexture::RAW_CHICKEN:
        return getBLOCKWDATA(BLOCK_ITEM, static_cast<uint8_t>(ItemTexture::COOKED_CHICKEN));
    case ItemTexture::RAW_COD:
        return getBLOCKWDATA(BLOCK_ITEM, static_cast<uint8_t>(ItemTexture::COOKED_COD));
    case ItemTexture::RAW_SALMON:
        return getBLOCKWDATA(BLOCK_ITEM, static_cast<uint8_t>(ItemTexture::COOKED_SALMON));
    case ItemTexture::POTATO:
        return getBLOCKWDATA(BLOCK_ITEM, static_cast<uint8_t>(ItemTexture::BAKED_POTATO));
    default:
        return BLOCK_AIR;
    }
}

constexpr int maxFurnaceStack = 64;

bool furnaceCanSmelt(const FurnaceTileData &d)
{
    const auto &in = d.stacks[0];
    if(furnaceStackEmpty(in))
        return false;

    const BLOCK_WDATA out_w = furnaceSmeltingResult(in.first);
    if(getBLOCK(out_w) == BLOCK_AIR)
        return false;

    const auto &out_slot = d.stacks[2];
    if(furnaceStackEmpty(out_slot))
        return true;
    if(out_slot.first != out_w)
        return false;
    return out_slot.second < maxFurnaceStack;
}

bool isCraftiWoodBlock(BLOCK b)
{
    return b == BLOCK_WOOD || b == BLOCK_PLANKS_NORMAL || b == BLOCK_PLANKS_DARK || b == BLOCK_PLANKS_BRIGHT;
}

/** MCP TileEntityFurnace.getItemBurnTime — subset for Crafti blocks/items. */
int furnaceFuelBurnTicks(BLOCK_WDATA stack)
{
    if(getBLOCK(stack) == BLOCK_AIR)
        return 0;
    const BLOCK b = getBLOCK(stack);
    if(isCraftiWoodBlock(b))
        return 300;
    if(b != BLOCK_ITEM)
        return 0;

    switch(static_cast<ItemTexture>(getBLOCKDATA(stack)))
    {
    case ItemTexture::STICK:
        return 100;
    case ItemTexture::COAL:
    case ItemTexture::CHARCOAL:
        return 1600;
    case ItemTexture::LAVA_BUCKET:
        return 20000;
    case ItemTexture::BLAZE_ROD:
        return 2400;
    default:
        return 0;
    }
}

void furnaceSmeltOne(FurnaceTileData &d)
{
    if(!furnaceCanSmelt(d))
        return;

    std::pair<BLOCK_WDATA, unsigned> &in = d.stacks[0];
    const BLOCK_WDATA result = furnaceSmeltingResult(in.first);
    std::pair<BLOCK_WDATA, unsigned> &out = d.stacks[2];

    if(furnaceStackEmpty(out))
    {
        out.first = result;
        out.second = 1;
    }
    else if(out.first == result)
        out.second += 1;

    if(in.second <= 1)
    {
        in.first = BLOCK_AIR;
        in.second = 0;
    }
    else
        in.second -= 1;
}

void setFurnaceBlockLit(World &world, int x, int y, int z, bool lit)
{
    BLOCK_WDATA cur = world.getBlock(x, y, z);
    if(getBLOCK(cur) != BLOCK_FURNACE)
        return;
    const uint8_t facing = getBLOCKDATA(cur);
    const BLOCK_WDATA nw = getBLOCKWDATAPower(BLOCK_FURNACE, facing, lit);
    if(nw != cur)
        world.setBlock(x, y, z, nw);
}

/** One game tick (MCP TileEntityFurnace.update server branch). */
void tickFurnaceTile(World &world, int x, int y, int z, FurnaceTileData &d)
{
    const bool burning_before = d.burn_time > 0;
    if(d.burn_time > 0)
        --d.burn_time;

    auto &in = d.stacks[0];
    auto &fuel = d.stacks[1];
    const bool has_fuel = !furnaceStackEmpty(fuel);
    const bool has_input = !furnaceStackEmpty(in);

    if(d.burn_time > 0 || (has_fuel && has_input))
    {
        if(d.burn_time <= 0 && furnaceCanSmelt(d))
        {
            const int bt = furnaceFuelBurnTicks(fuel.first);
            d.burn_time = bt;
            d.current_item_burn_time = bt;
            if(d.burn_time > 0 && has_fuel)
            {
                if(fuel.second <= 1)
                {
                    fuel.first = BLOCK_AIR;
                    fuel.second = 0;
                }
                else
                    --fuel.second;
            }
        }

        if(d.burn_time > 0 && furnaceCanSmelt(d))
        {
            ++d.cook_time;
            if(d.cook_time >= d.cook_time_total)
            {
                d.cook_time = 0;
                d.cook_time_total = 200;
                furnaceSmeltOne(d);
            }
        }
        else
            d.cook_time = 0;
    }
    else if(d.burn_time <= 0 && d.cook_time > 0)
        d.cook_time = std::max(0, std::min(d.cook_time_total, d.cook_time - 2));

    if(burning_before != (d.burn_time > 0))
        setFurnaceBlockLit(world, x, y, z, d.burn_time > 0);
}

} // namespace

void InventoryTask::openPlayerInventory()
{
    crafting_table_mode = false;
    furnace_mode = false;
    activate();
}

void InventoryTask::openCraftingTable()
{
    crafting_table_mode = true;
    furnace_mode = false;
    activate();
}

void InventoryTask::openFurnace(int block_x, int block_y, int block_z)
{
    crafting_table_mode = false;
    furnace_mode = true;
    furnace_bx = block_x;
    furnace_by = block_y;
    furnace_bz = block_z;
    const auto key = std::make_tuple(block_x, block_y, block_z);
    FurnaceTileData &fd = furnace_tiles[key];
    for(int i = 0; i < 3; ++i)
    {
        furnace_slots[i] = fd.stacks[static_cast<size_t>(i)].first;
        furnace_counts[i] = fd.stacks[static_cast<size_t>(i)].second;
    }
    activate();
}

void InventoryTask::syncFurnaceStorage(bool reset_cook_for_input_slot)
{
    if(!furnace_mode)
        return;
    const auto key = std::make_tuple(furnace_bx, furnace_by, furnace_bz);
    FurnaceTileData &fd = furnace_tiles[key];
    for(int i = 0; i < 3; ++i)
        fd.stacks[static_cast<size_t>(i)] = {furnace_slots[i], furnace_counts[i]};
    if(reset_cook_for_input_slot)
    {
        fd.cook_time = 0;
        fd.cook_time_total = 200;
    }
}

void InventoryTask::ensureFurnaceTile(int block_x, int block_y, int block_z)
{
    const auto key = std::make_tuple(block_x, block_y, block_z);
    (void)furnace_tiles[key];
}

void InventoryTask::removeFurnaceTile(int block_x, int block_y, int block_z)
{
    furnace_tiles.erase(std::make_tuple(block_x, block_y, block_z));
}

void InventoryTask::tickFurnaces(World &world)
{
    for(auto it = furnace_tiles.begin(); it != furnace_tiles.end();)
    {
        const int x = std::get<0>(it->first);
        const int y = std::get<1>(it->first);
        const int z = std::get<2>(it->first);
        BLOCK_WDATA cur = world.getBlock(x, y, z);
        if(getBLOCK(cur) != BLOCK_FURNACE)
        {
            it = furnace_tiles.erase(it);
            continue;
        }

        tickFurnaceTile(world, x, y, z, it->second);

        if(furnace_mode && x == furnace_bx && y == furnace_by && z == furnace_bz)
        {
            FurnaceTileData &fd = it->second;
            for(int i = 0; i < 3; ++i)
            {
                furnace_slots[i] = fd.stacks[static_cast<size_t>(i)].first;
                furnace_counts[i] = fd.stacks[static_cast<size_t>(i)].second;
            }
        }
        ++it;
    }
}

void InventoryTask::makeCurrent()
{
    openPlayerInventory();
}

void InventoryTask::reset()
{
    held_block = BLOCK_AIR;
    held_count = 0;
    crafting_output = BLOCK_AIR;
    crafting_output_count = 0;
    for(int i = 0; i < CRAFTING_INPUT_COUNT; ++i)
    {
        crafting_input[i] = BLOCK_AIR;
        crafting_counts[i] = 0;
    }
    matched_recipe_slot_count = 0;
}

void InventoryTask::activate()
{
    if(!background_saved)
        saveBackground();

#ifdef _TINSPIRE
    if(furnace_mode)
    {
        int sx, sy, sw, sh;
        furnacePlayerSlotBounds(0, sx, sy, sw, sh);
        cursor_x = sx + sw / 2;
        cursor_y = sy + sh / 2;
    }
    else
    {
        const int inv_x = inventoryOriginX();
        const int inv_y = inventoryOriginY();
        cursor_x = inv_x + hotbar_src_x * inv_draw_scale + inv_draw_slot_size / 2;
        cursor_y = inv_y + hotbar_src_y * inv_draw_scale + inv_draw_slot_size / 2;
    }
    nspire_select_was_down = false;
    nspire_single_was_down = false;
    nspire_half_was_down = false;
    nspire_tp_had_contact = false;
    nspire_tp_last_x = 0;
    nspire_tp_last_y = 0;
#endif

    Task::makeCurrent();
}

int InventoryTask::activeCraftingInputCount() const
{
    return crafting_table_mode ? 9 : 4;
}

int InventoryTask::activeCraftingCols() const
{
    return crafting_table_mode ? 3 : 2;
}

int InventoryTask::activeCraftingRows() const
{
    return crafting_table_mode ? 3 : 2;
}

void InventoryTask::craftingGridBounds(int &x, int &y, int &w, int &h) const
{
    if(!crafting_table_mode)
    {
        const int inv_x = inventoryOriginX();
        const int inv_y = inventoryOriginY();
        x = inv_x + crafting_src_x * inv_draw_scale;
        y = inv_y + crafting_src_y * inv_draw_scale;
        w = crafting_slot_width * activeCraftingCols() * inv_draw_scale;
        h = crafting_slot_height * activeCraftingRows() * inv_draw_scale;
        return;
    }

    TEXTURE *table_tex = craftingTableTexture();
    if(!table_tex)
    {
        const int inv_x = inventoryOriginX();
        const int inv_y = inventoryOriginY();
        x = inv_x + crafting_src_x * inv_draw_scale;
        y = inv_y + crafting_src_y * inv_draw_scale;
        w = crafting_slot_width * activeCraftingCols() * inv_draw_scale;
        h = crafting_slot_height * activeCraftingRows() * inv_draw_scale;
        return;
    }

    int panel_x = 0, panel_y = 0, panel_w = 0, panel_h = 0;
    craftingTablePanelRect(*table_tex, panel_x, panel_y, panel_w, panel_h);

    x = panel_x + (table_input_x0 * panel_w) / static_cast<int>(table_tex->width);
    y = panel_y + (table_input_y0 * panel_h) / static_cast<int>(table_tex->height);
    int x1 = panel_x + ((table_input_x1 + 1) * panel_w) / static_cast<int>(table_tex->width);
    int y1 = panel_y + ((table_input_y1 + 1) * panel_h) / static_cast<int>(table_tex->height);
    w = std::max(1, x1 - x);
    h = std::max(1, y1 - y);
}

void InventoryTask::craftingOutputBounds(int &x, int &y, int &w, int &h) const
{
    if(!crafting_table_mode)
    {
        const int inv_x = inventoryOriginX();
        const int inv_y = inventoryOriginY();
        const int output_src_w = 160 - 143 + 1;
        const int output_src_h = 52 - 35 + 1;
        x = inv_x + crafting_output_src_x * inv_draw_scale;
        y = inv_y + crafting_output_src_y * inv_draw_scale;
        w = output_src_w * inv_draw_scale;
        h = output_src_h * inv_draw_scale;
        return;
    }

    TEXTURE *table_tex = craftingTableTexture();
    if(!table_tex)
    {
        const int inv_x = inventoryOriginX();
        const int inv_y = inventoryOriginY();
        const int output_src_w = 160 - 143 + 1;
        const int output_src_h = 52 - 35 + 1;
        x = inv_x + crafting_output_src_x * inv_draw_scale;
        y = inv_y + crafting_output_src_y * inv_draw_scale;
        w = output_src_w * inv_draw_scale;
        h = output_src_h * inv_draw_scale;
        return;
    }

    int panel_x = 0, panel_y = 0, panel_w = 0, panel_h = 0;
    craftingTablePanelRect(*table_tex, panel_x, panel_y, panel_w, panel_h);

    x = panel_x + (table_output_x0 * panel_w) / static_cast<int>(table_tex->width);
    y = panel_y + (table_output_y0 * panel_h) / static_cast<int>(table_tex->height);
    int x1 = panel_x + ((table_output_x1 + 1) * panel_w) / static_cast<int>(table_tex->width);
    int y1 = panel_y + ((table_output_y1 + 1) * panel_h) / static_cast<int>(table_tex->height);
    w = std::max(1, x1 - x);
    h = std::max(1, y1 - y);
}

void InventoryTask::furnaceSlotBounds(int slot_index, int &x, int &y, int &w, int &h) const
{
    constexpr int slot_px = 18;
    int sx = 56;
    int sy = 17;
    if(slot_index == 1)
        sy = 53;
    else if(slot_index == 2)
    {
        sx = 116;
        sy = 35;
    }

    int panel_x = 0, panel_y = 0, panel_w = 0, panel_h = 0;
    furnaceGuiPanelRect(panel_x, panel_y, panel_w, panel_h);
    x = panel_x + (sx * panel_w) / furnace_gui_src_w;
    y = panel_y + (sy * panel_h) / furnace_gui_src_h;
    w = std::max(1, (slot_px * panel_w) / furnace_gui_src_w);
    h = std::max(1, (slot_px * panel_h) / furnace_gui_src_h);
}

int InventoryTask::furnaceSlotFromMouse(int mouse_x, int mouse_y) const
{
    for(int i = 0; i < 3; ++i)
    {
        int sx, sy, sw, sh;
        furnaceSlotBounds(i, sx, sy, sw, sh);
        if(mouse_x >= sx && mouse_x < sx + sw && mouse_y >= sy && mouse_y < sy + sh)
            return FURNACE_INPUT_SLOT + i;
    }
    return INVALID_SLOT;
}

void InventoryTask::furnacePlayerSlotBounds(int player_slot, int &x, int &y, int &w, int &h) const
{
    constexpr int slot_px = 18;
    int sx = 0;
    int sy = 0;
    if(player_slot < Inventory::hotbar_slot_count)
    {
        sx = 8 + player_slot * 18;
        sy = 142;
    }
    else
    {
        const int idx = player_slot - Inventory::hotbar_slot_count;
        const int col = idx % 9;
        const int row = idx / 9;
        sx = 8 + col * 18;
        sy = 84 + row * 18;
    }

    int panel_x = 0, panel_y = 0, panel_w = 0, panel_h = 0;
    furnaceGuiPanelRect(panel_x, panel_y, panel_w, panel_h);
    x = panel_x + (sx * panel_w) / furnace_gui_src_w;
    y = panel_y + (sy * panel_h) / furnace_gui_src_h;
    w = std::max(1, (slot_px * panel_w) / furnace_gui_src_w);
    h = std::max(1, (slot_px * panel_h) / furnace_gui_src_h);
}

int InventoryTask::slotFromMouse(int mouse_x, int mouse_y) const
{
    const int inv_x = inventoryOriginX();
    const int inv_y = inventoryOriginY();

    const int local_x = mouse_x - inv_x;
    const int local_y = mouse_y - inv_y;
    const int slot_size = inv_draw_slot_size;
    const int pitch = inv_draw_pitch;

    if(furnace_mode)
    {
        const int furnace_slot = furnaceSlotFromMouse(mouse_x, mouse_y);
        if(furnace_slot != INVALID_SLOT)
            return furnace_slot;

        for(int s = 0; s < Inventory::slot_count; ++s)
        {
            int sx, sy, sw, sh;
            furnacePlayerSlotBounds(s, sx, sy, sw, sh);
            if(mouse_x >= sx && mouse_x < sx + sw && mouse_y >= sy && mouse_y < sy + sh)
                return s;
        }
        return INVALID_SLOT;
    }

    int crafting_slot = craftingSlotFromMouse(mouse_x, mouse_y);
    if(crafting_slot != INVALID_SLOT)
        return CRAFTING_SLOT_OFFSET + crafting_slot;

    int output_draw_x, output_draw_y, output_draw_w, output_draw_h;
    craftingOutputBounds(output_draw_x, output_draw_y, output_draw_w, output_draw_h);
    if(mouse_x >= output_draw_x && mouse_x < output_draw_x + output_draw_w &&
       mouse_y >= output_draw_y && mouse_y < output_draw_y + output_draw_h)
        return CRAFTING_OUTPUT_SLOT;

    for(int i = 0; i < Inventory::hotbar_slot_count; ++i)
    {
        const int sx = hotbar_src_x * inv_draw_scale + i * pitch;
        const int sy = hotbar_src_y * inv_draw_scale;
        if(local_x >= sx && local_x < sx + slot_size && local_y >= sy && local_y < sy + slot_size)
            return i;
    }

    for(int row = 0; row < storage_rows; ++row)
        for(int col = 0; col < storage_cols; ++col)
        {
            const int sx = storage_src_x * inv_draw_scale + col * pitch;
            const int sy = storage_src_y * inv_draw_scale + row * pitch;
            if(local_x >= sx && local_x < sx + slot_size && local_y >= sy && local_y < sy + slot_size)
                return Inventory::hotbar_slot_count + row * storage_cols + col;
        }

    return INVALID_SLOT;
}

int InventoryTask::craftingSlotFromMouse(int mouse_x, int mouse_y) const
{
    if(furnace_mode)
        return INVALID_SLOT;

    int craft_draw_x, craft_draw_y, craft_width, craft_height;
    craftingGridBounds(craft_draw_x, craft_draw_y, craft_width, craft_height);

    const int local_x = mouse_x - craft_draw_x;
    const int local_y = mouse_y - craft_draw_y;

    // Check if click is within crafting grid bounds
    if(local_x < 0 || local_x >= craft_width || local_y < 0 || local_y >= craft_height)
        return INVALID_SLOT;

    const int cols = activeCraftingCols();
    const int rows = activeCraftingRows();
    const int col = (local_x * cols) / craft_width;
    const int row = (local_y * rows) / craft_height;

    if(col >= cols || row >= rows)
        return INVALID_SLOT;

    return row * cols + col;
}

bool InventoryTask::isCraftingSlot(int slot) const
{
    return slot >= CRAFTING_SLOT_OFFSET && slot <= CRAFTING_OUTPUT_SLOT;
}

void InventoryTask::drawSlotItem(TEXTURE &tex, int slot, int x, int y)
{
    const BLOCK_WDATA block = current_inventory.slotBlock(slot);
    const unsigned int count = current_inventory.slotCount(slot);
    if(getBLOCK(block) == BLOCK_AIR || count == 0)
        return;

    // Handle item rendering (BLOCK_ITEM stores ItemTexture in metadata)
    if(getBLOCK(block) == BLOCK_ITEM)
    {
        drawItemIcon(block, tex, x, y, inv_draw_slot_size);
    }
    else
    {
#ifdef _TINSPIRE
        const TextureAtlasEntry &icon_tex = global_block_renderer.materialTexture(block).resized;
        drawTexture(*terrain_resized, tex,
                    icon_tex.left, icon_tex.top,
                    icon_tex.right - icon_tex.left, icon_tex.bottom - icon_tex.top,
                    x, y,
                    inv_draw_slot_size, inv_draw_slot_size);
#else
        if(getBLOCK(block) == BLOCK_DOOR)
        {
            const int door_w = 16;
            const int door_h = 32;
            // DoorRenderer applies an internal +4 x-offset.
            const int preview_x = x + (inv_draw_slot_size - door_w) / 2 - 4;
            const int preview_y = y + (inv_draw_slot_size - door_h) / 2;
            global_block_renderer.drawPreview(block, tex, preview_x, preview_y);
        }
        else
        {
            const int icon_w = 24;
            const int icon_h = 24;
            const int preview_x = x + (inv_draw_slot_size - icon_w) / 2;
            const int preview_y = y + (inv_draw_slot_size - icon_h) / 2;
            global_block_renderer.drawPreview(block, tex, preview_x, preview_y);
        }
#endif
    }

    char count_text[12];
    snprintf(count_text, sizeof(count_text), "%u", count);
    drawString(count_text, 0xFFFF, tex, x + inv_draw_slot_size - 10, y + 2);
}

bool InventoryTask::isHoldingItem() const
{
    return getBLOCK(held_block) != BLOCK_AIR && held_count > 0;
}

void InventoryTask::tryCraft()
{
    matched_recipe_slot_count = 0;

    const int cols = activeCraftingCols();
    const int rows = activeCraftingRows();

    RecipeMatch match = findMatchingRecipe(crafting_input, crafting_counts, cols, rows);
    if(match.matched)
    {
        crafting_output = match.output;
        crafting_output_count = match.output_count;
        matched_recipe_slot_count = match.slot_count;
        for(int i = 0; i < matched_recipe_slot_count; ++i)
            matched_recipe_slots[i] = match.slots[i];
        return;
    }

    crafting_output = BLOCK_AIR;
    crafting_output_count = 0;
}

void InventoryTask::consumeCraftingIngredients()
{
    if(getBLOCK(crafting_output) == BLOCK_AIR || crafting_output_count == 0 || matched_recipe_slot_count <= 0)
        return;

    const int input_count = activeCraftingInputCount();
    for(int idx = 0; idx < matched_recipe_slot_count; ++idx)
    {
        const int slot = matched_recipe_slots[idx];
        if(slot < 0 || slot >= input_count || crafting_counts[slot] == 0)
            continue;

        crafting_counts[slot]--;
        if(crafting_counts[slot] == 0)
            crafting_input[slot] = BLOCK_AIR;
    }
}

void InventoryTask::handleLeftClick(int slot)
{
    if(furnace_mode && slot >= FURNACE_INPUT_SLOT && slot <= FURNACE_OUTPUT_SLOT)
    {
        const int fi = slot - FURNACE_INPUT_SLOT;
        if(slot == FURNACE_OUTPUT_SLOT)
        {
            if(furnace_counts[2] > 0 && getBLOCK(furnace_slots[2]) != BLOCK_AIR)
            {
                if(!isHoldingItem())
                {
                    held_block = furnace_slots[2];
                    held_count = furnace_counts[2];
                    furnace_slots[2] = BLOCK_AIR;
                    furnace_counts[2] = 0;
                    syncFurnaceStorage(false);
                    return;
                }
                else if(held_block == furnace_slots[2])
                {
                    held_count += furnace_counts[2];
                    furnace_slots[2] = BLOCK_AIR;
                    furnace_counts[2] = 0;
                    syncFurnaceStorage(false);
                    return;
                }
            }
            return;
        }

        const BLOCK_WDATA slot_block = furnace_slots[fi];
        const unsigned int slot_count = furnace_counts[fi];

        if(!isHoldingItem())
        {
            if(getBLOCK(slot_block) != BLOCK_AIR && slot_count > 0)
            {
                held_block = slot_block;
                held_count = slot_count;
                furnace_slots[fi] = BLOCK_AIR;
                furnace_counts[fi] = 0;
                syncFurnaceStorage(fi == 0);
            }
            return;
        }

        if(getBLOCK(slot_block) == BLOCK_AIR || slot_count == 0)
        {
            furnace_slots[fi] = held_block;
            furnace_counts[fi] = held_count;
            held_block = BLOCK_AIR;
            held_count = 0;
            syncFurnaceStorage(fi == 0);
        }
        else if(slot_block == held_block)
        {
            furnace_slots[fi] = slot_block;
            furnace_counts[fi] = slot_count + held_count;
            held_block = BLOCK_AIR;
            held_count = 0;
            syncFurnaceStorage(fi == 0);
        }
        else
        {
            furnace_slots[fi] = held_block;
            furnace_counts[fi] = held_count;
            held_block = slot_block;
            held_count = slot_count;
            syncFurnaceStorage(fi == 0);
        }
        return;
    }

    // Handle crafting output slot (read-only, can only pick up)
    if(slot == CRAFTING_OUTPUT_SLOT)
    {
        if(crafting_output_count > 0 && getBLOCK(crafting_output) != BLOCK_AIR)
        {
            if(!isHoldingItem())
            {
                held_block = crafting_output;
                held_count = crafting_output_count;
                consumeCraftingIngredients();
                
                crafting_output = BLOCK_AIR;
                crafting_output_count = 0;
                tryCraft();
                return;
            }
            else if(held_block == crafting_output)
            {
                // Add output to held items
                held_count += crafting_output_count;
                consumeCraftingIngredients();
                
                crafting_output = BLOCK_AIR;
                crafting_output_count = 0;
                tryCraft();
                return;
            }
        }
        return;
    }

    // Handle crafting input slots
    if(slot >= CRAFTING_SLOT_OFFSET && slot < CRAFTING_OUTPUT_SLOT)
    {
        int craft_slot = slot - CRAFTING_SLOT_OFFSET;
        const BLOCK_WDATA slot_block = crafting_input[craft_slot];
        const unsigned int slot_count = crafting_counts[craft_slot];

        if(!isHoldingItem())
        {
            if(getBLOCK(slot_block) != BLOCK_AIR && slot_count > 0)
            {
                held_block = slot_block;
                held_count = slot_count;
                crafting_input[craft_slot] = BLOCK_AIR;
                crafting_counts[craft_slot] = 0;
                tryCraft();
            }
            return;
        }

        if(getBLOCK(slot_block) == BLOCK_AIR || slot_count == 0)
        {
            crafting_input[craft_slot] = held_block;
            crafting_counts[craft_slot] = held_count;
            held_block = BLOCK_AIR;
            held_count = 0;
            tryCraft();
        }
        else if(slot_block == held_block)
        {
            crafting_input[craft_slot] = slot_block;
            crafting_counts[craft_slot] = slot_count + held_count;
            held_block = BLOCK_AIR;
            held_count = 0;
            tryCraft();
        }
        else
        {
            crafting_input[craft_slot] = held_block;
            crafting_counts[craft_slot] = held_count;
            held_block = slot_block;
            held_count = slot_count;
            tryCraft();
        }
        return;
    }

    // Handle regular inventory slots
    const BLOCK_WDATA slot_block = current_inventory.slotBlock(slot);
    const unsigned int slot_count = current_inventory.slotCount(slot);

    if(!isHoldingItem())
    {
        if(getBLOCK(slot_block) != BLOCK_AIR && slot_count > 0)
        {
            held_block = slot_block;
            held_count = slot_count;
            current_inventory.setSlot(slot, BLOCK_AIR, 0);
        }
        return;
    }

    if(getBLOCK(slot_block) == BLOCK_AIR || slot_count == 0)
    {
        current_inventory.setSlot(slot, held_block, held_count);
        held_block = BLOCK_AIR;
        held_count = 0;
    }
    else if(slot_block == held_block)
    {
        current_inventory.setSlot(slot, slot_block, slot_count + held_count);
        held_block = BLOCK_AIR;
        held_count = 0;
    }
    else
    {
        current_inventory.setSlot(slot, held_block, held_count);
        held_block = slot_block;
        held_count = slot_count;
    }
}

void InventoryTask::handleRightClick(int slot)
{
    if(furnace_mode && slot >= FURNACE_INPUT_SLOT && slot <= FURNACE_OUTPUT_SLOT)
    {
        if(slot == FURNACE_OUTPUT_SLOT)
        {
            if(furnace_counts[2] > 0 && getBLOCK(furnace_slots[2]) != BLOCK_AIR)
            {
                if(!isHoldingItem())
                {
                    held_block = furnace_slots[2];
                    held_count = furnace_counts[2];
                    furnace_slots[2] = BLOCK_AIR;
                    furnace_counts[2] = 0;
                    syncFurnaceStorage(false);
                    return;
                }
                else if(held_block == furnace_slots[2])
                {
                    held_count += furnace_counts[2];
                    furnace_slots[2] = BLOCK_AIR;
                    furnace_counts[2] = 0;
                    syncFurnaceStorage(false);
                    return;
                }
            }
            return;
        }

        const int fi = slot - FURNACE_INPUT_SLOT;
        const BLOCK_WDATA slot_block = furnace_slots[fi];
        const unsigned int slot_count = furnace_counts[fi];

        if(!isHoldingItem())
        {
            if(getBLOCK(slot_block) == BLOCK_AIR || slot_count == 0)
                return;

            const unsigned int picked = (slot_count + 1) / 2;
            const unsigned int remaining = slot_count - picked;
            held_block = slot_block;
            held_count = picked;
            furnace_slots[fi] = remaining == 0 ? BLOCK_AIR : slot_block;
            furnace_counts[fi] = remaining;
            syncFurnaceStorage(fi == 0);
            return;
        }

        if(getBLOCK(slot_block) == BLOCK_AIR || slot_count == 0)
        {
            furnace_slots[fi] = held_block;
            furnace_counts[fi] = 1;
            --held_count;
        }
        else if(slot_block == held_block)
        {
            furnace_slots[fi] = slot_block;
            furnace_counts[fi] = slot_count + 1;
            --held_count;
        }

        if(held_count == 0)
            held_block = BLOCK_AIR;

        syncFurnaceStorage(fi == 0);
        return;
    }

    // Handle crafting output slot (read-only, can only pick up one)
    if(slot == CRAFTING_OUTPUT_SLOT)
    {
        if(crafting_output_count > 0 && getBLOCK(crafting_output) != BLOCK_AIR)
        {
            if(!isHoldingItem())
            {
                held_block = crafting_output;
                held_count = crafting_output_count;
                consumeCraftingIngredients();
                
                crafting_output = BLOCK_AIR;
                crafting_output_count = 0;
                tryCraft();
                return;
            }
            else if(held_block == crafting_output)
            {
                held_count += crafting_output_count;
                consumeCraftingIngredients();
                
                crafting_output = BLOCK_AIR;
                crafting_output_count = 0;
                tryCraft();
                return;
            }
        }
        return;
    }

    // Handle crafting input slots
    if(slot >= CRAFTING_SLOT_OFFSET && slot < CRAFTING_OUTPUT_SLOT)
    {
        int craft_slot = slot - CRAFTING_SLOT_OFFSET;
        const BLOCK_WDATA slot_block = crafting_input[craft_slot];
        const unsigned int slot_count = crafting_counts[craft_slot];

        if(!isHoldingItem())
        {
            if(getBLOCK(slot_block) == BLOCK_AIR || slot_count == 0)
                return;

            const unsigned int picked = (slot_count + 1) / 2;
            const unsigned int remaining = slot_count - picked;
            held_block = slot_block;
            held_count = picked;
            crafting_input[craft_slot] = remaining == 0 ? BLOCK_AIR : slot_block;
            crafting_counts[craft_slot] = remaining;
            tryCraft();
            return;
        }

        if(getBLOCK(slot_block) == BLOCK_AIR || slot_count == 0)
        {
            crafting_input[craft_slot] = held_block;
            crafting_counts[craft_slot] = 1;
            --held_count;
        }
        else if(slot_block == held_block)
        {
            crafting_input[craft_slot] = slot_block;
            crafting_counts[craft_slot] = slot_count + 1;
            --held_count;
        }

        if(held_count == 0)
            held_block = BLOCK_AIR;
        
        tryCraft();
        return;
    }

    // Handle regular inventory slots
    const BLOCK_WDATA slot_block = current_inventory.slotBlock(slot);
    const unsigned int slot_count = current_inventory.slotCount(slot);

    if(!isHoldingItem())
    {
        if(getBLOCK(slot_block) == BLOCK_AIR || slot_count == 0)
            return;

        const unsigned int picked = (slot_count + 1) / 2;
        const unsigned int remaining = slot_count - picked;
        held_block = slot_block;
        held_count = picked;
        current_inventory.setSlot(slot, remaining == 0 ? BLOCK_AIR : slot_block, remaining);
        return;
    }

    if(getBLOCK(slot_block) == BLOCK_AIR || slot_count == 0)
    {
        current_inventory.setSlot(slot, held_block, 1);
        --held_count;
    }
    else if(slot_block == held_block)
    {
        current_inventory.setSlot(slot, slot_block, slot_count + 1);
        --held_count;
    }

    if(held_count == 0)
        held_block = BLOCK_AIR;
}

void InventoryTask::handleHalfPlace(int slot)
{
    if(!isHoldingItem())
        return;

    if(slot == CRAFTING_OUTPUT_SLOT)
        return;

    unsigned int place_count = held_count / 2;
    if(place_count == 0)
        place_count = 1;

    if(furnace_mode && slot >= FURNACE_INPUT_SLOT && slot <= FURNACE_FUEL_SLOT)
    {
        const int fi = slot - FURNACE_INPUT_SLOT;
        const BLOCK_WDATA slot_block = furnace_slots[fi];
        const unsigned int slot_count = furnace_counts[fi];

        if(getBLOCK(slot_block) == BLOCK_AIR || slot_count == 0)
        {
            furnace_slots[fi] = held_block;
            furnace_counts[fi] = place_count;
        }
        else if(slot_block == held_block)
        {
            furnace_counts[fi] = slot_count + place_count;
        }
        else
            return;

        held_count -= place_count;
        if(held_count == 0)
            held_block = BLOCK_AIR;

        syncFurnaceStorage(fi == 0);
        return;
    }

    if(slot >= CRAFTING_SLOT_OFFSET && slot < CRAFTING_OUTPUT_SLOT)
    {
        const int craft_slot = slot - CRAFTING_SLOT_OFFSET;
        const BLOCK_WDATA slot_block = crafting_input[craft_slot];
        const unsigned int slot_count = crafting_counts[craft_slot];

        if(getBLOCK(slot_block) == BLOCK_AIR || slot_count == 0)
        {
            crafting_input[craft_slot] = held_block;
            crafting_counts[craft_slot] = place_count;
        }
        else if(slot_block == held_block)
        {
            crafting_counts[craft_slot] = slot_count + place_count;
        }
        else
            return;

        held_count -= place_count;
        if(held_count == 0)
            held_block = BLOCK_AIR;

        tryCraft();
        return;
    }

    if(slot < 0 || slot >= Inventory::slot_count)
        return;

    const BLOCK_WDATA slot_block = current_inventory.slotBlock(slot);
    const unsigned int slot_count = current_inventory.slotCount(slot);

    if(getBLOCK(slot_block) == BLOCK_AIR || slot_count == 0)
    {
        current_inventory.setSlot(slot, held_block, place_count);
    }
    else if(slot_block == held_block)
    {
        current_inventory.setSlot(slot, slot_block, slot_count + place_count);
    }
    else
        return;

    held_count -= place_count;
    if(held_count == 0)
        held_block = BLOCK_AIR;
}

void InventoryTask::render()
{
    drawBackground();

    const int inv_x = inventoryOriginX();
    const int inv_y = inventoryOriginY();

    if(!furnace_mode)
    {
        const int inv_w = static_cast<int>(inventory2.width) * inv_draw_scale;
        const int inv_h = static_cast<int>(inventory2.height) * inv_draw_scale;

        const int clip_x0 = std::max(0, inv_x);
        const int clip_y0 = std::max(0, inv_y);
        const int clip_x1 = std::min(SCREEN_WIDTH, inv_x + inv_w);
        const int clip_y1 = std::min(SCREEN_HEIGHT, inv_y + inv_h);

        if(clip_x1 > clip_x0 && clip_y1 > clip_y0)
        {
            const int rel_left = clip_x0 - inv_x;
            const int rel_top = clip_y0 - inv_y;
            const int rel_right = clip_x1 - inv_x;
            const int rel_bottom = clip_y1 - inv_y;

            const int src_x = rel_left / inv_draw_scale;
            const int src_y = rel_top / inv_draw_scale;
            const int src_right = (rel_right + inv_draw_scale - 1) / inv_draw_scale;
            const int src_bottom = (rel_bottom + inv_draw_scale - 1) / inv_draw_scale;
            const int src_w = src_right - src_x;
            const int src_h = src_bottom - src_y;

            if(src_w > 0 && src_h > 0)
                drawTexture(inventory2, *screen,
                            src_x, src_y, src_w, src_h,
                            inv_x + src_x * inv_draw_scale,
                            inv_y + src_y * inv_draw_scale,
                            src_w * inv_draw_scale,
                            src_h * inv_draw_scale);
        }
    }

    if(furnace_mode)
    {
        int panel_x = 0, panel_y = 0, panel_w = 0, panel_h = 0;
        furnaceGuiPanelRect(panel_x, panel_y, panel_w, panel_h);
        const int src_w = std::min(static_cast<int>(furnace_gui.width), furnace_gui_src_w);
        const int src_h = std::min(static_cast<int>(furnace_gui.height), furnace_gui_src_h);
        drawTexture(furnace_gui, *screen,
                    static_cast<uint16_t>(0), static_cast<uint16_t>(0),
                    static_cast<uint16_t>(src_w), static_cast<uint16_t>(src_h),
                    static_cast<uint16_t>(panel_x), static_cast<uint16_t>(panel_y),
                    static_cast<uint16_t>(panel_w), static_cast<uint16_t>(panel_h));

        const auto fkey = std::make_tuple(furnace_bx, furnace_by, furnace_bz);
        FurnaceTileData fd_gui{};
        const auto fit_gui = furnace_tiles.find(fkey);
        if(fit_gui != furnace_tiles.end())
            fd_gui = fit_gui->second;

        auto vx = [&](int x) { return panel_x + (x * panel_w) / furnace_gui_src_w; };
        auto vy = [&](int y) { return panel_y + (y * panel_h) / furnace_gui_src_h; };
        auto vw = [&](int w) { return std::max(1, (w * panel_w) / furnace_gui_src_w); };
        auto vh = [&](int h) { return std::max(1, (h * panel_h) / furnace_gui_src_h); };

        int burn_scale = fd_gui.current_item_burn_time;
        if(burn_scale <= 0)
            burn_scale = 200;
        const int burn_h_pc = std::min(13, fd_gui.burn_time * 13 / burn_scale);
        if(burn_h_pc > 0 && fd_gui.burn_time > 0)
        {
            const int sh = burn_h_pc + 1;
            drawTexture(furnace_gui, *screen,
                        static_cast<uint16_t>(176), static_cast<uint16_t>(12 - burn_h_pc),
                        static_cast<uint16_t>(14), static_cast<uint16_t>(sh),
                        static_cast<uint16_t>(vx(56)), static_cast<uint16_t>(vy(36 + 12 - burn_h_pc)),
                        static_cast<uint16_t>(vw(14)), static_cast<uint16_t>(vh(sh)));
        }

        int cook_px = 0;
        if(fd_gui.cook_time_total > 0 && fd_gui.cook_time > 0)
            cook_px = fd_gui.cook_time * 24 / fd_gui.cook_time_total;
        if(cook_px > 0)
        {
            const int sw = cook_px + 1;
            drawTexture(furnace_gui, *screen,
                        static_cast<uint16_t>(176), static_cast<uint16_t>(14),
                        static_cast<uint16_t>(sw), static_cast<uint16_t>(16),
                        static_cast<uint16_t>(vx(79)), static_cast<uint16_t>(vy(34)),
                        static_cast<uint16_t>(vw(sw)), static_cast<uint16_t>(vh(16)));
        }
    }
    else if(crafting_table_mode)
    {
        if(TEXTURE *table_tex = craftingTableTexture())
        {
            int panel_x = 0, panel_y = 0, panel_w = 0, panel_h = 0;
            craftingTablePanelRect(*table_tex, panel_x, panel_y, panel_w, panel_h);
            drawTexture(*table_tex, *screen,
                        0, 0,
                        table_tex->width, table_tex->height,
                        panel_x, panel_y,
                        panel_w, panel_h);
        }
    }

    const int pitch = inv_draw_pitch;
    if(furnace_mode)
    {
        for(int s = 0; s < Inventory::slot_count; ++s)
        {
            int sx, sy, bw, bh;
            furnacePlayerSlotBounds(s, sx, sy, bw, bh);
            drawSlotItem(*screen, s, sx, sy);
        }
    }
    else
    {
        for(int i = 0; i < Inventory::hotbar_slot_count; ++i)
        {
            const int x = inv_x + hotbar_src_x * inv_draw_scale + i * pitch;
            const int y = inv_y + hotbar_src_y * inv_draw_scale;
            drawSlotItem(*screen, i, x, y);
        }

        for(int row = 0; row < storage_rows; ++row)
            for(int col = 0; col < storage_cols; ++col)
            {
                const int slot = Inventory::hotbar_slot_count + row * storage_cols + col;
                const int x = inv_x + storage_src_x * inv_draw_scale + col * pitch;
                const int y = inv_y + storage_src_y * inv_draw_scale + row * pitch;
                drawSlotItem(*screen, slot, x, y);
            }
    }

    if(furnace_mode)
    {
        for(int fi = 0; fi < 3; ++fi)
        {
            int x0, y0, slot_w, slot_h;
            furnaceSlotBounds(fi, x0, y0, slot_w, slot_h);
            const BLOCK_WDATA block = furnace_slots[fi];
            const unsigned int count = furnace_counts[fi];
            if(getBLOCK(block) == BLOCK_AIR || count == 0)
                continue;
            if(getBLOCK(block) == BLOCK_ITEM)
            {
                drawItemIcon(block, *screen, x0, y0, std::min(slot_w, slot_h));
            }
            else
            {
#ifdef _TINSPIRE
                const TextureAtlasEntry &icon_tex = global_block_renderer.materialTexture(block).resized;
                drawTexture(*terrain_resized, *screen,
                            icon_tex.left, icon_tex.top,
                            icon_tex.right - icon_tex.left, icon_tex.bottom - icon_tex.top,
                            x0, y0,
                            slot_w, slot_h);
#else
                const int icon_w = 24;
                const int icon_h = 24;
                const int preview_x = x0 + (slot_w - icon_w) / 2;
                const int preview_y = y0 + (slot_h - icon_h) / 2;
                global_block_renderer.drawPreview(block, *screen, preview_x, preview_y);
#endif
            }
            char count_text[12];
            snprintf(count_text, sizeof(count_text), "%u", count);
            drawString(count_text, 0xFFFF, *screen, x0 + slot_w - 10, y0 + 2);
        }
    }
    else
    {
        // Draw crafting grid items
        int craft_draw_x, craft_draw_y, craft_width, craft_height;
        craftingGridBounds(craft_draw_x, craft_draw_y, craft_width, craft_height);
        const int craft_cols = activeCraftingCols();
        const int craft_rows = activeCraftingRows();

        for(int row = 0; row < craft_rows; ++row)
            for(int col = 0; col < craft_cols; ++col)
            {
                const int craft_slot = row * craft_cols + col;
                const int x0 = craft_draw_x + (col * craft_width) / craft_cols;
                const int y0 = craft_draw_y + (row * craft_height) / craft_rows;
                const int x1 = craft_draw_x + ((col + 1) * craft_width) / craft_cols;
                const int y1 = craft_draw_y + ((row + 1) * craft_height) / craft_rows;
                const int slot_w = std::max(1, x1 - x0);
                const int slot_h = std::max(1, y1 - y0);

                const BLOCK_WDATA block = crafting_input[craft_slot];
                const unsigned int count = crafting_counts[craft_slot];
                if(getBLOCK(block) != BLOCK_AIR && count > 0)
                {
                    if(getBLOCK(block) == BLOCK_ITEM)
                    {
                        drawItemIcon(block, *screen, x0, y0, std::min(slot_w, slot_h));
                    }
                    else
                    {
#ifdef _TINSPIRE
                        const TextureAtlasEntry &icon_tex = global_block_renderer.materialTexture(block).resized;
                        drawTexture(*terrain_resized, *screen,
                                    icon_tex.left, icon_tex.top,
                                    icon_tex.right - icon_tex.left, icon_tex.bottom - icon_tex.top,
                                    x0, y0,
                                    slot_w, slot_h);
#else
                        const int icon_w = 24;
                        const int icon_h = 24;
                        const int preview_x = x0 + (slot_w - icon_w) / 2;
                        const int preview_y = y0 + (slot_h - icon_h) / 2;
                        global_block_renderer.drawPreview(block, *screen, preview_x, preview_y);
#endif
                    }

                    char count_text[12];
                    snprintf(count_text, sizeof(count_text), "%u", count);
                    drawString(count_text, 0xFFFF, *screen, x0 + slot_w - 10, y0 + 2);
                }
            }

        // Draw crafting output
        int output_draw_x, output_draw_y, output_draw_w, output_draw_h;
        craftingOutputBounds(output_draw_x, output_draw_y, output_draw_w, output_draw_h);

        if(getBLOCK(crafting_output) != BLOCK_AIR && crafting_output_count > 0)
        {
            if(getBLOCK(crafting_output) == BLOCK_ITEM)
            {
                drawItemIcon(crafting_output, *screen, output_draw_x, output_draw_y, std::min(output_draw_w, output_draw_h));
            }
            else
            {
#ifdef _TINSPIRE
                const TextureAtlasEntry &icon_tex = global_block_renderer.materialTexture(crafting_output).resized;
                drawTexture(*terrain_resized, *screen,
                            icon_tex.left, icon_tex.top,
                            icon_tex.right - icon_tex.left, icon_tex.bottom - icon_tex.top,
                            output_draw_x, output_draw_y,
                            output_draw_w, output_draw_h);
#else
                const int icon_w = 24;
                const int icon_h = 24;
                const int preview_x = output_draw_x + (output_draw_w - icon_w) / 2;
                const int preview_y = output_draw_y + (output_draw_h - icon_h) / 2;
                global_block_renderer.drawPreview(crafting_output, *screen, preview_x, preview_y);
#endif
            }

            char count_text[12];
            snprintf(count_text, sizeof(count_text), "%u", crafting_output_count);
            drawString(count_text, 0xFFFF, *screen, output_draw_x + output_draw_w - 10, output_draw_y + 2);
        }
    }

#ifndef _TINSPIRE
    int mx = 0, my = 0;
    SDL_GetMouseState(&mx, &my);
    if(getBLOCK(held_block) != BLOCK_AIR && held_count > 0)
    {
        if(getBLOCK(held_block) == BLOCK_ITEM)
        {
            const int held_size = inv_draw_slot_size;
            drawItemIcon(held_block, *screen, mx - held_size / 2, my - held_size / 2, held_size);
        }
        else
            global_block_renderer.drawPreview(held_block, *screen, mx - 8, my - (getBLOCK(held_block) == BLOCK_DOOR ? 14 : 10));

        char count_text[12];
        snprintf(count_text, sizeof(count_text), "%u", held_count);
        drawString(count_text, 0xFFFF, *screen, mx + 8, my + 8);
    }
#else
    if(getBLOCK(held_block) != BLOCK_AIR && held_count > 0)
    {
        const int held_size = inv_draw_slot_size;
        const int held_x = std::max(0, std::min(cursor_x - held_size / 2, SCREEN_WIDTH - held_size));
        const int held_y = std::max(0, std::min(cursor_y - held_size / 2, SCREEN_HEIGHT - held_size));
        if(getBLOCK(held_block) == BLOCK_ITEM)
            drawItemIcon(held_block, *screen, held_x, held_y, held_size);
        else
        {
            const TextureAtlasEntry &icon_tex = global_block_renderer.materialTexture(held_block).resized;
            drawTexture(*terrain_resized, *screen,
                        icon_tex.left, icon_tex.top,
                        icon_tex.right - icon_tex.left, icon_tex.bottom - icon_tex.top,
                        held_x, held_y,
                        held_size, held_size);
        }

        char count_text[12];
        snprintf(count_text, sizeof(count_text), "%u", held_count);
        drawString(count_text, 0xFFFF, *screen, std::min(held_x + held_size + 2, SCREEN_WIDTH - 20), held_y);
    }

    drawRectangle(*screen, std::max(0, cursor_x - 4), std::max(0, cursor_y - 4), 9, 9, 0xFFFF);
#endif
}

void InventoryTask::logic(GLFix dt)
{
#ifndef _TINSPIRE
    SDL_PumpEvents();
    const Uint8 *keys = SDL_GetKeyState(nullptr);
    int mx = 0, my = 0;
    const Uint8 mouse = SDL_GetMouseState(&mx, &my);
    const bool t_down = keys[SDLK_t] != 0;

    if(key_held_down)
        key_held_down = t_down;

    if(t_down)
    {
        if(!key_held_down)
        {
            world_task.makeCurrent();
            key_held_down = true;
        }
        return;
    }

    const bool left_down = (mouse & SDL_BUTTON(SDL_BUTTON_LEFT)) != 0;
    const bool right_down = (mouse & SDL_BUTTON(SDL_BUTTON_RIGHT)) != 0;
    const int slot = slotFromMouse(mx, my);

    if(!left_down)
        left_mouse_was_down = false;
    if(!right_down)
        right_mouse_was_down = false;

    if(left_down && !left_mouse_was_down)
    {
        left_mouse_was_down = true;
        if(slot >= 0)
            handleLeftClick(slot);
    }

    if(right_down && !right_mouse_was_down)
    {
        right_mouse_was_down = true;
        if(slot >= 0)
            handleRightClick(slot);
    }
#else
    if(key_held_down)
    {
        key_held_down = keyPressed(KEY_NSPIRE_A) || keyPressed(KEY_NSPIRE_PERIOD) || keyPressed(KEY_NSPIRE_ESC);
        return;
    }

    // On calculator builds, close with A, . or ESC.
    if(keyPressed(KEY_NSPIRE_A) || keyPressed(KEY_NSPIRE_PERIOD) || keyPressed(KEY_NSPIRE_ESC))
    {
        world_task.makeCurrent();
        key_held_down = true;
        return;
    }

    const int cursor_step = 1;
    const int touchpad_sensitivity_div = 10;
    if(has_touchpad)
    {
        touchpad_report_t touchpad;
        touchpad_scan(&touchpad);

        if(nspire_tp_had_contact && touchpad.contact)
        {
            const int dx = static_cast<int>(touchpad.x) - static_cast<int>(nspire_tp_last_x);
            const int dy = static_cast<int>(touchpad.y) - static_cast<int>(nspire_tp_last_y);
            cursor_x += dx / touchpad_sensitivity_div;
            // Invert touchpad Y so swiping up moves cursor up.
            cursor_y -= dy / touchpad_sensitivity_div;
        }

        if(touchpad.pressed)
        {
            switch(touchpad.arrow)
            {
            case TPAD_ARROW_LEFT:
            case TPAD_ARROW_LEFTUP:
                cursor_x -= cursor_step;
                break;
            case TPAD_ARROW_RIGHT:
            case TPAD_ARROW_RIGHTDOWN:
                cursor_x += cursor_step;
                break;
            case TPAD_ARROW_UP:
            case TPAD_ARROW_UPRIGHT:
                cursor_y += cursor_step;
                break;
            case TPAD_ARROW_DOWN:
            case TPAD_ARROW_DOWNLEFT:
                cursor_y -= cursor_step;
                break;
            default:
                break;
            }
        }

        nspire_tp_had_contact = touchpad.contact;
        nspire_tp_last_x = touchpad.x;
        nspire_tp_last_y = touchpad.y;
    }

    if(keyPressed(KEY_NSPIRE_LEFT))
        cursor_x -= cursor_step;
    else if(keyPressed(KEY_NSPIRE_RIGHT))
        cursor_x += cursor_step;

    if(keyPressed(KEY_NSPIRE_UP))
        cursor_y -= cursor_step;
    else if(keyPressed(KEY_NSPIRE_DOWN))
        cursor_y += cursor_step;

    cursor_x = std::max(0, std::min(cursor_x, SCREEN_WIDTH - 1));
    cursor_y = std::max(0, std::min(cursor_y, SCREEN_HEIGHT - 1));

    const bool select_down = keyPressed(KEY_NSPIRE_CLICK) || keyPressed(KEY_NSPIRE_5);
    const bool single_down = keyPressed(KEY_NSPIRE_SHIFT);
    const bool half_down = keyPressed(KEY_NSPIRE_VAR);

    const int slot = slotFromMouse(cursor_x, cursor_y);
    if(select_down && !nspire_select_was_down)
    {
        if(slot >= 0)
            handleLeftClick(slot);
    }
    if(single_down && !nspire_single_was_down)
    {
        if(slot >= 0)
            handleRightClick(slot);
    }
    if(half_down && !nspire_half_was_down)
    {
        if(slot >= 0)
            handleHalfPlace(slot);
    }

    nspire_select_was_down = select_down;
    nspire_single_was_down = single_down;
    nspire_half_was_down = half_down;
#endif

    static GLFix furnace_acc = 0;
    furnace_acc += dt;
    unsigned fn = 0;
    while(furnace_acc >= GLFix(1) && fn < 8)
    {
        furnace_acc -= GLFix(1);
        tickFurnaces(world);
        ++fn;
    }
}
