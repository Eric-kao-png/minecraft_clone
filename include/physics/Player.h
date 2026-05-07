#pragma once

#include <glm/glm.hpp>
#include <algorithm>
#include "physics/PhysicsBody.h"

class Player : public PhysicsBody {
public:
    // 初始狀態設定為泥土 (ID 2)
    uint8_t selectedBlockID = 2;

    float walkSpeed = 6.5f;
    float sprintSpeed = 10.0f;
    float jumpSpeed = 9.0f;

    float groundAccel = 55.0f;
    float airAccel = 20.0f;
    float groundFriction = 20.0f;

    float eyeOffsetY = 0.72f;

    void applyControl(const glm::vec3& wishDir, bool jumpPressed, bool sprint, float dt) {
        dt = std::max(0.0f, dt);

        const float speed = sprint ? sprintSpeed : walkSpeed;
        glm::vec3 targetVel = wishDir * speed;

        glm::vec3 velXZ(velocity.x, 0.0f, velocity.z);
        glm::vec3 delta = targetVel - velXZ;

        const float accel = onGround ? groundAccel : airAccel;
        const float maxChange = accel * dt;
        const float dlen = glm::length(delta);
        if (dlen > maxChange && dlen > 1e-6f) {
            delta = (delta / dlen) * maxChange;
        }

        velXZ += delta;

        if (onGround && glm::length(wishDir) < 1e-4f) {
            float v = glm::length(velXZ);
            v = std::max(0.0f, v - groundFriction * dt);
            if (v < 1e-4f) velXZ = glm::vec3(0.0f);
            else velXZ = glm::normalize(velXZ) * v;
        }

        velocity.x = velXZ.x;
        velocity.z = velXZ.z;

        if (jumpPressed && onGround) {
            velocity.y = jumpSpeed;
            onGround = false;
        }
    }

    glm::vec3 eyePosition() const {
        return position + glm::vec3(0.0f, eyeOffsetY, 0.0f);
    }
};