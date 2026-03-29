#include <cassert>
#include <cstdlib>
#include <algorithm>
#include <libndls.h>

#include "world.h"
#include "chunk.h"
#include "fastmath.h"
#include "blockrenderer.h"
#include "oregeneration.h"

//Texture with "Loading" written on it
#include "textures/loadingtext.h"

#ifndef NDEBUG
    #define debug(...) printf(__VA_ARGS__)
#else
    #define debug(...)
#endif

constexpr const int Chunk::SIZE;
int Chunk::pos_indices[SIZE + 1][SIZE + 1][SIZE + 1];

Chunk::Chunk(int x, int y, int z)
    : x(x), y(y), z(z), abs_x(x*SIZE*BLOCK_SIZE), abs_y(y*SIZE*BLOCK_SIZE), abs_z(z*SIZE*BLOCK_SIZE), aabb(abs_x, abs_y, abs_z, abs_x + SIZE*BLOCK_SIZE, abs_y + SIZE*BLOCK_SIZE, abs_z + SIZE*BLOCK_SIZE)
{}

static constexpr bool inBounds(int x, int y, int z)
{
    return x >= 0 && y >= 0 && z >= 0 && x < Chunk::SIZE && y < Chunk::SIZE && z < Chunk::SIZE;
}

unsigned int Chunk::getPosition(unsigned int x, unsigned int y, unsigned int z)
{
    assert (x <= Chunk::SIZE && y <= Chunk::SIZE && z <= Chunk::SIZE);

    if(pos_indices[x][y][z] == -1)
    {
        pos_indices[x][y][z] = build_positions.size();
        build_positions.emplace_back(VECTOR3{x*BLOCK_SIZE, y*BLOCK_SIZE, z*BLOCK_SIZE});
    }

    return pos_indices[x][y][z];
}

void Chunk::addAlignedVertex(const int x, const int y, const int z, GLFix u, GLFix v, const COLOR c)
{
    build_vertices.emplace_back(IndexedVertex{getPosition(x, y, z), u, v, c});
}

void Chunk::addUnalignedVertex(const GLFix x, const GLFix y, const GLFix z, const GLFix u, const GLFix v, const COLOR c)
{
    build_vertices_unaligned.emplace_back(VERTEX{x, y, z, u, v, c});
}

void Chunk::addUnalignedVertex(const VERTEX &v)
{
    build_vertices_unaligned.push_back(v);
}

void Chunk::addAnimation(const Chunk::Animation &animation)
{
    animations.push_back(animation);
}

void Chunk::addParticle(const Particle &particle)
{
    particles.push_back(particle);
}

void Chunk::addAlignedVertexQuad(const int x, const int y, const int z, GLFix u, GLFix v, const COLOR c)
{
    build_vertices_quad.emplace_back(IndexedVertex{getPosition(x, y, z), u, v, c});
}

void Chunk::addAlignedVertexForceColor(const int x, const int y, const int z, GLFix u, GLFix v, const COLOR c)
{
    build_vertices_color.emplace_back(IndexedVertex{getPosition(x, y, z), u, v, c});
}

void Chunk::setLocalBlockSideRendered(const int x, const int y, const int z, const BLOCK_SIDE_BITFIELD side)
{
    sides_rendered[x][y][z] |= side;
}

bool Chunk::isLocalBlockSideRendered(const int x, const int y, const int z, const BLOCK_SIDE_BITFIELD side)
{
    return sides_rendered[x][y][z] & side;
}

