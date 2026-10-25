#include "BulletHole.hpp"
#include "Config.hpp"
#include "Utils.hpp"

#include <glm/gtc/constants.hpp>
#include <cmath>

void BulletHoles::add(const glm::vec3& pos, const glm::vec3& normal) {
    glm::vec3 n = glm::normalize(normal);
    glm::vec3 helper = std::abs(n.y) > 0.9f ? glm::vec3(1, 0, 0) : glm::vec3(0, 1, 0);
    glm::vec3 t = glm::normalize(glm::cross(helper, n));
    glm::vec3 b = glm::cross(n, t);

    float ang = randf(0.0f, glm::two_pi<float>());
    float c = std::cos(ang), s = std::sin(ang);
    glm::vec3 t2 = t * c + b * s;
    glm::vec3 b2 = -t * s + b * c;

    float size = randf(0.26f, 0.34f);
    glm::mat4 m(1.0f);
    m[0] = glm::vec4(t2 * size, 0.0f);
    m[1] = glm::vec4(b2 * size, 0.0f);
    m[2] = glm::vec4(n, 0.0f);
    m[3] = glm::vec4(pos + n * 0.004f, 1.0f);     // nhich ra khoi be mat de tranh z-fighting

    holes_.push_back({m});
    while (holes_.size() > static_cast<size_t>(MAX_BULLET_HOLES)) holes_.pop_front();
}

void BulletHoles::draw(Shader& sh, const Mesh& quad, GLuint texture) const {
    if (holes_.empty()) return;
    sh.use();
    sh.setI("uTex", 0);
    sh.setF("uAlpha", 1.0f);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);

    glDepthMask(GL_FALSE);
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(-2.0f, -2.0f);
    for (const Hole& h : holes_) {
        sh.setM("uModel", h.model);
        quad.draw();
    }
    glDisable(GL_POLYGON_OFFSET_FILL);
    glDepthMask(GL_TRUE);
}
