#pragma once
// =============================================================================
// MixTable.h — the mix in one place. Every sound belongs to a class with a
// target level (its loudest 50 ms, RMS, dBFS); a sound file's trim brings it
// there, and the built voices (VoiceSynth.h) are made at it. Call sites keep
// their volume numbers only as context weights: 128 means "this sound at its
// class's level".
// =============================================================================
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

enum class MixClass : uint8_t { GUN, FOLEY, PLAYER, ACTION, WORLD, UI, TELL, CHATTER, COUNT };

// Loudest first: guns and the big hits, then tells, the player, UI and the
// world, the mechanical foley, and enemy chatter well below
inline float mixTargetDb(MixClass c) {
    static const float T[(int)MixClass::COUNT] = {-11.f, -18.f, -12.f, -10.f, -13.f, -12.f, -14.f, -22.f};
    return T[(int)c];
}

// The loudest 50 ms of a sound (windows a quarter apart), as RMS in dBFS;
// -120 for silence
inline float shortTermDb(const float* x, size_t n, float rate) {
    if (!x || n == 0) return -120.f;
    const size_t win = std::max<size_t>(1, (size_t)(rate * 0.05f)), hop = std::max<size_t>(1, win / 4);
    double best = 0.0;
    for (size_t s = 0;; s += hop) {
        const size_t e = std::min(n, s + win);
        double sum = 0.0;
        for (size_t i = s; i < e; ++i) sum += (double)x[i] * x[i];
        best = std::max(best, sum / (double)(e - s));
        if (e >= n) break;
    }
    return best > 1e-12 ? 10.f * (float)std::log10(best) : -120.f;
}

// Every sound file the game loads: its class and the trim (dB) that brings
// the file to the class's level. Measured with --mixreport; the test holds
// each within 2 dB. (The built voices are made at their level: no trim.)
struct MixEntry { const char* name; MixClass cls; float trimDb; };

inline const std::vector<MixEntry>& mixTable() {
    using C = MixClass;
    static const std::vector<MixEntry> T = {
        {"revolver", C::GUN, 0.2f}, {"shotgun", C::GUN, -0.2f}, {"kar", C::GUN, 0.f}, {"longshot", C::GUN, 0.f},
        {"bolt", C::FOLEY, -1.5f}, {"cell", C::FOLEY, 1.5f}, {"cyl_close", C::FOLEY, -1.6f}, {"cyl_open", C::FOLEY, 1.5f},
        {"dry", C::FOLEY, 2.4f}, {"eject", C::FOLEY, 0.7f}, {"pump", C::FOLEY, -0.6f}, {"reload", C::FOLEY, -0.8f},
        {"shell_in", C::FOLEY, -0.3f}, {"scope", C::FOLEY, 0.8f}, {"telegraph", C::FOLEY, 5.f}, {"hit", C::FOLEY, 2.5f},
        {"wade", C::FOLEY, -0.7f}, {"barrier", C::FOLEY, 0.6f},
        {"jump", C::PLAYER, -4.1f}, {"land", C::PLAYER, 1.3f}, {"dash", C::PLAYER, -2.6f}, {"grapple_fire", C::PLAYER, -0.6f},
        {"punch", C::PLAYER, 1.7f}, {"step1", C::PLAYER, 1.9f}, {"step2", C::PLAYER, 2.5f}, {"step3", C::PLAYER, 2.5f},
        {"step4", C::PLAYER, 2.5f}, {"skim", C::PLAYER, -3.2f}, {"boost", C::PLAYER, -1.9f}, {"player_hit", C::PLAYER, -2.f},
        {"parry", C::PLAYER, 3.5f}, {"switch_up0", C::PLAYER, 1.3f}, {"switch_up1", C::PLAYER, 0.9f},
        {"switch_up2", C::PLAYER, 0.6f}, {"switch_up3", C::PLAYER, 0.2f},
        {"explosion", C::ACTION, -1.2f}, {"slam", C::ACTION, -1.8f}, {"clank", C::ACTION, 2.f},
        {"door", C::WORLD, -1.5f}, {"door_close", C::WORLD, 1.6f}, {"split", C::WORLD, 0.7f}, {"pickup", C::WORLD, -2.2f},
        {"potion", C::WORLD, 0.5f},
        {"wave", C::UI, -4.2f}, {"levelup", C::UI, -0.1f}, {"upgrade", C::UI, 0.1f},
    };
    return T;
}
inline const MixEntry* mixEntry(const std::string& name) {
    static const std::unordered_map<std::string, const MixEntry*> M = [] {
        std::unordered_map<std::string, const MixEntry*> m;
        for (const auto& e : mixTable()) m[e.name] = &e;
        return m;
    }();
    auto it = M.find(name);
    return it == M.end() ? nullptr : it->second;
}
// The linear gain a sound plays at for volume 128
inline float mixGain(const std::string& name) {
    const MixEntry* e = mixEntry(name);
    return e ? std::pow(10.f, e->trimDb / 20.f) : 1.f;
}
