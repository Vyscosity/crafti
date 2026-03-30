#ifndef WORLD_H
#define WORLD_H

#include <vector>
#include <list>
#include <unordered_map>
#include <tuple>
#include <functional>

#include "terrain.h"
#include "chunk.h"
#include "perlinnoise.h"

// Hash function for std::tuple<int,int,int>
namespace std {
    template<>
    struct hash<std::tuple<int,int,int>> {
        size_t operator()(const std::tuple<int,int,int>& t) const {
            auto h1 = std::hash<int>{}(std::get<0>(t));
            auto h2 = std::hash<int>{}(std::get<1>(t));
            auto h3 = std::hash<int>{}(std::get<2>(t));
            return h1 ^ (h2 << 1) ^ (h3 << 2);
        }
    };
}

struct BLOCK_CHANGE {
    int chunk_x, chunk_y, chunk_z, local_x, local_y, local_z;
    BLOCK_WDATA block;
};

class World
{
public:
    World();
    ~World();
    void generateSeed();
    BLOCK_WDATA getBlock(const int x, const int y, const int z) const;
    void setBlock(const int x, const int y, const int z, const BLOCK_WDATA block, bool set_dirty = true);
    void changeBlock(const int x, const int y, const int z, const BLOCK_WDATA block);
    void setPosition(int x, int y, int z);
    bool blockAction(const int x, const int y, const int z);
    bool intersect(AABB &other) const;
    bool intersectsRay(GLFix x, GLFix y, GLFix z, GLFix dx, GLFix dy, GLFix dz, VECTOR3 &result, AABB::SIDE &side, GLFix &dist, bool ignore_water) const;
    const PerlinNoise &noiseGenerator() const;
    void clear();
    void setDirty();
    bool loadFromFile(gzFile file);
    bool saveToFile(gzFile file) const;
    void render();
    int fieldOfView() const { return field_of_view; }
    void setFieldOfView(int fov);

    enum class WorldType {
        Terrain,
        Flat
    };

    void setWorldType(WorldType type) { world_type = type; }
    WorldType worldType() const { return world_type; }

    Chunk *findChunk(int x, int y, int z) const;
    void spawnDestructionParticles(int x, int y, int z);

    static constexpr int HEIGHT = 5;

    void processBuildQueue(); // Process one chunk from the build queue

private:
    Chunk *generateChunk(int x, int y, int z);
    void setChunkVisible(int x, int y, int z);

    bool loaded = false;
    int cen_x = 0, cen_y = 0, cen_z = 0;
#ifdef _TINSPIRE
    int field_of_view = 120;
#else
    int field_of_view = 15;
#endif
    PerlinNoise perlin_noise;
    unsigned int seed;

    std::unordered_map<std::tuple<int,int,int>,Chunk*> all_chunks;
    std::vector<Chunk*> visible_chunks;
    std::list<Chunk*> build_queue; // Queue of chunks waiting for geometry build

    //If a block in a not yet loaded Chunk has been set, it's stored here until the Chunk has been loaded
    std::vector<BLOCK_CHANGE> pending_block_changes;

    WorldType world_type = WorldType::Terrain;
};

extern World world;

#endif // WORLD_H
