#include "world/WorldMesher.h"
#include "world/Chunk.h"
#include "world/ChunkManager.h"
#include "world/TerrainGenerator.h"

#include <vector>

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

void WorldMesher::buildChunkMesh(Chunk& chunk, const ChunkManager& chunkManager, const TerrainGenerator& generator) const {
    chunk.mesh.clear();
    for (int y = 0; y < Chunk::SIZE_Y; ++y) {
        for (int lz = 0; lz < Chunk::SIZE_Z; ++lz) {
            for (int lx = 0; lx < Chunk::SIZE_X; ++lx) {
                // 如果是空氣，就不處理
                if (chunk.at(lx, y, lz) == 0) continue;
                
                int wx = chunk.coord.x * Chunk::SIZE_X + lx;
                int wz = chunk.coord.z * Chunk::SIZE_Z + lz;
                
                for (const auto& f : kFaces) {
                    // 隱藏面剔除：查詢相鄰位置是否為實體方塊
                    if (!chunkManager.hasBlockGlobal(wx + f.dx, y + f.dy, wz + f.dz, generator)) {
                        for (int i = 0; i < 6; ++i) {
                            pushVertex(chunk.mesh, wx + f.v[i][0], y + f.v[i][1], wz + f.v[i][2], f.nx, f.ny, f.nz, f.v[i][3], f.v[i][4]);
                        }
                    }
                }
            }
        }
    }
    chunk.dirty = false;
}