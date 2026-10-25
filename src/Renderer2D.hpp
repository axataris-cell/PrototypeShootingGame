#pragma once
// Ve 2D theo batch: hinh chu nhat + chu bitmap 5x7 (khong can file font)

#include "Shader.hpp"

#include <glad/gl.h>
#include <glm/glm.hpp>
#include <string>
#include <vector>

enum class TextAlign { Left, Center, Right };

class Renderer2D {
public:
    bool init(Shader* uiShader);
    void destroy();

    void begin(int width, int height);
    void rect(float x, float y, float w, float h, const glm::vec4& color);
    void rectOutline(float x, float y, float w, float h, float thickness, const glm::vec4& color);
    // (x, y) la goc tren-trai (hoac tam/phai tuy align). scale = so pixel cho moi "diem anh" cua font
    void text(const std::string& s, float x, float y, float scale, const glm::vec4& color,
              TextAlign align = TextAlign::Left);
    static float textWidth(const std::string& s, float scale);
    static float textHeight(float scale) { return 7.0f * scale; }
    void end();     // flush

    float width() const { return static_cast<float>(w_); }
    float height() const { return static_cast<float>(h_); }

private:
    void flush();

    struct Vertex { float x, y, r, g, b, a; };
    Shader* shader_ = nullptr;
    GLuint vao_ = 0, vbo_ = 0;
    size_t vboCapacity_ = 0;
    std::vector<Vertex> verts_;
    int w_ = 1, h_ = 1;
};
