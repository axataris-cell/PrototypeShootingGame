#pragma once
#include "Input.hpp"
#include "Renderer2D.hpp"

#include <string>

// UI menu kieu immediate-mode: ho tro chuot (hover/click/keo slider) va ban phim (Len/Xuong/Enter/Trai/Phai)
class Ui {
public:
    void init(Renderer2D* r, Input* in) { r_ = r; in_ = in; }
    void begin(int fbW, int fbH);        // goi dau moi frame ve menu
    float scale() const { return scale_; }

    // Tra ve true khi duoc bam (chuot hoac Enter/Space khi dang focus)
    bool button(const std::string& label, float cx, float y, float w, float h);
    // Tra ve true khi gia tri thay doi
    bool slider(const std::string& label, float& value, float minV, float maxV, float step,
                float cx, float y, float w, float h);
    void resetFocus() { focus_ = 0; }

private:
    int nextId() { return counter_++; }
    bool hovered(float x, float y, float w, float h) const;

    Renderer2D* r_ = nullptr;
    Input* in_ = nullptr;
    float scale_ = 1.0f;
    int counter_ = 0;
    int lastCount_ = 0;
    int focus_ = 0;
    int dragging_ = -1;
};
