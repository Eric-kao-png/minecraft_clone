#pragma once

#include <glm/glm.hpp>
#include "world/Chunk.h"
#include <unordered_map>
#include <memory>
#include <functional>

// 宣告 TerrainGenerator，因為 ChunkManager 在建立新區塊時需要它來生成地形
class TerrainGenerator; 

class ChunkManager {
public:
    ChunkManager() = default;
    ~ChunkManager() = default;

    // 清除所有區塊資源
    void clear();

    // 核心介面：全域方塊的讀寫
    bool hasBlockGlobal(int wx, int wy, int wz, const TerrainGenerator& generator) const;
    void setBlockGlobal(int wx, int wy, int wz, bool solid, const TerrainGenerator& generator);

    // 取得特定的 Chunk
    Chunk* getLoadedChunk(int cx, int cz);
    const Chunk* getLoadedChunk(int cx, int cz) const;
    Chunk& getOrCreateChunk(int cx, int cz, const TerrainGenerator& generator);

    // 更新區塊串流 (載入新區塊 / 卸載遠區塊)
    void updateStreaming(const glm::vec3& playerPos, int loadRadius, int unloadRadius, const TerrainGenerator& generator);

    // 標記區塊與其相鄰區塊需要重新建模
    void markChunkAndNeighborsDirty(int cx, int cz);

    // 允許外部遍歷所有載入的區塊 (例如用於渲染)
    void forEachChunk(const std::function<void(Chunk&)>& action);
    void forEachChunk(const std::function<void(const Chunk&)>& action) const;

    // 靜態輔助函式：座標轉換
    static int floorDiv(int a, int b);
    static int positiveMod(int a, int b);
    static int worldToBlock(float v);

private:
    std::unordered_map<ChunkCoord, std::unique_ptr<Chunk>, ChunkCoordHash> chunks_;

    void generateChunk(Chunk& chunk, const TerrainGenerator& generator) const;
};