#include "World.hpp"
#include "Texture.hpp"

#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <cmath>

const glm::vec3 World::SKY_COLOR(0.53f, 0.74f, 0.92f);
const glm::vec3 World::LIGHT_DIR = glm::normalize(glm::vec3(-0.4f, -1.0f, -0.3f));

bool World::init() {
    cube_ = createCubeMesh();
    quad_ = createQuadMesh();
    holeTex_ = createBulletHoleTexture();
    buildLevel();
    return true;
}

void World::destroy() {
    cube_.destroy();
    quad_.destroy();
    if (holeTex_) glDeleteTextures(1, &holeTex_);
    holeTex_ = 0;
}

void World::buildLevel() {
    level_.clear();
    // Tuong bao quanh san (40x40)
    const float S = 20.0f;
    const glm::vec3 wallCol(0.55f, 0.55f, 0.6f);
    level_.push_back({{-S - 1, 0, -S - 1}, {S + 1, 4, -S}, wallCol});
    level_.push_back({{-S - 1, 0, S}, {S + 1, 4, S + 1}, wallCol});
    level_.push_back({{-S - 1, 0, -S}, {-S, 4, S}, wallCol});
    level_.push_back({{S, 0, -S}, {S + 1, 4, S}, wallCol});

    // Vat can ben trong
    level_.push_back({{-8, 0, -8},  {-5, 3, -5},    {0.65f, 0.45f, 0.30f}});
    level_.push_back({{5, 0, -10},  {9, 2, -7},     {0.35f, 0.55f, 0.65f}});
    level_.push_back({{-12, 0, 4},  {-9, 4, 10},    {0.60f, 0.35f, 0.40f}});
    level_.push_back({{6, 0, 6},    {10, 2.5f, 8},  {0.45f, 0.65f, 0.40f}});
    level_.push_back({{-2, 0, 12},  {2, 2, 13},     {0.70f, 0.65f, 0.35f}});
    level_.push_back({{12, 0, -2},  {14, 3, 3},     {0.50f, 0.40f, 0.65f}});
    level_.push_back({{-4, 0, -14}, {0, 1.5f, -13}, {0.40f, 0.60f, 0.55f}});
    level_.push_back({{-14, 0, -14},{-12, 3.5f, -11},{0.65f, 0.55f, 0.45f}});
}

void World::reset() {
    enemies_.clear();
    tracers_.clear();
    holes_.clear();
    spawnTimer_ = 1.0f;
    for (int i = 0; i < 3; ++i) spawnEnemy(glm::vec3(0.0f));
}

void World::spawnEnemy(const glm::vec3& playerPos) {
    for (int i = 0; i < 30; ++i) {
        glm::vec3 p(randf(-18, 18), 0.0f, randf(-18, 18));
        if (glm::length(glm::vec2(p.x - playerPos.x, p.z - playerPos.z)) < 14.0f) continue;
        bool bad = false;
        for (const auto& o : level_) {
            if (p.x > o.min.x - 1 && p.x < o.max.x + 1 && p.z > o.min.z - 1 && p.z < o.max.z + 1) {
                bad = true;
                break;
            }
        }
        if (bad) continue;
        Enemy e;
        e.pos = p;
        enemies_.push_back(e);
        return;
    }
}

int World::update(float dt, const glm::vec3& playerPos, float gameTime) {
    for (auto& t : tracers_) t.life -= dt;
    tracers_.erase(std::remove_if(tracers_.begin(), tracers_.end(), [](const Tracer& t) { return t.life <= 0.0f; }),
                   tracers_.end());

    // Don quai da chet
    enemies_.erase(std::remove_if(enemies_.begin(), enemies_.end(), [](const Enemy& e) { return !e.alive; }),
                   enemies_.end());

    spawnTimer_ -= dt;
    if (spawnTimer_ <= 0.0f) {
        if (static_cast<int>(enemies_.size()) < MAX_ENEMIES) spawnEnemy(playerPos);
        spawnTimer_ = std::max(0.8f, 2.5f - gameTime * 0.015f);
    }

    int damage = 0;
    float enemySpeed = std::min(4.5f, 2.0f + gameTime * 0.02f);
    for (auto& e : enemies_) {
        e.hitFlash = std::max(0.0f, e.hitFlash - dt);
        e.attackCd -= dt;

        glm::vec2 toP(playerPos.x - e.pos.x, playerPos.z - e.pos.z);
        float d = glm::length(toP);
        if (d > 0.01f) {
            glm::vec2 dir = toP / d;
            e.yaw = std::atan2(dir.x, dir.y);   // +Z cua mo hinh huong ve nguoi choi
            if (d > 1.0f) {
                e.pos.x += dir.x * enemySpeed * dt;
                e.pos.z += dir.y * enemySpeed * dt;
                e.walkPhase += dt * enemySpeed * 2.5f;
            }
        }
        if (d < 1.2f && e.attackCd <= 0.0f) {
            e.attackCd = 1.0f;
            damage += 10;
        }
    }

    // Tach quai ra khoi nhau + vat can
    for (size_t i = 0; i < enemies_.size(); ++i) {
        for (size_t j = i + 1; j < enemies_.size(); ++j) {
            glm::vec2 diff(enemies_[i].pos.x - enemies_[j].pos.x, enemies_[i].pos.z - enemies_[j].pos.z);
            float d = glm::length(diff);
            if (d < ENEMY_RADIUS * 2.0f && d > 1e-4f) {
                glm::vec2 push = diff / d * (ENEMY_RADIUS * 2.0f - d) * 0.5f;
                enemies_[i].pos.x += push.x; enemies_[i].pos.z += push.y;
                enemies_[j].pos.x -= push.x; enemies_[j].pos.z -= push.y;
            }
        }
    }
    for (auto& e : enemies_)
        for (const auto& o : level_) resolveCircleAABB(e.pos, ENEMY_RADIUS, o);

    return damage;
}

