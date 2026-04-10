#include <sys/stat.h>

#ifndef _TINSPIRE
#include <SDL/SDL.h>
#endif

#include "worldtask.h"

#include "aabb.h"
#include "blockrenderer.h"
#include "blocklisttask.h"
#include "menutask.h"
#include "settingstask.h"
#include "fastmath.h"
#include "font.h"
#include "inventory.h"
#include "inventorytask.h"
#include "graphtask.h"

#include "textures/items.h"

#include "textures/blockselection.h"
#include "textures/inventory.h"
#include "textures/icons.h"

#include "deathtask.h"
#include "humanentity.h"
#include "chickenentity.h"
#include "creeperentity.h"
#include "grounddrops.h"

WorldTask world_task;

extern unsigned char font_dat[];

static unsigned int measureTextWidth(const char *str)
{
    unsigned int w = 0;
    while(*str)
        w += font_dat[17 + static_cast<unsigned char>(*str++)];
    return w;
}

constexpr GLFix  WorldTask::player_width,  WorldTask::player_height,  WorldTask::eye_pos;

static BLOCK_WDATA inventoryDropItem(const BLOCK_WDATA block)
{
    if(getBLOCK(block) == BLOCK_STONE)
        return BLOCK_COBBLESTONE;

    if(getBLOCK(block) == BLOCK_GRASS)
        return BLOCK_DIRT;

    if(getBLOCK(block) == BLOCK_LEAVES)
        return BLOCK_AIR;

    // Fast water dropping regular water
    if(getBLOCK(block) == BLOCK_WATER_FAST)
        return getBLOCKWDATA(BLOCK_WATER, RANGE_WATER);

    if(global_block_renderer.isOriented(block))
        return getBLOCK(block);

    return getBLOCKWDATA(getBLOCK(block), getBLOCKDATA(block));
}

static int heldPickaxeTier(const BLOCK_WDATA held)
{
    if(getBLOCK(held) != BLOCK_ITEM)
        return 0;

    switch(static_cast<ItemTexture>(getBLOCKDATA(held)))
    {
    case ItemTexture::WOODEN_PICKAXE:
        return 1;
    case ItemTexture::STONE_PICKAXE:
        return 2;
    case ItemTexture::IRON_PICKAXE:
        return 3;
    case ItemTexture::DIAMOND_PICKAXE:
        return 4;
    case ItemTexture::GOLDEN_PICKAXE:
        return 5;
    default:
        return 0;
    }
}

static int requiredPickaxeTierForDrop(const BLOCK block)
{
    switch(block)
    {
    case BLOCK_STONE:
    case BLOCK_COBBLESTONE:
    case BLOCK_COAL_ORE:
    case BLOCK_FURNACE:
    case BLOCK_NETHERRACK:
        return 1; // Wooden+

    case BLOCK_IRON_ORE:
    case BLOCK_IRON:
        return 2; // Stone+

    case BLOCK_GOLD_ORE:
    case BLOCK_GOLD:
    case BLOCK_DIAMOND_ORE:
    case BLOCK_DIAMOND:
    case BLOCK_REDSTONE_ORE:
        return 3; // Iron+

    default:
        return 0;
    }
}

static bool isPickaxeMinedBlock(const BLOCK block)
{
    return requiredPickaxeTierForDrop(block) > 0;
}

/** Left click / KEY_7 primary: punch nearest mob in crosshair if closer than block hit (MC melee). */
static bool tryMeleeMob()
{
    GLFix yr = world_task.yr;
    yr.normaliseAngle();
    GLFix xr = world_task.xr;
    xr.normaliseAngle();

    GLFix dx = GLFix(fast_sin(yr)) * GLFix(fast_cos(xr));
    GLFix dy = -GLFix(fast_sin(xr));
    GLFix dz = GLFix(fast_cos(yr)) * GLFix(fast_cos(xr));
    const GLFix eye_y = world_task.y + WorldTask::eye_pos;

    HumanEntity *hit_h = nullptr;
    GLFix h_dist = GLFix::maxValue();
    for(auto &h : human_entities)
    {
        if(!h.isAliveMob())
            continue;
        GLFix dist;
        if(h.aabb.intersectsRay(world_task.x, eye_y, world_task.z, dx, dy, dz, dist) == AABB::NONE)
            continue;
        if(dist < GLFix(0))
            continue;
        if(dist < h_dist)
        {
            h_dist = dist;
            hit_h = &h;
        }
    }

    ChickenEntity *hit_c = nullptr;
    GLFix c_dist = GLFix::maxValue();
    for(auto &c : chicken_entities)
    {
        if(!c.isAliveMob())
            continue;
        GLFix dist;
        if(c.aabb.intersectsRay(world_task.x, eye_y, world_task.z, dx, dy, dz, dist) == AABB::NONE)
            continue;
        if(dist < GLFix(0))
            continue;
        if(dist < c_dist)
        {
            c_dist = dist;
            hit_c = &c;
        }
    }

    CreeperEntity *hit_cr = nullptr;
    GLFix cr_dist = GLFix::maxValue();
    for(auto &cr : creeper_entities)
    {
        if(!cr.isAliveMob())
            continue;
        GLFix dist;
        if(cr.aabb.intersectsRay(world_task.x, eye_y, world_task.z, dx, dy, dz, dist) == AABB::NONE)
            continue;
        if(dist < GLFix(0))
            continue;
        if(dist < cr_dist)
        {
            cr_dist = dist;
            hit_cr = &cr;
        }
    }

    if(hit_h == nullptr && hit_c == nullptr && hit_cr == nullptr)
        return false;

    GLFix best_dist = GLFix::maxValue();
    int pick = -1; // 0 human, 1 chicken, 2 creeper
    if(hit_h != nullptr && h_dist < best_dist)
    {
        best_dist = h_dist;
        pick = 0;
    }
    if(hit_c != nullptr && c_dist < best_dist)
    {
        best_dist = c_dist;
        pick = 1;
    }
    if(hit_cr != nullptr && cr_dist < best_dist)
    {
        best_dist = cr_dist;
        pick = 2;
    }
    if(pick < 0)
        return false;

    VECTOR3 hit_block;
    AABB::SIDE side = AABB::NONE;
    GLFix block_dist;
    const bool got_block = world.intersectsRay(world_task.x, eye_y, world_task.z, dx, dy, dz, hit_block, side, block_dist, false);
    if(got_block && side != AABB::NONE && block_dist <= best_dist)
        return false;

    if(pick == 1)
        hit_c->applyMeleeDamage(2, yr);
    else if(pick == 2)
        hit_cr->applyMeleeDamage(2, yr);
    else
        hit_h->applyMeleeDamage(2, yr);
    return true;
}

