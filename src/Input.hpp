#pragma once
#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <array>

// Doc ban phim / chuot theo kieu "khung hinh": co canh nhan (pressed) va nha (released)
class Input {
public:
    void attach(GLFWwindow* window);
    void update();                         // goi 1 lan moi frame SAU glfwPollEvents
    void resetMouse() { firstMouse_ = true; }

    bool down(int key) const { return key_[key]; }
    bool pressed(int key) const { return key_[key] && !prevKey_[key]; }
    bool mouseDown(int button) const { return mouse_[button]; }
    bool mousePressed(int button) const { return mouse_[button] && !prevMouse_[button]; }

    glm::vec2 mouseDelta() const { return mouseDelta_; }
    glm::vec2 mousePos() const { return mousePos_; }   // toa do pixel framebuffer
    float scrollY() const { return scroll_; }          // tong cuon trong frame nay (+ = len)

private:
    static void scrollCallback(GLFWwindow* w, double, double dy);

    GLFWwindow* window_ = nullptr;
    std::array<bool, GLFW_KEY_LAST + 1> key_{}, prevKey_{};
    std::array<bool, GLFW_MOUSE_BUTTON_LAST + 1> mouse_{}, prevMouse_{};
    glm::vec2 mousePos_{0.0f}, lastPos_{0.0f}, mouseDelta_{0.0f};
    bool firstMouse_ = true;
    float scroll_ = 0.0f;
    float scrollAccum_ = 0.0f;
};
