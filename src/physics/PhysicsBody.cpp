#include "physics/PhysicsBody.h"

#include "world/BlockQuery.h"

#include <algorithm>
#include <cmath>

voxel_physics::AABB PhysicsBody::aabb() const {
    return voxel_physics::makeAABB(position, halfSize);
}

void PhysicsBody::step(const BlockQuery& world, float dt) {
    dt = std::max(0.0f, dt);
    if (dt <= 0.0f) {
        return;
    }

    const float maxSubStep = 1.0f / 120.0f;
    const int steps = std::max(1, static_cast<int>(std::ceil(dt / maxSubStep)));
    const float subDt = dt / static_cast<float>(steps);

    for (int i = 0; i < steps; ++i) {
        velocity.y -= gravity * subDt;
        velocity.y = std::max(velocity.y, -maxFallSpeed);

        onGround = false;

        moveAxis(world, 0, velocity.x * subDt);
        moveAxis(world, 1, velocity.y * subDt);
        moveAxis(world, 2, velocity.z * subDt);
    }
}

void PhysicsBody::moveAxis(const BlockQuery& world, int axis, float delta) {
    if (std::abs(delta) < 1e-7f) {
        return;
    }

    position[axis] += delta;

    if (resolveAxisCollision(world, axis, delta)) {
        return;
    }

    if (axis == 1 && delta < 0.0f) {
        probeGroundContact(world);
    }
}

bool PhysicsBody::resolveAxisCollision(const BlockQuery& world, int axis, float delta) {
    const voxel_physics::AABB box = aabb();

    const int x0 = voxel_physics::minCellFromWorld(box.min.x);
    const int x1 = voxel_physics::maxCellFromWorld(box.max.x);
    const int y0 = voxel_physics::minCellFromWorld(box.min.y);
    const int y1 = voxel_physics::maxCellFromWorld(box.max.y);
    const int z0 = voxel_physics::minCellFromWorld(box.min.z);
    const int z1 = voxel_physics::maxCellFromWorld(box.max.z);

    bool collided = false;
    float newPos = position[axis];

    for (int z = z0; z <= z1; ++z) {
        for (int y = y0; y <= y1; ++y) {
            for (int x = x0; x <= x1; ++x) {
                if (!world.hasBlockGlobal(x, y, z)) {
                    continue;
                }

                const voxel_physics::AABB blk = voxel_physics::blockAABB(x, y, z);
                if (!voxel_physics::overlap(box, blk)) {
                    continue;
                }

                collided = true;

                if (axis == 0) {
                    if (delta > 0.0f) {
                        newPos = std::min(newPos, blk.min.x - halfSize.x - voxel_physics::EPS);
                    } else {
                        newPos = std::max(newPos, blk.max.x + halfSize.x + voxel_physics::EPS);
                    }
                } else if (axis == 1) {
                    if (delta > 0.0f) {
                        newPos = std::min(newPos, blk.min.y - halfSize.y - voxel_physics::EPS);
                    } else {
                        newPos = std::max(newPos, blk.max.y + halfSize.y + voxel_physics::EPS);
                    }
                } else {
                    if (delta > 0.0f) {
                        newPos = std::min(newPos, blk.min.z - halfSize.z - voxel_physics::EPS);
                    } else {
                        newPos = std::max(newPos, blk.max.z + halfSize.z + voxel_physics::EPS);
                    }
                }
            }
        }
    }

    if (!collided) {
        return false;
    }

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

    return true;
}

void PhysicsBody::probeGroundContact(const BlockQuery& world) {
    const float probeDepth = 0.02f;
    voxel_physics::AABB probe = aabb();
    probe.min.y -= probeDepth;

    const int px0 = voxel_physics::minCellFromWorld(probe.min.x);
    const int px1 = voxel_physics::maxCellFromWorld(probe.max.x);
    const int py0 = voxel_physics::minCellFromWorld(probe.min.y);
    const int py1 = voxel_physics::maxCellFromWorld(probe.max.y);
    const int pz0 = voxel_physics::minCellFromWorld(probe.min.z);
    const int pz1 = voxel_physics::maxCellFromWorld(probe.max.z);

    bool foundGround = false;
    float bestTopY = -1e9f;

    for (int z = pz0; z <= pz1; ++z) {
        for (int y = py0; y <= py1; ++y) {
            for (int x = px0; x <= px1; ++x) {
                if (!world.hasBlockGlobal(x, y, z)) {
                    continue;
                }

                const voxel_physics::AABB blk = voxel_physics::blockAABB(x, y, z);
                if (!voxel_physics::overlap(probe, blk)) {
                    continue;
                }

                foundGround = true;
                bestTopY = std::max(bestTopY, blk.max.y);
            }
        }
    }

    if (foundGround) {
        position.y = bestTopY + halfSize.y + voxel_physics::EPS;
        velocity.y = 0.0f;
        onGround = true;
        onCollideDown();
    }
}
