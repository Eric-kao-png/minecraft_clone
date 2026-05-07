#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stb_image.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <../learnopengl/camera.h>
#include <../learnopengl/shader_m.h>

#include "physics/Player.h"
#include "world/VoxelWorld.h"
#include "world/Chunk.h"

#include <algorithm>
#include <cmath>
#include <iostream>

static const unsigned int SCR_WIDTH = 1280;
static const unsigned int SCR_HEIGHT = 720;
static const int LOAD_RADIUS = 4;
static const int UNLOAD_RADIUS = 6;

static Camera camera(glm::vec3(0.0f, 45.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), -90.0f, -20.0f);
static float lastX = SCR_WIDTH / 2.0f;
static float lastY = SCR_HEIGHT / 2.0f;
static bool firstMouse = true;

static float deltaTime = 0.0f;
static float lastFrame = 0.0f;

static Player gPlayer;
static VoxelWorld gWorld;

struct InputState {
    bool forward = false;
    bool backward = false;
    bool left = false;
    bool right = false;
    bool sprint = false;
    bool jumpPressed = false;
};
static InputState gInput;
static bool gSpaceWasDown = false;

static GLFWwindow* initializeWindow();
static void initializeGameWorld();
static void updatePlayerMovement(float dt);
static void setupLightingAndCamera(Shader& shader, float currentFrame);
static bool editBlock(bool place);
static void processInput(GLFWwindow* window);
static unsigned int loadTexture(const char* path);

static void framebuffer_size_callback(GLFWwindow* window, int width, int height);
static void mouse_callback(GLFWwindow* window, double xpos, double ypos);
static void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);

int main() {
    GLFWwindow* window = initializeWindow();
    if (!window) return -1;

    Shader lightingShader("../shaders/colors.vs", "../shaders/colors.fs");

    const unsigned int diffuseMap = loadTexture("../resources/minecraft_atlas.png");
    const unsigned int specularMap = loadTexture("../resources/minecraft_atlas.png");

    initializeGameWorld();

    lastFrame = static_cast<float>(glfwGetTime());

    while (!glfwWindowShouldClose(window)) {
        const float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = std::min(currentFrame - lastFrame, 1.0f / 30.0f);
        lastFrame = currentFrame;

        processInput(window);
        updatePlayerMovement(deltaTime);
        gWorld.updateStreaming(gPlayer.position, LOAD_RADIUS, UNLOAD_RADIUS);

        glClearColor(0.07f, 0.07f, 0.10f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        setupLightingAndCamera(lightingShader, currentFrame);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, diffuseMap);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, specularMap);

        gWorld.render();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    gWorld.clear();
    glfwTerminate();
    return 0;
}

static GLFWwindow* initializeWindow() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Voxel Chunks", nullptr, nullptr);
    if (window == nullptr) {
        std::cout << "Failed to create GLFW window\n";
        glfwTerminate();
        return nullptr;
    }
    glfwMakeContextCurrent(window);

    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD\n";
        return nullptr;
    }

    glEnable(GL_DEPTH_TEST);
    return window;
}

static void initializeGameWorld() {
    gWorld.setSeed(2026);
    const int spawnX = 0;
    const int spawnZ = 0;
    const int surfaceY = gWorld.sampleSurfaceY(spawnX, spawnZ);
    const float groundTopY = static_cast<float>(surfaceY) + 0.5f;

    gPlayer.position = glm::vec3(static_cast<float>(spawnX), groundTopY + gPlayer.halfSize.y + 2.0f, static_cast<float>(spawnZ));
    gPlayer.velocity = glm::vec3(0.0f);
    gPlayer.onGround = false;

    camera.Position = gPlayer.eyePosition();
    gWorld.updateStreaming(gPlayer.position, LOAD_RADIUS, UNLOAD_RADIUS);
}

static void updatePlayerMovement(float dt) {
    glm::vec3 forward(camera.Front.x, 0.0f, camera.Front.z);
    float f2 = glm::dot(forward, forward);
    if (f2 < 1e-8f) forward = glm::vec3(0.0f, 0.0f, -1.0f);
    else forward /= std::sqrt(f2);

    glm::vec3 right(camera.Right.x, 0.0f, camera.Right.z);
    float r2 = glm::dot(right, right);
    if (r2 < 1e-8f) right = glm::normalize(glm::cross(forward, glm::vec3(0.0f, 1.0f, 0.0f)));
    else right /= std::sqrt(r2);

    glm::vec3 wish(0.0f);
    if (gInput.forward)  wish += forward;
    if (gInput.backward) wish -= forward;
    if (gInput.right)    wish += right;
    if (gInput.left)     wish -= right;

    const float w2 = glm::dot(wish, wish);
    if (w2 > 1e-8f) wish /= std::sqrt(w2);

    gPlayer.applyControl(wish, gInput.jumpPressed, gInput.sprint, dt);
    gPlayer.step(gWorld, dt);
    camera.Position = gPlayer.eyePosition();
}

