#pragma once

#include <glm/glm.hpp>

#include <algorithm>

#include "physics/PhysicsBody.h"

// A first-person player controller built on top of PhysicsBody.
// - applyControl() updates horizontal velocity and jump impulse
// - physics step is handled by PhysicsBody::step(world, dt)
class Player : public PhysicsBody {
public:
    // Movement tuning
    float walkSpeed   = 6.5f;
    float sprintSpeed = 10.0f;
    float jumpSpeed   = 9.0f;

    float groundAccel   = 55.0f; // how quickly you reach target speed
    float airAccel      = 20.0f;
    float groundFriction = 20.0f; // how quickly you stop when no input

    // Minecraft-ish eye height: 1.62 above feet -> 0.72 above AABB center (center is 0.9 above feet)
    float eyeOffsetY = 0.72f;

    // Call once per frame BEFORE step().
    // wishDir should be normalized (or zero).
    void applyControl(const glm::vec3& wishDir, bool jumpPressed, bool sprint, float dt) {
        dt = std::max(0.0f, dt);

        // Horizontal target velocity
        float speed = sprint ? sprintSpeed : walkSpeed;
        glm::vec3 targetVel = wishDir * speed;

        glm::vec3 velXZ(velocity.x, 0.0f, velocity.z);
        glm::vec3 delta = targetVel - velXZ;

        float accel = onGround ? groundAccel : airAccel;
        float maxChange = accel * dt;

        float dlen = glm::length(delta);
        if (dlen > maxChange && dlen > 1e-6f) {
            delta = (delta / dlen) * maxChange;
        }

        velXZ += delta;

        // Ground friction when no input
        if (onGround && glm::length(wishDir) < 1e-4f) {
            float v = glm::length(velXZ);
            float drop = groundFriction * dt;
            v = std::max(0.0f, v - drop);
            if (v < 1e-4f) velXZ = glm::vec3(0.0f);
            else velXZ = glm::normalize(velXZ) * v;
        }

        velocity.x = velXZ.x;
        velocity.z = velXZ.z;

        // Jump (edge-triggered recommended from input)
        if (jumpPressed && onGround) {
            velocity.y = jumpSpeed;
            onGround = false;
        }
    }

    glm::vec3 eyePosition() const {
        return position + glm::vec3(0.0f, eyeOffsetY, 0.0f);
    }

protected:
    void onCollideDown() override {
        // You can add landing sounds / events here later.
    }
};