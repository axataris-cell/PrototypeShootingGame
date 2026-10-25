#include "Animator.hpp"

#include <algorithm>

void Animator::init(const Model* model, int idleClip) {
    model_ = model;
    idleClip_ = idleClip;
    clip_ = -1;
    time_ = 0.0f;
    evaluate();
}

void Animator::play(int clip, float speed) {
    if (!model_ || clip < 0 || clip >= model_->clipCount()) return;
    clip_ = clip;
    time_ = 0.0f;
    speed_ = speed;
    evaluate();
}

void Animator::stopToIdle() {
    clip_ = -1;
    evaluate();
}

bool Animator::update(float dt) {
    bool finished = false;
    if (clip_ >= 0) {
        time_ += dt * speed_;
        if (time_ >= model_->clip(clip_).duration()) {
            clip_ = -1;
            finished = true;
        }
    }
    evaluate();
    return finished;
}

float Animator::progress() const {
    if (clip_ < 0 || !model_) return 0.0f;
    float d = model_->clip(clip_).duration();
    return d > 1e-5f ? std::clamp(time_ / d, 0.0f, 1.0f) : 1.0f;
}

float Animator::remaining() const {
    if (clip_ < 0 || !model_) return 0.0f;
    return std::max(0.0f, (model_->clip(clip_).duration() - time_) / std::max(speed_, 1e-3f));
}

float Animator::clipSeconds(int clip, float speed) const {
    if (!model_ || clip < 0) return 0.0f;
    return model_->clip(clip).duration() / std::max(speed, 1e-3f);
}

void Animator::evaluate() {
    if (!model_) return;
    if (clip_ >= 0) {
        const Clip& c = model_->clip(clip_);
        model_->evaluate(clip_, c.start + std::min(time_, c.duration()), globals_);
    } else if (idleClip_ >= 0) {
        model_->evaluate(idleClip_, model_->clip(idleClip_).end, globals_);
    } else {
        model_->evaluate(-1, 0.0f, globals_);
    }
}