static void setupLightingAndCamera(Shader& shader, float currentFrame) {
    const float orbitRadius = 120.0f;
    const float dayLengthSec = 60.0f;
    const float omega = 6.28318530718f / dayLengthSec;
    const glm::vec3 orbitUp(0.0f, 1.0f, 0.0f);
    const glm::vec3 orbitDiagXZ = glm::normalize(glm::vec3(1.0f, 0.0f, 1.0f));

    const float angle = currentFrame * omega;
    const glm::vec3 sunOffset = orbitRadius * (std::cos(angle) * orbitDiagXZ + std::sin(angle) * orbitUp);
    const glm::vec3 moonOffset = -sunOffset;

    const glm::vec3 sunRayDir = glm::normalize(-sunOffset);
    const glm::vec3 moonRayDir = glm::normalize(-moonOffset);

    const float sunUp = std::max(0.0f, -sunRayDir.y);
    const float moonUp = std::max(0.0f, -moonRayDir.y);
    const float sunStrength = sunUp * sunUp;
    const float moonStrength = std::sqrt(moonUp);

    const glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom),
                                                  static_cast<float>(SCR_WIDTH) / static_cast<float>(SCR_HEIGHT),
                                                  0.1f, 500.0f);
    const glm::mat4 view = camera.GetViewMatrix();

    shader.use();
    shader.setInt("material.diffuse", 0);
    shader.setInt("material.specular", 1);
    shader.setFloat("material.shininess", 64.0f);

    glm::vec3 minAmbient(0.03f, 0.03f, 0.04f);

    shader.setVec3("sunLight.direction", sunRayDir);
    shader.setVec3("sunLight.ambient", minAmbient + glm::vec3(0.02f, 0.015f, 0.010f) * sunStrength);
    shader.setVec3("sunLight.diffuse",  glm::vec3(1.00f, 0.85f, 0.65f) * (1.25f * sunStrength));
    shader.setVec3("sunLight.specular", glm::vec3(1.00f, 0.95f, 0.85f) * (1.10f * sunStrength));

    shader.setVec3("moonLight.direction", moonRayDir);
    shader.setVec3("moonLight.ambient", minAmbient + glm::vec3(0.012f) + glm::vec3(0.008f, 0.010f, 0.020f) * moonStrength);
    shader.setVec3("moonLight.diffuse",  glm::vec3(0.25f, 0.35f, 0.90f) * (1.20f * moonStrength));
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

static bool editBlock(bool place) {
    VoxelWorld::RaycastHit hit;
    if (!gWorld.raycast(camera.Position, camera.Front, 6.0f, 0.10f, hit)) return false;

    if (!place) {
        gWorld.setBlockGlobal(hit.block.x, hit.block.y, hit.block.z, 0);
        return true;
    }

    const glm::ivec3 target = hit.block + hit.normal;
    if (target.y < 0 || target.y >= Chunk::SIZE_Y) return false;
    if (gWorld.hasBlockGlobal(target.x, target.y, target.z)) return false;

    if (voxel_physics::overlap(gPlayer.aabb(), voxel_physics::blockAABB(target.x, target.y, target.z))) return false;

    // 放置目前選中的方塊
    gWorld.setBlockGlobal(target.x, target.y, target.z, gPlayer.selectedBlockID);
    return true;
}

static void processInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    gInput.forward = glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS;
    gInput.backward = glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS;
    gInput.left = glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS;
    gInput.right = glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS;
    gInput.sprint = glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS;

    const bool spaceDown = glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS;
    gInput.jumpPressed = spaceDown && !gSpaceWasDown;
    gSpaceWasDown = spaceDown;

    // 按鍵切換邏輯 (依據要求的順序)
    if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS) gPlayer.selectedBlockID = 2; // 泥土
    if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS) gPlayer.selectedBlockID = 3; // 草地
    if (glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS) gPlayer.selectedBlockID = 4; // 鵝卵石
    if (glfwGetKey(window, GLFW_KEY_4) == GLFW_PRESS) gPlayer.selectedBlockID = 1; // 石頭
}

static void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    (void)window;
    glViewport(0, 0, width, height);
}

static void mouse_callback(GLFWwindow* window, double xposIn, double yposIn) {
    (void)window;
    const float xpos = static_cast<float>(xposIn);
    const float ypos = static_cast<float>(yposIn);
    if (firstMouse) { lastX = xpos; lastY = ypos; firstMouse = false; }
    camera.ProcessMouseMovement(xpos - lastX, lastY - ypos);
    lastX = xpos; lastY = ypos;
}

static void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    (void)window; (void)xoffset;
    camera.ProcessMouseScroll(static_cast<float>(yoffset));
}

static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    (void)window; (void)mods;
    if (action != GLFW_PRESS) return;
    if (button == GLFW_MOUSE_BUTTON_LEFT) editBlock(false);
    else if (button == GLFW_MOUSE_BUTTON_RIGHT) editBlock(true);
}

static unsigned int loadTexture(const char* path) {
    unsigned int textureID;
    glGenTextures(1, &textureID);
    int width, height, nrComponents;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load(path, &width, &height, &nrComponents, 0);
    if (data) {
        GLenum format = (nrComponents == 1) ? GL_RED : (nrComponents == 3) ? GL_RGB : GL_RGBA;
        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        stbi_image_free(data);
    } else {
        std::cout << "Texture failed to load at path: " << path << "\n";
        stbi_image_free(data);
    }
    return textureID;
}