#pragma once

#include <glm/glm.hpp>

#include <algorithm>
#include <cmath>

#include "world/VoxelWorld.h"

namespace voxel_physics {

// World is rendered centered at origin:
// block center = (x - CX, y - CY, z - CZ), with each cube spanning +/- 0.5.
static constexpr float CX = (VoxelWorld::SX - 1) * 0.5f;
static constexpr float CY = (VoxelWorld::SY - 1) * 0.5f;
static constexpr float CZ = (VoxelWorld::SZ - 1) * 0.5f;

static constexpr float WORLD_MIN_X = -CX - 0.5f;
static constexpr float WORLD_MAX_X =  CX + 0.5f;
static constexpr float WORLD_MIN_Y = -CY - 0.5f;
static constexpr float WORLD_MAX_Y =  CY + 0.5f;
static constexpr float WORLD_MIN_Z = -CZ - 0.5f;
static constexpr float WORLD_MAX_Z =  CZ + 0.5f;

static constexpr float EPS = 0.001f;

struct AABB {
    glm::vec3 min{0.0f};
    glm::vec3 max{0.0f};
};

inline AABB makeAABB(const glm::vec3& center, const glm::vec3& half) {
    return AABB{ center - half, center + half };
}

inline AABB blockAABB(int x, int y, int z) {
    glm::vec3 bmin((float)x - CX - 0.5f,
                  (float)y - CY - 0.5f,
                  (float)z - CZ - 0.5f);
    return AABB{ bmin, bmin + glm::vec3(1.0f) };
}

inline bool overlap(const AABB& a, const AABB& b) {
    // strict overlap (touching faces is not considered penetration)
    return (a.max.x > b.min.x && a.min.x < b.max.x) &&
           (a.max.y > b.min.y && a.min.y < b.max.y) &&
           (a.max.z > b.min.z && a.min.z < b.max.z);
}

inline glm::ivec3 worldPosToCell(const glm::vec3& p) {
    // Find cell whose cube contains point p.
    // Derived from: cell center = (i - CX), cube spans [center-0.5, center+0.5)
    return glm::ivec3(
        (int)std::floor(p.x + CX + 0.5f),
        (int)std::floor(p.y + CY + 0.5f),
        (int)std::floor(p.z + CZ + 0.5f)
    );
}

inline int minCellFromWorld(float v, float C) {
    return (int)std::floor(v + C + 0.5f);
}

inline int maxCellFromWorld(float v, float C) {
    return (int)std::floor(v + C + 0.5f);
}

inline float clampf(float v, float lo, float hi) {
    return std::max(lo, std::min(hi, v));
}

} // namespace voxel_physics