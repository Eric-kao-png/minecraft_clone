#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stb_image.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <../learnopengl/shader_m.h>
#include <../learnopengl/camera.h>

#include "world/VoxelWorld.h"
#include "physics/Player.h"

#include <iostream>
#include <vector>
#include <string>
#include <cmath>
#include <algorithm>

// callbacks / helpers
static void framebuffer_size_callback(GLFWwindow* window, int width, int height);
static void mouse_callback(GLFWwindow* window, double xpos, double ypos);
static void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
static void processInput(GLFWwindow* window);
static unsigned int loadTexture(const char* path);

// settings
static const unsigned int SCR_WIDTH  = 1280;
static const unsigned int SCR_HEIGHT = 720;

// camera
static Camera camera(glm::vec3(0.0f, 25.0f, 0.0f),
                     glm::vec3(0.0f, 1.0f, 0.0f),
                     -90.0f, -20.0f);
static float lastX = SCR_WIDTH / 2.0f;
static float lastY = SCR_HEIGHT / 2.0f;
static bool firstMouse = true;

// timing
static float deltaTime = 0.0f;
static float lastFrame = 0.0f;

// -----------------------
// Player physics (AABB + gravity + voxel collisions)
// -----------------------
static Player gPlayer;

struct InputState {
    bool forward = false;
    bool backward = false;
    bool left = false;
    bool right = false;
    bool sprint = false;
    bool jumpPressed = false; // edge-triggered
};

static InputState gInput;
static bool gSpaceWasDown = false;

// -----------------------
// World + mesh globals
// -----------------------
static VoxelWorld gWorld;
static std::vector<float> gMesh;     // [x y z nx ny nz u v]...
static GLsizei gMeshVertexCount = 0;
static unsigned int gMeshVAO = 0;
static unsigned int gMeshVBO = 0;
static bool gMeshDirty = false;

// World is centered around origin in render-space
static constexpr float CX = (VoxelWorld::SX - 1) * 0.5f;
static constexpr float CY = (VoxelWorld::SY - 1) * 0.5f;
static constexpr float CZ = (VoxelWorld::SZ - 1) * 0.5f;

