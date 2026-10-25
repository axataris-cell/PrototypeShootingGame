#pragma once
#include <glm/glm.hpp>
#include <string>

struct AABB {
    glm::vec3 min, max;
    glm::vec3 color;
};

// Ngau nhien
float randf(float a, float b);

// Hinh hoc / va cham
bool rayAABB(const glm::vec3& o, const glm::vec3& d, const glm::vec3& mn, const glm::vec3& mx,
             float& tOut, glm::vec3* normalOut = nullptr);
bool rayOBB(const glm::vec3& origin, const glm::vec3& dir, const glm::mat4& model, float& tOut);
bool rayYCylinder(const glm::vec3& o, const glm::vec3& d, float radius, float y0, float y1, float& tOut);
void resolveCircleAABB(glm::vec3& pos, float r, const AABB& b);

// Di chuyen kieu Quake
void accelerate(glm::vec2& vel, const glm::vec2& wish, float wishSpeed, float accel, float dt);
void applyFriction(glm::vec2& vel, float friction, float dt);
void clampSpeed(glm::vec2& vel, float maxSpeed);

// Tim file trong thu muc assets (vd "model/M4.glb"). Tra ve duong dan day du.
std::string assetPath(const std::string& relative);
