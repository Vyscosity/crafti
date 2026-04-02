#include "grounddrops.h"

#include <cmath>
#include <cstdlib>
#include <vector>

#include "aabb.h"
#include "blockrenderer.h"
#include "fastmath.h"
#include "gl.h"
#include "inventory.h"
#include "itemicons.h"
#include "particle.h"
#include "terrain.h"
#include "world.h"
#include "worldtask.h"

namespace {

struct GroundDrop
{
    GLFix x, y, z;
    GLFix vx, vy, vz;
    BLOCK_WDATA stack;
    unsigned int count;
    int age_ticks;
};

std::vector<GroundDrop> ground_drops;

/** Collision box lifted by minStep so resting on a block top does not count as intersecting solid below. */
AABB dropCollisionAabb(GLFix x, GLFix y, GLFix z)
{
    const GLFix hw = GLFix(18);
    const GLFix h = GLFix(22);
    const GLFix lift = GLFix::minStep();
    return { x - hw, y + lift, z - hw, x + hw, y + h, z + hw };
}

TextureAtlasEntry blockDropIconUV(BLOCK_WDATA block)
{
    TextureAtlasEntry tae = global_block_renderer.materialTexture(block).current;
    const int tex_width = static_cast<int>(tae.right - tae.left);
    const int tex_height = static_cast<int>(tae.bottom - tae.top);
    if(tex_width > 0 && tex_height > 0)
    {
        tae.left += static_cast<unsigned>(tex_width / 4);
        tae.right -= static_cast<unsigned>(tex_width / 4);
        tae.top += static_cast<unsigned>(tex_height / 4);
        tae.bottom -= static_cast<unsigned>(tex_height / 4);
    }
    return tae;
}

} // namespace

void spawnWorldDrop(GLFix x, GLFix y, GLFix z, BLOCK_WDATA stack, unsigned int count)
{
    if(getBLOCK(stack) == BLOCK_AIR || count == 0)
        return;

    GroundDrop d;
    d.x = x + GLFix((rand() % 31) - 15);
    d.y = y + GLFix(24);
    d.z = z + GLFix((rand() % 31) - 15);
    d.vx = GLFix((rand() % 21) - 10) / GLFix(4);
    d.vz = GLFix((rand() % 21) - 10) / GLFix(4);
    d.vy = GLFix(28);
    d.stack = stack;
    d.count = count;
    d.age_ticks = 0;
    ground_drops.push_back(d);
}

