#pragma once

#include <glm/glm.hpp>

#include "physics/PhysicsBody.h"
#include "world/BlockId.h"

class Player : public PhysicsBody {
public:
    uint8_t selectedBlockID = blocks::kDirt;

    float walkSpeed = 6.5f;
    float sprintSpeed = 10.0f;
    float jumpSpeed = 9.0f;

    float groundAccel = 55.0f;
    float airAccel = 20.0f;
    float groundFriction = 20.0f;

    float eyeOffsetY = 0.72f;

    void applyControl(const glm::vec3& wishDir, bool jumpPressed, bool sprint, float dt);

    glm::vec3 eyePosition() const;
};
