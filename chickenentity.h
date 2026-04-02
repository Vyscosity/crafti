#ifndef CHICKENENTITY_H
#define CHICKENENTITY_H

#include <vector>

#include "gl.h"
#include "aabb.h"
#include "chunk.h"

// Overworld chicken (net.minecraft.client.model.ModelChicken + EntityChicken scale).
struct ChickenEntity
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

    unsigned ticks_alive;
    int dir_timer;
    bool on_ground;

    AABB aabb;

    static const GLFix WIDTH;
    static const GLFix HEIGHT;

    ChickenEntity();
    ChickenEntity(GLFix x, GLFix y, GLFix z);

    void update();
    void applyMeleeDamage(int amount, GLFix attacker_yaw);
    bool isAliveMob() const { return health > 0; }

    void render() const;
};

extern std::vector<ChickenEntity> chicken_entities;

void initChickenEntities();
void updateChickenEntities();
void renderChickenEntities();

#endif
