#pragma once
// =============================================================================
// SieveRig.h — THE SIEVE drawn (Sieve.h says what it is doing): four spindly
// stilt legs with knees high over a small iron hub, a slit of light for an eye,
// and slung under it on chains a mesh basket of slag with the white-hot heart
// in it. Also its rain (a marked circle, droplets) and the cracks of its stake.
// =============================================================================
#include "bosses/gleaner/Sieve.h"
#include "bosses/gleaner/GleanerRig.h"
#include <glm/gtc/matrix_transform.hpp>

namespace sieverig {
using glm::vec3; using glm::mat4;

inline void draw(std::vector<BoxInstance>& out, const Sieve& v, float ball) {
    using namespace rig;
    if (v.state == Sieve::DORMANT) return;
    const bool dead = v.state == Sieve::DEAD;
    const float t = v.clock, die = dead ? Sieve::ease(v.t / 2.2f) : 0.f;
    const vec3 iron{0.11f, 0.09f, 0.085f}, rust{0.26f, 0.12f, 0.06f}, bone{0.34f, 0.3f, 0.26f}, soot{0.05f, 0.04f, 0.035f};
    const vec3 melt = vec3{2.6f, 0.8f, 0.2f} * (1.f - die), white = vec3{1.5f} * v.flash;
    auto box = [&](const mat4& m, vec3 c, vec3 e = vec3{0.f}) { push(out, m, c, e + white * 0.25f); };
    auto lump = [&](const mat4& m, vec3 c, vec3 e = vec3{0.f}) { push(out, m, c, e + white * 0.25f); out.back().mesh = ball; };
    auto bar = [&](vec3 a, vec3 b, float w, vec3 c, vec3 e = vec3{0.f}) {
        vec3 d = b - a;
        box(gleaner::alongZ((a + b) * 0.5f, d) * S(vec3{w, w, glm::length(d) + w * 0.5f}), c, e);
    };
    // ---- the legs: thin iron, knees high over it, feet like spikes ----
    for (int i = 0; i < 4; ++i) {
        vec3 hp = v.hip(i), kn = v.knee(i), ft = v.footNow(i);
        const bool stake = i == v.stakeLeg && (v.state == Sieve::STAKE_RAISE || v.state == Sieve::STAKED);
        vec3 hot = stake ? melt * (v.state == Sieve::STAKE_RAISE ? 0.6f + 1.4f * Sieve::ease(v.t / v.raiseTime()) : 1.6f) : vec3{0.f};
        bar(hp, kn, 0.75f, iron);
        bar(glm::mix(hp, kn, 0.3f), glm::mix(hp, kn, 0.7f), 0.95f, rust);   // a sleeve of plate
        lump(T(kn) * S(vec3{1.5f}), bone * 0.8f);
        box(gleaner::alongZ(kn + vec3{0.f, 1.4f, 0.f}, vec3{0.f, 1.f, 0.1f}) * S(vec3{0.35f, 0.35f, 2.6f}), bone * 0.7f);   // a spur off the knee
        bar(kn, glm::mix(kn, ft, 0.88f), 0.55f, iron * 1.2f);
        bar(glm::mix(kn, ft, 0.88f), ft, 0.4f, bone * 0.7f, hot);             // the spike: white-hot when it stakes
    }
    // ---- the hub: an iron dome, a ring, a slit eye that follows you ----
    vec3 hub = v.hub() - vec3{0.f, 4.f * die, 0.f};
    mat4 H = T(hub) * RY(v.yaw);
    lump(H * S(vec3{4.6f, 3.2f, 4.6f}), iron);
    box(H * T(vec3{0.f, -0.6f, 0.f}) * S(vec3{5.2f, 0.6f, 5.2f}), rust);
    for (int k = 0; k < 4; ++k) box(H * RY(k * 1.5708f) * T(vec3{0.f, -0.7f, 2.4f}) * S(vec3{1.f, 0.5f, 0.4f}), soot, melt * 0.5f);   // vents round its rim
    box(H * T(vec3{0.f, 0.4f, 2.25f}) * S(vec3{1.8f, 0.3f, 0.2f}), vec3{0.f}, melt * 1.6f);   // the eye
    box(H * T(vec3{0.f, 1.9f, 0.f}) * S(vec3{0.6f, 1.6f, 0.6f}), bone * 0.7f);               // a hook on top

    // ---- the basket: tipped toward the stake (mouth to you) as it swings out; shaken; spilled ----
    const vec3 c = v.heart();
    const vec3 sd = v.swingDir();
    const float tilt = v.basketTilt() + 0.6f * die;
    mat4 Bk = T(c) * glm::rotate(mat4(1.f), tilt, vec3{sd.z, 0.f, -sd.x} + vec3{0.f, 0.f, 1e-4f});
    const float hh = (Sieve::BASKET_TOP - Sieve::BASKET_BOT) * 0.5f, R = Sieve::BASKET_R;
    // chains from the hub to its rim
    for (int k = 0; k < 4; ++k) {
        float a = k * 1.5708f + 0.785f;
        vec3 rim = vec3(Bk * glm::vec4(std::cos(a) * R * 0.9f, hh, std::sin(a) * R * 0.9f, 1.f));
        bar(hub + vec3{std::cos(a) * 1.4f, -1.f, std::sin(a) * 1.4f}, rim, 0.22f, soot);
    }
    const int BARS = 12;
    for (int k = 0; k < BARS; ++k) {   // the mesh: uprights, curving in toward the bottom
        float a = k * 6.2832f / BARS;
        vec3 top{std::cos(a) * R, hh, std::sin(a) * R}, mid{std::cos(a) * R * 1.05f, 0.f, std::sin(a) * R * 1.05f}, bot{std::cos(a) * R * 0.7f, -hh, std::sin(a) * R * 0.7f};
        vec3 wt = vec3(Bk * glm::vec4(top, 1.f)), wm = vec3(Bk * glm::vec4(mid, 1.f)), wb = vec3(Bk * glm::vec4(bot, 1.f));
        bar(wt, wm, 0.16f, iron * 1.3f); bar(wm, wb, 0.16f, iron * 1.3f);
    }
    for (int r = 0; r < 4; ++r) {   // and its hoops
        float y = -hh + (r + 0.5f) * hh * 0.5f, rr = R * (r < 2 ? glm::mix(0.75f, 1.02f, r / 2.f) : 1.03f - 0.03f * (r - 2));
        for (int k = 0; k < BARS; ++k) {
            float a0 = k * 6.2832f / BARS, a1 = (k + 1) * 6.2832f / BARS;
            bar(vec3(Bk * glm::vec4(std::cos(a0) * rr, y, std::sin(a0) * rr, 1.f)), vec3(Bk * glm::vec4(std::cos(a1) * rr, y, std::sin(a1) * rr, 1.f)), 0.13f, rust * 1.2f);
        }
    }
    for (int k = -1; k <= 1; ++k) {   // the grate under it
        bar(vec3(Bk * glm::vec4(k * 1.1f, -hh, -R * 0.7f, 1.f)), vec3(Bk * glm::vec4(k * 1.1f, -hh, R * 0.7f, 1.f)), 0.14f, iron);
        bar(vec3(Bk * glm::vec4(-R * 0.7f, -hh, k * 1.1f, 1.f)), vec3(Bk * glm::vec4(R * 0.7f, -hh, k * 1.1f, 1.f)), 0.14f, iron);
    }
    // what it has gathered: slag, glowing, rattling when it shakes
    const float shake = v.shake();
    for (int k = 0; k < 11; ++k) {
        float a = k * 2.399f, rr = 1.2f + 1.5f * std::fmod(k * 0.618f, 1.f), y = -hh + 0.9f + (k % 4) * 0.9f;
        vec3 jig = vec3{std::sin(t * 31.f + k), std::sin(t * 27.f + k * 2.f) * 0.6f, std::cos(t * 29.f + k)} * (0.25f * shake);
        lump(Bk * T(vec3{std::cos(a) * rr, y, std::sin(a) * rr} + jig) * RY(k) * S(vec3{0.9f + 0.4f * (k % 3)}), vec3{0.18f, 0.08f, 0.04f}, melt * (0.35f + 0.25f * (k % 2)));
    }
    // the heart: a dark crucible round a white-hot core, beating
    const float beat = 0.5f + 0.5f * std::pow(std::max(0.f, std::sin(t * (v.enraged() ? 7.f : 5.f))), 6.f);
    const float open = v.heartOpen() ? 1.f : 0.f;
    lump(T(c) * RY(t * 0.4f) * S(vec3{2.7f}), iron * 0.7f, melt * 0.2f);
    lump(T(c + vec3{0.f, 0.3f, 0.f}) * RY(-t) * S(vec3{1.9f + 0.25f * beat}), vec3{0.8f, 0.5f, 0.3f},
         (vec3{3.f, 1.6f, 0.7f} * (0.8f + 0.6f * beat) + vec3{1.2f} * open) * (1.f - die) + white);

    // ---- its rain: the marked circle while it aims, droplets through the mesh while it pours ----
    if (v.state == Sieve::SHAKE_AIM || v.rainLeft > 0.f) {
        const float aim = v.state == Sieve::SHAKE_AIM ? Sieve::ease(v.t / 1.3f) : 1.f;
        const float pulse = 0.6f + 0.4f * std::sin(t * (8.f + 10.f * aim));
        const float r = Sieve::RAIN_R;
        for (int k = 0; k < 24; ++k) {
            float a = k * 0.2618f;
            push(out, T(v.rainAt + vec3{std::cos(a) * r, 0.1f, std::sin(a) * r}) * RY(-a) * S(vec3{0.3f, 0.1f, r * 0.27f}),
                 vec3{0.5f, 0.15f, 0.05f}, vec3{2.6f, 0.6f, 0.12f} * pulse * (0.4f + 0.6f * aim));
        }
        if (v.rainLeft > 0.f) {
            const vec3 from = c - vec3{0.f, hh, 0.f};
            for (int k = 0; k < 46; ++k) {
                float u = std::fmod(t * 1.7f + k * 0.618f, 1.f);
                float a = k * 2.399f + std::floor(t * 1.7f + k * 0.618f) * 1.3f, rr = r * std::sqrt(std::fmod(k * 0.377f + std::floor(t * 1.7f + k * 0.618f) * 0.29f, 1.f));
                vec3 land = v.rainAt + vec3{std::cos(a) * rr, 0.f, std::sin(a) * rr};
                vec3 p = glm::mix(from + (land - v.rainAt) * 0.25f, land, u * u);
                push(out, T(p) * S(vec3{0.28f, 0.9f, 0.28f}), vec3{0.6f, 0.3f, 0.1f}, vec3{2.8f, 1.f, 0.25f});
                if (u > 0.85f) push(out, T(land + vec3{0.f, 0.08f, 0.f}) * S(vec3{0.9f, 0.08f, 0.9f}), vec3{0.3f, 0.1f, 0.05f}, vec3{2.2f, 0.6f, 0.12f});
            }
            for (int k = 0; k < 10; ++k) {   // the plate under it, smoking and glowing in splashes
                float a = k * 0.628f + 0.3f, rr = r * (0.3f + 0.6f * std::fmod(k * 0.53f, 1.f));
                push(out, T(v.rainAt + vec3{std::cos(a) * rr, 0.06f, std::sin(a) * rr}) * RY(a) * S(vec3{1.6f, 0.06f, 1.1f}), vec3{0.3f, 0.1f, 0.05f}, vec3{1.6f, 0.4f, 0.08f});
            }
        }
    }
    // ---- the cracks from its stake: glowing lines running out, then bursting ----
    if (v.crackT >= 0.f) {
        const float glow = v.crackGlow(), len = v.crackLen();
        const bool burst = v.bursting();
        for (const auto& d : v.crackDir) {
            float yaw = std::atan2(d.x, d.z);
            int n = std::max(1, (int)(len / 2.f));
            for (int i = 0; i < n; ++i) {
                float u = (i + 0.5f) * len / n;
                vec3 p = v.crackFrom + d * u + vec3{0.f, 0.06f, 0.f};
                float jag = 0.35f * std::sin(i * 2.7f + yaw * 3.f);
                push(out, T(p + vec3{d.z, 0.f, -d.x} * jag) * RY(yaw + jag * 0.6f) * S(vec3{0.5f, 0.07f, 2.2f}), soot, vec3{2.6f, 0.7f, 0.15f} * glow * (burst ? 1.6f : 1.f));
                if (burst && i % 2 == 0) {
                    float h = 2.2f + 1.4f * std::sin(i * 1.9f + v.crackT * 30.f);
                    push(out, T(p + vec3{0.f, h * 0.5f, 0.f}) * RY(i) * S(vec3{0.9f, h, 0.9f}), vec3{0.6f, 0.25f, 0.08f}, vec3{3.f, 1.f, 0.25f});
                }
            }
        }
        push(out, T(v.crackFrom + vec3{0.f, 0.08f, 0.f}) * S(vec3{2.6f, 0.12f, 2.6f}), soot, vec3{2.8f, 0.9f, 0.2f} * glow);
    }
}
}  // namespace sieverig
