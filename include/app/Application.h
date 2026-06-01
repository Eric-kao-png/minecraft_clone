#pragma once

#include <engine/Camera.h>

#include "app/GameConfig.h"
#include "game/GameInput.h"
#include "physics/Player.h"
#include "world/VoxelWorld.h"

struct GLFWwindow;
class Shader;

class Application {
public:
    Application();

    int run();

    Camera& camera() { return camera_; }
    const Camera& camera() const { return camera_; }

    Player& player() { return player_; }
    VoxelWorld& world() { return world_; }
    game::GameInput& input() { return input_; }

    bool editBlock(bool place);

private:
    bool initWindow();
    void initGameWorld();
    void updatePlayerMovement(float dt);

    GLFWwindow* window_ = nullptr;
    Camera camera_;
    Player player_;
    VoxelWorld world_;
    game::GameInput input_;

    float lastFrame_ = 0.0f;
    unsigned int diffuseMap_ = 0;
    unsigned int specularMap_ = 0;
    unsigned int dirtMap_ = 0;
    unsigned int grassTopMap_ = 0;
    unsigned int grassSideMap_ = 0;
};