void Chunk::buildGeometry()
{
    drawLoadingtext(8);

    std::fill(pos_indices[0][0] + 0, pos_indices[SIZE][SIZE] + SIZE + 1, -1);

    build_positions.clear();
    build_vertices.clear();
    build_vertices_quad.clear();
    build_vertices_color.clear();
    build_vertices_unaligned.clear();
    animations.clear();

    //Bottom of world doesn't need to be drawn
    int y_start = this->y == 0 ? 0 : -1;

    //Now go through map and search for transparent blocks and draw only the sides adjacent to them
    for(int x = -1; x <= SIZE; x++)
    {
        for(int y = y_start; y <= SIZE; y++)
        {
            for(int z = -1; z <= SIZE; z++)
            {
                BLOCK_WDATA block = getGlobalBlockRelative(x, y, z), block1;

                if(block != BLOCK_AIR && global_block_renderer.isOpaque(block))
                    continue;

                if(inBounds(x - 1, y, z) && (block1 = blocks[x - 1][y][z]) != BLOCK_AIR && !(sides_rendered[x - 1][y][z] & BLOCK_RIGHT_BIT))
                    global_block_renderer.geometryNormalBlock(block1, x - 1, y, z, BLOCK_RIGHT, *this);

                if(inBounds(x + 1, y, z) && (block1 = blocks[x + 1][y][z]) != BLOCK_AIR && !(sides_rendered[x + 1][y][z] & BLOCK_LEFT_BIT))
                    global_block_renderer.geometryNormalBlock(block1, x + 1, y, z, BLOCK_LEFT, *this);

                if(inBounds(x, y - 1, z) && (block1 = blocks[x][y - 1][z]) != BLOCK_AIR && !(sides_rendered[x][y - 1][z] & BLOCK_TOP_BIT))
                    global_block_renderer.geometryNormalBlock(block1, x, y - 1, z, BLOCK_TOP, *this);

                if(inBounds(x, y + 1, z) && (block1 = blocks[x][y + 1][z]) != BLOCK_AIR && !(sides_rendered[x][y + 1][z] & BLOCK_BOTTOM_BIT))
                    global_block_renderer.geometryNormalBlock(block1, x, y + 1, z, BLOCK_BOTTOM, *this);

                if(inBounds(x, y, z - 1) && (block1 = blocks[x][y][z - 1]) != BLOCK_AIR && !(sides_rendered[x][y][z - 1] & BLOCK_BACK_BIT))
                    global_block_renderer.geometryNormalBlock(block1, x, y, z - 1, BLOCK_BACK, *this);

                if(inBounds(x, y, z + 1) && (block1 = blocks[x][y][z + 1]) != BLOCK_AIR && !(sides_rendered[x][y][z + 1] & BLOCK_FRONT_BIT))
                    global_block_renderer.geometryNormalBlock(block1, x, y, z + 1, BLOCK_FRONT, *this);
            }
        }
    }

    //Special blocks
    GLFix pos_x = 0;
    for(int x = 0; x < SIZE; x++, pos_x += BLOCK_SIZE)
    {
        GLFix pos_y = 0;
        for(int y = 0; y < SIZE; y++, pos_y += BLOCK_SIZE)
        {
            GLFix pos_z = 0;
            for(int z = 0; z < SIZE; z++, pos_z += BLOCK_SIZE)
            {
                BLOCK_WDATA block = blocks[x][y][z];

                if(getBLOCK(block) == BLOCK_AIR)
                    continue;

                if(global_block_renderer.isOpaque(getGlobalBlockRelative(x - 1, y, z))
                    && global_block_renderer.isOpaque(getGlobalBlockRelative(x + 1, y, z))
                    && global_block_renderer.isOpaque(getGlobalBlockRelative(x, y - 1, z))
                    && global_block_renderer.isOpaque(getGlobalBlockRelative(x, y + 1, z))
                    && global_block_renderer.isOpaque(getGlobalBlockRelative(x, y, z - 1))
                    && global_block_renderer.isOpaque(getGlobalBlockRelative(x, y, z + 1)))
                    continue;

                global_block_renderer.renderSpecialBlock(block, pos_x, pos_y, pos_z, *this);
            }
        }
    }

    std::fill(sides_rendered[0][0] + 0, sides_rendered[SIZE - 1][SIZE - 1] + SIZE, 0);

    build_positions_processed.resize(build_positions.size());

    build_complete = true;
    build_dirty = false;

    debug("Done!\n");
}

void Chunk::swapMeshes()
{
    if(!build_complete)
        return;

    positions.swap(build_positions);
    positions_processed.swap(build_positions_processed);
    vertices.swap(build_vertices);
    vertices_quad.swap(build_vertices_quad);
    vertices_color.swap(build_vertices_color);
    vertices_unaligned.swap(build_vertices_unaligned);

    build_complete = false;
}

void Chunk::buildGeometryAsync()
{
    buildGeometry();
}

static bool behindClip(const VECTOR3 &v1)
{
    return (transformation->data[2][0]*v1.x + transformation->data[2][1]*v1.y + transformation->data[2][2]*v1.z + transformation->data[2][3]) <= GLFix(CLIP_PLANE);
}

void Chunk::logic(bool ticks_enabled)
{
    if(ticks_enabled)
    {
        tick_counter -= 1;
        if(tick_counter == 0)
        {
            tick_counter = 10; //Do a tick every 10th frame

            for(int x = 0; x < SIZE; x++)
                for(int y = 0; y < SIZE; y++)
                    for(int z = 0; z < SIZE; z++)
                    {
                        BLOCK_WDATA block = blocks[x][y][z];
                        if(block != BLOCK_AIR)
                            global_block_renderer.tick(block, x, y, z, *this);
                    }
        }
    }

    for(int i = particles.size() - 1; i >= 0; --i)
    {
        bool remove = false;
        particles[i].logic(&remove);
        if(remove)
            particles.erase(particles.begin() + i);
    }
}

