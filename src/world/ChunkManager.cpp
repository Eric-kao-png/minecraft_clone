#include "world/ChunkManager.h"

#include "world/BlockId.h"
#include "world/TerrainGenerator.h"
#include "world/WorldCoord.h"

#include <algorithm>
#include <cmath>

Chunk* ChunkManager::getLoadedChunk(int cx, int cz) {
    auto it = chunks_.find(ChunkCoord{cx, cz});
    return (it == chunks_.end()) ? nullptr : it->second.get();
}

const Chunk* ChunkManager::getLoadedChunk(int cx, int cz) const {
    auto it = chunks_.find(ChunkCoord{cx, cz});
    return (it == chunks_.end()) ? nullptr : it->second.get();
}

void ChunkManager::clear() {
    chunks_.clear();
}

Chunk& ChunkManager::getOrCreateChunk(int cx, int cz, const TerrainGenerator& generator) {
    auto [it, inserted] = chunks_.try_emplace(ChunkCoord{cx, cz}, nullptr);
    if (inserted) {
        it->second = std::make_unique<Chunk>(cx, cz);
        generateChunk(*it->second, generator);
    }
    return *it->second;
}

void ChunkManager::generateChunk(Chunk& chunk, const TerrainGenerator& generator) const {
    for (int y = 0; y < Chunk::SIZE_Y; ++y) {
        for (int lz = 0; lz < Chunk::SIZE_Z; ++lz) {
            for (int lx = 0; lx < Chunk::SIZE_X; ++lx) {
                const int wx = chunk.coord.x * Chunk::SIZE_X + lx;
                const int wz = chunk.coord.z * Chunk::SIZE_Z + lz;
                const int surfaceH = generator.sampleHeight(wx, wz);

                uint8_t id = blocks::kAir;
                if (y < surfaceH - 3) {
                    id = blocks::kStone;
                } else if (y < surfaceH) {
                    id = blocks::kDirt;
                } else if (y == surfaceH) {
                    id = blocks::kGrass;
                }

                chunk.at(lx, y, lz) = id;
            }
        }
    }
    chunk.dirty = true;
}

bool ChunkManager::hasBlockGlobal(int wx, int wy, int wz, const TerrainGenerator& generator) const {
    if (wy < 0 || wy >= Chunk::SIZE_Y) {
        return false;
    }
    if (const Chunk* chunk =
            getLoadedChunk(WorldCoord::floorDiv(wx, Chunk::SIZE_X), WorldCoord::floorDiv(wz, Chunk::SIZE_Z))) {
        return blocks::isSolid(
            chunk->at(WorldCoord::positiveMod(wx, Chunk::SIZE_X), wy, WorldCoord::positiveMod(wz, Chunk::SIZE_Z)));
    }
    return wy <= generator.sampleHeight(wx, wz);
}

void ChunkManager::setBlockGlobal(int wx, int wy, int wz, uint8_t blockID, const TerrainGenerator& generator) {
    if (wy < 0 || wy >= Chunk::SIZE_Y) {
        return;
    }

    const int cx = WorldCoord::floorDiv(wx, Chunk::SIZE_X);
    const int cz = WorldCoord::floorDiv(wz, Chunk::SIZE_Z);
    Chunk& chunk = getOrCreateChunk(cx, cz, generator);

    const int lx = WorldCoord::positiveMod(wx, Chunk::SIZE_X);
    const int lz = WorldCoord::positiveMod(wz, Chunk::SIZE_Z);

    if (chunk.at(lx, wy, lz) == blockID) {
        return;
    }

    chunk.at(lx, wy, lz) = blockID;
    chunk.modified = true;
    markChunkAndNeighborsDirty(cx, cz);
}

void ChunkManager::markChunkAndNeighborsDirty(int cx, int cz) {
    for (int dz = -1; dz <= 1; ++dz) {
        for (int dx = -1; dx <= 1; ++dx) {
            if (Chunk* c = getLoadedChunk(cx + dx, cz + dz)) {
                c->dirty = true;
            }
        }
    }
}

void ChunkManager::updateStreaming(const glm::vec3& playerPos, int loadRadius, int unloadRadius,
                                   const TerrainGenerator& generator) {
    const int centerCX = WorldCoord::floorDiv(WorldCoord::worldToBlock(playerPos.x), Chunk::SIZE_X);
    const int centerCZ = WorldCoord::floorDiv(WorldCoord::worldToBlock(playerPos.z), Chunk::SIZE_Z);

    for (int dz = -loadRadius; dz <= loadRadius; ++dz) {
        for (int dx = -loadRadius; dx <= loadRadius; ++dx) {
            getOrCreateChunk(centerCX + dx, centerCZ + dz, generator);
        }
    }

    for (auto it = chunks_.begin(); it != chunks_.end();) {
        if (std::abs(it->first.x - centerCX) > unloadRadius || std::abs(it->first.z - centerCZ) > unloadRadius) {
            if (!it->second->modified) {
                it = chunks_.erase(it);
                continue;
            }
        }
        ++it;
    }
}

void ChunkManager::forEachChunk(const std::function<void(Chunk&)>& action) {
    for (auto& [coord, chunkPtr] : chunks_) {
        (void)coord;
        action(*chunkPtr);
    }
}

void ChunkManager::forEachChunk(const std::function<void(const Chunk&)>& action) const {
    for (const auto& [coord, chunkPtr] : chunks_) {
        (void)coord;
        action(*chunkPtr);
    }
}
