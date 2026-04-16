#pragma once

#include <glm/glm.hpp>
#include "world/Chunk.h"
#include "world/TerrainGenerator.h"
#include <unordered_map>
#include <memory>

class VoxelWorld {
public:
    struct RaycastHit {
        bool hit = false;
        glm::ivec3 block{0};
        glm::ivec3 normal{0};
        float t = 0.0f;
    };

    VoxelWorld() = default;

    void setSeed(uint32_t seed); // 實作將改為呼叫 generator_
    bool hasBlockGlobal(int wx, int wy, int wz) const;
    void setBlockGlobal(int wx, int wy, int wz, bool solid);
    int sampleSurfaceY(int wx, int wz) const;

    void updateStreaming(const glm::vec3& playerPos, int loadRadius, int unloadRadius);
    void render() const;
    void clear();

    bool raycast(const glm::vec3& origin, const glm::vec3& dir, float maxDist, float step, RaycastHit& out) const;

private:
    TerrainGenerator generator_;

    std::unordered_map<ChunkCoord, std::unique_ptr<Chunk>, ChunkCoordHash> chunks_;
    static int floorDiv(int a, int b);
    static int positiveMod(int a, int b);
    static int worldToBlock(float v);

    Chunk* getLoadedChunk(int cx, int cz);
    const Chunk* getLoadedChunk(int cx, int cz) const;
    Chunk& getOrCreateChunk(int cx, int cz);

    void generateChunk(Chunk& chunk) const;
    void buildChunkMesh(Chunk& chunk) const;
    void markChunkAndNeighborsDirty(int cx, int cz);
};