void Chunk::render()
{
    // Swap if build is complete
    if(__builtin_expect(build_complete, 0))
        swapMeshes();

    //If there's nothing to render, skip it completely
    if(positions.size() == 0 && vertices_unaligned.size() == 0
            && animations.size() == 0 && particles.size() == 0)
        return;

    glPushMatrix();
    glTranslatef(abs_x, abs_y, abs_z);

    //Basic culling
    VECTOR3 v1{0,                            0,                            0},
            v2{0 + Chunk::SIZE * BLOCK_SIZE, 0,                            0},
            v3{0,                            0 + Chunk::SIZE * BLOCK_SIZE, 0},
            v4{0 + Chunk::SIZE * BLOCK_SIZE, 0 + Chunk::SIZE * BLOCK_SIZE, 0},
            v5{0,                            0,                            0 + Chunk::SIZE * BLOCK_SIZE},
            v6{0 + Chunk::SIZE * BLOCK_SIZE, 0,                            0 + Chunk::SIZE * BLOCK_SIZE},
            v7{0,                            0 + Chunk::SIZE * BLOCK_SIZE, 0 + Chunk::SIZE * BLOCK_SIZE},
            v8{0 + Chunk::SIZE * BLOCK_SIZE, 0 + Chunk::SIZE * BLOCK_SIZE, 0 + Chunk::SIZE * BLOCK_SIZE};

    //Z-Culling (now, it's a bit cheaper than a full MultMatVectRes)
    if(behindClip(v1) && behindClip(v2) && behindClip(v3) && behindClip(v4) && behindClip(v5) && behindClip(v6) && behindClip(v7) && behindClip(v8))
        return glPopMatrix();

    VECTOR3 v9, v10, v11, v12, v13, v14, v15, v16;

    nglMultMatVectRes(transformation, &v1, &v9);
    nglMultMatVectRes(transformation, &v2, &v10);
    nglMultMatVectRes(transformation, &v3, &v11);
    nglMultMatVectRes(transformation, &v4, &v12);
    nglMultMatVectRes(transformation, &v5, &v13);
    nglMultMatVectRes(transformation, &v6, &v14);
    nglMultMatVectRes(transformation, &v7, &v15);
    nglMultMatVectRes(transformation, &v8, &v16);

    nglPerspective(&v9);
    nglPerspective(&v10);
    nglPerspective(&v11);
    nglPerspective(&v12);
    nglPerspective(&v13);
    nglPerspective(&v14);
    nglPerspective(&v15);
    nglPerspective(&v16);

    //X and Y-Culling

    if(v9.x < GLFix(0) && v10.x < GLFix(0) && v11.x < GLFix(0) && v12.x < GLFix(0) && v13.x < GLFix(0) && v14.x < GLFix(0) && v15.x < GLFix(0) && v16.x < GLFix(0))
        return glPopMatrix();

    if(v9.y < GLFix(0) && v10.y < GLFix(0) && v11.y < GLFix(0) && v12.y < GLFix(0) && v13.y < GLFix(0) && v14.y < GLFix(0) && v15.y < GLFix(0) && v16.y < GLFix(0))
        return glPopMatrix();

    if(v9.x >= SCREEN_WIDTH && v10.x >= SCREEN_WIDTH && v11.x >= SCREEN_WIDTH && v12.x >= SCREEN_WIDTH
            && v13.x >= SCREEN_WIDTH && v14.x >= SCREEN_WIDTH && v15.x >= SCREEN_WIDTH && v16.x >= SCREEN_WIDTH)
        return glPopMatrix();

    if(v9.y >= SCREEN_HEIGHT && v10.y >= SCREEN_HEIGHT && v11.y >= SCREEN_HEIGHT && v12.y >= SCREEN_HEIGHT
            && v13.y >= SCREEN_HEIGHT && v14.y >= SCREEN_HEIGHT && v15.y >= SCREEN_HEIGHT && v16.y >= SCREEN_HEIGHT)
        return glPopMatrix();

    glBindTexture(nullptr);
    nglDrawArray(vertices_color.data(), vertices_color.size(), positions.data(), positions.size(), positions_processed.data(), GL_QUADS, true);

    glBindTexture(terrain_quad);
    nglDrawArray(vertices_quad.data(), vertices_quad.size(), positions.data(), positions.size(), positions_processed.data(), GL_QUADS, false);

    glBindTexture(terrain_current);
    nglDrawArray(vertices.data(), vertices.size(), positions.data(), positions.size(), positions_processed.data(), GL_QUADS, false);

    const VERTEX *ve = vertices_unaligned.data();
    for(unsigned int i = 0; i < vertices_unaligned.size(); i += 4, ve += 4)
    {
        VERTEX v1, v2, v3, v4;

        nglMultMatVectRes(transformation, &ve[0], &v1);
        nglMultMatVectRes(transformation, &ve[1], &v2);
        nglMultMatVectRes(transformation, &ve[2], &v3);

        //nglMultMatVectRes doesn't copy u,v and c
        v1.u = ve[0].u;
        v1.v = ve[0].v;
        v1.c = ve[0].c;
        v2.u = ve[1].u;
        v2.v = ve[1].v;
        v2.c = ve[1].c;
        v3.u = ve[2].u;
        v3.v = ve[2].v;
        v3.c = ve[2].c;

        if(nglDrawTriangle(&v1, &v2, &v3, (v1.c & TEXTURE_DRAW_BACKFACE) == 0) || (v1.c & INDEPENDENT_TRIS))
        {
            nglMultMatVectRes(transformation, &ve[3], &v4);
            v4.u = ve[3].u;
            v4.v = ve[3].v;
            v4.c = ve[3].c;

            nglDrawTriangle(&v3, &v4, &v1, v1.c & INDEPENDENT_TRIS);
        }
    }

    for(auto &animation : animations)
        animation.animate(animation.x, animation.y, animation.z, *this);

    for(auto &particle : particles)
        particle.render();

    return glPopMatrix();
}

