#pragma once
// =============================================================================
// WorldEmitters.h — the places themselves, heard from where they are. Lava
// that rumbles and spits, the Core's reactor humming, fans turning in their
// housings, water lapping at a wall, an anchor's mechanism ticking: each a point
// in the world you can walk toward and away from, on top of the room's bed
// (RoomTone.h), which has no direction.
//
// The sounds are built in code at load (no files, nothing added to the
// download). An emitter doesn't hold a loop: it lets go of short overlapping
// grains (two to three seconds, soft edges) a little apart, each placed where
// the emitter is, so it is always a little different and costs a voice only
// while you are near. The director picks who sounds: the nearest three within
// 30 m of the listener, three grains a second at most.
//
// No GL and no audio device: build() returns buffers, the director returns
// cues, the game plays them (GameplayState::updateWorldEmitters).
// =============================================================================
#include "audio/MixTable.h"
#include <glm/glm.hpp>
#include <cstdint>
#include <string>
#include <vector>

namespace worldsound {

enum class Kind : uint8_t { LAVA, HUM, FAN, WATER, COUNT };
constexpr float RATE = 22050.f;
constexpr int VARIANTS = 3;
inline const char* nameOf(Kind k) {
    static const char* N[] = {"em_lava", "em_hum", "em_fan", "em_water"};
    return N[(int)k];
}
// Each kind's level (its loudest 50 ms, RMS dBFS): under the enemies' chatter. They are the room, not the fight.
inline float levelDb(Kind k) {
    static const float L[] = {-25.f, -29.f, -28.f, -28.f};
    return L[(int)k];
}
// A grain's length, and how long an emitter waits before its next (lo..hi seconds)
inline float grainSeconds(Kind k) { static const float S[] = {2.4f, 3.4f, 2.6f, 2.2f}; return S[(int)k]; }
inline void gapOf(Kind k, float& lo, float& hi) {
    static const float G[][2] = {{1.3f, 2.4f}, {2.2f, 2.9f}, {1.7f, 2.3f}, {1.4f, 2.8f}};
    lo = G[(int)k][0]; hi = G[(int)k][1];
}

struct Rng {
    uint32_t s;
    explicit Rng(uint32_t seed) : s(seed ? seed : 1u) {}
    uint32_t next() { s ^= s << 13; s ^= s >> 17; s ^= s << 5; return s; }
    float unit() { return (float)(next() & 0xffffffu) / 16777215.f; }        // 0..1
    float bi() { return unit() * 2.f - 1.f; }                                 // -1..1
    float range(float lo, float hi) { return lo + (hi - lo) * unit(); }
};

// One grain, at RATE. Deterministic for a kind and variant.
inline std::vector<float> build(Kind kind, int variant) {
    const int n = (int)(grainSeconds(kind) * RATE);
    std::vector<float> x((size_t)n, 0.f);
    Rng r(0x51ed270bu ^ ((uint32_t)kind * 2654435761u) ^ ((uint32_t)variant * 40503u));
    const float TWO_PI = 6.2831853f;
    auto lowpass = [&](std::vector<float>& v, float hz) {
        const float a = 1.f - std::exp(-TWO_PI * hz / RATE); float y = 0.f;
        for (float& s : v) { y += (s - y) * a; s = y; }
    };
    auto highpass = [&](std::vector<float>& v, float hz) {
        const float a = 1.f - std::exp(-TWO_PI * hz / RATE); float y = 0.f;
        for (float& s : v) { y += (s - y) * a; s -= y; }
    };
    auto noise = [&](int len) { std::vector<float> v((size_t)len); for (float& s : v) s = r.bi(); return v; };
    switch (kind) {
    case Kind::LAVA: {
        // A slow heavy rumble, a breath of hiss over it, and a few thick bubbles breaking
        std::vector<float> rumble = noise(n); lowpass(rumble, 140.f); lowpass(rumble, 140.f);
        std::vector<float> hiss = noise(n); highpass(hiss, 2600.f); lowpass(hiss, 6000.f);
        const float ph = r.range(0.f, TWO_PI);
        for (int i = 0; i < n; ++i) {
            const float t = (float)i / RATE;
            x[i] = rumble[i] * 6.f * (0.75f + 0.25f * std::sin(t * 2.1f + ph)) + hiss[i] * 0.05f * (0.5f + 0.5f * std::sin(t * 3.3f + ph * 2.f));
        }
        const int bubbles = 3 + (int)(r.next() % 3u);
        for (int b = 0; b < bubbles; ++b) {
            const int at = (int)(r.range(0.25f, grainSeconds(kind) - 0.5f) * RATE), len = (int)(r.range(0.07f, 0.13f) * RATE);
            const float f0 = r.range(95.f, 150.f), f1 = f0 * r.range(1.5f, 2.1f), amp = r.range(0.25f, 0.5f);
            float phase = 0.f;
            for (int i = 0; i < len && at + i < n; ++i) {
                const float u = (float)i / (float)len;
                phase += TWO_PI * (f0 + (f1 - f0) * u * u) / RATE;   // (a bubble's pitch rises as it thins and breaks)
                x[at + i] += std::sin(phase) * amp * std::sin(3.14159265f * u) * (1.f - u);
            }
        }
        break;
    }
    case Kind::HUM: {
        // Mains through heavy plant: a fundamental and its odd harmonics, two of them a hair apart so it beats
        const float f = variant == 0 ? 55.f : variant == 1 ? 58.27f : 49.f, beat = r.range(0.5f, 0.9f), ph = r.range(0.f, TWO_PI);
        for (int i = 0; i < n; ++i) {
            const float t = (float)i / RATE;
            x[i] = std::sin(TWO_PI * f * t) * 0.5f + std::sin(TWO_PI * f * 2.f * t + ph) * 0.3f + std::sin(TWO_PI * (f * 2.f + beat) * t) * 0.3f
                 + std::sin(TWO_PI * f * 3.f * t) * 0.16f + std::sin(TWO_PI * f * 5.f * t + ph) * 0.06f + std::sin(TWO_PI * f * 7.f * t) * 0.03f;
        }
        std::vector<float> air = noise(n); lowpass(air, 900.f); highpass(air, 300.f);
        for (int i = 0; i < n; ++i) x[i] += air[i] * 0.12f;
        break;
    }
    case Kind::FAN: {
        // Air pushed by blades: band noise that swells as each blade passes, a low motor note under it
        std::vector<float> air = noise(n); highpass(air, 180.f); lowpass(air, 1100.f); lowpass(air, 1100.f);
        const float pass = variant == 0 ? 9.f : variant == 1 ? 11.5f : 7.5f, ph = r.range(0.f, TWO_PI);
        for (int i = 0; i < n; ++i) {
            const float t = (float)i / RATE;
            x[i] = air[i] * (0.62f + 0.38f * std::sin(TWO_PI * pass * t + ph)) + std::sin(TWO_PI * pass * 4.f * t) * 0.07f;
        }
        break;
    }
    case Kind::WATER: {
        // Small water against stone: restless band noise, and now and then a drop
        std::vector<float> wash = noise(n); highpass(wash, 350.f); lowpass(wash, 2400.f);
        std::vector<float> swell = noise(n); lowpass(swell, 2.5f); lowpass(swell, 2.5f);
        float peak = 1e-6f; for (float s : swell) peak = std::max(peak, std::fabs(s));
        for (int i = 0; i < n; ++i) x[i] = wash[i] * (0.45f + 0.55f * std::fabs(swell[i]) / peak);
        const int drops = 1 + (int)(r.next() % 3u);
        for (int d = 0; d < drops; ++d) {
            const int at = (int)(r.range(0.3f, grainSeconds(kind) - 0.4f) * RATE), len = (int)(0.035f * RATE);
            const float f0 = r.range(800.f, 1200.f); float phase = 0.f;
            for (int i = 0; i < len && at + i < n; ++i) {
                const float u = (float)i / (float)len;
                phase += TWO_PI * f0 * (1.f + 0.6f * u) / RATE;
                x[at + i] += std::sin(phase) * 0.35f * (1.f - u) * (1.f - u);
            }
        }
        break;
    }
    default: break;
    }
    // No DC, soft edges (grains overlap: an abrupt start would click), and at its level
    double mean = 0.; for (float s : x) mean += s; mean /= (double)n;
    const int edge = (int)(0.3f * RATE);
    for (int i = 0; i < n; ++i) {
        x[i] -= (float)mean;
        const float in = i < edge ? (float)i / (float)edge : 1.f, out = i > n - edge ? (float)(n - i) / (float)edge : 1.f;
        const float w = in * in * (3.f - 2.f * in) * out * out * (3.f - 2.f * out);
        x[i] *= w;
    }
    const float have = shortTermDb(x.data(), x.size(), RATE);
    const float gain = std::pow(10.f, (levelDb(kind) - have) / 20.f);
    for (float& s : x) s = glm::clamp(s * gain, -0.98f, 0.98f);
    return x;
}

struct Emitter { glm::vec3 pos{0.f}; Kind kind = Kind::HUM; float size = 1.f; float wait = 0.f; };   // size: 0.5 a small one .. 1.5 a big one (its level)
struct Cue { const char* name; glm::vec3 pos; float volume, pitch; Kind kind; };

struct Director {
    static constexpr float RANGE = 30.f;      // further than this an emitter is silent (and costs nothing)
    static constexpr int   NEAREST = 3;       // how many may sound at once
    static constexpr float MAX_PER_SECOND = 3.f;
    std::vector<Emitter> emitters;
    Rng rng{0x77a1c3u};
    float budget = 1.f;                       // grains allowed right now (refills at MAX_PER_SECOND)
    std::vector<Cue> cues;                    // this tick's (cleared by tick)

