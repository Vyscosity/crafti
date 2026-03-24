#include "inventorytask.h"

#ifndef _TINSPIRE
#include <SDL/SDL.h>
#endif

#include <algorithm>
#include <cstdio>

#include "blockrenderer.h"
#include "font.h"
#include "inventory.h"
#include "worldtask.h"

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
}

void InventoryTask::makeCurrent()
{
    if(!background_saved)
        saveBackground();

    Task::makeCurrent();
}

int InventoryTask::slotFromMouse(int mouse_x, int mouse_y) const
{
    const int inv_x = inventoryOriginX();
    const int inv_y = inventoryOriginY();

    const int local_x = mouse_x - inv_x;
    const int local_y = mouse_y - inv_y;
    const int slot_size = inv_draw_slot_size;
    const int pitch = inv_draw_pitch;

    // Check crafting area
    int crafting_slot = craftingSlotFromMouse(mouse_x, mouse_y);
    if(crafting_slot != INVALID_SLOT)
        return CRAFTING_SLOT_OFFSET + crafting_slot;

    // Check output slot
    const int output_src_w = 160 - 143 + 1;
    const int output_src_h = 52 - 35 + 1;
    const int output_draw_x = inv_x + crafting_output_src_x * inv_draw_scale;
    const int output_draw_y = inv_y + crafting_output_src_y * inv_draw_scale;
    const int output_draw_w = output_src_w * inv_draw_scale;
    const int output_draw_h = output_src_h * inv_draw_scale;
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

    return -1;
}

int InventoryTask::craftingSlotFromMouse(int mouse_x, int mouse_y) const
{
    const int inv_x = inventoryOriginX();
    const int inv_y = inventoryOriginY();

    const int craft_draw_x = inv_x + crafting_src_x * inv_draw_scale;
    const int craft_draw_y = inv_y + crafting_src_y * inv_draw_scale;

    const int local_x = mouse_x - craft_draw_x;
    const int local_y = mouse_y - craft_draw_y;

    // Check if click is within crafting grid bounds
    const int craft_width = crafting_slot_width * crafting_cols * inv_draw_scale;
    const int craft_height = crafting_slot_height * crafting_rows * inv_draw_scale;

    if(local_x < 0 || local_x >= craft_width || local_y < 0 || local_y >= craft_height)
        return INVALID_SLOT;

    const int slot_draw_size = crafting_slot_width * inv_draw_scale;
    const int col = local_x / slot_draw_size;
    const int row = local_y / slot_draw_size;

    if(col >= crafting_cols || row >= crafting_rows)
        return INVALID_SLOT;

    return row * crafting_cols + col;
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

    char count_text[12];
    snprintf(count_text, sizeof(count_text), "%u", count);
    drawString(count_text, 0xFFFF, tex, x + inv_draw_slot_inset + 20, y + inv_draw_slot_inset + 2);
}

bool InventoryTask::isHoldingItem() const
{
    return getBLOCK(held_block) != BLOCK_AIR && held_count > 0;
}

void InventoryTask::tryCraft()
{
    // Count items in crafting grid
    int log_count = 0, plank_count = 0;
    
    for(int i = 0; i < CRAFTING_INPUT_COUNT; ++i)
    {
        BLOCK b = getBLOCK(crafting_input[i]);
        if(b == BLOCK_WOOD)
            log_count += crafting_counts[i];
        else if(b == BLOCK_PLANKS_NORMAL)
            plank_count += crafting_counts[i];
    }

    BLOCK_WDATA new_output = BLOCK_AIR;
    unsigned int new_count = 0;

    // Recipe 1: 1 Log → 4 Planks
    if(log_count >= 1 && plank_count == 0)
    {
        new_output = getBLOCKWDATA(BLOCK_PLANKS_NORMAL, 0);
        new_count = 4;
    }
    // Recipe 2: 4 Planks → 1 Crafting Table
    else if(plank_count >= 4 && log_count == 0)
    {
        new_output = getBLOCKWDATA(BLOCK_CRAFTING_TABLE, 0);
        new_count = 1;
    }

    crafting_output = new_output;
    crafting_output_count = new_count;
}

