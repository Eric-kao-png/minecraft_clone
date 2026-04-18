#pragma once

#include <glm/glm.hpp>
#include "world/Chunk.h"
#include <unordered_map>
#include <memory>
#include <functional>

class TerrainGenerator;

class ChunkManager {
public:
    ChunkManager() = default;
    ~ChunkManager() = default;

    void clear();

    bool hasBlockGlobal(int wx, int wy, int wz, const TerrainGenerator& generator) const;
    // [修改] 將 bool solid 改為 uint8_t blockID
    void setBlockGlobal(int wx, int wy, int wz, uint8_t blockID, const TerrainGenerator& generator);

    Chunk* getLoadedChunk(int cx, int cz);
    const Chunk* getLoadedChunk(int cx, int cz) const;
    Chunk& getOrCreateChunk(int cx, int cz, const TerrainGenerator& generator);

    void updateStreaming(const glm::vec3& playerPos, int loadRadius, int unloadRadius, const TerrainGenerator& generator);
    void markChunkAndNeighborsDirty(int cx, int cz);

    void forEachChunk(const std::function<void(Chunk&)>& action);
    void forEachChunk(const std::function<void(const Chunk&)>& action) const;

    static int floorDiv(int a, int b);
    static int positiveMod(int a, int b);
    static int worldToBlock(float v);

private:
    std::unordered_map<ChunkCoord, std::unique_ptr<Chunk>, ChunkCoordHash> chunks_;
    void generateChunk(Chunk& chunk, const TerrainGenerator& generator) const;
};