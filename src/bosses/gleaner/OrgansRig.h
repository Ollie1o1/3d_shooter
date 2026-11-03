#pragma once
// =============================================================================
// OrgansRig.h — the Gleaner's organs drawn (Organs.h says what they are doing):
// the Crucible Heart hanging on its arteries over a floor it floods, the two
// Bellows Lungs breathing on the far wall, the Sorting Core's drum of hooks and
// bodies with its lens under it. Also the shafts of breath that open out of
// each room once its organ is dead.
// =============================================================================
#include "bosses/gleaner/Organs.h"
#include "bosses/gleaner/GleanerRig.h"
#include "world/LevelPlain.h"

namespace organrig {
using glm::vec3; using glm::mat4;
inline const vec3 IRON{0.12f, 0.09f, 0.08f}, FLESH{0.28f, 0.07f, 0.05f}, LEATHER{0.2f, 0.11f, 0.07f}, BONE{0.36f, 0.31f, 0.27f}, SOOT{0.04f, 0.03f, 0.03f};
inline const vec3 MELT{2.6f, 0.75f, 0.18f};

inline void bar(std::vector<BoxInstance>& out, vec3 a, vec3 b, float w, vec3 c, vec3 e = vec3{0.f}) {
    vec3 d = b - a;
    rig::push(out, gleaner::alongZ((a + b) * 0.5f, d) * rig::S(vec3{w, w, glm::length(d) + w * 0.5f}), c, e);
}

inline void drawHeart(std::vector<BoxInstance>& out, const CrucibleHeart& h, float ball, float time) {
    using namespace rig;
    if (h.state == CrucibleHeart::DORMANT) return;
    const bool dead = h.state == CrucibleHeart::DEAD;
    const float die = dead ? organ::ease(h.t / 2.f) : 0.f, beat = dead ? 0.f : h.pulse(), sw = h.swell();
    const vec3 white = vec3{1.4f} * h.flash;
    const vec3 c = h.pos - vec3{0.f, 6.f * die, 0.f};
    const float R = CrucibleHeart::R * (1.f + 0.06f * beat + 0.12f * sw) * (1.f - 0.25f * die);
    auto lump = [&](const mat4& m, vec3 col, vec3 e) { push(out, m, col, e + white * 0.3f); out.back().mesh = ball; };
    // Its arteries: up to the roof and down the walls, throbbing
    for (int k = 0; k < 6; ++k) {
        float a = k * 1.0472f + 0.3f;
        vec3 root = c + vec3{std::cos(a) * R * 0.7f, R * 0.5f, std::sin(a) * R * 0.7f};
        vec3 top = gut::O() + vec3{std::cos(a) * 14.f, gut::ceilY(0), std::sin(a) * 14.f - 2.f};
        bar(out, root, top, 1.1f + 0.2f * beat, FLESH * 0.8f, MELT * (0.15f + 0.4f * beat) * (1.f - die));
    }
    // The body: dark meat round a crucible of slag, its seams beating with it
    lump(T(c) * RY(time * 0.15f) * S(vec3{R * 2.f, R * 1.8f, R * 2.f}), FLESH, MELT * (0.08f + 0.35f * beat + 0.5f * sw) * (1.f - die));
    for (int k = 0; k < 8; ++k) {   // iron bands it has grown round
        float a = k * 0.785f;
        push(out, T(c + vec3{std::cos(a) * R * 0.95f, 0.f, std::sin(a) * R * 0.95f}) * RY(-a) * S(vec3{0.6f, R * 1.5f, 1.4f}), IRON, vec3{0.f});
    }
    lump(T(c - vec3{0.f, R * 0.75f, 0.f}) * S(vec3{R * 1.1f, R * 0.5f, R * 1.1f}), vec3{0.5f, 0.2f, 0.05f}, MELT * (0.6f + 1.2f * sw) * (1.f - die));   // its spout
    // The three valves: shut iron, or open and pouring white-hot
    for (int k = 0; k < 3; ++k) {
        vec3 v = h.valve(k);
        float open = h.valvesOpen() ? 1.f : 0.f;
        push(out, gleaner::alongZ(v, v - c) * S(vec3{1.8f, 1.8f, 1.2f}), IRON * 1.3f, MELT * (0.2f + 2.f * open));
        if (open > 0.f) bar(out, v, vec3{v.x, h.floorY, v.z}, 0.7f, vec3{0.6f, 0.3f, 0.1f}, MELT * 1.8f);
    }
    // The flood: slag over the floor (and the floor glowing as it swells: the tell)
    const glm::vec2 hf = gut::half(0);
    float lv = h.level();
    if (sw > 0.01f || lv > 0.01f)
        push(out, T(vec3{gut::O().x, h.floorY + 0.04f + lv * 0.5f, gut::O().z}) * S(vec3{hf.x * 2.f, 0.08f + lv, hf.y * 2.f}),
             vec3{0.4f, 0.12f, 0.04f}, MELT * (lv > 0.05f ? 1.2f + 0.2f * std::sin(time * 3.f) : 0.5f * sw * (0.6f + 0.4f * std::sin(time * 12.f))));
}

inline void drawLungs(std::vector<BoxInstance>& out, const BellowsLungs& L, float time) {
    using namespace rig;
    if (!L.woke) return;
    for (int k = 0; k < 2; ++k) {
        const auto& w = L.b[k];
        const vec3 white = vec3{1.4f} * w.flash;
        const float f = L.fill(k);
        const vec3 p = w.pos, fwd = L.blow;
        // The bellows: leather folds between iron boards, opening as it draws in
        const float depth = 3.f + 4.5f * f;
        for (int j = 0; j <= 5; ++j) {
            float u = (float)j / 5.f;
            vec3 q = p + fwd * (u * depth - depth * 0.5f);
            float wide = (j == 0 || j == 5) ? 1.f : 0.86f + 0.1f * (j % 2);
            push(out, T(q) * S(vec3{8.5f * wide, 9.f * wide, j == 0 || j == 5 ? 0.6f : 0.35f}), j == 0 || j == 5 ? IRON : LEATHER * (w.dead ? 0.6f : 1.f), white * 0.3f);
        }
        for (int j = 0; j < 5; ++j) {   // the folds' ribs, glowing as it holds its breath
            float u = (j + 0.5f) / 5.f;
            vec3 q = p + fwd * (u * depth - depth * 0.5f);
            push(out, T(q) * S(vec3{8.9f, 0.3f, 0.2f}), SOOT, MELT * (w.ph == BellowsLungs::HOLD && !w.dead ? 1.2f : 0.15f));
        }
        // the throat: a nozzle of iron, its valve open (white) while it draws in
        vec3 th = L.throat(k);
        push(out, gleaner::alongZ(th, fwd) * S(vec3{2.4f, 2.4f, 2.f}), IRON * 1.2f, MELT * (L.throatOpen(k) ? 1.8f + 0.4f * std::sin(time * 20.f) : 0.1f) + white * 0.3f);
        // the gust: streaks of air down the room
        if (L.gusting(k))
            for (int s = 0; s < 18; ++s) {
                float u = std::fmod(time * 1.6f + s * 0.137f, 1.f);
                vec3 q = th + fwd * (2.f + u * 40.f) + vec3{std::sin(s * 2.3f) * (2.f + u * 7.f), -1.5f + std::sin(s * 1.7f) * 2.f, 0.f};
                push(out, T(q) * S(vec3{0.08f, 0.08f, 2.5f}), vec3{0.f}, vec3{1.2f, 0.7f, 0.45f} * (1.f - u) * 0.6f);
            }
    }
}

inline void drawCore(std::vector<BoxInstance>& out, const SortingCore& c, float ball, float time) {
    using namespace rig;
    if (c.state == SortingCore::DORMANT) return;
    const bool dead = c.state == SortingCore::DEAD;
    const float die = dead ? organ::ease(c.t / 2.f) : 0.f;
    const vec3 white = vec3{1.4f} * c.flash, p = c.pos - vec3{0.f, 8.f * die, 0.f};
    const float R = SortingCore::R;
    // The axle from the roof, the hub
    bar(out, p, vec3{p.x, gut::ceilY(2), p.z}, 1.6f, IRON);
    push(out, T(p) * RY(c.spin) * S(vec3{4.f, 4.f, 4.f}), IRON * 1.2f, white * 0.3f);
    out.back().mesh = ball;
    // The drum: spokes out to a ring of hooks, each with a body on it
    for (int k = 0; k < 12; ++k) {
        float a = c.spin + k * 0.5236f;
        vec3 rim = p + vec3{std::cos(a) * R, 0.5f, std::sin(a) * R};
        bar(out, p, rim, 0.5f, IRON);
        bar(out, rim, rim - vec3{0.f, 2.f, 0.f}, 0.2f, SOOT);
        if (k % 3 != 0 || c.state != SortingCore::THROW) {   // the bodies it sorts (the ones it throws are gone)
            mat4 B = T(rim - vec3{0.f, 3.2f, 0.f}) * RY(-a + 0.3f * std::sin(time + k));
            push(out, B * S(vec3{0.7f, 1.3f, 0.45f}), BONE * 0.9f, vec3{0.f});
            push(out, B * T(vec3{0.f, 1.f, 0.f}) * S(vec3{0.45f}), BONE * 0.8f, vec3{0.f});
            for (float s : {-1.f, 1.f}) push(out, B * T(vec3{s * 0.2f, -1.2f, 0.f}) * S(vec3{0.2f, 1.2f, 0.22f}), BONE * 0.75f, vec3{0.f});
        }
    }
    // The lens under the hub: iron petals opening on a burning eye
    const float open = c.lensU();
    const vec3 l = c.lens() - vec3{0.f, 8.f * die, 0.f};
    push(out, T(l) * S(vec3{2.6f}), vec3{0.6f, 0.15f, 0.05f}, (MELT * (0.3f + 2.4f * open) + vec3{1.2f, 1.1f, 1.f} * open) * (1.f - die) + white * 0.4f);
    out.back().mesh = ball;
    for (int k = 0; k < 4; ++k) {
        float a = k * 1.5708f + 0.785f;
        vec3 d{std::cos(a), 0.f, std::sin(a)};
        push(out, T(l + d * (0.8f + 1.8f * open) - vec3{0.f, 0.4f, 0.f}) * RY(-a) * RZ(0.6f * open) * S(vec3{1.6f, 0.4f, 1.6f}), IRON * 1.3f, vec3{0.f});
    }
}

// A shaft of the furnace's breath, open: embers pouring up it
inline void drawShaft(std::vector<BoxInstance>& out, glm::vec3 base, float height, float time) {
    using namespace rig;
    for (int k = 0; k < 40; ++k) {
        float u = std::fmod(time * 0.35f + k * 0.0611f, 1.f), a = k * 2.399f + time, r = 0.5f + (gut::SHAFT_R - 0.6f) * std::fmod(k * 0.618f, 1.f);
        push(out, T(base + vec3{std::cos(a) * r, u * height, std::sin(a) * r}) * S(vec3{0.2f, 0.9f + 1.5f * u, 0.2f}), vec3{0.f}, MELT * (1.f - 0.6f * u));
    }
    push(out, T(base + vec3{0.f, 0.06f, 0.f}) * S(vec3{gut::SHAFT_R * 2.f, 0.1f, gut::SHAFT_R * 2.f}), vec3{0.f}, MELT * 0.9f);
}
}  // namespace organrig
