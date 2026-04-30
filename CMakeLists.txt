cmake_minimum_required(VERSION 3.10)
project(minecraft_clone)

set(CMAKE_CXX_STANDARD 17)

find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED)

include_directories(${CMAKE_SOURCE_DIR})
include_directories(${CMAKE_SOURCE_DIR}/external/glad/include)
include_directories(${CMAKE_SOURCE_DIR}/external)
include_directories(${CMAKE_SOURCE_DIR}/include)

add_executable(venture
        src/main.cpp
        external/glad/src/glad.c
        src/stb_image.cpp
        learnopengl/shader_m.h
        learnopengl/camera.h
        include/world/VoxelWorld.h
        src/world/VoxelWorld.cpp
        include/physics/VoxelPhysics.h
        include/physics/PhysicsBody.h
        include/physics/Player.h
        include/world/Chunk.h
        src/world/Chunk.cpp
        include/world/TerrainGenerator.h
        src/world/TerrainGenerator.cpp
        include/world/ChunkManager.h
        src/world/ChunkManager.cpp
        include/world/WorldMesher.h
        src/world/WorldMesher.cpp
        include/world/WorldRaycaster.h
        src/world/WorldRaycaster.cpp
)

target_link_libraries(venture
        glfw
        ${OPENGL_gl_LIBRARY}
)
