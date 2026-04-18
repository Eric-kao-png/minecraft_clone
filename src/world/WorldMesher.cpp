#include "world/WorldMesher.h"
#include "world/Chunk.h"
#include "world/ChunkManager.h"
#include "world/TerrainGenerator.h"

#include <vector>

namespace {
// (FaceDef 與 kFaces 結構保持不變)
struct FaceDef {
    int dx, dy, dz;
    float nx, ny, nz;
    float v[6][5];
};

constexpr FaceDef kFaces[6] = {
    {-0, 0,-1,  0, 0,-1, {{-0.5f,-0.5f,-0.5f, 0,0}, { 0.5f,-0.5f,-0.5f, 1,0}, { 0.5f, 0.5f,-0.5f, 1,1}, { 0.5f, 0.5f,-0.5f, 1,1}, {-0.5f, 0.5f,-0.5f, 0,1}, {-0.5f,-0.5f,-0.5f, 0,0}}},
    { 0, 0, 1,  0, 0, 1, {{-0.5f,-0.5f, 0.5f, 0,0}, { 0.5f,-0.5f, 0.5f, 1,0}, { 0.5f, 0.5f, 0.5f, 1,1}, { 0.5f, 0.5f, 0.5f, 1,1}, {-0.5f, 0.5f, 0.5f, 0,1}, {-0.5f,-0.5f, 0.5f, 0,0}}},
    {-1, 0, 0, -1, 0, 0, {{-0.5f, 0.5f, 0.5f, 1,0}, {-0.5f, 0.5f,-0.5f, 1,1}, {-0.5f,-0.5f,-0.5f, 0,1}, {-0.5f,-0.5f,-0.5f, 0,1}, {-0.5f,-0.5f, 0.5f, 0,0}, {-0.5f, 0.5f, 0.5f, 1,0}}},
    { 1, 0, 0,  1, 0, 0, {{ 0.5f, 0.5f, 0.5f, 1,0}, { 0.5f, 0.5f,-0.5f, 1,1}, { 0.5f,-0.5f,-0.5f, 0,1}, { 0.5f,-0.5f,-0.5f, 0,1}, { 0.5f,-0.5f, 0.5f, 0,0}, { 0.5f, 0.5f, 0.5f, 1,0}}},
    { 0,-1, 0,  0,-1, 0, {{-0.5f,-0.5f,-0.5f, 0,1}, { 0.5f,-0.5f,-0.5f, 1,1}, { 0.5f,-0.5f, 0.5f, 1,0}, { 0.5f,-0.5f, 0.5f, 1,0}, {-0.5f,-0.5f, 0.5f, 0,0}, {-0.5f,-0.5f,-0.5f, 0,1}}},
    { 0, 1, 0,  0, 1, 0, {{-0.5f, 0.5f,-0.5f, 0,1}, { 0.5f, 0.5f,-0.5f, 1,1}, { 0.5f, 0.5f, 0.5f, 1,0}, { 0.5f, 0.5f, 0.5f, 1,0}, {-0.5f, 0.5f, 0.5f, 0,0}, {-0.5f, 0.5f,-0.5f, 0,1}}}
};

static void pushVertex(std::vector<float>& out, float px, float py, float pz, float nx, float ny, float nz, float u, float v) {
    out.push_back(px); out.push_back(py); out.push_back(pz);
    out.push_back(nx); out.push_back(ny); out.push_back(nz);
    out.push_back(u);  out.push_back(v);
}
} // namespace

void WorldMesher::buildChunkMesh(Chunk& chunk, const ChunkManager& chunkManager, const TerrainGenerator& generator) const {
    chunk.mesh.clear();
    const float tileSize = 1.0f / 16.0f; // 16x16 Texture Atlas

    for (int y = 0; y < Chunk::SIZE_Y; ++y) {
        for (int lz = 0; lz < Chunk::SIZE_Z; ++lz) {
            for (int lx = 0; lx < Chunk::SIZE_X; ++lx) {
                uint8_t blockID = chunk.at(lx, y, lz);
                if (blockID == 0) continue; // 空氣不處理

                // [新增] 根據 Block ID 映射到 Atlas 上的索引 (0~255)
                int atlasIndex = 0;
                switch(blockID) {
                    case 1: atlasIndex = 1; break;  // 石頭 (Row 0, Col 1)
                    case 2: atlasIndex = 2; break;  // 泥土 (Row 0, Col 2)
                    case 3: atlasIndex = 204; break;  // 草地側面 (Row 12, Col 12)
                    case 4: atlasIndex = 16; break; // 鵝卵石 (Row 1, Col 0)
                    default: atlasIndex = 1; break; // 找不到就預設顯示石頭
                }

                // 計算該格子左下角的 UV 座標
                int col = atlasIndex % 16;
                int row = atlasIndex / 16;
                float u0 = col * tileSize;
                float v0 = 1.0f - (row + 1) * tileSize; // OpenGL 的 V 軸是朝上的

                int wx = chunk.coord.x * Chunk::SIZE_X + lx;
                int wz = chunk.coord.z * Chunk::SIZE_Z + lz;

                for (const auto& f : kFaces) {
                    if (!chunkManager.hasBlockGlobal(wx + f.dx, y + f.dy, wz + f.dz, generator)) {
                        for (int i = 0; i < 6; ++i) {
                            // [修改] 將原本 0~1 的 UV 縮小到 1/16，並加上該方塊專屬的偏移量
                            float u = u0 + f.v[i][3] * tileSize;
                            float v = v0 + f.v[i][4] * tileSize;
                            pushVertex(chunk.mesh, wx + f.v[i][0], y + f.v[i][1], wz + f.v[i][2], f.nx, f.ny, f.nz, u, v);
                        }
                    }
                }
            }
        }
    }
    chunk.dirty = false;
}