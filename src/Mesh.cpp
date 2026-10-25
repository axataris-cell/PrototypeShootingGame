#include "Mesh.hpp"

#include <glm/glm.hpp>
#include <vector>

static Mesh uploadMesh(const std::vector<float>& data) {
    Mesh m;
    m.count = static_cast<GLsizei>(data.size() / 8);
    glGenVertexArrays(1, &m.vao);
    glGenBuffers(1, &m.vbo);
    glBindVertexArray(m.vao);
    glBindBuffer(GL_ARRAY_BUFFER, m.vbo);
    glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(float), data.data(), GL_STATIC_DRAW);
    const GLsizei stride = 8 * sizeof(float);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glBindVertexArray(0);
    return m;
}

void Mesh::draw() const {
    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, 0, count);
}

void Mesh::destroy() {
    if (vao) glDeleteVertexArrays(1, &vao);
    if (vbo) glDeleteBuffers(1, &vbo);
    vao = vbo = 0;
    count = 0;
}

Mesh createCubeMesh() {
    struct Face { glm::vec3 n, u, v; };
    const Face faces[6] = {
        {{0, 0, 1},  {1, 0, 0},  {0, 1, 0}},
        {{0, 0, -1}, {-1, 0, 0}, {0, 1, 0}},
        {{1, 0, 0},  {0, 0, -1}, {0, 1, 0}},
        {{-1, 0, 0}, {0, 0, 1},  {0, 1, 0}},
        {{0, 1, 0},  {1, 0, 0},  {0, 0, -1}},
        {{0, -1, 0}, {1, 0, 0},  {0, 0, 1}},
    };

    std::vector<float> data;
    auto push = [&](const glm::vec3& p, const glm::vec3& n, float u, float v) {
        data.insert(data.end(), {p.x, p.y, p.z, n.x, n.y, n.z, u, v});
    };

    for (const Face& f : faces) {
        glm::vec3 c0 = f.n * 0.5f - f.u * 0.5f - f.v * 0.5f;
        glm::vec3 c1 = f.n * 0.5f + f.u * 0.5f - f.v * 0.5f;
        glm::vec3 c2 = f.n * 0.5f + f.u * 0.5f + f.v * 0.5f;
        glm::vec3 c3 = f.n * 0.5f - f.u * 0.5f + f.v * 0.5f;
        push(c0, f.n, 0, 0); push(c1, f.n, 1, 0); push(c2, f.n, 1, 1);
        push(c0, f.n, 0, 0); push(c2, f.n, 1, 1); push(c3, f.n, 0, 1);
    }
    return uploadMesh(data);
}

Mesh createQuadMesh() {
    std::vector<float> data;
    auto push = [&](float x, float y, float u, float v) {
        data.insert(data.end(), {x, y, 0.0f, 0.0f, 0.0f, 1.0f, u, v});
    };
    push(-0.5f, -0.5f, 0, 0); push(0.5f, -0.5f, 1, 0); push(0.5f, 0.5f, 1, 1);
    push(-0.5f, -0.5f, 0, 0); push(0.5f, 0.5f, 1, 1); push(-0.5f, 0.5f, 0, 1);
    return uploadMesh(data);
}