void InventoryTask::handleLeftClick(int slot)
{
    // Handle crafting output slot (read-only, can only pick up)
    if(slot == CRAFTING_OUTPUT_SLOT)
    {
        if(crafting_output_count > 0 && getBLOCK(crafting_output) != BLOCK_AIR)
        {
            if(!isHoldingItem())
            {
                held_block = crafting_output;
                held_count = crafting_output_count;
                
                // Consume ingredients from crafting grid
                if(getBLOCK(crafting_output) == BLOCK_CRAFTING_TABLE)
                {
                    // Consume 4 planks from the grid (can be spread across slots)
                    int remaining = 4;
                    for(int i = 0; i < CRAFTING_INPUT_COUNT && remaining > 0; ++i)
                    {
                        if(getBLOCK(crafting_input[i]) == BLOCK_PLANKS_NORMAL)
                        {
                            int consume = std::min(remaining, (int)crafting_counts[i]);
                            crafting_counts[i] -= consume;
                            remaining -= consume;
                            if(crafting_counts[i] == 0)
                                crafting_input[i] = BLOCK_AIR;
                        }
                    }
                }
                else if(getBLOCK(crafting_output) == BLOCK_PLANKS_NORMAL)
                {
                    // Consume 1 log
                    for(int i = 0; i < CRAFTING_INPUT_COUNT; ++i)
                    {
                        if(getBLOCK(crafting_input[i]) == BLOCK_WOOD && crafting_counts[i] > 0)
                        {
                            crafting_counts[i]--;
                            if(crafting_counts[i] == 0)
                                crafting_input[i] = BLOCK_AIR;
                            break;
                        }
                    }
                }
                
                crafting_output = BLOCK_AIR;
                crafting_output_count = 0;
                tryCraft();
                return;
            }
            else if(held_block == crafting_output)
            {
                // Add output to held items
                held_count++;
                
                // Consume ingredients
                if(getBLOCK(crafting_output) == BLOCK_CRAFTING_TABLE)
                {
                    // Consume 4 planks
                    int remaining = 4;
                    for(int i = 0; i < CRAFTING_INPUT_COUNT && remaining > 0; ++i)
                    {
                        if(getBLOCK(crafting_input[i]) == BLOCK_PLANKS_NORMAL)
                        {
                            int consume = std::min(remaining, (int)crafting_counts[i]);
                            crafting_counts[i] -= consume;
                            remaining -= consume;
                            if(crafting_counts[i] == 0)
                                crafting_input[i] = BLOCK_AIR;
                        }
                    }
                }
                else if(getBLOCK(crafting_output) == BLOCK_PLANKS_NORMAL)
                {
                    // Consume 1 log
                    for(int i = 0; i < CRAFTING_INPUT_COUNT; ++i)
                    {
                        if(getBLOCK(crafting_input[i]) == BLOCK_WOOD && crafting_counts[i] > 0)
                        {
                            crafting_counts[i]--;
                            if(crafting_counts[i] == 0)
                                crafting_input[i] = BLOCK_AIR;
                            break;
                        }
                    }
                }
                
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
    // Handle crafting output slot (read-only, can only pick up one)
    if(slot == CRAFTING_OUTPUT_SLOT)
    {
        if(crafting_output_count > 0 && getBLOCK(crafting_output) != BLOCK_AIR)
        {
            if(!isHoldingItem())
            {
                held_block = crafting_output;
                held_count = 1;
                
                // Consume ingredients
                if(getBLOCK(crafting_output) == BLOCK_CRAFTING_TABLE)
                {
                    // Consume 4 planks
                    int remaining = 4;
                    for(int i = 0; i < CRAFTING_INPUT_COUNT && remaining > 0; ++i)
                    {
                        if(getBLOCK(crafting_input[i]) == BLOCK_PLANKS_NORMAL)
                        {
                            int consume = std::min(remaining, (int)crafting_counts[i]);
                            crafting_counts[i] -= consume;
                            remaining -= consume;
                            if(crafting_counts[i] == 0)
                                crafting_input[i] = BLOCK_AIR;
                        }
                    }
                }
                else if(getBLOCK(crafting_output) == BLOCK_PLANKS_NORMAL)
                {
                    // Consume 1 log
                    for(int i = 0; i < CRAFTING_INPUT_COUNT; ++i)
                    {
                        if(getBLOCK(crafting_input[i]) == BLOCK_WOOD && crafting_counts[i] > 0)
                        {
                            crafting_counts[i]--;
                            if(crafting_counts[i] == 0)
                                crafting_input[i] = BLOCK_AIR;
                            break;
                        }
                    }
                }
                
                crafting_output = BLOCK_AIR;
                crafting_output_count = 0;
                tryCraft();
                return;
            }
            else if(held_block == crafting_output)
            {
                held_count++;
                
                // Consume ingredients
                if(getBLOCK(crafting_output) == BLOCK_CRAFTING_TABLE)
                {
                    // Consume 4 planks
                    int remaining = 4;
                    for(int i = 0; i < CRAFTING_INPUT_COUNT && remaining > 0; ++i)
                    {
                        if(getBLOCK(crafting_input[i]) == BLOCK_PLANKS_NORMAL)
                        {
                            int consume = std::min(remaining, (int)crafting_counts[i]);
                            crafting_counts[i] -= consume;
                            remaining -= consume;
                            if(crafting_counts[i] == 0)
                                crafting_input[i] = BLOCK_AIR;
                        }
                    }
                }
                else if(getBLOCK(crafting_output) == BLOCK_PLANKS_NORMAL)
                {
                    // Consume 1 log
                    for(int i = 0; i < CRAFTING_INPUT_COUNT; ++i)
                    {
                        if(getBLOCK(crafting_input[i]) == BLOCK_WOOD && crafting_counts[i] > 0)
                        {
                            crafting_counts[i]--;
                            if(crafting_counts[i] == 0)
                                crafting_input[i] = BLOCK_AIR;
                            break;
                        }
                    }
                }
                
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

void InventoryTask::render()
{
    drawBackground();

    const int inv_w = static_cast<int>(inventory2.width) * inv_draw_scale;
    const int inv_h = static_cast<int>(inventory2.height) * inv_draw_scale;
    const int inv_x = inventoryOriginX();
    const int inv_y = inventoryOriginY();

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

    const int pitch = inv_draw_pitch;
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

    // Draw crafting grid items
    const int craft_draw_x = inv_x + crafting_src_x * inv_draw_scale;
    const int craft_draw_y = inv_y + crafting_src_y * inv_draw_scale;
    const int slot_draw_size = crafting_slot_width * inv_draw_scale;
    
    for(int row = 0; row < crafting_rows; ++row)
        for(int col = 0; col < crafting_cols; ++col)
        {
            const int craft_slot = row * crafting_cols + col;
            const int x = craft_draw_x + col * slot_draw_size;
            const int y = craft_draw_y + row * slot_draw_size;
            
            const BLOCK_WDATA block = crafting_input[craft_slot];
            const unsigned int count = crafting_counts[craft_slot];
            if(getBLOCK(block) != BLOCK_AIR && count > 0)
            {
#ifdef _TINSPIRE
                const TextureAtlasEntry &icon_tex = global_block_renderer.materialTexture(block).resized;
                drawTexture(*terrain_resized, *screen,
                            icon_tex.left, icon_tex.top,
                            icon_tex.right - icon_tex.left, icon_tex.bottom - icon_tex.top,
                            x, y,
                            slot_draw_size, slot_draw_size);
#else
                const int icon_w = 24;
                const int icon_h = 24;
                const int preview_x = x + (slot_draw_size - icon_w) / 2;
                const int preview_y = y + (slot_draw_size - icon_h) / 2;
                global_block_renderer.drawPreview(block, *screen, preview_x, preview_y);
#endif
                
                char count_text[12];
                snprintf(count_text, sizeof(count_text), "%u", count);
                drawString(count_text, 0xFFFF, *screen, x + inv_draw_slot_inset + 20, y + inv_draw_slot_inset + 2);
            }
        }

    // Draw crafting output
    const int output_src_w = 160 - 143 + 1;
    const int output_src_h = 52 - 35 + 1;
    const int output_draw_x = inv_x + crafting_output_src_x * inv_draw_scale;
    const int output_draw_y = inv_y + crafting_output_src_y * inv_draw_scale;
    const int output_draw_w = output_src_w * inv_draw_scale;
    const int output_draw_h = output_src_h * inv_draw_scale;
    
    if(getBLOCK(crafting_output) != BLOCK_AIR && crafting_output_count > 0)
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
        
        char count_text[12];
        snprintf(count_text, sizeof(count_text), "%u", crafting_output_count);
        drawString(count_text, 0xFFFF, *screen, output_draw_x + inv_draw_slot_inset + 4, output_draw_y + inv_draw_slot_inset + 2);
    }

#ifndef _TINSPIRE
    int mx = 0, my = 0;
    SDL_GetMouseState(&mx, &my);
    if(getBLOCK(held_block) != BLOCK_AIR && held_count > 0)
    {
        global_block_renderer.drawPreview(held_block, *screen, mx - 8, my - (getBLOCK(held_block) == BLOCK_DOOR ? 14 : 10));
        char count_text[12];
        snprintf(count_text, sizeof(count_text), "%u", held_count);
        drawString(count_text, 0xFFFF, *screen, mx + 8, my + 8);
    }
#endif
}

void InventoryTask::logic()
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
    }
#endif
}
