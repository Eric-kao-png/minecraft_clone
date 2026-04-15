#include "world/VoxelWorld.h"

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
        { 0.5f,-0.5f,-0.5f, 0,1}, { 0.5f,-0.5f, 0.5f, 0,0}, { 0.5f, 0.5f, 0.5f, 1,0},
    }},
    // -Y
    { 0,-1, 0,  0,-1, 0, {
        {-0.5f,-0.5f,-0.5f, 0,1}, { 0.5f,-0.5f,-0.5f, 1,1}, { 0.5f,-0.5f, 0.5f, 1,0},
        { 0.5f,-0.5f, 0.5f, 1,0}, {-0.5f,-0.5f, 0.5f, 0,0}, {-0.5f,-0.5f,-0.5f, 0,1},
    }},
    // +Y
    { 0, 1, 0,  0, 1, 0, {
        {-0.5f, 0.5f,-0.5f, 0,1}, { 0.5f, 0.5f,-0.5f, 1,1}, { 0.5f, 0.5f, 0.5f, 1,0},
        { 0.5f, 0.5f, 0.5f, 1,0}, {-0.5f, 0.5f, 0.5f, 0,0}, {-0.5f, 0.5f,-0.5f, 0,1},
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

static double smoothstep(double t) {
    return t * t * (3.0 - 2.0 * t);
}

static double lerp(double a, double b, double t) {
    return a + (b - a) * t;
}

static double valueNoise2D(double x, double z, uint32_t seed) {
    const int x0 = static_cast<int>(std::floor(x));
    const int z0 = static_cast<int>(std::floor(z));
    const int x1 = x0 + 1;
    const int z1 = z0 + 1;

    const double tx = x - static_cast<double>(x0);
    const double tz = z - static_cast<double>(z0);
    const double sx = smoothstep(tx);
    const double sz = smoothstep(tz);

    const double v00 = randomSigned(x0, z0, seed);
    const double v10 = randomSigned(x1, z0, seed);
    const double v01 = randomSigned(x0, z1, seed);
    const double v11 = randomSigned(x1, z1, seed);

    return lerp(lerp(v00, v10, sx), lerp(v01, v11, sx), sz);
}

static double fbm2D(double x,
                    double z,
                    uint32_t seed,
                    int octaves,
                    double lacunarity,
                    double gain) {
    double sum = 0.0;
    double amp = 1.0;
    double freq = 1.0;
    double ampSum = 0.0;

    for (int i = 0; i < octaves; ++i) {
        sum += amp * valueNoise2D(x * freq, z * freq, seed + static_cast<uint32_t>(i * 97));
        ampSum += amp;
        amp *= gain;
        freq *= lacunarity;
    }

    return (ampSum > 0.0) ? (sum / ampSum) : 0.0;
}

} // namespace

std::size_t VoxelWorld::ChunkCoordHash::operator()(const ChunkCoord& c) const {
    const std::size_t h1 = std::hash<int>{}(c.x);
    const std::size_t h2 = std::hash<int>{}(c.z);
    return h1 ^ (h2 + 0x9e3779b9u + (h1 << 6u) + (h1 >> 2u));
}

VoxelWorld::Chunk::Chunk(int cx, int cz)
    : coord{cx, cz},
      blocks(static_cast<std::size_t>(CHUNK_SIZE_X) * CHUNK_SIZE_Y * CHUNK_SIZE_Z, 0) {}

uint8_t& VoxelWorld::Chunk::at(int lx, int y, int lz) {
    const std::size_t index = (static_cast<std::size_t>(y) * CHUNK_SIZE_Z + lz) * CHUNK_SIZE_X + lx;
    return blocks[index];
}

uint8_t VoxelWorld::Chunk::at(int lx, int y, int lz) const {
    const std::size_t index = (static_cast<std::size_t>(y) * CHUNK_SIZE_Z + lz) * CHUNK_SIZE_X + lx;
    return blocks[index];
}

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

int VoxelWorld::worldToBlock(float v) {
    return static_cast<int>(std::floor(v + 0.5f));
}

VoxelWorld::Chunk* VoxelWorld::getLoadedChunk(int cx, int cz) {
    auto it = chunks_.find(ChunkCoord{cx, cz});
    return (it == chunks_.end()) ? nullptr : it->second.get();
}

const VoxelWorld::Chunk* VoxelWorld::getLoadedChunk(int cx, int cz) const {
    auto it = chunks_.find(ChunkCoord{cx, cz});
    return (it == chunks_.end()) ? nullptr : it->second.get();
}

VoxelWorld::Chunk& VoxelWorld::getOrCreateChunk(int cx, int cz) {
    auto [it, inserted] = chunks_.try_emplace(ChunkCoord{cx, cz}, std::make_unique<Chunk>(cx, cz));
    if (inserted) {
        generateChunk(*it->second);
    }
    return *it->second;
}

int VoxelWorld::sampleSurfaceY(int wx, int wz) const {
    const double n = fbm2D(static_cast<double>(wx) * noiseScale_,
                           static_cast<double>(wz) * noiseScale_,
                           seed_,
                           octaves_,
                           lacunarity_,
                           gain_);

    int h = static_cast<int>(std::lround(static_cast<double>(baseHeight_) + amplitude_ * n));
    h = std::clamp(h, 1, CHUNK_SIZE_Y - 1);
    return h;
}

bool VoxelWorld::sampleGeneratedBlock(int wx, int wy, int wz) const {
    if (wy < 0 || wy >= CHUNK_SIZE_Y) return false;
    return wy <= sampleSurfaceY(wx, wz);
}

bool VoxelWorld::hasBlockGlobal(int wx, int wy, int wz) const {
    if (wy < 0 || wy >= CHUNK_SIZE_Y) return false;

    const int cx = worldToChunkX(wx);
    const int cz = worldToChunkZ(wz);
    if (const Chunk* chunk = getLoadedChunk(cx, cz)) {
        const int lx = worldToLocalX(wx);
        const int lz = worldToLocalZ(wz);
        return chunk->at(lx, wy, lz) != 0;
    }

    return sampleGeneratedBlock(wx, wy, wz);
}

void VoxelWorld::generateChunk(Chunk& chunk) const {
    for (int y = 0; y < CHUNK_SIZE_Y; ++y) {
        for (int lz = 0; lz < CHUNK_SIZE_Z; ++lz) {
            for (int lx = 0; lx < CHUNK_SIZE_X; ++lx) {
                const int wx = chunk.coord.x * CHUNK_SIZE_X + lx;
                const int wz = chunk.coord.z * CHUNK_SIZE_Z + lz;
                chunk.at(lx, y, lz) = sampleGeneratedBlock(wx, y, wz) ? 1u : 0u;
            }
        }
    }

    chunk.dirty = true;
}

void VoxelWorld::buildChunkMesh(Chunk& chunk) const {
    chunk.mesh.clear();
    chunk.mesh.reserve(180000);

    for (int y = 0; y < CHUNK_SIZE_Y; ++y) {
        for (int lz = 0; lz < CHUNK_SIZE_Z; ++lz) {
            for (int lx = 0; lx < CHUNK_SIZE_X; ++lx) {
                if (chunk.at(lx, y, lz) == 0) continue;

                const int wx = chunk.coord.x * CHUNK_SIZE_X + lx;
                const int wy = y;
                const int wz = chunk.coord.z * CHUNK_SIZE_Z + lz;

                for (const auto& face : kFaces) {
                    if (hasBlockGlobal(wx + face.dx, wy + face.dy, wz + face.dz))
                        continue;

                    for (int i = 0; i < 6; ++i) {
                        pushVertex(chunk.mesh,
                                   static_cast<float>(wx) + face.v[i][0],
                                   static_cast<float>(wy) + face.v[i][1],
                                   static_cast<float>(wz) + face.v[i][2],
                                   face.nx, face.ny, face.nz,
                                   face.v[i][3], face.v[i][4]);
                    }
                }
            }
        }
    }

    chunk.vertexCount = static_cast<int>(chunk.mesh.size() / 8);
    chunk.dirty = false;
}

void VoxelWorld::uploadChunkMesh(Chunk& chunk) const {
    if (chunk.vao == 0) {
        glGenVertexArrays(1, &chunk.vao);
        glGenBuffers(1, &chunk.vbo);

        glBindVertexArray(chunk.vao);
        glBindBuffer(GL_ARRAY_BUFFER, chunk.vbo);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
        glEnableVertexAttribArray(2);
    }

    glBindVertexArray(chunk.vao);
    glBindBuffer(GL_ARRAY_BUFFER, chunk.vbo);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(chunk.mesh.size() * sizeof(float)),
                 chunk.mesh.empty() ? nullptr : chunk.mesh.data(),
                 GL_DYNAMIC_DRAW);
}

