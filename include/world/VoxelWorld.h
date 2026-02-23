#pragma once
#include <cstdint>
#include <cstddef>

// Fixed-size voxel world: 100 x 100 x 100
// blocks_[x][y][z] = 1 means a cube exists at that cell, 0 means air.
class VoxelWorld {
public:
    static constexpr int SX = 100;
    static constexpr int SY = 100;
    static constexpr int SZ = 100;

    using Grid = uint8_t[SX][SY][SZ];

    VoxelWorld();

    Grid& grid() { return blocks_; }
    const Grid& grid() const { return blocks_; }

    bool inBounds(int x, int y, int z) const;
    bool hasBlock(int x, int y, int z) const;
    void setBlock(int x, int y, int z, bool on);

    void clear();
    std::size_t countSolid() const;

    // Column terrain across the whole map:
    // For each (x,z):
    //   h = baseY + amplitude * fbm2D(x*noiseScale, z*noiseScale)
    // Fill blocks for y <= h.
    void generateTerrainMidLevel(
        uint32_t seed = 1337,
        int baseY = SY / 2,
        double noiseScale = 0.08,
        double amplitude = 20.0,
        int octaves = 5,
        double lacunarity = 2.0,
        double gain = 0.5
    );

private:
    Grid blocks_{}; // zero-initialized
};