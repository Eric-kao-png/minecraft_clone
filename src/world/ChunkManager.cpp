#include "world/ChunkManager.h"
#include "world/TerrainGenerator.h"
#include <cmath>
#include <algorithm>

int ChunkManager::floorDiv(int a, int b) {
    int q = a / b;
    int r = a % b;
    if (r != 0 && ((r > 0) != (b > 0))) --q;
    return q;
}

int ChunkManager::positiveMod(int a, int b) {
    int m = a % b;
    if (m < 0) m += (b < 0 ? -b : b);
    return m;
}

int ChunkManager::worldToBlock(float v) {
    return static_cast<int>(std::floor(v + 0.5f));
}

void ChunkManager::clear() {
    chunks_.clear();
}

Chunk* ChunkManager::getLoadedChunk(int cx, int cz) {
    auto it = chunks_.find(ChunkCoord{cx, cz});
    return (it == chunks_.end()) ? nullptr : it->second.get();
}

const Chunk* ChunkManager::getLoadedChunk(int cx, int cz) const {
    auto it = chunks_.find(ChunkCoord{cx, cz});
    return (it == chunks_.end()) ? nullptr : it->second.get();
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
                int wx = chunk.coord.x * Chunk::SIZE_X + lx;
                int wz = chunk.coord.z * Chunk::SIZE_Z + lz;
                chunk.at(lx, y, lz) = (y <= generator.sampleHeight(wx, wz)) ? 1u : 0u;
            }
        }
    }
    chunk.dirty = true;
}

bool ChunkManager::hasBlockGlobal(int wx, int wy, int wz, const TerrainGenerator& generator) const {
    if (wy < 0 || wy >= Chunk::SIZE_Y) return false;
    if (const Chunk* chunk = getLoadedChunk(floorDiv(wx, Chunk::SIZE_X), floorDiv(wz, Chunk::SIZE_Z))) {
        return chunk->at(positiveMod(wx, Chunk::SIZE_X), wy, positiveMod(wz, Chunk::SIZE_Z)) != 0;
    }
    return wy <= generator.sampleHeight(wx, wz);
}

void ChunkManager::setBlockGlobal(int wx, int wy, int wz, bool solid, const TerrainGenerator& generator) {
    if (wy < 0 || wy >= Chunk::SIZE_Y) return;
    int cx = floorDiv(wx, Chunk::SIZE_X), cz = floorDiv(wz, Chunk::SIZE_Z);
    Chunk& chunk = getOrCreateChunk(cx, cz, generator);
    
    uint8_t newVal = solid ? 1u : 0u;
    int lx = positiveMod(wx, Chunk::SIZE_X), lz = positiveMod(wz, Chunk::SIZE_Z);
    
    if (chunk.at(lx, wy, lz) == newVal) return;
    chunk.at(lx, wy, lz) = newVal;
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

void ChunkManager::updateStreaming(const glm::vec3& playerPos, int loadRadius, int unloadRadius, const TerrainGenerator& generator) {
    int centerCX = floorDiv(worldToBlock(playerPos.x), Chunk::SIZE_X);
    int centerCZ = floorDiv(worldToBlock(playerPos.z), Chunk::SIZE_Z);
    
    // 1. 載入視野內的區塊
    for (int dz = -loadRadius; dz <= loadRadius; ++dz) {
        for (int dx = -loadRadius; dx <= loadRadius; ++dx) {
            getOrCreateChunk(centerCX + dx, centerCZ + dz, generator);
        }
    }

    // 2. 卸載遠處未修改的區塊
    for (auto it = chunks_.begin(); it != chunks_.end(); ) {
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
        action(*chunkPtr);
    }
}

void ChunkManager::forEachChunk(const std::function<void(const Chunk&)>& action) const {
    for (const auto& [coord, chunkPtr] : chunks_) {
        action(*chunkPtr);
    }
}