void VoxelWorld::markChunkAndNeighborsDirty(int cx, int cz) {
    static const int offsets[5][2] = {
        { 0, 0}, { 1, 0}, {-1, 0}, { 0, 1}, { 0,-1}
    };

    for (const auto& off : offsets) {
        if (Chunk* chunk = getLoadedChunk(cx + off[0], cz + off[1])) {
            chunk->dirty = true;
        }
    }
}

void VoxelWorld::setBlockGlobal(int wx, int wy, int wz, bool solid) {
    if (wy < 0 || wy >= CHUNK_SIZE_Y) return;

    const int cx = worldToChunkX(wx);
    const int cz = worldToChunkZ(wz);
    Chunk& chunk = getOrCreateChunk(cx, cz);

    const int lx = worldToLocalX(wx);
    const int lz = worldToLocalZ(wz);
    const uint8_t next = solid ? 1u : 0u;
    if (chunk.at(lx, wy, lz) == next) return;

    chunk.at(lx, wy, lz) = next;
    chunk.modified = true;
    markChunkAndNeighborsDirty(cx, cz);
}

void VoxelWorld::updateStreaming(const glm::vec3& playerPos, int loadRadius, int unloadRadius) {
    const int playerBlockX = worldToBlock(playerPos.x);
    const int playerBlockZ = worldToBlock(playerPos.z);
    const int centerCX = worldToChunkX(playerBlockX);
    const int centerCZ = worldToChunkZ(playerBlockZ);

    for (int dz = -loadRadius; dz <= loadRadius; ++dz) {
        for (int dx = -loadRadius; dx <= loadRadius; ++dx) {
            getOrCreateChunk(centerCX + dx, centerCZ + dz);
        }
    }

    for (auto it = chunks_.begin(); it != chunks_.end(); ) {
        const int dx = std::abs(it->first.x - centerCX);
        const int dz = std::abs(it->first.z - centerCZ);
        const bool tooFar = (dx > unloadRadius || dz > unloadRadius);

        if (tooFar && !it->second->modified) {
            if (it->second->vao != 0) {
                glDeleteVertexArrays(1, &it->second->vao);
                glDeleteBuffers(1, &it->second->vbo);
            }
            it = chunks_.erase(it);
        } else {
            ++it;
        }
    }

    for (auto& [coord, chunkPtr] : chunks_) {
        Chunk& chunk = *chunkPtr;
        if (!chunk.dirty) continue;
        buildChunkMesh(chunk);
        uploadChunkMesh(chunk);
    }
}

