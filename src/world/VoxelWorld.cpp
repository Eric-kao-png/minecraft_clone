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

} // namespace

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
    // 1. 委託 ChunkManager 處理區塊的載入與卸載
    chunkManager_.updateStreaming(playerPos, loadRadius, unloadRadius, generator_);

    // 2. 處理需要重新建模 (Meshing) 的區塊
    chunkManager_.forEachChunk([this](Chunk& chunk) {
        if (chunk.dirty) {
            buildChunkMesh(chunk);
            chunk.uploadMesh();
        }
    });
}

void VoxelWorld::render() const {
    chunkManager_.forEachChunk([](const Chunk& chunk) {
        chunk.render();
    });
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