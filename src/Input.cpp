#include "Input.hpp"

void Input::scrollCallback(GLFWwindow* w, double, double dy) {
    if (auto* in = static_cast<Input*>(glfwGetWindowUserPointer(w))) in->scrollAccum_ += static_cast<float>(dy);
}

void Input::attach(GLFWwindow* window) {
    window_ = window;
    glfwSetScrollCallback(window, &Input::scrollCallback);
}

void Input::update() {
    prevKey_ = key_;
    prevMouse_ = mouse_;
    for (int k = GLFW_KEY_SPACE; k <= GLFW_KEY_LAST; ++k) key_[k] = glfwGetKey(window_, k) == GLFW_PRESS;
    for (int b = 0; b <= GLFW_MOUSE_BUTTON_LAST; ++b) mouse_[b] = glfwGetMouseButton(window_, b) == GLFW_PRESS;

    double mx, my;
    glfwGetCursorPos(window_, &mx, &my);
    int ww, wh, fw, fh;
    glfwGetWindowSize(window_, &ww, &wh);
    glfwGetFramebufferSize(window_, &fw, &fh);
    float sx = ww > 0 ? static_cast<float>(fw) / ww : 1.0f;
    float sy = wh > 0 ? static_cast<float>(fh) / wh : 1.0f;

    glm::vec2 cur(static_cast<float>(mx), static_cast<float>(my));   // pixel cua so (de nhay chuot khong phu thuoc DPI)
    if (firstMouse_) { lastPos_ = cur; firstMouse_ = false; }
    mouseDelta_ = cur - lastPos_;
    lastPos_ = cur;
    mousePos_ = glm::vec2(cur.x * sx, cur.y * sy);

    scroll_ = scrollAccum_;
    scrollAccum_ = 0.0f;
}
