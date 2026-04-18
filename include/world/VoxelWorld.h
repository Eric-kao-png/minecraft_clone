#pragma once

#include <glm/glm.hpp>
#include "world/ChunkManager.h"
#include "world/TerrainGenerator.h"
#include "world/WorldMesher.h"
#include "world/WorldRaycaster.h"

class VoxelWorld {
public:
    using RaycastHit = WorldRaycaster::RaycastHit;

    VoxelWorld() = default;

    void setSeed(uint32_t seed) { generator_.setSeed(seed); }
    int sampleSurfaceY(int wx, int wz) const { return generator_.sampleHeight(wx, wz); }

    bool hasBlockGlobal(int wx, int wy, int wz) const { return chunkManager_.hasBlockGlobal(wx, wy, wz, generator_); }
    // [修改] 將 bool solid 改為 uint8_t blockID
    void setBlockGlobal(int wx, int wy, int wz, uint8_t blockID) { chunkManager_.setBlockGlobal(wx, wy, wz, blockID, generator_); }

    void updateStreaming(const glm::vec3& playerPos, int loadRadius, int unloadRadius);
    void render() const;
    void clear() { chunkManager_.clear(); }

    bool raycast(const glm::vec3& origin, const glm::vec3& dir, float maxDist, float step, RaycastHit& out) const {
        return WorldRaycaster::raycast(origin, dir, maxDist, step, out, chunkManager_, generator_);
    }

private:
    TerrainGenerator generator_;
    ChunkManager chunkManager_;
    WorldMesher mesher_;
};