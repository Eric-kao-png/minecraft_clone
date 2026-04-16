#pragma once

// 前置宣告，減少標頭檔依賴
class Chunk;
class ChunkManager;
class TerrainGenerator;

class WorldMesher {
public:
    WorldMesher() = default;
    ~WorldMesher() = default;

    // 核心介面：讀取 Chunk 資料，並根據 ChunkManager 的全域資訊進行隱藏面剔除，最後生成網格
    void buildChunkMesh(Chunk& chunk, const ChunkManager& chunkManager, const TerrainGenerator& generator) const;
};