BLOCK_WDATA Chunk::getLocalBlock(const int x, const int y, const int z) const
{
    assert(inBounds(x, y, z));
    return blocks[x][y][z];
}

void Chunk::setLocalBlock(const int x, const int y, const int z, const BLOCK_WDATA block, bool set_dirty)
{
    assert(inBounds(x, y, z));
    blocks[x][y][z] = block;
    if(!set_dirty)
        return;

    setDirty();

    if(x == 0)
        if(Chunk *c = world.findChunk(this->x - 1, this->y, this->z))
            c->setDirty();

    if(x == Chunk::SIZE - 1)
        if(Chunk *c = world.findChunk(this->x + 1, this->y, this->z))
            c->setDirty();

    if(y == 0)
        if(Chunk *c = world.findChunk(this->x, this->y - 1, this->z))
            c->setDirty();

    if(y == Chunk::SIZE - 1)
        if(Chunk *c = world.findChunk(this->x, this->y + 1, this->z))
            c->setDirty();

    if(z == 0)
        if(Chunk *c = world.findChunk(this->x, this->y, this->z - 1))
            c->setDirty();

    if(z == Chunk::SIZE - 1)
        if(Chunk *c = world.findChunk(this->x, this->y, this->z + 1))
            c->setDirty();
}

void Chunk::changeLocalBlock(const int x, const int y, const int z, const BLOCK_WDATA block)
{
    BLOCK_WDATA current_block = blocks[x][y][z];
    setLocalBlock(x, y, z, block);
    global_block_renderer.removedBlock(current_block, x, y, z, *this);
    global_block_renderer.addedBlock(block, x, y, z, *this);
}

void Chunk::changeGlobalBlockRelative(const int x, const int y, const int z, const BLOCK_WDATA block)
{
    if(inBounds(x, y, z))
        return changeLocalBlock(x, y, z, block);

    world.changeBlock(x + this->x*SIZE, y + this->y*SIZE, z + this->z*SIZE, block);
}

BLOCK_WDATA Chunk::getGlobalBlockRelative(const int x, const int y, const int z) const
{
    if(inBounds(x, y, z))
        return getLocalBlock(x, y, z);

    return world.getBlock(x + this->x*SIZE, y + this->y*SIZE, z + this->z*SIZE);
}

void Chunk::setGlobalBlockRelative(const int x, const int y, const int z, const BLOCK_WDATA block, bool set_dirty)
{
    if(inBounds(x, y, z))
        return setLocalBlock(x, y, z, block, set_dirty);

    return world.setBlock(x + this->x*SIZE, y + this->y*SIZE, z + this->z*SIZE, block, set_dirty);
}

//Ignores any non-obstacle blocks
bool Chunk::intersects(AABB &other)
{
    if(!aabb.intersects(other))
        return false;

    AABB aabb;
    aabb.low_x = abs_x;

    for(unsigned int x = 0; x < SIZE; x++, aabb.low_x += BLOCK_SIZE)
    {
        aabb.high_x = aabb.low_x + BLOCK_SIZE;

        aabb.low_y = abs_y;

        for(unsigned int y = 0; y < SIZE; y++, aabb.low_y += BLOCK_SIZE)
        {
            aabb.high_y = aabb.low_y + BLOCK_SIZE;

            aabb.low_z = abs_z;

            for(unsigned int z = 0; z < SIZE; z++, aabb.low_z += BLOCK_SIZE)
            {
                aabb.high_z = aabb.low_z + BLOCK_SIZE;

                const BLOCK_WDATA block = blocks[x][y][z];

                if(!global_block_renderer.isObstacle(block))
                    continue;

                if(global_block_renderer.isBlockShaped(block))
                {
                    if(aabb.intersects(other))
                        return true;
                }
                else if(global_block_renderer.getAABB(block, aabb.low_x, aabb.low_y, aabb.low_z).intersects(other))
                    return true;
            }
        }
    }

    return false;
}

