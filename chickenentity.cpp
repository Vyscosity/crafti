#include "chickenentity.h"

#include <cstdlib>
#include <cmath>

#include "gl.h"
#include "world.h"
#include "terrain.h"
#include "fastmath.h"
#include "grounddrops.h"

#include "textures/chicken.h"
#include "textures/items.h"

const GLFix ChickenEntity::WIDTH  = GLFix(52);
const GLFix ChickenEntity::HEIGHT = GLFix(84);

std::vector<ChickenEntity> chicken_entities;

static inline TextureAtlasEntry skinArea(int u, int v, int w, int h)
{
    return { (unsigned)u, (unsigned)(u + w), (unsigned)v, (unsigned)(v + h) };
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
    nglAddVertex({ ax, ay, az, GLFix((int)tex.left),  GLFix((int)tex.bottom), flags });
    nglAddVertex({ bx, by, bz, GLFix((int)tex.left),  GLFix((int)tex.top),    flags });
    nglAddVertex({ cx, cy, cz, GLFix((int)tex.right), GLFix((int)tex.top),    flags });
    nglAddVertex({ dx, dy, dz, GLFix((int)tex.right), GLFix((int)tex.bottom), flags });
}

/** Same UV layout as HumanEntity::drawBipedBox; atlas size may be 64×32 (v stays valid). */
static void drawChickBox(
    GLFix bx, GLFix by, GLFix bz,
    GLFix bw, GLFix bh, GLFix bd,
    int u0, int v0, int wp, int hp, int dp,
    bool mirror = false)
{
    auto top  = skinArea(u0 + dp,      v0,    wp, dp);
    auto bot  = skinArea(u0 + dp + wp, v0,    wp, dp);
    auto rgt  = skinArea(u0,           v0 + dp, dp, hp);
    auto frt  = skinArea(u0 + dp,      v0 + dp, wp, hp);
    auto lft  = skinArea(u0 + dp + wp, v0 + dp, dp, hp);
    auto bck  = skinArea(u0 + dp + wp + dp, v0 + dp, wp, hp);

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

ChickenEntity::ChickenEntity()
    : x(0), y(GLFix(World::HEIGHT * Chunk::SIZE) * BLOCK_SIZE), z(0),
      vx(0), vy(0), vz(0),
      yaw(0), walk_timer(0), swing_intensity(0),
      health(4), hurt_time(0), hurt_resistant(0), death_time(0),
      ticks_alive(0), dir_timer(50), on_ground(false), loot_spawned(false)
{
    aabb = { x - WIDTH / 2, y, z - WIDTH / 2, x + WIDTH / 2, y + HEIGHT, z + WIDTH / 2 };
}

ChickenEntity::ChickenEntity(GLFix px, GLFix py, GLFix pz)
    : x(px), y(py), z(pz),
      vx(0), vy(0), vz(0),
      yaw(GLFix(rand() % 360)), walk_timer(0), swing_intensity(0),
      health(4), hurt_time(0), hurt_resistant(0), death_time(0),
      ticks_alive(0), dir_timer(rand() % 60), on_ground(false), loot_spawned(false)
{
    aabb = { x - WIDTH / 2, y, z - WIDTH / 2, x + WIDTH / 2, y + HEIGHT, z + WIDTH / 2 };
}

void ChickenEntity::update()
{
    if(hurt_time > 0)
        --hurt_time;
    if(hurt_resistant > 0)
        --hurt_resistant;

    const bool dead = health <= 0;
    if(dead && !loot_spawned)
    {
        loot_spawned = true;
        spawnWorldDrop(x, y, z,
                       getBLOCKWDATA(BLOCK_ITEM, static_cast<uint8_t>(ItemTexture::RAW_CHICKEN)), 1u);
    }
    if(dead)
    {
        if(death_time <= 20)
            ++death_time;
        vx *= GLFix(0.92f);
        vz *= GLFix(0.92f);
    }
    else if(--dir_timer <= 0)
    {
        int r = rand() % 8;
        const GLFix speed(2);
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
        dir_timer = 35 + rand() % 70;
    }

    GLFix old_x = x;
    GLFix old_z = z;
    ++ticks_alive;

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

        if(!dead && on_ground && (vx != GLFix(0) || vz != GLFix(0)) && (rand() % 100 == 0))
        {
            vy = GLFix(40);
            on_ground = false;
        }
    }

    aabb = { x - WIDTH / 2, y, z - WIDTH / 2, x + WIDTH / 2, y + HEIGHT, z + WIDTH / 2 };

    if(dead)
    {
        swing_intensity *= GLFix(0.85f);
        return;
    }

    GLFix dx = x - old_x;
    GLFix dz = z - old_z;
    GLFix actual_dist = GLFix(std::sqrt((float)(dx * dx + dz * dz)));
    GLFix target_amp = actual_dist * GLFix(0.33f);
    if(target_amp > GLFix(1))
        target_amp = GLFix(1);
    swing_intensity += (target_amp - swing_intensity) * GLFix(0.4f);

    GLFix horizontal_speed = GLFix(std::sqrt((float)(vx * vx + vz * vz)));
    if(horizontal_speed > GLFix(0))
    {
        walk_timer += horizontal_speed * GLFix(2.2f);
        walk_timer.normaliseAngle();
    }
}

