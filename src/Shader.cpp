#include "Shader.hpp"
#include "ShaderSources.hpp"

#include <glm/gtc/type_ptr.hpp>
#include <iostream>

static GLuint compileShader(GLenum type, const char* src, const char* name) {
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);
    GLint ok;
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[2048];
        glGetShaderInfoLog(s, 2048, nullptr, log);
        std::cerr << "Loi compile shader (" << name << (type == GL_VERTEX_SHADER ? ", vertex" : ", fragment")
                  << "):\n" << log << std::endl;
    }
    return s;
}

bool Shader::build(const char* vertSrc, const char* fragSrc, const char* debugName) {
    GLuint vs = compileShader(GL_VERTEX_SHADER, vertSrc, debugName);
    GLuint fs = compileShader(GL_FRAGMENT_SHADER, fragSrc, debugName);
    id_ = glCreateProgram();
    glAttachShader(id_, vs);
    glAttachShader(id_, fs);
    glLinkProgram(id_);
    GLint ok;
    glGetProgramiv(id_, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[2048];
        glGetProgramInfoLog(id_, 2048, nullptr, log);
        std::cerr << "Loi link program (" << debugName << "):\n" << log << std::endl;
    }
    glDeleteShader(vs);
    glDeleteShader(fs);
    return ok != 0;
}

void Shader::destroy() {
    if (id_) glDeleteProgram(id_);
    id_ = 0;
    cache_.clear();
}

GLint Shader::loc(const char* name) {
    auto it = cache_.find(name);
    if (it != cache_.end()) return it->second;
    GLint l = glGetUniformLocation(id_, name);
    cache_[name] = l;
    return l;
}

void Shader::setM(const char* n, const glm::mat4& m) { glUniformMatrix4fv(loc(n), 1, GL_FALSE, glm::value_ptr(m)); }
void Shader::setMv(const char* n, const glm::mat4* m, int count) { glUniformMatrix4fv(loc(n), count, GL_FALSE, glm::value_ptr(*m)); }
void Shader::set3(const char* n, const glm::vec3& v) { glUniform3fv(loc(n), 1, glm::value_ptr(v)); }
void Shader::set4(const char* n, const glm::vec4& v) { glUniform4fv(loc(n), 1, glm::value_ptr(v)); }
void Shader::setF(const char* n, float v) { glUniform1f(loc(n), v); }
void Shader::setI(const char* n, int v) { glUniform1i(loc(n), v); }

bool Shaders::init() {
    bool ok = true;
    ok &= world.build(shader_src::PRIMITIVE_VERT, shader_src::WORLD_FRAG, "world");
    ok &= enemy.build(shader_src::PRIMITIVE_VERT, shader_src::ENEMY_FRAG, "enemy");
    ok &= viewmodel.build(shader_src::VIEWMODEL_VERT, shader_src::VIEWMODEL_FRAG, "viewmodel");
    ok &= decal.build(shader_src::PRIMITIVE_VERT, shader_src::DECAL_FRAG, "decal");
    ok &= ui.build(shader_src::UI_VERT, shader_src::UI_FRAG, "ui");
    return ok;
}

void Shaders::destroy() {
    world.destroy();
    enemy.destroy();
    viewmodel.destroy();
    decal.destroy();
    ui.destroy();
}