void WorldTask::hurtPlayer(unsigned int dmg, const char *msg)
{
    if(dmg == 0)
        return;
    if(dmg >= hearts)
        hearts = 0;
    else
        hearts -= dmg;

    if(hearts == 0)
    {
        death_task.makeCurrent();
        return;
    }
    if(msg && msg[0])
        setMessage(msg);
}

void WorldTask::makeCurrent()
{
    Task::background_saved = false;

    Task::makeCurrent();
}

//Invert pixel at (x|y) relative to the center of the screen
void WorldTask::crosshairPixel(int x, int y)
{
    int pos = SCREEN_WIDTH/2 + x + (SCREEN_HEIGHT/2 + y)*SCREEN_WIDTH;
    screen->bitmap[pos] = ~screen->bitmap[pos];
}

void WorldTask::getForward(GLFix *x, GLFix *z)
{
    *x = GLFix(fast_sin(yr)) * speed();
    *z = GLFix(fast_cos(yr)) * speed();
}

void WorldTask::getRight(GLFix *x, GLFix *z)
{
    *x = GLFix(fast_sin((yr + 90).normaliseAngle())) * speed();
    *z = GLFix(fast_cos((yr + 90).normaliseAngle())) * speed();
}

GLFix WorldTask::speed()
{
    GLFix base = 10 * settings_task.getValue(SettingsTask::SPEED) + 10;

    if(keyPressed(KEY_NSPIRE_CTRL)) // Sprint
        return base * 2;

    if(keyPressed(KEY_NSPIRE_SHIFT)) // Sneak
        return base / 2;

    return base;
}

