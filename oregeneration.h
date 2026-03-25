#ifndef OREGENERATION_H
#define OREGENERATION_H

#include <cstdint>
#include "terrain.h"
#include "gl.h"

// Ore distribution system based on Minecraft 1.18+ distribution
// Each ore has specific Y-level ranges, vein sizes, and spawn frequencies

enum class OreDistributionType {
    Uniform,   // Equal probability across entire Y-range
    Triangle   // Higher probability at peak_y, tapers at min/max
};

struct OreDistribution {
    BLOCK ore_block;
    int vein_size;              // Maximum blocks per vein
    int veins_per_chunk;        // Number of vein spawn attempts per chunk
    int y_min;                  // Minimum Y coordinate (relative to sea level area)
    int y_max;                  // Maximum Y coordinate
    GLFix peak_y;              // For Triangle distribution: Y with highest spawn chance. For Uniform: ignored
    OreDistributionType distribution;
    float air_exposure_discard_chance; // Chance to discard vein if exposed to air (0.0-1.0)
};

// Minecraft 1.18+ ore distribution parameters
namespace OreDistributions {
    // Coal: Very common, widespread
    const OreDistribution COAL = {
        BLOCK_COAL_ORE,
        17,     // vein_size
        20,     // veins per chunk
        0,      // y_min (world height relative)
        192,    // y_max
        96,     // peak at middle height
        OreDistributionType::Triangle,
        0.0f    // Not discarded on air exposure
    };

    // Iron: Common, mid-range
    const OreDistribution IRON = {
        BLOCK_IRON_ORE,
        9,      // vein_size (larger variant)
        10,     // veins per chunk
        -24,    // y_min
        56,     // y_max
        32,     // peak in mid-range
        OreDistributionType::Triangle,
        0.0f
    };

    // Gold: Rarer, deeper
    const OreDistribution GOLD = {
        BLOCK_GOLD_ORE,
        9,      // vein_size
        4,      // veins per chunk (rarer)
        -64,    // y_min (deep only)
        32,     // y_max
        -18,    // peak deep down
        OreDistributionType::Triangle,
        0.5f    // 50% chance to discard if exposed to air
    };

    // Redstone: Deep only
    const OreDistribution REDSTONE = {
        BLOCK_REDSTONE_ORE,
        8,      // vein_size
        4,      // veins per chunk
        -64,    // y_min (very deep)
        15,     // y_max
        -32,    // peak even deeper
        OreDistributionType::Triangle,
        0.0f
    };

    // Diamond: Very rare, deepest
    const OreDistribution DIAMOND = {
        BLOCK_DIAMOND_ORE,
        4,      // vein_size (small veins)
        7,      // veins per chunk
        -64,    // y_min (bedrock level)
        16,     // y_max
        -59,    // peak near bottom
        OreDistributionType::Triangle,
        0.5f    // 50% discard on air exposure
    };

    // All ores to generate
    const OreDistribution ore_list[] = {
        COAL,
        IRON,
        GOLD,
        REDSTONE,
        DIAMOND
    };
    
    constexpr int ore_count = sizeof(ore_list) / sizeof(ore_list[0]);
}

// Helper function to calculate triangle distribution probability for a given Y level
float triangleDistributionProbability(int y, const OreDistribution &dist);

// Helper function to determine Y position based on distribution type
// Returns Y position offset within chunk for a given ore distribution
int getOreYPosition(const OreDistribution &dist, int rand_seed, int chunk_world_y, int chunk_size);

#endif // OREGENERATION_H