void VoxelWorld::render() const {
    for (const auto& [coord, chunkPtr] : chunks_) {
        const Chunk& chunk = *chunkPtr;
        if (chunk.vertexCount <= 0 || chunk.vao == 0) continue;
        glBindVertexArray(chunk.vao);
        glDrawArrays(GL_TRIANGLES, 0, chunk.vertexCount);
    }
}

bool VoxelWorld::raycast(const glm::vec3& origin,
                         const glm::vec3& dir,
                         float maxDist,
                         float step,
                         RaycastHit& out) const {
    const float dirLen = glm::length(dir);
    if (dirLen <= 1e-6f) return false;

    const glm::vec3 d = dir / dirLen;

    auto approxNormalFromDir = [](const glm::vec3& rayDir) -> glm::ivec3 {
        const glm::vec3 a = glm::abs(rayDir);
        if (a.x >= a.y && a.x >= a.z) return glm::ivec3(rayDir.x > 0.0f ? -1 : 1, 0, 0);
        if (a.y >= a.x && a.y >= a.z) return glm::ivec3(0, rayDir.y > 0.0f ? -1 : 1, 0);
        return glm::ivec3(0, 0, rayDir.z > 0.0f ? -1 : 1);
    };

    auto worldPosToCell = [](const glm::vec3& p) -> glm::ivec3 {
        return glm::ivec3(
            static_cast<int>(std::floor(p.x + 0.5f)),
            static_cast<int>(std::floor(p.y + 0.5f)),
            static_cast<int>(std::floor(p.z + 0.5f))
        );
    };

    glm::ivec3 lastCell = worldPosToCell(origin);
    glm::ivec3 prevCell = lastCell;
    bool hasPrev = false;

    for (float t = 0.0f; t <= maxDist; t += step) {
        const glm::vec3 p = origin + d * t;
        const glm::ivec3 cell = worldPosToCell(p);

        if (cell != lastCell) {
            prevCell = lastCell;
            hasPrev = true;
            lastCell = cell;
        }

        if (hasBlockGlobal(cell.x, cell.y, cell.z)) {
            out.hit = true;
            out.block = cell;
            out.normal = hasPrev ? (prevCell - cell) : approxNormalFromDir(d);
            out.t = t;
            if (out.normal == glm::ivec3(0)) {
                out.normal = approxNormalFromDir(d);
            }
            return true;
        }
    }

    return false;
}

void VoxelWorld::clear() {
    for (auto& [coord, chunkPtr] : chunks_) {
        if (chunkPtr->vao != 0) {
            glDeleteVertexArrays(1, &chunkPtr->vao);
            glDeleteBuffers(1, &chunkPtr->vbo);
        }
    }
    chunks_.clear();
}