// -----------------------
// Meshing (only faces adjacent to air)
// -----------------------
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

    for (int z = 0; z < VoxelWorld::SZ; ++z) {
        for (int y = 0; y < VoxelWorld::SY; ++y) {
            for (int x = 0; x < VoxelWorld::SX; ++x) {
                if (!world.hasBlock(x, y, z)) continue;

                float px = (float)x - CX;
                float py = (float)y - CY;
                float pz = (float)z - CZ;

                for (const auto& f : faces) {
                    int nxCell = x + f.dx;
                    int nyCell = y + f.dy;
                    int nzCell = z + f.dz;

                    if (!isAir(world, nxCell, nyCell, nzCell))
                        continue;

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

static void uploadMeshToGPU() {
    gMeshVertexCount = (GLsizei)(gMesh.size() / 8);

    glBindBuffer(GL_ARRAY_BUFFER, gMeshVBO);
    glBufferData(GL_ARRAY_BUFFER,
                 gMesh.size() * sizeof(float),
                 gMesh.empty() ? nullptr : gMesh.data(),
                 GL_DYNAMIC_DRAW);
}

static void rebuildWorldMesh() {
    gMesh = buildVisibleFaceMesh(gWorld);
    uploadMeshToGPU();
    std::cout << "Rebuilt mesh. Vertices: " << gMeshVertexCount << "\n";
}

// -----------------------
// Simple step raycast
// -----------------------
struct RaycastHit {
    bool hit = false;
    glm::ivec3 block{0};     // voxel cell index
    glm::ivec3 normal{0};    // face normal from hit block toward air
    float t = 0.0f;
};

static glm::ivec3 worldPosToCell(const glm::vec3& p) {
    return glm::ivec3(
        (int)std::floor(p.x + CX + 0.5f),
        (int)std::floor(p.y + CY + 0.5f),
        (int)std::floor(p.z + CZ + 0.5f)
    );
}

static glm::ivec3 approxHitNormalFromDir(const glm::vec3& dir) {
    glm::vec3 a = glm::abs(dir);
    if (a.x >= a.y && a.x >= a.z) return glm::ivec3(dir.x > 0 ? -1 : 1, 0, 0);
    if (a.y >= a.x && a.y >= a.z) return glm::ivec3(0, dir.y > 0 ? -1 : 1, 0);
    return glm::ivec3(0, 0, dir.z > 0 ? -1 : 1);
}

static bool raycastStep(const VoxelWorld& world,
                        const glm::vec3& origin,
                        const glm::vec3& dir,
                        float maxDist,
                        float step,
                        RaycastHit& out) {
    glm::vec3 d = glm::normalize(dir);

    glm::ivec3 lastCell = worldPosToCell(origin);
    glm::ivec3 prevCell = lastCell;
    bool hasPrev = false;

    for (float t = 0.0f; t <= maxDist; t += step) {
        glm::vec3 p = origin + d * t;
        glm::ivec3 cell = worldPosToCell(p);

        if (cell != lastCell) {
            prevCell = lastCell;
            hasPrev = true;
            lastCell = cell;
        }

        if (!world.inBounds(cell.x, cell.y, cell.z))
            continue;

        if (world.hasBlock(cell.x, cell.y, cell.z)) {
            out.hit = true;
            out.block = cell;
            out.t = t;
            out.normal = hasPrev ? (prevCell - cell) : approxHitNormalFromDir(d);
            if (out.normal == glm::ivec3(0)) out.normal = approxHitNormalFromDir(d);
            return true;
        }
    }

    return false;
}

static bool editBlock(bool place) {
    const float maxReach = 6.0f;
    const float step = 0.10f;

    RaycastHit hit;
    if (!raycastStep(gWorld, camera.Position, camera.Front, maxReach, step, hit))
        return false;

    if (!place) {
        gWorld.setBlock(hit.block.x, hit.block.y, hit.block.z, false);
        return true;
    }

    glm::ivec3 placeCell = hit.block + hit.normal;
    if (!gWorld.inBounds(placeCell.x, placeCell.y, placeCell.z))
        return false;
    if (gWorld.hasBlock(placeCell.x, placeCell.y, placeCell.z))
        return false;

    // Don't allow placing a block inside the player
    voxel_physics::AABB playerBox = gPlayer.aabb();
    voxel_physics::AABB blockBox  = voxel_physics::blockAABB(placeCell.x, placeCell.y, placeCell.z);
    if (voxel_physics::overlap(playerBox, blockBox))
        return false;

    gWorld.setBlock(placeCell.x, placeCell.y, placeCell.z, true);
    return true;
}

int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Voxel World (DirLight Orbit in XZ-diagonal plane)", NULL, NULL);
    if (window == NULL) {
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
    Shader lightCubeShader("../shaders/light_cube.vs", "../shaders/light_cube.fs");

    unsigned int diffuseMap  = loadTexture("../resources/container2.png");
    unsigned int specularMap = loadTexture("../resources/container2_specular.png");

    // World
    gWorld.generateTerrainMidLevel(2026, 50, 0.08, 20.0, 5, 2.0, 0.5);

    // Spawn player at the center, standing on terrain
    {
        const int sx = VoxelWorld::SX / 2;
        const int sz = VoxelWorld::SZ / 2;

        int topY = -1;
        for (int y = VoxelWorld::SY - 1; y >= 0; --y) {
            if (gWorld.hasBlock(sx, y, sz)) {
                topY = y;
                break;
            }
        }

        float groundTop = (topY >= 0) ? ((float)topY - CY + 0.5f) : 0.0f;
        gPlayer.position = glm::vec3((float)sx - CX,
                                    groundTop + gPlayer.halfSize.y + voxel_physics::EPS,
                                    (float)sz - CZ);
        gPlayer.velocity = glm::vec3(0.0f);
        gPlayer.onGround = true;

        camera.Position = gPlayer.eyePosition();
    }

    // Initial mesh
    gMesh = buildVisibleFaceMesh(gWorld);

    glGenVertexArrays(1, &gMeshVAO);
    glGenBuffers(1, &gMeshVBO);

    glBindVertexArray(gMeshVAO);
    glBindBuffer(GL_ARRAY_BUFFER, gMeshVBO);

    // Upload initial mesh
    uploadMeshToGPU();

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    // Light cubes VAO/VBO (positions only)
    float lampCubeVertices[] = {
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

    // ----------------------
    // Day/Night (Directional Light)
    // ----------------------
    const glm::vec3 orbitCenter(0.0f, 0.0f, 0.0f);
    const float orbitRadius = 120.0f;
    const float orbitHeightBias = 20.0f;

    const float dayLengthSec = 60.0f;
    const float omega = 6.28318530718f / dayLengthSec;

    // Orbit plane is spanned by:
    //  - Up axis (Y)
    //  - A diagonal horizontal axis between X and Z
    // This ensures sun/moon direction has BOTH x and z components during the cycle,
    // so Z faces won't be permanently dark.
    const glm::vec3 orbitUp(0.0f, 1.0f, 0.0f);
    glm::vec3 orbitDiagXZ = glm::normalize(glm::vec3(1.0f, 0.0f, 1.0f)); // between +X and +Z

    // (Optional) if you want more Z influence than X, you can tweak it:
    // orbitDiagXZ = glm::normalize(glm::vec3(0.6f, 0.0f, 0.8f));

    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = (float)glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window);

        // --- Player control + physics ---
        {
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

            float w2 = glm::dot(wish, wish);
            if (w2 > 1e-8f) wish /= std::sqrt(w2);

            gPlayer.applyControl(wish, gInput.jumpPressed, gInput.sprint, deltaTime);
            gPlayer.step(gWorld, deltaTime);

            camera.Position = gPlayer.eyePosition();
        }

        if (gMeshDirty) {
            rebuildWorldMesh();
            gMeshDirty = false;
        }

        glClearColor(0.07f, 0.07f, 0.10f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // ----- Orbit update -----
        float t = (float)glfwGetTime();
        float angle = t * omega;

        // Circle in the plane spanned by (orbitDiagXZ, orbitUp)
        // offset = R * (cos(angle)*diag + sin(angle)*up)
        glm::vec3 sunOffset  = orbitRadius * (std::cos(angle) * orbitDiagXZ + std::sin(angle) * orbitUp);
        glm::vec3 moonOffset = -sunOffset;

        // Visual positions (for cubes)
        // glm::vec3 sunPos  = orbitCenter + sunOffset  + glm::vec3(0.0f, orbitHeightBias, 0.0f);
        // glm::vec3 moonPos = orbitCenter + moonOffset + glm::vec3(0.0f, orbitHeightBias, 0.0f);

        // Directional light ray direction (light -> scene)
        // Use offset WITHOUT height bias so day/night comes from the orbit itself.
        glm::vec3 sunRayDir  = glm::normalize(-sunOffset);
        glm::vec3 moonRayDir = glm::normalize(-moonOffset);

        // Strength based on whether rays point downward
        float sunUp  = std::max(0.0f, -sunRayDir.y);
        float moonUp = std::max(0.0f, -moonRayDir.y);

        // Curves
        float sunStrength  = sunUp * sunUp;
        float moonStrength = std::sqrt(moonUp);

        // Blend direction smoothly (avoid sudden flip near horizon)
        glm::vec3 mixedDir = glm::vec3(0.0f);
        float wSum = sunStrength + moonStrength;
        if (wSum > 1e-6f) {
            mixedDir = glm::normalize(sunRayDir * sunStrength + moonRayDir * moonStrength);
        } else {
            mixedDir = sunRayDir; // fallback
        }

        // Colors (same style as before)
        glm::vec3 baseAmbient(0.012f);

        glm::vec3 sunAmb = glm::vec3(0.02f, 0.015f, 0.010f) * sunStrength;
        glm::vec3 sunDif = glm::vec3(1.00f, 0.85f, 0.65f)  * (1.25f * sunStrength);
        glm::vec3 sunSpe = glm::vec3(1.00f, 0.95f, 0.85f)  * (1.10f * sunStrength);

        glm::vec3 moonAmb = glm::vec3(0.008f, 0.010f, 0.020f) * moonStrength;
        glm::vec3 moonDif = glm::vec3(0.25f,  0.35f,  0.90f)  * (1.20f * moonStrength);
        glm::vec3 moonSpe = glm::vec3(0.30f,  0.40f,  1.00f)  * (1.10f * moonStrength);

        glm::vec3 dirAmbient  = baseAmbient + sunAmb + moonAmb;
        glm::vec3 dirDiffuse  = sunDif + moonDif;
        glm::vec3 dirSpecular = sunSpe + moonSpe;

        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom),
                                                (float)SCR_WIDTH / (float)SCR_HEIGHT,
                                                0.1f, 500.0f);
        glm::mat4 view = camera.GetViewMatrix();

        // ----- World mesh lighting -----
        lightingShader.use();

        lightingShader.setInt("material.diffuse", 0);
        lightingShader.setInt("material.specular", 1);
        lightingShader.setFloat("material.shininess", 64.0f);

        // Directional light = blended Sun/Moon
        lightingShader.setVec3("dirLight.direction", mixedDir);
        lightingShader.setVec3("dirLight.ambient",   dirAmbient);
        lightingShader.setVec3("dirLight.diffuse",   dirDiffuse);
        lightingShader.setVec3("dirLight.specular",  dirSpecular);

        // Spot light (flashlight)
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
        lightingShader.setMat4("model", glm::mat4(1.0f));

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, diffuseMap);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, specularMap);

        glBindVertexArray(gMeshVAO);
        glDrawArrays(GL_TRIANGLES, 0, gMeshVertexCount);

        // ----- Visual sun/moon cubes -----
        // lightCubeShader.use();
        // lightCubeShader.setMat4("projection", projection);
        // lightCubeShader.setMat4("view", view);
        //
        // glBindVertexArray(lightCubeVAO);
        //
        // {   // sun cube
        //     glm::mat4 m = glm::mat4(1.0f);
        //     m = glm::translate(m, sunPos);
        //     m = glm::scale(m, glm::vec3(1.2f));
        //     lightCubeShader.setMat4("model", m);
        //     glDrawArrays(GL_TRIANGLES, 0, 36);
        // }
        // {   // moon cube
        //     glm::mat4 m = glm::mat4(1.0f);
        //     m = glm::translate(m, moonPos);
        //     m = glm::scale(m, glm::vec3(1.0f));
        //     lightCubeShader.setMat4("model", m);
        //     glDrawArrays(GL_TRIANGLES, 0, 36);
        // }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &gMeshVAO);
    glDeleteBuffers(1, &gMeshVBO);
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

    gInput.forward  = glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS;
    gInput.backward = glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS;
    gInput.left     = glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS;
    gInput.right    = glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS;

    gInput.sprint = glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS;

    // edge-trigger jump
    bool spaceDown = glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS;
    gInput.jumpPressed = spaceDown && !gSpaceWasDown;
    gSpaceWasDown = spaceDown;
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

static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    (void)window; (void)mods;
    if (action != GLFW_PRESS) return;

    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (editBlock(false)) gMeshDirty = true;
    } else if (button == GLFW_MOUSE_BUTTON_RIGHT) {
        if (editBlock(true)) gMeshDirty = true;
    }
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