#pragma once
// TEMPORARY (gun-files migration): the gun recipes exactly as they were before the
// move to assets/guns/*.glb, so the test can prove the files draw the same guns.
#include "render/GunKit.h"
namespace gunkit_legacy {
using glm::vec3; using glm::mat4;
const vec3 BLACK{0.10f, 0.10f, 0.11f}, STEEL{0.32f, 0.32f, 0.34f}, EDGE{0.46f, 0.46f, 0.49f},
           BONE{0.76f, 0.71f, 0.61f}, BRASS{0.70f, 0.52f, 0.24f};
inline vec3 glowOf(int gun) {
    static const vec3 G[4] = {{1.f, 0.68f, 0.22f}, {1.f, 0.28f, 0.10f}, {0.30f, 0.90f, 1.f}, {0.70f, 0.35f, 1.f}};
    return G[std::clamp(gun, 0, 3)];
}
// One framing rule: every gun at the hip in the same lower-right region, at a
// similar size on screen (the framing test holds them to it). Each gun's hip
// is set in camera space - never by offsetting its model - so the rifles'
// sights and scope stay true when you aim.
inline vec3 hipOf(int gun) {
    static const vec3 H[4] = {{0.15f, -0.125f, 0.27f},   // revolver: high, the cylinder in view
                              {0.15f, -0.115f, 0.36f},   // shotgun: up and out, its barrel on screen
                              {0.19f, -0.17f,  0.31f},   // lancer
                              {0.20f, -0.195f, 0.34f}};  // longshot: lower and further, it's huge
    return H[std::clamp(gun, 0, 3)];
}
// The Lancer's sight line and the Longshot's scope axis above the gun origin:
// aiming moves the gun down by these so they sit on the screen's centre
constexpr float LANCER_SIGHT_Y = 0.052f, LONG_SCOPE_Y = 0.098f;
inline mat4 T(vec3 v) { return glm::translate(mat4(1.f), v); }
inline mat4 R(float deg, vec3 axis) { return glm::rotate(mat4(1.f), glm::radians(deg), axis); }
inline float seg(float u, float a, float b) { return std::clamp((u - a) / (b - a), 0.f, 1.f); }
inline float smooth(float t) { return t * t * (3.f - 2.f * t); }

struct Builder {
    std::vector<GunPart>& out;
    vec3 glow;
    vec3 emi{0.f};
    void box(const mat4& base, vec3 c, vec3 size, vec3 col) { out.push_back({base * T(c) * glm::scale(mat4(1.f), size), col, emi, 0}); }
    void lit(vec3 e) { emi = e; }
    void unlit() { emi = vec3{0.f}; }
    // n ammo cells laid out from `start` stepping `step` (wrapping into rows of
    // `perRow` along `rowStep`), the first `on` lit
    void cells(const mat4& base, vec3 start, vec3 step, vec3 rowStep, int perRow, vec3 size, int n, int on, float boost = 1.f) {
        for (int i = 0; i < n; ++i) {
            vec3 c = start + step * (float)(i % perRow) + rowStep * (float)(i / perRow);
            bool l = i < on;
            out.push_back({base * T(c) * glm::scale(mat4(1.f), size), l ? glow * 0.5f : BLACK * 0.6f,
                           l ? glow * (1.6f * boost) : vec3{0.f}, l ? 1 : 2});
        }
    }
    // One ammo cell (tag 1 lit, 2 dark)
    void cell(const mat4& base, vec3 c, vec3 size, bool on, float boost = 1.f) {
        out.push_back({base * T(c) * glm::scale(mat4(1.f), size), on ? glow * 0.5f : BLACK * 0.6f,
                       on ? glow * (1.6f * boost) : vec3{0.f}, on ? 1 : 2});
    }
    // The accent strip: brighter the fuller the gun, flaring as it fires
    vec3 accent(const GunLook& L) const {
        float fill = L.mag > 0 ? (float)L.ammo / L.mag : 0.f;
        float k = L.ammo == 0 ? 0.15f : 0.35f + 0.65f * fill;
        return glow * (k + 1.5f * L.flash);
    }
    void muzzleFlash(const mat4& base, vec3 at, float size, float flash) {
        if (flash <= 0.f) return;
        vec3 hot = glm::mix(glow, vec3{1.f}, 0.45f);
        lit(glow * flash * 1.8f);
        box(base, at, vec3{size, size, size * 1.4f} * (0.6f + 0.4f * flash), hot);
        box(base, at + vec3{0.f, 0.f, size * 0.6f}, vec3{size * 0.45f, size * 0.45f, size * 1.6f} * flash, hot);
        unlit();
    }
};

inline void buildRevolver(Builder& b, const GunLook& L, const GunPose& P);
inline void buildShotgun (Builder& b, const GunLook& L, const GunPose& P);
inline void buildLancer  (Builder& b, const GunLook& L, const GunPose& P);
inline void buildLongshot(Builder& b, const GunLook& L, const GunPose& P);

inline void buildGun(int gun, const GunLook& L, const GunPose& P, std::vector<GunPart>& out) {
    Builder b{out, glowOf(gun)};
    switch (gun) {
        case 0: buildRevolver(b, L, P); break;
        case 1: buildShotgun(b, L, P);  break;
        case 2: buildLancer(b, L, P);   break;
        default: buildLongshot(b, L, P); break;
    }
}

// The rifles' bolt: rotated up, drawn back (both 0..1)
inline void bolt(Builder& b, const mat4& base, vec3 pivot, float len, float lift, float pull, vec3 knob) {
    mat4 m = base * T(pivot + vec3{0.f, 0.f, -pull * 0.07f}) * glm::rotate(mat4(1.f), lift * 1.4f, vec3{0.f, 0.f, 1.f});
    b.box(m, {len * 0.5f, 0.f, 0.f}, {len, 0.011f, 0.011f}, STEEL);
    b.box(m, {len, -0.004f, 0.f}, {0.016f, 0.016f, 0.016f}, knob);
}
// Where the rifles' bolt is: the cycle after a shot, or the reload's OPEN/CLOSE
inline void boltState(const GunPose& P, float& lift, float& pull) {
    lift = pull = 0.f;
    if (P.cycleU >= 0.f) {
        float u = P.cycleU;
        lift = seg(u, 0.f, 0.2f) - seg(u, 0.8f, 1.f);
        pull = seg(u, 0.2f, 0.45f) - seg(u, 0.55f, 0.8f);
    }
    if (P.reloadU >= 0.f) {
        float u = P.reloadU;
        lift = seg(u, 0.03f, 0.08f) - seg(u, 0.92f, 0.97f);
        pull = seg(u, 0.08f, 0.15f) - seg(u, 0.86f, 0.92f);
    }
}

// THE REVOLVER — a hand cannon: a Desert Eagle's slab-sided bulk built round a
// swing-out cylinder. A tall bevelled barrel with a slotted top rail, a ported
// brake, the hooked guard, a raked grip with a beavertail. Industrial
// dressing - brass hex bolts, a heat gauge, hazard pinstripes, lightening
// cuts - and it breathes: a row of etched glyphs and the vents behind its cuts
// glow and fade on a slow breath (GunLook::breath), quicker and hotter as you
// fan it, dimmer as the cylinder empties. The chambers glow amber while loaded
// (the cells: one per round, a second ring when upgrades take it past eight).
inline void buildRevolver(Builder& b, const GunLook& L, const GunPose& P) {
    const mat4 base(1.f);
    const float CY = 0.018f, CZ = 0.036f;
    const vec3 acc = b.accent(L);
    float u = P.reloadU;
    bool  rl = u >= 0.f;
    float boost = 0.4f + 0.6f * P.boot;
    float swing = rl ? smooth(seg(u, 0.03f, 0.12f)) * (1.f - smooth(seg(u, 0.86f, 0.92f))) : 0.f;
    float spin  = rl ? smooth(seg(u, 0.86f, 0.97f)) * 540.f : 0.f;
    float fill  = L.mag > 0 ? (float)L.ammo / L.mag : 0.f;
    // The breath: what the glyphs and vents glow with
    float br = 0.18f + 0.82f * L.breath * L.breath;
    vec3 breath = b.glow * ((0.25f + 0.75f * fill) * br * (0.7f + 0.9f * L.heat) * boost + 2.0f * L.flash);
    const float S = 1.f;   // (both sides of a symmetric part)
    const float FTOP = 0.051f;   // the top of the frame behind the cylinder: what the sight and the pin sit on

    // ---- the frame behind the cylinder: slab sides, bolts, levers ----
    b.box(base, {0.f, 0.008f, -0.036f}, {0.052f, 0.086f, 0.064f}, BLACK);
    b.box(base, {0.f, 0.050f, -0.036f}, {0.044f, 0.004f, 0.062f}, EDGE * 0.8f);         // worn top edge
    for (float sx : {-1.f, 1.f}) {
        b.box(base, {sx * 0.0262f, 0.006f, -0.036f}, {0.002f, 0.066f, 0.056f}, STEEL * 0.8f);   // side plate
        b.lit(acc * (0.5f + L.heat * 0.8f));
        b.box(base, {sx * 0.0275f, 0.044f, -0.036f}, {0.001f, 0.003f, 0.054f}, b.glow);        // heat stripe
        b.unlit();
        for (vec3 at : {vec3{0.f, 0.034f, -0.062f}, vec3{0.f, 0.034f, -0.010f}, vec3{0.f, -0.022f, -0.062f}}) {
            b.box(base, at + vec3{sx * 0.0275f, 0.f, 0.f}, {0.002f, 0.006f, 0.006f}, BRASS);   // hex bolts
            b.box(base, at + vec3{sx * 0.0283f, 0.f, 0.f}, {0.001f, 0.0025f, 0.0025f}, BRASS * 0.5f);
        }
    }
    // The frame's dressing: it's the part that faces you at the hip
    {
        const float FZ0 = -0.068f, FZ1 = -0.004f;
        int n8 = std::max(1, L.mag);
        for (float sx : {-1.f, 1.f}) {
            float x = sx * 0.0274f;
            // a recessed panel, framed in bevelled steel
            b.box(base, {x, 0.010f, -0.036f}, {0.001f, 0.032f, 0.044f}, BLACK * 0.55f);
            b.box(base, {x + sx * 0.0004f, 0.0265f, -0.036f}, {0.001f, 0.0015f, 0.046f}, EDGE * 0.9f);
            b.box(base, {x + sx * 0.0004f, -0.0065f, -0.036f}, {0.001f, 0.0015f, 0.046f}, EDGE * 0.6f);
            for (float z : {-0.0585f, -0.0135f}) b.box(base, {x + sx * 0.0004f, 0.010f, z}, {0.001f, 0.034f, 0.0015f}, EDGE * 0.75f);
            // in it: a vent grille breathing (an upgraded cylinder's extra cells take its place on the left)
            if (sx > 0.f || n8 <= 8) {
                for (int k = 0; k < 5; ++k) {
                    float z = -0.052f + k * 0.0075f;
                    b.box(base, {x + sx * 0.0003f, 0.012f, z}, {0.001f, 0.022f, 0.0035f}, BLACK * 0.25f);
                    b.lit(breath * (0.55f + 0.15f * k));
                    b.box(base, {x + sx * 0.0006f, 0.012f, z}, {0.0005f, 0.016f, 0.0012f}, b.glow * 0.5f);
                    b.unlit();
                }
                // and a round seal beside it: a ring, a glyph breathing inside
                vec3 c{x + sx * 0.0003f, 0.012f, -0.0215f};
                b.box(base, c, {0.001f, 0.011f, 0.011f}, STEEL * 0.9f);
                b.box(base * T(c) * R(45.f, {1.f, 0.f, 0.f}), {}, {0.001f, 0.011f, 0.011f}, STEEL * 0.9f);
                b.box(base, c + vec3{sx * 0.0004f, 0.f, 0.f}, {0.001f, 0.008f, 0.008f}, BLACK * 0.4f);
                b.lit(breath);
                b.box(base, c + vec3{sx * 0.0008f, 0.f, 0.f}, {0.0005f, 0.0062f, 0.0011f}, b.glow * 0.7f);
                b.box(base, c + vec3{sx * 0.0008f, 0.0016f, 0.0016f}, {0.0005f, 0.0011f, 0.0034f}, b.glow * 0.7f);
                b.box(base, c + vec3{sx * 0.0008f, -0.0016f, -0.0016f}, {0.0005f, 0.0011f, 0.0034f}, b.glow * 0.7f);
                b.unlit();
            }
            // the serial plate under it: two lines of stamped marks
            b.box(base, {x, -0.019f, -0.044f}, {0.001f, 0.009f, 0.026f}, STEEL);
            for (int r = 0; r < 2; ++r)
                for (int k = 0; k < 9; ++k)
                    if ((k * 7 + r * 3) % 5 != 0)
                        b.box(base, {x + sx * 0.0005f, -0.0168f - r * 0.0042f, -0.0545f + k * 0.0026f}, {0.0005f, 0.0018f, 0.0012f}, BLACK * 0.6f);
            // chamfers down its back and front corners
            for (float z : {FZ0 + 0.001f, FZ1 - 0.001f})
                b.box(base * T({sx * 0.0255f, 0.008f, z}) * R(45.f, {0.f, 1.f, 0.f}), {}, {0.003f, 0.084f, 0.003f}, EDGE * 0.7f);
        }
        // the cylinder release: a ridged thumb latch on the left, just behind the cylinder
        b.box(base, {-0.0285f, 0.004f, -0.0075f}, {0.003f, 0.010f, 0.008f}, STEEL);
        for (int k = 0; k < 4; ++k) b.box(base, {-0.0302f, 0.0005f + k * 0.0024f, -0.0075f}, {0.0006f, 0.001f, 0.0085f}, EDGE);
        // a brass line running forward along the frame's lower edge, clipped down
        b.box(base, {-0.0245f, -0.031f, 0.010f}, {0.003f, 0.003f, 0.11f}, BRASS * 0.85f);
        for (float z : {-0.040f, 0.010f, 0.058f}) b.box(base, {-0.0247f, -0.031f, z}, {0.004f, 0.005f, 0.004f}, STEEL);
        b.box(base, {-0.0245f, -0.031f, -0.046f}, {0.004f, 0.004f, 0.004f}, BRASS);
        // the back face: an inset plate, two screws, a breathing slot under the hammer, grooves either side
        b.box(base, {0.f, 0.002f, FZ0 - 0.0005f}, {0.040f, 0.060f, 0.001f}, STEEL * 0.7f);
        for (float sx : {-1.f, 1.f}) {
            b.box(base, {sx * 0.015f, -0.022f, FZ0 - 0.0012f}, {0.005f, 0.005f, 0.001f}, BRASS);
            b.box(base, {sx * 0.015f, -0.022f, FZ0 - 0.0018f}, {0.0045f, 0.001f, 0.0005f}, BRASS * 0.4f);
            for (int k = 0; k < 4; ++k) b.box(base, {sx * 0.0155f, 0.008f + k * 0.0055f, FZ0 - 0.0012f}, {0.008f, 0.0015f, 0.001f}, BLACK * 0.5f);
        }
        b.box(base, {0.f, 0.000f, FZ0 - 0.0012f}, {0.005f, 0.026f, 0.001f}, BLACK * 0.3f);
        b.lit(breath * 0.9f);
        b.box(base, {0.f, 0.000f, FZ0 - 0.0018f}, {0.0018f, 0.020f, 0.0005f}, b.glow * 0.6f);
        b.unlit();
        // on top: the rail's slots carry on over the frame; glowing dots either side of the notch
        for (int k = 0; k < 2; ++k) b.box(base, {0.f, 0.0515f, -0.046f + k * 0.018f}, {0.020f, 0.002f, 0.006f}, BLACK * 0.6f);
        b.lit(b.glow * 1.1f);
        for (float sx : {-1.f, 1.f}) b.box(base, {sx * 0.008f, FTOP + 0.0125f, -0.0645f}, {0.0025f, 0.0025f, 0.001f}, b.glow);
        b.unlit();
        // a loaded-chamber pin that stands up while there's a round left
        float pin = L.ammo > 0 ? 0.004f : 0.f;
        b.box(base, {0.f, FTOP + (0.004f + pin) * 0.5f - 0.001f, -0.010f}, {0.004f, 0.004f + pin, 0.004f}, L.ammo > 0 ? BRASS : STEEL * 0.6f);
        // the recoil shield: a steel ring behind the cylinder
        for (int k = 0; k < 4; ++k) b.box(base * T({0.f, CY, -0.0035f}) * R(k * 45.f, {0.f, 0.f, 1.f}), {}, {0.024f, 0.058f, 0.002f}, STEEL * 0.75f);
        b.box(base, {0.f, CY, -0.0045f}, {0.006f, 0.006f, 0.002f}, EDGE);   // the firing pin's bushing
    }
    b.box(base, {0.0275f, -0.004f, -0.020f}, {0.003f, 0.012f, 0.016f}, STEEL);             // the release, right side
    b.box(base, {0.0285f, 0.002f, -0.020f}, {0.002f, 0.004f, 0.020f}, EDGE);
    b.box(base, {-0.0272f, 0.050f, -0.066f}, {0.004f, 0.006f, 0.010f}, STEEL);              // safety lever, left
    // top strap over the cylinder, and the frame under it
    b.box(base, {0.f, 0.056f, CZ}, {0.040f, 0.014f, 0.082f}, BLACK);
    b.box(base, {0.f, 0.0635f, CZ}, {0.024f, 0.002f, 0.080f}, EDGE * 0.8f);
    b.box(base, {0.f, -0.026f, CZ}, {0.046f, 0.020f, 0.080f}, BLACK);
    b.box(base, {0.f, 0.022f, 0.078f}, {0.046f, 0.070f, 0.014f}, BLACK);                  // the face the barrel bolts to
    b.box(base, {0.f, 0.022f, 0.0715f}, {0.050f, 0.074f, 0.002f}, STEEL * 0.6f);
    // rear sight: a wide block with a notch, sat on the frame
    b.box(base, {0.f, FTOP + 0.002f, -0.058f}, {0.024f, 0.004f, 0.014f}, STEEL * 0.7f);
    for (float sx : {-1.f, 1.f}) b.box(base, {sx * 0.008f, FTOP + 0.010f, -0.058f}, {0.007f, 0.012f, 0.012f}, BLACK);

    // ---- the barrel: a tall slab, bevelled to a ribbed rail ----
    const float BZ = 0.21f, BL = 0.25f;
    b.box(base, {0.f, 0.010f, BZ}, {0.044f, 0.040f, BL}, BLACK);                          // lower slab
    b.box(base, {0.f, 0.042f, BZ}, {0.030f, 0.026f, BL}, BLACK);                          // upper slab
    for (float sx : {-1.f, 1.f}) {
        mat4 bev = base * T({sx * 0.0185f, 0.0315f, BZ}) * R(sx * 38.f, {0.f, 0.f, 1.f});
        b.box(bev, {}, {0.014f, 0.004f, BL}, STEEL * 0.9f);                                // the bevel, worn bright
        b.box(base, {sx * 0.0222f, 0.029f, BZ}, {0.001f, 0.002f, BL - 0.01f}, EDGE);        // its lower arris
    }
    b.box(base, {0.f, 0.0585f, BZ}, {0.020f, 0.005f, BL - 0.01f}, STEEL);                 // top rail
    for (int k = 0; k < 10; ++k) b.box(base, {0.f, 0.0605f, 0.100f + k * 0.024f}, {0.022f, 0.002f, 0.009f}, BLACK);
    // lightening cuts down both sides, the vents breathing behind them
    for (float sx : {-1.f, 1.f})
        for (int k = 0; k < 3; ++k) {
            float z = 0.150f + k * 0.052f;
            b.box(base, {sx * 0.0221f, 0.013f, z}, {0.002f, 0.014f, 0.040f}, BLACK * 0.3f);
            b.lit(breath * 0.8f);
            b.box(base, {sx * 0.0226f, 0.013f, z}, {0.001f, 0.004f, 0.030f}, b.glow * 0.6f);
            b.unlit();
            for (int f = -1; f <= 1; f += 2) b.box(base, {sx * 0.0226f, 0.013f + f * 0.0045f, z}, {0.001f, 0.001f, 0.032f}, BLACK * 0.5f);   // the vent's fins
        }
    // the etched glyphs along the lower slab, the side you see most: each a few strokes
    static const float GLYPH[6][3][4] = {   // per stroke: dy, dz, height, length (mm-ish, scaled below)
        {{0, 0, 7, 1}, {3, 2, 1, 5}, {-3, 2, 1, 5}},
        {{0, -2, 7, 1}, {0, 2, 7, 1}, {0, 0, 1, 5}},
        {{3, 0, 1, 6}, {0, 0, 1, 6}, {-3, 0, 1, 6}},
        {{0, 0, 7, 1}, {3, 2, 1, 4}, {0, 2, 1, 4}},
        {{2, -1, 3, 1}, {-2, 1, 3, 1}, {0, 0, 1, 6}},
        {{0, 0, 1, 6}, {2, 0, 3, 1}, {-2, 0, 3, 1}}};
    for (float sx : {-1.f, 1.f})
        for (int g = 0; g < 6; ++g) {
            float z0 = 0.118f + g * 0.011f + (g >= 3 ? 0.006f : 0.f);
            float pulse = 0.75f + 0.25f * std::sin(L.breath * 3.14159f + g * 0.6f);   // the breath runs along the row
            b.lit(breath * pulse);
            for (auto& s : GLYPH[g]) b.box(base, {sx * 0.0223f, -0.003f + s[0] * 0.0008f, z0 + s[1] * 0.0008f},
                                           {0.001f, s[2] * 0.0008f + 0.0006f, s[3] * 0.0008f + 0.0006f}, b.glow * 0.7f);
            b.unlit();
        }
    // a heat gauge on the left of the slab: its needle climbs as you fan it
    {
        vec3 c{-0.0225f, 0.010f, 0.104f};
        b.box(base, c, {0.002f, 0.018f, 0.018f}, EDGE);
        b.box(base * T(c) * R(45.f, {1.f, 0.f, 0.f}), {}, {0.002f, 0.018f, 0.018f}, EDGE);
        b.box(base, c + vec3{-0.0008f, 0.f, 0.f}, {0.002f, 0.015f, 0.015f}, BLACK * 0.4f);
        for (int t = 0; t < 5; ++t) {   // its ticks
            float a = glm::radians(-60.f + t * 30.f);
            b.box(base, c + vec3{-0.0016f, std::cos(a) * 0.006f, std::sin(a) * 0.006f}, {0.001f, 0.0012f, 0.0012f}, t == 4 ? vec3{0.6f, 0.15f, 0.1f} : BONE);
        }
        b.lit(b.glow * (0.6f + 1.4f * L.heat));
        mat4 nd = base * T(c + vec3{-0.0018f, 0.f, 0.f}) * R(60.f - 120.f * L.heat, {1.f, 0.f, 0.f});
        b.box(nd, {0.f, 0.0028f, 0.f}, {0.001f, 0.0056f, 0.0008f}, b.glow);
        b.unlit();
        b.box(base, c + vec3{-0.0019f, 0.f, 0.f}, {0.001f, 0.0018f, 0.0018f}, BRASS);
    }
    // the underlug: hazard pinstripes and a line of bolts
    b.box(base, {0.f, -0.018f, BZ + 0.005f}, {0.034f, 0.018f, BL - 0.02f}, BLACK);
    b.box(base, {0.f, -0.0275f, BZ + 0.005f}, {0.028f, 0.002f, BL - 0.03f}, STEEL * 0.7f);
    for (float sx : {-1.f, 1.f}) {
        for (int k = 0; k < 14; ++k) {
            mat4 st = base * T({sx * 0.0172f, -0.018f, 0.130f + k * 0.009f}) * R(40.f, {1.f, 0.f, 0.f});
            if (k % 2) { b.lit(b.glow * 0.25f); b.box(st, {}, {0.001f, 0.012f, 0.003f}, b.glow * 0.5f); b.unlit(); }
            else b.box(st, {}, {0.001f, 0.012f, 0.003f}, BLACK * 0.5f);
        }
        for (int k = 0; k < 3; ++k) b.box(base, {sx * 0.0173f, -0.018f, 0.272f + k * 0.018f}, {0.002f, 0.004f, 0.004f}, BRASS);
    }
    // the brake: a heavy block, ported both sides and on top, the bore
    const float MZ = 0.356f;
    b.box(base, {0.f, 0.020f, MZ}, {0.048f, 0.068f, 0.042f}, BLACK);
    b.box(base, {0.f, 0.020f, MZ + 0.0215f}, {0.040f, 0.060f, 0.002f}, STEEL * 0.7f);
    b.box(base, {0.f, 0.026f, MZ + 0.0222f}, {0.016f, 0.016f, 0.002f}, BLACK * 0.15f);    // the bore
    vec3 port = b.glow * (0.15f + 1.6f * L.heat + 2.5f * L.flash);
    for (float sx : {-1.f, 1.f})
        for (int k = 0; k < 3; ++k) {
            b.box(base, {sx * 0.0242f, 0.024f, MZ - 0.012f + k * 0.012f}, {0.002f, 0.042f, 0.006f}, BLACK * 0.2f);
            b.lit(port);
            b.box(base, {sx * 0.0236f, 0.024f, MZ - 0.012f + k * 0.012f}, {0.001f, 0.034f, 0.003f}, b.glow * 0.4f);
            b.unlit();
        }
    for (int k = 0; k < 2; ++k) {
        b.box(base, {0.f, 0.0545f, MZ - 0.006f + k * 0.013f}, {0.030f, 0.002f, 0.006f}, BLACK * 0.2f);
        b.lit(port); b.box(base, {0.f, 0.0538f, MZ - 0.006f + k * 0.013f}, {0.024f, 0.001f, 0.003f}, b.glow * 0.4f); b.unlit();
    }
    // front sight: a blade with a glowing insert
    b.box(base, {0.f, 0.063f, 0.320f}, {0.006f, 0.012f, 0.020f}, BLACK);
    b.lit(b.glow * 1.4f);
    b.box(base, {0.f, 0.067f, 0.326f}, {0.004f, 0.004f, 0.004f}, b.glow);
    b.unlit();

    // ---- hammer: a ring with a grooved spur, falls as it fires ----
    mat4 hm = base * T({0.f, 0.040f, -0.068f}) * R(-28.f + P.fireU * 28.f, {1.f, 0.f, 0.f});
    for (float sx : {-1.f, 1.f}) b.box(hm, {sx * 0.0045f, 0.012f, -0.004f}, {0.003f, 0.024f, 0.010f}, BLACK);
    b.box(hm, {0.f, 0.025f, -0.006f}, {0.012f, 0.005f, 0.014f}, STEEL);
    for (int k = 0; k < 3; ++k) b.box(hm, {0.f, 0.0278f, -0.011f + k * 0.004f}, {0.012f, 0.001f, 0.0015f}, BLACK);

    // ---- the cylinder on its crane ----
    vec3 pivot{-0.024f, -0.022f, CZ};
    mat4 crane = base * T(pivot) * R(swing * 82.f, {0.f, 0.f, 1.f}) * T(-pivot);
    b.box(crane, {-0.012f, -0.022f, CZ + 0.044f}, {0.012f, 0.008f, 0.02f}, STEEL);
    mat4 cm = crane * T({0.f, CY, CZ}) * R(L.cylSpin + spin, {0.f, 0.f, 1.f});
    for (int k = 0; k < 8; ++k)   // sixteen-sided: it reads round
        b.box(cm * R(k * 22.5f, {0.f, 0.f, 1.f}), {}, {0.0123f, 0.062f, 0.070f}, k % 2 ? STEEL * 0.85f : STEEL);
    for (int k = 0; k < 8; ++k) {   // the flutes between the chambers
        mat4 f = cm * R(22.5f + k * 45.f, {0.f, 0.f, 1.f});
        b.box(f, {0.f, 0.0305f, 0.f}, {0.006f, 0.002f, 0.052f}, BLACK * 0.6f);
    }
    for (float z : {0.034f, -0.034f})   // bevelled rims, front and back
        for (int k = 0; k < 4; ++k) b.box(cm * R(k * 45.f, {0.f, 0.f, 1.f}), {0.f, 0.f, z}, {0.0245f, 0.0595f, 0.003f}, EDGE);
    b.lit(acc * (0.6f + L.heat));
    for (int k = 0; k < 4; ++k) b.box(cm * R(k * 45.f, {0.f, 0.f, 1.f}), {0.f, 0.f, 0.03f}, {0.027f, 0.0645f, 0.003f}, b.glow);
    b.unlit();
    // The cells: slots round the cylinder's side for the first eight rounds;
    // any beyond (an upgraded cylinder) in rows of four on the frame's left
    // side plate, the side you see
    int n = std::max(1, L.mag), ring1 = std::min(n, 8);
    for (int k = 0; k < ring1; ++k) {
        mat4 f = cm * R(k * 360.f / ring1, {0.f, 0.f, 1.f});
        b.cell(f, {0.f, 0.0313f, -0.006f}, {0.011f, 0.002f, 0.044f}, k < L.lit, boost);
    }
    if (n > 8)
        b.cells(base, {-0.0285f, 0.018f, -0.054f}, {0.f, 0.f, 0.011f}, {0.f, -0.012f, 0.f}, 4,
                {0.002f, 0.008f, 0.008f}, n - 8, std::max(0, L.lit - 8), boost);
    for (int k = 0; k < 8; ++k) {   // the chamber faces, front and back (dressing)
        float a = glm::radians(90.f + k * 45.f);
        vec3 c{std::cos(a) * 0.021f, std::sin(a) * 0.021f, 0.f};
        for (float z : {0.0365f, -0.0365f}) b.box(cm, c + vec3{0.f, 0.f, z}, {0.011f, 0.011f, 0.003f}, BLACK * 0.5f);
    }
    b.box(crane, {0.f, CY, CZ + 0.05f}, {0.006f, 0.006f, 0.03f}, EDGE);
    b.box(crane, {0.f, CY, CZ}, {0.012f, 0.012f, 0.076f}, EDGE);
    // the brass tumbling out (OPEN), the speedloader (FEED)
    if (rl && u > 0.10f && u < 0.30f) {
        float t = seg(u, 0.10f, 0.30f);
        for (int k = 0; k < 8; ++k) {
            float a = glm::radians(90.f + k * 45.f);
            vec3 from{std::cos(a) * 0.021f, CY + std::sin(a) * 0.021f, CZ - 0.04f};
            float tk = std::max(0.f, t * 1.3f - k * 0.03f);
            vec3 at = from + vec3{-0.03f * tk + std::cos(a) * 0.02f * tk, -0.35f * tk * tk, -0.12f * tk};
            b.box(crane * T(at) * R(tk * 400.f + k * 30.f, {1.f, 0.3f, 0.f}), {}, {0.009f, 0.009f, 0.030f}, BRASS * 0.8f);
        }
    }
    if (rl && u > 0.25f && u < 0.70f) {
        float in = smooth(seg(u, 0.25f, 0.45f)), away = smooth(seg(u, 0.55f, 0.70f));
        vec3 seat{0.f, CY, CZ - 0.06f};
        vec3 at = seat + vec3{-0.02f, -0.22f, -0.10f} * (1.f - in) + vec3{-0.06f, -0.25f, -0.05f} * away;
        mat4 lm = crane * T(at);
        b.box(lm, {0.f, 0.f, -0.012f}, {0.05f, 0.05f, 0.016f}, BLACK);
        b.box(lm, {0.f, 0.f, -0.026f}, {0.018f, 0.018f, 0.014f}, EDGE);
        if (u < 0.47f)
            for (int k = 0; k < 8; ++k) {
                float a = glm::radians(90.f + k * 45.f);
                vec3 c{std::cos(a) * 0.021f, std::sin(a) * 0.021f, 0.012f};
                b.box(lm, c, {0.009f, 0.009f, 0.024f}, BRASS);
                b.lit(b.glow * 0.6f);
                b.box(lm, c + vec3{0.f, 0.f, 0.013f}, {0.006f, 0.006f, 0.004f}, b.glow);
                b.unlit();
            }
    }

    // ---- the guard (squared, hooked at the front), the trigger ----
    b.box(base, {0.f, -0.058f, 0.004f}, {0.016f, 0.007f, 0.070f}, BLACK);
    b.box(base, {0.f, -0.042f, 0.037f}, {0.016f, 0.034f, 0.007f}, BLACK);
    b.box(base, {0.f, -0.061f, 0.044f}, {0.016f, 0.010f, 0.012f}, BLACK);                 // the hook
    b.box(base, {0.f, -0.0545f, 0.004f}, {0.010f, 0.001f, 0.060f}, EDGE * 0.7f);
    b.box(base, {0.f, -0.036f, -0.002f + P.fireU * 0.004f}, {0.007f, 0.028f, 0.008f}, EDGE);
    b.box(base, {0.f, -0.049f, 0.000f + P.fireU * 0.004f}, {0.008f, 0.004f, 0.010f}, EDGE);

    // ---- the grip: raked, stippled bone panels, a beavertail, a flared butt ----
    mat4 gm = base * T({0.f, -0.034f, -0.052f}) * R(-20.f, {1.f, 0.f, 0.f});
    b.box(gm, {0.f, -0.056f, -0.004f}, {0.044f, 0.116f, 0.054f}, BLACK);
    b.box(gm, {0.f, 0.006f, -0.034f}, {0.040f, 0.012f, 0.026f}, BLACK);                     // beavertail
    for (int k = 0; k < 3; ++k) b.box(gm, {0.f, -0.040f - k * 0.024f, 0.024f}, {0.040f, 0.010f, 0.006f}, BLACK * 1.4f);   // finger ridges
    for (float sx : {-1.f, 1.f}) {
        b.box(gm, {sx * 0.0225f, -0.060f, -0.004f}, {0.004f, 0.094f, 0.046f}, BONE);
        for (int r = 0; r < 6; ++r)       // stippling
            for (int c = 0; c < 3; ++c)
                if ((r + c) % 2 == 0) b.box(gm, {sx * 0.0246f, -0.094f + r * 0.010f, -0.016f + c * 0.012f}, {0.001f, 0.004f, 0.004f}, BONE * 0.62f);
        b.lit(BRASS * 0.3f);
        b.box(gm, {sx * 0.0248f, -0.026f, -0.004f}, {0.002f, 0.014f, 0.014f}, BRASS);       // medallion
        b.unlit();
        b.lit(breath * 0.6f);
        b.box(gm, {sx * 0.0251f, -0.026f, -0.004f}, {0.001f, 0.006f, 0.002f}, b.glow * 0.5f);   // its glyph, breathing too
        b.box(gm, {sx * 0.0251f, -0.026f, -0.004f}, {0.001f, 0.002f, 0.006f}, b.glow * 0.5f);
        b.unlit();
    }
    b.box(gm, {0.f, -0.118f, -0.004f}, {0.050f, 0.014f, 0.062f}, STEEL);                   // the butt, flared
    b.box(gm, {0.f, -0.1255f, -0.004f}, {0.040f, 0.002f, 0.050f}, BLACK);
    b.box(gm, {0.f, -0.122f, -0.036f}, {0.010f, 0.008f, 0.006f}, EDGE);                   // lanyard loop

    b.muzzleFlash(base, {0.f, 0.026f, 0.392f}, 0.05f, L.flash);
    if (L.flash > 0.f) {   // the brake throws its jets sideways and up
        b.lit(b.glow * L.flash * 1.5f);
        for (float sx : {-1.f, 1.f}) b.box(base, {sx * (0.034f + 0.02f * L.flash), 0.024f, MZ}, {0.03f * L.flash, 0.008f, 0.01f}, glm::mix(b.glow, vec3{1.f}, 0.5f));
        b.box(base, {0.f, 0.066f + L.flash * 0.02f, MZ}, {0.012f, 0.03f * L.flash, 0.012f}, glm::mix(b.glow, vec3{1.f}, 0.5f));
        b.unlit();
    }
    (void)S;
}

// THE SHOTGUN — a short, wide pump gun: a heavy barrel over its tube, a bone
// pump grip, a shell window on the right that glows ember for each shell
// loaded, an ember strip along the barrel.
inline void buildShotgun(Builder& b, const GunLook& L, const GunPose& P) {
    const mat4 base(1.f);
    const vec3 acc = b.accent(L);
    float boost = 0.4f + 0.6f * P.boot;
    float slide = 0.f;
    if (P.cycleU >= 0.f) slide = -0.08f * std::sin(P.cycleU * 3.14159265f);
    if (P.reloadU >= 0.f) slide = -0.08f * std::sin(seg(P.reloadU, 0.86f, 0.98f) * 3.14159265f);

    b.box(base, {0.f, 0.028f, 0.12f}, {0.048f, 0.048f, 0.32f}, STEEL);           // barrel
    b.lit(acc);
    b.box(base, {0.f, 0.053f, 0.12f}, {0.006f, 0.003f, 0.30f}, b.glow);          // the strip along its top
    b.unlit();
    b.box(base, {0.f, -0.018f, 0.14f}, {0.032f, 0.032f, 0.26f}, BLACK);          // magazine tube
    b.box(base, {0.f, 0.005f, -0.04f}, {0.065f, 0.075f, 0.14f}, BLACK);          // receiver
    b.box(base, {0.f, 0.042f, -0.04f}, {0.04f, 0.004f, 0.12f}, EDGE);            // its top rail
    // the shell window on top of the receiver (the side you see): a cell per shell
    b.box(base, {0.f, 0.044f, -0.04f}, {0.03f, 0.003f, 0.13f}, STEEL);
    b.cells(base, {-0.007f, 0.0465f, -0.09f}, {0.f, 0.f, 0.024f}, {0.014f, 0.f, 0.f}, 5,
            {0.01f, 0.004f, 0.018f}, std::max(1, L.mag), L.lit, boost);
    // shells fed one by one through the loading port (FEED), thumb behind
    if (P.reloadU >= 0.f) {
        float u = P.reloadU;
        int n = std::max(1, L.refill);
        float span = 0.6f / n;
        for (int i = 0; i < n; ++i) {
            float t = seg(u, 0.25f + i * span, 0.25f + (i + 0.85f) * span);
            if (t <= 0.f || t >= 1.f) continue;
            float up = smooth(std::min(1.f, t * 1.7f)), push = smooth(seg(t, 0.55f, 1.f));
            vec3 port{0.f, -0.046f, -0.02f};
            vec3 at = port + vec3{-0.13f, -0.05f, -0.03f} * (1.f - up) + vec3{0.f, 0.02f, 0.07f} * push;
            mat4 sm = base * T(at) * R((1.f - up) * 40.f, {1.f, 0.f, 0.f});
            b.box(sm, {}, {0.022f, 0.022f, 0.050f}, BLACK * 1.6f);
            b.lit(b.glow * 0.8f);
            b.box(sm, {0.f, 0.f, 0.012f}, {0.023f, 0.023f, 0.006f}, b.glow);   // the shell's glowing band
            b.unlit();
            b.box(sm, {0.f, 0.f, -0.029f}, {0.024f, 0.024f, 0.010f}, BRASS);
            b.box(sm, {-0.016f, -0.012f, -0.04f + push * 0.01f}, {0.022f, 0.026f, 0.03f}, BLACK * 1.3f);
        }
    }
    // the pump: bone grip on the slide
    b.box(base, {0.f, -0.005f, 0.08f + slide}, {0.050f, 0.044f, 0.10f}, BONE);
    for (int k = 0; k < 4; ++k) b.box(base, {0.f, -0.005f, 0.044f + k * 0.024f + slide}, {0.052f, 0.046f, 0.006f}, BONE * 0.7f);
    // side saddle: four spares on the left
    b.box(base, {-0.036f, 0.004f, -0.04f}, {0.008f, 0.05f, 0.11f}, BLACK);
    for (int k = 0; k < 4; ++k) {
        b.box(base, {-0.044f, 0.004f, -0.08f + k * 0.026f}, {0.012f, 0.044f, 0.02f}, BLACK * 1.6f);
        b.box(base, {-0.044f, -0.022f, -0.08f + k * 0.026f}, {0.013f, 0.009f, 0.021f}, BRASS);
    }
    b.box(base, {0.f, -0.046f, -0.02f}, {0.026f, 0.004f, 0.05f}, {0.05f, 0.05f, 0.05f});   // loading port
    b.box(base, {0.f, -0.035f, -0.16f}, {0.048f, 0.065f, 0.10f}, BLACK);                   // stock
    b.box(base, {0.f, -0.060f, -0.22f}, {0.040f, 0.055f, 0.06f}, BLACK);
    b.box(base, {0.f, -0.050f, -0.04f}, {0.020f, 0.022f, 0.070f}, BLACK);                  // guard
    b.box(base, {0.f, -0.120f, -0.08f}, {0.044f, 0.100f, 0.055f}, BLACK);                  // grip
    for (float sx : {-1.f, 1.f}) b.box(base, {sx * 0.023f, -0.120f, -0.08f}, {0.003f, 0.085f, 0.045f}, BONE);
    b.muzzleFlash(base, {0.f, 0.028f, 0.30f}, 0.05f, L.flash);
}

// THE LANCER — a long, slim bolt rifle in blackened steel: a bone wrist and
// cheek panel, worn-steel bands, a cyan strip down the forend and a row of
// cyan cells along the receiver. Rear notch at z 0.11 and front post at
// z 0.63 both peak at LANCER_SIGHT_Y, so aiming lines them up on the crosshair.
inline void buildLancer(Builder& b, const GunLook& L, const GunPose& P) {
    const mat4 base(1.f);
    const vec3 acc = b.accent(L);
    float boost = 0.4f + 0.6f * P.boot;
    b.box(base, {0.f, -0.040f, -0.150f}, {0.050f, 0.066f, 0.30f}, BLACK);         // stock
    b.box(base, {0.f, -0.080f, -0.270f}, {0.052f, 0.100f, 0.07f}, BLACK);
    b.box(base, {0.f, -0.082f, -0.308f}, {0.054f, 0.104f, 0.012f}, STEEL);         // butt plate
    b.box(base, {0.f, -0.068f, -0.040f}, {0.040f, 0.070f, 0.07f}, BONE);           // the wrist
    b.box(base, {0.f, -0.012f, -0.170f}, {0.052f, 0.012f, 0.12f}, STEEL);          // cheek plate (in your eye line when aimed)
    b.box(base, {0.f, 0.014f, 0.020f}, {0.040f, 0.044f, 0.17f}, BLACK);            // receiver
    b.box(base, {0.f, -0.034f, 0.050f}, {0.034f, 0.026f, 0.08f}, BLACK);
    b.box(base, {0.f, -0.056f, -0.005f}, {0.016f, 0.020f, 0.05f}, BLACK);          // guard
    b.cells(base, {-0.0215f, 0.026f, -0.03f}, {0.f, 0.f, 0.022f}, {0.f, -0.012f, 0.f}, 8,   // the left side: the one you see
            {0.004f, 0.008f, 0.016f}, std::max(1, L.mag), L.lit, boost);
    b.box(base, {0.f, -0.004f, 0.250f}, {0.044f, 0.040f, 0.32f}, BLACK);           // forend
    b.lit(acc);
    b.box(base, {0.0225f, -0.004f, 0.25f}, {0.002f, 0.006f, 0.30f}, b.glow);      // its strip
    b.unlit();
    b.box(base, {0.f, 0.004f, 0.200f}, {0.048f, 0.050f, 0.012f}, EDGE);            // bands
    b.box(base, {0.f, 0.004f, 0.380f}, {0.048f, 0.050f, 0.012f}, EDGE);
    b.box(base, {0.f, 0.028f, 0.360f}, {0.020f, 0.020f, 0.58f}, STEEL);            // barrel
    b.box(base, {-0.010f, 0.044f, 0.110f}, {0.008f, 0.024f, 0.012f}, EDGE);        // rear sight
    b.box(base, { 0.010f, 0.044f, 0.110f}, {0.008f, 0.024f, 0.012f}, EDGE);
    b.box(base, { 0.f,    0.036f, 0.110f}, {0.028f, 0.008f, 0.016f}, EDGE);
    b.lit(b.glow * 0.8f);
    b.box(base, { 0.f, 0.044f, 0.630f}, {0.004f, 0.016f, 0.008f}, glm::mix(b.glow, vec3{1.f}, 0.5f));   // front post
    b.unlit();
    b.box(base, {-0.011f, 0.046f, 0.630f}, {0.004f, 0.024f, 0.014f}, BLACK);
    b.box(base, { 0.011f, 0.046f, 0.630f}, {0.004f, 0.024f, 0.014f}, BLACK);
    float lift, pull;
    boltState(P, lift, pull);
    bolt(b, base, {0.020f, 0.022f, -0.035f}, 0.036f, lift, pull, EDGE);
    // the stripper-clip reload: the empty (OPEN), the clip in, the rounds pressed (FEED), the clip flicked
    if (P.reloadU >= 0.f) {
        float u = P.reloadU;
        float ej = seg(u, 0.10f, 0.24f);
        if (ej > 0.f && ej < 1.f) {
            vec3 at = vec3{0.02f, 0.04f, 0.03f} + vec3{0.16f, 0.f, -0.05f} * ej + vec3{0.f, 0.11f * std::sin(ej * 3.14159f) - 0.05f * ej * ej, 0.f};
            b.box(base * T(at) * R(ej * 540.f, {0.3f, 0.2f, 1.f}), {}, {0.01f, 0.01f, 0.04f}, BRASS);
        }
        float in = smooth(seg(u, 0.25f, 0.38f)), press = smooth(seg(u, 0.40f, 0.80f)), flick = seg(u, 0.80f, 0.86f);
        if (u >= 0.25f && flick < 1.f) {
            vec3 seat{0.f, 0.07f, -0.005f};
            vec3 clipAt = seat + vec3{0.09f, 0.1f, -0.04f} * (1.f - in)
                        + vec3{0.12f * flick, 0.09f * std::sin(flick * 3.14159f) + 0.02f * flick, -0.03f * flick};
            mat4 cm = base * T(clipAt) * R((1.f - in) * -35.f + flick * 300.f, {0.f, 0.f, 1.f});
            b.box(cm, {0.f, 0.f, -0.009f}, {0.016f, 0.066f, 0.004f}, STEEL);
            b.box(cm, {0.f, 0.032f, 0.f}, {0.016f, 0.004f, 0.02f}, STEEL);
            b.box(cm, {0.f, -0.032f, 0.f}, {0.016f, 0.004f, 0.02f}, STEEL);
            int n = std::clamp(L.refill, 1, 5);
            if (flick <= 0.f)
                for (int i = 0; i < n; ++i) {
                    mat4 rm = base * T(clipAt + vec3{0.f, 0.024f - i * 0.012f - press * 0.075f, 0.022f});
                    b.box(rm, {}, {0.0095f, 0.0095f, 0.042f}, BRASS);
                    b.lit(b.glow * 0.5f);
                    b.box(rm, {0.f, 0.f, 0.029f}, {0.007f, 0.007f, 0.018f}, b.glow);
                    b.unlit();
                }
            float handIn = smooth(seg(u, 0.21f, 0.31f)) * (1.f - smooth(seg(u, 0.82f, 0.86f)));
            if (handIn > 0.01f) {
                vec3 thumb = clipAt + vec3{0.f, 0.04f - press * 0.06f, 0.012f};
                vec3 hand = thumb + vec3{-0.03f, -0.005f, -0.004f} + vec3{-0.12f, -0.1f, -0.04f} * (1.f - handIn);
                mat4 hm = base * T(hand) * R(20.f, {0.f, 0.f, 1.f});
                b.box(hm, {}, {0.028f, 0.034f, 0.03f}, BLACK * 1.3f);
                b.box(hm, {-0.03f, -0.03f, -0.02f}, {0.026f, 0.026f, 0.08f}, BLACK * 1.6f);
                b.box(base * T(thumb), {}, {0.013f, 0.012f, 0.022f}, BONE * 0.8f);
            }
        }
    }
    b.muzzleFlash(base, {0.f, 0.028f, 0.68f}, 0.05f, L.flash);
}

// THE LONGSHOT — a huge .50 in blackened steel: a skeleton stock, a fluted
// barrel with a violet strip, a muzzle brake, a big scope whose lenses glow
// violet and whose side carries the cells. Its scope axis is LONG_SCOPE_Y.
inline void buildLongshot(Builder& b, const GunLook& L, const GunPose& P) {
    const mat4 base(1.f);   // no offset: the scope axis must stay at LONG_SCOPE_Y
    const vec3 acc = b.accent(L);
    float boost = 0.4f + 0.6f * P.boot;
    b.box(base, {0.f, -0.030f, -0.200f}, {0.050f, 0.050f, 0.20f}, BLACK);          // stock
    b.box(base, {0.f, -0.090f, -0.250f}, {0.046f, 0.030f, 0.14f}, BLACK);
    b.box(base, {0.f, 0.020f, -0.210f}, {0.044f, 0.030f, 0.14f}, BONE);            // cheek riser
    b.box(base, {0.f, -0.040f, -0.315f}, {0.056f, 0.140f, 0.030f}, STEEL);
    b.box(base, {0.f, 0.012f, 0.020f}, {0.066f, 0.066f, 0.25f}, BLACK);            // receiver
    b.box(base, {0.f, -0.085f, -0.060f}, {0.040f, 0.110f, 0.050f}, BLACK);         // grip
    for (float sx : {-1.f, 1.f}) b.box(base, {sx * 0.021f, -0.085f, -0.060f}, {0.003f, 0.095f, 0.04f}, BONE);
    // the magazine: out and dropped (OPEN), a new one up (FEED)
    float magDrop = 0.f;
    if (P.reloadU >= 0.f) {
        float u = P.reloadU;
        magDrop = u < 0.25f ? smooth(seg(u, 0.05f, 0.22f)) : 1.f - smooth(seg(u, 0.25f, 0.45f));
    }
    b.box(base, {0.f, -0.070f - magDrop * 0.18f, 0.060f}, {0.046f, 0.090f, 0.070f}, STEEL);
    b.box(base, {0.f, 0.026f, 0.440f}, {0.034f, 0.034f, 0.62f}, BLACK);            // fluted barrel
    b.box(base, {0.f, 0.026f, 0.300f}, {0.040f, 0.012f, 0.30f}, STEEL);
    b.lit(acc);
    b.box(base, {0.f, 0.044f, 0.30f}, {0.006f, 0.003f, 0.30f}, b.glow);           // its strip
    b.unlit();
    b.box(base, {0.f, 0.026f, 0.770f}, {0.062f, 0.046f, 0.075f}, STEEL);           // muzzle brake
    b.box(base, {0.f, 0.026f, 0.770f}, {0.070f, 0.016f, 0.030f}, BLACK);
    b.box(base, {-0.016f, -0.012f, 0.380f}, {0.010f, 0.010f, 0.24f}, EDGE);        // folded bipod
    b.box(base, { 0.016f, -0.012f, 0.380f}, {0.010f, 0.010f, 0.24f}, EDGE);
    b.box(base, { 0.f, 0.000f, 0.260f}, {0.050f, 0.018f, 0.030f}, BLACK);
    // the scope: rings, tube, bells, turrets, glowing lenses, the cells on its side
    b.box(base, {0.f, 0.058f, -0.020f}, {0.030f, 0.030f, 0.022f}, BLACK);
    b.box(base, {0.f, 0.058f,  0.110f}, {0.030f, 0.030f, 0.022f}, BLACK);
    b.box(base, {0.f, LONG_SCOPE_Y, 0.040f}, {0.044f, 0.044f, 0.30f}, STEEL);
    b.box(base, {0.f, LONG_SCOPE_Y, 0.215f}, {0.066f, 0.066f, 0.070f}, STEEL);
    b.box(base, {0.f, LONG_SCOPE_Y, -0.135f}, {0.056f, 0.056f, 0.060f}, STEEL);
    b.box(base, {0.f, LONG_SCOPE_Y + 0.030f, 0.045f}, {0.022f, 0.022f, 0.026f}, EDGE);
    b.box(base, {0.030f, LONG_SCOPE_Y, 0.045f}, {0.022f, 0.022f, 0.026f}, EDGE);
    b.cells(base, {-0.0235f, LONG_SCOPE_Y, -0.04f}, {0.f, 0.f, 0.028f}, {0.f, -0.012f, 0.f}, 8,
            {0.004f, 0.01f, 0.02f}, std::max(1, L.mag), L.lit, boost);
    b.lit(b.glow * 0.4f);
    b.box(base, {0.f, LONG_SCOPE_Y, 0.251f}, {0.054f, 0.054f, 0.004f}, BLACK);
    b.box(base, {0.f, LONG_SCOPE_Y, -0.166f}, {0.044f, 0.044f, 0.004f}, BLACK);
    b.unlit();
    float lift, pull;
    boltState(P, lift, pull);
    bolt(b, base, {0.034f, 0.025f, -0.060f}, 0.055f, lift, pull, STEEL);
    b.muzzleFlash(base, {0.f, 0.026f, 0.84f}, 0.07f, L.flash);
}
} // namespace gunkit_legacy
