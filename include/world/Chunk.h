#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <cstdint>

struct ChunkCoord {
    int x = 0;
    int z = 0;

    bool operator==(const ChunkCoord& other) const {
        return x == other.x && z == other.z;
    }
};

struct ChunkCoordHash {
    std::size_t operator()(const ChunkCoord& c) const {
        const std::size_t h1 = std::hash<int>{}(c.x);
        const std::size_t h2 = std::hash<int>{}(c.z);
        return h1 ^ (h2 + 0x9e3779b9u + (h1 << 6u) + (h1 >> 2u));
    }
};

class Chunk {
public:
    static constexpr int SIZE_X = 16;
    static constexpr int SIZE_Y = 96;
    static constexpr int SIZE_Z = 16;

    ChunkCoord coord;
    std::vector<uint8_t> blocks;
    std::vector<float> mesh;

    bool dirty = true;     // 是否需要重新建立網格 (Meshing)
    bool modified = false; // 玩家是否修改過（決定是否需要存檔或防止卸載）
    int vertexCount = 0;

    unsigned int vao = 0;
    unsigned int vbo = 0;

    Chunk(int cx, int cz);
    ~Chunk();

    // 刪除拷貝構造，防止重複釋放 OpenGL 資源
    Chunk(const Chunk&) = delete;
    Chunk& operator=(const Chunk&) = delete;

    uint8_t& at(int lx, int y, int lz);
    uint8_t at(int lx, int y, int lz) const;

    void uploadMesh();
    void render() const;
    void clearOpenGL();
};