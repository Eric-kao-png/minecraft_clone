#pragma once

struct GLFWwindow;

class Application;

namespace game {

struct InputState {
    bool forward = false;
    bool backward = false;
    bool left = false;
    bool right = false;
    bool sprint = false;
    bool jumpPressed = false;
};

class GameInput {
public:
    void install(GLFWwindow* window, Application* app);

    void pollKeyboard(GLFWwindow* window);

    const InputState& state() const { return state_; }

private:
    static void framebufferSizeCallback(GLFWwindow* window, int width, int height);
    static void mouseMoveCallback(GLFWwindow* window, double xpos, double ypos);
    static void scrollCallback(GLFWwindow* window, double xoffset, double yoffset);
    static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);

    Application* app_ = nullptr;
    InputState state_;
    bool firstMouse_ = true;
    bool spaceWasDown_ = false;
    float lastMouseX_ = 0.0f;
    float lastMouseY_ = 0.0f;
};

} // namespace game
