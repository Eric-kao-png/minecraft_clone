cmake_minimum_required(VERSION 3.10)
project(minecraft_clone)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED)

add_executable(minecraft_clone
        src/main.cpp
        src/app/Application.cpp
        src/game/GameInput.cpp
        src/render/TextureLoader.cpp
        src/render/DayNightLighting.cpp
        external/glad/src/glad.c
        src/StbImage.cpp
        src/world/VoxelWorld.cpp
        src/world/WorldCoord.cpp
        src/world/BlockAtlas.cpp
        src/world/Chunk.cpp
        src/world/TerrainGenerator.cpp
        src/world/ChunkManager.cpp
        src/world/WorldMesher.cpp
        src/world/WorldRaycaster.cpp
        src/physics/PhysicsBody.cpp
        src/physics/Player.cpp
)

target_include_directories(minecraft_clone PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/engine
        ${CMAKE_SOURCE_DIR}/external
        ${CMAKE_SOURCE_DIR}/external/glad/include
        ${CMAKE_SOURCE_DIR}/external/stb
)

target_link_libraries(minecraft_clone PRIVATE glfw ${OPENGL_gl_LIBRARY})
