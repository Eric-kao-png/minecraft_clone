#pragma once
#include <cstdint>
#include <cstddef>

class VoxelWorld {
public:
    static constexpr int SX = 100;
    static constexpr int SY = 100;
    static constexpr int SZ = 100;

    // 0 = air, 1 = solid
    using Grid = uint8_t[SX][SY][SZ];

    VoxelWorld(); // (no-arg constructor)

    // Raw 3D array access
    Grid& grid() { return blocks_; }
    const Grid& grid() const { return blocks_; }

    // Bounds / accessors
    bool inBounds(int x, int y, int z) const;
    bool hasBlock(int x, int y, int z) const;
    void setBlock(int x, int y, int z, bool on);

    // Utilities
    void clear();
    std::size_t countSolid() const;

    // Generate a heightmap terrain across the whole world:
    // For each (x,z):
    //   n = fbm2D(x*noiseScale, z*noiseScale)
    //   h = baseY + amplitude * n
    //   fill blocks for y <= h
    //
    // Parameters:
    //   seed        : controls the random permutation table -> deterministic terrain
    //   baseY       : average ground level (default ~ middle)
    //   noiseScale  : larger -> more frequent bumps; smaller -> wider hills
    //   amplitude   : height variation around baseY
    //   octaves     : number of fBm layers (detail level)
    //   lacunarity  : frequency multiplier per octave
    //   gain        : amplitude multiplier per octave
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
    Grid blocks_; // blocks_[x][y][z]
};