#include "humanentity.h"

#include <cstdlib>
#include <vector>
#include <cmath>

#include "gl.h"
#include "world.h"
#include "terrain.h"
#include "fastmath.h"

#include "textures/steve.h"

// ─── static member definitions ───────────────────────────────────────────────
const GLFix HumanEntity::WIDTH  = GLFix(77);  // ~0.6 * BLOCK_SIZE(128)
const GLFix HumanEntity::HEIGHT = GLFix(230); // ~1.8 * BLOCK_SIZE(128)

// ─── globals ─────────────────────────────────────────────────────────────────
std::vector<HumanEntity> human_entities;

// ─── UV helpers ──────────────────────────────────────────────────────────────
// Pixel-absolute UV coordinates into the 64×64 steve skin.
static inline TextureAtlasEntry skinArea(int u, int v, int w, int h)
{
    return { (unsigned)u, (unsigned)(u + w), (unsigned)v, (unsigned)(v + h) };
}

static inline TextureAtlasEntry mirrorU(TextureAtlasEntry t)
{
    unsigned tmp = t.left; t.left = t.right; t.right = tmp;
    return t;
}

// ─── constructors ────────────────────────────────────────────────────────────
HumanEntity::HumanEntity()
    : x(0), y(GLFix(World::HEIGHT * Chunk::SIZE) * BLOCK_SIZE), z(0),
      vx(0), vy(0), vz(0),
      yaw(0), walk_timer(0), swing_intensity(0),
      health(20), hurt_time(0), hurt_resistant(0), death_time(0),
      dir_timer(60), on_ground(false)
{
    aabb = { x - WIDTH/2, y, z - WIDTH/2,
             x + WIDTH/2, y + HEIGHT, z + WIDTH/2 };
}

HumanEntity::HumanEntity(GLFix px, GLFix py, GLFix pz)
    : x(px), y(py), z(pz),
      vx(0), vy(0), vz(0),
      yaw(GLFix(rand() % 360)), walk_timer(0), swing_intensity(0),
      health(20), hurt_time(0), hurt_resistant(0), death_time(0),
      dir_timer(rand() % 60), on_ground(false)
{
    aabb = { x - WIDTH/2, y, z - WIDTH/2,
             x + WIDTH/2, y + HEIGHT, z + WIDTH/2 };
}

// ─── update ──────────────────────────────────────────────────────────────────
void HumanEntity::update()
{
    if(hurt_time > 0)
        --hurt_time;
    if(hurt_resistant > 0)
        --hurt_resistant;

    const bool dead = health <= 0;
    if(dead)
    {
        if(death_time <= 20)
            ++death_time;
        vx *= GLFix(0.92f);
        vz *= GLFix(0.92f);
    }
    else
    {
        // ── AI: random direction changes ──────────────────────────────
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

        if(!dead && on_ground && (vx != GLFix(0) || vz != GLFix(0)) && (rand() % 80 == 0))
        {
            vy = GLFix(50);
            on_ground = false;
        }
    }

    aabb = { x - WIDTH/2, y, z - WIDTH/2, x + WIDTH/2, y + HEIGHT, z + WIDTH/2 };

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
        walk_timer += horizontal_speed * GLFix(1.66f);
        walk_timer.normaliseAngle();
    }
}

// ─── rendering ───────────────────────────────────────────────────────────────
//
// Minecraft biped UV on 64×64 skin (ModelBiped / ModelPlayer source):
//
//  Part          UV offset  box dims (w×h×d in model px)
//  Head          ( 0, 0)    8×8×8
//  Body          (16,16)    8×12×4
//  Right Arm     (40,16)    4×12×4
//  Left Arm      (32,48)    4×12×4  (mirrored)
//  Right Leg     ( 0,16)    4×12×4
//  Left Leg      (16,48)    4×12×4  (mirrored)
//
// Box UV wrapping (UV offset u0,v0; box w×h×d pixels):
//   top    : (u0+d,    v0,   w,  d)
//   bottom : (u0+d+w,  v0,   w,  d)
//   right  : (u0,      v0+d, d,  h)
//   front  : (u0+d,    v0+d, w,  h)
//   left   : (u0+d+w,  v0+d, d,  h)
//   back   : (u0+d+w+d,v0+d, w,  h)

// Emit one textured quad (4 vertices, CCW winding from front)
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

