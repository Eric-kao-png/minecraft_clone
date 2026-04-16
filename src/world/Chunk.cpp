#include "world/Chunk.h"
#include <glad/glad.h>
#include <cstddef>

Chunk::Chunk(int cx, int cz)
    : coord{cx, cz},
      blocks(static_cast<std::size_t>(SIZE_X) * SIZE_Y * SIZE_Z, 0) {}

Chunk::~Chunk() {
    clearOpenGL();
}

uint8_t& Chunk::at(int lx, int y, int lz) {
    return blocks[(static_cast<std::size_t>(y) * SIZE_Z + lz) * SIZE_X + lx];
}

uint8_t Chunk::at(int lx, int y, int lz) const {
    return blocks[(static_cast<std::size_t>(y) * SIZE_Z + lz) * SIZE_X + lx];
}

void Chunk::uploadMesh() {
    if (vao == 0) {
        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);

        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);

        // Position: 3 floats, Normal: 3 floats, UV: 2 floats (Total 8 floats)
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
        glEnableVertexAttribArray(2);
    }

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(mesh.size() * sizeof(float)),
                 mesh.empty() ? nullptr : mesh.data(),
                 GL_DYNAMIC_DRAW);
    
    vertexCount = static_cast<int>(mesh.size() / 8);
    dirty = false;
}

void Chunk::render() const {
    if (vertexCount > 0 && vao != 0) {
        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLES, 0, vertexCount);
    }
}

void Chunk::clearOpenGL() {
    if (vao != 0) {
        glDeleteVertexArrays(1, &vao);
        glDeleteBuffers(1, &vbo);
        vao = 0;
        vbo = 0;
    }
}