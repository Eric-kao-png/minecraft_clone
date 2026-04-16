#pragma once

#include <glm/glm.hpp>
#include "world/ChunkManager.h"
#include "world/TerrainGenerator.h"
#include "world/WorldMesher.h"
#include "world/WorldRaycaster.h" // 1. 引入新模組

class VoxelWorld {
public:
    // 2. 為了不破壞 main.cpp 的依賴，這裡設定型別別名
    using RaycastHit = WorldRaycaster::RaycastHit;

    VoxelWorld() = default;

    // --- 地形生成器相關 ---
    void setSeed(uint32_t seed) { generator_.setSeed(seed); }
    int sampleSurfaceY(int wx, int wz) const { return generator_.sampleHeight(wx, wz); }

    // --- 方塊讀寫介面 ---
    bool hasBlockGlobal(int wx, int wy, int wz) const { return chunkManager_.hasBlockGlobal(wx, wy, wz, generator_); }
    void setBlockGlobal(int wx, int wy, int wz, bool solid) { chunkManager_.setBlockGlobal(wx, wy, wz, solid, generator_); }

    // --- 更新與渲染 ---
    void updateStreaming(const glm::vec3& playerPos, int loadRadius, int unloadRadius);
    void render() const;
    void clear() { chunkManager_.clear(); }

    // --- 物理與互動 ---
    // 3. 委託給 WorldRaycaster 執行
    bool raycast(const glm::vec3& origin, const glm::vec3& dir, float maxDist, float step, RaycastHit& out) const {
        return WorldRaycaster::raycast(origin, dir, maxDist, step, out, chunkManager_, generator_);
    }

private:
    TerrainGenerator generator_;
    ChunkManager chunkManager_;
    WorldMesher mesher_;
};