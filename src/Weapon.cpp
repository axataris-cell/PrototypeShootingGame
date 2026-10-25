#include "Weapon.hpp"

#include <algorithm>

static const WeaponDef DEFS[WEAPON_COUNT] = {
    // ---------------- M4 ----------------
    {
        "M4", "model/M4.glb", "Main_j_049",
        "Draw", "Holster", "Reload", "Reload_Empty", "Fire",
        30, false, 60,                                   // bang dan 30, khong co dan du phong luc dau
        0.10f, true,
        150, 60, 22,
        0.27f, 3.4f, 0.10f,
        Sfx::M4Shot,
        1.5f, 1.3f, 1.4f, 3.0f,
        RecoilKind::Spray, 0.102f, 8, 0.0044f,
        0.0f, 0.0f, 0.0f,
        1.0f, glm::vec3(0.19f, -0.19f, -0.52f), glm::vec3(0.19f, -0.155f, -1.03f), 0.55f
    },
    // ---------------- Mark23 ----------------
    {
        "MARK23", "model/Mark23.glb", "main_j_050",
        "Draw", "Hide", "Reload", nullptr, "Shoot",
        12, true, 0,                                    // dan vo han
        0.28f, false,
        150, 50, 20,
        0.15f, 2.5f, 0.0f,
        Sfx::Mark23Shot,
        1.5f, 1.3f, 1.4f, 1.2f,
        RecoilKind::Kick, 0.0f, 0, 0.0f,
        0.085f, 0.014f, 1.0f,                           // giat ~4.9 do, hoi tam 1 giay
        0.01f, glm::vec3(0.07f, -0.17f, -0.28f), glm::vec3(0.07f, -0.085f, -0.49f), 1.0f
    },
};

const WeaponDef& weaponDef(WeaponId id) { return DEFS[static_cast<int>(id)]; }

void Weapon::init(WeaponId id, const Model* model, Audio* audio) {
    id_ = id;
    def_ = &weaponDef(id);
    model_ = model;
    audio_ = audio;
    anim_.init(model_, clip(def_->clipDraw));
    int an = model_->findNode(def_->anchorNode);
    anchor_ = model_->nodePosition(anim_.globals(), an);   // vi tri anchor o pose idle
    resetAmmo();
}

void Weapon::resetAmmo() {
    mag_ = def_->magSize;
    reserve_ = def_->startReserve;
    fireTimer_ = 0.0f;
    state_ = State::Idle;
    anim_.stopToIdle();
}

void Weapon::onFire() {
    --mag_;
    fireTimer_ = def_->fireInterval;
    anim_.play(clip(def_->clipFire), def_->fireAnimSpeed);
    if (audio_) audio_->play(def_->fireSfx);
}

void Weapon::startReload() {
    if (!canReload()) return;
    int c = -1;
    if (mag_ == 0 && def_->clipReloadEmpty) c = clip(def_->clipReloadEmpty);
    if (c < 0) c = clip(def_->clipReload);
    state_ = State::Reloading;
    anim_.play(c, def_->reloadSpeed);
    if (audio_) audio_->play(Sfx::Reload);
}

void Weapon::cancelReload() {
    if (state_ != State::Reloading) return;
    state_ = State::Idle;
    anim_.stopToIdle();
    if (audio_) audio_->stop(Sfx::Reload);
}

void Weapon::beginHolster() {
    state_ = State::Holstering;
    anim_.play(clip(def_->clipHolster), def_->holsterSpeed);
}

void Weapon::beginDraw() {
    state_ = State::Drawing;
    anim_.play(clip(def_->clipDraw), def_->drawSpeed);
}

Weapon::Event Weapon::update(float dt) {
    fireTimer_ -= dt;
    Event ev = Event::None;
    if (anim_.update(dt)) {                      // clip vua ket thuc
        switch (state_) {
        case State::Reloading: {
            int need = def_->magSize - mag_;
            int take = def_->infiniteReserve ? need : std::min(need, reserve_);
            mag_ += take;
            if (!def_->infiniteReserve) reserve_ -= take;
            state_ = State::Idle;
            break;
        }
        case State::Holstering:
            ev = Event::HolsterDone;
            break;
        case State::Drawing:
            state_ = State::Idle;
            break;
        default: break;
        }
    }
    return ev;
}
