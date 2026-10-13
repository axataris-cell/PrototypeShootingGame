// feat Claude

// Game ban sung 3D goc nhin thu nhat (FPS) co ban
// C++17 / OpenGL 3.3 Core / GLAD 2 / GLFW / GLM
//
// WASD: di chuyen | Shift: di bo | Ctrl: ngoi | Space: nhay | Chuot: ngam
// Chuot trai: ban (giu de ban lien thanh) | R: nap dan | Enter: choi lai | ESC: thoat

#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <algorithm>
#include <cmath>
#include <iostream>
#include <random>
#include <string>
#include <unordered_map>
#include <vector>

// ============================================================
// Hang so
// ============================================================
constexpr float EYE_HEIGHT       = 1.7f;
constexpr float CROUCH_EYE       = 1.05f;
constexpr float PLAYER_RADIUS    = 0.4f;
constexpr float RUN_SPEED        = 7.4f;
constexpr float WALK_SPEED       = 4.4f;
constexpr float CROUCH_SPEED     = 3.2f;
constexpr float GROUND_ACCEL     = 14.0f;
constexpr float AIR_ACCEL        = 9.0f;
constexpr float GROUND_FRICTION  = 8.0f;
constexpr float JUMP_SPEED       = 7.2f;
constexpr float GRAVITY          = 20.0f;
constexpr float MOUSE_SENS       = 0.0020f; // radian / pixel

constexpr int   MAG_SIZE         = 25;
constexpr float FIRE_INTERVAL    = 0.11f;
constexpr float RELOAD_TIME      = 1.4f;
constexpr float MAX_RANGE        = 120.0f;
constexpr float FIRST_SHOT_ACC   = 0.994f;              // vandal FSA
constexpr float SPRAY_PITCH_CAP  = 0.102f;              // ~5.8 deg, khong leo them
constexpr int   SPRAY_CLIMB_SHOTS = 8;                  // dat cap sau phat thu 8
constexpr float SPRAY_YAW_MAX    = 0.0044f;             // ~0.25 deg, lac ngang rat nho
constexpr float SPRAY_RESET_TIME = 0.40f;               // het spray -> FSA lai
constexpr float RECOIL_FOLLOW    = 36.0f;               // bam pattern khi dang ban
constexpr float RECOIL_RECOVER   = 13.0f;               // ve tam goc khi ngung ban

constexpr int   PLAYER_MAX_HP    = 150;
constexpr int   ENEMY_MAX_HP     = 150;
constexpr int   DMG_HEAD         = 150;
constexpr int   DMG_BODY         = 60;
constexpr int   DMG_LEGS         = 22;
constexpr float ENEMY_RADIUS     = 0.45f;
constexpr int   MAX_ENEMIES      = 14;

// ============================================================
// Shader
// ============================================================
static const char* VERT_SRC = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProj;

out vec3 vWorldPos;
out vec3 vNormal;

void main() {
    vec4 world = uModel * vec4(aPos, 1.0);
    vWorldPos = world.xyz;
    vNormal = mat3(transpose(inverse(uModel))) * aNormal;
    gl_Position = uProj * uView * world;
}
)";

static const char* FRAG_SRC = R"(
#version 330 core
in vec3 vWorldPos;
in vec3 vNormal;
out vec4 FragColor;

uniform vec4  uColor;
uniform vec3  uLightDir;   // huong anh sang (tu nguon di xuong)
uniform vec3  uViewPos;
uniform vec3  uFogColor;
uniform int   uUnlit;      // 1 = khong tinh anh sang (HUD, tia dan...)
uniform int   uChecker;    // 1 = ke o ca ro (mat dat)

void main() {
    if (uUnlit == 1) {
        FragColor = uColor;
        return;
    }

    vec3 base = uColor.rgb;
    if (uChecker == 1) {
        float c = mod(floor(vWorldPos.x) + floor(vWorldPos.z), 2.0);
        base *= mix(0.65, 1.0, c);
    }

    vec3 N = normalize(vNormal);
    vec3 L = normalize(-uLightDir);
    vec3 V = normalize(uViewPos - vWorldPos);
    vec3 H = normalize(L + V);

    float diff = max(dot(N, L), 0.0);
    float spec = pow(max(dot(N, H), 0.0), 32.0) * 0.25;

    vec3 col = base * (0.35 + 0.75 * diff) + vec3(spec);

    // Suong mu theo khoang cach
    float dist = length(uViewPos - vWorldPos);
    float fog = clamp((dist - 30.0) / 60.0, 0.0, 1.0);
    col = mix(col, uFogColor, fog);

    FragColor = vec4(col, uColor.a);
}
)";

static GLuint compileShader(GLenum type, const char* src) {
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);
    GLint ok;
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[1024];
        glGetShaderInfoLog(s, 1024, nullptr, log);
        std::cerr << "Loi compile shader:\n" << log << std::endl;
    }
    return s;
}

struct Shader {
    GLuint id = 0;
    std::unordered_map<std::string, GLint> cache;

