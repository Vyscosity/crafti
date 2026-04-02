#ifndef GROUNDDROPS_H
#define GROUNDDROPS_H

#include "gl.h"
#include "terrain.h"

/** World-space position in nGL units (same as entities). stack is BLOCK_ITEM or a placeable block. */
void spawnWorldDrop(GLFix x, GLFix y, GLFix z, BLOCK_WDATA stack, unsigned int count);
void updateGroundDrops();
void renderGroundDrops();
void clearGroundDrops();

#endif
