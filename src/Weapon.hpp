#pragma once
#include "Animator.hpp"
#include "Audio.hpp"

#include <glm/glm.hpp>

enum class WeaponId { M4 = 0, Mark23 = 1 };
constexpr int WEAPON_COUNT = 2;

enum class RecoilKind {
    Spray,   // leo dan theo pattern khi ban lien thanh, ve tam khi ngung (M4)
    Kick     // giat manh moi phat, ve tam sau kickRecover giay (Mark23)
};

struct WeaponDef {
    const char* name;
    const char* modelFile;      // trong assets/model
    const char* anchorNode;     // node dung lam moc dat sung truoc camera

    // Ten clip animation trong file GLB
    const char* clipDraw;
    const char* clipHolster;
    const char* clipReload;
    const char* clipReloadEmpty;   // nullptr = dung clipReload
    const char* clipFire;

    // Dan
    int   magSize;
    bool  infiniteReserve;
    int   startReserve;

    // Ban
    float fireInterval;
    bool  automatic;
    int   dmgHead, dmgBody, dmgLegs;
    float spreadFirstDeg, spreadMoveDeg, spreadSprayDeg;
    Sfx   fireSfx;

    // Toc do phat animation (1 = nguyen goc)
    float reloadSpeed, drawSpeed, holsterSpeed, fireAnimSpeed;

    // Recoil
    RecoilKind recoil;
    float sprayPitchCap;     // rad
    int   sprayClimbShots;
    float sprayYawMax;       // rad
    float kickPitch;         // rad moi phat (Kick)
    float kickYaw;           // rad (ngau nhien +-)
    float kickRecover;       // giay de ve tam hoan toan (Kick)

    // Hien thi viewmodel
    float     viewScale;     // he so nhan them (Mark23 la cm -> 0.01)
    glm::vec3 viewOffset;    // vi tri node anchor trong khong gian camera
    glm::vec3 muzzleCam;     // dau nong sung trong khong gian camera (tia dan / lua dau nong)
    float     visualKick;    // do giat hinh anh cua sung (lui ve sau)
};

const WeaponDef& weaponDef(WeaponId id);

class Weapon {
public:
    enum class State { Idle, Reloading, Holstering, Drawing };
    enum class Event { None, HolsterDone };

    void init(WeaponId id, const Model* model, Audio* audio);
    void resetAmmo();

    Event update(float dt);

    bool canFire() const { return state_ == State::Idle && mag_ > 0 && fireTimer_ <= 0.0f; }
    void onFire();
    bool hasReserve() const { return def_->infiniteReserve || reserve_ > 0; }
    bool canReload() const { return state_ == State::Idle && mag_ < def_->magSize && hasReserve(); }
    void startReload();
    void cancelReload();
    void beginHolster();
    void beginDraw();
    void addReserve(int n) { if (!def_->infiniteReserve) reserve_ += n; }

    WeaponId id() const { return id_; }
    const WeaponDef& def() const { return *def_; }
    State state() const { return state_; }
    int mag() const { return mag_; }
    int reserve() const { return reserve_; }
    float reloadProgress() const { return state_ == State::Reloading ? anim_.progress() : 0.0f; }
    const Animator& animator() const { return anim_; }
    const Model* model() const { return model_; }
    const glm::vec3& anchor() const { return anchor_; }

private:
    int clip(const char* name) const { return name ? model_->findClip(name) : -1; }

    WeaponId id_ = WeaponId::M4;
    const WeaponDef* def_ = nullptr;
    const Model* model_ = nullptr;
    Audio* audio_ = nullptr;
    Animator anim_;
    State state_ = State::Idle;
    int mag_ = 0, reserve_ = 0;
    float fireTimer_ = 0.0f;
    glm::vec3 anchor_{0.0f};
};
