#include "inventorytask.h"

#ifndef _TINSPIRE
#include <SDL/SDL.h>
#endif

#include <cstdio>

#include "blockrenderer.h"
#include "font.h"
#include "inventory.h"
#include "worldtask.h"

#include "textures/inventory2.h"

InventoryTask inventory_task;

namespace {
constexpr int inv_src_slot_size = 16;
constexpr int inv_src_pitch = 17;
constexpr int hotbar_src_x = 8;
constexpr int hotbar_src_y = 142;
constexpr int storage_src_x = 8;
constexpr int storage_src_y = 84;
constexpr int storage_cols = 9;
constexpr int storage_rows = 3;
}

void InventoryTask::makeCurrent()
{
    if(!background_saved)
        saveBackground();

    Task::makeCurrent();
}

int InventoryTask::slotFromMouse(int mouse_x, int mouse_y) const
{
    const int inv_x = (SCREEN_WIDTH - inventory2.width) / 2;
    const int inv_y = (SCREEN_HEIGHT - inventory2.height) / 2;

    const int local_x = mouse_x - inv_x;
    const int local_y = mouse_y - inv_y;

    for(int i = 0; i < Inventory::hotbar_slot_count; ++i)
    {
        const int sx = hotbar_src_x + i * inv_src_pitch;
        const int sy = hotbar_src_y;
        if(local_x >= sx && local_x < sx + inv_src_slot_size && local_y >= sy && local_y < sy + inv_src_slot_size)
            return i;
    }

    for(int row = 0; row < storage_rows; ++row)
        for(int col = 0; col < storage_cols; ++col)
        {
            const int sx = storage_src_x + col * inv_src_pitch;
            const int sy = storage_src_y + row * inv_src_pitch;
            if(local_x >= sx && local_x < sx + inv_src_slot_size && local_y >= sy && local_y < sy + inv_src_slot_size)
                return Inventory::hotbar_slot_count + row * storage_cols + col;
        }

    return -1;
}

void InventoryTask::drawSlotItem(TEXTURE &tex, int slot, int x, int y)
{
    const BLOCK_WDATA block = current_inventory.slotBlock(slot);
    const unsigned int count = current_inventory.slotCount(slot);
    if(getBLOCK(block) == BLOCK_AIR || count == 0)
        return;

    global_block_renderer.drawPreview(block, tex, x - 4, y - (getBLOCK(block) == BLOCK_DOOR ? 10 : 6));

    char count_text[12];
    snprintf(count_text, sizeof(count_text), "%u", count);
    drawString(count_text, 0xFFFF, tex, x + 8, y + 2);
}

bool InventoryTask::isHoldingItem() const
{
    return getBLOCK(held_block) != BLOCK_AIR && held_count > 0;
}

void InventoryTask::handleLeftClick(int slot)
{
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

    const int inv_x = (SCREEN_WIDTH - inventory2.width) / 2;
    const int inv_y = (SCREEN_HEIGHT - inventory2.height) / 2;
    int src_x = 0;
    int src_y = 0;
    int dst_x = inv_x;
    int dst_y = inv_y;
    int draw_w = static_cast<int>(inventory2.width);
    int draw_h = static_cast<int>(inventory2.height);

    if(dst_x < 0)
    {
        src_x = -dst_x;
        draw_w += dst_x;
        dst_x = 0;
    }
    if(dst_y < 0)
    {
        src_y = -dst_y;
        draw_h += dst_y;
        dst_y = 0;
    }

    if(draw_w > 0 && draw_h > 0)
        drawTextureOverlay(inventory2, src_x, src_y, *screen, dst_x, dst_y, draw_w, draw_h);

    for(int i = 0; i < Inventory::hotbar_slot_count; ++i)
    {
        const int x = inv_x + hotbar_src_x + i * inv_src_pitch;
        const int y = inv_y + hotbar_src_y;
        drawSlotItem(*screen, i, x, y);
    }

    for(int row = 0; row < storage_rows; ++row)
        for(int col = 0; col < storage_cols; ++col)
        {
            const int slot = Inventory::hotbar_slot_count + row * storage_cols + col;
            const int x = inv_x + storage_src_x + col * inv_src_pitch;
            const int y = inv_y + storage_src_y + row * inv_src_pitch;
            drawSlotItem(*screen, slot, x, y);
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
    // On calculator builds, close with the block list key.
    if(keyPressed(KEY_NSPIRE_PERIOD))
    {
        world_task.makeCurrent();
        key_held_down = true;
    }
#endif
}
