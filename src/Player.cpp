#include "Player.hpp"

#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <cmath>

// Pattern giat M4: leo doc den cap, ngang rat hep
static void sprayTarget(int shots, const WeaponDef& d, float& pitch, float& yaw) {
    if (shots <= 0) { pitch = yaw = 0.0f; return; }
    float t = std::min(shots, d.sprayClimbShots) / static_cast<float>(d.sprayClimbShots);
    t = 1.0f - (1.0f - t) * (1.0f - t);
    pitch = t * d.sprayPitchCap;
    static const float zig[] = {
        0.35f, -0.50f, 0.62f, -0.40f, 0.70f, -0.55f, 0.28f, -0.68f,
        0.48f, -0.32f, 0.58f, -0.45f, 0.22f, -0.60f, 0.40f, -0.38f,
        0.52f, -0.25f, 0.44f, -0.56f, 0.30f, -0.42f, 0.36f, -0.28f
    };
    yaw = zig[(shots - 1) % 24] * d.sprayYawMax;
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

void Player::init(const Model* m4, const Model* mark23, Audio* audio) {
    weapons_[static_cast<int>(WeaponId::M4)].init(WeaponId::M4, m4, audio);
    weapons_[static_cast<int>(WeaponId::Mark23)].init(WeaponId::Mark23, mark23, audio);
    reset();
}

void Player::reset() {
    for (auto& w : weapons_) w.resetAmmo();
    current_ = static_cast<int>(WeaponId::M4);
    pending_ = -1;
    weapons_[current_].beginDraw();

    pos_ = glm::vec3(0.0f);
    velXZ_ = glm::vec2(0.0f);
    velY_ = 0.0f;
    onGround_ = true;
    yaw_ = -glm::half_pi<float>();
    pitch_ = 0.0f;
    crouchAmt_ = 0.0f;
    sprayP_ = sprayY_ = sprayTargetP_ = sprayTargetY_ = 0.0f;
    sprayCount_ = 0;
    sprayReset_ = 0.0f;
    kickP_ = kickY_ = kickFromP_ = kickFromY_ = 0.0f;
    kickT_ = 10.0f;
    fireBuffer_ = 0.0f;
    visualKick_ = muzzleFlash_ = hurt_ = hitMarker_ = 0.0f;
    hp_ = PLAYER_MAX_HP;
    score_ = kills_ = 0;
}

void Player::look(const glm::vec2& d, float radPerPixel) {
    yaw_ += d.x * radPerPixel;
    pitch_ -= d.y * radPerPixel;
    pitch_ = glm::clamp(pitch_, glm::radians(-89.0f), glm::radians(89.0f));
}

void Player::takeDamage(int dmg) {
    if (dmg <= 0) return;
    hp_ = std::max(0, hp_ - dmg);
    hurt_ = 0.3f;
}

glm::vec3 Player::eyePos() const {
    return pos_ + glm::vec3(0, glm::mix(EYE_HEIGHT, CROUCH_EYE, crouchAmt_), 0);
}

glm::vec3 Player::front() const {
    float p = glm::clamp(pitch_ + recoilPitch(), glm::radians(-89.0f), glm::radians(89.0f));
    float y = yaw_ + recoilYaw();
    return glm::normalize(glm::vec3(std::cos(p) * std::cos(y), std::sin(p), std::cos(p) * std::sin(y)));
}

glm::mat4 Player::viewMatrix() const {
    glm::vec3 eye = eyePos();
    return glm::lookAt(eye, eye + front(), glm::vec3(0, 1, 0));
}

glm::vec3 Player::muzzleWorld(const glm::mat4& view) const {
    return glm::vec3(glm::inverse(view) * glm::vec4(weapon().def().muzzleCam, 1.0f));
}

void Player::updateEffects(float dt) {
    hurt_ = std::max(0.0f, hurt_ - dt);
    hitMarker_ = std::max(0.0f, hitMarker_ - dt);
    muzzleFlash_ = std::max(0.0f, muzzleFlash_ - dt);
    visualKick_ = std::max(0.0f, visualKick_ - dt * 10.0f);
}

// ============================================================
void Player::move(const Input& in, GLFWwindow*, float dt, World& world) {
    bool crouchHeld = in.down(GLFW_KEY_LEFT_CONTROL) || in.down(GLFW_KEY_RIGHT_CONTROL);
    walking_ = !crouchHeld && (in.down(GLFW_KEY_LEFT_SHIFT) || in.down(GLFW_KEY_RIGHT_SHIFT));
    crouchAmt_ = glm::mix(crouchAmt_, crouchHeld ? 1.0f : 0.0f, 1.0f - std::exp(-dt * 12.0f));

    glm::vec3 forward(std::cos(yaw_), 0.0f, std::sin(yaw_));
    glm::vec3 right = glm::normalize(glm::cross(forward, glm::vec3(0, 1, 0)));
    glm::vec3 wish(0.0f);
    if (in.down(GLFW_KEY_W)) wish += forward;
    if (in.down(GLFW_KEY_S)) wish -= forward;
    if (in.down(GLFW_KEY_D)) wish += right;
    if (in.down(GLFW_KEY_A)) wish -= right;

    float maxSpeed = crouchHeld ? CROUCH_SPEED : (walking_ ? WALK_SPEED : RUN_SPEED);
    moving_ = glm::length(wish) > 0.0f;
    glm::vec2 wishXZ(wish.x, wish.z);

    if (onGround_) {
        applyFriction(velXZ_, GROUND_FRICTION, dt);
        if (moving_) accelerate(velXZ_, wishXZ, maxSpeed, GROUND_ACCEL, dt);
    } else if (moving_) {
        accelerate(velXZ_, wishXZ, maxSpeed, AIR_ACCEL, dt);
    }
    clampSpeed(velXZ_, maxSpeed);

    pos_.x += velXZ_.x * dt;
    pos_.z += velXZ_.y * dt;
    if (onGround_ && glm::length(velXZ_) > 0.2f) walkTime_ += dt * glm::length(velXZ_);

    if (in.down(GLFW_KEY_SPACE) && onGround_) {
        velY_ = JUMP_SPEED;
        onGround_ = false;
    }
    velY_ -= GRAVITY * dt;
    pos_.y += velY_ * dt;
    if (pos_.y <= 0.0f) {
        pos_.y = 0.0f;
        velY_ = 0.0f;
        onGround_ = true;
    }
    for (const auto& o : world.level()) resolveCircleAABB(pos_, PLAYER_RADIUS, o);
}

float Player::computeSpread(const Weapon& w) const {
    const WeaponDef& d = w.def();
    float move = glm::clamp(glm::length(velXZ_) / RUN_SPEED, 0.0f, 1.0f);
    if (!onGround_) move = std::max(move, 0.75f);

    float s;
    if (d.recoil == RecoilKind::Kick || sprayCount_ == 0) {
        s = glm::radians(d.spreadFirstDeg) + move * glm::radians(d.spreadMoveDeg);
        if (d.recoil == RecoilKind::Kick) s += std::fabs(kickP_) * 0.25f;   // vua giat xong thi kem chinh xac
    } else {
        s = glm::radians(d.spreadSprayDeg) + std::min(sprayCount_, 12) * glm::radians(0.03f) +
            move * glm::radians(d.spreadMoveDeg + 0.2f);
    }
    if (!onGround_) s += glm::radians(1.1f);
    if (crouchAmt_ > 0.5f && move < 0.15f) s *= 0.72f;
    else if (walking_ && onGround_) s *= 0.82f;
    return s;
}

void Player::requestSlot(int slot) {
    slot = glm::clamp(slot, 0, WEAPON_COUNT - 1);
    int target = pending_ >= 0 ? pending_ : current_;
    if (slot == target) return;
    pending_ = slot;
}

void Player::handleSwitching(const Input& in, float) {
    if (in.pressed(GLFW_KEY_1)) requestSlot(0);
    if (in.pressed(GLFW_KEY_2)) requestSlot(1);

    // Con lan: KHONG tuan hoan. Cuon len = sung truoc (slot 1), cuon xuong = sung sau (slot 2), dung o dau cuoi.
    float sc = in.scrollY();
    if (sc > 0.5f)  requestSlot((pending_ >= 0 ? pending_ : current_) - 1);
    if (sc < -0.5f) requestSlot((pending_ >= 0 ? pending_ : current_) + 1);
}

void Player::shoot(Weapon& w, World& world) {
    const WeaponDef& d = w.def();
    spread_ = computeSpread(w);

    glm::vec3 origin = eyePos();
    glm::vec3 dir = applySpread(front(), spread_);
    glm::mat4 view = viewMatrix();
    glm::vec3 muzzle = muzzleWorld(view);

    w.onFire();
    ShotResult r = world.hitscan(origin, dir, d.dmgHead, d.dmgBody, d.dmgLegs);
    world.addTracer(muzzle, r.endPoint);

    if (r.hitEnemy) {
        hitMarker_ = 0.15f;
        if (r.killed) {
            kills_++;
            score_ += (r.zone == HitZone::Head) ? 150 : (r.zone == HitZone::Legs ? 50 : 100);
            // Phan thuong: giet 1 quai -> M4 +3 dan (Mark23 vo han nen khong can)
            weapons_[static_cast<int>(WeaponId::M4)].addReserve(M4_KILL_AMMO_REWARD);
        }
    }
    if (r.hitSurface) world.addBulletHole(r.endPoint, r.surfaceNormal);

    // Recoil
    if (d.recoil == RecoilKind::Spray) {
        sprayCount_++;
        sprayReset_ = 0.40f;
        sprayTarget(sprayCount_, d, sprayTargetP_, sprayTargetY_);
    } else {
        float t = std::min(kickT_ / d.kickRecover, 1.0f);
        float f = 1.0f - t * t * (3.0f - 2.0f * t);            // gia tri muc tieu hien tai
        kickFromP_ = std::min(kickFromP_ * f + d.kickPitch, 0.35f);
        kickFromY_ = kickFromY_ * f + randf(-1.0f, 1.0f) * d.kickYaw;
        kickT_ = 0.0f;
    }
    sinceShot_ = 0.0f;
    visualKick_ = d.visualKick;
    muzzleFlash_ = 0.05f;
}

void Player::updateRecoil(const Input& in, float dt) {
    const Weapon& w = weapons_[current_];
    const WeaponDef& d = w.def();
    sinceShot_ += dt;

    // ----- Spray (M4) -----
    bool holding = d.recoil == RecoilKind::Spray && d.automatic && in.mouseDown(GLFW_MOUSE_BUTTON_LEFT) &&
                   w.state() == Weapon::State::Idle && w.mag() > 0;
    if (holding || (d.recoil == RecoilKind::Spray && sinceShot_ < d.fireInterval)) {
        float k = 1.0f - std::exp(-dt * 36.0f);
        sprayP_ = glm::mix(sprayP_, sprayTargetP_, k);
        sprayY_ = glm::mix(sprayY_, sprayTargetY_, k);
    } else {
        float k = 1.0f - std::exp(-dt * 13.0f);
        sprayP_ = glm::mix(sprayP_, 0.0f, k);
        sprayY_ = glm::mix(sprayY_, 0.0f, k);
        sprayReset_ -= dt;
        if (sprayReset_ <= 0.0f) {
            sprayCount_ = 0;
            sprayTargetP_ = sprayTargetY_ = 0.0f;
            sprayReset_ = 0.0f;
        }
    }

    // ----- Kick (Mark23): muc tieu ve 0 trong kickRecover giay, hien thi bam theo rat nhanh -----
    float recover = std::max(weaponDef(WeaponId::Mark23).kickRecover, 0.01f);
    kickT_ += dt;
    float t = std::min(kickT_ / recover, 1.0f);
    float f = 1.0f - t * t * (3.0f - 2.0f * t);
    float k = 1.0f - std::exp(-dt * 35.0f);
    kickP_ += (kickFromP_ * f - kickP_) * k;
    kickY_ += (kickFromY_ * f - kickY_) * k;
    if (t >= 1.0f && std::fabs(kickP_) < 1e-4f) kickP_ = kickY_ = 0.0f;
}

void Player::update(const Input& in, GLFWwindow* win, float dt, World& world) {
    updateEffects(dt);
    move(in, win, dt, world);

    // ----- Doi sung -----
    handleSwitching(in, dt);
    {
        Weapon& cur = weapons_[current_];
        bool busy = cur.state() == Weapon::State::Holstering;
        if (pending_ == current_ && !busy) pending_ = -1;
        if (pending_ >= 0 && pending_ != current_ &&
            (cur.state() == Weapon::State::Idle || cur.state() == Weapon::State::Reloading)) {
            cur.cancelReload();          // dang nap ma doi sung thi huy nap
            cur.beginHolster();
        }
    }
    {
        Weapon::Event ev = weapons_[current_].update(dt);
        if (ev == Weapon::Event::HolsterDone) {
            if (pending_ >= 0) current_ = pending_;
            pending_ = -1;
            weapons_[current_].beginDraw();
        }
    }

    Weapon& w = weapons_[current_];
    const WeaponDef& d = w.def();
    spread_ = computeSpread(w);

    // ----- Nap dan -----
    if (w.state() == Weapon::State::Idle) {
        if (in.pressed(GLFW_KEY_R) && w.canReload()) w.startReload();
        else if (w.mag() == 0 && w.hasReserve()) w.startReload();
    }

    // ----- Ban -----
    if (in.mousePressed(GLFW_MOUSE_BUTTON_LEFT)) fireBuffer_ = 0.15f;
    fireBuffer_ -= dt;
    bool want = d.automatic ? in.mouseDown(GLFW_MOUSE_BUTTON_LEFT) : fireBuffer_ > 0.0f;
    if (want && w.canFire()) {
        shoot(w, world);
        fireBuffer_ = 0.0f;
    }

    updateRecoil(in, dt);
}

// ============================================================
void Player::drawViewmodel(Shaders& sh, const glm::mat4& view, const glm::mat4& proj, bool fx) {
    const Weapon& w = weapons_[current_];
    const WeaponDef& d = w.def();

    glClear(GL_DEPTH_BUFFER_BIT);   // khong xuyen vao tuong

    float bobX = 0.0f, bobY = 0.0f;
    if (moving_ && onGround_) {
        float amp = glm::mix(1.0f, 0.35f, crouchAmt_);
        if (walking_) amp *= 0.45f;
        bobX = std::sin(walkTime_ * 1.2f) * 0.008f * amp;
        bobY = std::fabs(std::cos(walkTime_ * 1.2f)) * 0.01f * amp;
    }
    glm::vec3 off = d.viewOffset + glm::vec3(bobX, bobY, visualKick_ * 0.03f);

    glm::mat4 camToWorld = glm::inverse(view);
    glm::mat4 M = camToWorld * glm::translate(glm::mat4(1.0f), off) *
                  glm::rotate(glm::mat4(1.0f), visualKick_ * 0.05f, glm::vec3(1, 0, 0)) *
                  glm::rotate(glm::mat4(1.0f), glm::pi<float>(), glm::vec3(0, 1, 0)) *    // model nhin +Z, camera nhin -Z
                  glm::scale(glm::mat4(1.0f), glm::vec3(d.viewScale)) *
                  glm::translate(glm::mat4(1.0f), -w.anchor());

    Shader& s = sh.viewmodel;
    s.use();
    s.setM("uView", view);
    s.setM("uProj", proj);
    s.set3("uLightDir", World::LIGHT_DIR);
    s.set3("uViewPos", eyePos());
    s.setI("uFx", fx ? 1 : 0);
    s.setF("uFlash", muzzleFlash_ > 0.0f ? 1.0f : 0.0f);
    w.model()->draw(s, M, w.animator().globals());
}
