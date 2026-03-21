#ifndef INVENTORY_H
#define INVENTORY_H

#include "gl.h"
#include "terrain.h"

class Inventory
{
public:
    Inventory();

    void draw(TEXTURE &tex);

    static unsigned int height();
    BLOCK_WDATA currentSlot() const;
    unsigned int currentSlotCount() const;
    int currentSlotIndex() const { return current_slot; }

    void setCurrentSlot(BLOCK_WDATA block, unsigned int count = 1);
    bool addItem(BLOCK_WDATA block, unsigned int count = 1);
    bool removeFromCurrentSlot(unsigned int count = 1);
    void importLegacyCounts();

    BLOCK_WDATA slotBlock(int slot) const;
    unsigned int slotCount(int slot) const;
    void setSlot(int slot, BLOCK_WDATA block, unsigned int count);
    void swapSlots(int a, int b);

    void previousSlot();
    void nextSlot();

    static constexpr int hotbar_slot_count = 9;
    static constexpr int storage_slot_count = 27;
    static constexpr int slot_count = hotbar_slot_count + storage_slot_count;
    BLOCK_WDATA entries[slot_count] = {};
    unsigned int counts[slot_count] = {};
    int current_slot = 0;
};

extern Inventory current_inventory;

#endif // INVENTORY_H
