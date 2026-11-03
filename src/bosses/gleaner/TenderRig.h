#pragma once
// =============================================================================
// TenderRig.h — THE TENDER drawn (Tender.h says what it is doing): a squat
// furnace keeper, about 9 m, hunched under a crucible bigger than its body,
// a slag-crusted mask with a burning slit, two long tongs for arms. The
// crucible tips over its shoulder to pour; the tongs rise and glow before a
// sweep. Also the river of metal it pours.
// =============================================================================
#include "bosses/gleaner/Tender.h"
#include "bosses/gleaner/GleanerRig.h"

namespace tenderrig {
using glm::vec3; using glm::mat4;

inline void draw(std::vector<BoxInstance>& out, const Tender& k, float ball) {
    using namespace rig;
    if (k.state == Tender::DORMANT) return;
    const bool dead = k.state == Tender::DEAD;
    const float t = k.clock, die = dead ? Tender::ease(k.t / 1.6f) : 0.f;
    const vec3 iron{0.12f, 0.1f, 0.09f}, crust{0.2f, 0.12f, 0.08f}, bone{0.36f, 0.31f, 0.26f};
    const vec3 melt = vec3{2.6f, 0.75f, 0.18f} * (dead ? 1.f - die : 1.f), white = vec3{1.4f} * k.flash;
    auto box = [&](const mat4& m, vec3 c, vec3 e = vec3{0.f}) { push(out, m, c, e + white * 0.3f); };
    auto lump = [&](const mat4& m, vec3 c, vec3 e = vec3{0.f}) { push(out, m, c, e + white * 0.3f); out.back().mesh = ball; };
    // Its frame: on the deck, facing its way; it slumps forward as it dies
    mat4 R = T(k.base()) * RY(k.yaw) * RX(0.9f * die) * T(vec3{0.f, -2.f * die, 0.f});
    const float walk = std::sin(k.walkPhase * 6.2832f), bob = std::abs(walk) * 0.25f;
    const float stag = k.staggered() ? std::sin(k.t * 14.f) * 0.12f : 0.f;
    // Legs: thick, short, stamping
    for (float s : {-1.f, 1.f}) {
        float sw = walk * s * 0.35f;
        mat4 hip = R * T(vec3{s * 1.7f, 3.f, 0.f}) * RX(sw);
        box(hip * T(vec3{0.f, -0.9f, 0.f}) * S(vec3{1.5f, 2.f, 1.6f}), iron);
        box(hip * T(vec3{0.f, -2.2f, 0.25f}) * S(vec3{1.3f, 1.4f, 1.4f}), iron * 0.9f);
        box(hip * T(vec3{0.f, -2.85f, 0.6f}) * S(vec3{1.6f, 0.5f, 2.4f}), crust);   // a foot like an anvil
    }
    // The body: hunched forward, belly glowing through its plates
    mat4 B = R * T(vec3{0.f, 3.4f + bob, 0.f}) * RX(0.32f + stag) * RZ(stag * 0.5f);
    box(B * T(vec3{0.f, 1.5f, 0.f}) * S(vec3{4.4f, 3.4f, 3.6f}), iron);
    box(B * T(vec3{0.f, 1.1f, 1.82f}) * S(vec3{3.2f, 1.6f, 0.2f}), crust, melt * 0.5f);
    for (int p = 0; p < 3; ++p) box(B * T(vec3{0.f, 0.4f + p * 1.05f, 1.9f}) * RX(-0.1f) * S(vec3{4.f, 0.5f, 0.35f}), crust * 1.2f);
    // The head: a slag mask jutting from the front of its shoulders, a burning slit
    mat4 H = B * T(vec3{0.f, 3.4f, 2.f}) * RX(-0.3f);
    box(H * S(vec3{2.f, 1.8f, 1.9f}), crust);
    box(H * T(vec3{0.f, 0.15f, 0.97f}) * S(vec3{1.4f, 0.22f, 0.1f}), vec3{0.f}, melt * 1.4f);
    for (float s : {-1.f, 1.f}) box(H * T(vec3{s * 1.1f, 0.6f, 0.2f}) * RZ(s * 0.5f) * S(vec3{0.3f, 1.2f, 0.3f}), bone);   // horns of slag
    // The crucible on its back, tipping over its shoulder to pour
    const float tip = k.tip();
    mat4 C = B * T(vec3{0.f, 3.6f, -1.2f}) * RX(-2.1f * tip) * T(vec3{0.f, 2.4f, -0.8f});
    lump(C * S(vec3{5.6f, 4.8f, 5.6f}), iron * 0.9f, melt * 0.15f);
    for (int r = 0; r < 2; ++r) box(C * T(vec3{0.f, -1.f + r * 1.8f, 0.f}) * S(vec3{6.f, 0.5f, 6.f}), crust);   // its bands
    lump(C * T(vec3{0.f, 2.1f, 0.f}) * S(vec3{4.2f, 0.9f, 4.2f}), vec3{0.5f, 0.2f, 0.05f}, melt * (1.3f + 0.3f * std::sin(t * 3.f)));   // molten metal
    // the lip: where it pours, glowing hotter as it opens to you (its weak point)
    box(C * T(vec3{0.f, 2.6f, 2.9f}) * S(vec3{1.8f, 0.6f, 1.f}), crust, melt * (0.4f + 1.6f * (k.lipOpen() ? 1.f : 0.f)));
    // The pour: a falling column of metal from the lip to the deck
    if (k.state == Tender::POUR) {
        vec3 lip = k.lipPoint(), ground = k.riverFrom;
        vec3 mid = (lip + ground) * 0.5f;
        float len = glm::length(lip - ground);
        box(gleaner::alongZ(mid, ground - lip) * S(vec3{0.9f, 0.9f, len}), vec3{0.6f, 0.3f, 0.1f}, melt * 1.8f);
    }
    // The tongs: long arms, raised and glowing for the sweep, swinging through it
    const float up = k.tongsUp(), sweep = k.sweepU();
    const float reach = k.grabReach(), gopen = k.grabOpen();
    const float lift = k.state == Tender::HOLD ? Tender::ease(k.t / 1.2f) : 0.f;
    const vec3 grabWhite = vec3{3.f, 2.8f, 2.4f} * (0.6f + 0.4f * std::sin(t * 30.f));   // the grab's tell: white (punch it)
    for (float s : {-1.f, 1.f}) {
        float swing = sweep >= 0.f ? s * (1.4f - 2.8f * Tender::ease(sweep)) * (k.sweeps % 2 ? -1.f : 1.f) : 0.f;
        swing += -s * 0.35f * reach;   // (the grab: both tongs forward, together)
        float raise = up * 1.3f + reach * 1.45f + lift * 0.5f, hang = 0.25f * walk * s;
        mat4 sh = B * T(vec3{s * 2.5f, 2.8f, 0.4f}) * RY(swing) * RX(-raise + hang - (sweep >= 0.f ? 1.25f : 0.f)) * RZ(s * 0.18f);
        box(sh * T(vec3{0.f, -2.f, 0.f}) * S(vec3{0.9f, 4.f, 0.9f}), iron);
        mat4 el = sh * T(vec3{0.f, -4.f, 0.f}) * RX(0.5f - 0.4f * up - 0.4f * reach);
        box(el * T(vec3{0.f, -3.6f, 0.f}) * S(vec3{0.55f, 7.2f, 0.55f}), iron * 1.2f, melt * 0.6f * up + grabWhite * 0.25f * gopen);
        for (float j : {-1.f, 1.f})   // the jaws of the tongs
            box(el * T(vec3{j * 0.35f, -7.6f, 0.f}) * RZ(j * (0.3f + 0.3f * up + 0.9f * gopen)) * S(vec3{0.25f, 1.6f, 0.4f}), bone, melt * 1.2f * up + grabWhite * gopen);
    }
    // Its own metal spilled down over it: a burning pool round its feet, smoke off its back
    if (k.spilled > 0.f && !dead) {
        float f = std::min(1.f, k.spilled / 0.8f);
        for (int i = 0; i < 9; ++i) {
            float a = i * 0.698f + 0.3f, r = 2.2f + 1.6f * std::fmod(i * 0.618f, 1.f);
            push(out, T(k.base() + vec3{std::cos(a) * r, 0.1f, std::sin(a) * r}) * RY(a) * S(vec3{2.6f, 0.2f, 1.8f}), vec3{0.5f, 0.2f, 0.05f}, vec3{2.8f, 0.8f, 0.2f} * f);
        }
        for (int i = 0; i < 6; ++i) {   // streams of it down its back
            float u = std::fmod(t * 2.f + i * 0.17f, 1.f);
            vec3 p = k.base() + vec3{std::sin(i * 1.7f) * 1.8f, 11.f - 10.f * u, std::cos(i * 2.3f) * 1.4f} - k.fwd() * 1.2f;
            push(out, T(p) * S(vec3{0.5f, 1.4f, 0.5f}), vec3{0.f}, vec3{3.f, 1.f, 0.25f} * f);
        }
    }
}

// The river of metal it poured, cooling at its edges
inline void drawRiver(std::vector<BoxInstance>& out, const Tender& k, float ball) {
    using namespace rig;
    if (k.riverLeft <= 0.f || k.riverLen <= 0.f) return;
    float fade = std::min(1.f, k.riverLeft / 1.5f);
    float yaw = std::atan2(k.riverDir.x, k.riverDir.z);
    int n = std::max(1, (int)(k.riverLen / 2.5f));
    for (int i = 0; i < n; ++i) {
        float u = (i + 0.5f) * k.riverLen / n;
        vec3 p = k.riverFrom + k.riverDir * u + vec3{0.f, 0.08f, 0.f};
        float w = Tender::RIVER_HALF * 2.f * (0.85f + 0.15f * std::sin(i * 1.7f + k.clock * 2.f));
        push(out, T(p) * RY(yaw + 0.2f * std::sin(i * 2.3f)) * S(vec3{w, 0.25f, 3.2f}), vec3{0.5f, 0.2f, 0.05f},
             vec3{2.4f, 0.65f, 0.14f} * fade * (0.8f + 0.2f * std::sin(k.clock * 4.f + i)));
        if (i % 2 == 0) { push(out, T(p + vec3{0.f, 0.15f, 0.f}) * S(vec3{w * 0.5f, 0.4f, 1.6f}), vec3{0.2f, 0.08f, 0.04f}, vec3{1.f, 0.25f, 0.05f} * fade); out.back().mesh = ball; }
    }
}
}  // namespace tenderrig
