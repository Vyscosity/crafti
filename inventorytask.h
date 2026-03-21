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
    int slotFromMouse(int mouse_x, int mouse_y) const;
    void drawSlotItem(TEXTURE &tex, int slot, int x, int y);
    bool isHoldingItem() const;
    void handleLeftClick(int slot);
    void handleRightClick(int slot);

    BLOCK_WDATA held_block = BLOCK_AIR;
    unsigned int held_count = 0;
    bool left_mouse_was_down = false;
    bool right_mouse_was_down = false;
};

extern InventoryTask inventory_task;

#endif // INVENTORYTASK_H
