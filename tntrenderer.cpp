#include "world.h"
#include "tntrenderer.h"

void TNTRenderer::tick(const BLOCK_WDATA /*block*/, int local_x, int local_y, int local_z, Chunk &c)
{
    if(c.isBlockPowered(local_x, local_y, local_z))
        explode(local_x, local_y, local_z, c);
}

void TNTRenderer::explode(const int local_x, const int local_y, const int local_z, Chunk &c)
{
    const int gx = local_x + c.x * Chunk::SIZE;
    const int gy = local_y + c.y * Chunk::SIZE;
    const int gz = local_z + c.z * Chunk::SIZE;
    world.explosionTNT(gx, gy, gz);
}
