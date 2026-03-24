#ifndef INVENTORYTASK_H
#define INVENTORYTASK_H

#include "task.h"
#include "terrain.h"

class InventoryTask : public Task
{
public:
    virtual void makeCurrent() override;
    virtual void render() override;
    virtual void logic() override;

private:
    // Inventory slot constants
    static constexpr int INVALID_SLOT = -1;
    static constexpr int CRAFTING_SLOT_OFFSET = 36; // After hotbar (9) + storage (27)
    static constexpr int CRAFTING_INPUT_COUNT = 4; // 2x2 grid
    static constexpr int CRAFTING_OUTPUT_SLOT = CRAFTING_SLOT_OFFSET + CRAFTING_INPUT_COUNT;
    
    int slotFromMouse(int mouse_x, int mouse_y) const;
    int craftingSlotFromMouse(int mouse_x, int mouse_y) const;
    void drawSlotItem(TEXTURE &tex, int slot, int x, int y);
    bool isHoldingItem() const;
    void handleLeftClick(int slot);
    void handleRightClick(int slot);
    void tryCraft();
    bool isCraftingSlot(int slot) const;

    BLOCK_WDATA held_block = BLOCK_AIR;
    unsigned int held_count = 0;
    bool left_mouse_was_down = false;
    bool right_mouse_was_down = false;
    
    // Crafting table
    BLOCK_WDATA crafting_input[CRAFTING_INPUT_COUNT] = {};
    unsigned int crafting_counts[CRAFTING_INPUT_COUNT] = {};
    BLOCK_WDATA crafting_output = BLOCK_AIR;
    unsigned int crafting_output_count = 0;
};

extern InventoryTask inventory_task;

#endif // INVENTORYTASK_H
