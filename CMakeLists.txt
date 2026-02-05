cmake_minimum_required(VERSION 3.10)
project(venture)

set(CMAKE_CXX_STANDARD 17)

# OpenGL
find_package(OpenGL REQUIRED)

# GLFW (brew install glfw)
find_package(glfw3 REQUIRED)

# GLAD (local folder)
include_directories(${CMAKE_SOURCE_DIR})
include_directories(${CMAKE_SOURCE_DIR}/external/glad/include)
include_directories(${CMAKE_SOURCE_DIR}/external)
include_directories(${CMAKE_SOURCE_DIR}/include)


add_executable(venture
        src/main.cpp
        external/glad/src/glad.c
        include/Shader.h
        src/stb_image.cpp
        learnopengl/shader_m.h
        learnopengl/camera.h
)

target_link_libraries(venture
        glfw
        ${OPENGL_gl_LIBRARY}
)
