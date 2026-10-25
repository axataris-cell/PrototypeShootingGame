#pragma once
#include "Model.hpp"

// Phat clip animation cua Model (khong lap). Khi het clip se tro ve "pose idle"
// = khung cuoi cua clip idle (vd clip Draw).
class Animator {
public:
    void init(const Model* model, int idleClip);
    void play(int clip, float speed = 1.0f);
    void stopToIdle();
    // Tra ve true o khung hinh clip vua ket thuc
    bool update(float dt);

    bool playing() const { return clip_ >= 0; }
    int currentClip() const { return clip_; }
    float progress() const;                       // 0..1
    float remaining() const;                      // giay con lai (theo toc do hien tai)
    float clipSeconds(int clip, float speed) const;
    const std::vector<glm::mat4>& globals() const { return globals_; }

private:
    void evaluate();

    const Model* model_ = nullptr;
    int idleClip_ = -1;
    int clip_ = -1;
    float time_ = 0.0f;
    float speed_ = 1.0f;
    std::vector<glm::mat4> globals_;
};
