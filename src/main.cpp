#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stb_image.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <../learnopengl/shader_m.h>
#include <../learnopengl/camera.h>

#include "world/VoxelWorld.h"

#include <iostream>
#include <vector>
#include <string>
#include <cmath>

// callbacks / helpers
static void framebuffer_size_callback(GLFWwindow* window, int width, int height);
static void mouse_callback(GLFWwindow* window, double xpos, double ypos);
static void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
static void processInput(GLFWwindow* window);
static unsigned int loadTexture(const char* path);

// settings
static const unsigned int SCR_WIDTH  = 1280;
static const unsigned int SCR_HEIGHT = 720;

// camera
// World is centered around origin, put camera above and back a bit.
static Camera camera(glm::vec3(0.0f, 25.0f, 80.0f),
                     glm::vec3(0.0f, 1.0f, 0.0f),
                     -90.0f, -20.0f);

static float lastX = SCR_WIDTH / 2.0f;
static float lastY = SCR_HEIGHT / 2.0f;
static bool firstMouse = true;

// timing
static float deltaTime = 0.0f;
static float lastFrame = 0.0f;

int main()
{
    // glfw init
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Voxel World (naive cubes)", NULL, NULL);
    if (window == NULL) {
        std::cout << "Failed to create GLFW window\n";
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);

    // capture mouse
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // glad load
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD\n";
        return -1;
    }

    glEnable(GL_DEPTH_TEST);

    // shaders (paths relative to build dir; in your project we used ../shaders and ../resources)
    Shader lightingShader("../shaders/colors.vs", "../shaders/colors.fs");
    Shader lightCubeShader("../shaders/light_cube.vs", "../shaders/light_cube.fs");

    // cube vertex data: pos(3) normal(3) uv(2) => 8 floats per vertex, 36 vertices
    float vertices[] = {
        // back face
        -0.5f,-0.5f,-0.5f,   0.0f, 0.0f,-1.0f,   0.0f,0.0f,
         0.5f,-0.5f,-0.5f,   0.0f, 0.0f,-1.0f,   1.0f,0.0f,
         0.5f, 0.5f,-0.5f,   0.0f, 0.0f,-1.0f,   1.0f,1.0f,
         0.5f, 0.5f,-0.5f,   0.0f, 0.0f,-1.0f,   1.0f,1.0f,
        -0.5f, 0.5f,-0.5f,   0.0f, 0.0f,-1.0f,   0.0f,1.0f,
        -0.5f,-0.5f,-0.5f,   0.0f, 0.0f,-1.0f,   0.0f,0.0f,
        // front face
        -0.5f,-0.5f, 0.5f,   0.0f, 0.0f, 1.0f,   0.0f,0.0f,
         0.5f,-0.5f, 0.5f,   0.0f, 0.0f, 1.0f,   1.0f,0.0f,
         0.5f, 0.5f, 0.5f,   0.0f, 0.0f, 1.0f,   1.0f,1.0f,
         0.5f, 0.5f, 0.5f,   0.0f, 0.0f, 1.0f,   1.0f,1.0f,
        -0.5f, 0.5f, 0.5f,   0.0f, 0.0f, 1.0f,   0.0f,1.0f,
        -0.5f,-0.5f, 0.5f,   0.0f, 0.0f, 1.0f,   0.0f,0.0f,
        // left face
        -0.5f, 0.5f, 0.5f,  -1.0f, 0.0f, 0.0f,   1.0f,0.0f,
        -0.5f, 0.5f,-0.5f,  -1.0f, 0.0f, 0.0f,   1.0f,1.0f,
        -0.5f,-0.5f,-0.5f,  -1.0f, 0.0f, 0.0f,   0.0f,1.0f,
        -0.5f,-0.5f,-0.5f,  -1.0f, 0.0f, 0.0f,   0.0f,1.0f,
        -0.5f,-0.5f, 0.5f,  -1.0f, 0.0f, 0.0f,   0.0f,0.0f,
        -0.5f, 0.5f, 0.5f,  -1.0f, 0.0f, 0.0f,   1.0f,0.0f,
        // right face
         0.5f, 0.5f, 0.5f,   1.0f, 0.0f, 0.0f,   1.0f,0.0f,
         0.5f, 0.5f,-0.5f,   1.0f, 0.0f, 0.0f,   1.0f,1.0f,
         0.5f,-0.5f,-0.5f,   1.0f, 0.0f, 0.0f,   0.0f,1.0f,
         0.5f,-0.5f,-0.5f,   1.0f, 0.0f, 0.0f,   0.0f,1.0f,
         0.5f,-0.5f, 0.5f,   1.0f, 0.0f, 0.0f,   0.0f,0.0f,
         0.5f, 0.5f, 0.5f,   1.0f, 0.0f, 0.0f,   1.0f,0.0f,
        // bottom face
        -0.5f,-0.5f,-0.5f,   0.0f,-1.0f, 0.0f,   0.0f,1.0f,
         0.5f,-0.5f,-0.5f,   0.0f,-1.0f, 0.0f,   1.0f,1.0f,
         0.5f,-0.5f, 0.5f,   0.0f,-1.0f, 0.0f,   1.0f,0.0f,
         0.5f,-0.5f, 0.5f,   0.0f,-1.0f, 0.0f,   1.0f,0.0f,
        -0.5f,-0.5f, 0.5f,   0.0f,-1.0f, 0.0f,   0.0f,0.0f,
        -0.5f,-0.5f,-0.5f,   0.0f,-1.0f, 0.0f,   0.0f,1.0f,
        // top face
        -0.5f, 0.5f,-0.5f,   0.0f, 1.0f, 0.0f,   0.0f,1.0f,
         0.5f, 0.5f,-0.5f,   0.0f, 1.0f, 0.0f,   1.0f,1.0f,
         0.5f, 0.5f, 0.5f,   0.0f, 1.0f, 0.0f,   1.0f,0.0f,
         0.5f, 0.5f, 0.5f,   0.0f, 1.0f, 0.0f,   1.0f,0.0f,
        -0.5f, 0.5f, 0.5f,   0.0f, 1.0f, 0.0f,   0.0f,0.0f,
        -0.5f, 0.5f,-0.5f,   0.0f, 1.0f, 0.0f,   0.0f,1.0f
    };

    // VAOs / VBO
    unsigned int VBO, cubeVAO;
    glGenVertexArrays(1, &cubeVAO);
    glGenBuffers(1, &VBO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindVertexArray(cubeVAO);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    unsigned int lightCubeVAO;
    glGenVertexArrays(1, &lightCubeVAO);
    glBindVertexArray(lightCubeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // load textures
    unsigned int diffuseMap  = loadTexture("../resources/container2.png");
    unsigned int specularMap = loadTexture("../resources/container2_specular.png");

    // ---------------------------
    // Build voxel world (no-arg ctor)
    // ---------------------------
    VoxelWorld world;
    world.generateTerrainMidLevel(
        2026,   // seed
        50,     // baseY (middle-ish)
        0.08,   // noiseScale
        20.0,   // amplitude
        5, 2.0, 0.5
    );

    // Precompute positions of all solid blocks once (naive rendering)
    std::vector<glm::vec3> blockPositions;
    blockPositions.reserve(world.countSolid());

    // Center the 100^3 world around origin: x,y,z in [-49.5, +49.5]
    const float cx = (VoxelWorld::SX - 1) * 0.5f;
    const float cy = (VoxelWorld::SY - 1) * 0.5f;
    const float cz = (VoxelWorld::SZ - 1) * 0.5f;

    for (int z = 0; z < VoxelWorld::SZ; ++z) {
        for (int y = 0; y < VoxelWorld::SY; ++y) {
            for (int x = 0; x < VoxelWorld::SX; ++x) {
                if (!world.hasBlock(x, y, z)) continue;
                blockPositions.emplace_back(
                    (float)x - cx,
                    (float)y - cy,
                    (float)z - cz
                );
            }
        }
    }

    std::cout << "Solid blocks: " << blockPositions.size() << "\n";

    // Orbit settings (tweak these)
    const glm::vec3 orbitCenter(0.0f, 0.0f, 0.0f); // world is centered around origin
    const float orbitRadius = 120.0f;              // how far from center
    const float orbitHeightBias = 20.0f;           // lift the whole orbit up/down
    const float dayLengthSec = 60.0f;              // one full day-night cycle = 60 seconds
    const float omega = 6.28318530718f / dayLengthSec; // 2*pi / period

    // render loop
    while (!glfwWindowShouldClose(window))
    {
        // timing
        float currentFrame = (float)glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window);

        glClearColor(0.07f, 0.07f, 0.10f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        float t = (float)glfwGetTime();
        float angle = t * omega;

        // Vertical orbit in the YZ plane (sun rises/sets)
        glm::vec3 sunPos  = orbitCenter + glm::vec3(0.0f,
                                                    std::sin(angle) * orbitRadius + orbitHeightBias,
                                                    std::cos(angle) * orbitRadius);

        glm::vec3 moonPos = orbitCenter + glm::vec3(0.0f,
                                                    std::sin(angle + 3.14159265359f) * orbitRadius + orbitHeightBias,
                                                    std::cos(angle + 3.14159265359f) * orbitRadius);

        // yFactor: >0 means above horizon, <=0 means below horizon
        float sunUp  = std::max(0.0f, std::sin(angle));
        float moonUp = std::max(0.0f, std::sin(angle + 3.14159265359f));

        // Optional: make transitions smoother (gamma-like curve)
        sunUp  = sunUp  * sunUp;
        moonUp = moonUp * moonUp;



        // -------------------
        // render voxel cubes
        // -------------------
        lightingShader.use();

        lightingShader.setInt("material.diffuse", 0);
        lightingShader.setInt("material.specular", 1);
        lightingShader.setFloat("material.shininess", 64.0f);

        // directional light
        lightingShader.setVec3("dirLight.direction", 0.0f, 0.0f, -1.0f);
        lightingShader.setVec3("dirLight.ambient",   0.05f, 0.05f, 0.05f);
        lightingShader.setVec3("dirLight.diffuse",   0.4f,  0.4f,  0.4f);
        lightingShader.setVec3("dirLight.specular",  0.5f,  0.5f,  0.5f);

        // Attenuation (make it decay slower so it can light a 100^3 world)
        const float kC = 1.0f;
        const float kL = 0.0f;   // slower decay
        const float kQ = 0.0f;

        // --- Sun (warm, bright) ---
        {
            std::string base = "pointLights[0].";
            lightingShader.setVec3((base + "position").c_str(), sunPos);

            glm::vec3 sunAmbient  = glm::vec3(0.02f, 0.015f, 0.010f) * sunUp;
            glm::vec3 sunDiffuse  = glm::vec3(1.00f, 0.85f, 0.65f)  * (1.2f * sunUp);
            glm::vec3 sunSpecular = glm::vec3(1.00f, 0.95f, 0.85f)  * (1.2f * sunUp);

            lightingShader.setVec3((base + "ambient").c_str(),  sunAmbient);
            lightingShader.setVec3((base + "diffuse").c_str(),  sunDiffuse);
            lightingShader.setVec3((base + "specular").c_str(), sunSpecular);
            lightingShader.setFloat((base + "constant").c_str(),  kC);
            lightingShader.setFloat((base + "linear").c_str(),    kL);
            lightingShader.setFloat((base + "quadratic").c_str(), kQ);
        }

        // --- Moon (cool, dim) ---
        {
            std::string base = "pointLights[1].";
            lightingShader.setVec3((base + "position").c_str(), moonPos);

            glm::vec3 moonAmbient  = glm::vec3(0.01f, 0.01f, 0.02f) * moonUp;
            glm::vec3 moonDiffuse  = glm::vec3(0.35f, 0.45f, 0.90f) * (0.5f * moonUp);
            glm::vec3 moonSpecular = glm::vec3(0.40f, 0.50f, 1.00f) * (0.6f * moonUp);

            lightingShader.setVec3((base + "ambient").c_str(),  moonAmbient);
            lightingShader.setVec3((base + "diffuse").c_str(),  moonDiffuse);
            lightingShader.setVec3((base + "specular").c_str(), moonSpecular);
            lightingShader.setFloat((base + "constant").c_str(),  kC);
            lightingShader.setFloat((base + "linear").c_str(),    kL);
            lightingShader.setFloat((base + "quadratic").c_str(), kQ);
        }

        // flashlight from camera (if your colors.fs includes spotLight)
        lightingShader.setVec3("spotLight.position",  camera.Position);
        lightingShader.setVec3("spotLight.direction", camera.Front);
        lightingShader.setVec3("spotLight.ambient",   0.0f, 0.0f, 0.0f);
        lightingShader.setVec3("spotLight.diffuse",   1.0f, 1.0f, 1.0f);
        lightingShader.setVec3("spotLight.specular",  1.0f, 1.0f, 1.0f);
        lightingShader.setFloat("spotLight.constant",  1.0f);
        lightingShader.setFloat("spotLight.linear",    0.09f);
        lightingShader.setFloat("spotLight.quadratic", 0.032f);
        lightingShader.setFloat("spotLight.cutOff",      glm::cos(glm::radians(12.5f)));
        lightingShader.setFloat("spotLight.outerCutOff", glm::cos(glm::radians(15.0f)));

        lightingShader.setVec3("viewPos", camera.Position);

        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom),
                                                (float)SCR_WIDTH / (float)SCR_HEIGHT,
                                                0.1f, 500.0f);
        glm::mat4 view = camera.GetViewMatrix();
        lightingShader.setMat4("projection", projection);
        lightingShader.setMat4("view", view);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, diffuseMap);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, specularMap);

        glBindVertexArray(cubeVAO);

        for (const glm::vec3& pos : blockPositions)
        {
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, pos);
            lightingShader.setMat4("model", model);

            glDrawArrays(GL_TRIANGLES, 0, 36);
        }

        // -------------------
        // render light cubes
        // -------------------
        lightCubeShader.use();
        lightCubeShader.setMat4("projection", projection);
        lightCubeShader.setMat4("view", view);

        glBindVertexArray(lightCubeVAO);

        // sun cube
        {
            glm::mat4 m = glm::mat4(1.0f);
            m = glm::translate(m, sunPos);
            m = glm::scale(m, glm::vec3(1.0f));
            lightCubeShader.setMat4("model", m);
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }

        // moon cube
        {
            glm::mat4 m = glm::mat4(1.0f);
            m = glm::translate(m, moonPos);
            m = glm::scale(m, glm::vec3(0.8f));
            lightCubeShader.setMat4("model", m);
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &cubeVAO);
    glDeleteVertexArrays(1, &lightCubeVAO);
    glDeleteBuffers(1, &VBO);

    glfwTerminate();
    return 0;
}

// ------------------------
// input & callbacks
// ------------------------
static void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, deltaTime);
}

static void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

static void mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
{
    float xpos = (float)xposIn;
    float ypos = (float)yposIn;

    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;

    lastX = xpos;
    lastY = ypos;

    camera.ProcessMouseMovement(xoffset, yoffset);
}

static void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    (void)xoffset;
    camera.ProcessMouseScroll((float)yoffset);
}

// ------------------------
// texture loading
// ------------------------
static unsigned int loadTexture(const char* path)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load(path, &width, &height, &nrComponents, 0);

    if (data)
    {
        GLenum format = GL_RGB;
        if (nrComponents == 1) format = GL_RED;
        else if (nrComponents == 3) format = GL_RGB;
        else if (nrComponents == 4) format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0,
                     format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }
    else
    {
        std::cout << "Texture failed to load at path: " << path << "\n";
        stbi_image_free(data);
    }

    return textureID;
}