bool Chunk::intersectsRay(GLFix rx, GLFix ry, GLFix rz, GLFix dx, GLFix dy, GLFix dz, GLFix &dist, VECTOR3 &pos, AABB::SIDE &side, bool ignore_water)
{
    GLFix shortest_dist;
    if(aabb.intersectsRay(rx, ry, rz, dx, dy, dz, shortest_dist) == AABB::NONE)
        return false;

    shortest_dist = GLFix::maxValue();

    AABB aabb;
    aabb.low_x = abs_x;

    for(unsigned int x = 0; x < SIZE; x++, aabb.low_x += BLOCK_SIZE)
    {
        aabb.high_x = aabb.low_x + BLOCK_SIZE;

        aabb.low_y = abs_y;

        for(unsigned int y = 0; y < SIZE; y++, aabb.low_y += BLOCK_SIZE)
        {
            aabb.high_y = aabb.low_y + BLOCK_SIZE;

            aabb.low_z = abs_z;

            for(unsigned int z = 0; z < SIZE; z++, aabb.low_z += BLOCK_SIZE)
            {
                aabb.high_z = aabb.low_z + BLOCK_SIZE;

                BLOCK_WDATA block = blocks[x][y][z];

                if(block == BLOCK_AIR || (getBLOCK(block) == BLOCK_WATER && ignore_water))
                    continue;

                AABB test = aabb;
                if(!global_block_renderer.isBlockShaped(block))
                    test = global_block_renderer.getAABB(block, aabb.low_x, aabb.low_y, aabb.low_z);

                GLFix new_dist;
                AABB::SIDE new_side = test.intersectsRay(rx, ry, rz, dx, dy, dz, new_dist);
                if(new_side != AABB::NONE)
                {
                    if(new_dist < shortest_dist)
                    {
                        pos.x = x;
                        pos.y = y;
                        pos.z = z;
                        side = new_side;
                        shortest_dist = new_dist;
                    }
                }
            }
        }
    }

    if(shortest_dist == GLFix::maxValue())
        return false;

    dist = shortest_dist;
    return true;
}

void Chunk::generate()
{
    //Everything air
    std::fill(blocks[0][0] + 0, blocks[SIZE - 1][SIZE - 1] + SIZE, BLOCK_AIR);

    debug("Generating chunk %d:%d:%d...\t", x, y, z);

    if(world.worldType() == World::WorldType::Flat)
    {
        if(this->y == 0)
        {
            for(int x = 0; x < SIZE; ++x)
                for(int z = 0; z < SIZE; ++z)
                {
                    blocks[x][0][z] = BLOCK_BEDROCK;
                    if(SIZE > 1)
                        blocks[x][1][z] = BLOCK_DIRT;
                    if(SIZE > 2)
                        blocks[x][2][z] = BLOCK_GRASS;
                }
        }

        // Leave everything else as air and skip terrain generation.
        debug("Done!\n");
        return;
    }

    const PerlinNoise &noise = world.noiseGenerator();

    constexpr int max_trees = (Chunk::SIZE * Chunk::SIZE) / 45;
    constexpr int sea_level = 12;
    int trees = 0;

    for(int x = 0; x < SIZE; x++)
        for(int z = 0; z < SIZE; z++)
        {
            GLFix nx = (GLFix(x)/Chunk::SIZE + this->x)/4;
            GLFix nz = (GLFix(z)/Chunk::SIZE + this->z)/4;

            GLFix e1 = noise.noise(nx, nz, 10);
            GLFix e2 = noise.noise(nx * 2, nz * 2, 20) * GLFix(0.65f);
            GLFix e3 = noise.noise(nx * 4, nz * 4, 30) * GLFix(0.35f);
            GLFix e4 = noise.noise(nx * 8, nz * 8, 40) * GLFix(0.20f);

            GLFix noise_val = (e1 + e2 + e3 + e4) / GLFix(2.20f);
            // Push valleys down and peaks up for less flat terrain.
            noise_val = (noise_val + noise_val * noise_val) / 2;

            int world_gen_min = 4, world_gen_max = World::HEIGHT * Chunk::SIZE - 3;
            int height = world_gen_min + (noise_val * (world_gen_max - world_gen_min)).round();
            int height_left = height - this->y * Chunk::SIZE;
            int height_here = std::min(height_left, Chunk::SIZE);

            int y = 0;

            //Bottom layer of lowest chunk is bedrock
            if(this->y == 0)
                blocks[x][y++][z] = BLOCK_BEDROCK;

            for(; y < height_here; y++)
            {
                int to_surface = height_left - y;

                //Deep underground
                if(to_surface > 5)
                {
                    blocks[x][y][z] = BLOCK_STONE;
                }
                else if(height > sea_level + 1)
                {
                    if(to_surface == 1)
                    {
                        blocks[x][y][z] = BLOCK_GRASS;
                        if((rand() & 0xFF) == 0x0)
                            setGlobalBlockRelative(x, y + 1, z, getBLOCKWDATA(BLOCK_FLOWER, rand() & 0x1));
                    }
                    else
                        blocks[x][y][z] = BLOCK_DIRT;
                }
                else
                    blocks[x][y][z] = BLOCK_SAND;
            }

            const int local_sea_level = sea_level - this->y * Chunk::SIZE;
            int water_start = std::max(height_left, 0);
            // Keep water one block below the prior sea-line so beaches stay sand-first.
            int water_end = std::min(local_sea_level, Chunk::SIZE);
            for(int y = water_start; y < water_end; ++y)
            {
                if(blocks[x][y][z] == BLOCK_AIR)
                    blocks[x][y][z] = getBLOCKWDATA(BLOCK_WATER, RANGE_WATER);
            }

            if(trees < max_trees
                && height > sea_level + 1
                && height_left > 0
                && height_left <= Chunk::SIZE
                && blocks[x][height_left - 1][z] == BLOCK_GRASS
                && noise.noise(GLFix(x)/Chunk::SIZE + this->x, GLFix(z)/Chunk::SIZE + this->z, 25) < GLFix(0.3f))
            {
                makeTree(x, height_here, z);
                trees++;
            }
        }

    // Generate ore veins using Minecraft-like distribution
    generateOreVeins();

    debug("Done!\n");
}

