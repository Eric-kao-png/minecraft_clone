#include "render/DayNightLighting.h"

#include <engine/Camera.h>
#include <engine/Shader.h>

#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <cmath>

namespace render {

void applyDayNightLighting(Shader& shader, Camera& camera, float currentFrame,
                           unsigned int screenWidth, unsigned int screenHeight) {
    const float orbitRadius = 120.0f;
    const float dayLengthSec = 60.0f;
    const float omega = 6.28318530718f / dayLengthSec;
    const glm::vec3 orbitUp(0.0f, 1.0f, 0.0f);
    const glm::vec3 orbitDiagXZ = glm::normalize(glm::vec3(1.0f, 0.0f, 1.0f));

    const float angle = currentFrame * omega;
    const glm::vec3 sunOffset =
        orbitRadius * (std::cos(angle) * orbitDiagXZ + std::sin(angle) * orbitUp);
    const glm::vec3 moonOffset = -sunOffset;

    const glm::vec3 sunRayDir = glm::normalize(-sunOffset);
    const glm::vec3 moonRayDir = glm::normalize(-moonOffset);

    const float sunUp = std::max(0.0f, -sunRayDir.y);
    const float moonUp = std::max(0.0f, -moonRayDir.y);
    const float sunStrength = sunUp * sunUp;
    const float moonStrength = std::sqrt(moonUp);

    const glm::mat4 projection =
        glm::perspective(glm::radians(camera.Zoom),
                         static_cast<float>(screenWidth) / static_cast<float>(screenHeight),
                         0.1f, 500.0f);
    const glm::mat4 view = camera.GetViewMatrix();

    shader.use();
    shader.setInt("material.diffuse", 0);
    shader.setInt("material.specular", 1);
    shader.setFloat("material.shininess", 64.0f);

    const glm::vec3 minAmbient(0.03f, 0.03f, 0.04f);

    shader.setVec3("sunLight.direction", sunRayDir);
    shader.setVec3("sunLight.ambient", minAmbient + glm::vec3(0.02f, 0.015f, 0.010f) * sunStrength);
    shader.setVec3("sunLight.diffuse", glm::vec3(1.00f, 0.85f, 0.65f) * (1.25f * sunStrength));
    shader.setVec3("sunLight.specular", glm::vec3(1.00f, 0.95f, 0.85f) * (1.10f * sunStrength));

    shader.setVec3("moonLight.direction", moonRayDir);
    shader.setVec3("moonLight.ambient",
                   minAmbient + glm::vec3(0.012f) + glm::vec3(0.008f, 0.010f, 0.020f) * moonStrength);
    shader.setVec3("moonLight.diffuse", glm::vec3(0.25f, 0.35f, 0.90f) * (1.20f * moonStrength));
    shader.setVec3("moonLight.specular", glm::vec3(0.30f, 0.40f, 1.00f) * (1.10f * moonStrength));

    shader.setVec3("spotLight.position", camera.Position);
    shader.setVec3("spotLight.direction", camera.Front);
    shader.setFloat("spotLight.constant", 1.0f);
    shader.setFloat("spotLight.linear", 0.09f);
    shader.setFloat("spotLight.quadratic", 0.032f);
    shader.setFloat("spotLight.cutOff", glm::cos(glm::radians(12.5f)));
    shader.setFloat("spotLight.outerCutOff", glm::cos(glm::radians(15.0f)));

    shader.setVec3("viewPos", camera.Position);
    shader.setMat4("projection", projection);
    shader.setMat4("view", view);
    shader.setMat4("model", glm::mat4(1.0f));
}

} // namespace render
