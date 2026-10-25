#include "Enemy.hpp"
#include "Utils.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <cmath>

static glm::vec3 worldToEnemyLocal(const glm::vec3& p, const Enemy& e) {
    glm::vec3 d = p - e.pos;
    float c = std::cos(e.yaw), s = std::sin(e.yaw);
    return glm::vec3(c * d.x - s * d.z, d.y, s * d.x + c * d.z);
}

static glm::vec3 dirToEnemyLocal(const glm::vec3& d, const Enemy& e) {
    float c = std::cos(e.yaw), s = std::sin(e.yaw);
    return glm::vec3(c * d.x - s * d.z, d.y, s * d.x + c * d.z);
}

HitZone raycastEnemy(const Enemy& e, const glm::vec3& origin, const glm::vec3& dir, float tMax, float& tOut) {
    float bob = std::fabs(std::sin(e.walkPhase)) * 0.06f;
    glm::mat4 base = glm::translate(glm::mat4(1.0f), e.pos + glm::vec3(0, bob, 0)) *
                     glm::rotate(glm::mat4(1.0f), e.yaw, glm::vec3(0, 1, 0));

    struct Part { glm::vec3 c, s; HitZone z; float pad; };
    const Part parts[] = {
        {{0.0f, 1.675f, 0.0f}, {0.35f, 0.35f, 0.35f}, HitZone::Head, 1.45f},
        {{0.0f, 1.15f, 0.0f}, {0.60f, 0.70f, 0.35f}, HitZone::Body, 1.22f},
        {{-0.38f, 1.25f, 0.2f}, {0.15f, 0.15f, 0.55f}, HitZone::Body, 1.35f},
        {{0.38f, 1.25f, 0.2f}, {0.15f, 0.15f, 0.55f}, HitZone::Body, 1.35f},
        {{0.0f, 0.40f, 0.0f}, {0.50f, 0.80f, 0.30f}, HitZone::Legs, 1.28f},
    };

    HitZone zone = HitZone::None;
    float best = tMax;
    for (const Part& p : parts) {
        glm::mat4 model = base * glm::translate(glm::mat4(1.0f), p.c) * glm::scale(glm::mat4(1.0f), p.s * p.pad);
        float t;
        if (rayOBB(origin, dir, model, t) && t >= 0.0f && t < best) {
            best = t;
            zone = p.z;
        }
    }

    // Tru bao toan than: ban sat mat / mep van dinh
    glm::vec3 o = worldToEnemyLocal(origin, e);
    o.y -= bob;
    glm::vec3 d = dirToEnemyLocal(dir, e);
    float tCyl;
    if (rayYCylinder(o, d, 0.42f, 0.0f, 1.92f, tCyl) && tCyl >= 0.0f && tCyl < best) {
        best = tCyl;
        float hy = o.y + d.y * tCyl;
        zone = (hy >= 1.48f) ? HitZone::Head : (hy >= 0.78f ? HitZone::Body : HitZone::Legs);
    }

    tOut = best;
    return zone;
}

void drawEnemy(const Enemy& e, Shader& sh, const Mesh& cube) {
    float bob = std::fabs(std::sin(e.walkPhase)) * 0.06f;
    glm::mat4 base = glm::translate(glm::mat4(1.0f), e.pos + glm::vec3(0, bob, 0)) *
                     glm::rotate(glm::mat4(1.0f), e.yaw, glm::vec3(0, 1, 0));
    sh.setF("uHitFlash", glm::clamp(e.hitFlash / 0.1f, 0.0f, 1.0f));

    auto part = [&](glm::vec3 c, glm::vec3 s, glm::vec4 col, glm::vec3 emissive = glm::vec3(0.0f)) {
        sh.setM("uModel", base * glm::translate(glm::mat4(1.0f), c) * glm::scale(glm::mat4(1.0f), s));
        sh.set4("uColor", col);
        sh.set3("uEmissive", emissive);
        cube.draw();
    };

    part({0, 0.4f, 0},       {0.5f, 0.8f, 0.3f},    {0.2f, 0.2f, 0.5f, 1});   // chan
    part({0, 1.15f, 0},      {0.6f, 0.7f, 0.35f},   {0.75f, 0.15f, 0.15f, 1}); // than
    part({0, 1.675f, 0},     {0.35f, 0.35f, 0.35f}, {0.85f, 0.7f, 0.55f, 1});  // dau
    part({-0.38f, 1.25f, 0.2f}, {0.15f, 0.15f, 0.55f}, {0.85f, 0.7f, 0.55f, 1}); // tay trai
    part({0.38f, 1.25f, 0.2f},  {0.15f, 0.15f, 0.55f}, {0.85f, 0.7f, 0.55f, 1}); // tay phai
    part({-0.08f, 1.7f, 0.18f}, {0.06f, 0.05f, 0.02f}, {0.1f, 0, 0, 1}, {1.0f, 0.15f, 0.05f}); // mat phat sang
    part({0.08f, 1.7f, 0.18f},  {0.06f, 0.05f, 0.02f}, {0.1f, 0, 0, 1}, {1.0f, 0.15f, 0.05f});
}
