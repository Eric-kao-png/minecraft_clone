#include "app/Application.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <engine/Shader.h>

#include "render/DayNightLighting.h"
#include "render/TextureLoader.h"

#include "physics/VoxelPhysics.h"
#include "world/BlockId.h"
#include "world/Chunk.h"

#include <algorithm>
#include <cmath>
#include <iostream>

Application::Application()
    : camera_(glm::vec3(0.0f, 45.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), -90.0f, -20.0f) {}

int Application::run() {
    if (!initWindow()) {
        return -1;
    }

    Shader lightingShader(GameConfig::kShaderVertexPath, GameConfig::kShaderFragmentPath);
    constexpr render::TextureFilter kBlockTextureFilter = render::TextureFilter::PixelArt;
    diffuseMap_ = render::loadTexture2D(GameConfig::kAtlasTexturePath, kBlockTextureFilter);
    specularMap_ = render::loadTexture2D(GameConfig::kAtlasTexturePath, kBlockTextureFilter);

    initGameWorld();
    lastFrame_ = static_cast<float>(glfwGetTime());

    while (!glfwWindowShouldClose(window_)) {
        const float currentFrame = static_cast<float>(glfwGetTime());
        const float deltaTime = std::min(currentFrame - lastFrame_, 1.0f / 30.0f);
        lastFrame_ = currentFrame;

        input_.pollKeyboard(window_);
        if (input_.isCursorCaptured()) {
            updatePlayerMovement(deltaTime);
        }
        world_.updateStreaming(player_.position, GameConfig::kLoadRadius, GameConfig::kUnloadRadius);

        glClearColor(0.07f, 0.07f, 0.10f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        render::applyDayNightLighting(lightingShader, camera_, currentFrame,
                                      GameConfig::kScreenWidth, GameConfig::kScreenHeight);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, diffuseMap_);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, specularMap_);

        world_.render();

        glfwSwapBuffers(window_);
        glfwPollEvents();
    }

    world_.clear();
    glfwTerminate();
    return 0;
}

bool Application::initWindow() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    window_ = glfwCreateWindow(static_cast<int>(GameConfig::kScreenWidth),
                               static_cast<int>(GameConfig::kScreenHeight),
                               GameConfig::kWindowTitle, nullptr, nullptr);
    if (window_ == nullptr) {
        std::cout << "Failed to create GLFW window\n";
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(window_);
    glfwSetWindowUserPointer(window_, this);
    input_.install(window_, this);
    glfwSetInputMode(window_, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) {
        std::cout << "Failed to initialize GLAD\n";
        return false;
    }

    glEnable(GL_DEPTH_TEST);
    return true;
}

void Application::initGameWorld() {
    world_.setSeed(GameConfig::kWorldSeed);

    const int spawnX = 0;
    const int spawnZ = 0;
    const int surfaceY = world_.sampleSurfaceY(spawnX, spawnZ);
    const float groundTopY = static_cast<float>(surfaceY) + 0.5f;

    player_.position =
        glm::vec3(static_cast<float>(spawnX), groundTopY + player_.halfSize.y + 2.0f, static_cast<float>(spawnZ));
    player_.velocity = glm::vec3(0.0f);
    player_.onGround = false;

    camera_.Position = player_.eyePosition();
    world_.updateStreaming(player_.position, GameConfig::kLoadRadius, GameConfig::kUnloadRadius);
}

void Application::updatePlayerMovement(float dt) {
    const auto& input = input_.state();

    glm::vec3 forward(camera_.Front.x, 0.0f, camera_.Front.z);
    float f2 = glm::dot(forward, forward);
    if (f2 < 1e-8f) {
        forward = glm::vec3(0.0f, 0.0f, -1.0f);
    } else {
        forward /= std::sqrt(f2);
    }

    glm::vec3 right(camera_.Right.x, 0.0f, camera_.Right.z);
    const float r2 = glm::dot(right, right);
    if (r2 < 1e-8f) {
        right = glm::normalize(glm::cross(forward, glm::vec3(0.0f, 1.0f, 0.0f)));
    } else {
        right /= std::sqrt(r2);
    }

    glm::vec3 wish(0.0f);
    if (input.forward) wish += forward;
    if (input.backward) wish -= forward;
    if (input.right) wish += right;
    if (input.left) wish -= right;

    const float w2 = glm::dot(wish, wish);
    if (w2 > 1e-8f) {
        wish /= std::sqrt(w2);
    }

    player_.applyControl(wish, input.jumpPressed, input.sprint, dt);
    player_.step(world_, dt);
    camera_.Position = player_.eyePosition();
}

bool Application::editBlock(bool place) {
    VoxelWorld::RaycastHit hit;
    if (!world_.raycast(camera_.Position, camera_.Front, 6.0f, 0.10f, hit)) {
        return false;
    }

    if (!place) {
        world_.setBlockGlobal(hit.block.x, hit.block.y, hit.block.z, blocks::kAir);
        return true;
    }

    const glm::ivec3 target = hit.block + hit.normal;
    if (target.y < 0 || target.y >= Chunk::SIZE_Y) {
        return false;
    }
    if (world_.hasBlockGlobal(target.x, target.y, target.z)) {
        return false;
    }
    if (voxel_physics::overlap(player_.aabb(), voxel_physics::blockAABB(target.x, target.y, target.z))) {
        return false;
    }

    world_.setBlockGlobal(target.x, target.y, target.z, player_.selectedBlockID);
    return true;
}
