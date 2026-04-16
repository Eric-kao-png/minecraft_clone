#pragma once

#include <cstdint>

class TerrainGenerator {
public:
    TerrainGenerator(uint32_t seed = 2026);

    // 核心介面：輸入世界座標 (x, z)，回傳地表高度 Y
    int sampleHeight(int wx, int wz) const;

    // 設定與獲取參數
    void setSeed(uint32_t seed) { seed_ = seed; }
    uint32_t getSeed() const { return seed_; }

private:
    uint32_t seed_;
    int baseHeight_ = 36;
    double amplitude_ = 18.0;
    double noiseScale_ = 0.045;

    // 內部數學運算
    static uint32_t hash2D(int x, int z, uint32_t seed);
    static double randomSigned(int x, int z, uint32_t seed);
    static double valueNoise2D(double x, double z, uint32_t seed);
    static double fbm2D(double x, double z, uint32_t seed, int octaves, double lacunarity, double gain);
};