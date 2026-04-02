#ifndef ITEMICONS_H
#define ITEMICONS_H

#include "terrain.h"

void drawItemIcon(BLOCK_WDATA block, TEXTURE &dest, int x, int y, int size);
TextureAtlasEntry itemIconAtlasUV(BLOCK_WDATA block);
const TEXTURE *itemsTextureAtlas();
const char *getItemName(BLOCK_WDATA block);

#endif