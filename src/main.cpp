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
#include <algorithm>

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
static Camera camera(glm::vec3(0.0f, 25.0f, 80.0f),
                     glm::vec3(0.0f, 1.0f, 0.0f),
                     -90.0f, -20.0f);

static float lastX = SCR_WIDTH / 2.0f;
static float lastY = SCR_HEIGHT / 2.0f;
static bool firstMouse = true;

// timing
static float deltaTime = 0.0f;
static float lastFrame = 0.0f;

// --------- Meshing (only faces next to air) ---------
struct FaceDef {
    int dx, dy, dz;        // neighbor direction
    float nx, ny, nz;      // face normal
    // 6 vertices; each vertex: offset(x,y,z,u,v)
    float v[6][5];
};

static bool isAir(const VoxelWorld& w, int x, int y, int z) {
    if (!w.inBounds(x, y, z)) return true;
    return !w.hasBlock(x, y, z);
}

static void pushVertex(std::vector<float>& out,
                       float px, float py, float pz,
                       float nx, float ny, float nz,
                       float u, float v) {
    out.push_back(px); out.push_back(py); out.push_back(pz);
    out.push_back(nx); out.push_back(ny); out.push_back(nz);
    out.push_back(u);  out.push_back(v);
}

static std::vector<float> buildVisibleFaceMesh(const VoxelWorld& world) {
    static const FaceDef faces[6] = {
        // -Z (back)
        { 0, 0,-1,  0, 0,-1, {
            {-0.5f,-0.5f,-0.5f, 0,0}, { 0.5f,-0.5f,-0.5f, 1,0}, { 0.5f, 0.5f,-0.5f, 1,1},
            { 0.5f, 0.5f,-0.5f, 1,1}, {-0.5f, 0.5f,-0.5f, 0,1}, {-0.5f,-0.5f,-0.5f, 0,0},
        }},
        // +Z (front)
        { 0, 0, 1,  0, 0, 1, {
            {-0.5f,-0.5f, 0.5f, 0,0}, { 0.5f,-0.5f, 0.5f, 1,0}, { 0.5f, 0.5f, 0.5f, 1,1},
            { 0.5f, 0.5f, 0.5f, 1,1}, {-0.5f, 0.5f, 0.5f, 0,1}, {-0.5f,-0.5f, 0.5f, 0,0},
        }},
        // -X (left)
        {-1, 0, 0, -1, 0, 0, {
            {-0.5f, 0.5f, 0.5f, 1,0}, {-0.5f, 0.5f,-0.5f, 1,1}, {-0.5f,-0.5f,-0.5f, 0,1},
            {-0.5f,-0.5f,-0.5f, 0,1}, {-0.5f,-0.5f, 0.5f, 0,0}, {-0.5f, 0.5f, 0.5f, 1,0},
        }},
        // +X (right)
        { 1, 0, 0,  1, 0, 0, {
            { 0.5f, 0.5f, 0.5f, 1,0}, { 0.5f, 0.5f,-0.5f, 1,1}, { 0.5f,-0.5f,-0.5f, 0,1},
            { 0.5f,-0.5f,-0.5f, 0,1}, { 0.5f,-0.5f, 0.5f, 0,0}, { 0.5f, 0.5f, 0.5f, 1,0},
        }},
        // -Y (bottom)
        { 0,-1, 0,  0,-1, 0, {
            {-0.5f,-0.5f,-0.5f, 0,1}, { 0.5f,-0.5f,-0.5f, 1,1}, { 0.5f,-0.5f, 0.5f, 1,0},
            { 0.5f,-0.5f, 0.5f, 1,0}, {-0.5f,-0.5f, 0.5f, 0,0}, {-0.5f,-0.5f,-0.5f, 0,1},
        }},
        // +Y (top)
        { 0, 1, 0,  0, 1, 0, {
            {-0.5f, 0.5f,-0.5f, 0,1}, { 0.5f, 0.5f,-0.5f, 1,1}, { 0.5f, 0.5f, 0.5f, 1,0},
            { 0.5f, 0.5f, 0.5f, 1,0}, {-0.5f, 0.5f, 0.5f, 0,0}, {-0.5f, 0.5f,-0.5f, 0,1},
        }},
    };

    std::vector<float> mesh;
    mesh.reserve(2'000'000);

    const float cx = (VoxelWorld::SX - 1) * 0.5f;
    const float cy = (VoxelWorld::SY - 1) * 0.5f;
    const float cz = (VoxelWorld::SZ - 1) * 0.5f;

    for (int z = 0; z < VoxelWorld::SZ; ++z) {
        for (int y = 0; y < VoxelWorld::SY; ++y) {
            for (int x = 0; x < VoxelWorld::SX; ++x) {
                if (!world.hasBlock(x, y, z)) continue;

                float px = (float)x - cx;
                float py = (float)y - cy;
                float pz = (float)z - cz;

                for (const auto& f : faces) {
                    int nxCell = x + f.dx;
                    int nyCell = y + f.dy;
                    int nzCell = z + f.dz;

                    if (!isAir(world, nxCell, nyCell, nzCell))
                        continue;

                    // optional: don't render bottom faces at world bottom
                    // if (f.dy == -1 && y == 0) continue;

                    for (int i = 0; i < 6; ++i) {
                        float ox = f.v[i][0], oy = f.v[i][1], oz = f.v[i][2];
                        float u  = f.v[i][3], v  = f.v[i][4];
                        pushVertex(mesh,
                                   px + ox, py + oy, pz + oz,
                                   f.nx, f.ny, f.nz,
                                   u, v);
                    }
                }
            }
        }
    }

    return mesh;
}

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

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Voxel World (visible faces mesh)", NULL, NULL);
    if (window == NULL) {
        std::cout << "Failed to create GLFW window\n";
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD\n";
        return -1;
    }

    glEnable(GL_DEPTH_TEST);

    Shader lightingShader("../shaders/colors.vs", "../shaders/colors.fs");
    Shader lightCubeShader("../shaders/light_cube.vs", "../shaders/light_cube.fs");

    // Load textures
    unsigned int diffuseMap  = loadTexture("../resources/container2.png");
    unsigned int specularMap = loadTexture("../resources/container2_specular.png");

    // ---------------------------
    // Build voxel world (COLUMN terrain)
    // ---------------------------
    VoxelWorld world;
    world.generateTerrainMidLevel(
        2026,   // seed
        50,     // baseY
        0.08,   // noiseScale
        20.0,   // amplitude
        5, 2.0, 0.5
    );

    // Build optimized mesh (only faces adjacent to air)
    std::vector<float> mesh = buildVisibleFaceMesh(world);
    const GLsizei meshVertexCount = (GLsizei)(mesh.size() / 8);
    std::cout << "Mesh vertices: " << meshVertexCount << "\n";

    // Upload world mesh to GPU
    unsigned int meshVAO, meshVBO;
    glGenVertexArrays(1, &meshVAO);
    glGenBuffers(1, &meshVBO);

    glBindVertexArray(meshVAO);
    glBindBuffer(GL_ARRAY_BUFFER, meshVBO);
    glBufferData(GL_ARRAY_BUFFER, mesh.size() * sizeof(float), mesh.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);

    // A cube VBO just for drawing sun/moon "lamp cubes" (position only used)
    float lampCubeVertices[] = {
        // positions only (we can still keep stride=3 for lamp cube shader)
        -0.5f,-0.5f,-0.5f,  0.5f,-0.5f,-0.5f,  0.5f, 0.5f,-0.5f,
         0.5f, 0.5f,-0.5f, -0.5f, 0.5f,-0.5f, -0.5f,-0.5f,-0.5f,

        -0.5f,-0.5f, 0.5f,  0.5f,-0.5f, 0.5f,  0.5f, 0.5f, 0.5f,
         0.5f, 0.5f, 0.5f, -0.5f, 0.5f, 0.5f, -0.5f,-0.5f, 0.5f,

        -0.5f, 0.5f, 0.5f, -0.5f, 0.5f,-0.5f, -0.5f,-0.5f,-0.5f,
        -0.5f,-0.5f,-0.5f, -0.5f,-0.5f, 0.5f, -0.5f, 0.5f, 0.5f,

         0.5f, 0.5f, 0.5f,  0.5f, 0.5f,-0.5f,  0.5f,-0.5f,-0.5f,
         0.5f,-0.5f,-0.5f,  0.5f,-0.5f, 0.5f,  0.5f, 0.5f, 0.5f,

        -0.5f,-0.5f,-0.5f,  0.5f,-0.5f,-0.5f,  0.5f,-0.5f, 0.5f,
         0.5f,-0.5f, 0.5f, -0.5f,-0.5f, 0.5f, -0.5f,-0.5f,-0.5f,

        -0.5f, 0.5f,-0.5f,  0.5f, 0.5f,-0.5f,  0.5f, 0.5f, 0.5f,
         0.5f, 0.5f, 0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f,-0.5f
    };

    unsigned int lightCubeVAO, lightCubeVBO;
    glGenVertexArrays(1, &lightCubeVAO);
    glGenBuffers(1, &lightCubeVBO);
    glBindVertexArray(lightCubeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, lightCubeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(lampCubeVertices), lampCubeVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    // Orbit settings
    const glm::vec3 orbitCenter(0.0f, 0.0f, 0.0f);
    const float orbitRadius = 120.0f;
    const float orbitHeightBias = 20.0f;
    const float dayLengthSec = 60.0f;
    const float omega = 6.28318530718f / dayLengthSec;

    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = (float)glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window);

        glClearColor(0.07f, 0.07f, 0.10f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        float t = (float)glfwGetTime();
        float angle = t * omega;

        glm::vec3 sunPos  = orbitCenter + glm::vec3(0.0f,
                                                    std::sin(angle) * orbitRadius + orbitHeightBias,
                                                    std::cos(angle) * orbitRadius);
        glm::vec3 moonPos = orbitCenter + glm::vec3(0.0f,
                                                    std::sin(angle + 3.14159265359f) * orbitRadius + orbitHeightBias,
                                                    std::cos(angle + 3.14159265359f) * orbitRadius);

        float sunUp  = std::max(0.0f, std::sin(angle));
        float moonUp = std::max(0.0f, std::sin(angle + 3.14159265359f));
        sunUp  = sunUp  * sunUp;
        moonUp = moonUp * moonUp;

        // matrices
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom),
                                                (float)SCR_WIDTH / (float)SCR_HEIGHT,
                                                0.1f, 500.0f);
        glm::mat4 view = camera.GetViewMatrix();

        // -------------------
        // render world mesh
        // -------------------
        lightingShader.use();

        lightingShader.setInt("material.diffuse", 0);
        lightingShader.setInt("material.specular", 1);
        lightingShader.setFloat("material.shininess", 64.0f);

        // (optional) keep a very weak directional light as base fill
        lightingShader.setVec3("dirLight.direction", 0.0f, -1.0f, 0.0f);
        lightingShader.setVec3("dirLight.ambient",   0.01f, 0.01f, 0.01f);
        lightingShader.setVec3("dirLight.diffuse",   0.00f, 0.00f, 0.00f);
        lightingShader.setVec3("dirLight.specular",  0.00f, 0.00f, 0.00f);

        // Attenuation (slow decay for "sun/moon-like" point lights)
        const float kC = 1.0f;
        const float kL = 0.0020f;
        const float kQ = 0.00002f;

        // Sun = pointLights[0]
        {
            std::string base = "pointLights[0].";
            lightingShader.setVec3((base + "position").c_str(), sunPos);

            glm::vec3 amb = glm::vec3(0.02f, 0.015f, 0.010f) * sunUp;
            glm::vec3 dif = glm::vec3(1.00f, 0.85f, 0.65f)  * (1.4f * sunUp);
            glm::vec3 spe = glm::vec3(1.00f, 0.95f, 0.85f)  * (1.2f * sunUp);

            lightingShader.setVec3((base + "ambient").c_str(),  amb);
            lightingShader.setVec3((base + "diffuse").c_str(),  dif);
            lightingShader.setVec3((base + "specular").c_str(), spe);
            lightingShader.setFloat((base + "constant").c_str(),  kC);
            lightingShader.setFloat((base + "linear").c_str(),    kL);
            lightingShader.setFloat((base + "quadratic").c_str(), kQ);
        }

        // Moon = pointLights[1]
        {
            std::string base = "pointLights[1].";
            lightingShader.setVec3((base + "position").c_str(), moonPos);

            glm::vec3 amb = glm::vec3(0.005f, 0.008f, 0.015f) * moonUp;
            glm::vec3 dif = glm::vec3(0.20f,  0.30f,  0.70f)  * (1.2f * moonUp);
            glm::vec3 spe = glm::vec3(0.25f,  0.35f,  0.90f)  * (1.2f * moonUp);

            lightingShader.setVec3((base + "ambient").c_str(),  amb);
            lightingShader.setVec3((base + "diffuse").c_str(),  dif);
            lightingShader.setVec3((base + "specular").c_str(), spe);
            lightingShader.setFloat((base + "constant").c_str(),  kC);
            lightingShader.setFloat((base + "linear").c_str(),    kL);
            lightingShader.setFloat((base + "quadratic").c_str(), kQ);
        }

        // disable remaining point lights if shader expects 4
        for (int i = 2; i < 4; ++i) {
            std::string base = "pointLights[" + std::to_string(i) + "].";
            lightingShader.setVec3((base + "ambient").c_str(),  0.0f, 0.0f, 0.0f);
            lightingShader.setVec3((base + "diffuse").c_str(),  0.0f, 0.0f, 0.0f);
            lightingShader.setVec3((base + "specular").c_str(), 0.0f, 0.0f, 0.0f);
            lightingShader.setFloat((base + "constant").c_str(),  1.0f);
            lightingShader.setFloat((base + "linear").c_str(),    0.0f);
            lightingShader.setFloat((base + "quadratic").c_str(), 0.0f);
        }

        // flashlight from camera (keep if your colors.fs includes spotLight)
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
        lightingShader.setMat4("projection", projection);
        lightingShader.setMat4("view", view);

        // world mesh already in world coordinates => identity model
        glm::mat4 model = glm::mat4(1.0f);
        lightingShader.setMat4("model", model);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, diffuseMap);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, specularMap);

        glBindVertexArray(meshVAO);
        glDrawArrays(GL_TRIANGLES, 0, meshVertexCount);

        // -------------------
        // render light cubes (sun & moon)
        // -------------------
        lightCubeShader.use();
        lightCubeShader.setMat4("projection", projection);
        lightCubeShader.setMat4("view", view);

        glBindVertexArray(lightCubeVAO);

        {
            glm::mat4 m = glm::mat4(1.0f);
            m = glm::translate(m, sunPos);
            m = glm::scale(m, glm::vec3(1.2f));
            lightCubeShader.setMat4("model", m);
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }
        {
            glm::mat4 m = glm::mat4(1.0f);
            m = glm::translate(m, moonPos);
            m = glm::scale(m, glm::vec3(1.0f));
            lightCubeShader.setMat4("model", m);
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &meshVAO);
    glDeleteBuffers(1, &meshVBO);
    glDeleteVertexArrays(1, &lightCubeVAO);
    glDeleteBuffers(1, &lightCubeVBO);

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