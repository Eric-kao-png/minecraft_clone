#include "world/VoxelWorld.h"
#include "world/Chunk.h"

#include <glad/glad.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>

namespace {

struct FaceDef {
    int dx, dy, dz;
    float nx, ny, nz;
    float v[6][5];
};

constexpr FaceDef kFaces[6] = {
    // -Z
    { 0, 0,-1,  0, 0,-1, {
        {-0.5f,-0.5f,-0.5f, 0,0}, { 0.5f,-0.5f,-0.5f, 1,0}, { 0.5f, 0.5f,-0.5f, 1,1},
        { 0.5f, 0.5f,-0.5f, 1,1}, {-0.5f, 0.5f,-0.5f, 0,1}, {-0.5f,-0.5f,-0.5f, 0,0},
    }},
    // +Z
    { 0, 0, 1,  0, 0, 1, {
        {-0.5f,-0.5f, 0.5f, 0,0}, { 0.5f,-0.5f, 0.5f, 1,0}, { 0.5f, 0.5f, 0.5f, 1,1},
        { 0.5f, 0.5f, 0.5f, 1,1}, {-0.5f, 0.5f, 0.5f, 0,1}, {-0.5f,-0.5f, 0.5f, 0,0},
    }},
    // -X
    {-1, 0, 0, -1, 0, 0, {
        {-0.5f, 0.5f, 0.5f, 1,0}, {-0.5f, 0.5f,-0.5f, 1,1}, {-0.5f,-0.5f,-0.5f, 0,1},
        {-0.5f,-0.5f,-0.5f, 0,1}, {-0.5f,-0.5f, 0.5f, 0,0}, {-0.5f, 0.5f, 0.5f, 1,0},
    }},
    // +X
    { 1, 0, 0,  1, 0, 0, {
        { 0.5f, 0.5f, 0.5f, 1,0}, { 0.5f, 0.5f,-0.5f, 1,1}, { 0.5f,-0.5f,-0.5f, 0,1},
        { 0.5f,-0.5f,-0.5f, 0,1}, { 0.5f,-0.5f, 0.5f, 0,0}, { 0.5f, 0.5f, 0.5f, 1,0}
    }},
    // -Y
    { 0,-1, 0,  0,-1, 0, {
        {-0.5f,-0.5f,-0.5f, 0,1}, { 0.5f,-0.5f,-0.5f, 1,1}, { 0.5f,-0.5f, 0.5f, 1,0},
        { 0.5f,-0.5f, 0.5f, 1,0}, {-0.5f,-0.5f, 0.5f, 0,0}, {-0.5f,-0.5f,-0.5f, 0,1}
    }},
    // +Y
    { 0, 1, 0,  0, 1, 0, {
        {-0.5f, 0.5f,-0.5f, 0,1}, { 0.5f, 0.5f,-0.5f, 1,1}, { 0.5f, 0.5f, 0.5f, 1,0},
        { 0.5f, 0.5f, 0.5f, 1,0}, {-0.5f, 0.5f, 0.5f, 0,0}, {-0.5f, 0.5f,-0.5f, 0,1}
    }},
};

static void pushVertex(std::vector<float>& out,
                       float px, float py, float pz,
                       float nx, float ny, float nz,
                       float u, float v) {
    out.push_back(px); out.push_back(py); out.push_back(pz);
    out.push_back(nx); out.push_back(ny); out.push_back(nz);
    out.push_back(u);  out.push_back(v);
}

static uint32_t hash2D(int x, int z, uint32_t seed) {
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

static double randomSigned(int x, int z, uint32_t seed) {
    const uint32_t h = hash2D(x, z, seed);
    return (static_cast<double>(h & 0x00ffffffu) / static_cast<double>(0x00ffffffu)) * 2.0 - 1.0;
}

static double smoothstep(double t) { return t * t * (3.0 - 2.0 * t); }
static double lerp(double a, double b, double t) { return a + (b - a) * t; }

static double valueNoise2D(double x, double z, uint32_t seed) {
    const int x0 = static_cast<int>(std::floor(x)), z0 = static_cast<int>(std::floor(z));
    const double tx = x - x0, tz = z - z0;
    const double sx = smoothstep(tx), sz = smoothstep(tz);
    const double v00 = randomSigned(x0, z0, seed), v10 = randomSigned(x0 + 1, z0, seed);
    const double v01 = randomSigned(x0, z0 + 1, seed), v11 = randomSigned(x0 + 1, z0 + 1, seed);
    return lerp(lerp(v00, v10, sx), lerp(v01, v11, sx), sz);
}

static double fbm2D(double x, double z, uint32_t seed, int octaves, double lacunarity, double gain) {
    double sum = 0.0, amp = 1.0, freq = 1.0, ampSum = 0.0;
    for (int i = 0; i < octaves; ++i) {
        sum += amp * valueNoise2D(x * freq, z * freq, seed + static_cast<uint32_t>(i * 97));
        ampSum += amp; amp *= gain; freq *= lacunarity;
    }
    return (ampSum > 0.0) ? (sum / ampSum) : 0.0;
}

} // namespace

int VoxelWorld::floorDiv(int a, int b) {
    int q = a / b;
    int r = a % b;
    if (r != 0 && ((r > 0) != (b > 0))) --q;
    return q;
}

int VoxelWorld::positiveMod(int a, int b) {
    int m = a % b;
    if (m < 0) m += (b < 0 ? -b : b);
    return m;
}

int VoxelWorld::worldToBlock(float v) { return static_cast<int>(std::floor(v + 0.5f)); }

Chunk* VoxelWorld::getLoadedChunk(int cx, int cz) {
    auto it = chunks_.find(ChunkCoord{cx, cz});
    return (it == chunks_.end()) ? nullptr : it->second.get();
}

const Chunk* VoxelWorld::getLoadedChunk(int cx, int cz) const {
    auto it = chunks_.find(ChunkCoord{cx, cz});
    return (it == chunks_.end()) ? nullptr : it->second.get();
}

Chunk& VoxelWorld::getOrCreateChunk(int cx, int cz) {
    auto [it, inserted] = chunks_.try_emplace(ChunkCoord{cx, cz}, nullptr);
    if (inserted) {
        it->second = std::make_unique<Chunk>(cx, cz);
        generateChunk(*it->second);
    }
    return *it->second;
}

int VoxelWorld::sampleSurfaceY(int wx, int wz) const {
    double n = fbm2D(wx * noiseScale_, wz * noiseScale_, seed_, 4, 2.0, 0.5);
    int y = static_cast<int>(std::lround(baseHeight_ + amplitude_ * n));
    return std::clamp(y, 0, Chunk::SIZE_Y - 1);
}

bool VoxelWorld::hasBlockGlobal(int wx, int wy, int wz) const {
    if (wy < 0 || wy >= Chunk::SIZE_Y) return false;
    if (const Chunk* chunk = getLoadedChunk(floorDiv(wx, Chunk::SIZE_X), floorDiv(wz, Chunk::SIZE_Z)))
        return chunk->at(positiveMod(wx, Chunk::SIZE_X), wy, positiveMod(wz, Chunk::SIZE_Z)) != 0;
    return wy <= sampleSurfaceY(wx, wz);
}

void VoxelWorld::setBlockGlobal(int wx, int wy, int wz, bool solid) {
    if (wy < 0 || wy >= Chunk::SIZE_Y) return;
    int cx = floorDiv(wx, Chunk::SIZE_X), cz = floorDiv(wz, Chunk::SIZE_Z);
    Chunk& chunk = getOrCreateChunk(cx, cz);
    uint8_t newVal = solid ? 1u : 0u;
    int lx = positiveMod(wx, Chunk::SIZE_X), lz = positiveMod(wz, Chunk::SIZE_Z);
    if (chunk.at(lx, wy, lz) == newVal) return;
    chunk.at(lx, wy, lz) = newVal;
    chunk.modified = true;
    markChunkAndNeighborsDirty(cx, cz);
}

void VoxelWorld::markChunkAndNeighborsDirty(int cx, int cz) {
    for (int dz = -1; dz <= 1; ++dz)
        for (int dx = -1; dx <= 1; ++dx)
            if (Chunk* c = getLoadedChunk(cx + dx, cz + dz)) c->dirty = true;
}

void VoxelWorld::generateChunk(Chunk& chunk) const {
    for (int y = 0; y < Chunk::SIZE_Y; ++y)
        for (int lz = 0; lz < Chunk::SIZE_Z; ++lz)
            for (int lx = 0; lx < Chunk::SIZE_X; ++lx)
                chunk.at(lx, y, lz) = (y <= sampleSurfaceY(chunk.coord.x * Chunk::SIZE_X + lx, chunk.coord.z * Chunk::SIZE_Z + lz)) ? 1u : 0u;
    chunk.dirty = true;
}

void VoxelWorld::buildChunkMesh(Chunk& chunk) const {
    chunk.mesh.clear();
    for (int y = 0; y < Chunk::SIZE_Y; ++y)
        for (int lz = 0; lz < Chunk::SIZE_Z; ++lz)
            for (int lx = 0; lx < Chunk::SIZE_X; ++lx) {
                if (chunk.at(lx, y, lz) == 0) continue;
                int wx = chunk.coord.x * Chunk::SIZE_X + lx, wz = chunk.coord.z * Chunk::SIZE_Z + lz;
                for (const auto& f : kFaces)
                    if (!hasBlockGlobal(wx + f.dx, y + f.dy, wz + f.dz))
                        for (int i = 0; i < 6; ++i)
                            pushVertex(chunk.mesh, wx + f.v[i][0], y + f.v[i][1], wz + f.v[i][2], f.nx, f.ny, f.nz, f.v[i][3], f.v[i][4]);
            }
    chunk.dirty = false;
}

void VoxelWorld::updateStreaming(const glm::vec3& playerPos, int loadRadius, int unloadRadius) {
    int centerCX = floorDiv(worldToBlock(playerPos.x), Chunk::SIZE_X);
    int centerCZ = floorDiv(worldToBlock(playerPos.z), Chunk::SIZE_Z);
    for (int dz = -loadRadius; dz <= loadRadius; ++dz)
        for (int dx = -loadRadius; dx <= loadRadius; ++dx) getOrCreateChunk(centerCX + dx, centerCZ + dz);

    for (auto it = chunks_.begin(); it != chunks_.end(); ) {
        if (std::abs(it->first.x - centerCX) > unloadRadius || std::abs(it->first.z - centerCZ) > unloadRadius) {
            if (!it->second->modified) { it = chunks_.erase(it); continue; }
        }
        if (it->second->dirty) { buildChunkMesh(*it->second); it->second->uploadMesh(); }
        ++it;
    }
}

void VoxelWorld::render() const {
    for (const auto& [coord, chunkPtr] : chunks_) chunkPtr->render();
}

bool VoxelWorld::raycast(const glm::vec3& origin, const glm::vec3& dir, float maxDist, float step, RaycastHit& out) const {
    glm::vec3 nDir = glm::normalize(dir);
    for (float t = 0; t < maxDist; t += step) {
        glm::vec3 p = origin + nDir * t;
        glm::ivec3 cell(std::floor(p.x + 0.5f), std::floor(p.y + 0.5f), std::floor(p.z + 0.5f));
        if (hasBlockGlobal(cell.x, cell.y, cell.z)) {
            out.hit = true; out.block = cell; out.t = t;
            glm::vec3 prevP = origin + nDir * (t - step);
            out.normal = glm::ivec3(std::floor(prevP.x + 0.5f), std::floor(prevP.y + 0.5f), std::floor(prevP.z + 0.5f)) - cell;
            return true;
        }
    }
    return false;
}

void VoxelWorld::clear() { chunks_.clear(); }