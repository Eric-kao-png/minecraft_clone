#include "world/WorldMesher.h"
#include "world/Chunk.h"
#include "world/ChunkManager.h"
#include "world/TerrainGenerator.h"

#include <vector>

namespace {
// 這裡定義了每個面的頂點結構：dx/dy/dz 用於隱藏面剔除，nx/ny/nz 是法向量，v[6][5] 是 6 個頂點的 (x,y,z, u,v)
struct FaceDef {
    int dx, dy, dz;
    float nx, ny, nz;
    float v[6][5];
};

// ======================================================================================
// [修正重點]：調整了 kFaces 中四個側面的 UV 座標（f.v[i][3] 與 f.v[i][4]），
// 以確保紋理在所有側面上都能正立顯示，且不會出現相對面旋轉方向相反的問題。
// ======================================================================================
constexpr FaceDef kFaces[6] = {
    // 0: 前面 (Front, Z-)
    {-0, 0,-1,  0, 0,-1, {{-0.5f,-0.5f,-0.5f, 0,0}, { 0.5f,-0.5f,-0.5f, 1,0}, { 0.5f, 0.5f,-0.5f, 1,1}, { 0.5f, 0.5f,-0.5f, 1,1}, {-0.5f, 0.5f,-0.5f, 0,1}, {-0.5f,-0.5f,-0.5f, 0,0}}},

    // 1: 後面 (Back, Z+)
    { 0, 0, 1,  0, 0, 1, {{ 0.5f,-0.5f, 0.5f, 0,0}, {-0.5f,-0.5f, 0.5f, 1,0}, {-0.5f, 0.5f, 0.5f, 1,1}, {-0.5f, 0.5f, 0.5f, 1,1}, { 0.5f, 0.5f, 0.5f, 0,1}, { 0.5f,-0.5f, 0.5f, 0,0}}},

    // 2: 左面 (Left, X-) - 已修正 UV
    {-1, 0, 0, -1, 0, 0, {{-0.5f,-0.5f, 0.5f, 0,0}, {-0.5f,-0.5f,-0.5f, 1,0}, {-0.5f, 0.5f,-0.5f, 1,1}, {-0.5f, 0.5f,-0.5f, 1,1}, {-0.5f, 0.5f, 0.5f, 0,1}, {-0.5f,-0.5f, 0.5f, 0,0}}},

    // 3: 右面 (Right, X+) - 已修正 UV
    { 1, 0, 0,  1, 0, 0, {{ 0.5f,-0.5f,-0.5f, 0,0}, { 0.5f,-0.5f, 0.5f, 1,0}, { 0.5f, 0.5f, 0.5f, 1,1}, { 0.5f, 0.5f, 0.5f, 1,1}, { 0.5f, 0.5f,-0.5f, 0,1}, { 0.5f,-0.5f,-0.5f, 0,0}}},

    // 4: 下面 (Bottom, Y-)
    { 0,-1, 0,  0,-1, 0, {{-0.5f,-0.5f,-0.5f, 0,1}, { 0.5f,-0.5f,-0.5f, 1,1}, { 0.5f,-0.5f, 0.5f, 1,0}, { 0.5f,-0.5f, 0.5f, 1,0}, {-0.5f,-0.5f, 0.5f, 0,0}, {-0.5f,-0.5f,-0.5f, 0,1}}},

    // 5: 上面 (Top, Y+)
    { 0, 1, 0,  0, 1, 0, {{-0.5f, 0.5f,-0.5f, 0,1}, { 0.5f, 0.5f,-0.5f, 1,1}, { 0.5f, 0.5f, 0.5f, 1,0}, { 0.5f, 0.5f, 0.5f, 1,0}, {-0.5f, 0.5f, 0.5f, 0,0}, {-0.5f, 0.5f,-0.5f, 0,1}}}
};

static void pushVertex(std::vector<float>& out, float px, float py, float pz, float nx, float ny, float nz, float u, float v) {
    out.push_back(px); out.push_back(py); out.push_back(pz); // 位置
    out.push_back(nx); out.push_back(ny); out.push_back(nz); // 法線
    out.push_back(u);  out.push_back(v);                     // UV
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

                int wx = chunk.coord.x * Chunk::SIZE_X + lx;
                int wz = chunk.coord.z * Chunk::SIZE_Z + lz;

                // 遍歷六個面
                for (const auto& f : kFaces) {
                    // 執行隱藏面剔除：只有當相鄰方塊不存在時才繪製該面
                    if (!chunkManager.hasBlockGlobal(wx + f.dx, y + f.dy, wz + f.dz, generator)) {

                        // 根據 Block ID 與面的方向 (ny) 決定材質索引
                        int atlasIndex = 0;

                        if (blockID == 3) {
                            // 處理草地 (Block ID 3) 的多重材質
                            if (f.ny == 1.0f) {
                                atlasIndex = 204; // 上面：草地 (12, 12)
                            } else if (f.ny == -1.0f) {
                                atlasIndex = 2;   // 下面：泥土 (0, 2)
                            } else {
                                atlasIndex = 3;   // 側面：草地側面 (0, 3)
                            }
                        } else {
                            // 處理其它六面材質相同的方塊
                            switch(blockID) {
                                case 1: atlasIndex = 1; break;  // 石頭 (0, 1)
                                case 2: atlasIndex = 2; break;  // 泥土 (0, 2)
                                case 4: atlasIndex = 16; break; // 鵝卵石 (1, 0)
                                default: atlasIndex = 1; break; // 預設顯示石頭
                            }
                        }

                        // 計算該方塊在 Atlas 上的 UV 偏移量
                        int col = atlasIndex % 16;
                        int row = atlasIndex / 16;
                        float u0 = col * tileSize;
                        float v0 = 1.0f - (row + 1) * tileSize; // OpenGL 的 V 軸是朝上的

                        // 生成 6 個頂點（組成 2 個三角形）並推入 mesh
                        for (int i = 0; i < 6; ++i) {
                            // 將原始 0~1 的 UV 座標縮小到 1/16 大小，並加上該方塊的偏移量
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