    void build() {
        GLuint vs = compileShader(GL_VERTEX_SHADER, VERT_SRC);
        GLuint fs = compileShader(GL_FRAGMENT_SHADER, FRAG_SRC);
        id = glCreateProgram();
        glAttachShader(id, vs);
        glAttachShader(id, fs);
        glLinkProgram(id);
        GLint ok;
        glGetProgramiv(id, GL_LINK_STATUS, &ok);
        if (!ok) {
            char log[1024];
            glGetProgramInfoLog(id, 1024, nullptr, log);
            std::cerr << "Loi link program:\n" << log << std::endl;
        }
        glDeleteShader(vs);
        glDeleteShader(fs);
    }
    void use() const { glUseProgram(id); }
    GLint loc(const char* name) {
        auto it = cache.find(name);
        if (it != cache.end()) return it->second;
        GLint l = glGetUniformLocation(id, name);
        cache[name] = l;
        return l;
    }
    void setM(const char* n, const glm::mat4& m) { glUniformMatrix4fv(loc(n), 1, GL_FALSE, glm::value_ptr(m)); }
    void set3(const char* n, const glm::vec3& v) { glUniform3fv(loc(n), 1, glm::value_ptr(v)); }
    void set4(const char* n, const glm::vec4& v) { glUniform4fv(loc(n), 1, glm::value_ptr(v)); }
    void setI(const char* n, int v) { glUniform1i(loc(n), v); }
};

// ============================================================
// Mesh hinh hop (cube don vi, tam o goc, co normal)
// ============================================================
struct Mesh {
    GLuint vao = 0, vbo = 0;
    GLsizei count = 0;
};

static Mesh createCubeMesh() {
    struct Face { glm::vec3 n, u, v; };
    const Face faces[6] = {
        {{0, 0, 1},  {1, 0, 0},  {0, 1, 0}},
        {{0, 0, -1}, {-1, 0, 0}, {0, 1, 0}},
        {{1, 0, 0},  {0, 0, -1}, {0, 1, 0}},
        {{-1, 0, 0}, {0, 0, 1},  {0, 1, 0}},
        {{0, 1, 0},  {1, 0, 0},  {0, 0, -1}},
        {{0, -1, 0}, {1, 0, 0},  {0, 0, 1}},
    };

    std::vector<float> data;
    auto push = [&](const glm::vec3& p, const glm::vec3& n) {
        data.insert(data.end(), {p.x, p.y, p.z, n.x, n.y, n.z});
    };

    for (const Face& f : faces) {
        glm::vec3 c0 = f.n * 0.5f - f.u * 0.5f - f.v * 0.5f;
        glm::vec3 c1 = f.n * 0.5f + f.u * 0.5f - f.v * 0.5f;
        glm::vec3 c2 = f.n * 0.5f + f.u * 0.5f + f.v * 0.5f;
        glm::vec3 c3 = f.n * 0.5f - f.u * 0.5f + f.v * 0.5f;
        push(c0, f.n); push(c1, f.n); push(c2, f.n);
        push(c0, f.n); push(c2, f.n); push(c3, f.n);
    }

    Mesh m;
    m.count = static_cast<GLsizei>(data.size() / 6);
    glGenVertexArrays(1, &m.vao);
    glGenBuffers(1, &m.vbo);
    glBindVertexArray(m.vao);
    glBindBuffer(GL_ARRAY_BUFFER, m.vbo);
    glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(float), data.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);
    return m;
}

// ============================================================
// Du lieu game
// ============================================================
struct AABB {
    glm::vec3 min, max;
    glm::vec3 color;
};

struct Enemy {
    glm::vec3 pos{0.0f};     // vi tri chan
    float yaw = 0.0f;        // goc quay quanh truc Y
    int   hp = ENEMY_MAX_HP;
    float attackCd = 0.0f;
    float hitFlash = 0.0f;
    float walkPhase = 0.0f;
    bool  alive = true;
};

struct Tracer {
    glm::vec3 a, b;
    float life;
};

struct Game {
    // Nguoi choi
    glm::vec3 pos{0.0f, 0.0f, 0.0f}; // vi tri chan
    glm::vec2 velXZ{0.0f};
    float velY = 0.0f;
    bool  onGround = true;
    float yaw = -glm::half_pi<float>();
    float pitch = 0.0f;
    float recoilPitch = 0.0f;       // offset tam, hoi ve 0 khi ngung ban
    float recoilYaw = 0.0f;
    float recoilTargetP = 0.0f;
    float recoilTargetY = 0.0f;
    int   sprayCount = 0;           // so phat trong chuoi spray hien tai
    float crouchAmt = 0.0f;   // 0 dung, 1 ngoi
    bool  walking = false;
    int   hp = PLAYER_MAX_HP;

    // Sung
    int   ammo = MAG_SIZE;
    float fireTimer = 0.0f;
    bool  reloading = false;
    float reloadTimer = 0.0f;
    float kick = 0.0f;        // giat sung (hinh anh)
    float muzzleFlash = 0.0f;
    float walkTime = 0.0f;
    bool  moving = false;
    float spread = 0.0f;      // goc lech dan (radian)
    float sprayReset = 0.0f;

    // Hieu ung HUD
    float hurt = 0.0f;
    float hitMarker = 0.0f;

    std::vector<Enemy>  enemies;
    std::vector<Tracer> tracers;

    int   score = 0;
    int   kills = 0;
    float time = 0.0f;
    float spawnTimer = 1.0f;
    bool  gameOver = false;
};

// ============================================================
// Bien toan cuc
// ============================================================
static Shader g_shader;
static Mesh   g_cube;
static std::vector<AABB> g_level;
static std::mt19937 g_rng{std::random_device{}()};
static int g_fbW = 1280, g_fbH = 720;

static const glm::vec3 SKY_COLOR(0.53f, 0.74f, 0.92f);
static const glm::vec3 LIGHT_DIR = glm::normalize(glm::vec3(-0.4f, -1.0f, -0.3f));

static float randf(float a, float b) {
    return std::uniform_real_distribution<float>(a, b)(g_rng);
}

