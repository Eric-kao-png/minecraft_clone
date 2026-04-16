#pragma once

#include <glm/glm.hpp>

// 前置宣告
class ChunkManager;
class TerrainGenerator;

class WorldRaycaster {
public:
    // 將原本在 VoxelWorld 裡的結構體搬過來
    struct RaycastHit {
        bool hit = false;
        glm::ivec3 block{0};
        glm::ivec3 normal{0};
        float t = 0.0f;
    };

    // 核心介面：需要傳入 ChunkManager 與 Generator 才能查詢世界狀態
    static bool raycast(const glm::vec3& origin, const glm::vec3& dir, float maxDist, float step, 
                        RaycastHit& out, const ChunkManager& chunkManager, const TerrainGenerator& generator);
};