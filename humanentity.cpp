#include "humanentity.h"

#include <cstdlib>
#include <cmath>

#include "gl.h"
#include "world.h"
#include "terrain.h"
#include "fastmath.h"

#include "textures/steve.h"

const GLFix HumanEntity::WIDTH  = GLFix(70);
const GLFix HumanEntity::HEIGHT = GLFix(208);

std::vector<HumanEntity> human_entities;

HumanEntity::HumanEntity()
    : x(0), y(GLFix(World::HEIGHT * Chunk::SIZE) * BLOCK_SIZE), z(0),
      vx(0), vy(0), vz(0),
      yaw(0), walk_timer(0),
      health(20), hurt_time(0), hurt_resistant(0), death_time(0)
{
    aabb = { x - WIDTH / 2, y, z - WIDTH / 2, x + WIDTH / 2, y + HEIGHT, z + WIDTH / 2 };
}

HumanEntity::HumanEntity(GLFix px, GLFix py, GLFix pz)
    : x(px), y(py), z(pz),
      vx(0), vy(0), vz(0),
      yaw(GLFix(rand() % 360)), walk_timer(0),
      health(20), hurt_time(0), hurt_resistant(0), death_time(0)
{
    aabb = { x - WIDTH / 2, y, z - WIDTH / 2, x + WIDTH / 2, y + HEIGHT, z + WIDTH / 2 };
}

void HumanEntity::update()
{
    if(hurt_time > 0)
        --hurt_time;
    if(hurt_resistant > 0)
        --hurt_resistant;

    if(health <= 0)
    {
        if(death_time <= 20)
            ++death_time;
        vx *= GLFix(0.92f);
        vz *= GLFix(0.92f);
    }
    else if((rand() % 32) == 0)
    {
        int r = rand() % 8;
        const GLFix speed(4);
        if(r < 6)
        {
            yaw = GLFix(rand() % 360);
            vx = GLFix(fast_sin(yaw)) * speed;
            vz = GLFix(fast_cos(yaw)) * speed;
        }
        else
        {
            vx = 0;
            vz = 0;
        }
    }

    if(!world.intersect(aabb))
    {
        AABB moved = aabb;
        moved.low_x += vx;
        moved.high_x += vx;
        if(!world.intersect(moved))
        {
            x += vx;
            aabb = moved;
        }
        else
            vx = -vx;

        moved = aabb;
        moved.low_z += vz;
        moved.high_z += vz;
        if(!world.intersect(moved))
        {
            z += vz;
            aabb = moved;
        }
        else
            vz = -vz;

        AABB moved_y = aabb;
        moved_y.low_y += vy;
        moved_y.high_y += vy;
        bool hit = world.intersect(moved_y);
        if(!hit)
        {
            y += vy;
            aabb = moved_y;
        }
        else
            vy = 0;

        vy -= GLFix(5);
    }

    aabb = { x - WIDTH / 2, y, z - WIDTH / 2, x + WIDTH / 2, y + HEIGHT, z + WIDTH / 2 };

    if(vx != GLFix(0) || vz != GLFix(0))
    {
        GLFix horizontal_speed = GLFix(std::sqrt((float)(vx * vx + vz * vz)));
        walk_timer += horizontal_speed * GLFix(1.4f);
        walk_timer.normaliseAngle();
    }
}

void HumanEntity::render() const
{
    if(health <= 0 && death_time > 20)
        return;

    if(hurt_time > 0)
    {
        GLFix t = GLFix(hurt_time) / GLFix(10);
        nglSetTextureModulate(GLFix(1), GLFix(1) - t * GLFix(0.52f), GLFix(1) - t * GLFix(0.48f));
    }

    GLFix render_yaw = yaw + GLFix(180);
    render_yaw.normaliseAngle();

    glPushMatrix();
    glTranslatef(x, y + HEIGHT / 2, z);
    nglRotateY(render_yaw);

    if(health <= 0 && death_time > 0)
    {
        float df = float(death_time - 1) / 20.0f * 1.6f;
        if(df < 0.f)
            df = 0.f;
        float f = std::sqrt(df);
        if(f > 1.f)
            f = 1.f;
        nglRotateZ(GLFix(f * 90.0f));
    }

    // Billboarded textured quad placeholder for the mob body.
    const GLFix hw = WIDTH / 2;
    const GLFix hh = HEIGHT / 2;
    const COLOR flags = TEXTURE_TRANSPARENT | TEXTURE_DRAW_BACKFACE;

    glBegin(GL_QUADS);
    nglAddVertex({-hw, -hh, 0, 8, 32, flags});
    nglAddVertex({-hw,  hh, 0, 8, 0,  flags});
    nglAddVertex({ hw,  hh, 0, 24, 0, flags});
    nglAddVertex({ hw, -hh, 0, 24, 32, flags});
    glEnd();

    glPopMatrix();

    nglResetTextureModulate();
}

void HumanEntity::applyMeleeDamage(int amount, GLFix attacker_yaw)
{
    if(health <= 0 || hurt_resistant > 0)
        return;

    health -= amount;
    hurt_time = 10;
    hurt_resistant = 10;

    GLFix ay = attacker_yaw;
    ay.normaliseAngle();
    GLFix kx = GLFix(fast_sin(ay));
    GLFix kz = GLFix(fast_cos(ay));
    vx /= 2;
    vz /= 2;
    vx += kx * GLFix(10);
    vz += kz * GLFix(10);

    vy /= 2;
    vy += GLFix(12);
    const GLFix cap(22);
    if(vy > cap)
        vy = cap;

    if(health < 0)
        health = 0;
}

void initHumanEntities()
{
    human_entities.clear();
    constexpr int COUNT = 4;
    for(int i = 0; i < COUNT; ++i)
    {
        GLFix px = GLFix((rand() % 40) - 20) * BLOCK_SIZE + GLFix(rand() % 64);
        GLFix py = GLFix(World::HEIGHT * Chunk::SIZE) * BLOCK_SIZE;
        GLFix pz = GLFix((rand() % 40) - 15) * BLOCK_SIZE + GLFix(rand() % 64);
        human_entities.emplace_back(px, py, pz);
    }
}

void updateHumanEntities()
{
    for(auto it = human_entities.begin(); it != human_entities.end();)
    {
        it->update();
        if(it->health <= 0 && it->death_time > 20)
            it = human_entities.erase(it);
        else
            ++it;
    }
}

void renderHumanEntities()
{
    if(human_entities.empty())
        return;

    glBindTexture(&steve_tex);
    for(const auto &h : human_entities)
        h.render();
}