static void buildLevel() {
    g_level.clear();
    // Tuong bao quanh san (40x40)
    const float S = 20.0f;
    const glm::vec3 wallCol(0.55f, 0.55f, 0.6f);
    g_level.push_back({{-S - 1, 0, -S - 1}, {S + 1, 4, -S}, wallCol});
    g_level.push_back({{-S - 1, 0, S}, {S + 1, 4, S + 1}, wallCol});
    g_level.push_back({{-S - 1, 0, -S}, {-S, 4, S}, wallCol});
    g_level.push_back({{S, 0, -S}, {S + 1, 4, S}, wallCol});

    // Vat can ben trong
    g_level.push_back({{-8, 0, -8},  {-5, 3, -5},    {0.65f, 0.45f, 0.30f}});
    g_level.push_back({{5, 0, -10},  {9, 2, -7},     {0.35f, 0.55f, 0.65f}});
    g_level.push_back({{-12, 0, 4},  {-9, 4, 10},    {0.60f, 0.35f, 0.40f}});
    g_level.push_back({{6, 0, 6},    {10, 2.5f, 8},  {0.45f, 0.65f, 0.40f}});
    g_level.push_back({{-2, 0, 12},  {2, 2, 13},     {0.70f, 0.65f, 0.35f}});
    g_level.push_back({{12, 0, -2},  {14, 3, 3},     {0.50f, 0.40f, 0.65f}});
    g_level.push_back({{-4, 0, -14}, {0, 1.5f, -13}, {0.40f, 0.60f, 0.55f}});
    g_level.push_back({{-14, 0, -14},{-12, 3.5f, -11},{0.65f, 0.55f, 0.45f}});
}

// ============================================================
// Tien ich hinh hoc
// ============================================================
static bool rayAABB(const glm::vec3& o, const glm::vec3& d,
                    const glm::vec3& mn, const glm::vec3& mx, float& tOut) {
    float tmin = -1e9f, tmax = 1e9f;
    for (int i = 0; i < 3; ++i) {
        if (std::fabs(d[i]) < 1e-8f) {
            if (o[i] < mn[i] || o[i] > mx[i]) return false;
        } else {
            float inv = 1.0f / d[i];
            float t1 = (mn[i] - o[i]) * inv;
            float t2 = (mx[i] - o[i]) * inv;
            if (t1 > t2) std::swap(t1, t2);
            tmin = std::max(tmin, t1);
            tmax = std::min(tmax, t2);
            if (tmin > tmax) return false;
        }
    }
    if (tmax < 0.0f) return false; // hop nam sau goc ban
    tOut = tmin < 0.0f ? 0.0f : tmin; // ben trong hop van tinh la trung
    return true;
}

static bool rayOBB(const glm::vec3& origin, const glm::vec3& dir, const glm::mat4& model, float& tOut) {
    glm::mat4 inv = glm::inverse(model);
    glm::vec3 o = glm::vec3(inv * glm::vec4(origin, 1.0f));
    glm::vec3 d = glm::mat3(inv) * dir; // giu tham so t theo the gioi
    return rayAABB(o, d, glm::vec3(-0.5f), glm::vec3(0.5f), tOut);
}

