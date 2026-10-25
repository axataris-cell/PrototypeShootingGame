#pragma once
// SFX bang miniaudio. Moi am thanh duoc giai ma 1 lan va co nhieu "voice" de phat chong len nhau
// (vi du M4 ban lien thanh).

#include <memory>

enum class Sfx {
    M4Shot = 0,
    Mark23Shot,
    Reload,
    Count
};

class Audio {
public:
    Audio();
    ~Audio();

    bool init();                 // false neu khong co thiet bi am thanh (game van chay, im lang)
    void shutdown();
    void play(Sfx id);
    void stop(Sfx id);
    void setMasterVolume(float v);
    bool ready() const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
