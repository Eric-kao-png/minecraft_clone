#pragma once

#include <glm/glm.hpp>

#include <algorithm>
#include <cmath>

#include "physics/VoxelPhysics.h"

// A generic physics body that collides against the voxel world using AABB.
// - Uses axis-separated movement (X -> Y -> Z)
// - Resolves penetration by clamping the body's center to block boundaries
// - Designed for Minecraft-like "player capsule replaced by AABB" collisions
class PhysicsBody {
public:
    virtual ~PhysicsBody() = default;

    glm::vec3 position{0.0f}; // AABB center, in render-space (same space as your mesh)
    glm::vec3 velocity{0.0f};

    // Player-like defaults: width ~0.6, height ~1.8
    glm::vec3 halfSize{0.30f, 0.90f, 0.30f};

    bool onGround = false;

    float gravity = -25.0f;           // units/s^2 (negative = down)
    float terminalVelocity = -70.0f;  // clamp falling speed

    voxel_physics::AABB aabb() const {
        return voxel_physics::makeAABB(position, halfSize);
    }

    // Step physics with sub-stepping for stability.
    void step(const VoxelWorld& world, float dt) {
        if (dt <= 0.0f) return;

        // Prevent huge dt spikes from tunneling too much.
        dt = std::min(dt, 0.05f);

        const float maxSubStep = 1.0f / 120.0f;
        int steps = (int)std::ceil(dt / maxSubStep);
        steps = std::max(1, steps);
        float subDt = dt / (float)steps;

        onGround = false;

        for (int i = 0; i < steps; ++i) {
            // Integrate gravity
            velocity.y += gravity * subDt;
            if (velocity.y < terminalVelocity)
                velocity.y = terminalVelocity;

            glm::vec3 delta = velocity * subDt;

            moveAxis(world, 0, delta.x);
            moveAxis(world, 1, delta.y);
            moveAxis(world, 2, delta.z);
        }
    }

protected:
    // Override points for subclasses (optional)
    virtual void onCollideDown() {}
    virtual void onCollideUp() {}
    virtual void onCollideSide() {}

private:
    static bool isSolidCell(const VoxelWorld& world, int x, int y, int z) {
        return world.inBounds(x, y, z) && world.hasBlock(x, y, z);
    }

    void clampToWorldBounds(int axis) {
        using namespace voxel_physics;
        float lo = 0.0f, hi = 0.0f;
        if (axis == 0) {
            lo = WORLD_MIN_X + halfSize.x;
            hi = WORLD_MAX_X - halfSize.x;
        } else if (axis == 1) {
            lo = WORLD_MIN_Y + halfSize.y;
            hi = WORLD_MAX_Y - halfSize.y;
        } else {
            lo = WORLD_MIN_Z + halfSize.z;
            hi = WORLD_MAX_Z - halfSize.z;
        }

        float& p = position[axis];
        if (p < lo) {
            p = lo;
            velocity[axis] = 0.0f;
            if (axis == 1) {
                onGround = true;
                onCollideDown();
            } else {
                onCollideSide();
            }
        } else if (p > hi) {
            p = hi;
            velocity[axis] = 0.0f;
            if (axis == 1) {
                onCollideUp();
            } else {
                onCollideSide();
            }
        }
    }

    void moveAxis(const VoxelWorld& world, int axis, float delta) {
        if (delta == 0.0f) {
            clampToWorldBounds(axis);
            return;
        }

        position[axis] += delta;

        // Broadphase: compute which voxel cells the AABB overlaps
        using namespace voxel_physics;
        voxel_physics::AABB box = aabb();

        int x0 = minCellFromWorld(box.min.x, CX);
        int x1 = maxCellFromWorld(box.max.x, CX);
        int y0 = minCellFromWorld(box.min.y, CY);
        int y1 = maxCellFromWorld(box.max.y, CY);
        int z0 = minCellFromWorld(box.min.z, CZ);
        int z1 = maxCellFromWorld(box.max.z, CZ);

        x0 = std::clamp(x0, 0, VoxelWorld::SX - 1);
        x1 = std::clamp(x1, 0, VoxelWorld::SX - 1);
        y0 = std::clamp(y0, 0, VoxelWorld::SY - 1);
        y1 = std::clamp(y1, 0, VoxelWorld::SY - 1);
        z0 = std::clamp(z0, 0, VoxelWorld::SZ - 1);
        z1 = std::clamp(z1, 0, VoxelWorld::SZ - 1);

        bool collided = false;
        float newPos = position[axis];

        for (int z = z0; z <= z1; ++z) {
            for (int y = y0; y <= y1; ++y) {
                for (int x = x0; x <= x1; ++x) {
                    if (!isSolidCell(world, x, y, z))
                        continue;

                    voxel_physics::AABB blk = blockAABB(x, y, z);
                    if (!overlap(box, blk))
                        continue;

                    collided = true;

                    if (axis == 0) {
                        if (delta > 0.0f) {
                            newPos = std::min(newPos, blk.min.x - halfSize.x - EPS);
                        } else {
                            newPos = std::max(newPos, blk.max.x + halfSize.x + EPS);
                        }
                    } else if (axis == 1) {
                        if (delta > 0.0f) {
                            newPos = std::min(newPos, blk.min.y - halfSize.y - EPS);
                        } else {
                            newPos = std::max(newPos, blk.max.y + halfSize.y + EPS);
                        }
                    } else {
                        if (delta > 0.0f) {
                            newPos = std::min(newPos, blk.min.z - halfSize.z - EPS);
                        } else {
                            newPos = std::max(newPos, blk.max.z + halfSize.z + EPS);
                        }
                    }
                }
            }
        }

        if (collided) {
            position[axis] = newPos;
            velocity[axis] = 0.0f;

            if (axis == 1) {
                if (delta < 0.0f) {
                    onGround = true;
                    onCollideDown();
                } else {
                    onCollideUp();
                }
            } else {
                onCollideSide();
            }
        }

        // Keep player inside the finite world volume.
        clampToWorldBounds(axis);
    }
};