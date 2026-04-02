#ifndef HUMANENTITY_H
#define HUMANENTITY_H

#include <vector>

#include "gl.h"      // GLFix, TEXTURE, nglAddVertex – found via -I nGL
#include "aabb.h"
#include "chunk.h"   // Chunk::SIZE

// A simple humanoid entity that wanders randomly using the Steve skin.
struct HumanEntity
{
    // World-space feet position (same coordinate frame as the player)
    GLFix x, y, z;

    // Velocity components (applied each tick)
    GLFix vx, vy, vz;

    // Horizontal facing direction (degrees)
    GLFix yaw;

    // Walk-cycle phase (degrees); advances while moving.
    GLFix walk_timer;

    // Smoothed walk amplitude (like EntityLivingBase.limbSwingAmount): 0 at rest,
    // ~1 when moving. Limb rotations are scaled by this so limbs settle down when still.
    GLFix swing_intensity;

    // Survival / melee (Minecraft-style)
    int health;
    int hurt_time;
    int hurt_resistant;
    int death_time;

    // Ticks remaining before the AI picks a new direction
    int dir_timer;

    bool on_ground;

    // Collision bounding box
    AABB aabb;

    // Entity size in nGL units
    // (BLOCK_SIZE is 128; player is 0.6×1.8 blocks)
    static const GLFix WIDTH;
    static const GLFix HEIGHT;

    HumanEntity();
    HumanEntity(GLFix x, GLFix y, GLFix z);

    // One game-logic tick: AI + physics
    void update();

    // Player melee: damage, knockback (EntityLivingBase.knockBack), hurt flash. attacker_yaw in degrees.
    void applyMeleeDamage(int amount, GLFix attacker_yaw);

    bool isAliveMob() const { return health > 0; }

    // Emit all 6 faces of all body parts via nglAddVertex.
    // Call between glBegin(GL_QUADS) / glEnd().
    void render() const;
};

// Global list – managed by the functions below
extern std::vector<HumanEntity> human_entities;

void initHumanEntities();     // spawn initial humans (call on world reset)
void updateHumanEntities();   // call every logic tick
void renderHumanEntities();   // call inside world render pass

#endif // HUMANENTITY_H
