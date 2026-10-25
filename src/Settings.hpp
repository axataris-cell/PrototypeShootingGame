#pragma once
#include <vector>

struct Settings {
    float sensitivity = 1.0f;   // nhan voi BASE_MOUSE_SENS
    bool  shaderOn = true;

    static constexpr float MIN_SENS = 0.1f;
    static constexpr float MAX_SENS = 3.0f;

    void load();
    void save() const;
};

struct ScoreEntry {
    int score = 0;
    int kills = 0;
};

class HighScores {
public:
    static constexpr int MAX_ENTRIES = 5;

    void load();
    void save() const;
    // Them diem; tra ve true neu dat top 1 (ky luc moi)
    bool add(int score, int kills);
    int best() const { return entries_.empty() ? 0 : entries_.front().score; }
    const std::vector<ScoreEntry>& entries() const { return entries_; }

private:
    std::vector<ScoreEntry> entries_;
};
