#pragma once

#include <glm/glm.hpp>
#include "world/ChunkManager.h"
#include "world/TerrainGenerator.h"
#include "world/WorldMesher.h" // 引入新模組

class VoxelWorld {
public:
    struct RaycastHit {
        bool hit = false;
        glm::ivec3 block{0};
        glm::ivec3 normal{0};
        float t = 0.0f;
    };

    VoxelWorld() = default;

    // 地形生成器相關
    void setSeed(uint32_t seed) { generator_.setSeed(seed); }
    int sampleSurfaceY(int wx, int wz) const { return generator_.sampleHeight(wx, wz); }

    // 方塊讀寫介面 (委託給 ChunkManager)
    bool hasBlockGlobal(int wx, int wy, int wz) const { return chunkManager_.hasBlockGlobal(wx, wy, wz, generator_); }
    void setBlockGlobal(int wx, int wy, int wz, bool solid) { chunkManager_.setBlockGlobal(wx, wy, wz, solid, generator_); }

    // 更新與渲染
    void updateStreaming(const glm::vec3& playerPos, int loadRadius, int unloadRadius);
    void render() const;
    void clear() { chunkManager_.clear(); }

    // 物理與互動
    bool raycast(const glm::vec3& origin, const glm::vec3& dir, float maxDist, float step, RaycastHit& out) const;

private:
    TerrainGenerator generator_;
    ChunkManager chunkManager_;
    WorldMesher mesher_; // 專屬建模師
};