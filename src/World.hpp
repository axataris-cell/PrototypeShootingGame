#pragma once
#include "BulletHole.hpp"
#include "Enemy.hpp"
#include "Mesh.hpp"
#include "Shader.hpp"
#include "Utils.hpp"

#include <glm/glm.hpp>
#include <vector>

struct Tracer {
    glm::vec3 a, b;
    float life;
};

struct ShotResult {
    glm::vec3 endPoint{0.0f};
    bool hitEnemy = false;
    bool killed = false;
    HitZone zone = HitZone::None;
    bool hitSurface = false;           // trung tuong / vat can / mat dat (de tao lo dan)
    glm::vec3 surfaceNormal{0.0f, 1.0f, 0.0f};
};

// Mat dat, tuong, vat can, quai, tia dan, lo dan
class World {
public:
    bool init();
    void destroy();
    void reset();                      // bat dau van moi

    // Cap nhat quai + hieu ung. Tra ve sat thuong gay ra cho nguoi choi trong frame nay.
    int update(float dt, const glm::vec3& playerPos, float gameTime);

    ShotResult hitscan(const glm::vec3& origin, const glm::vec3& dir, int dmgHead, int dmgBody, int dmgLegs);
    void addTracer(const glm::vec3& a, const glm::vec3& b) { tracers_.push_back({a, b, 0.06f}); }
    void addBulletHole(const glm::vec3& p, const glm::vec3& n) { holes_.add(p, n); }

    void render(Shaders& sh, const glm::mat4& view, const glm::mat4& proj, const glm::vec3& eye,
                float time, bool fx);

    const std::vector<AABB>& level() const { return level_; }
    const std::vector<Enemy>& enemies() const { return enemies_; }
    size_t bulletHoleCount() const { return holes_.size(); }
    const Mesh& cube() const { return cube_; }

    static const glm::vec3 SKY_COLOR;
    static const glm::vec3 LIGHT_DIR;

private:
    void buildLevel();
    void spawnEnemy(const glm::vec3& playerPos);

    std::vector<AABB> level_;
    std::vector<Enemy> enemies_;
    std::vector<Tracer> tracers_;
    BulletHoles holes_;
    Mesh cube_, quad_;
    GLuint holeTex_ = 0;
    float spawnTimer_ = 1.0f;
};
