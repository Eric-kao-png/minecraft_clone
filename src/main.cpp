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

#include <algorithm>
#include <cmath>
#include <iostream>

static void framebuffer_size_callback(GLFWwindow* window, int width, int height);
static void mouse_callback(GLFWwindow* window, double xpos, double ypos);
static void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
static void processInput(GLFWwindow* window);
static unsigned int loadTexture(const char* path);
static bool editBlock(bool place);

static const unsigned int SCR_WIDTH = 1280;
static const unsigned int SCR_HEIGHT = 720;

static const int LOAD_RADIUS = 4;
static const int UNLOAD_RADIUS = 6;

static Camera camera(glm::vec3(0.0f, 45.0f, 0.0f),
                     glm::vec3(0.0f, 1.0f, 0.0f),
                     -90.0f, -20.0f);
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

static bool editBlock(bool place) {
    VoxelWorld::RaycastHit hit;
    if (!gWorld.raycast(camera.Position, camera.Front, 6.0f, 0.10f, hit)) {
        return false;
    }

    if (!place) {
        gWorld.setBlockGlobal(hit.block.x, hit.block.y, hit.block.z, false);
        return true;
    }

    const glm::ivec3 target = hit.block + hit.normal;
    if (target.y < 0 || target.y >= VoxelWorld::CHUNK_SIZE_Y) {
        return false;
    }
    if (gWorld.hasBlockGlobal(target.x, target.y, target.z)) {
        return false;
    }

    const voxel_physics::AABB playerBox = gPlayer.aabb();
    const voxel_physics::AABB blockBox = voxel_physics::blockAABB(target.x, target.y, target.z);
    if (voxel_physics::overlap(playerBox, blockBox)) {
        return false;
    }

    gWorld.setBlockGlobal(target.x, target.y, target.z, true);
    return true;
}