void WorldTask::logic(GLFix dt)
{
    const bool graph_mode = world.worldType() == World::WorldType::Graph;
#ifndef _TINSPIRE
    const Uint8 *desktop_keys = SDL_GetKeyState(nullptr);
    const bool desktop_t_held = desktop_keys[SDLK_t] != 0;
    const bool desktop_g_held = desktop_keys[SDLK_g] != 0;
    const bool desktop_j_held = desktop_keys[SDLK_j] != 0;
    const bool desktop_x_held = desktop_keys[SDLK_x] != 0;
    const bool desktop_z_held = desktop_keys[SDLK_z] != 0;
#else
    const bool desktop_t_held = false;
    const bool desktop_g_held = false;
    const bool desktop_j_held = false;
    const bool desktop_x_held = false;
    const bool desktop_z_held = false;
#endif

    GLFix dx = 0, dz = 0;

    if(keyPressed(KEY_NSPIRE_8)) //Forward
    {
        GLFix dx1, dz1;
        getForward(&dx1, &dz1);

        dx += dx1;
        dz += dz1;
    }
    else if(keyPressed(KEY_NSPIRE_2)) //Backward
    {
        GLFix dx1, dz1;
        getForward(&dx1, &dz1);

        dx -= dx1;
        dz -= dz1;
    }

    if(keyPressed(KEY_NSPIRE_4)) //Left
    {
        GLFix dx1, dz1;
        getRight(&dx1, &dz1);

        dx -= dx1;
        dz -= dz1;
    }
    else if(keyPressed(KEY_NSPIRE_6)) //Right
    {
        GLFix dx1, dz1;
        getRight(&dx1, &dz1);

        dx += dx1;
        dz += dz1;
    }

    dx *= dt;
    dz *= dt;

    if(graph_mode)
    {
        x += dx;
        z += dz;

        GLFix dy = 0;
        if(keyPressed(KEY_NSPIRE_5))
            dy += speed() * dt;
        if(keyPressed(KEY_NSPIRE_CTRL))
            dy -= speed() * dt;

        y += dy;
        if(y < GLFix(2 * BLOCK_SIZE))
            y = GLFix(2 * BLOCK_SIZE);

        vy = 0;
        can_jump = true;
        in_water = false;
        fall_distance = 0;
        safe_spawn_pending = false;
    }
    else if(!world.intersect(aabb))
    {
        AABB aabb_moved = aabb;
        aabb_moved.low_x += dx;
        aabb_moved.high_x += dx;

        if(!world.intersect(aabb_moved))
        {
            x += dx;
            aabb = aabb_moved;
        }

        aabb_moved = aabb;
        aabb_moved.low_z += dz;
        aabb_moved.high_z += dz;

        if(!world.intersect(aabb_moved))
        {
            z += dz;
            aabb = aabb_moved;
        }

        const GLFix vy_before = vy;
        aabb_moved = aabb;
        aabb_moved.low_y += vy * dt;
        aabb_moved.high_y += vy * dt;

        can_jump = world.intersect(aabb_moved);

        const bool landed_from_fall = (vy_before < GLFix(0)) && can_jump && fall_distance > GLFix(0);

        if(!can_jump)
        {
            y += vy * dt;
            aabb = aabb_moved;

            // While falling (downward velocity) and we actually move, accumulate fall distance.
            if(vy_before < GLFix(0))
                fall_distance += (-vy_before) * dt;
        }
        else if(vy > GLFix(0))
        {
            can_jump = false;
            vy = 0;
        }
        else
            vy = 0;

        vy -= GLFix(5) * dt;

        in_water = getBLOCK(world.getBlock((x / BLOCK_SIZE).floor(), ((y + eye_pos) / BLOCK_SIZE).floor(), (z / BLOCK_SIZE).floor())) == BLOCK_WATER
                || getBLOCK(world.getBlock((x / BLOCK_SIZE).floor(), ((y + eye_pos) / BLOCK_SIZE).floor(), (z / BLOCK_SIZE).floor())) == BLOCK_WATER_FAST;

        if(landed_from_fall)
        {
            // Apply fall damage only when not in water, and never during the initial safe spawn.
            if(!safe_spawn_pending && !in_water)
            {
                const int fall_blocks = fall_distance.toInteger<int>() / BLOCK_SIZE;
                const int dmg = fall_blocks > 3 ? (fall_blocks - 3) : 0;
                if(dmg > 0)
                {
                    if(static_cast<unsigned int>(dmg) >= hearts)
                        hearts = 0;
                    else
                        hearts -= static_cast<unsigned int>(dmg);

                    if(hearts == 0)
                    {
                        // Switch to death screen; the player can respawn from there.
                        death_task.makeCurrent();
                        return;
                    }
                    else
                    {
                        setMessage("Ouch!");
                    }
                }
            }

            // Whether we took damage or not, we've impacted.
            fall_distance = 0;
        }

        if(in_water)
            can_jump = true;
    }

    if(!graph_mode && keyPressed(KEY_NSPIRE_5) && can_jump) //Jump
    {
        vy = 50;
        can_jump = false;
    }

#ifndef _TINSPIRE
    int rel_x = 0, rel_y = 0;
    SDL_GetRelativeMouseState(&rel_x, &rel_y);
    if(rel_x != 0 || rel_y != 0)
    {
        yr += GLFix(rel_x) / 3;
        xr += GLFix(rel_y) / 3;
    }
#endif

    if(has_touchpad)
    {
        touchpad_report_t touchpad;
        touchpad_scan(&touchpad);

        if(touchpad.pressed)
        {
            switch(touchpad.arrow)
            {
            case TPAD_ARROW_DOWN:
                xr += speed()/2 * dt;
                break;
            case TPAD_ARROW_UP:
                xr -= speed()/2 * dt;
                break;
            case TPAD_ARROW_LEFT:
                yr -= speed()/2 * dt;
                break;
            case TPAD_ARROW_RIGHT:
                yr += speed()/2 * dt;
                break;
            case TPAD_ARROW_RIGHTDOWN:
                xr += speed()/2 * dt;
                yr += speed()/2 * dt;
                break;
            case TPAD_ARROW_UPRIGHT:
                xr -= speed()/2 * dt;
                yr += speed()/2 * dt;
                break;
            case TPAD_ARROW_DOWNLEFT:
                xr += speed()/2 * dt;
                yr -= speed()/2 * dt;
                break;
            case TPAD_ARROW_LEFTUP:
                xr -= speed()/2 * dt;
                yr -= speed()/2 * dt;
                break;
            }
        }
        else if(tp_had_contact && touchpad.contact)
        {
            yr += (touchpad.x - tp_last_x) / 17;
            xr -= (touchpad.y - tp_last_y) / 17;
        }

        tp_had_contact = touchpad.contact;
        tp_last_x = touchpad.x;
        tp_last_y = touchpad.y;
    }
    else
    {
        if(keyPressed(KEY_NSPIRE_UP))
            xr -= speed()/3 * dt;
        else if(keyPressed(KEY_NSPIRE_DOWN))
            xr += speed()/3 * dt;

        if(keyPressed(KEY_NSPIRE_LEFT))
            yr -= speed()/3 * dt;
        else if(keyPressed(KEY_NSPIRE_RIGHT))
            yr += speed()/3 * dt;
    }

    //Normalisation required for rotation with nglRotate
    yr.normaliseAngle();
    xr.normaliseAngle();

    //xr and yr are normalised, so we can't test for negative values
    if(xr > GLFix(180))
        if(xr <= GLFix(270))
            xr = 269;

    if(xr < GLFix(180))
        if(xr >= GLFix(90))
            xr = 89;

    //Do test only on every second frame, it's expensive
    if(do_test)
    {
        GLFix dx = fast_sin(yr)*fast_cos(xr), dy = -fast_sin(xr), dz = fast_cos(yr)*fast_cos(xr);
        GLFix dist;
        if(!world.intersectsRay(x, y + eye_pos, z, dx, dy, dz, selection_pos, selection_side, dist, in_water))
            selection_side = AABB::NONE;
        else
            selection_pos_abs = {x + dx * dist, y + eye_pos + dy * dist, z + dz * dist};
    }

    world.setPosition(x, y, z);

    if(safe_spawn_pending)
    {
        VECTOR3 hit_pos = {-1, -1, -1};
        AABB::SIDE hit_side = AABB::NONE;
        GLFix dist;
        constexpr GLFix dx = GLFix(0);
        constexpr GLFix dz = GLFix(0);
        constexpr GLFix dy = GLFix(-1);

        // Raycast straight down from the player's eye.
        if(world.intersectsRay(x, y + eye_pos, z, dx, dy, dz, hit_pos, hit_side, dist, false))
        {
            const int hit_block_y = hit_pos.y.toInteger<int>();
            // Place player 1 block above the first solid block hit.
            y = GLFix(hit_block_y + 1) * BLOCK_SIZE + GLFix::minStep();
        }

        // Reset velocities and fall tracking after teleporting.
        vy = 0;
        fall_distance = 0;
        safe_spawn_pending = false;
    }

    do_test = !do_test;

    if(!keyPressed(KEY_NSPIRE_9))
    {
        mining_progress = 0;
        mining_tick_accum = 0;
    }

    if(key_held_down)
    {
        key_held_down = keyPressed(KEY_NSPIRE_ESC) || keyPressed(KEY_NSPIRE_7) || keyPressed(KEY_NSPIRE_1) || keyPressed(KEY_NSPIRE_3) || keyPressed(KEY_NSPIRE_PERIOD) || keyPressed(KEY_NSPIRE_MINUS) || keyPressed(KEY_NSPIRE_PLUS) || keyPressed(KEY_NSPIRE_MENU) || keyPressed(KEY_NSPIRE_A) || desktop_t_held;
        key_held_down = key_held_down || desktop_g_held || desktop_j_held || desktop_x_held || desktop_z_held;
    }

    else if(keyPressed(KEY_NSPIRE_ESC) || keyPressed(KEY_NSPIRE_MENU))
    {
        menu_task.makeCurrent();
        key_held_down = true;
        return;
    }
    else if(keyPressed(KEY_NSPIRE_7)) //Put block down
    {
        key_held_down = true;

        // Melee first; do not return from logic() so mob updates still run this tick (avoids stale state / render glitches).
        if(!tryMeleeMob())
        {
            if(selection_side == AABB::NONE)
                return;

            if(world.intersect(aabb))
                return;

            if(world.blockAction(selection_pos.x, selection_pos.y, selection_pos.z))
                return;

            BLOCK_WDATA current_block = world.getBlock(selection_pos.x, selection_pos.y, selection_pos.z),
                        block_to_place = current_inventory.currentSlot();

            if(getBLOCK(block_to_place) == BLOCK_AIR)
                return;

            if(getBLOCK(block_to_place) == BLOCK_ITEM)
                return;

            // When placing fluid onto a non-full fluid block of the same type, "fill" it
            if(current_block != block_to_place
               && ((getBLOCK(current_block) == BLOCK_WATER && getBLOCK(block_to_place) == BLOCK_WATER)
                   || (getBLOCK(current_block) == BLOCK_LAVA && getBLOCK(block_to_place) == BLOCK_LAVA)))
            {
                world.changeBlock(selection_pos.x, selection_pos.y, selection_pos.z, block_to_place);
                current_inventory.removeFromCurrentSlot();
                return;
            }

            VECTOR3 pos = selection_pos;
            switch(selection_side)
            {
            case AABB::BACK:
                ++pos.z;
                break;
            case AABB::FRONT:
                --pos.z;
                break;
            case AABB::LEFT:
                --pos.x;
                break;
            case AABB::RIGHT:
                ++pos.x;
                break;
            case AABB::BOTTOM:
                --pos.y;
                break;
            case AABB::TOP:
                ++pos.y;
                break;
            default:
                puts("This can't normally happen #1");
                break;
            }

            current_block = world.getBlock(pos.x, pos.y, pos.z);

            //Only set the block if there's air
            if(current_block == BLOCK_AIR || (in_water && getBLOCK(current_block) == BLOCK_WATER))
            {
                bool placed = false;
                if(!global_block_renderer.isOriented(block_to_place))
                {
                    world.changeBlock(pos.x, pos.y, pos.z, block_to_place);
                    placed = true;
                }
                else
                {
                    AABB::SIDE side = selection_side;
                    //If the block is not fully oriented and has been placed on top or bottom of another block, determine the orientation by yr
                    if(!global_block_renderer.isFullyOriented(block_to_place) && (side == AABB::TOP || side == AABB::BOTTOM))
                        side = yr < GLFix(45) ? AABB::FRONT : yr < GLFix(135) ? AABB::LEFT : yr < GLFix(225) ? AABB::BACK : yr < GLFix(315) ? AABB::RIGHT : AABB::FRONT;

                    world.changeBlock(pos.x, pos.y, pos.z, getBLOCKWDATA(block_to_place, side)); //AABB::SIDE is compatible to BLOCK_SIDE
                    placed = true;
                }

                //If the player is stuck now, it's because of the block change, so remove it again
                if(placed && world.intersect(aabb))
                {
                    world.changeBlock(pos.x, pos.y, pos.z, current_block);
                    placed = false;
                }

                if(placed)
                {
                    if(getBLOCK(block_to_place) == BLOCK_FURNACE)
                        inventory_task.ensureFurnaceTile(pos.x, pos.y, pos.z);
                    current_inventory.removeFromCurrentSlot();
                }
            }
        }
    }
    else if(keyPressed(KEY_NSPIRE_9)) //Remove block
    {
        BLOCK_WDATA b = world.getBlock(selection_pos.x, selection_pos.y, selection_pos.z);
        if(selection_side != AABB::NONE && getBLOCK(b) != BLOCK_BEDROCK && getBLOCK(b) != BLOCK_AIR)
        {
            const BLOCK b_type = getBLOCK(b);
            const int pickaxe_tier = heldPickaxeTier(current_inventory.currentSlot());

            if (mining_pos.x != selection_pos.x || mining_pos.y != selection_pos.y || mining_pos.z != selection_pos.z) {
                mining_pos = selection_pos;
                mining_progress = 0;
                mining_tick_accum = 0;

                mining_duration = 30; // Default
                if (b_type == BLOCK_DIRT || b_type == BLOCK_SAND || b_type == BLOCK_LEAVES || b_type == BLOCK_GRASS) mining_duration = 10;
                else if (b_type == BLOCK_STONE || b_type == BLOCK_COBBLESTONE || b_type == BLOCK_IRON_ORE || b_type == BLOCK_COAL_ORE || b_type == BLOCK_FURNACE) mining_duration = 60;
                else if (b_type == BLOCK_WOOD || b_type == BLOCK_PLANKS_NORMAL || b_type == BLOCK_CRAFTING_TABLE) mining_duration = 30;
                else if (b_type == BLOCK_GLASS) mining_duration = 15;
                else if (b_type == BLOCK_IRON || b_type == BLOCK_GOLD || b_type == BLOCK_DIAMOND) mining_duration = 100;

                if(isPickaxeMinedBlock(b_type))
                {
                    if(pickaxe_tier == 0)
                        mining_duration *= 3;
                    else if(pickaxe_tier == 1)
                        mining_duration = mining_duration * 100 / 100;
                    else if(pickaxe_tier == 2)
                        mining_duration = mining_duration * 70 / 100;
                    else if(pickaxe_tier == 3)
                        mining_duration = mining_duration * 50 / 100;
                    else if(pickaxe_tier == 4)
                        mining_duration = mining_duration * 40 / 100;
                    else
                        mining_duration = mining_duration * 35 / 100;

                    if(mining_duration < 3)
                        mining_duration = 3;
                }
            }
            mining_tick_accum += dt;
            while(mining_tick_accum >= GLFix(1))
            {
                mining_tick_accum -= GLFix(1);
                mining_progress++;

                if (mining_progress % 10 == 0) {
                    world.spawnDestructionParticles(selection_pos.x, selection_pos.y, selection_pos.z);
                }

                if (mining_progress >= mining_duration) {
                    world.spawnDestructionParticles(selection_pos.x, selection_pos.y, selection_pos.z);
                    const int required_pickaxe_tier = requiredPickaxeTierForDrop(b_type);
                    const bool can_harvest = required_pickaxe_tier == 0 || pickaxe_tier >= required_pickaxe_tier;
                    if(can_harvest)
                    {
                        const BLOCK_WDATA drop_stack = inventoryDropItem(b);
                        if(getBLOCK(drop_stack) != BLOCK_AIR)
                        {
                            const GLFix sx = selection_pos.x * GLFix(BLOCK_SIZE) + GLFix(BLOCK_SIZE / 2);
                            const GLFix sy = selection_pos.y * GLFix(BLOCK_SIZE) + GLFix(BLOCK_SIZE) + GLFix::minStep();
                            const GLFix sz = selection_pos.z * GLFix(BLOCK_SIZE) + GLFix(BLOCK_SIZE / 2);
                            spawnWorldDrop(sx, sy, sz, drop_stack, 1u);
                        }
                    }
                    if(b_type == BLOCK_FURNACE)
                        inventory_task.removeFurnaceTile(selection_pos.x, selection_pos.y, selection_pos.z);
                    world.changeBlock(selection_pos.x, selection_pos.y, selection_pos.z, BLOCK_AIR);
                    mining_progress = 0;
                    mining_tick_accum = 0;
                    break;
                }
            }
        }
        else
        {
            mining_progress = 0;
            mining_tick_accum = 0;
        }
    }
    else if(keyPressed(KEY_NSPIRE_1)) //Switch inventory slot
    {
        current_inventory.previousSlot();

        key_held_down = true;
    }
    else if(keyPressed(KEY_NSPIRE_3))
    {
        current_inventory.nextSlot();

        key_held_down = true;
    }
    else if(keyPressed(KEY_NSPIRE_PERIOD)) //Open list of blocks (or take screenshot with Ctrl + .)
    {
        if(keyPressed(KEY_NSPIRE_CTRL))
        {
            //Find a filename that doesn't exist
            char buf[45];

            unsigned int i;
            for(i = 0; i <= 999; ++i)
            {
                snprintf(buf, sizeof(buf), "/documents/ndless/screenshot_%d.ppm.tns", i);

                struct stat stat_buf;
                if(stat(buf, &stat_buf) != 0)
                    break;
            }

            if(i > 999 || !saveTextureToFile(*screen, buf))
                setMessage("Screenshot failed!");
            else
            {
                snprintf(message, sizeof(message), "Screenshot taken (%d)!", i);
                message_timeout = 20;
            }
        }
        else
        {
            draw_inventory = false;
            render();
            draw_inventory = true;
            block_list_task.makeCurrent();
        }

        key_held_down = true;
    }
    else if(graph_mode && desktop_z_held)
    {
        const int old_zoom = world.graphZoomPercent();
        if(world.setGraphZoomPercent(old_zoom - 10))
        {
            world.clear();
            char msg[32];
            snprintf(msg, sizeof(msg), "Graph zoom: %d%%", world.graphZoomPercent());
            setMessage(msg);
        }
        else
            setMessage("Graph zoom min: 20%");

        key_held_down = true;
    }
    else if(graph_mode && desktop_x_held)
    {
        const int old_zoom = world.graphZoomPercent();
        if(world.setGraphZoomPercent(old_zoom + 10))
        {
            world.clear();
            char msg[32];
            snprintf(msg, sizeof(msg), "Graph zoom: %d%%", world.graphZoomPercent());
            setMessage(msg);
        }
        else
            setMessage("Graph zoom max: 800%");

        key_held_down = true;
    }
    else if(keyPressed(KEY_NSPIRE_MINUS)) //Decrease max view distance
    {
        int fov = world.fieldOfView() - 1;
        world.setFieldOfView(fov < 1 ? 1 : fov);

        key_held_down = true;
    }
    else if(keyPressed(KEY_NSPIRE_PLUS)) //Increase max view distance
    {
        world.setFieldOfView(world.fieldOfView() + 1);

        key_held_down = true;
    }
    // Handled above with ESC
    else if(graph_mode && desktop_g_held)
    {
        graph_task.makeCurrent();
        key_held_down = true;
    }
    else if(graph_mode && desktop_j_held)
    {
        world.setGraphUnbounded(!world.graphUnbounded());
        world.clear();
        setMessage(world.graphUnbounded() ? "Graph bounds: infinite" : "Graph bounds: [-30,30]");
        key_held_down = true;
    }
    else if(keyPressed(KEY_NSPIRE_A))
    {
        inventory_task.makeCurrent();

        key_held_down = true;
    }
#ifndef _TINSPIRE
    else
    {
        const Uint8 *keys = SDL_GetKeyState(nullptr);
        if(keys[SDLK_t] != 0)
        {
            inventory_task.makeCurrent();
            key_held_down = true;
        }
    }
#endif
    // Discrete tick-based entities: advance ~dt worth of old 1-tick steps (carry fractional remainder).
    sim_tick_accum += dt;
    unsigned sim_steps = 0;
    while(!graph_mode && sim_tick_accum >= GLFix(1) && sim_steps < 8)
    {
        sim_tick_accum -= GLFix(1);
        inventory_task.tickFurnaces(world);
        updateHumanEntities();
        updateChickenEntities();
        updateCreeperEntities();
        updateGroundDrops();
        ++sim_steps;
    }
}

