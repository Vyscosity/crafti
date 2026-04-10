#ifndef HUMANENTITY_H
#define HUMANENTITY_H

#include <vector>

#include "gl.h"
#include "aabb.h"
#include "chunk.h"

struct HumanEntity
{
    GLFix x, y, z;
    GLFix vx, vy, vz;

    GLFix yaw;

    GLFix walk_timer;

    int health;
    int hurt_time;
    int hurt_resistant;
    int death_time;

    AABB aabb;
    static const GLFix WIDTH;
    static const GLFix HEIGHT;

    HumanEntity();
    HumanEntity(GLFix x, GLFix y, GLFix z);

    void update();

    void applyMeleeDamage(int amount, GLFix attacker_yaw);

    bool isAliveMob() const { return health > 0; }

    void render() const;
};

extern std::vector<HumanEntity> human_entities;

void initHumanEntities();
void updateHumanEntities();
void renderHumanEntities();

#endif // HUMANENTITY_H