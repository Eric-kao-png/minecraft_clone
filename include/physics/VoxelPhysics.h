#pragma once

#include <glm/glm.hpp>

#include <cmath>

namespace voxel_physics {

    static constexpr float EPS = 0.001f;

    struct AABB {
        glm::vec3 min{0.0f};
        glm::vec3 max{0.0f};
    };

    inline AABB makeAABB(const glm::vec3& center, const glm::vec3& half) {
        return AABB{center - half, center + half};
    }

    inline AABB blockAABB(int x, int y, int z) {
        const glm::vec3 bmin(static_cast<float>(x) - 0.5f,
                             static_cast<float>(y) - 0.5f,
                             static_cast<float>(z) - 0.5f);
        return AABB{bmin, bmin + glm::vec3(1.0f, 1.0f, 1.0f)};
    }

    inline bool overlap(const AABB& a, const AABB& b) {
        return (a.max.x > b.min.x && a.min.x < b.max.x) &&
               (a.max.y > b.min.y && a.min.y < b.max.y) &&
               (a.max.z > b.min.z && a.min.z < b.max.z);
    }

    inline int minCellFromWorld(float v) {
        return static_cast<int>(std::floor(v + 0.5f));
    }

    inline int maxCellFromWorld(float v) {
        return static_cast<int>(std::floor(v + 0.5f));
    }

} // namespace voxel_physics
