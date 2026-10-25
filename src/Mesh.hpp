#pragma once
#include <glad/gl.h>

// Mesh don gian: vertex = pos(3) + normal(3) + uv(2)
struct Mesh {
    GLuint vao = 0, vbo = 0;
    GLsizei count = 0;

    void draw() const;
    void destroy();
};

Mesh createCubeMesh();   // cube don vi, tam o goc
Mesh createQuadMesh();   // quad don vi tren mat phang XY, normal +Z
