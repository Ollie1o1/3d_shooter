#pragma once
// =============================================================================
// GunKit.h — the arsenal's look: one body for every gun (blackened gunmetal,
// worn steel, bone grips, a little brass), one glow colour per gun, ammo cells
// you can read on the gun, and each gun's recipe built from those parts. No
// OpenGL: buildGun() returns boxes in gun space (+X right, +Y up, +Z along the
// barrel) that ViewModel draws, so tests can count cells and check framing.
// =============================================================================
#include "GunMotion.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <cmath>
#include <algorithm>

struct GunPart { glm::mat4 xf; glm::vec3 color, emissive; int tag; };   // tag 1: lit cell, 2: dark cell
struct GunLook { int ammo = 8, mag = 8, lit = 8; float flash = 0.f, heat = 0.f, cylSpin = 0.f; int refill = 1; float aim = 0.f; };

namespace gunkit {
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
} // namespace gunkit

// =============================================================================
// The recipes. Gun space: +X right, +Y up, +Z along the barrel; the origin is
// where the shared hip puts it. Reload timings follow GunMotion's beats:
// OPEN 0-0.25, FEED 0.25-0.85 (the cells relight), CLOSE 0.85-1.
// =============================================================================
namespace gunkit {

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

// THE REVOLVER — a heavy blackened magnum: a worn-steel vent rib over a full
// underlug, a ported compensator, its chambers glowing amber while loaded (the
// cells: one per round, a second ring when upgrades take it past eight), a heat
// stripe that brightens as you fan it, bone grip panels.
inline void buildRevolver(Builder& b, const GunLook& L, const GunPose& P) {
    const mat4 base(1.f);
    const float CY = 0.018f, CZ = 0.036f;
    const vec3 acc = b.accent(L);
    float u = P.reloadU;
    bool  rl = u >= 0.f;
    float boost = 0.4f + 0.6f * P.boot;
    float swing = rl ? smooth(seg(u, 0.03f, 0.12f)) * (1.f - smooth(seg(u, 0.86f, 0.92f))) : 0.f;
    float spin  = rl ? smooth(seg(u, 0.86f, 0.97f)) * 540.f : 0.f;

    // frame
    b.box(base, {0.f, 0.006f, -0.036f}, {0.050f, 0.082f, 0.062f}, BLACK);
    b.box(base, {0.f, -0.026f, CZ},     {0.044f, 0.018f, 0.078f}, BLACK);
    b.box(base, {0.f, 0.050f, 0.030f},  {0.040f, 0.014f, 0.13f},  STEEL);
    b.box(base, {0.f, 0.020f, 0.080f},  {0.044f, 0.064f, 0.012f}, BLACK);
    for (float sx : {-1.f, 1.f}) {
        b.box(base, {sx * 0.0255f, 0.004f, -0.036f}, {0.002f, 0.06f, 0.05f}, STEEL);
        b.lit(acc * (0.6f + L.heat * 0.8f));
        b.box(base, {sx * 0.0268f, 0.03f, -0.036f}, {0.001f, 0.004f, 0.048f}, b.glow);
        b.unlit();
    }
    // barrel: tube, underlug, vent rib, compensator, sights
    b.box(base, {0.f, 0.028f, 0.195f}, {0.030f, 0.030f, 0.23f}, STEEL);
    b.box(base, {0.f, 0.000f, 0.185f}, {0.034f, 0.032f, 0.21f}, BLACK);
    b.box(base, {0.f, 0.048f, 0.195f}, {0.014f, 0.010f, 0.23f}, EDGE);
    for (int k = 0; k < 5; ++k) b.box(base, {0.f, 0.0535f, 0.105f + k * 0.045f}, {0.015f, 0.002f, 0.018f}, BLACK);
    for (float sx : {-1.f, 1.f}) {
        b.lit(acc * (0.5f + L.heat * 1.6f));
        b.box(base, {sx * 0.0172f, 0.0f, 0.185f}, {0.001f, 0.005f, 0.19f}, b.glow);
        b.unlit();
        b.box(base, {sx * 0.0158f, 0.028f, 0.195f}, {0.001f, 0.026f, 0.22f}, EDGE);
    }
    b.box(base, {0.f, 0.020f, 0.326f}, {0.040f, 0.058f, 0.036f}, BLACK);
    for (int k = 0; k < 2; ++k) b.box(base, {0.f, 0.0495f, 0.316f + k * 0.017f}, {0.026f, 0.002f, 0.008f}, STEEL * 0.5f);
    b.lit(b.glow * 1.4f);
    b.box(base, {0.f, 0.062f, 0.322f}, {0.004f, 0.012f, 0.014f}, b.glow);
    b.unlit();
    b.box(base, {-0.009f, 0.062f, -0.052f}, {0.006f, 0.012f, 0.008f}, BLACK);
    b.box(base, { 0.009f, 0.062f, -0.052f}, {0.006f, 0.012f, 0.008f}, BLACK);
    // hammer: falls as it fires
    mat4 hm = base * T({0.f, 0.040f, -0.064f}) * R(-28.f + P.fireU * 28.f, {1.f, 0.f, 0.f});
    b.box(hm, {0.f, 0.012f, -0.004f}, {0.011f, 0.024f, 0.010f}, BLACK);
    b.box(hm, {0.f, 0.024f, -0.012f}, {0.012f, 0.006f, 0.014f}, STEEL);
    // the cylinder on its crane
    vec3 pivot{-0.024f, -0.022f, CZ};
    mat4 crane = base * T(pivot) * R(swing * 82.f, {0.f, 0.f, 1.f}) * T(-pivot);
    b.box(crane, {-0.012f, -0.022f, CZ + 0.044f}, {0.012f, 0.008f, 0.02f}, STEEL);
    mat4 cm = crane * T({0.f, CY, CZ}) * R(L.cylSpin + spin, {0.f, 0.f, 1.f});
    for (int k = 0; k < 4; ++k) b.box(cm * R(k * 45.f, {0.f, 0.f, 1.f}), {}, {0.0257f, 0.062f, 0.070f}, STEEL);
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
        b.cells(base, {-0.0275f, 0.018f, -0.054f}, {0.f, 0.f, 0.011f}, {0.f, -0.012f, 0.f}, 4,
                {0.002f, 0.008f, 0.008f}, n - 8, std::max(0, L.lit - 8), boost);
    for (int k = 0; k < 8; ++k) {   // the chamber faces, front and back (dressing)
        float a = glm::radians(90.f + k * 45.f);
        vec3 c{std::cos(a) * 0.021f, std::sin(a) * 0.021f, 0.f};
        for (float z : {0.0355f, -0.0355f}) b.box(cm, c + vec3{0.f, 0.f, z}, {0.011f, 0.011f, 0.003f}, BLACK * 0.5f);
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
    // trigger, guard, the bone grip
    b.box(base, {0.f, -0.052f, 0.000f}, {0.016f, 0.008f, 0.064f}, BLACK);
    b.box(base, {0.f, -0.040f, 0.030f}, {0.016f, 0.030f, 0.008f}, BLACK);
    b.box(base, {0.f, -0.034f, -0.004f + P.fireU * 0.004f}, {0.007f, 0.026f, 0.008f}, EDGE);
    mat4 gm = base * T({0.f, -0.034f, -0.050f}) * R(-17.f, {1.f, 0.f, 0.f});
    b.box(gm, {0.f, -0.055f, -0.004f}, {0.040f, 0.112f, 0.050f}, BLACK);
    for (float sx : {-1.f, 1.f}) {
        b.box(gm, {sx * 0.0215f, -0.058f, -0.004f}, {0.004f, 0.098f, 0.046f}, BONE);
        for (int r = 0; r < 4; ++r) b.box(gm, {sx * 0.0237f, -0.085f + r * 0.012f, -0.004f}, {0.001f, 0.004f, 0.04f}, BONE * 0.7f);
        b.lit(BRASS * 0.25f);
        b.box(gm, {sx * 0.0238f, -0.032f, -0.004f}, {0.002f, 0.016f, 0.016f}, BRASS);
        b.unlit();
    }
    b.box(gm, {0.f, -0.114f, -0.004f}, {0.044f, 0.012f, 0.054f}, STEEL);
    b.muzzleFlash(base, {0.f, 0.020f, 0.37f}, 0.042f, L.flash);
    if (L.flash > 0.f) {   // compensator jets
        b.lit(b.glow * L.flash * 1.5f);
        for (float sx : {-0.5f, 0.5f})
            b.box(base, {sx * 0.02f, 0.062f + L.flash * 0.02f, 0.325f}, {0.008f, 0.03f * L.flash, 0.012f}, glm::mix(b.glow, vec3{1.f}, 0.5f));
        b.unlit();
    }
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

// The gauntlet that punches: blackened metal, bone knuckles, a seam glowing in
// the gun's colour (blazing on a parry). Fist space: +Z the way it punches.
inline void buildFist(vec3 glow, float punch, bool hit, std::vector<GunPart>& out) {
    Builder b{out, glow};
    const mat4 base(1.f);
    b.box(base, {0.f, -0.008f, -0.15f}, {0.058f, 0.058f, 0.17f}, BLACK * 1.3f);   // forearm
    b.box(base, {0.f, 0.f, -0.065f}, {0.072f, 0.05f, 0.04f}, STEEL);               // cuff
    b.lit(glow * (hit ? 2.f * punch : 0.f));
    b.box(base, {0.f, -0.005f, 0.f}, {0.08f, 0.07f, 0.075f}, BLACK);               // fist
    b.unlit();
    for (int k = 0; k < 4; ++k) b.box(base, {-0.03f + 0.02f * k, 0.022f, 0.042f}, {0.017f, 0.022f, 0.016f}, BONE);
    b.box(base, {0.045f, -0.012f, 0.012f}, {0.02f, 0.03f, 0.045f}, BLACK * 1.3f);  // thumb
    b.lit(glow * (0.8f + (hit ? 2.f * punch : 0.f)));
    b.box(base, {0.f, 0.03f, -0.02f}, {0.06f, 0.004f, 0.09f}, glow);              // the seam
    b.unlit();
}
} // namespace gunkit
