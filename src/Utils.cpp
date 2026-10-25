#include "Utils.hpp"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <random>

static std::mt19937& rng() {
    static std::mt19937 r{std::random_device{}()};
    return r;
}

float randf(float a, float b) {
    return std::uniform_real_distribution<float>(a, b)(rng());
}

bool rayAABB(const glm::vec3& o, const glm::vec3& d, const glm::vec3& mn, const glm::vec3& mx,
             float& tOut, glm::vec3* normalOut) {
    float tmin = -1e9f, tmax = 1e9f;
    int axis = 0;
    for (int i = 0; i < 3; ++i) {
        if (std::fabs(d[i]) < 1e-8f) {
            if (o[i] < mn[i] || o[i] > mx[i]) return false;
        } else {
            float inv = 1.0f / d[i];
            float t1 = (mn[i] - o[i]) * inv;
            float t2 = (mx[i] - o[i]) * inv;
            if (t1 > t2) std::swap(t1, t2);
            if (t1 > tmin) { tmin = t1; axis = i; }
            tmax = std::min(tmax, t2);
            if (tmin > tmax) return false;
        }
    }
    if (tmax < 0.0f) return false; // hop nam sau goc ban
    if (normalOut) {
        if (tmin < 0.0f) {
            *normalOut = -d;                     // goc ban nam trong hop
        } else {
            glm::vec3 n(0.0f);
            n[axis] = d[axis] > 0.0f ? -1.0f : 1.0f;
            *normalOut = n;
        }
    }
    tOut = tmin < 0.0f ? 0.0f : tmin;
    return true;
}

bool rayOBB(const glm::vec3& origin, const glm::vec3& dir, const glm::mat4& model, float& tOut) {
    glm::mat4 inv = glm::inverse(model);
    glm::vec3 o = glm::vec3(inv * glm::vec4(origin, 1.0f));
    glm::vec3 d = glm::mat3(inv) * dir; // giu tham so t theo the gioi
    return rayAABB(o, d, glm::vec3(-0.5f), glm::vec3(0.5f), tOut);
}

bool rayYCylinder(const glm::vec3& o, const glm::vec3& d, float radius, float y0, float y1, float& tOut) {
    float a = d.x * d.x + d.z * d.z;
    float tEnter = 0.0f, tExit = 1e9f;
    float r2 = radius * radius;
    float dist2 = o.x * o.x + o.z * o.z;

    if (a < 1e-10f) {
        if (dist2 > r2) return false;
    } else {
        float b = 2.0f * (o.x * d.x + o.z * d.z);
        float c = dist2 - r2;
        float disc = b * b - 4.0f * a * c;
        if (disc < 0.0f) return false;
        float sq = std::sqrt(disc);
        tEnter = (-b - sq) / (2.0f * a);
        tExit = (-b + sq) / (2.0f * a);
        if (tExit < 0.0f) return false;
        if (tEnter < 0.0f) tEnter = 0.0f;
    }

    if (d.y > 1e-8f) {
        tEnter = std::max(tEnter, (y0 - o.y) / d.y);
        tExit = std::min(tExit, (y1 - o.y) / d.y);
    } else if (d.y < -1e-8f) {
        tEnter = std::max(tEnter, (y1 - o.y) / d.y);
        tExit = std::min(tExit, (y0 - o.y) / d.y);
    } else if (o.y < y0 || o.y > y1) {
        return false;
    }
    if (tEnter > tExit || tExit < 0.0f) return false;
    tOut = tEnter < 0.0f ? 0.0f : tEnter;
    return true;
}

// Day hinh tron (mat phang XZ) ra khoi hop
void resolveCircleAABB(glm::vec3& pos, float r, const AABB& b) {
    float cx = glm::clamp(pos.x, b.min.x, b.max.x);
    float cz = glm::clamp(pos.z, b.min.z, b.max.z);
    float dx = pos.x - cx, dz = pos.z - cz;
    float d2 = dx * dx + dz * dz;
    if (d2 >= r * r) return;

    if (d2 > 1e-8f) {
        float d = std::sqrt(d2);
        float push = r - d;
        pos.x += dx / d * push;
        pos.z += dz / d * push;
    } else { // tam nam trong hop: day ra mat gan nhat
        float l = pos.x - b.min.x, rr = b.max.x - pos.x;
        float t = pos.z - b.min.z, bt = b.max.z - pos.z;
        float m = std::min({l, rr, t, bt});
        if (m == l)       pos.x = b.min.x - r;
        else if (m == rr) pos.x = b.max.x + r;
        else if (m == t)  pos.z = b.min.z - r;
        else              pos.z = b.max.z + r;
    }
}

void accelerate(glm::vec2& vel, const glm::vec2& wish, float wishSpeed, float accel, float dt) {
    float wlen = glm::length(wish);
    if (wlen < 1e-6f || wishSpeed <= 0.0f) return;
    glm::vec2 wdir = wish / wlen;
    float current = glm::dot(vel, wdir);
    float add = wishSpeed - current;
    if (add <= 0.0f) return;
    float acc = accel * wishSpeed * dt;
    if (acc > add) acc = add;
    vel += wdir * acc;
}

void applyFriction(glm::vec2& vel, float friction, float dt) {
    float sp = glm::length(vel);
    if (sp < 1e-4f) {
        vel = glm::vec2(0.0f);
        return;
    }
    float ns = std::max(sp - sp * friction * dt, 0.0f);
    vel *= ns / sp;
}

void clampSpeed(glm::vec2& vel, float maxSpeed) {
    float sp = glm::length(vel);
    if (sp > maxSpeed && sp > 1e-6f) vel *= maxSpeed / sp;
}

std::string assetPath(const std::string& relative) {
    namespace fs = std::filesystem;
    static std::string root;
    if (root.empty()) {
        const char* candidates[] = {
#ifdef ASSETS_DIR
            ASSETS_DIR,
#endif
            "assets", "../assets", "../../assets", "../../../assets"
        };
        for (const char* c : candidates) {
            std::error_code ec;
            if (fs::exists(fs::path(c) / "model", ec)) { root = c; break; }
        }
        if (root.empty()) root = "assets";
    }
    return (fs::path(root) / relative).generic_string();
}