static bool rayYCylinder(const glm::vec3& o, const glm::vec3& d,
                         float radius, float y0, float y1, float& tOut) {
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

static void accelerate(glm::vec2& vel, const glm::vec2& wish, float wishSpeed, float accel, float dt) {
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

static void applyFriction(glm::vec2& vel, float friction, float dt) {
    float sp = glm::length(vel);
    if (sp < 1e-4f) {
        vel = glm::vec2(0.0f);
        return;
    }
    float ns = std::max(sp - sp * friction * dt, 0.0f);
    vel *= ns / sp;
}

static void clampSpeed(glm::vec2& vel, float maxSpeed) {
    float sp = glm::length(vel);
    if (sp > maxSpeed && sp > 1e-6f) vel *= maxSpeed / sp;
}

// Day hinh tron (mat phang XZ) ra khoi hop
static void resolveCircleAABB(glm::vec3& pos, float r, const AABB& b) {
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

static glm::vec3 frontVector(const Game& g) {
    float pitch = g.pitch + g.recoilPitch;
    float yaw = g.yaw + g.recoilYaw;
    return glm::normalize(glm::vec3(std::cos(pitch) * std::cos(yaw),
                                    std::sin(pitch),
                                    std::cos(pitch) * std::sin(yaw)));
}

static glm::vec3 eyePos(const Game& g) {
    float h = glm::mix(EYE_HEIGHT, CROUCH_EYE, g.crouchAmt);
    return g.pos + glm::vec3(0, h, 0);
}

static glm::vec3 applySpread(const glm::vec3& dir, float spreadRad) {
    if (spreadRad <= 1e-6f) return dir;
    glm::vec3 up = std::fabs(dir.y) > 0.99f ? glm::vec3(1, 0, 0) : glm::vec3(0, 1, 0);
    glm::vec3 r = glm::normalize(glm::cross(dir, up));
    glm::vec3 u = glm::cross(r, dir);
    float theta = randf(0.0f, glm::two_pi<float>());
    float mag = std::sqrt(randf(0.0f, 1.0f)) * spreadRad;
    return glm::normalize(dir + r * (std::cos(theta) * mag) + u * (std::sin(theta) * mag));
}

static float moveInaccuracy(const Game& g) {
    float t = glm::clamp(glm::length(g.velXZ) / RUN_SPEED, 0.0f, 1.0f);
    if (!g.onGround) t = std::max(t, 0.75f);
    return t;
}

static float computeSpread(const Game& g) {
    float move = moveInaccuracy(g);
    float s;
    if (g.sprayCount == 0) {
        // Phat dau: 99.4% chinh xac (~0.27 deg khi dung yen)
        s = (1.0f - FIRST_SHOT_ACC) * glm::radians(45.0f);
        s += move * glm::radians(3.4f);
        if (!g.onGround) s += glm::radians(1.1f);
    } else {
        s = glm::radians(0.10f) + std::min(g.sprayCount, 12) * glm::radians(0.03f);
        s += move * glm::radians(3.6f);
        if (!g.onGround) s += glm::radians(1.1f);
    }
    if (g.crouchAmt > 0.5f && move < 0.15f) s *= 0.72f;
    else if (g.walking && g.onGround) s *= 0.82f;
    return s;
}

// Pattern Vandal: phat 1 khong giật; sau do leo doc den cap, ngang rat hep.
static void vandalRecoilTarget(int shotsFired, float& pitch, float& yaw) {
    if (shotsFired <= 0) {
        pitch = 0.0f;
        yaw = 0.0f;
        return;
    }
    float t = std::min(shotsFired, SPRAY_CLIMB_SHOTS) / static_cast<float>(SPRAY_CLIMB_SHOTS);
    t = 1.0f - (1.0f - t) * (1.0f - t);
    pitch = t * SPRAY_PITCH_CAP;

    static const float zig[] = {
        0.35f, -0.50f, 0.62f, -0.40f, 0.70f, -0.55f, 0.28f, -0.68f,
        0.48f, -0.32f, 0.58f, -0.45f, 0.22f, -0.60f, 0.40f, -0.38f,
        0.52f, -0.25f, 0.44f, -0.56f, 0.30f, -0.42f, 0.36f, -0.28f
    };
    yaw = zig[(shotsFired - 1) % 24] * SPRAY_YAW_MAX;
}

enum class HitZone { None, Head, Body, Legs };

static glm::vec3 worldToEnemyLocal(const glm::vec3& p, const Enemy& e) {
    glm::vec3 d = p - e.pos;
    float c = std::cos(e.yaw), s = std::sin(e.yaw);
    return glm::vec3(c * d.x - s * d.z, d.y, s * d.x + c * d.z);
}

static glm::vec3 dirToEnemyLocal(const glm::vec3& d, const Enemy& e) {
    float c = std::cos(e.yaw), s = std::sin(e.yaw);
    return glm::vec3(c * d.x - s * d.z, d.y, s * d.x + c * d.z);
}

static glm::mat4 viewMatrix(const Game& g) {
    glm::vec3 eye = eyePos(g);
    return glm::lookAt(eye, eye + frontVector(g), glm::vec3(0, 1, 0));
}

// Vi tri dau nong sung trong khong gian the gioi (dung de ve tia dan)
static glm::vec3 muzzleWorldPos(const Game& g) {
    glm::mat4 camToWorld = glm::inverse(viewMatrix(g));
    return glm::vec3(camToWorld * glm::vec4(0.22f, -0.185f, -1.1f, 1.0f));
}

// ============================================================
// Logic game
// ============================================================
static void spawnEnemy(Game& g) {
    for (int i = 0; i < 30; ++i) {
        glm::vec3 p(randf(-18, 18), 0.0f, randf(-18, 18));
        if (glm::length(glm::vec2(p.x - g.pos.x, p.z - g.pos.z)) < 14.0f) continue;
        bool bad = false;
        for (const auto& o : g_level) {
            if (p.x > o.min.x - 1 && p.x < o.max.x + 1 && p.z > o.min.z - 1 && p.z < o.max.z + 1) {
                bad = true;
                break;
            }
        }
        if (bad) continue;
        Enemy e;
        e.pos = p;
        g.enemies.push_back(e);
        return;
    }
}

static void resetGame(Game& g) {
    g = Game();
    for (int i = 0; i < 3; ++i) spawnEnemy(g);
}

static HitZone raycastEnemy(const Enemy& e, const glm::vec3& origin, const glm::vec3& dir,
                            float tMax, float& tOut) {
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
        glm::mat4 model = base * glm::translate(glm::mat4(1.0f), p.c) *
                          glm::scale(glm::mat4(1.0f), p.s * p.pad);
        float t;
        if (rayOBB(origin, dir, model, t) && t >= 0.0f && t < best) {
            best = t;
            zone = p.z;
        }
    }

    // Trụ bao toàn thân: bắn sát mặt / mép vẫn dính
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

static void shoot(Game& g) {
    glm::vec3 origin = eyePos(g);
    glm::vec3 dir = applySpread(frontVector(g), g.spread);

    // Khoang cach toi vat can gan nhat (tuong, vat the, mat dat)
    float tWorld = MAX_RANGE;
    for (const auto& o : g_level) {
        float t;
        if (rayAABB(origin, dir, o.min, o.max, t) && t < tWorld) tWorld = t;
    }
    if (dir.y < -1e-4f) {
        float t = -origin.y / dir.y;
        if (t > 0.0f && t < tWorld) tWorld = t;
    }

    Enemy* hit = nullptr;
    float tHit = tWorld;
    HitZone zone = HitZone::None;
    for (auto& e : g.enemies) {
        if (!e.alive) continue;
        float t;
        HitZone z = raycastEnemy(e, origin, dir, tHit, t);
        if (z != HitZone::None && t < tHit) {
            tHit = t;
            hit = &e;
            zone = z;
        }
    }

    glm::vec3 endPoint = origin + dir * tHit;
    g.tracers.push_back({muzzleWorldPos(g), endPoint, 0.06f});

    if (hit) {
        int dmg = (zone == HitZone::Head) ? DMG_HEAD : (zone == HitZone::Legs ? DMG_LEGS : DMG_BODY);
        hit->hp -= dmg;
        hit->hitFlash = 0.1f;
        g.hitMarker = 0.15f;
        if (hit->hp <= 0) {
            hit->alive = false;
            g.kills++;
            g.score += (zone == HitZone::Head) ? 150 : (zone == HitZone::Legs ? 50 : 100);
        }
    }
}

static void update(Game& g, GLFWwindow* win, float dt) {
    // Hieu ung
    g.hurt = std::max(0.0f, g.hurt - dt);
    g.hitMarker = std::max(0.0f, g.hitMarker - dt);
    g.muzzleFlash = std::max(0.0f, g.muzzleFlash - dt);
    g.kick = std::max(0.0f, g.kick - dt * 10.0f);
    for (auto& t : g.tracers) t.life -= dt;
    g.tracers.erase(std::remove_if(g.tracers.begin(), g.tracers.end(),
                                   [](const Tracer& t) { return t.life <= 0.0f; }),
                    g.tracers.end());

    if (g.gameOver) return;
    g.time += dt;

    // ---------- Di chuyen ----------
    bool crouchHeld = glfwGetKey(win, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS ||
                      glfwGetKey(win, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS;
    g.walking = !crouchHeld && (glfwGetKey(win, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS ||
                                glfwGetKey(win, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS);
    float crouchTarget = crouchHeld ? 1.0f : 0.0f;
    g.crouchAmt = glm::mix(g.crouchAmt, crouchTarget, 1.0f - std::exp(-dt * 12.0f));

    glm::vec3 forward(std::cos(g.yaw), 0.0f, std::sin(g.yaw));
    glm::vec3 right = glm::normalize(glm::cross(forward, glm::vec3(0, 1, 0)));
    glm::vec3 wish(0.0f);
    if (glfwGetKey(win, GLFW_KEY_W) == GLFW_PRESS) wish += forward;
    if (glfwGetKey(win, GLFW_KEY_S) == GLFW_PRESS) wish -= forward;
    if (glfwGetKey(win, GLFW_KEY_D) == GLFW_PRESS) wish += right;
    if (glfwGetKey(win, GLFW_KEY_A) == GLFW_PRESS) wish -= right;

    float maxSpeed = crouchHeld ? CROUCH_SPEED : (g.walking ? WALK_SPEED : RUN_SPEED);
    g.moving = glm::length(wish) > 0.0f;
    glm::vec2 wishXZ(wish.x, wish.z);

    if (g.onGround) {
        applyFriction(g.velXZ, GROUND_FRICTION, dt);
        if (g.moving) accelerate(g.velXZ, wishXZ, maxSpeed, GROUND_ACCEL, dt);
        clampSpeed(g.velXZ, maxSpeed);
    } else {
        if (g.moving) accelerate(g.velXZ, wishXZ, maxSpeed, AIR_ACCEL, dt);
        clampSpeed(g.velXZ, maxSpeed);
    }

    g.pos.x += g.velXZ.x * dt;
    g.pos.z += g.velXZ.y * dt;
    if (g.onGround && glm::length(g.velXZ) > 0.2f)
        g.walkTime += dt * glm::length(g.velXZ);

    // Nhay + trong luc
    if (glfwGetKey(win, GLFW_KEY_SPACE) == GLFW_PRESS && g.onGround) {
        g.velY = JUMP_SPEED;
        g.onGround = false;
    }
    g.velY -= GRAVITY * dt;
    g.pos.y += g.velY * dt;
    if (g.pos.y <= 0.0f) {
        g.pos.y = 0.0f;
        g.velY = 0.0f;
        g.onGround = true;
    }

    for (const auto& o : g_level) resolveCircleAABB(g.pos, PLAYER_RADIUS, o);
    g.spread = computeSpread(g);

    // ---------- Nap dan & ban ----------
    g.fireTimer -= dt;
    if (g.reloading) {
        g.reloadTimer -= dt;
        if (g.reloadTimer <= 0.0f) {
            g.reloading = false;
            g.ammo = MAG_SIZE;
        }
    } else if ((glfwGetKey(win, GLFW_KEY_R) == GLFW_PRESS && g.ammo < MAG_SIZE) || g.ammo == 0) {
        g.reloading = true;
        g.reloadTimer = RELOAD_TIME;
    }

    bool holdingFire = !g.reloading && g.ammo > 0 &&
                       glfwGetMouseButton(win, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;

    if (holdingFire && g.fireTimer <= 0.0f) {
        g.fireTimer = FIRE_INTERVAL;
        g.ammo--;
        g.spread = computeSpread(g);
        shoot(g);
        g.sprayCount++;
        g.sprayReset = SPRAY_RESET_TIME;
        vandalRecoilTarget(g.sprayCount, g.recoilTargetP, g.recoilTargetY);
        g.kick = glm::clamp(0.55f + std::min(g.sprayCount, 8) * 0.08f, 0.0f, 1.2f);
        g.muzzleFlash = 0.05f;
    }

    // Recoil: dang ban theo pattern; ngung ban thi ve dung tam goc (pitch/yaw chuot khong doi)
    if (holdingFire || g.fireTimer > 0.0f) {
        float k = 1.0f - std::exp(-dt * RECOIL_FOLLOW);
        g.recoilPitch = glm::mix(g.recoilPitch, g.recoilTargetP, k);
        g.recoilYaw = glm::mix(g.recoilYaw, g.recoilTargetY, k);
    } else {
        float k = 1.0f - std::exp(-dt * RECOIL_RECOVER);
        g.recoilPitch = glm::mix(g.recoilPitch, 0.0f, k);
        g.recoilYaw = glm::mix(g.recoilYaw, 0.0f, k);
        g.sprayReset -= dt;
        if (g.sprayReset <= 0.0f) {
            g.sprayCount = 0;
            g.recoilTargetP = 0.0f;
            g.recoilTargetY = 0.0f;
            g.sprayReset = 0.0f;
        }
    }

    // ---------- Sinh ke dich ----------
    g.spawnTimer -= dt;
    if (g.spawnTimer <= 0.0f) {
        if ((int)g.enemies.size() < MAX_ENEMIES) spawnEnemy(g);
        g.spawnTimer = std::max(0.8f, 2.5f - g.time * 0.015f);
    }

    // ---------- Cap nhat ke dich ----------
    float enemySpeed = std::min(4.5f, 2.0f + g.time * 0.02f);
    for (auto& e : g.enemies) {
        if (!e.alive) continue;
        e.hitFlash = std::max(0.0f, e.hitFlash - dt);
        e.attackCd -= dt;

        glm::vec2 toP(g.pos.x - e.pos.x, g.pos.z - e.pos.z);
        float d = glm::length(toP);
        if (d > 0.01f) {
            glm::vec2 dir = toP / d;
            e.yaw = std::atan2(dir.x, dir.y); // +Z cua mo hinh huong ve nguoi choi
            if (d > 1.0f) {
                e.pos.x += dir.x * enemySpeed * dt;
                e.pos.z += dir.y * enemySpeed * dt;
                e.walkPhase += dt * enemySpeed * 2.5f;
            }
        }

        // Can don
        if (d < 1.2f && e.attackCd <= 0.0f) {
            e.attackCd = 1.0f;
            g.hp -= 10;
            g.hurt = 0.3f;
            if (g.hp <= 0) {
                g.hp = 0;
                g.gameOver = true;
            }
        }
    }

    // Tach ke dich ra khoi nhau + vat can
    for (size_t i = 0; i < g.enemies.size(); ++i) {
        for (size_t j = i + 1; j < g.enemies.size(); ++j) {
            glm::vec2 diff(g.enemies[i].pos.x - g.enemies[j].pos.x,
                           g.enemies[i].pos.z - g.enemies[j].pos.z);
            float d = glm::length(diff);
            if (d < ENEMY_RADIUS * 2.0f && d > 1e-4f) {
                glm::vec2 push = diff / d * (ENEMY_RADIUS * 2.0f - d) * 0.5f;
                g.enemies[i].pos.x += push.x; g.enemies[i].pos.z += push.y;
                g.enemies[j].pos.x -= push.x; g.enemies[j].pos.z -= push.y;
            }
        }
    }
    for (auto& e : g.enemies)
        for (const auto& o : g_level) resolveCircleAABB(e.pos, ENEMY_RADIUS, o);

    g.enemies.erase(std::remove_if(g.enemies.begin(), g.enemies.end(),
                                   [](const Enemy& e) { return !e.alive; }),
                    g.enemies.end());
}

// ============================================================
// Ve
// ============================================================
static void drawBox(const glm::mat4& model, const glm::vec4& color) {
    g_shader.setM("uModel", model);
    g_shader.set4("uColor", color);
    glBindVertexArray(g_cube.vao);
    glDrawArrays(GL_TRIANGLES, 0, g_cube.count);
}

static void drawEnemy(const Enemy& e) {
    float bob = std::fabs(std::sin(e.walkPhase)) * 0.06f;
    glm::mat4 base = glm::translate(glm::mat4(1.0f), e.pos + glm::vec3(0, bob, 0)) *
                     glm::rotate(glm::mat4(1.0f), e.yaw, glm::vec3(0, 1, 0));
    float f = glm::clamp(e.hitFlash / 0.1f, 0.0f, 1.0f);

    auto part = [&](glm::vec3 c, glm::vec3 s, glm::vec4 col) {
        col = glm::mix(col, glm::vec4(1.0f), f);
        drawBox(base * glm::translate(glm::mat4(1.0f), c) * glm::scale(glm::mat4(1.0f), s), col);
    };

    part({0, 0.4f, 0},       {0.5f, 0.8f, 0.3f},    {0.2f, 0.2f, 0.5f, 1});   // chan
    part({0, 1.15f, 0},      {0.6f, 0.7f, 0.35f},   {0.75f, 0.15f, 0.15f, 1}); // than
    part({0, 1.675f, 0},     {0.35f, 0.35f, 0.35f}, {0.85f, 0.7f, 0.55f, 1});  // dau
    part({-0.38f, 1.25f, 0.2f}, {0.15f, 0.15f, 0.55f}, {0.85f, 0.7f, 0.55f, 1}); // tay trai
    part({0.38f, 1.25f, 0.2f},  {0.15f, 0.15f, 0.55f}, {0.85f, 0.7f, 0.55f, 1}); // tay phai
    part({-0.08f, 1.7f, 0.18f}, {0.06f, 0.05f, 0.02f}, {0, 0, 0, 1});            // mat
    part({0.08f, 1.7f, 0.18f},  {0.06f, 0.05f, 0.02f}, {0, 0, 0, 1});
}

static void drawGun(const Game& g, const glm::mat4& view) {
    glm::mat4 camToWorld = glm::inverse(view);

    float bobX = 0.0f, bobY = 0.0f;
    if (g.moving && g.onGround) {
        float bobAmp = glm::mix(1.0f, 0.35f, g.crouchAmt);
        if (g.walking) bobAmp *= 0.45f;
        bobX = std::sin(g.walkTime * 1.2f) * 0.008f * bobAmp;
        bobY = std::fabs(std::cos(g.walkTime * 1.2f)) * 0.01f * bobAmp;
    }
    float reloadDrop = 0.0f;
    if (g.reloading) {
        float p = 1.0f - g.reloadTimer / RELOAD_TIME;
        reloadDrop = -0.25f * std::sin(p * glm::pi<float>());
    }

    glm::vec3 origin(0.22f + bobX, -0.2f + bobY + reloadDrop, -0.6f + g.kick * 0.07f);

    auto part = [&](glm::vec3 c, glm::vec3 s, glm::vec4 col) {
        drawBox(camToWorld * glm::translate(glm::mat4(1.0f), origin + c) *
                    glm::scale(glm::mat4(1.0f), s), col);
    };

    part({0, 0, 0},          {0.07f, 0.09f, 0.45f},  {0.22f, 0.22f, 0.25f, 1}); // than sung
    part({0, 0.015f, -0.3f}, {0.035f, 0.035f, 0.25f},{0.12f, 0.12f, 0.14f, 1}); // nong sung
    part({0, -0.11f, 0.12f}, {0.06f, 0.15f, 0.07f},  {0.35f, 0.22f, 0.12f, 1}); // bao tay
    part({0, 0.06f, -0.05f}, {0.02f, 0.03f, 0.04f},  {0.1f, 0.1f, 0.1f, 1});    // dieu ngam

    if (g.muzzleFlash > 0.0f) {
        g_shader.setI("uUnlit", 1);
        part({0, 0.015f, -0.5f}, {0.12f, 0.12f, 0.12f}, {1.0f, 0.85f, 0.3f, 0.9f});
        g_shader.setI("uUnlit", 0);
    }
}

static void hudRect(float x, float y, float w, float h, const glm::vec4& c) {
    glm::mat4 m = glm::translate(glm::mat4(1.0f), glm::vec3(x + w * 0.5f, y + h * 0.5f, 0.0f)) *
                  glm::scale(glm::mat4(1.0f), glm::vec3(w, h, 1.0f));
    drawBox(m, c);
}

static void drawHUD(const Game& g) {
    const float W = (float)g_fbW, H = (float)g_fbH;

    glDisable(GL_DEPTH_TEST);
    g_shader.setM("uView", glm::mat4(1.0f));
    g_shader.setM("uProj", glm::ortho(0.0f, W, H, 0.0f, -1.0f, 1.0f));
    g_shader.setI("uUnlit", 1);

    // Mau do khi bi danh
    if (g.hurt > 0.0f)
        hudRect(0, 0, W, H, {0.8f, 0.0f, 0.0f, g.hurt / 0.3f * 0.35f});
    if (g.gameOver)
        hudRect(0, 0, W, H, {0.5f, 0.0f, 0.0f, 0.45f});

    // Tam ngam (nho, gian theo spread)
    float cx = W * 0.5f, cy = H * 0.5f;
    glm::vec4 cc = g.hitMarker > 0.0f ? glm::vec4(1.0f, 0.2f, 0.2f, 1.0f)
                                      : glm::vec4(1.0f, 1.0f, 1.0f, 0.85f);
    float arm = 5.0f;
    float th = 1.0f;
    float gap = 2.0f + g.spread * 900.0f;
    if (g.hitMarker > 0.0f) gap += 2.0f;
    hudRect(cx - gap - arm, cy - th * 0.5f, arm, th, cc);
    hudRect(cx + gap, cy - th * 0.5f, arm, th, cc);
    hudRect(cx - th * 0.5f, cy - gap - arm, th, arm, cc);
    hudRect(cx - th * 0.5f, cy + gap, th, arm, cc);

    // Thanh mau (trai duoi)
    float hpFrac = (float)g.hp / PLAYER_MAX_HP;
    hudRect(18, H - 42, 224, 22, {0, 0, 0, 0.6f});
    hudRect(20, H - 40, 220.0f * hpFrac, 18,
            glm::vec4(glm::mix(glm::vec3(0.9f, 0.15f, 0.15f), glm::vec3(0.2f, 0.85f, 0.3f), hpFrac), 1.0f));

    // Thanh dan (phai duoi); khi nap dan hien tien do
    float ammoFrac = g.reloading ? 1.0f - g.reloadTimer / RELOAD_TIME : (float)g.ammo / MAG_SIZE;
    hudRect(W - 242, H - 42, 224, 22, {0, 0, 0, 0.6f});
    hudRect(W - 240, H - 40, 220.0f * ammoFrac, 18,
            g.reloading ? glm::vec4(1.0f, 0.8f, 0.2f, 1.0f) : glm::vec4(0.9f, 0.9f, 0.95f, 1.0f));
}

static void render(const Game& g) {
    if (g_fbW <= 0 || g_fbH <= 0) return;

    glClearColor(SKY_COLOR.r, SKY_COLOR.g, SKY_COLOR.b, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);

    glm::mat4 view = viewMatrix(g);
    glm::mat4 proj = glm::perspective(glm::radians(75.0f), (float)g_fbW / (float)g_fbH, 0.05f, 200.0f);

    g_shader.use();
    g_shader.setM("uView", view);
    g_shader.setM("uProj", proj);
    g_shader.set3("uViewPos", eyePos(g));
    g_shader.set3("uLightDir", LIGHT_DIR);
    g_shader.set3("uFogColor", SKY_COLOR);
    g_shader.setI("uUnlit", 0);
    g_shader.setI("uChecker", 0);

    // Mat dat
    g_shader.setI("uChecker", 1);
    drawBox(glm::translate(glm::mat4(1.0f), glm::vec3(0, -0.5f, 0)) *
                glm::scale(glm::mat4(1.0f), glm::vec3(100, 1, 100)),
            {0.45f, 0.62f, 0.40f, 1.0f});
    g_shader.setI("uChecker", 0);

    // Tuong va vat can
    for (const auto& o : g_level) {
        glm::vec3 center = (o.min + o.max) * 0.5f;
        glm::vec3 size = o.max - o.min;
        drawBox(glm::translate(glm::mat4(1.0f), center) * glm::scale(glm::mat4(1.0f), size),
                glm::vec4(o.color, 1.0f));
    }

    // Ke dich
    for (const auto& e : g.enemies) drawEnemy(e);

    // Tia dan
    g_shader.setI("uUnlit", 1);
    for (const auto& t : g.tracers) {
        glm::vec3 d = t.b - t.a;
        float len = glm::length(d);
        if (len < 0.01f) continue;
        d /= len;
        glm::vec3 up = std::fabs(d.y) > 0.99f ? glm::vec3(1, 0, 0) : glm::vec3(0, 1, 0);
        glm::mat4 m = glm::inverse(glm::lookAt(t.a, t.b, up)) *
                      glm::translate(glm::mat4(1.0f), glm::vec3(0, 0, -len * 0.5f)) *
                      glm::scale(glm::mat4(1.0f), glm::vec3(0.015f, 0.015f, len));
        drawBox(m, {1.0f, 0.9f, 0.4f, glm::clamp(t.life / 0.06f, 0.0f, 1.0f)});
    }
    g_shader.setI("uUnlit", 0);

    // Sung (xoa depth de khong bi xuyen vao tuong)
    if (!g.gameOver) {
        glClear(GL_DEPTH_BUFFER_BIT);
        drawGun(g, view);
    }

    drawHUD(g);
}

// ============================================================
// main
// ============================================================
int main() {
    if (!glfwInit()) {
        std::cerr << "Khong the khoi tao GLFW\n";
        return -1;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
    glfwWindowHint(GLFW_SAMPLES, 4);

    GLFWwindow* window = glfwCreateWindow(1280, 720, "Shooter 3D", nullptr, nullptr);
    if (!window) {
        std::cerr << "Khong the tao cua so\n";
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    int version = gladLoadGL((GLADloadfunc)glfwGetProcAddress);
    if (version == 0) {
        std::cerr << "Khong the khoi tao GLAD\n";
        return -1;
    }
    std::cout << "OpenGL " << GLAD_VERSION_MAJOR(version) << "." << GLAD_VERSION_MINOR(version) << "\n";

    glfwGetFramebufferSize(window, &g_fbW, &g_fbH);
    glViewport(0, 0, g_fbW, g_fbH);
    glfwSetFramebufferSizeCallback(window, [](GLFWwindow*, int w, int h) {
        g_fbW = w;
        g_fbH = h;
        glViewport(0, 0, w, h);
    });

    // Khoa chuot vao cua so
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    if (glfwRawMouseMotionSupported())
        glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_MULTISAMPLE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    g_shader.build();
    g_cube = createCubeMesh();
    buildLevel();

    Game game;
    resetGame(game);

    double lastX = 0.0, lastY = 0.0;
    bool firstMouse = true;
    double lastTime = glfwGetTime();
    double titleTimer = 0.0;
    bool enterWasPressed = false;

    while (!glfwWindowShouldClose(window)) {
        double now = glfwGetTime();
        float dt = std::min((float)(now - lastTime), 0.05f);
        lastTime = now;

        glfwPollEvents();
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, GLFW_TRUE);

        // Nhin theo chuot
        double mx, my;
        glfwGetCursorPos(window, &mx, &my);
        if (firstMouse) {
            lastX = mx;
            lastY = my;
            firstMouse = false;
        }
        game.yaw += (float)(mx - lastX) * MOUSE_SENS;
        game.pitch -= (float)(my - lastY) * MOUSE_SENS;
        game.pitch = glm::clamp(game.pitch, glm::radians(-89.0f), glm::radians(89.0f));
        lastX = mx;
        lastY = my;

        // Choi lai
        bool enter = glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_PRESS;
        if (enter && !enterWasPressed && game.gameOver) resetGame(game);
        enterWasPressed = enter;

        update(game, window, dt);
        render(game);

        titleTimer += dt;
        if (titleTimer > 0.1) {
            titleTimer = 0.0;
            std::string title;
            if (game.gameOver) {
                title = "GAME OVER - Diem: " + std::to_string(game.score) +
                        " - Ha guc: " + std::to_string(game.kills) + " - Nhan Enter de choi lai";
            } else {
                title = "Shooter 3D | Diem: " + std::to_string(game.score) +
                        " | Mau: " + std::to_string(game.hp) +
                        " | Dan: " + (game.reloading ? std::string("dang nap...")
                                                     : std::to_string(game.ammo) + "/" + std::to_string(MAG_SIZE)) +
                        " | Dich: " + std::to_string(game.enemies.size());
            }
            glfwSetWindowTitle(window, title.c_str());
        }

        glfwSwapBuffers(window);
    }

    glDeleteVertexArrays(1, &g_cube.vao);
    glDeleteBuffers(1, &g_cube.vbo);
    glDeleteProgram(g_shader.id);
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}