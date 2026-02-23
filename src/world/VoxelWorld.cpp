#include "world/VoxelWorld.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <numeric>
#include <random>

namespace {

class PerlinNoise {
public:
    explicit PerlinNoise(uint32_t seed) {
        std::array<int, 256> perm{};
        std::iota(perm.begin(), perm.end(), 0);

        std::mt19937 rng(seed);
        std::shuffle(perm.begin(), perm.end(), rng);

        for (int i = 0; i < 256; ++i) {
            p_[i] = perm[i];
            p_[i + 256] = perm[i];
        }
    }

    // 3D Perlin noise; for 2D we call noise(x,y,0)
    // output roughly in [-1, 1]
    double noise(double x, double y, double z = 0.0) const {
        int X = static_cast<int>(std::floor(x)) & 255;
        int Y = static_cast<int>(std::floor(y)) & 255;
        int Z = static_cast<int>(std::floor(z)) & 255;

        double xf = x - std::floor(x);
        double yf = y - std::floor(y);
        double zf = z - std::floor(z);

        double u = fade(xf);
        double v = fade(yf);
        double w = fade(zf);

        // Hash cube corners
        int A  = p_[X] + Y;
        int AA = p_[A] + Z;
        int AB = p_[A + 1] + Z;
        int B  = p_[X + 1] + Y;
        int BA = p_[B] + Z;
        int BB = p_[B + 1] + Z;

        double x1 = lerp(grad(p_[AA],     xf,       yf,       zf),
                         grad(p_[BA],     xf - 1.0, yf,       zf), u);
        double x2 = lerp(grad(p_[AB],     xf,       yf - 1.0, zf),
                         grad(p_[BB],     xf - 1.0, yf - 1.0, zf), u);
        double y1 = lerp(x1, x2, v);

        double x3 = lerp(grad(p_[AA + 1], xf,       yf,       zf - 1.0),
                         grad(p_[BA + 1], xf - 1.0, yf,       zf - 1.0), u);
        double x4 = lerp(grad(p_[AB + 1], xf,       yf - 1.0, zf - 1.0),
                         grad(p_[BB + 1], xf - 1.0, yf - 1.0, zf - 1.0), u);
        double y2 = lerp(x3, x4, v);

        return lerp(y1, y2, w);
    }

    // fBm in 2D
    double fbm2D(double x, double y, int octaves, double lacunarity, double gain) const {
        double sum = 0.0;
        double amp = 1.0;
        double freq = 1.0;
        double ampSum = 0.0;

        for (int i = 0; i < octaves; ++i) {
            sum += amp * noise(x * freq, y * freq, 0.0);
            ampSum += amp;
            amp *= gain;
            freq *= lacunarity;
        }

        if (ampSum > 0.0) sum /= ampSum;
        return sum;
    }

private:
    int p_[512]{};

    static double fade(double t) {
        return t * t * t * (t * (t * 6.0 - 15.0) + 10.0);
    }
    static double lerp(double a, double b, double t) {
        return a + t * (b - a);
    }
    static double grad(int hash, double x, double y, double z) {
        int h = hash & 15;
        double u = (h < 8) ? x : y;
        double v = (h < 4) ? y : ((h == 12 || h == 14) ? x : z);
        return ((h & 1) ? -u : u) + ((h & 2) ? -v : v);
    }
};

} // namespace

VoxelWorld::VoxelWorld() { clear(); }

bool VoxelWorld::inBounds(int x, int y, int z) const {
    return (0 <= x && x < SX) && (0 <= y && y < SY) && (0 <= z && z < SZ);
}

bool VoxelWorld::hasBlock(int x, int y, int z) const {
    if (!inBounds(x, y, z)) return false;
    return blocks_[x][y][z] != 0;
}

void VoxelWorld::setBlock(int x, int y, int z, bool on) {
    if (!inBounds(x, y, z)) return;
    blocks_[x][y][z] = on ? 1 : 0;
}

void VoxelWorld::clear() {
    for (int x = 0; x < SX; ++x)
        for (int y = 0; y < SY; ++y)
            for (int z = 0; z < SZ; ++z)
                blocks_[x][y][z] = 0;
}

std::size_t VoxelWorld::countSolid() const {
    std::size_t c = 0;
    for (int x = 0; x < SX; ++x)
        for (int y = 0; y < SY; ++y)
            for (int z = 0; z < SZ; ++z)
                if (blocks_[x][y][z]) ++c;
    return c;
}

void VoxelWorld::generateTerrainMidLevel(
    uint32_t seed,
    int baseY,
    double noiseScale,
    double amplitude,
    int octaves,
    double lacunarity,
    double gain
) {
    clear();

    baseY = std::clamp(baseY, 0, SY - 1);
    octaves = std::max(1, octaves);
    noiseScale = std::max(0.00001, noiseScale);

    PerlinNoise pn(seed);

    double ox = static_cast<double>((seed * 37u) % 1000u) * 0.001;
    double oz = static_cast<double>((seed * 91u) % 1000u) * 0.001;

    for (int z = 0; z < SZ; ++z) {
        for (int x = 0; x < SX; ++x) {
            double nx = (static_cast<double>(x) + ox) * noiseScale;
            double nz = (static_cast<double>(z) + oz) * noiseScale;

            double n = pn.fbm2D(nx, nz, octaves, lacunarity, gain);

            int h = static_cast<int>(std::lround(static_cast<double>(baseY) + amplitude * n));
            h = std::clamp(h, 0, SY - 1);

            for (int y = 0; y <= h; ++y) {
                blocks_[x][y][z] = 1;
            }
        }
    }
}