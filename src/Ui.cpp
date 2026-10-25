#include "Ui.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>

void Ui::begin(int fbW, int fbH) {
    scale_ = std::max(0.5f, fbH / 720.0f);
    (void)fbW;
    lastCount_ = counter_;
    counter_ = 0;
    if (lastCount_ > 0) {
        if (in_->pressed(GLFW_KEY_DOWN) || in_->pressed(GLFW_KEY_S)) focus_ = (focus_ + 1) % lastCount_;
        if (in_->pressed(GLFW_KEY_UP) || in_->pressed(GLFW_KEY_W)) focus_ = (focus_ + lastCount_ - 1) % lastCount_;
    }
    if (!in_->mouseDown(GLFW_MOUSE_BUTTON_LEFT)) dragging_ = -1;
}

bool Ui::hovered(float x, float y, float w, float h) const {
    glm::vec2 m = in_->mousePos();
    return m.x >= x && m.x <= x + w && m.y >= y && m.y <= y + h;
}

bool Ui::button(const std::string& label, float cx, float y, float w, float h) {
    int id = nextId();
    float x = cx - w * 0.5f;
    bool hov = hovered(x, y, w, h);
    if (hov && (in_->mouseDelta().x != 0.0f || in_->mouseDelta().y != 0.0f)) focus_ = id;
    bool focused = focus_ == id;

    glm::vec4 bg = focused ? glm::vec4(0.95f, 0.45f, 0.12f, 0.92f) : glm::vec4(0.08f, 0.09f, 0.12f, 0.82f);
    r_->rect(x, y, w, h, bg);
    r_->rectOutline(x, y, w, h, 2.0f * scale_, focused ? glm::vec4(1, 0.9f, 0.7f, 1) : glm::vec4(0.7f, 0.72f, 0.8f, 0.7f));
    float ts = 3.0f * scale_;
    r_->text(label, cx, y + (h - Renderer2D::textHeight(ts)) * 0.5f, ts,
             focused ? glm::vec4(0.05f, 0.05f, 0.05f, 1) : glm::vec4(0.92f, 0.94f, 1, 1), TextAlign::Center);

    bool clicked = (hov && in_->mousePressed(GLFW_MOUSE_BUTTON_LEFT));
    if (focused && (in_->pressed(GLFW_KEY_ENTER) || in_->pressed(GLFW_KEY_SPACE) || in_->pressed(GLFW_KEY_KP_ENTER)))
        clicked = true;
    return clicked;
}

bool Ui::slider(const std::string& label, float& value, float minV, float maxV, float step,
                float cx, float y, float w, float h) {
    int id = nextId();
    float x = cx - w * 0.5f;
    bool hov = hovered(x, y, w, h);
    if (hov && (in_->mouseDelta().x != 0.0f || in_->mouseDelta().y != 0.0f)) focus_ = id;
    bool focused = focus_ == id;
    float old = value;

    // Nen
    r_->rect(x, y, w, h, focused ? glm::vec4(0.25f, 0.14f, 0.06f, 0.92f) : glm::vec4(0.08f, 0.09f, 0.12f, 0.82f));
    r_->rectOutline(x, y, w, h, 2.0f * scale_, focused ? glm::vec4(1, 0.7f, 0.3f, 1) : glm::vec4(0.7f, 0.72f, 0.8f, 0.7f));

    // Thanh keo
    float pad = 18.0f * scale_;
    float tx = x + pad, tw = w - 2 * pad;
    float ty = y + h * 0.66f;
    if (hov && in_->mousePressed(GLFW_MOUSE_BUTTON_LEFT)) dragging_ = id;
    if (dragging_ == id) {
        float t = std::clamp((in_->mousePos().x - tx) / tw, 0.0f, 1.0f);
        value = minV + t * (maxV - minV);
    }
    if (focused) {
        if (in_->pressed(GLFW_KEY_LEFT) || in_->pressed(GLFW_KEY_A)) value -= step;
        if (in_->pressed(GLFW_KEY_RIGHT) || in_->pressed(GLFW_KEY_D)) value += step;
    }
    value = std::clamp(value, minV, maxV);

    float frac = (value - minV) / (maxV - minV);
    r_->rect(tx, ty - 3.0f * scale_, tw, 6.0f * scale_, {0.3f, 0.32f, 0.38f, 1});
    r_->rect(tx, ty - 3.0f * scale_, tw * frac, 6.0f * scale_, {0.95f, 0.5f, 0.15f, 1});
    r_->rect(tx + tw * frac - 5.0f * scale_, ty - 11.0f * scale_, 10.0f * scale_, 22.0f * scale_, {1, 1, 1, 1});

    char buf[32];
    std::snprintf(buf, sizeof(buf), "%s: %.2f", label.c_str(), value);
    r_->text(buf, cx, y + 9.0f * scale_, 3.0f * scale_, {0.92f, 0.94f, 1, 1}, TextAlign::Center);
    return std::fabs(value - old) > 1e-6f;
}