int main() {
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
        return -1;
    }
    glfwMakeContextCurrent(window);

    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD\n";
        return -1;
    }

    glEnable(GL_DEPTH_TEST);

    Shader lightingShader("../shaders/colors.vs", "../shaders/colors.fs");

    const unsigned int diffuseMap = loadTexture("../resources/container2.png");
    const unsigned int specularMap = loadTexture("../resources/container2_specular.png");

    gWorld.setSeed(2026);

    const int spawnX = 0;
    const int spawnZ = 0;
    const int surfaceY = gWorld.sampleSurfaceY(spawnX, spawnZ);
    const float groundTopY = static_cast<float>(surfaceY) + 0.5f;

    // 先出生在地表上方一點，避免一開始半卡地面
    gPlayer.position = glm::vec3(
        static_cast<float>(spawnX),
        groundTopY + gPlayer.halfSize.y + 2.0f,
        static_cast<float>(spawnZ)
    );

    gPlayer.velocity = glm::vec3(0.0f);
    gPlayer.onGround = false;
    camera.Position = gPlayer.eyePosition();

    gWorld.updateStreaming(gPlayer.position, LOAD_RADIUS, UNLOAD_RADIUS);

    // 很重要：把 lastFrame 設成現在，避免第一幀 dt 過大
    lastFrame = static_cast<float>(glfwGetTime());

    const glm::vec3 orbitCenter(0.0f, 0.0f, 0.0f);
    const float orbitRadius = 120.0f;
    const float dayLengthSec = 60.0f;
    const float omega = 6.28318530718f / dayLengthSec;
    const glm::vec3 orbitUp(0.0f, 1.0f, 0.0f);
    const glm::vec3 orbitDiagXZ = glm::normalize(glm::vec3(1.0f, 0.0f, 1.0f));

    while (!glfwWindowShouldClose(window)) {
        const float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        deltaTime = std::min(deltaTime, 1.0f / 30.0f);

        processInput(window);

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

        gPlayer.applyControl(wish, gInput.jumpPressed, gInput.sprint, deltaTime);
        gPlayer.step(gWorld, deltaTime);
        camera.Position = gPlayer.eyePosition();

        gWorld.updateStreaming(gPlayer.position, LOAD_RADIUS, UNLOAD_RADIUS);

        glClearColor(0.07f, 0.07f, 0.10f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

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

        lightingShader.use();
        lightingShader.setInt("material.diffuse", 0);
        lightingShader.setInt("material.specular", 1);
        lightingShader.setFloat("material.shininess", 64.0f);

        glm::vec3 minAmbient(0.03f, 0.03f, 0.04f);

        lightingShader.setVec3("sunLight.direction", sunRayDir);
        lightingShader.setVec3("sunLight.ambient", minAmbient + glm::vec3(0.02f, 0.015f, 0.010f) * sunStrength);
        lightingShader.setVec3("sunLight.diffuse",  glm::vec3(1.00f, 0.85f, 0.65f) * (1.25f * sunStrength));
        lightingShader.setVec3("sunLight.specular", glm::vec3(1.00f, 0.95f, 0.85f) * (1.10f * sunStrength));

        lightingShader.setVec3("moonLight.direction", moonRayDir);
        lightingShader.setVec3("moonLight.ambient", minAmbient + glm::vec3(0.012f) + glm::vec3(0.008f, 0.010f, 0.020f) * moonStrength);
        lightingShader.setVec3("moonLight.diffuse",  glm::vec3(0.25f, 0.35f, 0.90f) * (1.20f * moonStrength));
        lightingShader.setVec3("moonLight.specular", glm::vec3(0.30f, 0.40f, 1.00f) * (1.10f * moonStrength));

        lightingShader.setVec3("spotLight.position", camera.Position);
        lightingShader.setVec3("spotLight.direction", camera.Front);
        // lightingShader.setVec3("spotLight.ambient", 0.0f, 0.0f, 0.0f);
        // lightingShader.setVec3("spotLight.diffuse", 1.0f, 1.0f, 1.0f);
        // lightingShader.setVec3("spotLight.specular", 1.0f, 1.0f, 1.0f);
        lightingShader.setFloat("spotLight.constant", 1.0f);
        lightingShader.setFloat("spotLight.linear", 0.09f);
        lightingShader.setFloat("spotLight.quadratic", 0.032f);
        lightingShader.setFloat("spotLight.cutOff", glm::cos(glm::radians(12.5f)));
        lightingShader.setFloat("spotLight.outerCutOff", glm::cos(glm::radians(15.0f)));

        lightingShader.setVec3("viewPos", camera.Position);
        lightingShader.setMat4("projection", projection);
        lightingShader.setMat4("view", view);
        lightingShader.setMat4("model", glm::mat4(1.0f));

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
}

static void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    (void)window;
    glViewport(0, 0, width, height);
}

static void mouse_callback(GLFWwindow* window, double xposIn, double yposIn) {
    (void)window;

    const float xpos = static_cast<float>(xposIn);
    const float ypos = static_cast<float>(yposIn);

    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    const float xoffset = xpos - lastX;
    const float yoffset = lastY - ypos;
    lastX = xpos;
    lastY = ypos;

    camera.ProcessMouseMovement(xoffset, yoffset);
}

static void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    (void)window;
    (void)xoffset;
    camera.ProcessMouseScroll(static_cast<float>(yoffset));
}

static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    (void)window;
    (void)mods;

    if (action != GLFW_PRESS) return;

    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        editBlock(false);
    } else if (button == GLFW_MOUSE_BUTTON_RIGHT) {
        editBlock(true);
    }
}

static unsigned int loadTexture(const char* path) {
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load(path, &width, &height, &nrComponents, 0);

    if (data) {
        GLenum format = GL_RGB;
        if (nrComponents == 1) format = GL_RED;
        else if (nrComponents == 3) format = GL_RGB;
        else if (nrComponents == 4) format = GL_RGBA;

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