void WorldTask::render()
{
    const bool graph_mode = world.worldType() == World::WorldType::Graph;
    aabb = {x - player_width/2, y, z - player_width/2, x + player_width/2, y + player_height, z + player_width/2};
    //printf("X: %f Y: %f Z: %f XR: %d YR: %d\n", x.toFloat(), y.toFloat(), z.toFloat(), xr.toInt(), yr.toInt());

    glColor3f(0.75f, 0.85f, 1.0f); //c0d8ff background
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glPushMatrix();

    //Inverted rotation of the world
    nglRotateX((GLFix(359) - xr).normaliseAngle());
    nglRotateY((GLFix(359) - yr).normaliseAngle());
    //Inverted translation of the world
    glTranslatef(-x, -y - eye_pos, -z);

    glBindTexture(terrain_current);

    world.render();

    // Render entities only in normal worlds.
    if(!graph_mode)
    {
        renderHumanEntities();
        renderChickenEntities();
        renderCreeperEntities();
        renderGroundDrops();
    }
    // Re-bind terrain texture for the rest of the world rendering
    glBindTexture(terrain_current);

    // Draw selection / breaking indication.
    TextureAtlasEntry tex;
    const bool show_breaking_overlay = mining_progress > 0 && mining_duration > 0 &&
                                       selection_pos.x == mining_pos.x && selection_pos.y == mining_pos.y && selection_pos.z == mining_pos.z;
    if(show_breaking_overlay)
    {
        glBindTexture(terrain_current);
        static constexpr unsigned int breaking_frames = 10; // (0,15) to (9,15)
        unsigned int breaking_frame = (static_cast<unsigned int>(mining_progress) * breaking_frames) / static_cast<unsigned int>(mining_duration);
        if(breaking_frame >= breaking_frames)
            breaking_frame = breaking_frames - 1;

        tex = terrain_atlas[breaking_frame][15].current;
    }
    else
    {
        glBindTexture(&blockselection);

        //Do a quick animation
        const unsigned int blockselection_frame_width = blockselection.width / blockselection_frames;
        tex = textureArea(0, 0, blockselection_frame_width, blockselection.height);
        tex.left += blockselection_frame_width * blockselection_frame;
        tex.right += blockselection_frame_width * blockselection_frame;

        //Only increment the frame nr each 5 frames
        if(++blockselection_frame_fraction == 5)
        {
            blockselection_frame_fraction = 0;

            if(++blockselection_frame == blockselection_frames)
                blockselection_frame = 0;
        }
    }

    const GLFix indicator_x = selection_pos.x * BLOCK_SIZE, indicator_y = selection_pos.y * BLOCK_SIZE, indicator_z = selection_pos.z * BLOCK_SIZE;
    const GLFix selection_offset = 3; //Needed to prevent Z-fighting

    glPushMatrix();
    glTranslatef(indicator_x, indicator_y, indicator_z);

    glBegin(GL_QUADS);
    if(show_breaking_overlay)
    {
        const GLFix block_size_fix = GLFix(BLOCK_SIZE);
        const GLFix minus_offset = GLFix(0) - selection_offset;
        const GLFix plus_offset = block_size_fix + selection_offset;

        // Front
        nglAddVertex({0, 0, minus_offset, tex.left, tex.bottom, TEXTURE_TRANSPARENT | TEXTURE_DRAW_BACKFACE});
        nglAddVertex({0, block_size_fix, minus_offset, tex.left, tex.top, TEXTURE_TRANSPARENT | TEXTURE_DRAW_BACKFACE});
        nglAddVertex({block_size_fix, block_size_fix, minus_offset, tex.right, tex.top, TEXTURE_TRANSPARENT | TEXTURE_DRAW_BACKFACE});
        nglAddVertex({block_size_fix, 0, minus_offset, tex.right, tex.bottom, TEXTURE_TRANSPARENT | TEXTURE_DRAW_BACKFACE});

        // Back
        nglAddVertex({block_size_fix, 0, plus_offset, tex.left, tex.bottom, TEXTURE_TRANSPARENT | TEXTURE_DRAW_BACKFACE});
        nglAddVertex({block_size_fix, block_size_fix, plus_offset, tex.left, tex.top, TEXTURE_TRANSPARENT | TEXTURE_DRAW_BACKFACE});
        nglAddVertex({0, block_size_fix, plus_offset, tex.right, tex.top, TEXTURE_TRANSPARENT | TEXTURE_DRAW_BACKFACE});
        nglAddVertex({0, 0, plus_offset, tex.right, tex.bottom, TEXTURE_TRANSPARENT | TEXTURE_DRAW_BACKFACE});

        // Right
        nglAddVertex({plus_offset, 0, 0, tex.right, tex.bottom, TEXTURE_TRANSPARENT | TEXTURE_DRAW_BACKFACE});
        nglAddVertex({plus_offset, block_size_fix, 0, tex.right, tex.top, TEXTURE_TRANSPARENT | TEXTURE_DRAW_BACKFACE});
        nglAddVertex({plus_offset, block_size_fix, block_size_fix, tex.left, tex.top, TEXTURE_TRANSPARENT | TEXTURE_DRAW_BACKFACE});
        nglAddVertex({plus_offset, 0, block_size_fix, tex.left, tex.bottom, TEXTURE_TRANSPARENT | TEXTURE_DRAW_BACKFACE});

        // Left
        nglAddVertex({minus_offset, 0, block_size_fix, tex.left, tex.bottom, TEXTURE_TRANSPARENT | TEXTURE_DRAW_BACKFACE});
        nglAddVertex({minus_offset, block_size_fix, block_size_fix, tex.left, tex.top, TEXTURE_TRANSPARENT | TEXTURE_DRAW_BACKFACE});
        nglAddVertex({minus_offset, block_size_fix, 0, tex.right, tex.top, TEXTURE_TRANSPARENT | TEXTURE_DRAW_BACKFACE});
        nglAddVertex({minus_offset, 0, 0, tex.right, tex.bottom, TEXTURE_TRANSPARENT | TEXTURE_DRAW_BACKFACE});

        // Top
        nglAddVertex({0, plus_offset, 0, tex.left, tex.bottom, TEXTURE_TRANSPARENT | TEXTURE_DRAW_BACKFACE});
        nglAddVertex({0, plus_offset, block_size_fix, tex.left, tex.top, TEXTURE_TRANSPARENT | TEXTURE_DRAW_BACKFACE});
        nglAddVertex({block_size_fix, plus_offset, block_size_fix, tex.right, tex.top, TEXTURE_TRANSPARENT | TEXTURE_DRAW_BACKFACE});
        nglAddVertex({block_size_fix, plus_offset, 0, tex.right, tex.bottom, TEXTURE_TRANSPARENT | TEXTURE_DRAW_BACKFACE});

        // Bottom
        nglAddVertex({block_size_fix, minus_offset, 0, tex.left, tex.bottom, TEXTURE_TRANSPARENT | TEXTURE_DRAW_BACKFACE});
        nglAddVertex({block_size_fix, minus_offset, block_size_fix, tex.left, tex.top, TEXTURE_TRANSPARENT | TEXTURE_DRAW_BACKFACE});
        nglAddVertex({0, minus_offset, block_size_fix, tex.right, tex.top, TEXTURE_TRANSPARENT | TEXTURE_DRAW_BACKFACE});
        nglAddVertex({0, minus_offset, 0, tex.right, tex.bottom, TEXTURE_TRANSPARENT | TEXTURE_DRAW_BACKFACE});
    }
    else switch(selection_side)
    {
    case AABB::FRONT:
        nglAddVertex({0, 0, selection_pos_abs.z - indicator_z - selection_offset, tex.left, tex.bottom, TEXTURE_TRANSPARENT});
        nglAddVertex({0, BLOCK_SIZE, selection_pos_abs.z - indicator_z - selection_offset, tex.left, tex.top, TEXTURE_TRANSPARENT});
        nglAddVertex({BLOCK_SIZE, BLOCK_SIZE, selection_pos_abs.z - indicator_z - selection_offset, tex.right, tex.top, TEXTURE_TRANSPARENT});
        nglAddVertex({BLOCK_SIZE, 0, selection_pos_abs.z - indicator_z - selection_offset, tex.right, tex.bottom, TEXTURE_TRANSPARENT});
        break;
    case AABB::BACK:
        nglAddVertex({BLOCK_SIZE, 0, selection_pos_abs.z - indicator_z + selection_offset, tex.left, tex.bottom, TEXTURE_TRANSPARENT});
        nglAddVertex({BLOCK_SIZE, BLOCK_SIZE, selection_pos_abs.z - indicator_z + selection_offset, tex.left, tex.top, TEXTURE_TRANSPARENT});
        nglAddVertex({0, BLOCK_SIZE, selection_pos_abs.z - indicator_z + selection_offset, tex.right, tex.top, TEXTURE_TRANSPARENT});
        nglAddVertex({0, 0, selection_pos_abs.z - indicator_z + selection_offset, tex.right, tex.bottom, TEXTURE_TRANSPARENT});
        break;
    case AABB::RIGHT:
        nglAddVertex({selection_pos_abs.x - indicator_x + selection_offset, 0, 0, tex.right, tex.bottom, TEXTURE_TRANSPARENT});
        nglAddVertex({selection_pos_abs.x - indicator_x + selection_offset, BLOCK_SIZE, 0, tex.right, tex.top, TEXTURE_TRANSPARENT});
        nglAddVertex({selection_pos_abs.x - indicator_x + selection_offset, BLOCK_SIZE, BLOCK_SIZE, tex.left, tex.top, TEXTURE_TRANSPARENT});
        nglAddVertex({selection_pos_abs.x - indicator_x + selection_offset, 0, BLOCK_SIZE, tex.left, tex.bottom, TEXTURE_TRANSPARENT});
        break;
    case AABB::LEFT:
        nglAddVertex({selection_pos_abs.x - indicator_x - selection_offset, 0, BLOCK_SIZE, tex.left, tex.bottom, TEXTURE_TRANSPARENT});
        nglAddVertex({selection_pos_abs.x - indicator_x - selection_offset, BLOCK_SIZE, BLOCK_SIZE, tex.left, tex.top, TEXTURE_TRANSPARENT});
        nglAddVertex({selection_pos_abs.x - indicator_x - selection_offset, BLOCK_SIZE, 0, tex.right, tex.top, TEXTURE_TRANSPARENT});
        nglAddVertex({selection_pos_abs.x - indicator_x - selection_offset, 0, 0, tex.right, tex.bottom, TEXTURE_TRANSPARENT});
        break;
    case AABB::TOP:
        nglAddVertex({0, selection_pos_abs.y - indicator_y + selection_offset, 0, tex.left, tex.bottom, TEXTURE_TRANSPARENT});
        nglAddVertex({0, selection_pos_abs.y - indicator_y + selection_offset, BLOCK_SIZE, tex.left, tex.top, TEXTURE_TRANSPARENT});
        nglAddVertex({BLOCK_SIZE, selection_pos_abs.y - indicator_y + selection_offset, BLOCK_SIZE, tex.right, tex.top, TEXTURE_TRANSPARENT});
        nglAddVertex({BLOCK_SIZE, selection_pos_abs.y - indicator_y + selection_offset, 0, tex.right, tex.bottom, TEXTURE_TRANSPARENT});
        break;
    case AABB::BOTTOM:
        nglAddVertex({BLOCK_SIZE, selection_pos_abs.y - indicator_y - selection_offset, 0, tex.left, tex.bottom, TEXTURE_TRANSPARENT});
        nglAddVertex({BLOCK_SIZE, selection_pos_abs.y - indicator_y - selection_offset, BLOCK_SIZE, tex.left, tex.top, TEXTURE_TRANSPARENT});
        nglAddVertex({0, selection_pos_abs.y - indicator_y - selection_offset, BLOCK_SIZE, tex.right, tex.top, TEXTURE_TRANSPARENT});
        nglAddVertex({0, selection_pos_abs.y - indicator_y - selection_offset, 0, tex.right, tex.bottom, TEXTURE_TRANSPARENT});
        break;
    case AABB::NONE:
        break;
    }
    glEnd();

    glPopMatrix();

    glPopMatrix();

    crosshairPixel(0, 0);
    crosshairPixel(-1, 0);
    crosshairPixel(-2, 0);
    crosshairPixel(0, -1);
    crosshairPixel(0, -2);
    crosshairPixel(1, 0);
    crosshairPixel(2, 0);
    crosshairPixel(0, 1);
    crosshairPixel(0, 2);

    // HUD from textures/gui/icons.png (MCP Gui.field_110324_m) — health, hunger, XP (GuiIngame 1.4.x+ layout).
    {
        constexpr int hotbar_src_width = 22 * 9;
        const int hud_scale = SCREEN_WIDTH >= hotbar_src_width * 2 ? 2 : 1;
        constexpr int sp = 9;
        const int row_y = SCREEN_HEIGHT - 39 * hud_scale;
        const int hud_left = SCREEN_WIDTH / 2 - 91 * hud_scale;
        const int hud_right = SCREEN_WIDTH / 2 + 91 * hud_scale;
        const int icon_s = sp * hud_scale;

        for (unsigned int i = 0; i < max_hearts; ++i)
        {
            const int hx = hud_left + static_cast<int>(i) * 8 * hud_scale;
            drawTexture(icons, *screen, 16, 0, sp, sp, hx, row_y, icon_s, icon_s);
            if (i < hearts)
                drawTexture(icons, *screen, 52, 0, sp, sp, hx, row_y, icon_s, icon_s);
        }

        for (unsigned int i = 0; i < max_food; ++i)
        {
            const int fx = hud_right - static_cast<int>(i) * 8 * hud_scale - sp * hud_scale;
            drawTexture(icons, *screen, 16, 27, sp, sp, fx, row_y, icon_s, icon_s);
            if (i < food)
                drawTexture(icons, *screen, 52, 27, sp, sp, fx, row_y, icon_s, icon_s);
        }

        if (xp_level > 0 || xp_bar > 0.001f)
        {
            const int bar_x = SCREEN_WIDTH / 2 - 91 * hud_scale;
            const int bar_y = SCREEN_HEIGHT - 29 * hud_scale;
            constexpr int bar_w = 182;
            constexpr int bar_h = 5;
            drawTexture(icons, *screen, 0, 64, bar_w, bar_h,
                        bar_x, bar_y, bar_w * hud_scale, bar_h * hud_scale);
            int fill = static_cast<int>(xp_bar * 183.0f);
            if (fill > bar_w)
                fill = bar_w;
            if (fill > 0)
                drawTexture(icons, *screen, 0, 69, fill, bar_h,
                            bar_x, bar_y, fill * hud_scale, bar_h * hud_scale);
        }

        if (xp_level > 0)
        {
            char lvl[12];
            snprintf(lvl, sizeof(lvl), "%u", xp_level);
            drawStringCenter(lvl, 0x87E0, *screen, SCREEN_WIDTH / 2,
                             static_cast<unsigned int>(SCREEN_HEIGHT - 35 * hud_scale));
        }
    }

    //Don't draw the inventory when drawing the background for BlockListTask
    if(draw_inventory)
    {
        const BLOCK_WDATA current_slot = current_inventory.currentSlot();
        current_inventory.draw(*screen);
        drawStringCenter(current_inventory.currentSlotCount() == 0 ? "Empty" : global_block_renderer.getName(current_slot), 0xFFFF, *screen, SCREEN_WIDTH / 2, SCREEN_HEIGHT - current_inventory.height() - fontHeight());
        
        // Draw selection indicator using inventory texture at (1,23) to (2,44)
        constexpr int hotbar_src_width = 22 * 9; // 22 * hotbar_slot_count
        constexpr int hotbar_src_height = 22;
        constexpr int hotbar_slot_src_left = 3;
        constexpr int hotbar_slot_src_pitch = 20;
        
        const int hotbar_scale = SCREEN_WIDTH >= hotbar_src_width * 2 ? 2 : 1;
        const int hotbar_draw_width = hotbar_src_width * hotbar_scale;
        const int hotbar_draw_height = hotbar_src_height * hotbar_scale;
        const int hotbar_slot_pitch = hotbar_slot_src_pitch * hotbar_scale;
        const int hotbar_slots_left = hotbar_slot_src_left * hotbar_scale;
        const int hotbar_slots_top = 3 * hotbar_scale;
        
        const int inventory_x = (SCREEN_WIDTH - hotbar_draw_width) / 2;
        const int inventory_y = SCREEN_HEIGHT - hotbar_draw_height - 3;
        
        // Selector: 22x22 from inventory.png at (1,23) to (22,44)
        constexpr int selector_src_x = 1;
        constexpr int selector_src_y = 23;
        constexpr int selector_src_w = 22;
        constexpr int selector_src_h = 22;
        
        const int slot_offset = current_inventory.currentSlotIndex() * hotbar_slot_pitch;
        const int draw_x = inventory_x + hotbar_slots_left + slot_offset - 2 * hotbar_scale;
        const int draw_y = inventory_y + hotbar_slots_top - 2 * hotbar_scale;
        
        drawTexture(inventory, *screen,
                    selector_src_x, selector_src_y, selector_src_w, selector_src_h,
                    draw_x, draw_y,
                    selector_src_w * hotbar_scale, selector_src_h * hotbar_scale);
    }

    if(message_timeout > 0)
    {
        const int message_y = graph_mode ? static_cast<int>(fontHeight()) + 7 : 5;
        drawString(message, 0xFFFF, *screen, 2, message_y);
        --message_timeout;
    }

    if(graph_mode)
    {
        char bounds_msg[64];
        const int zoom = world.graphZoomPercent();
        const int range = world.graphRange();
        if(world.graphUnbounded())
            snprintf(bounds_msg, sizeof(bounds_msg), "Graph zoom:%d%% n:%d x,y:[-inf,+inf]", zoom, world.graphFillDepth());
        else
        {
            const int bound_times_100 = (range * 10000) / zoom;
            const int bound_int = bound_times_100 / 100;
            const int bound_frac = bound_times_100 % 100;
            snprintf(bounds_msg, sizeof(bounds_msg), "Graph zoom:%d%% n:%d x,y:[-%d.%02d,%d.%02d]",
                     zoom, world.graphFillDepth(), bound_int, bound_frac, bound_int, bound_frac);
        }
        drawString(bounds_msg, 0xFFFF, *screen, 2, 5);

        char expr_msg[72];
        snprintf(expr_msg, sizeof(expr_msg), "z=%s", world.graphExpression());
        const unsigned int expr_w = measureTextWidth(expr_msg);
        const int expr_x = std::max(2, static_cast<int>(SCREEN_WIDTH - expr_w - 2));
        drawString(expr_msg, 0xFFFF, *screen, expr_x, 5);
    }

    if(selection_side != AABB::NONE)
    {
        char pos_msg[64];
        const int bx = selection_pos.x.toInteger<int>();
        const int by = selection_pos.y.toInteger<int>();
        const int bz = selection_pos.z.toInteger<int>();

        if(graph_mode && world.graphMode() == World::GraphMode::Complex)
        {
            const int zoom = world.graphZoomPercent();
            const int rx100 = (bx * 10000) / zoom;
            const int iz100 = (bz * 10000) / zoom;

            const int rx_sign = rx100 < 0 ? -1 : 1;
            const int iz_sign = iz100 < 0 ? -1 : 1;
            const int rx_abs = rx100 * rx_sign;
            const int iz_abs = iz100 * iz_sign;

            snprintf(pos_msg, sizeof(pos_msg), "z=%s%d.%02d%s%d.%02di",
                     rx_sign < 0 ? "-" : "",
                     rx_abs / 100,
                     rx_abs % 100,
                     iz_sign < 0 ? "-" : "+",
                     iz_abs / 100,
                     iz_abs % 100);
        }
        else
        {
            snprintf(pos_msg, sizeof(pos_msg), "Block (%d,%d,%d)", bx, by, bz);
        }

        const int y_pos = graph_mode ? static_cast<int>(fontHeight()) + 7 : 5;
        drawString(pos_msg, 0xFFFF, *screen, 2, y_pos);
    }

    #ifdef FPS_COUNTER
        if(message_timeout == 0 && settings_task.getValue(SettingsTask::SHOW_FPS))
        {
            snprintf(this->message, sizeof(this->message), "FPS: %u", fps);
            message_timeout = 20;
        }
    #endif

    frame_counter++;
}

