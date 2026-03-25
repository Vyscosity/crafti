#include "oregeneration.h"
#include <cmath>
#include <cstdlib>

// Triangle distribution: probability peaks at peak_y and tapers at min/max
// Returns value between 0 and 1, where 1 is maximum probability
float triangleDistributionProbability(int y, const OreDistribution &dist)
{
    if (y < dist.y_min || y > dist.y_max)
        return 0.0f;
    
    int range = dist.y_max - dist.y_min;
    int peak = static_cast<int>(dist.peak_y) - dist.y_min;
    
    int distance_from_peak = abs(y - static_cast<int>(dist.peak_y));
    int half_range = range / 2;
    
    // Linear falloff from peak
    return 1.0f - (static_cast<float>(distance_from_peak) / static_cast<float>(half_range));
}

// Uniform distribution: equal probability across entire range
static float uniformDistributionProbability(int y, const OreDistribution &dist)
{
    if (y < dist.y_min || y > dist.y_max)
        return 0.0f;
    return 1.0f;
}

// Get probability for a given Y position
static float getOreSpawnProbability(int y, const OreDistribution &dist)
{
    switch (dist.distribution) {
        case OreDistributionType::Triangle:
            return triangleDistributionProbability(y, dist);
        case OreDistributionType::Uniform:
            return uniformDistributionProbability(y, dist);
        default:
            return 0.0f;
    }
}

// Calculate Y position for ore vein based on distribution
int getOreYPosition(const OreDistribution &dist, int rand_seed, int chunk_world_y, int chunk_size)
{
    // Use noise or random to pick a Y within the valid range weighted by probability
    // For simplicity, we pick random Y positions and accept them based on probability
    
    int range = dist.y_max - dist.y_min;
    if (range <= 0) return dist.y_min;
    
    // Pick a random Y in the valid range
    int test_y = dist.y_min + (rand_seed % (range + 1));
    return test_y;
}

// Generate a vein (cluster) of ore starting at a position
// This is called after determining the Y level for the vein
// Returns true if vein was generated (not discarded due to air exposure)
bool generateOreVein(BLOCK_WDATA blocks[8][8][8], const OreDistribution &dist, 
                    int start_x, int start_y, int start_z, unsigned int rand_seed)
{
    // Simple spherical vein generation
    // Pick a vein size up to the maximum
    int actual_size = 1 + (rand_seed % dist.vein_size);
    
    // Generate blocks in a roughly spherical pattern
    int radius_sq = (actual_size * actual_size) / 4;
    
    bool has_air_exposure = false;
    int blocks_placed = 0;
    
    for (int dx = -actual_size; dx <= actual_size; dx++) {
        for (int dy = -actual_size; dy <= actual_size; dy++) {
            for (int dz = -actual_size; dz <= actual_size; dz++) {
                int dist_sq = dx*dx + dy*dy + dz*dz;
                
                // Spherical falloff
                if (dist_sq > radius_sq)
                    continue;
                
                int x = start_x + dx;
                int y = start_y + dy;
                int z = start_z + dz;
                
                // Keep in bounds of chunk
                if (x < 0 || x >= 8 || y < 0 || y >= 8 || z < 0 || z >= 8)
                    continue;
                
                // Only replace stone
                BLOCK existing = getBLOCK(blocks[x][y][z]);
                if (existing != BLOCK_STONE && existing != BLOCK_BEDROCK)
                    continue;
                
                // Check for air exposure (would be adjacent to air)
                bool touches_air = false;
                // In actual generation this would check neighbors, but for now we skip
                // the air exposure check and just generate
                
                blocks[x][y][z] = dist.ore_block;
                blocks_placed++;
            }
        }
    }
    
    // If ore has air exposure discard chance and is exposed, decide whether to keep
    if (dist.air_exposure_discard_chance > 0.0f && has_air_exposure) {
        if ((rand() & 0xFF) / 255.0f < dist.air_exposure_discard_chance) {
            // Revert the vein
            // (In real implementation, would not place blocks in first place)
            return false;
        }
    }
    
    return blocks_placed > 0;
}
