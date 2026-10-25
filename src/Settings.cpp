#include "Settings.hpp"

#include <algorithm>
#include <fstream>

static const char* SETTINGS_FILE = "settings.cfg";
static const char* SCORES_FILE = "highscore.txt";

void Settings::load() {
    std::ifstream f(SETTINGS_FILE);
    if (!f) return;
    float s;
    int sh;
    if (f >> s >> sh) {
        sensitivity = std::clamp(s, MIN_SENS, MAX_SENS);
        shaderOn = sh != 0;
    }
}

void Settings::save() const {
    std::ofstream f(SETTINGS_FILE);
    if (f) f << sensitivity << " " << (shaderOn ? 1 : 0) << "\n";
}

void HighScores::load() {
    entries_.clear();
    std::ifstream f(SCORES_FILE);
    ScoreEntry e;
    while (f >> e.score >> e.kills) entries_.push_back(e);
    std::sort(entries_.begin(), entries_.end(), [](const ScoreEntry& a, const ScoreEntry& b) { return a.score > b.score; });
    if (entries_.size() > MAX_ENTRIES) entries_.resize(MAX_ENTRIES);
}

void HighScores::save() const {
    std::ofstream f(SCORES_FILE);
    if (!f) return;
    for (const auto& e : entries_) f << e.score << " " << e.kills << "\n";
}

bool HighScores::add(int score, int kills) {
    if (score <= 0) return false;
    bool record = score > best();
    entries_.push_back({score, kills});
    std::stable_sort(entries_.begin(), entries_.end(), [](const ScoreEntry& a, const ScoreEntry& b) { return a.score > b.score; });
    if (entries_.size() > MAX_ENTRIES) entries_.resize(MAX_ENTRIES);
    save();
    return record;
}
