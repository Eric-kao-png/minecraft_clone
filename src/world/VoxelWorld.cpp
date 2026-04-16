#include "world/VoxelWorld.h"
#include <cmath>

void VoxelWorld::updateStreaming(const glm::vec3& playerPos, int loadRadius, int unloadRadius) {
    // 1. 委託 ChunkManager 處理區塊的載入與卸載
    chunkManager_.updateStreaming(playerPos, loadRadius, unloadRadius, generator_);

    // 2. 處理需要重新建模的區塊 (交給 WorldMesher)
    chunkManager_.forEachChunk([this](Chunk& chunk) {
        if (chunk.dirty) {
            // 呼叫 mesher_ 並傳入所需的環境資訊
            mesher_.buildChunkMesh(chunk, chunkManager_, generator_);
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