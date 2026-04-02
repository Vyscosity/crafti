#include "creeperentity.h"

#include <cstdlib>
#include <cmath>

#include "gl.h"
#include "world.h"
#include "terrain.h"
#include "fastmath.h"
#include "grounddrops.h"
#include "worldtask.h"

#include "textures/creeper.h"
#include "textures/items.h"

const GLFix CreeperEntity::WIDTH = GLFix(70);
const GLFix CreeperEntity::HEIGHT = GLFix(208);

std::vector<CreeperEntity> creeper_entities;

static constexpr int fuse_trigger_ticks = 55;
static const GLFix attract_radius = GLFix(BLOCK_SIZE) * GLFix(4);
/** Player hurt radius matches TNT blast sphere (3 blocks, Euclidean). */
static const GLFix blast_radius = GLFix(BLOCK_SIZE) * GLFix(3);

static inline TextureAtlasEntry skinArea(int u, int v, int w, int h)
{
    return {(unsigned)u, (unsigned)(u + w), (unsigned)v, (unsigned)(v + h)};
}

static inline TextureAtlasEntry mirrorU(TextureAtlasEntry t)
{
    unsigned tmp = t.left;
    t.left = t.right;
    t.right = tmp;
    return t;
}

static void emitQuad(
    GLFix ax, GLFix ay, GLFix az,
    GLFix bx, GLFix by, GLFix bz,
    GLFix cx, GLFix cy, GLFix cz,
    GLFix dx, GLFix dy, GLFix dz,
    const TextureAtlasEntry &tex)
{
    const COLOR flags = TEXTURE_TRANSPARENT | TEXTURE_DRAW_BACKFACE;
    nglAddVertex({ax, ay, az, GLFix((int)tex.left), GLFix((int)tex.bottom), flags});
    nglAddVertex({bx, by, bz, GLFix((int)tex.left), GLFix((int)tex.top), flags});
    nglAddVertex({cx, cy, cz, GLFix((int)tex.right), GLFix((int)tex.top), flags});
    nglAddVertex({dx, dy, dz, GLFix((int)tex.right), GLFix((int)tex.bottom), flags});
}

static void drawCreeperBox(
    GLFix bx, GLFix by, GLFix bz,
    GLFix bw, GLFix bh, GLFix bd,
    int u0, int v0, int wp, int hp, int dp,
    bool mirror = false)
{
    auto top = skinArea(u0 + dp, v0, wp, dp);
    auto bot = skinArea(u0 + dp + wp, v0, wp, dp);
    auto rgt = skinArea(u0, v0 + dp, dp, hp);
    auto frt = skinArea(u0 + dp, v0 + dp, wp, hp);
    auto lft = skinArea(u0 + dp + wp, v0 + dp, dp, hp);
    auto bck = skinArea(u0 + dp + wp + dp, v0 + dp, wp, hp);

    if(mirror)
    {
        top = mirrorU(top);
        bot = mirrorU(bot);
        auto tmp = mirrorU(rgt);
        rgt = mirrorU(lft);
        lft = tmp;
        frt = mirrorU(frt);
        bck = mirrorU(bck);
    }

    GLFix x0 = bx, x1 = bx + bw;
    GLFix y0 = by, y1 = by + bh;
    GLFix z0 = bz, z1 = bz + bd;

    emitQuad(x0, y0, z0, x0, y1, z0, x1, y1, z0, x1, y0, z0, frt);
    emitQuad(x1, y0, z1, x1, y1, z1, x0, y1, z1, x0, y0, z1, bck);
    emitQuad(x0, y0, z1, x0, y1, z1, x0, y1, z0, x0, y0, z0, rgt);
    emitQuad(x1, y0, z0, x1, y1, z0, x1, y1, z1, x1, y0, z1, lft);
    emitQuad(x0, y1, z0, x0, y1, z1, x1, y1, z1, x1, y1, z0, top);
    emitQuad(x1, y0, z0, x1, y0, z1, x0, y0, z1, x0, y0, z0, bot);
}

CreeperEntity::CreeperEntity()
    : x(0), y(GLFix(World::HEIGHT * Chunk::SIZE) * BLOCK_SIZE), z(0),
      vx(0), vy(0), vz(0),
      yaw(0), walk_timer(0), swing_intensity(0),
      health(20), hurt_time(0), hurt_resistant(0), death_time(0),
      dir_timer(60), on_ground(false), loot_spawned(false),
      fuse_timer(0), died_by_explosion(false)
{
    aabb = {x - WIDTH / 2, y, z - WIDTH / 2, x + WIDTH / 2, y + HEIGHT, z + WIDTH / 2};
}