    void clear() { emitters.clear(); cues.clear(); budget = 1.f; }
    void add(glm::vec3 p, Kind k, float size = 1.f) {
        Emitter e; e.pos = p; e.kind = k; e.size = glm::clamp(size, 0.4f, 1.6f);
        e.wait = rng.range(0.f, 1.5f);        // (they don't all start together)
        emitters.push_back(e);
    }
    // level 0..1: how much of the world's sound the player has asked for (0: none)
    const std::vector<Cue>& tick(float dt, glm::vec3 listener, float level = 1.f) {
        cues.clear();
        budget = std::min(1.5f, budget + dt * MAX_PER_SECOND);
        if (level <= 0.f || emitters.empty()) return cues;
        // the nearest few in range
        int near[NEAREST]; float nearD[NEAREST]; int count = 0;
        for (int i = 0; i < (int)emitters.size(); ++i) {
            const float d = glm::length(emitters[i].pos - listener);
            if (d > RANGE) continue;
            int at = count < NEAREST ? count++ : -1;
            if (at < 0) { int worst = 0; for (int k = 1; k < NEAREST; ++k) if (nearD[k] > nearD[worst]) worst = k; if (d < nearD[worst]) at = worst; }
            if (at >= 0) { near[at] = i; nearD[at] = d; }
        }
        for (int k = 0; k < count; ++k) {
            Emitter& e = emitters[near[k]];
            e.wait -= dt;
            if (e.wait > 0.f || budget < 1.f) continue;
            budget -= 1.f;
            float lo, hi; gapOf(e.kind, lo, hi);
            e.wait = rng.range(lo, hi);
            cues.push_back({nameOf(e.kind), e.pos, glm::clamp(level * e.size, 0.f, 1.6f), rng.range(0.94f, 1.06f), e.kind});
        }
        return cues;
    }
};

}  // namespace worldsound
