#include "world/VoxelWorld.h"

#include "world/WorldRaycaster.h"

void VoxelWorld::setBlockGlobal(int wx, int wy, int wz, uint8_t blockID) {
    chunkManager_.setBlockGlobal(wx, wy, wz, blockID, generator_);
}

void VoxelWorld::updateStreaming(const glm::vec3& playerPos, int loadRadius, int unloadRadius) {
    chunkManager_.updateStreaming(playerPos, loadRadius, unloadRadius, generator_);

    chunkManager_.forEachChunk([this](Chunk& chunk) {
        if (chunk.dirty) {
            mesher_.buildChunkMesh(chunk, chunkManager_, generator_);
            chunk.uploadMesh();
        }
    });
}

void VoxelWorld::render() const {
    chunkManager_.forEachChunk([](const Chunk& chunk) { chunk.render(); });
}

bool VoxelWorld::raycast(const glm::vec3& origin, const glm::vec3& dir, float maxDist, float step,
                         RaycastHit& out) const {
    return WorldRaycaster::raycast(origin, dir, maxDist, step, out, chunkManager_, generator_);
}
