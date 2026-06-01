#pragma once

#include <glm/glm.hpp>

#include "physics/VoxelPhysics.h"

class BlockQuery;

class PhysicsBody {
public:
    glm::vec3 position{0.0f};
    glm::vec3 velocity{0.0f};

    glm::vec3 halfSize{0.30f, 0.90f, 0.30f};

    float gravity = 24.0f;
    float maxFallSpeed = 40.0f;

    bool onGround = false;

    voxel_physics::AABB aabb() const;
    void step(const BlockQuery& world, float dt);

protected:
    virtual void onCollideDown() {}
    virtual void onCollideUp() {}
    virtual void onCollideSide() {}

private:
    void moveAxis(const BlockQuery& world, int axis, float delta);
    bool resolveAxisCollision(const BlockQuery& world, int axis, float delta);
    void probeGroundContact(const BlockQuery& world);
};
