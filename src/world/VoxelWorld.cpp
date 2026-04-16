#include "world/VoxelWorld.h"

void VoxelWorld::updateStreaming(const glm::vec3& playerPos, int loadRadius, int unloadRadius) {
    // 1. 委託 ChunkManager 處理區塊的載入與卸載
    chunkManager_.updateStreaming(playerPos, loadRadius, unloadRadius, generator_);

    // 2. 處理需要重新建模的區塊 (交給 WorldMesher)
    chunkManager_.forEachChunk([this](Chunk& chunk) {
        if (chunk.dirty) {
            mesher_.buildChunkMesh(chunk, chunkManager_, generator_);
            chunk.uploadMesh();
        }
    });
}

void VoxelWorld::render() const {
    // 3. 委託 ChunkManager 遍歷渲染
    chunkManager_.forEachChunk([](const Chunk& chunk) {
        chunk.render();
    });
}