bool Chunk::saveToFile(gzFile file)
{
    if(gzfwrite(blocks, sizeof(***blocks), SIZE*SIZE*SIZE, file) == SIZE*SIZE*SIZE)
    {
        debug("Saved chunk %d:%d:%d successfully.\n", x, y, z);
        return true;
    }
    else
    {
        debug("Saving chunk %d:%d:%d failed!\n", x, y, z);
        return false;
    }
}

bool Chunk::loadFromFile(gzFile file)
{
    if(gzfread(blocks, sizeof(***blocks), SIZE*SIZE*SIZE, file) == SIZE*SIZE*SIZE)
    {
        debug("Loaded chunk %d:%d:%d successfully.\n", x, y, z);
        return true;
    }
    else
    {
        debug("Loading chunk %d:%d:%d failed!\n", x, y, z);
        return false;
    }
}

bool Chunk::gettingPowerFrom(const int x, const int y, const int z, BLOCK_SIDE side, bool ignore_redstone_wire)
{
    auto gettingStrongPowerFrom = [this, ignore_redstone_wire](const int x, const int y, const int z, BLOCK_SIDE side) {
        BLOCK_WDATA block = getGlobalBlockRelative(x, y, z);
        if(getBLOCK(block) == BLOCK_AIR || (ignore_redstone_wire && getBLOCK(block) == BLOCK_REDSTONE_WIRE))
            return false;

        return global_block_renderer.powersSide(block, side) == PowerState::StronglyPowered;
    };

    BLOCK_WDATA block = getGlobalBlockRelative(x, y, z);
    if(getBLOCK(block) == BLOCK_AIR || (ignore_redstone_wire && getBLOCK(block) == BLOCK_REDSTONE_WIRE))
        return false;

    if(global_block_renderer.powersSide(block, side) != PowerState::NotPowered)
        return true;

    if(!global_block_renderer.isOpaque(block))
        return false;

    return gettingStrongPowerFrom(x-1, y, z, BLOCK_RIGHT)
        || gettingStrongPowerFrom(x+1, y, z, BLOCK_LEFT)
        || gettingStrongPowerFrom(x, y-1, z, BLOCK_TOP)
        || gettingStrongPowerFrom(x, y+1, z, BLOCK_BOTTOM)
        || gettingStrongPowerFrom(x, y, z-1, BLOCK_BACK)
        || gettingStrongPowerFrom(x, y, z+1, BLOCK_FRONT);
}

bool Chunk::isBlockPowered(const int x, const int y, const int z, bool ignore_redstone_wire)
{
    return gettingPowerFrom(x-1, y, z, BLOCK_RIGHT, ignore_redstone_wire)
        || gettingPowerFrom(x+1, y, z, BLOCK_LEFT, ignore_redstone_wire)
        || gettingPowerFrom(x, y-1, z, BLOCK_TOP, ignore_redstone_wire)
        || gettingPowerFrom(x, y+1, z, BLOCK_BOTTOM, ignore_redstone_wire)
        || gettingPowerFrom(x, y, z-1, BLOCK_BACK, ignore_redstone_wire)
        || gettingPowerFrom(x, y, z+1, BLOCK_FRONT, ignore_redstone_wire);
}

void Chunk::generateOreVeins()
{
    // Generate ore veins after terrain filling by sampling world Y with distribution weight.
    for(int ore_idx = 0; ore_idx < OreDistributions::ore_count; ++ore_idx)
    {
        const OreDistribution &ore_dist = OreDistributions::ore_list[ore_idx];

        for(int vein_attempt = 0; vein_attempt < ore_dist.veins_per_chunk; ++vein_attempt)
        {
            unsigned int seed = (x * 73856093u) ^ (y * 19349663u) ^ (z * 83492791u) ^ (ore_idx * 2654435761u) ^ vein_attempt;
            auto nextRand = [&seed]() {
                seed = seed * 1664525u + 1013904223u;
                return seed;
            };

            const int y_range = ore_dist.y_max - ore_dist.y_min;
            if(y_range < 0)
                continue;

            int world_y = ore_dist.y_min + static_cast<int>(nextRand() % static_cast<unsigned int>(y_range + 1));
            for(int i = 0; i < 6; ++i)
            {
                const int candidate = ore_dist.y_min + static_cast<int>(nextRand() % static_cast<unsigned int>(y_range + 1));
                const float weight = ore_dist.distribution == OreDistributionType::Triangle
                    ? triangleDistributionProbability(candidate, ore_dist)
                    : 1.0f;
                const float roll = static_cast<float>(nextRand() & 0xFFFFu) / 65535.0f;
                if(roll <= weight)
                {
                    world_y = candidate;
                    break;
                }
            }

            const int local_y = world_y - y * Chunk::SIZE;
            if(local_y < 0 || local_y >= Chunk::SIZE)
                continue;

            const int center_x = static_cast<int>(nextRand() % Chunk::SIZE);
            const int center_z = static_cast<int>(nextRand() % Chunk::SIZE);
            generateSingleOreVein(ore_dist, center_x, local_y, center_z, nextRand());
        }
    }
}

