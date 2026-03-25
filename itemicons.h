#ifndef ITEMICONS_H
#define ITEMICONS_H

#include "terrain.h"

void drawItemIcon(BLOCK_WDATA block, TEXTURE &dest, int x, int y, int size);
const char *getItemName(BLOCK_WDATA block);

#endif