#include "inventory.h"

#include <cstdio>
#include <utility>

#include "texturetools.h"
#include "blockrenderer.h"
#include "font.h"

#include "textures/inventory.h"
#include "textures/inventory2.h"

Inventory current_inventory;

static constexpr int hotbar_src_x = 8;
static constexpr int hotbar_src_y = 142;
static constexpr int hotbar_src_width = 160;
static constexpr int hotbar_src_height = 16;
static constexpr int hotbar_scale = 2;
static constexpr int hotbar_draw_width = hotbar_src_width * hotbar_scale;
static constexpr int hotbar_draw_height = hotbar_src_height * hotbar_scale;
static constexpr int hotbar_slot_pitch = 17 * hotbar_scale;
static constexpr int hotbar_slot_size = 16 * hotbar_scale;
static constexpr int hotbar_slot_inset = 1 * hotbar_scale;

Inventory::Inventory()
{
}

void Inventory::draw(TEXTURE &tex)
{
    const int inventory_x = (SCREEN_WIDTH - hotbar_draw_width) / 2;
    const int inventory_y = SCREEN_HEIGHT - hotbar_draw_height - 3;

    drawTexture(inventory2, tex, hotbar_src_x, hotbar_src_y, hotbar_src_width, hotbar_src_height, inventory_x, inventory_y, hotbar_draw_width, hotbar_draw_height);
    for(unsigned int i = 0; i < hotbar_slot_count; ++i)
    {
        const BLOCK_WDATA block = entries[i];
        if(counts[i] == 0 || getBLOCK(block) == BLOCK_AIR)
            continue;

        const int slot_x = inventory_x + static_cast<int>(i) * hotbar_slot_pitch;
        const int preview_x = slot_x + hotbar_slot_inset + 8;
        const int preview_y = inventory_y + hotbar_slot_inset + (getBLOCK(block) == BLOCK_DOOR ? -6 : -2);
        global_block_renderer.drawPreview(block, tex, preview_x, preview_y);

        char count_text[12];
        snprintf(count_text, sizeof(count_text), "%u", counts[i]);
        drawString(count_text, 0xFFFF, tex, slot_x + hotbar_slot_inset + 20, inventory_y + hotbar_slot_inset + 2);
    }

    const int selector_src_x = 24;
    const int selector_src_y = 22;
    const int selector_size = 24;
    drawTexture(inventory, tex,
                selector_src_x, selector_src_y, selector_size, selector_size,
                inventory_x + current_slot * hotbar_slot_pitch - 8,
                inventory_y - 8,
                selector_size * hotbar_scale, selector_size * hotbar_scale);
}

unsigned int Inventory::height()
{
    return hotbar_draw_height;
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