void updateGroundDrops()
{
    const float px = world_task.x.toFloat();
    const float py = world_task.y.toFloat();
    const float pz = world_task.z.toFloat();
    const float chest_y = (world_task.y + WorldTask::eye_pos * GLFix(2) / GLFix(3)).toFloat();
    const float magnet_ty = py * (2.f / 3.f) + chest_y * (1.f / 3.f);

    const float pickup_hr = float(BLOCK_SIZE) * 1.25f;
    const float pickup_hr2 = pickup_hr * pickup_hr;
    const float pickup_vmax = float(BLOCK_SIZE) * 1.125f;

    const float attract_r = float(BLOCK_SIZE) * 3.25f;
    const float attract_vmax = float(BLOCK_SIZE) * 2.25f;
    const float drop_center_y_offset_f = 11.f;
    const GLFix vel_cap = GLFix(42);

    for(auto &d : ground_drops)
    {
        ++d.age_ticks;

        const float dcx = d.x.toFloat();
        const float dcz = d.z.toFloat();
        const float dcy = d.y.toFloat() + drop_center_y_offset_f;
        const float horiz2 = (dcx - px) * (dcx - px) + (dcz - pz) * (dcz - pz);
        const float vdiff = dcy - chest_y;
        const bool in_pickup_box = horiz2 <= pickup_hr2 && vdiff <= pickup_vmax && vdiff >= -pickup_vmax;

        if(!in_pickup_box && horiz2 <= attract_r * attract_r && vdiff <= attract_vmax && vdiff >= -attract_vmax)
        {
            const float tdx = px - dcx;
            const float tdy = magnet_ty - dcy;
            const float tdz = pz - dcz;
            const float dist = std::sqrt(tdx * tdx + tdy * tdy + tdz * tdz);
            constexpr float min_dist = 8.f;
            if(dist > min_dist)
            {
                const float inv = 1.f / dist;
                float pull = 12.f * (1.f - dist / attract_r);
                if(pull < 0.f)
                    pull = 0.f;
                if(pull > 0.f)
                {
                    d.vx += GLFix(tdx * inv * pull);
                    d.vy += GLFix(tdy * inv * pull);
                    d.vz += GLFix(tdz * inv * pull);
                }
            }
        }

        if(d.vx > vel_cap)
            d.vx = vel_cap;
        else if(d.vx < -vel_cap)
            d.vx = -vel_cap;
        if(d.vz > vel_cap)
            d.vz = vel_cap;
        else if(d.vz < -vel_cap)
            d.vz = -vel_cap;
        if(d.vy > vel_cap)
            d.vy = vel_cap;
        else if(d.vy < -vel_cap)
            d.vy = -vel_cap;

        AABB aabb = dropCollisionAabb(d.x, d.y, d.z);
        if(!world.intersect(aabb))
        {
            AABB moved = aabb;
            moved.low_x += d.vx;
            moved.high_x += d.vx;
            if(!world.intersect(moved))
            {
                d.x += d.vx;
                aabb = moved;
            }
            else
                d.vx = GLFix(0);

            moved = aabb;
            moved.low_z += d.vz;
            moved.high_z += d.vz;
            if(!world.intersect(moved))
            {
                d.z += d.vz;
                aabb = moved;
            }
            else
                d.vz = GLFix(0);

            moved = aabb;
            moved.low_y += d.vy;
            moved.high_y += d.vy;
            const bool hit_blocking_y = world.intersect(moved);
            if(!hit_blocking_y)
            {
                d.y += d.vy;
                aabb = moved;
            }
            else
                d.vy = GLFix(0);
        }

        d.vy -= GLFix(5);

        d.vx *= GLFix(0.985f);
        d.vz *= GLFix(0.985f);
    }

    for(size_t i = 0; i < ground_drops.size();)
    {
        const auto &d = ground_drops[i];
        const float fdx = d.x.toFloat() - px;
        const float fdz = d.z.toFloat() - pz;
        const float horiz2 = fdx * fdx + fdz * fdz;
        const float dcy = d.y.toFloat() + drop_center_y_offset_f;
        const float vdiff_pick = dcy - chest_y;
        const bool in_range = horiz2 <= pickup_hr2 && vdiff_pick <= pickup_vmax && vdiff_pick >= -pickup_vmax;

        if(in_range && current_inventory.addItem(d.stack, d.count))
            ground_drops.erase(ground_drops.begin() + i);
        else
            ++i;
    }
}

void renderGroundDrops()
{
    if(ground_drops.empty())
        return;

    glBindTexture(itemsTextureAtlas());
    glBegin(GL_QUADS);
    for(const auto &d : ground_drops)
    {
        if(getBLOCK(d.stack) != BLOCK_ITEM)
            continue;

        const TextureAtlasEntry tae = itemIconAtlasUV(d.stack);
        if(tae.right == 0)
            continue;

        const GLFix bob = fast_sin(GLFix((d.age_ticks * 11) % 360)) * GLFix(6);
        const VECTOR3 c{ d.x, d.y + GLFix(20) + bob, d.z };
        Particle::render(c, GLFix(36), tae);
    }
    glEnd();

    glBindTexture(terrain_current);
    glBegin(GL_QUADS);
    for(const auto &d : ground_drops)
    {
        if(getBLOCK(d.stack) == BLOCK_ITEM)
            continue;

        const TextureAtlasEntry tae = blockDropIconUV(d.stack);
        if(static_cast<int>(tae.right) - static_cast<int>(tae.left) <= 0)
            continue;

        const GLFix bob = fast_sin(GLFix((d.age_ticks * 11) % 360)) * GLFix(6);
        const VECTOR3 c{ d.x, d.y + GLFix(20) + bob, d.z };
        Particle::render(c, GLFix(40), tae);
    }
    glEnd();
}

void clearGroundDrops()
{
    ground_drops.clear();
}
