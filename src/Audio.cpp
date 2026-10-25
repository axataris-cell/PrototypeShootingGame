#include "Audio.hpp"
#include "Utils.hpp"

#define MA_NO_ENCODING
#define MA_NO_GENERATION
#define MINIAUDIO_IMPLEMENTATION
#include <miniaudio.h>

#include <array>
#include <iostream>
#include <memory>
#include <vector>

namespace {
struct SfxDef {
    const char* file;
    int voices;
    float volume;
};

// Duong dan tinh tu assets/sounds
const SfxDef SFX_DEFS[static_cast<int>(Sfx::Count)] = {
    {"sounds/M4.mp3",             8, 0.55f},   // M4Shot
    {"sounds/Mark23.mp3",         4, 0.80f},   // Mark23Shot
    {"sounds/gun_reload.mp3",     1, 0.80f},   // Reload
};
} // namespace

struct Audio::Impl {
    struct Bank {
        std::unique_ptr<ma_sound> base;
        std::vector<std::unique_ptr<ma_sound>> voices;
        size_t next = 0;
        bool loaded = false;
    };

    ma_engine engine{};
    bool engineReady = false;
    std::array<Bank, static_cast<int>(Sfx::Count)> banks;
};

Audio::Audio() : impl_(new Impl) {}
Audio::~Audio() { shutdown(); }

bool Audio::ready() const { return impl_ && impl_->engineReady; }

bool Audio::init() {
    if (impl_->engineReady) return true;
    if (ma_engine_init(nullptr, &impl_->engine) != MA_SUCCESS) {
        std::cerr << "Khong khoi tao duoc audio engine - game se chay khong co am thanh\n";
        return false;
    }
    impl_->engineReady = true;

    for (int i = 0; i < static_cast<int>(Sfx::Count); ++i) {
        const SfxDef& def = SFX_DEFS[i];
        Impl::Bank& bank = impl_->banks[i];
        std::string path = assetPath(def.file);

        bank.base = std::make_unique<ma_sound>();
        // DECODE: giai ma het vao bo nho 1 lan; cac voice sau chi copy tham chieu
        if (ma_sound_init_from_file(&impl_->engine, path.c_str(), MA_SOUND_FLAG_DECODE, nullptr, nullptr,
                                    bank.base.get()) != MA_SUCCESS) {
            std::cerr << "Khong nap duoc am thanh: " << path << std::endl;
            bank.base.reset();
            continue;
        }
        ma_sound_set_volume(bank.base.get(), def.volume);
        for (int v = 0; v < def.voices; ++v) {
            auto voice = std::make_unique<ma_sound>();
            if (ma_sound_init_copy(&impl_->engine, bank.base.get(), 0, nullptr, voice.get()) == MA_SUCCESS) {
                ma_sound_set_volume(voice.get(), def.volume);
                bank.voices.push_back(std::move(voice));
            }
        }
        bank.loaded = !bank.voices.empty();
    }
    return true;
}

void Audio::shutdown() {
    if (!impl_ || !impl_->engineReady) return;
    for (auto& bank : impl_->banks) {
        for (auto& v : bank.voices) ma_sound_uninit(v.get());
        bank.voices.clear();
        if (bank.base) ma_sound_uninit(bank.base.get());
        bank.base.reset();
        bank.loaded = false;
    }
    ma_engine_uninit(&impl_->engine);
    impl_->engineReady = false;
}

void Audio::play(Sfx id) {
    if (!ready()) return;
    Impl::Bank& bank = impl_->banks[static_cast<int>(id)];
    if (!bank.loaded) return;

    // Chon voice dang nghi; neu tat ca ban thi cat voice cu nhat (xoay vong)
    ma_sound* chosen = nullptr;
    for (size_t i = 0; i < bank.voices.size(); ++i) {
        size_t idx = (bank.next + i) % bank.voices.size();
        if (!ma_sound_is_playing(bank.voices[idx].get())) { chosen = bank.voices[idx].get(); bank.next = idx + 1; break; }
    }
    if (!chosen) {
        chosen = bank.voices[bank.next % bank.voices.size()].get();
        bank.next++;
    }
    ma_sound_stop(chosen);
    ma_sound_seek_to_pcm_frame(chosen, 0);
    ma_sound_start(chosen);
}

void Audio::stop(Sfx id) {
    if (!ready()) return;
    Impl::Bank& bank = impl_->banks[static_cast<int>(id)];
    for (auto& v : bank.voices) {
        ma_sound_stop(v.get());
        ma_sound_seek_to_pcm_frame(v.get(), 0);
    }
}

void Audio::setMasterVolume(float v) {
    if (ready()) ma_engine_set_volume(&impl_->engine, v);
}
