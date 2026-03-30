#include "inventory.h"

#include <cstdio>
#include <utility>

#include "texturetools.h"
#include "blockrenderer.h"
#include "font.h"
#include "itemicons.h"

#include "textures/inventory.h"
#include "textures/inventory2.h"

Inventory current_inventory;

static constexpr int hotbar_src_x = 0;
static constexpr int hotbar_src_y = 0;
static constexpr int hotbar_src_width = 22 * Inventory::hotbar_slot_count;
static constexpr int hotbar_src_height = 22;
static constexpr int hotbar_slot_src_left = 3;
static constexpr int hotbar_slot_src_top = 3;
static constexpr int hotbar_slot_src_size = 16;
static constexpr int hotbar_slot_src_pitch = 20;

static int hotbarScale()
{
    return SCREEN_WIDTH >= hotbar_src_width * 2 ? 2 : 1;
}

Inventory::Inventory()
{
}

void Inventory::draw(TEXTURE &tex)
{
    const int hotbar_scale = hotbarScale();
    const int hotbar_draw_width = hotbar_src_width * hotbar_scale;
    const int hotbar_draw_height = hotbar_src_height * hotbar_scale;
    const int hotbar_slot_size = hotbar_slot_src_size * hotbar_scale;
    const int hotbar_slot_pitch = hotbar_slot_src_pitch * hotbar_scale;
    const int hotbar_slots_left = hotbar_slot_src_left * hotbar_scale;
    const int hotbar_slots_top = hotbar_slot_src_top * hotbar_scale;

    const int inventory_x = (SCREEN_WIDTH - hotbar_draw_width) / 2;
    const int inventory_y = SCREEN_HEIGHT - hotbar_draw_height - 3;

    drawTexture(inventory, tex, hotbar_src_x, hotbar_src_y, hotbar_src_width, hotbar_src_height, inventory_x, inventory_y, hotbar_draw_width, hotbar_draw_height);
    for(unsigned int i = 0; i < hotbar_slot_count; ++i)
    {
        const BLOCK_WDATA block = entries[i];
        if(counts[i] == 0 || getBLOCK(block) == BLOCK_AIR)
            continue;

        const int slot_x = inventory_x + hotbar_slots_left + static_cast<int>(i) * hotbar_slot_pitch;
        const int slot_y = inventory_y + hotbar_slots_top;
        const int slot_px = hotbar_slot_size;

        if(getBLOCK(block) == BLOCK_ITEM)
        {
            drawItemIcon(block, tex, slot_x, slot_y, slot_px);
        }
        else
        {

    #ifdef _TINSPIRE
        const TextureAtlasEntry &icon_tex = global_block_renderer.materialTexture(block).resized;
        drawTexture(*terrain_resized, tex,
                icon_tex.left, icon_tex.top,
                icon_tex.right - icon_tex.left, icon_tex.bottom - icon_tex.top,
                slot_x, slot_y,
                slot_px, slot_px);
    #else
        if(getBLOCK(block) == BLOCK_DOOR)
        {
            const int door_w = 16;
            const int door_h = 32;
            // DoorRenderer applies an internal +4 x-offset.
            const int preview_x = slot_x + (slot_px - door_w) / 2 - 4;
            const int preview_y = slot_y + (slot_px - door_h) / 2;
            global_block_renderer.drawPreview(block, tex, preview_x, preview_y);
        }
        else
        {
            const int icon_w = 24;
            const int icon_h = 24;
            const int preview_x = slot_x + (slot_px - icon_w) / 2;
            const int preview_y = slot_y + (slot_px - icon_h) / 2;
            global_block_renderer.drawPreview(block, tex, preview_x, preview_y);
        }
#endif
        }

        char count_text[12];
        snprintf(count_text, sizeof(count_text), "%u", counts[i]);
        drawString(count_text, 0xFFFF, tex, slot_x + hotbar_slot_size - 10, slot_y + 2);
    }

    const int selector_src_x = 24;
    const int selector_src_y = 22;
    const int selector_size = 24;
    drawTexture(inventory, tex,
                selector_src_x, selector_src_y, selector_size, selector_size,
                inventory_x + hotbar_slots_left + current_slot * hotbar_slot_pitch - 4 * hotbar_scale,
                inventory_y + hotbar_slots_top - 4 * hotbar_scale,
                selector_size * hotbar_scale, selector_size * hotbar_scale);
}

unsigned int Inventory::height()
{
    return hotbar_src_height * hotbarScale();
}

BLOCK_WDATA Inventory::currentSlot() const
{
    if(counts[current_slot] == 0)
        return BLOCK_AIR;

    return entries[current_slot];
}

BLOCK_WDATA Inventory::slotBlock(int slot) const
{
    if(slot < 0 || slot >= slot_count || counts[slot] == 0)
        return BLOCK_AIR;

    return entries[slot];
}

unsigned int Inventory::slotCount(int slot) const
{
    if(slot < 0 || slot >= slot_count)
        return 0;

    return counts[slot];
}

void Inventory::setSlot(int slot, BLOCK_WDATA block, unsigned int count)
{
    if(slot < 0 || slot >= slot_count)
        return;

    entries[slot] = block;
    counts[slot] = (getBLOCK(block) == BLOCK_AIR || count == 0) ? 0 : count;
    if(counts[slot] == 0)
        entries[slot] = BLOCK_AIR;
}

void Inventory::swapSlots(int a, int b)
{
    if(a < 0 || a >= slot_count || b < 0 || b >= slot_count || a == b)
        return;

    std::swap(entries[a], entries[b]);
    std::swap(counts[a], counts[b]);
}

unsigned int Inventory::currentSlotCount() const
{
    return counts[current_slot];
}

void Inventory::setCurrentSlot(BLOCK_WDATA block, unsigned int count)
{
    setSlot(current_slot, block, count);
}

bool Inventory::addItem(BLOCK_WDATA block, unsigned int count)
{
    if(getBLOCK(block) == BLOCK_AIR || count == 0)
        return false;

    for(unsigned int i = 0; i < slot_count; ++i)
    {
        if(counts[i] > 0 && entries[i] == block)
        {
            counts[i] += count;
            return true;
        }
    }

    for(unsigned int i = 0; i < slot_count; ++i)
    {
        if(counts[i] == 0)
        {
            entries[i] = block;
            counts[i] = count;
            return true;
        }
    }

    return false;
}

bool Inventory::removeFromCurrentSlot(unsigned int count)
{
    if(count == 0 || counts[current_slot] == 0)
        return false;

    if(counts[current_slot] <= count)
    {
        counts[current_slot] = 0;
        entries[current_slot] = BLOCK_AIR;
    }
    else
        counts[current_slot] -= count;

    return true;
}

void Inventory::importLegacyCounts()
{
    for(unsigned int i = 0; i < slot_count; ++i)
    {
        if(getBLOCK(entries[i]) == BLOCK_AIR)
            counts[i] = 0;
        else if(counts[i] == 0)
            counts[i] = 1;
    }
}

void Inventory::previousSlot()
{
    --current_slot;
    if(current_slot < 0)
        current_slot = hotbar_slot_count - 1;
}

void Inventory::nextSlot()
{
    ++current_slot;
    if(current_slot >= hotbar_slot_count)
        current_slot = 0;
}

void Inventory::reset()
{
    for(int i = 0; i < slot_count; ++i)
    {
        entries[i] = BLOCK_AIR;
        counts[i] = 0;
    }
    current_slot = 0;
}