CreeperEntity::CreeperEntity(GLFix px, GLFix py, GLFix pz)
    : x(px), y(py), z(pz),
      vx(0), vy(0), vz(0),
      yaw(GLFix(rand() % 360)), walk_timer(0), swing_intensity(0),
      health(20), hurt_time(0), hurt_resistant(0), death_time(0),
      dir_timer(rand() % 60), on_ground(false), loot_spawned(false),
      fuse_timer(0), died_by_explosion(false)
{
    aabb = {x - WIDTH / 2, y, z - WIDTH / 2, x + WIDTH / 2, y + HEIGHT, z + WIDTH / 2};
}

void CreeperEntity::update()
{
    if(hurt_time > 0)
        --hurt_time;
    if(hurt_resistant > 0)
        --hurt_resistant;

    const bool dead = health <= 0;
    if(dead && !loot_spawned && !died_by_explosion)
    {
        loot_spawned = true;
        const unsigned int n = static_cast<unsigned int>(rand() % 3);
        if(n > 0u)
            spawnWorldDrop(x, y, z,
                           getBLOCKWDATA(BLOCK_ITEM, static_cast<uint8_t>(ItemTexture::GUNPOWDER)), n);
    }
    if(dead)
    {
        if(death_time <= 20)
            ++death_time;
        vx *= GLFix(0.92f);
        vz *= GLFix(0.92f);
        fuse_timer = 0;
        aabb = {x - WIDTH / 2, y, z - WIDTH / 2, x + WIDTH / 2, y + HEIGHT, z + WIDTH / 2};
        return;
    }

    const float px = world_task.x.toFloat();
    const float py = world_task.y.toFloat();
    const float pz = world_task.z.toFloat();
    const float cx = x.toFloat();
    const float cy = y.toFloat();
    const float cz = z.toFloat();
    const float dxp = px - cx;
    const float dzp = pz - cz;
    const float hyp = std::sqrt(dxp * dxp + dzp * dzp);
    const float dyp = std::fabs(py - cy);
    const bool player_near = hyp < attract_radius.toFloat() && dyp < (BLOCK_SIZE * 2.0f);

    if(player_near)
    {
        ++fuse_timer;
        vx = vz = GLFix(0);
        dir_timer = 1;
        if(hyp > 1e-3f)
        {
            static constexpr float kRadToDeg = 180.0f / 3.14159265f;
            float ang = std::atan2(dxp, dzp) * kRadToDeg;
            yaw = GLFix(ang);
        }

        if(fuse_timer >= fuse_trigger_ticks)
        {
            died_by_explosion = true;
            health = 0;
            fuse_timer = 0;
            const int bx = (x / GLFix(BLOCK_SIZE)).floor();
            const int by = ((y + HEIGHT / 2) / GLFix(BLOCK_SIZE)).floor();
            const int bz = (z / GLFix(BLOCK_SIZE)).floor();
            world.explosionTNT(bx, by, bz);
            const float dist3 =
                std::sqrt(dxp * dxp + dyp * dyp + dzp * dzp);
            if(dist3 <= blast_radius.toFloat())
                world_task.hurtPlayer(4, "Kaboom!");
            aabb = {x - WIDTH / 2, y, z - WIDTH / 2, x + WIDTH / 2, y + HEIGHT, z + WIDTH / 2};
            return;
        }
    }
    else
    {
        fuse_timer = 0;
        if(--dir_timer <= 0)
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
            dir_timer = 40 + rand() % 80;
        }
    }

    GLFix old_x = x;
    GLFix old_z = z;

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
        {
            vx = -vx;
            dir_timer = 0;
        }

        moved = aabb;
        moved.low_z += vz;
        moved.high_z += vz;
        if(!world.intersect(moved))
        {
            z += vz;
            aabb = moved;
        }
        else
        {
            vz = -vz;
            dir_timer = 0;
        }

        AABB moved_y = aabb;
        moved_y.low_y += vy;
        moved_y.high_y += vy;
        bool hit = world.intersect(moved_y);
        if(!hit)
        {
            y += vy;
            aabb = moved_y;
            on_ground = false;
        }
        else
        {
            if(vy < GLFix(0))
                on_ground = true;
            vy = 0;
        }
        vy -= GLFix(5);

        if(on_ground && (vx != GLFix(0) || vz != GLFix(0)) && (rand() % 80 == 0))
        {
            vy = GLFix(50);
            on_ground = false;
        }
    }

    aabb = {x - WIDTH / 2, y, z - WIDTH / 2, x + WIDTH / 2, y + HEIGHT, z + WIDTH / 2};

    GLFix ddx = x - old_x;
    GLFix ddz = z - old_z;
    GLFix actual_dist = GLFix(std::sqrt((float)(ddx * ddx + ddz * ddz)));
    GLFix target_amp = actual_dist * GLFix(0.33f);
    if(target_amp > GLFix(1))
        target_amp = GLFix(1);
    swing_intensity += (target_amp - swing_intensity) * GLFix(0.4f);

    GLFix horizontal_speed = GLFix(std::sqrt((float)(vx * vx + vz * vz)));
    if(horizontal_speed > GLFix(0))
    {
        walk_timer += horizontal_speed * GLFix(1.66f);
        walk_timer.normaliseAngle();
    }
}

