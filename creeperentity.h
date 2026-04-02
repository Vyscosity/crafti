#ifndef CREEPERENTITY_H
#define CREEPERENTITY_H

#include <vector>

#include "gl.h"
#include "aabb.h"
#include "chunk.h"

// Creeper mob: wanders, fuses and explodes near the player (no gunpowder on self-destruct).
struct CreeperEntity
{
    GLFix x, y, z;
    GLFix vx, vy, vz;
    GLFix yaw;

    GLFix walk_timer;
    GLFix swing_intensity;

    int health;
    int hurt_time;
    int hurt_resistant;
    int death_time;

    int dir_timer;
    bool on_ground;
    bool loot_spawned;

    /** >0 while charging explosion; 0 when idle or after blast. */
    int fuse_timer;
    bool died_by_explosion;

    AABB aabb;

    static const GLFix WIDTH;
    static const GLFix HEIGHT;

    CreeperEntity();
    CreeperEntity(GLFix x, GLFix y, GLFix z);

    void update();
    void applyMeleeDamage(int amount, GLFix attacker_yaw);
    bool isAliveMob() const { return health > 0; }

    void render() const;
};

extern std::vector<CreeperEntity> creeper_entities;

void initCreeperEntities();
void updateCreeperEntities();
void renderCreeperEntities();

#endif
