#include "world/WorldRaycaster.h"
#include "world/ChunkManager.h"
#include "world/TerrainGenerator.h"
#include <cmath>

bool WorldRaycaster::raycast(const glm::vec3& origin, const glm::vec3& dir, float maxDist, float step, 
                             RaycastHit& out, const ChunkManager& chunkManager, const TerrainGenerator& generator) {
    glm::vec3 nDir = glm::normalize(dir);
    for (float t = 0; t < maxDist; t += step) {
        glm::vec3 p = origin + nDir * t;
        glm::ivec3 cell(std::floor(p.x + 0.5f), std::floor(p.y + 0.5f), std::floor(p.z + 0.5f));
        
        // 透過 chunkManager 查詢全域方塊狀態
        if (chunkManager.hasBlockGlobal(cell.x, cell.y, cell.z, generator)) {
            out.hit = true; 
            out.block = cell; 
            out.t = t;
            glm::vec3 prevP = origin + nDir * (t - step);
            out.normal = glm::ivec3(std::floor(prevP.x + 0.5f), std::floor(prevP.y + 0.5f), std::floor(prevP.z + 0.5f)) - cell;
            return true;
        }
    }
    return false;
}