void CreeperEntity::applyMeleeDamage(int amount, GLFix attacker_yaw)
{
    if(health <= 0 || hurt_resistant > 0)
        return;

    fuse_timer = 0;

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
    if(on_ground)
    {
        vy /= 2;
        vy += GLFix(12);
        const GLFix cap(22);
        if(vy > cap)
            vy = cap;
    }

    if(health < 0)
        health = 0;
}

void CreeperEntity::render() const
{
    if(health <= 0 && death_time > 20)
        return;

    if(hurt_time > 0)
    {
        GLFix t = GLFix(hurt_time) / GLFix(10);
        nglSetTextureModulate(GLFix(1), GLFix(1) - t * GLFix(0.52f), GLFix(1) - t * GLFix(0.48f));
    }
    else if(fuse_timer > 0 && ((fuse_timer / 4) % 2) == 0)
        nglSetTextureModulate(GLFix(1.12f), GLFix(1.12f), GLFix(1.12f));

    const GLFix S = GLFix(BLOCK_SIZE) / GLFix(16);

    // ModelCreeper.setRotationAngles: leg1/leg4 cos(0.6662*limbSwing), leg2/leg3 opposite phase
    // (same leg pairing as biped R/L with 1.4 rad ≈ 80° max swing, not cos(2*t).)
    GLFix t = walk_timer;
    t.normaliseAngle();
    const GLFix cos_t = GLFix(fast_cos(t));
    const GLFix neg_cos_t = -cos_t;
    GLFix l14 = cos_t * GLFix(80) * swing_intensity;
    GLFix l23 = neg_cos_t * GLFix(80) * swing_intensity;
    l14.normaliseAngle();
    l23.normaliseAngle();

    GLFix render_yaw = yaw + GLFix(180);
    render_yaw.normaliseAngle();

    glPushMatrix();
    glTranslatef(x, y, z);
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

    // Head (neck y = 18S from feet; head 8S tall)
    glPushMatrix();
    glTranslatef(0, GLFix(18) * S, 0);
    drawCreeperBox(GLFix(-4) * S, GLFix(0), GLFix(-4) * S,
                   GLFix(8) * S, GLFix(8) * S, GLFix(8) * S,
                   0, 0, 8, 8, 8);
    glPopMatrix();

    // Body
    glPushMatrix();
    glTranslatef(0, GLFix(18) * S, 0);
    drawCreeperBox(GLFix(-4) * S, GLFix(-12) * S, GLFix(-2) * S,
                   GLFix(8) * S, GLFix(12) * S, GLFix(4) * S,
                   16, 16, 8, 12, 4);
    glPopMatrix();

    // Legs: MC pivots (−2,24,±4) and (2,24,±4) → hip height 6S; foot z spans ±4 in model space
    const GLFix hip_y = GLFix(6) * S;
    auto leg = [&](GLFix gx, GLFix gz, GLFix ang)
    {
        glPushMatrix();
        glTranslatef(gx, hip_y, gz);
        nglRotateX(ang);
        drawCreeperBox(GLFix(-2) * S, GLFix(-6) * S, GLFix(-2) * S,
                       GLFix(4) * S, GLFix(6) * S, GLFix(4) * S,
                       0, 16, 4, 6, 4);
        glPopMatrix();
    };
    leg(GLFix(-2) * S, GLFix(4) * S, l14);
    leg(GLFix(2) * S, GLFix(4) * S, l23);
    leg(GLFix(-2) * S, GLFix(-4) * S, l23);
    leg(GLFix(2) * S, GLFix(-4) * S, l14);

    glPopMatrix();

    nglResetTextureModulate();
}

void initCreeperEntities()
{
    creeper_entities.clear();
    const int COUNT = 2;
    for(int i = 0; i < COUNT; ++i)
    {
        GLFix px = GLFix((rand() % 40) - 20) * BLOCK_SIZE;
        GLFix py = GLFix(World::HEIGHT * Chunk::SIZE) * BLOCK_SIZE;
        GLFix pz = GLFix((rand() % 40) - 20) * BLOCK_SIZE;
        creeper_entities.emplace_back(px, py, pz);
    }
}

void updateCreeperEntities()
{
    for(auto it = creeper_entities.begin(); it != creeper_entities.end();)
    {
        it->update();
        if(it->health <= 0 && it->death_time > 20)
            it = creeper_entities.erase(it);
        else
            ++it;
    }
}

void renderCreeperEntities()
{
    if(creeper_entities.empty())
        return;

    glBindTexture(&creeper_tex);
    glBegin(GL_QUADS);
    for(const auto &c : creeper_entities)
        c.render();
    glEnd();
}
