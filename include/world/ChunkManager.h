#pragma once

#include "world/Chunk.h"

#include <glm/glm.hpp>

#include <cstdint>
#include <functional>
#include <memory>
#include <unordered_map>

class TerrainGenerator;

class ChunkManager {
public:
    ChunkManager() = default;
    ~ChunkManager() = default;

    void clear();

    bool hasBlockGlobal(int wx, int wy, int wz, const TerrainGenerator& generator) const;
    void setBlockGlobal(int wx, int wy, int wz, uint8_t blockID, const TerrainGenerator& generator);

    Chunk* getLoadedChunk(int cx, int cz);
    const Chunk* getLoadedChunk(int cx, int cz) const;
    Chunk& getOrCreateChunk(int cx, int cz, const TerrainGenerator& generator);

    void updateStreaming(const glm::vec3& playerPos, int loadRadius, int unloadRadius, const TerrainGenerator& generator);
    void markChunkAndNeighborsDirty(int cx, int cz);

    void forEachChunk(const std::function<void(Chunk&)>& action);
    void forEachChunk(const std::function<void(const Chunk&)>& action) const;

private:
    std::unordered_map<ChunkCoord, std::unique_ptr<Chunk>, ChunkCoordHash> chunks_;
    void generateChunk(Chunk& chunk, const TerrainGenerator& generator) const;
};