// Draw all 6 faces of a biped box.
// bx/by/bz  – min local-space corner (nGL units)
// bw/bh/bd  – box dimensions (nGL units)
// u0/v0     – UV origin in pixel-space on the 64×64 skin
// wp/hp/dp  – box dimensions in texture pixels
// mirror    – flip horizontally (for left-side limbs)
static void drawBipedBox(
    GLFix bx, GLFix by, GLFix bz,
    GLFix bw, GLFix bh, GLFix bd,
    int u0, int v0, int wp, int hp, int dp,
    bool mirror = false)
{
    auto top  = skinArea(u0+dp,      v0,    wp, dp);
    auto bot  = skinArea(u0+dp+wp,   v0,    wp, dp);
    auto rgt  = skinArea(u0,         v0+dp, dp, hp);
    auto frt  = skinArea(u0+dp,      v0+dp, wp, hp);
    auto lft  = skinArea(u0+dp+wp,   v0+dp, dp, hp);
    auto bck  = skinArea(u0+dp+wp+dp,v0+dp, wp, hp);

    if (mirror)
    {
        top = mirrorU(top); bot = mirrorU(bot);
        // swap right↔left panels and flip each
        auto tmp = mirrorU(rgt); rgt = mirrorU(lft); lft = tmp;
        frt = mirrorU(frt); bck = mirrorU(bck);
    }

    GLFix x0=bx, x1=bx+bw;
    GLFix y0=by, y1=by+bh;
    GLFix z0=bz, z1=bz+bd;

    // Front  (-z face, normal towards -z)
    emitQuad(x0,y0,z0, x0,y1,z0, x1,y1,z0, x1,y0,z0, frt);
    // Back   (+z face)
    emitQuad(x1,y0,z1, x1,y1,z1, x0,y1,z1, x0,y0,z1, bck);
    // Right  (-x face)
    emitQuad(x0,y0,z1, x0,y1,z1, x0,y1,z0, x0,y0,z0, rgt);
    // Left   (+x face)
    emitQuad(x1,y0,z0, x1,y1,z0, x1,y1,z1, x1,y0,z1, lft);
    // Top
    emitQuad(x0,y1,z0, x0,y1,z1, x1,y1,z1, x1,y1,z0, top);
    // Bottom
    emitQuad(x1,y0,z0, x1,y0,z1, x0,y0,z1, x0,y0,z0, bot);
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

    // S: 1 MC model pixel = BLOCK_SIZE/16 nGL units
    const GLFix S = GLFix(BLOCK_SIZE) / GLFix(16);

    // ─────────────────────────────────────────────────────────────────────
    // MC setRotationAngles formula (ModelBiped.java lines 158-163):
    //   R_arm.rotX = cos(limbSwing * 0.6662 + PI) * arm_scale  = -cos(t)
    //   L_arm.rotX = cos(limbSwing * 0.6662)       * arm_scale  = +cos(t)
    //   R_leg.rotX = cos(limbSwing * 0.6662)       * leg_scale  = +cos(t)
    //   L_leg.rotX = cos(limbSwing * 0.6662 + PI)  * leg_scale  = -cos(t)
    //
    // walk_timer is in degrees. cos(x°) = sin(x° + 90°).
    // Max swing: arms ≈ 1 rad ≈ 57° scaled by limbSwingAmount (≈50°),
    //            legs ≈ 1.4 rad (≈70°).
    // ─────────────────────────────────────────────────────────────────────
    // walk_timer is normalized in [0, 360).
    // MC biped phases:
    //   Left Arm & Right Leg: cos(t)
    //   Right Arm & Left Leg: -cos(t)
    // ─────────────────────────────────────────────────────────────────────
    GLFix t = walk_timer;
    t.normaliseAngle(); // Extra safety
    
    const GLFix cos_t = fast_cos(t);
    const GLFix neg_cos_t = -cos_t;

    // Scale like ModelBiped: cos terms × limbSwingAmount so rest is 0, not cos(0)×45°.
    GLFix la_ang = cos_t     * GLFix(45) * swing_intensity;
    GLFix ra_ang = neg_cos_t * GLFix(45) * swing_intensity;
    GLFix rl_ang = cos_t     * GLFix(40) * swing_intensity;
    GLFix ll_ang = neg_cos_t * GLFix(40) * swing_intensity;

    la_ang.normaliseAngle(); ra_ang.normaliseAngle();
    rl_ang.normaliseAngle(); ll_ang.normaliseAngle();

    // The model's front face sits at −Z; movement direction uses
    // vz = cos(yaw), vx = sin(yaw) → yaw=0 → +Z movement.
    // Flip 180° so the front faces the direction of travel.
    GLFix render_yaw = yaw + GLFix(180);
    render_yaw.normaliseAngle();

    glPushMatrix();
    glTranslatef(x, y, z);
    nglRotateY(render_yaw);

    // RenderLivingBase.applyRotations: death tilt around Z (sqrt ease, up to 90°)
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

    // ─────────────────────────────────────────────────────────────────────
    // MC model coordinate system: Y increases DOWNWARD, Y=0 is neck level,
    // Y=24 is feet level. nGL Y increases UPWARD, Y=0 is feet.
    //
    // Pivot conversion:  ngl_pivot_y = (24 - mc_pivot_y) * S
    // addBox Y offset:   because MC Y is down, addBox(-,  0,…) means "at
    //   pivot level", addBox(-,-12,…) means "12 px UP from pivot" → +12S
    //   in nGL. addBox(-,+12,…) means 12 px DOWN → -12S in nGL.
    //   In short: ngl_by = -mc_box_y, ngl_bh = mc_box_h
    //
    // Part       MC pivot Y   addBox Y   addBox H   ngl pivot   ngl by  ngl bh
    // Head            0          -8          8        24S         0      8S  (box above pivot)
    // Body            0          +0         12        24S        -12S   12S  (box below pivot)
    // R/L Arm         2          -2         12        22S        -10S   12S  (2 above → 10 below)
    // R/L Leg        12          +0         12        12S        -12S   12S  (box below pivot)
    // ─────────────────────────────────────────────────────────────────────

    // ── Head: pivot at (0, 24S, 0), box goes 8S upward from pivot ────────
    glPushMatrix();
    glTranslatef(0, GLFix(24)*S, 0);
    drawBipedBox(GLFix(-4)*S, GLFix(0),       GLFix(-4)*S,
                 GLFix(8)*S,  GLFix(8)*S,     GLFix(8)*S,
                 0, 0, 8, 8, 8);
    glPopMatrix();

    // ── Body: pivot at (0, 24S, 0), box extends 12S downward ─────────────
    glPushMatrix();
    glTranslatef(0, GLFix(24)*S, 0);
    drawBipedBox(GLFix(-4)*S, GLFix(-12)*S,   GLFix(-2)*S,
                 GLFix(8)*S,  GLFix(12)*S,    GLFix(4)*S,
                 16, 16, 8, 12, 4);
    glPopMatrix();

    // ── Right Arm: pivot at (−5S, 22S, 0); arm extends from 2S above to
    //   10S below pivot → by=−10S, bh=12S ─────────────────────────────────
    glPushMatrix();
    glTranslatef(GLFix(-5)*S, GLFix(22)*S, 0);
    nglRotateX(ra_ang);
    drawBipedBox(GLFix(-3)*S, GLFix(-10)*S,   GLFix(-2)*S,
                 GLFix(4)*S,  GLFix(12)*S,    GLFix(4)*S,
                 40, 16, 4, 12, 4);
    glPopMatrix();

    // ── Left Arm: pivot at (+5S, 22S, 0), mirrored ───────────────────────
    glPushMatrix();
    glTranslatef(GLFix(5)*S, GLFix(22)*S, 0);
    nglRotateX(la_ang);
    drawBipedBox(GLFix(-1)*S, GLFix(-10)*S,   GLFix(-2)*S,
                 GLFix(4)*S,  GLFix(12)*S,    GLFix(4)*S,
                 32, 48, 4, 12, 4, /*mirror=*/true);
    glPopMatrix();

    // ── Right Leg: pivot at (−2S, 12S, 0); leg extends 12S downward ──────
    glPushMatrix();
    glTranslatef(GLFix(-2)*S, GLFix(12)*S, 0);
    nglRotateX(rl_ang);
    drawBipedBox(GLFix(-2)*S, GLFix(-12)*S,   GLFix(-2)*S,
                 GLFix(4)*S,  GLFix(12)*S,    GLFix(4)*S,
                 0, 16, 4, 12, 4);
    glPopMatrix();

    // ── Left Leg: pivot at (+2S, 12S, 0), mirrored ───────────────────────
    glPushMatrix();
    glTranslatef(GLFix(2)*S, GLFix(12)*S, 0);
    nglRotateX(ll_ang);
    drawBipedBox(GLFix(-2)*S, GLFix(-12)*S,   GLFix(-2)*S,
                 GLFix(4)*S,  GLFix(12)*S,    GLFix(4)*S,
                 16, 48, 4, 12, 4, /*mirror=*/true);
    glPopMatrix();

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

    // Push away along player look (WorldTask::getForward: sin(yaw), cos(yaw)). Vanilla subtracts
    // (sin, -cos) in their axes; our forward is (sin, cos), so we add here, not subtract.
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


// ─── global management ────────────────────────────────────────────────────────
void initHumanEntities()
{
    human_entities.clear();
    const int COUNT = 3;
    for (int i = 0; i < COUNT; ++i)
    {
        GLFix px = GLFix((rand() % 32) - 16) * BLOCK_SIZE;
        GLFix py = GLFix(World::HEIGHT * Chunk::SIZE) * BLOCK_SIZE;
        GLFix pz = GLFix((rand() % 32) - 16) * BLOCK_SIZE;
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
    if (human_entities.empty())
        return;

    glBindTexture(&steve_tex);
    glBegin(GL_QUADS);
    for (const auto &h : human_entities)
        h.render();
    glEnd();
}
