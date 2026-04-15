#pragma once

#include <glm/glm.hpp>

#include <cstdint>
#include <memory>
#include <unordered_map>
#include <vector>

class VoxelWorld {
public:
    static constexpr int CHUNK_SIZE_X = 16;
    static constexpr int CHUNK_SIZE_Y = 96;   // vertical build height is still fixed
    static constexpr int CHUNK_SIZE_Z = 16;

    struct RaycastHit {
        bool hit = false;
        glm::ivec3 block{0};
        glm::ivec3 normal{0};
        float t = 0.0f;
    };

    VoxelWorld() = default;

    void setSeed(uint32_t seed) { seed_ = seed; }

    bool hasBlockGlobal(int wx, int wy, int wz) const;
    void setBlockGlobal(int wx, int wy, int wz, bool solid);

    int sampleSurfaceY(int wx, int wz) const;

    void updateStreaming(const glm::vec3& playerPos, int loadRadius, int unloadRadius);
    void render() const;
    void clear();

    bool raycast(const glm::vec3& origin,
                 const glm::vec3& dir,
                 float maxDist,
                 float step,
                 RaycastHit& out) const;

    int loadedChunkCount() const { return static_cast<int>(chunks_.size()); }

private:
    struct ChunkCoord {
        int x = 0;
        int z = 0;

        bool operator==(const ChunkCoord& other) const {
            return x == other.x && z == other.z;
        }
    };

    struct ChunkCoordHash {
        std::size_t operator()(const ChunkCoord& c) const;
    };

    struct Chunk {
        ChunkCoord coord;
        std::vector<uint8_t> blocks;
        std::vector<float> mesh;

        bool dirty = true;
        bool modified = false;
        int vertexCount = 0;

        unsigned int vao = 0;
        unsigned int vbo = 0;

        Chunk(int cx, int cz);

        uint8_t& at(int lx, int y, int lz);
        uint8_t at(int lx, int y, int lz) const;
    };

private:
    uint32_t seed_ = 2026;
    int baseHeight_ = 36;
    double amplitude_ = 18.0;
    double noiseScale_ = 0.045;
    int octaves_ = 4;
    double lacunarity_ = 2.0;
    double gain_ = 0.5;

    std::unordered_map<ChunkCoord, std::unique_ptr<Chunk>, ChunkCoordHash> chunks_;

    static int floorDiv(int a, int b);
    static int positiveMod(int a, int b);
    static int worldToBlock(float v);

    static int worldToChunkX(int wx) { return floorDiv(wx, CHUNK_SIZE_X); }
    static int worldToChunkZ(int wz) { return floorDiv(wz, CHUNK_SIZE_Z); }
    static int worldToLocalX(int wx) { return positiveMod(wx, CHUNK_SIZE_X); }
    static int worldToLocalZ(int wz) { return positiveMod(wz, CHUNK_SIZE_Z); }

    Chunk* getLoadedChunk(int cx, int cz);
    const Chunk* getLoadedChunk(int cx, int cz) const;
    Chunk& getOrCreateChunk(int cx, int cz);

    bool sampleGeneratedBlock(int wx, int wy, int wz) const;

    void generateChunk(Chunk& chunk) const;
    void buildChunkMesh(Chunk& chunk) const;
    void uploadChunkMesh(Chunk& chunk) const;
    void markChunkAndNeighborsDirty(int cx, int cz);
};