void WorldTask::resetWorld()
{
    x = z = 0;
    y = world.worldType() == World::WorldType::Graph ? GLFix((world.graphOriginY() + 8) * BLOCK_SIZE) : GLFix(World::HEIGHT * Chunk::SIZE * BLOCK_SIZE);
    xr = yr = 0;
    world.generateSeed();
    if(world.worldType() != World::WorldType::Graph)
    {
        initHumanEntities();
        initChickenEntities();
        initCreeperEntities();
    }
    else
    {
        human_entities.clear();
        chicken_entities.clear();
        creeper_entities.clear();
    }
    clearGroundDrops();
    world.clear();
    current_inventory.reset();
    inventory_task.reset();
    block_list_task.current_selection = 1;

    hearts = max_hearts;
    food = max_food;
    xp_level = 0;
    xp_bar = 0.f;
    fall_distance = 0;
    safe_spawn_pending = world.worldType() != World::WorldType::Graph;

    vy = 0;
    can_jump = false;
    tp_had_contact = false;
    in_water = false;
    mining_pos = {-1, -1, -1};
    mining_progress = 0;
    mining_tick_accum = 0;
    sim_tick_accum = 0;
    graph_line_tick_accum = 0;
    message_timeout = 0;
}

void WorldTask::respawnPlayer()
{
    // Respawn without wiping the world.
    hearts = max_hearts;
    food = max_food;
    xp_level = 0;
    xp_bar = 0.f;
    vy = 0;
    fall_distance = 0;
    safe_spawn_pending = world.worldType() != World::WorldType::Graph;

    // Place above the world so the down-ray has something to hit.
    y = world.worldType() == World::WorldType::Graph ? GLFix((world.graphOriginY() + 8) * BLOCK_SIZE) : GLFix(World::HEIGHT * Chunk::SIZE * BLOCK_SIZE);
    in_water = false;
    can_jump = false;
    message_timeout = 0;
}

void WorldTask::setMessage(const char *message)
{
    if(strlen(message) >= sizeof(this->message))
        return;

    strcpy(this->message, message);
    message_timeout = 50;
}