// Note: triangleDistributionProbability is now implemented in oregeneration.cpp
// This local version is not needed - removed to avoid redefinition

void Chunk::generateSingleOreVein(const OreDistribution &ore_dist, int center_x, int center_y, int center_z, unsigned int seed)
{
    auto nextRand = [&seed]() {
        seed = seed * 1664525u + 1013904223u;
        return seed;
    };

    int x = center_x;
    int y = center_y;
    int z = center_z;
    const int target_blocks = std::max(1, 1 + static_cast<int>(nextRand() % static_cast<unsigned int>(std::max(1, ore_dist.vein_size))));

    int placed = 0;
    const int tries = target_blocks * 8;
    for(int i = 0; i < tries && placed < target_blocks; ++i)
    {
        x += static_cast<int>(nextRand() % 3) - 1;
        y += static_cast<int>(nextRand() % 3) - 1;
        z += static_cast<int>(nextRand() % 3) - 1;

        if(x < 0 || x >= Chunk::SIZE || y < 0 || y >= Chunk::SIZE || z < 0 || z >= Chunk::SIZE)
            continue;

        if(getBLOCK(blocks[x][y][z]) != BLOCK_STONE)
            continue;

        blocks[x][y][z] = ore_dist.ore_block;
        ++placed;
    }
}

void Chunk::makeTree(unsigned int x, unsigned int y, unsigned int z)
{
    // Minecraft-like oak template (10 layers, 7x7 each).
    static const int tree[10][7][7] = {
        {{ 0, 0, 0, 0, 0, 0, 0 },{ 0, 0, 0, 0, 0, 0, 0 },{ 0, 0, 0, 0, 0, 0, 0 },{ 0, 0, 0, 4, 0, 0, 0 },{ 0, 0, 0, 0, 0, 0, 0 },{ 0, 0, 0, 0, 0, 0, 0 },{ 0, 0, 0, 0, 0, 0, 0 }},
        {{ 0, 0, 0, 0, 0, 0, 0 },{ 0, 0, 0, 0, 0, 0, 0 },{ 0, 0, 0, 0, 0, 0, 0 },{ 0, 0, 0, 4, 0, 0, 0 },{ 0, 0, 0, 0, 0, 0, 0 },{ 0, 0, 0, 0, 0, 0, 0 },{ 0, 0, 0, 0, 0, 0, 0 }},
        {{ 0, 0, 0, 0, 0, 0, 0 },{ 0, 0, 0, 0, 0, 0, 0 },{ 0, 0, 0, 0, 0, 0, 0 },{ 0, 0, 0, 4, 0, 0, 0 },{ 0, 0, 0, 0, 0, 0, 0 },{ 0, 0, 0, 0, 0, 0, 0 },{ 0, 0, 0, 0, 0, 0, 0 }},
        {{ 0, 0, 0, 5, 0, 0, 0 },{ 0, 0, 5, 5, 5, 0, 0 },{ 0, 5, 5, 5, 5, 5, 0 },{ 5, 5, 5, 4, 5, 5, 5 },{ 0, 5, 5, 5, 5, 5, 0 },{ 0, 0, 5, 5, 5, 0, 0 },{ 0, 0, 0, 5, 0, 0, 0 }},
        {{ 0, 0, 0, 0, 0, 0, 0 },{ 0, 0, 0, 0, 0, 0, 0 },{ 0, 0, 5, 5, 5, 0, 0 },{ 0, 0, 5, 4, 5, 0, 0 },{ 0, 0, 5, 5, 5, 0, 0 },{ 0, 0, 0, 0, 0, 0, 0 },{ 0, 0, 0, 0, 0, 0, 0 }},
        {{ 0, 0, 0, 0, 0, 0, 0 },{ 0, 0, 5, 5, 5, 0, 0 },{ 0, 5, 5, 5, 5, 5, 0 },{ 0, 5, 5, 4, 5, 5, 0 },{ 0, 5, 5, 5, 5, 5, 0 },{ 0, 0, 5, 5, 5, 0, 0 },{ 0, 0, 0, 0, 0, 0, 0 }},
        {{ 0, 0, 0, 0, 0, 0, 0 },{ 0, 0, 0, 0, 0, 0, 0 },{ 0, 0, 0, 5, 0, 0, 0 },{ 0, 0, 5, 4, 5, 0, 0 },{ 0, 0, 0, 5, 0, 0, 0 },{ 0, 0, 0, 0, 0, 0, 0 },{ 0, 0, 0, 0, 0, 0, 0 }},
        {{ 0, 0, 0, 0, 0, 0, 0 },{ 0, 0, 0, 5, 0, 0, 0 },{ 0, 0, 5, 5, 5, 0, 0 },{ 0, 5, 5, 4, 5, 5, 0 },{ 0, 0, 5, 5, 5, 0, 0 },{ 0, 0, 0, 5, 0, 0, 0 },{ 0, 0, 0, 0, 0, 0, 0 }},
        {{ 0, 0, 0, 0, 0, 0, 0 },{ 0, 0, 0, 0, 0, 0, 0 },{ 0, 0, 0, 5, 0, 0, 0 },{ 0, 0, 5, 5, 5, 0, 0 },{ 0, 0, 0, 5, 0, 0, 0 },{ 0, 0, 0, 0, 0, 0, 0 },{ 0, 0, 0, 0, 0, 0, 0 }},
        {{ 0, 0, 0, 0, 0, 0, 0 },{ 0, 0, 0, 0, 0, 0, 0 },{ 0, 0, 0, 0, 0, 0, 0 },{ 0, 0, 0, 5, 0, 0, 0 },{ 0, 0, 0, 0, 0, 0, 0 },{ 0, 0, 0, 0, 0, 0, 0 },{ 0, 0, 0, 0, 0, 0, 0 }},
    };

    int max_world_y = World::HEIGHT * Chunk::SIZE;
    int base_y = static_cast<int>(y) + this->y * Chunk::SIZE;
    if(base_y + 10 >= max_world_y)
        return;

    int trunk_x = static_cast<int>(x);
    int trunk_z = static_cast<int>(z);

    for(int layer = 0; layer < 10; ++layer)
        for(int dz = -3; dz <= 3; ++dz)
            for(int dx = -3; dx <= 3; ++dx)
            {
                int block = tree[layer][dz + 3][dx + 3];
                if(block == 0)
                    continue;

                int tx = trunk_x + dx;
                int ty = static_cast<int>(y) + layer;
                int tz = trunk_z + dz;

                if(block == BLOCK_WOOD)
                {
                    setGlobalBlockRelative(tx, ty, tz, BLOCK_WOOD);
                }
                else
                {
                    // During generation, unloaded neighboring chunks read as stone;
                    // always queue leaf placement so full canopies appear at chunk edges.
                    setGlobalBlockRelative(tx, ty, tz, BLOCK_LEAVES);
                }
            }
}

