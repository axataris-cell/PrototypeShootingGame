#pragma once
#include <glad/gl.h>
#include <glm/glm.hpp>
#include <string>
#include <unordered_map>

class Shader {
public:
    bool build(const char* vertSrc, const char* fragSrc, const char* debugName = "shader");
    void destroy();
    void use() const { glUseProgram(id_); }
    GLuint id() const { return id_; }

    void setM(const char* n, const glm::mat4& m);
    void setMv(const char* n, const glm::mat4* m, int count);
    void set3(const char* n, const glm::vec3& v);
    void set4(const char* n, const glm::vec4& v);
    void setF(const char* n, float v);
    void setI(const char* n, int v);

private:
    GLint loc(const char* name);
    GLuint id_ = 0;
    std::unordered_map<std::string, GLint> cache_;
};

// Tat ca chuong trinh shader cua game
struct Shaders {
    Shader world;      // dat, tuong, vat can, tia dan
    Shader enemy;      // quai
    Shader viewmodel;  // sung + tay nguoi choi (skinning + texture)
    Shader decal;      // lo dan
    Shader ui;         // HUD / menu 2D

    bool init();
    void destroy();
};
