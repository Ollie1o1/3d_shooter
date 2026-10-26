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
        {"revolver", C::GUN, -2.0f}, {"revolver_echo", C::GUN, 2.4f}, {"shotgun", C::GUN, -2.5f}, {"kar", C::GUN, 0.f}, {"longshot", C::GUN, 0.f},
        {"bolt", C::FOLEY, -6.0f}, {"cell", C::FOLEY, 7.5f}, {"cyl_close", C::FOLEY, 1.0f}, {"cyl_open", C::FOLEY, 8.8f},
        {"dry", C::FOLEY, 5.0f}, {"eject", C::FOLEY, 0.7f}, {"pump", C::FOLEY, -3.2f}, {"reload", C::FOLEY, 8.6f},
        {"shell_in", C::FOLEY, -0.3f}, {"scope", C::FOLEY, 0.8f}, {"telegraph", C::FOLEY, 5.f},
        {"wade", C::FOLEY, -0.7f}, {"barrier", C::FOLEY, 0.6f}, {"ammo_low", C::FOLEY, 0.2f},
        {"jump", C::PLAYER, -2.3f}, {"hit", C::PLAYER, 1.5f}, {"land", C::PLAYER, 1.3f}, {"dash", C::PLAYER, -2.6f}, {"grapple_fire", C::PLAYER, -0.6f},
        {"punch", C::PLAYER, 1.7f}, {"step1", C::PLAYER, 1.9f}, {"step2", C::PLAYER, 2.5f}, {"step3", C::PLAYER, 2.5f},
        {"step4", C::PLAYER, 2.5f}, {"jump_air", C::PLAYER, 0.4f}, {"land_heavy", C::PLAYER, -3.9f}, {"slide", C::PLAYER, 4.1f}, {"heartbeat", C::PLAYER, -3.9f}, {"hit_armor", C::PLAYER, 2.7f}, {"step_metal", C::PLAYER, -0.2f}, {"land_metal", C::PLAYER, 1.2f}, {"lastkill", C::ACTION, -3.5f}, {"hit_body", C::ACTION, 1.2f}, {"stun", C::ACTION, 3.4f}, {"finisher", C::ACTION, -2.9f}, {"overdrive_on", C::ACTION, -2.7f}, {"overdrive_off", C::UI, -3.8f}, {"finish_ready", C::TELL, -5.8f}, {"skim", C::PLAYER, -3.2f}, {"boost", C::PLAYER, -1.9f}, {"player_hit", C::PLAYER, -2.f},
        {"parry", C::PLAYER, 3.5f}, {"sword_draw", C::PLAYER, 1.f}, {"sword_swing", C::PLAYER, 0.f}, {"sword_hit", C::PLAYER, 2.f},
        {"sword_thrust", C::PLAYER, 4.9f}, {"sword_guard", C::PLAYER, 9.7f}, {"sword_block", C::PLAYER, 2.f}, {"sword_break", C::PLAYER, 0.8f}, {"switch_up0", C::PLAYER, 1.3f}, {"switch_up1", C::PLAYER, 0.9f},
        {"switch_up2", C::PLAYER, 0.6f}, {"switch_up3", C::PLAYER, 0.2f},
        // THE ABSOLUTE: wind-ups (tells), the cues (must be heard), the booms, his flight, the player's side
        {"ab_wind_jab", C::TELL, -1.4f}, {"ab_wind_cross", C::TELL, -1.7f}, {"ab_wind_hook", C::TELL, -0.7f}, {"ab_wind_spin", C::TELL, 1.6f},
        {"ab_wind_meteor", C::TELL, -2.8f}, {"ab_wind_blade", C::TELL, -6.1f}, {"ab_thrum", C::TELL, -7.4f}, {"ab_whistle", C::TELL, -4.7f},
        {"ab_ray_charge", C::TELL, -4.7f}, {"ab_rumble", C::TELL, -2.5f}, {"ab_zoop", C::TELL, -7.4f},
        {"ab_ting", C::ACTION, -5.9f}, {"ab_doom", C::ACTION, -6.2f}, {"ab_pulse", C::ACTION, -5.1f},
        {"ab_boom_jab", C::ACTION, -3.8f}, {"ab_boom_cross", C::ACTION, -4.4f}, {"ab_boom_hook", C::ACTION, -2.2f}, {"ab_boom_heavy", C::ACTION, -3.5f},
        {"ab_boom_kick", C::ACTION, -5.3f}, {"ab_boom_stone", C::ACTION, -4.9f}, {"ab_boom_clap", C::ACTION, 0.7f}, {"ab_boom_thunder", C::ACTION, 0.f},
        {"ab_ray_beam", C::ACTION, -1.9f}, {"ab_zray_beam", C::ACTION, -2.3f}, {"ab_takeoff", C::ACTION, 1.0f}, {"ab_shotdown", C::ACTION, 0.3f},
        {"ab_zap", C::ACTION, 1.5f}, {"ab_flight", C::WORLD, -2.0f},
        {"ab_clang", C::PLAYER, 0.0f}, {"ab_shatter", C::PLAYER, 0.5f}, {"ab_creak", C::PLAYER, 1.5f},
        {"ab_tsk", C::UI, 4.8f}, {"ab_wrong", C::UI, -6.8f},
        {"explosion", C::ACTION, -1.2f}, {"slam", C::ACTION, -1.8f}, {"clank", C::ACTION, 2.f},
        {"door", C::WORLD, -1.5f}, {"door_close", C::WORLD, 1.6f}, {"split", C::WORLD, 0.7f}, {"pickup", C::WORLD, -2.2f},
        {"potion", C::WORLD, 0.5f}, {"impact", C::WORLD, 3.2f}, {"casing", C::FOLEY, -4.6f}, {"whiz", C::ACTION, 2.8f},
        {"hit_head", C::UI, -1.0f}, {"kill_light", C::UI, 8.5f}, {"kill_heavy", C::UI, -3.7f}, {"kill_rifle", C::UI, 0.f}, {"kill_blade", C::UI, 0.f},   // kill confirms
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