void Chunk::spawnDestructionParticles(const int x, const int y, const int z)
{
    if(!inBounds(x, y, z))
        return;

    const auto block = getLocalBlock(x, y, z);
    Particle p;
    p.size = 14;
    p.tae = global_block_renderer.materialTexture(block).current;

    // Use the center quarter of the texture
    const int tex_width = p.tae.right - p.tae.left,
              tex_height = p.tae.bottom - p.tae.top;
    p.tae.left += tex_width / 4;
    p.tae.right -= tex_width / 4;
    p.tae.top += tex_height / 4;
    p.tae.bottom -= tex_height / 4;

    // Random value between 0 and max (inclusive)
    const auto randMax = [](GLFix max) { return max * (rand() & 0xFF) / 0xFF; };

    // Get the center of the block contents (chunk relative coordinates)
    const auto aabb = global_block_renderer.getAABB(block, x * BLOCK_SIZE, y * BLOCK_SIZE, z * BLOCK_SIZE);
    const auto center = VECTOR3{(aabb.low_x + aabb.high_x) / 2, (aabb.low_y + aabb.high_y) / 2, (aabb.low_z + aabb.high_z) / 2};

    // Spawn four particles at the center with random velocity and offset
    for(int i = 0; i < 4; ++i)
    {
        p.vel = {randMax(10) - 5, randMax(5), randMax(10) - 5};
        p.pos = center;
        p.pos.x += randMax(100) - 50;
        p.pos.y += randMax(100) - 50;
        p.pos.z += randMax(100) - 50;
        addParticle(p);
    }
}

void drawLoadingtext(const int i)
{
    static int count = 0;
    static bool shown = false;

    if(i == -1)
    {
        count = 0;
        shown = false;
        return;
    }

    if(shown == true)
        return;

    count += 1;
    if(count < i)
        return;

    shown = true;

    if(lcd_type() != SCR_320x240_565 && lcd_type() != SCR_320x240_4)
        return;
    #ifdef _TINSPIRE
        TEXTURE screen;
        screen.width = SCREEN_WIDTH;
        screen.height = SCREEN_HEIGHT;
        screen.bitmap = reinterpret_cast<COLOR*>(REAL_SCREEN_BASE_ADDRESS);
        drawTexture(loadingtext, screen, 0, 0, loadingtext.width, loadingtext.height, (SCREEN_WIDTH - loadingtext.width) / 2, 0, loadingtext.width, loadingtext.height);
    #endif
}
