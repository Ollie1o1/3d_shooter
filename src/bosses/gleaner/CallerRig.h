#pragma once
// =============================================================================
// CallerRig.h — THE CALLER drawn (Caller.h says what it is doing): a cracked
// iron bell the size of a house, flared at the mouth, a red seam of light down
// its crack, the lamp hanging inside where its clapper should be, and two
// ragged wings of hanging chain with rags of cloth between the fingers.
// =============================================================================
#include "bosses/gleaner/Caller.h"
#include "bosses/gleaner/GleanerRig.h"

namespace callerrig {
using glm::vec3; using glm::mat4;

inline void draw(std::vector<BoxInstance>& out, const Caller& c, float ball) {
    using namespace rig;
    if (c.state == Caller::DORMANT) return;
    const bool dead = c.state == Caller::DEAD;
    const float t = c.clock, die = dead ? Caller::ease(c.t / 1.5f) : 0.f;
    const vec3 iron{0.13f, 0.11f, 0.1f}, verd{0.12f, 0.16f, 0.13f}, rust{0.28f, 0.13f, 0.07f}, cloth{0.09f, 0.05f, 0.05f}, soot{0.04f, 0.035f, 0.03f};
    const float flare = c.lampFlare();
    const vec3 red = vec3{3.2f, 0.25f, 0.12f} * (1.f - die) + vec3{2.2f, 1.8f, 1.6f} * flare * (0.7f + 0.3f * std::sin(t * 40.f));
    const vec3 white = vec3{1.5f} * c.flash;
    auto box = [&](const mat4& m, vec3 col, vec3 e = vec3{0.f}) { push(out, m, col, e + white * 0.2f); };
    auto lump = [&](const mat4& m, vec3 col, vec3 e = vec3{0.f}) { push(out, m, col, e + white * 0.2f); out.back().mesh = ball; };
    // Its frame: +Y up through the bell (the mouth below), swinging as it tolls
    mat4 M = gleaner::alongZ(c.pos, c.axis) * RX(1.5708f) * RZ(0.35f * c.swing * std::sin(t * 4.2f)) * RX(0.06f * std::sin(t * 1.3f));
    const float H = Caller::BELL_H, R = Caller::BELL_R;
    // ---- the bell: rings of plate, flaring to the mouth; a crack down one side glowing from inside ----
    const int LV = 6, PAN = 14;
    for (int j = 0; j < LV; ++j) {
        float u = (j + 0.5f) / LV;
        float y = -H * 0.5f + u * H;
        float r = R * (0.55f + 0.45f * std::pow(1.f - u, 1.8f)) + (j == 0 ? 0.3f : 0.f);
        float r2 = R * (0.55f + 0.45f * std::pow(1.f - (u + 1.f / LV), 1.8f));
        float slope = std::atan2(r - r2, H / LV);
        for (int k = 0; k < PAN; ++k) {
            float a = k * 6.2832f / PAN;
            bool crack = k == 3 && j < 5;
            mat4 P = M * RY(-a) * T(vec3{r - 0.1f, y, 0.f}) * RZ(slope);
            box(P * S(vec3{0.45f, H / LV * 1.06f, 2.f * 3.1416f * r / PAN * 1.08f}), (j + k) % 3 == 0 ? verd : iron * (0.9f + 0.2f * ((k * 7 + j) % 3) / 2.f));
            if (crack) box(P * T(vec3{0.25f, 0.f, 0.f}) * RX(0.4f * std::sin(j * 2.f)) * S(vec3{0.1f, H / LV * 1.1f, 0.25f}), soot, red * 0.7f);
        }
    }
    for (int k = 0; k < PAN; ++k) {   // the lip: a heavy rim at the mouth
        float a = k * 6.2832f / PAN;
        box(M * RY(-a) * T(vec3{R + 0.35f, -H * 0.5f + 0.15f, 0.f}) * S(vec3{0.7f, 0.6f, 2.f * 3.1416f * R / PAN * 1.1f}), rust);
    }
    box(M * T(vec3{0.f, H * 0.5f + 0.1f, 0.f}) * S(vec3{R * 1.15f, 0.5f, R * 1.15f}), iron);   // its crown
    for (float s : {-1.f, 1.f}) box(M * T(vec3{s * 0.7f, H * 0.5f + 1.1f, 0.f}) * S(vec3{0.5f, 1.8f, 0.6f}), rust);   // the hanger it was cast with
    box(M * T(vec3{0.f, H * 0.5f + 2.f, 0.f}) * S(vec3{1.9f, 0.5f, 0.6f}), rust);
    // ---- the lamp where its clapper should hang, on a chain ----
    vec3 lampAt = vec3{0.f, -1.3f, 0.f};
    box(M * T(vec3{0.f, (lampAt.y + H * 0.5f) * 0.5f, 0.f}) * S(vec3{0.25f, H * 0.5f - lampAt.y, 0.25f}), soot);
    lump(M * T(lampAt) * RY(t * 0.8f) * S(vec3{1.6f + 0.25f * flare}), vec3{0.5f, 0.06f, 0.04f}, red * 1.4f);
    box(M * T(lampAt + vec3{0.f, -0.6f, 0.f}) * S(vec3{2.6f, 0.08f, 2.6f}), vec3{0.f}, red * 0.25f);   // its light on the inside of the bell
    // ---- the wings: fingers of hanging chain, rags between, beating ----
    const float beat = c.grounded() ? -0.5f + 0.1f * die : 0.55f * std::sin(c.flap * 6.2832f);
    for (float s : {-1.f, 1.f}) {
        vec3 root{s * R * 0.75f, H * 0.18f, 0.f};
        vec3 tips[5];
        for (int f = 0; f < 5; ++f) {
            float spread = -0.35f + f * 0.32f;
            float len = 8.f + 5.f * std::sin(f * 0.8f + 0.6f);
            float lift = beat * (0.6f + 0.15f * f) + 0.1f;
            vec3 dir = glm::normalize(vec3{s * std::cos(spread), std::sin(lift) + 0.1f, std::sin(spread) * 0.9f});
            vec3 prev = root;
            const int LINKS = 7;
            for (int l = 1; l <= LINKS; ++l) {
                float u = (float)l / LINKS;
                vec3 p = root + dir * (len * u) + vec3{0.f, -1.6f * u * u * (1.f + 0.4f * std::sin(t * 2.f + f)), 0.f};   // sagging at the ends
                vec3 wp = vec3(M * glm::vec4(prev, 1.f)), wq = vec3(M * glm::vec4(p, 1.f));
                vec3 d = wq - wp;
                box(gleaner::alongZ((wp + wq) * 0.5f, d) * RZ(l % 2 ? 0.f : 1.5708f) * S(vec3{0.5f, 0.18f, glm::length(d) * 0.95f}), l % 2 ? iron * 1.3f : rust);
                prev = p;
            }
            tips[f] = prev;
            box(M * T(prev) * S(vec3{0.6f}), rust);
        }
        for (int f = 0; f < 4; ++f) {   // rags of cloth hung between the fingers, tattered, fluttering
            for (int r = 0; r < 3; ++r) {
                float u = 0.3f + 0.25f * r;
                vec3 a = glm::mix(root, tips[f], u), b = glm::mix(root, tips[f + 1], u);
                vec3 mid = (a + b) * 0.5f;
                float w = glm::length(b - a), drop = 1.6f + 1.2f * std::sin(f * 1.7f + r * 2.3f) + 0.6f;
                float flut = 0.25f * std::sin(t * 6.f + f * 2.f + r);
                vec3 wm = vec3(M * glm::vec4(mid + vec3{0.f, -drop * 0.5f, 0.f}, 1.f));
                vec3 wa = vec3(M * glm::vec4(a, 1.f)), wb = vec3(M * glm::vec4(b, 1.f));
                mat4 F = gleaner::alongZ(wm, wb - wa) * RZ(flut);
                box(F * S(vec3{0.06f, drop, w * (0.7f + 0.2f * std::sin(r + f))}), cloth * (1.f + 0.3f * (r % 2)));
            }
        }
    }
}
}  // namespace callerrig
