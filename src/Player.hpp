#pragma once
#include "Audio.hpp"
#include "Config.hpp"
#include "Input.hpp"
#include "Model.hpp"
#include "Shader.hpp"
#include "Weapon.hpp"
#include "World.hpp"

#include <array>
#include <glm/glm.hpp>

class Player {
public:
    void init(const Model* m4, const Model* mark23, Audio* audio);
    void reset();                                             // van moi

    void look(const glm::vec2& mouseDelta, float radPerPixel);
    void update(const Input& in, GLFWwindow* win, float dt, World& world);
    void updateEffects(float dt);                             // hieu ung khi game over van chay
    void takeDamage(int dmg);

    glm::vec3 eyePos() const;
    glm::vec3 front() const;
    glm::mat4 viewMatrix() const;

    // Ve sung + tay. Goi sau khi ve the gioi.
    void drawViewmodel(Shaders& sh, const glm::mat4& view, const glm::mat4& proj, bool fx);
    // Dau nong sung trong khong gian the gioi
    glm::vec3 muzzleWorld(const glm::mat4& view) const;

    // ----- Trang thai cho HUD -----
    int hp() const { return hp_; }
    bool dead() const { return hp_ <= 0; }
    int score() const { return score_; }
    int kills() const { return kills_; }
    float hurt() const { return hurt_; }
    float hitMarker() const { return hitMarker_; }
    float spread() const { return spread_; }
    float muzzleFlash() const { return muzzleFlash_; }
    const Weapon& weapon() const { return weapons_[current_]; }
    const Weapon& weaponAt(int slot) const { return weapons_[slot]; }
    int currentSlot() const { return current_; }
    int pendingSlot() const { return pending_; }
    const glm::vec3& position() const { return pos_; }

private:
    void move(const Input& in, GLFWwindow* win, float dt, World& world);
    void handleSwitching(const Input& in, float dt);
    void shoot(Weapon& w, World& world);
    void updateRecoil(const Input& in, float dt);
    float computeSpread(const Weapon& w) const;
    void requestSlot(int slot);
    float recoilPitch() const { return sprayP_ + kickP_; }
    float recoilYaw() const { return sprayY_ + kickY_; }

    std::array<Weapon, WEAPON_COUNT> weapons_;
    int current_ = 0;
    int pending_ = -1;

    // Chuyen dong
    glm::vec3 pos_{0.0f};
    glm::vec2 velXZ_{0.0f};
    float velY_ = 0.0f;
    bool onGround_ = true;
    float yaw_ = -1.5707963f;
    float pitch_ = 0.0f;
    float crouchAmt_ = 0.0f;
    bool walking_ = false;
    bool moving_ = false;
    float walkTime_ = 0.0f;

    // Recoil kieu Spray (M4)
    float sprayP_ = 0.0f, sprayY_ = 0.0f;
    float sprayTargetP_ = 0.0f, sprayTargetY_ = 0.0f;
    int sprayCount_ = 0;
    float sprayReset_ = 0.0f;
    float sinceShot_ = 10.0f;

    // Recoil kieu Kick (Mark23): tang nhanh, ve tam dung kickRecover giay
    float kickP_ = 0.0f, kickY_ = 0.0f;
    float kickFromP_ = 0.0f, kickFromY_ = 0.0f;
    float kickT_ = 10.0f;

    float fireBuffer_ = 0.0f;
    float spread_ = 0.0f;
    float visualKick_ = 0.0f;
    float muzzleFlash_ = 0.0f;
    float hurt_ = 0.0f;
    float hitMarker_ = 0.0f;
    int hp_ = PLAYER_MAX_HP;
    int score_ = 0;
    int kills_ = 0;
};
