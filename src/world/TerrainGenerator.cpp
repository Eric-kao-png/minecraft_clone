#include "world/TerrainGenerator.h"
#include "world/Chunk.h" // 為了使用 Chunk::SIZE_Y 的限制
#include <cmath>
#include <algorithm>

namespace {
    // 內部輔助數學函式
    inline double smoothstep(double t) { return t * t * (3.0 - 2.0 * t); }
    inline double lerp(double a, double b, double t) { return a + (b - a) * t; }
}

TerrainGenerator::TerrainGenerator(uint32_t seed) : seed_(seed) {}

int TerrainGenerator::sampleHeight(int wx, int wz) const {
    double n = fbm2D(wx * noiseScale_, wz * noiseScale_, seed_, 4, 2.0, 0.5);
    int y = static_cast<int>(std::lround(baseHeight_ + amplitude_ * n));
    return std::clamp(y, 0, Chunk::SIZE_Y - 1);
}

uint32_t TerrainGenerator::hash2D(int x, int z, uint32_t seed) {
    uint32_t h = seed;
    h ^= static_cast<uint32_t>(x) * 0x27d4eb2du;
    h = (h << 15u) | (h >> 17u);
    h ^= static_cast<uint32_t>(z) * 0x85ebca6bu;
    h ^= h >> 16u;
    h *= 0x7feb352du;
    h ^= h >> 15u;
    h *= 0x846ca68bu;
    h ^= h >> 16u;
    return h;
}

double TerrainGenerator::randomSigned(int x, int z, uint32_t seed) {
    const uint32_t h = hash2D(x, z, seed);
    return (static_cast<double>(h & 0x00ffffffu) / static_cast<double>(0x00ffffffu)) * 2.0 - 1.0;
}

double TerrainGenerator::valueNoise2D(double x, double z, uint32_t seed) {
    const int x0 = static_cast<int>(std::floor(x)), z0 = static_cast<int>(std::floor(z));
    const double tx = x - x0, tz = z - z0;
    const double sx = smoothstep(tx), sz = smoothstep(tz);
    const double v00 = randomSigned(x0, z0, seed), v10 = randomSigned(x0 + 1, z0, seed);
    const double v01 = randomSigned(x0, z0 + 1, seed), v11 = randomSigned(x0 + 1, z0 + 1, seed);
    return lerp(lerp(v00, v10, sx), lerp(v01, v11, sx), sz);
}

double TerrainGenerator::fbm2D(double x, double z, uint32_t seed, int octaves, double lacunarity, double gain) {
    double sum = 0.0, amp = 1.0, freq = 1.0, ampSum = 0.0;
    for (int i = 0; i < octaves; ++i) {
        sum += amp * valueNoise2D(x * freq, z * freq, seed + static_cast<uint32_t>(i * 97));
        ampSum += amp; amp *= gain; freq *= lacunarity;
    }
    return (ampSum > 0.0) ? (sum / ampSum) : 0.0;
}