void ChickenEntity::render() const
{
    if(health <= 0 && death_time > 20)
        return;

    if(hurt_time > 0)
    {
        GLFix t = GLFix(hurt_time) / GLFix(10);
        nglSetTextureModulate(GLFix(1), GLFix(1) - t * GLFix(0.52f), GLFix(1) - t * GLFix(0.48f));
    }

    const GLFix S = GLFix(BLOCK_SIZE) / GLFix(16);
    GLFix t = walk_timer;
    t.normaliseAngle();
    const GLFix cos_t = fast_cos(t);
    const GLFix neg_cos_t = -cos_t;
    GLFix rl = cos_t * GLFix(55) * swing_intensity;
    GLFix ll = neg_cos_t * GLFix(55) * swing_intensity;
    // nglRotate* → fast_sin/fast_cos use non-negative table indices; same as HumanEntity / Creeper legs.
    rl.normaliseAngle();
    ll.normaliseAngle();

    GLFix wing_z = fast_sin(GLFix((int)((ticks_alive * 11) % 360))) * GLFix(32);
    GLFix wing_z_neg = -wing_z;
    wing_z.normaliseAngle();
    wing_z_neg.normaliseAngle();

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

    auto pivotY = [&](GLFix mc_py) -> GLFix {
        return (GLFix(24) - mc_py) * S;
    };

    // Head + bill + chin (shared pivot ModelChicken 0,15,-4).
    // MC addBox uses Y+ down; nGL Y+ up → min corner y = -(mc_y + mc_h) * S for boxes
    // extending from mc_y upward (negative mc_y), same mapping as HumanEntity comments.
    glPushMatrix();
    glTranslatef(0, pivotY(GLFix(15)), GLFix(-4) * S);
    drawChickBox(GLFix(-2) * S, GLFix(0), GLFix(-2) * S,
                 GLFix(4) * S, GLFix(6) * S, GLFix(3) * S,
                 0, 0, 4, 6, 3);
    drawChickBox(GLFix(-2) * S, GLFix(2) * S, GLFix(-4) * S,
                 GLFix(4) * S, GLFix(2) * S, GLFix(2) * S,
                 14, 0, 4, 2, 2);
    drawChickBox(GLFix(-1) * S, GLFix(0), GLFix(-3) * S,
                 GLFix(2) * S, GLFix(2) * S, GLFix(2) * S,
                 14, 4, 2, 2, 2);
    glPopMatrix();

    // Body (horizontal: rotate X 90° like ModelChicken.body.rotateAngleX = π/2)
    glPushMatrix();
    glTranslatef(0, pivotY(GLFix(16)), 0);
    nglRotateX(GLFix(90));
    drawChickBox(GLFix(-3) * S, GLFix(-4) * S, GLFix(-3) * S,
                 GLFix(6) * S, GLFix(8) * S, GLFix(6) * S,
                 0, 9, 6, 8, 6);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(GLFix(-2) * S, pivotY(GLFix(19)), GLFix(1) * S);
    nglRotateX(rl);
    drawChickBox(GLFix(-1) * S, GLFix(-5) * S, GLFix(-3) * S,
                 GLFix(3) * S, GLFix(5) * S, GLFix(3) * S,
                 26, 0, 3, 5, 3);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(GLFix(1) * S, pivotY(GLFix(19)), GLFix(1) * S);
    nglRotateX(ll);
    drawChickBox(GLFix(-1) * S, GLFix(-5) * S, GLFix(-3) * S,
                 GLFix(3) * S, GLFix(5) * S, GLFix(3) * S,
                 26, 0, 3, 5, 3, true);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(GLFix(-4) * S, pivotY(GLFix(13)), 0);
    nglRotateZ(wing_z);
    drawChickBox(0, GLFix(-4) * S, GLFix(-3) * S,
                 GLFix(1) * S, GLFix(4) * S, GLFix(6) * S,
                 24, 13, 1, 4, 6);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(GLFix(4) * S, pivotY(GLFix(13)), 0);
    nglRotateZ(wing_z_neg);
    drawChickBox(GLFix(-1) * S, GLFix(-4) * S, GLFix(-3) * S,
                 GLFix(1) * S, GLFix(4) * S, GLFix(6) * S,
                 24, 13, 1, 4, 6, true);
    glPopMatrix();

    glPopMatrix();

    nglResetTextureModulate();
}

void ChickenEntity::applyMeleeDamage(int amount, GLFix attacker_yaw)
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

void initChickenEntities()
{
    chicken_entities.clear();
    constexpr int COUNT = 5;
    for(int i = 0; i < COUNT; ++i)
    {
        GLFix px = GLFix((rand() % 40) - 20) * BLOCK_SIZE + GLFix(rand() % 64);
        GLFix py = GLFix(World::HEIGHT * Chunk::SIZE) * BLOCK_SIZE;
        GLFix pz = GLFix((rand() % 40) - 15) * BLOCK_SIZE + GLFix(rand() % 64);
        chicken_entities.emplace_back(px, py, pz);
    }
}

void updateChickenEntities()
{
    for(auto it = chicken_entities.begin(); it != chicken_entities.end();)
    {
        it->update();
        if(it->health <= 0 && it->death_time > 20)
            it = chicken_entities.erase(it);
        else
            ++it;
    }
}

void renderChickenEntities()
{
    if(chicken_entities.empty())
        return;

    glBindTexture(&chicken_tex);
    glBegin(GL_QUADS);
    for(const auto &c : chicken_entities)
        c.render();
    glEnd();
}
