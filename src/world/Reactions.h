#pragma once
// =============================================================================
// Reactions.h — the room answers what happens in it. A blast is a jolt: for
// most of a second the neon near it stutters (its light, on everything it
// lights), dust comes off the ceiling over it and lava close by throws a gout.
// The jolts live here; the game asks what each means (GameplayState::worldJolt
// spawns the dust and the gout, the renderer asks lightScale for each strip).
//
// With REDUCE FLASHING on, the neon dips once and comes back instead of
// stuttering. No GL, no audio, no randomness from rand(): the stutter is a
// hash of the clock, so it is the same at any frame rate.
// =============================================================================
#include "core/Collision.h"
#include <glm/glm.hpp>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

namespace react {

struct Jolt { glm::vec3 pos{0.f}; float power = 1.f, age = 0.f; };

constexpr float JOLT_LIFE = 0.85f;      // seconds a jolt is felt
constexpr int   MAX_JOLTS = 8;
constexpr float STUTTER_HZ = 28.f;      // how fast a shaken strip stutters
// How far a jolt of this power reaches (m)
inline float reachOf(float power) { return 7.f + 11.f * std::clamp(power, 0.f, 1.5f); }

inline float hash01(uint32_t a, uint32_t b) {
    uint32_t h = a * 0x9e3779b1u ^ (b + 0x7f4a7c15u) * 0x85ebca6bu;
    h ^= h >> 15; h *= 0x2c1b3c6du; h ^= h >> 12; h *= 0x297a2d39u; h ^= h >> 15;
    return (float)(h & 0xffffffu) / 16777215.f;
}

struct Field {
    std::vector<Jolt> jolts;

    void clear() { jolts.clear(); }
    void add(glm::vec3 at, float power) {
        if (power <= 0.f) return;
        if ((int)jolts.size() >= MAX_JOLTS) jolts.erase(jolts.begin());   // (the oldest: nearly spent)
        jolts.push_back({at, std::min(power, 1.5f), 0.f});
    }
    void tick(float dt) {
        for (auto& j : jolts) j.age += dt;
        jolts.erase(std::remove_if(jolts.begin(), jolts.end(), [](const Jolt& j) { return j.age >= JOLT_LIFE; }), jolts.end());
    }

    // How shaken a point is right now, 0..1: nearer and fresher is more
    float shakenAt(glm::vec3 p) const {
        float w = 0.f;
        for (const Jolt& j : jolts) {
            const float reach = reachOf(j.power), d = glm::length(p - j.pos);
            if (d >= reach) continue;
            const float fresh = 1.f - j.age / JOLT_LIFE;
            w = std::max(w, (1.f - d / reach) * fresh * fresh * std::min(1.f, j.power + 0.3f));
        }
        return std::min(w, 1.f);
    }

    // What to multiply a light's brightness by: 1 when nothing has shaken it. `index` is the
    // strip's own number (each stutters on its own pattern), `calm` is REDUCE FLASHING.
    float lightScale(const AABB& box, int index, float clock, bool calm) const {
        if (jolts.empty()) return 1.f;
        const float w = shakenAt(glm::clamp((box.min + box.max) * 0.5f, box.min, box.max));
        if (w <= 0.f) return 1.f;
        if (calm) return 1.f - 0.3f * w;
        const float step = hash01((uint32_t)index, (uint32_t)std::floor(clock * STUTTER_HZ));
        const float lit = step < 0.38f ? 0.12f : step < 0.7f ? 0.7f : 1.15f;   // out, low, or a little over
        return 1.f + (lit - 1.f) * std::min(1.f, w * 1.6f);
    }
};

}  // namespace react
