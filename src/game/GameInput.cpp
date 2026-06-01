#include "game/GameInput.h"

#include "app/Application.h"
#include "app/GameConfig.h"
#include "world/BlockId.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

namespace game {

namespace {

Application* appFromWindow(GLFWwindow* window) {
    return static_cast<Application*>(glfwGetWindowUserPointer(window));
}

} // namespace

void GameInput::install(GLFWwindow* window, Application* app) {
    app_ = app;
    lastMouseX_ = static_cast<float>(GameConfig::kScreenWidth) * 0.5f;
    lastMouseY_ = static_cast<float>(GameConfig::kScreenHeight) * 0.5f;

    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
    glfwSetCursorPosCallback(window, mouseMoveCallback);
    glfwSetScrollCallback(window, scrollCallback);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);
}

void GameInput::pollKeyboard(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }

    state_.forward = glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS;
    state_.backward = glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS;
    state_.left = glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS;
    state_.right = glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS;
    state_.sprint = glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS;

    const bool spaceDown = glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS;
    state_.jumpPressed = spaceDown && !spaceWasDown_;
    spaceWasDown_ = spaceDown;

    if (!app_) {
        return;
    }

    Player& player = app_->player();
    if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS) player.selectedBlockID = blocks::kDirt;
    if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS) player.selectedBlockID = blocks::kGrass;
    if (glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS) player.selectedBlockID = blocks::kCobblestone;
    if (glfwGetKey(window, GLFW_KEY_4) == GLFW_PRESS) player.selectedBlockID = blocks::kStone;
}

void GameInput::framebufferSizeCallback(GLFWwindow* window, int width, int height) {
    (void)window;
    glViewport(0, 0, width, height);
}

void GameInput::mouseMoveCallback(GLFWwindow* window, double xposIn, double yposIn) {
    Application* app = appFromWindow(window);
    if (!app) {
        return;
    }

    GameInput& input = app->input();
    const float xpos = static_cast<float>(xposIn);
    const float ypos = static_cast<float>(yposIn);

    if (input.firstMouse_) {
        input.lastMouseX_ = xpos;
        input.lastMouseY_ = ypos;
        input.firstMouse_ = false;
    }

    Camera& camera = app->camera();
    camera.ProcessMouseMovement(xpos - input.lastMouseX_, input.lastMouseY_ - ypos);
    input.lastMouseX_ = xpos;
    input.lastMouseY_ = ypos;
}

void GameInput::scrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
    (void)xoffset;
    Application* app = appFromWindow(window);
    if (!app) {
        return;
    }
    app->camera().ProcessMouseScroll(static_cast<float>(yoffset));
}

void GameInput::mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    (void)mods;
    if (action != GLFW_PRESS) {
        return;
    }

    Application* app = appFromWindow(window);
    if (!app) {
        return;
    }

    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        app->editBlock(false);
    } else if (button == GLFW_MOUSE_BUTTON_RIGHT) {
        app->editBlock(true);
    }
}

} // namespace game
