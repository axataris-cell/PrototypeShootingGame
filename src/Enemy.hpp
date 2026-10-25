#pragma once
#include "Config.hpp"
#include "Mesh.hpp"
#include "Shader.hpp"

#include <glm/glm.hpp>

enum class HitZone { None, Head, Body, Legs };

struct Enemy {
    glm::vec3 pos{0.0f};     // vi tri chan
    float yaw = 0.0f;
    int   hp = ENEMY_MAX_HP;
    float attackCd = 0.0f;
    float hitFlash = 0.0f;
    float walkPhase = 0.0f;
    bool  alive = true;
};

// Ban tia vao quai (hop con theo bo phan + tru bao). tOut = khoang cach trung.
HitZone raycastEnemy(const Enemy& e, const glm::vec3& origin, const glm::vec3& dir, float tMax, float& tOut);

// Ve quai bang enemy shader (shader da duoc use va gan uniform chung)
void drawEnemy(const Enemy& e, Shader& enemyShader, const Mesh& cube);