ShotResult World::hitscan(const glm::vec3& origin, const glm::vec3& dir, int dmgHead, int dmgBody, int dmgLegs) {
    ShotResult r;

    // Vat can gan nhat (tuong, vat the, mat dat)
    float tWorld = MAX_RANGE;
    glm::vec3 worldNormal(0.0f, 1.0f, 0.0f);
    bool worldHit = false;
    for (const auto& o : level_) {
        float t;
        glm::vec3 n;
        if (rayAABB(origin, dir, o.min, o.max, t, &n) && t < tWorld) {
            tWorld = t;
            worldNormal = n;
            worldHit = true;
        }
    }
    if (dir.y < -1e-4f) {
        float t = -origin.y / dir.y;
        if (t > 0.0f && t < tWorld) {
            tWorld = t;
            worldNormal = glm::vec3(0, 1, 0);
            worldHit = true;
        }
    }

    Enemy* hit = nullptr;
    float tHit = tWorld;
    HitZone zone = HitZone::None;
    for (auto& e : enemies_) {
        if (!e.alive) continue;
        float t;
        HitZone z = raycastEnemy(e, origin, dir, tHit, t);
        if (z != HitZone::None && t < tHit) {
            tHit = t;
            hit = &e;
            zone = z;
        }
    }

    r.endPoint = origin + dir * tHit;
    if (hit) {
        int dmg = (zone == HitZone::Head) ? dmgHead : (zone == HitZone::Legs ? dmgLegs : dmgBody);
        hit->hp -= dmg;
        hit->hitFlash = 0.1f;
        r.hitEnemy = true;
        r.zone = zone;
        if (hit->hp <= 0) {
            hit->alive = false;
            r.killed = true;
        }
    } else if (worldHit && tWorld < MAX_RANGE) {
        r.hitSurface = true;
        r.surfaceNormal = worldNormal;
    }
    return r;
}

void World::render(Shaders& sh, const glm::mat4& view, const glm::mat4& proj, const glm::vec3& eye,
                   float time, bool fx) {
    const int fxI = fx ? 1 : 0;

    // ---------- Vat the ----------
    Shader& w = sh.world;
    w.use();
    w.setM("uView", view);
    w.setM("uProj", proj);
    w.set3("uViewPos", eye);
    w.set3("uLightDir", LIGHT_DIR);
    w.set3("uFogColor", SKY_COLOR);
    w.setI("uFx", fxI);
    w.setF("uTime", time);
    w.setI("uUnlit", 0);

    w.setI("uChecker", 1);
    w.setM("uModel", glm::translate(glm::mat4(1.0f), glm::vec3(0, -0.5f, 0)) *
                         glm::scale(glm::mat4(1.0f), glm::vec3(100, 1, 100)));
    w.set4("uColor", {0.45f, 0.62f, 0.40f, 1.0f});
    cube_.draw();
    w.setI("uChecker", 0);

    for (const auto& o : level_) {
        w.setM("uModel", glm::translate(glm::mat4(1.0f), (o.min + o.max) * 0.5f) *
                             glm::scale(glm::mat4(1.0f), o.max - o.min));
        w.set4("uColor", glm::vec4(o.color, 1.0f));
        cube_.draw();
    }

    // ---------- Lo dan ----------
    sh.decal.use();
    sh.decal.setM("uView", view);
    sh.decal.setM("uProj", proj);
    holes_.draw(sh.decal, quad_, holeTex_);

    // ---------- Quai ----------
    Shader& e = sh.enemy;
    e.use();
    e.setM("uView", view);
    e.setM("uProj", proj);
    e.set3("uViewPos", eye);
    e.set3("uLightDir", LIGHT_DIR);
    e.set3("uFogColor", SKY_COLOR);
    e.setI("uFx", fxI);
    e.setF("uTime", time);
    for (const auto& en : enemies_) drawEnemy(en, e, cube_);

    // ---------- Tia dan ----------
    w.use();
    w.setI("uUnlit", 1);
    for (const auto& t : tracers_) {
        glm::vec3 d = t.b - t.a;
        float len = glm::length(d);
        if (len < 0.01f) continue;
        d /= len;
        glm::vec3 up = std::fabs(d.y) > 0.99f ? glm::vec3(1, 0, 0) : glm::vec3(0, 1, 0);
        glm::mat4 m = glm::inverse(glm::lookAt(t.a, t.b, up)) *
                      glm::translate(glm::mat4(1.0f), glm::vec3(0, 0, -len * 0.5f)) *
                      glm::scale(glm::mat4(1.0f), glm::vec3(0.015f, 0.015f, len));
        w.setM("uModel", m);
        w.set4("uColor", {1.0f, 0.9f, 0.4f, glm::clamp(t.life / 0.06f, 0.0f, 1.0f)});
        cube_.draw();
    }
    w.setI("uUnlit", 0);
}
