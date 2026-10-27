#pragma once
// =============================================================================
// GunKit.h — the arsenal's look: one body for every gun (blackened gunmetal,
// worn steel, bone grips, a little brass), one glow colour per gun, ammo cells
// you can read on the gun. Each gun's shape is a file (assets/guns/*.glb, made
// in Blender: GunModel.h, GunFile.h); here are the rigs that move it and the
// extras code still draws on it. No OpenGL: buildGun() returns boxes in gun
// space (+X right, +Y up, +Z along the barrel) that ViewModel draws, so tests
// can count cells and check framing.
// =============================================================================
#include "render/GunMotion.h"
#include "render/GunModel.h"
#include "render/GunFile.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <cmath>
#include <algorithm>

struct GunPart { glm::mat4 xf; glm::vec3 color, emissive; int tag; };   // tag 1: lit cell, 2: dark cell
struct GunLook { int ammo = 8, mag = 8, lit = 8; float flash = 0.f, heat = 0.f, cylSpin = 0.f; int refill = 1; float aim = 0.f;
                 float breath = 0.5f; };   // 0..1: the revolver's slow breath (its glyphs and vents), quicker as it heats
// OATHBREAKER's look (the sword, Sword.h): its guard meter on the fuller, dark
// when broken, brighter charging a thrust or holding a riposte
struct SwordLook { float guard01 = 1.f; bool broken = false; float charge = 0.f; float riposte = 0.f; };
// Where the sword is in camera space (x right, y up, z forward): the sword
// hand on the grip, the way the blade points, the way its edge faces, and the
// off hand (on the grip below it, or bracing the flat in a guard)
struct SwordPose {
    glm::vec3 hand{0.2f, -0.22f, 0.36f};
    glm::vec3 blade{0.f, 0.f, 1.f};
    glm::vec3 edge{0.f, -1.f, 0.f};
    float twoHand = 0.f;   // 0..1: the off hand comes up onto the grip
    float brace = 0.f;     // 0..1: ...or onto the flat of the blade (the guard)
};

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

// A place on a rigged gun: a joint, and a transform within it. Boxes placed
// from one of these ride that joint (and, recorded, keep it in the file).
struct JM { int j; mat4 rel; };
inline JM operator*(const JM& a, const mat4& m) { return {a.j, a.rel * m}; }

struct Builder {
    std::vector<GunPart>& out;
    vec3 glow;
    vec3 emi{0.f};
    Builder(std::vector<GunPart>& o, vec3 g) : out(o), glow(g) {}
    // ---- rigged guns (GunModel.h): this frame's rig, where each joint is ----
    const GunRig* rig = nullptr;
    GunModel* rec = nullptr;          // recording a gun's shape into a model (the exporter)
    mat4 W[J_COUNT], F[J_COUNT];      // each joint now / at rest, in gun space
    int chan = -1; vec3 k{0.f}; bool hasPhase = false; float phase = 0.f;
    int flag = -1; bool want = true;

    void box(const mat4& base, vec3 c, vec3 size, vec3 col) { out.push_back({base * T(c) * glm::scale(mat4(1.f), size), col, emi, 0}); }
    void lit(vec3 e) { emi = e; }
    void unlit() { emi = vec3{0.f}; }

    void rigged(const GunRig& r) { rig = &r; for (int j = 0; j < J_COUNT; ++j) W[j] = F[j] = mat4(1.f); }
    // A joint at rest at `frame` (gun space), riding `parent`, moved by the rig
    void joint(int j, int parent, const mat4& frame) {
        F[j] = frame;
        W[j] = W[parent] * glm::inverse(F[parent]) * frame * rig->motion[j];
        if (rec) rec->joints.push_back({j, parent, frame});
    }
    JM at(int j) const { return {j, glm::inverse(F[j])}; }   // a joint, placing in gun space (at rest)
    JM in(int j) const { return {j, mat4(1.f)}; }            // a joint, placing in its own frame
    void glowOn(int c, vec3 kk) { chan = c; k = kk; hasPhase = false; }
    void glowOn(int c, float kk) { glowOn(c, vec3{kk}); }
    void pulse(float ph) { hasPhase = true; phase = ph; }
    void glowOff() { chan = -1; k = vec3{0.f}; hasPhase = false; }
    void when(int f, bool w) { flag = f; want = w; }
    void always() { flag = -1; want = true; }
    void box(const JM& b, vec3 c, vec3 size, vec3 col) {
        ModelPart p;
        p.joint = b.j; p.rel = b.rel * T(c) * glm::scale(mat4(1.f), size); p.color = col;
        p.chan = chan; p.k = k; p.hasPhase = hasPhase; p.phase = phase; p.flag = flag; p.want = want;
        if (rec) {
            float s = glm::dot(col, glow) / glm::dot(glow, glow);   // a shade of the glow: kept as one
            if (s > 0.f && glm::length(col - glow * s) < 1e-6f) p.glowScale = s;
            rec->parts.push_back(p);
        }
        place(p);
    }
    // A model's part, posed by this frame's rig
    void place(const ModelPart& p) {
        if (p.flag >= 0 && rig->flag[p.flag] != p.want) return;
        out.push_back({W[p.joint] * p.rel, p.glowScale >= 0.f ? glow * p.glowScale : p.color, emission(*rig, p), 0});
    }

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
    void muzzleFlash(const mat4& base, vec3 at, float size, float flash) {
        if (flash <= 0.f) return;
        vec3 hot = glm::mix(glow, vec3{1.f}, 0.45f);
        lit(glow * flash * 1.8f);
        box(base, at, vec3{size, size, size * 1.4f} * (0.6f + 0.4f * flash), hot);
        box(base, at + vec3{0.f, 0.f, size * 0.6f}, vec3{size * 0.45f, size * 0.45f, size * 1.6f} * flash, hot);
        unlit();
    }
};

// The accent strip: brighter the fuller the gun, flaring as it fires
inline vec3 accentOf(vec3 glow, const GunLook& L) {
    float fill = L.mag > 0 ? (float)L.ammo / L.mag : 0.f;
    float k = L.ammo == 0 ? 0.15f : 0.35f + 0.65f * fill;
    return glow * (k + 1.5f * L.flash);
}
} // namespace gunkit

// =============================================================================
// The arsenal. Each gun is three pieces:
//   rig     (here) how it moves and glows this frame: GunLook/GunPose in,
//           joint motions, channel strengths and flags out
//   shape   (assets/guns/<name>.glb) its boxes, on their joints
//   extras  (here) what code still draws on it: the ammo cells (one per
//           round, whatever the mag), the muzzle flash, the brass, the
//           speedloader, the shells and clips of a reload
// Gun space: +X right, +Y up, +Z along the barrel; the origin is where the
// shared hip puts it. Reload timings follow GunMotion's beats: OPEN 0-0.25,
// FEED 0.25-0.85 (the cells relight), CLOSE 0.85-1.
// =============================================================================
namespace gunkit {
// The arsenal's files: the four guns, and (GUN_SWORD) OATHBREAKER, the sword
constexpr int GUN_SWORD = 4, GUN_FILES = 5;
inline const char* gunName(int g) {
    static const char* N[GUN_FILES] = {"revolver", "shotgun", "lancer", "longshot", "sword"};
    return N[std::clamp(g, 0, GUN_FILES - 1)];
}

// The rifles' bolt: rotated up, drawn back (both 0..1)
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
inline mat4 boltMotion(const GunPose& P) {
    float lift, pull;
    boltState(P, lift, pull);
    return T({0.f, 0.f, -pull * 0.07f}) * glm::rotate(mat4(1.f), lift * 1.4f, vec3{0.f, 0.f, 1.f});
}

// ---- the rigs -----------------------------------------------------------------
// THE REVOLVER breathes: its glyphs and vents glow and fade on a slow breath
// (GunLook::breath), quicker and hotter as you fan it, dimmer as the cylinder
// empties. The crane swings out to reload, the cylinder spins, the hammer
// falls as it fires, the heat gauge's needle climbs.
inline GunRig rigRevolver(vec3 glow, const GunLook& L, const GunPose& P) {
    GunRig r;
    float u = P.reloadU;
    bool  rl = u >= 0.f;
    float boost = 0.4f + 0.6f * P.boot;
    float swing = rl ? smooth(seg(u, 0.03f, 0.12f)) * (1.f - smooth(seg(u, 0.86f, 0.92f))) : 0.f;
    float spin  = rl ? smooth(seg(u, 0.86f, 0.97f)) * 540.f : 0.f;
    float fill  = L.mag > 0 ? (float)L.ammo / L.mag : 0.f;
    float br = 0.18f + 0.82f * L.breath * L.breath;
    vec3 acc = accentOf(glow, L);
    r.motion[J_CRANE]    = R(swing * 82.f, {0.f, 0.f, 1.f});
    r.motion[J_CYLINDER] = R(L.cylSpin + spin, {0.f, 0.f, 1.f});
    r.motion[J_HAMMER]   = R(P.fireU * 28.f, {1.f, 0.f, 0.f});
    r.motion[J_TRIGGER]  = T({0.f, 0.f, P.fireU * 0.004f});
    r.motion[J_NEEDLE]   = R(-120.f * L.heat, {1.f, 0.f, 0.f});
    r.chan[C_GLOW]   = glow;
    r.chan[C_ACCENT] = acc;
    r.chan[C_BREATH] = glow * ((0.25f + 0.75f * fill) * br * (0.7f + 0.9f * L.heat) * boost + 2.0f * L.flash);
    r.chan[C_STRIPE] = acc * (0.5f + L.heat * 0.8f);
    r.chan[C_RIM]    = acc * (0.6f + L.heat);
    r.chan[C_NEEDLE] = glow * (0.6f + 1.4f * L.heat);
    r.chan[C_PORT]   = glow * (0.15f + 1.6f * L.heat + 2.5f * L.flash);
    r.wave = L.breath * 3.14159f;   // the breath runs along the glyphs
    r.flag[F_LOADED] = L.ammo > 0;
    r.flag[F_BIGMAG] = std::max(1, L.mag) > 8;
    return r;
}
// THE SHOTGUN: the pump slides back after a shot and on a reload's CLOSE
inline GunRig rigShotgun(vec3 glow, const GunLook& L, const GunPose& P) {
    GunRig r;
    float slide = 0.f;
    if (P.cycleU >= 0.f) slide = -0.08f * std::sin(P.cycleU * 3.14159265f);
    if (P.reloadU >= 0.f) slide = -0.08f * std::sin(seg(P.reloadU, 0.86f, 0.98f) * 3.14159265f);
    r.motion[J_PUMP] = T({0.f, 0.f, slide});
    r.chan[C_GLOW] = glow; r.chan[C_ACCENT] = accentOf(glow, L);
    r.flag[F_LOADED] = L.ammo > 0; r.flag[F_BIGMAG] = std::max(1, L.mag) > 8;
    return r;
}
// THE LANCER and THE LONGSHOT: the bolt lifts and draws back; the Longshot's
// magazine drops out (OPEN) and a new one goes up (FEED)
inline GunRig rigRifle(vec3 glow, const GunLook& L, const GunPose& P, bool longshot) {
    GunRig r;
    r.motion[J_BOLT] = boltMotion(P);
    if (longshot && P.reloadU >= 0.f) {
        float u = P.reloadU;
        float magDrop = u < 0.25f ? smooth(seg(u, 0.05f, 0.22f)) : 1.f - smooth(seg(u, 0.25f, 0.45f));
        r.motion[J_MAG] = T({0.f, -magDrop * 0.18f, 0.f});
    }
    r.chan[C_GLOW] = glow; r.chan[C_ACCENT] = accentOf(glow, L);
    r.flag[F_LOADED] = L.ammo > 0; r.flag[F_BIGMAG] = std::max(1, L.mag) > 8;
    return r;
}
inline GunRig rigOf(int gun, const GunLook& L, const GunPose& P) {
    vec3 glow = glowOf(gun);
    switch (gun) {
        case 0: return rigRevolver(glow, L, P);
        case 1: return rigShotgun(glow, L, P);
        case 2: return rigRifle(glow, L, P, false);
        default: return rigRifle(glow, L, P, true);
    }
}
// The joints and sockets each gun's file must have (the rig moves them; the extras hang on them)
inline std::vector<int> jointsNeeded(int gun) {
    switch (gun) {
        case 0: return {J_CRANE, J_CYLINDER, J_HAMMER, J_TRIGGER, J_NEEDLE, J_MUZZLE, J_CELLS};
        case 1: return {J_PUMP, J_MUZZLE, J_CELLS};
        case 2: return {J_BOLT, J_MUZZLE, J_CELLS};
        case GUN_SWORD: return {J_CELLS};
        default: return {J_BOLT, J_MAG, J_MUZZLE, J_CELLS};
    }
}

// ---- the extras ---------------------------------------------------------------
inline void extrasRevolver(Builder& b, const GunLook& L, const GunPose& P) {
    const float CY = 0.018f, CZ = 0.036f;
    float u = P.reloadU;
    bool  rl = u >= 0.f;
    float boost = 0.4f + 0.6f * P.boot;
    const mat4 crane = b.W[J_CRANE] * glm::inverse(b.F[J_CRANE]);   // gun space as the crane has it
    const mat4 cm = b.W[J_CYLINDER];
    // The cells: slots round the cylinder's side for the first eight rounds;
    // any beyond (an upgraded cylinder) in rows of four at the cells socket,
    // on the frame's left side plate, the side you see
    int n = std::max(1, L.mag), ring1 = std::min(n, 8);
    for (int k = 0; k < ring1; ++k) {
        mat4 f = cm * R(k * 360.f / ring1, {0.f, 0.f, 1.f});
        b.cell(f, {0.f, 0.0313f, -0.006f}, {0.011f, 0.002f, 0.044f}, k < L.lit, boost);
    }
    if (n > 8)
        b.cells(b.W[J_CELLS], {0.f, 0.f, 0.f}, {0.f, 0.f, 0.011f}, {0.f, -0.012f, 0.f}, 4,
                {0.002f, 0.008f, 0.008f}, n - 8, std::max(0, L.lit - 8), boost);
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
    const mat4 mz = b.W[J_MUZZLE];
    b.muzzleFlash(mz, {0.f, 0.f, 0.f}, 0.05f, L.flash);
    if (L.flash > 0.f) {   // the brake throws its jets sideways and up
        b.lit(b.glow * L.flash * 1.5f);
        for (float sx : {-1.f, 1.f}) b.box(mz, {sx * (0.034f + 0.02f * L.flash), -0.002f, -0.036f}, {0.03f * L.flash, 0.008f, 0.01f}, glm::mix(b.glow, vec3{1.f}, 0.5f));
        b.box(mz, {0.f, 0.040f + L.flash * 0.02f, -0.036f}, {0.012f, 0.03f * L.flash, 0.012f}, glm::mix(b.glow, vec3{1.f}, 0.5f));
        b.unlit();
    }
}
inline void extrasShotgun(Builder& b, const GunLook& L, const GunPose& P) {
    const mat4 base(1.f);
    float boost = 0.4f + 0.6f * P.boot;
    // the shell window on top of the receiver (the side you see): a cell per shell
    b.cells(b.W[J_CELLS], {0.f, 0.f, 0.f}, {0.f, 0.f, 0.024f}, {0.014f, 0.f, 0.f}, 5,
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
    b.muzzleFlash(b.W[J_MUZZLE], {0.f, 0.f, 0.f}, 0.05f, L.flash);
}
inline void extrasLancer(Builder& b, const GunLook& L, const GunPose& P) {
    const mat4 base(1.f);
    float boost = 0.4f + 0.6f * P.boot;
    b.cells(b.W[J_CELLS], {0.f, 0.f, 0.f}, {0.f, 0.f, 0.022f}, {0.f, -0.012f, 0.f}, 8,   // the left side: the one you see
            {0.004f, 0.008f, 0.016f}, std::max(1, L.mag), L.lit, boost);
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
    b.muzzleFlash(b.W[J_MUZZLE], {0.f, 0.f, 0.f}, 0.05f, L.flash);
}
inline void extrasLongshot(Builder& b, const GunLook& L, const GunPose& P) {
    float boost = 0.4f + 0.6f * P.boot;
    b.cells(b.W[J_CELLS], {0.f, 0.f, 0.f}, {0.f, 0.f, 0.028f}, {0.f, -0.012f, 0.f}, 8,
            {0.004f, 0.01f, 0.02f}, std::max(1, L.mag), L.lit, boost);
    b.muzzleFlash(b.W[J_MUZZLE], {0.f, 0.f, 0.f}, 0.07f, L.flash);
}
inline void extrasOf(int gun, Builder& b, const GunLook& L, const GunPose& P) {
    switch (gun) {
        case 0: extrasRevolver(b, L, P); break;
        case 1: extrasShotgun(b, L, P);  break;
        case 2: extrasLancer(b, L, P);   break;
        default: extrasLongshot(b, L, P); break;
    }
}

// ---- the guns, from their files -------------------------------------------------
inline std::string gunPath(int gun) { return std::string("assets/guns/") + gunName(gun) + ".glb"; }
inline GunModel& gunModelSlot(int gun) { static GunModel M[GUN_FILES]; return M[std::clamp(gun, 0, GUN_FILES - 1)]; }
// (Re)load a gun's file; false (and the model's error says why) if it couldn't be.
// A gun whose file is missing or broken draws only its extras.
inline bool loadGunModel(int gun, const std::string& path) {
    GunModel m = readGunFile(path);
    if (m.error.empty())
        for (int j : jointsNeeded(gun))
            if (!m.has(j)) { m.error = path + ": no '" + jointName(j) + "' (the " + gunName(gun) + "'s rig needs it)"; break; }
    if (!m.error.empty()) std::fprintf(stderr, "gun: %s\n", m.error.c_str());
    bool ok = m.error.empty();
    if (ok || gunModelSlot(gun).parts.empty()) gunModelSlot(gun) = std::move(m);   // a broken save keeps the last good one
    else gunModelSlot(gun).error = m.error;
    return ok;
}
inline const GunModel& gunModel(int gun) {
    static bool loaded[GUN_FILES] = {};
    int g = std::clamp(gun, 0, GUN_FILES - 1);
    if (!loaded[g]) { loaded[g] = true; loadGunModel(g, gunPath(g)); }
    return gunModelSlot(g);
}
// Pose a model with this frame's rig, then hang the extras on it
inline void buildGunFrom(const GunModel& m, int gun, const GunLook& L, const GunPose& P, std::vector<GunPart>& out) {
    Builder b{out, glowOf(gun)};
    GunRig r = rigOf(gun, L, P);
    b.rigged(r);
    for (const auto& J : m.joints) b.joint(J.id, J.parent, J.frame);
    for (const auto& p : m.parts) b.place(p);
    extrasOf(gun, b, L, P);
}
inline void buildGun(int gun, const GunLook& L, const GunPose& P, std::vector<GunPart>& out) {
    buildGunFrom(gunModel(gun), gun, L, P, out);
}
// Where each of a model's joints is this frame, in gun space (the viewer marks them)
inline void jointWorlds(const GunModel& m, const GunRig& r, mat4 W[J_COUNT]) {
    std::vector<GunPart> sink;
    Builder b{sink, vec3{1.f}};
    b.rigged(r);
    for (const auto& J : m.joints) b.joint(J.id, J.parent, J.frame);
    for (int j = 0; j < J_COUNT; ++j) W[j] = b.W[j];
}

// ---- the gauntlet and the arm (recipes), the sword (a file) ----


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

// The parrying gauntlet, over the fist the arm closes (buildArm): a blackened shell, a steel
// knuckle-duster with four bone studs, curled plated fingers, the thumb across them, a plate over
// the back of the hand with a rune line, a cuff with a ring of light. flare: 0..1, the studs and
// runes blazing as it lands a parry. Frame: +Z the punch, +Y the back of the hand.
inline void buildGauntlet(Builder& b, const mat4& f, vec3 glow, float flare) {
    b.box(f, {0.f, 0.f, 0.f}, {0.085f, 0.075f, 0.08f}, BLACK * 1.1f);                   // the shell
    b.box(f, {0.f, -0.022f, 0.036f}, {0.08f, 0.03f, 0.036f}, BLACK * 1.3f);             // curled fingers
    for (int k = 0; k < 4; ++k)                                                          // their plates
        b.box(f, {-0.03f + 0.02f * k, -0.006f, 0.05f}, {0.017f, 0.012f, 0.016f}, STEEL * 0.85f);
    b.box(f, {0.f, 0.012f, 0.046f}, {0.092f, 0.046f, 0.02f}, STEEL);                     // the knuckle-duster
    b.lit(glow * (0.7f + 3.5f * flare) + vec3{1.f, 0.9f, 0.7f} * (2.f * flare));
    for (int k = 0; k < 4; ++k)                                                          // bone studs, lit
        b.box(f, {-0.03f + 0.02f * k, 0.016f, 0.059f}, {0.015f, 0.02f, 0.01f}, BONE);
    b.unlit();
    b.box(f, {0.04f, -0.012f, 0.024f}, {0.02f, 0.026f, 0.05f}, BLACK * 1.3f);            // the thumb across them
    b.box(f, {0.f, 0.041f, -0.004f}, {0.076f, 0.012f, 0.07f}, STEEL * 0.9f);             // the back-of-hand plate
    b.lit(glow * (1.f + 3.f * flare));
    b.box(f, {0.f, 0.048f, -0.004f}, {0.012f, 0.004f, 0.06f}, glow);                     // its rune line
    b.box(f, {0.f, 0.f, -0.056f}, {0.094f, 0.089f, 0.008f}, glow);                       // the cuff's ring
    b.unlit();
    b.box(f, {0.f, 0.f, -0.068f}, {0.09f, 0.085f, 0.022f}, STEEL);                       // the cuff
}

// OATHBREAKER (Sword.h): its shape is a file like the guns' (assets/guns/sword.glb,
// made in Blender); the rig lights it, and code draws the fuller's 8 cells (the
// guard meter, dark when it breaks) at its "cells" socket.
// Sword space: +Z along the blade from the guard, the flats facing +-Y (+Y the
// side you see at rest), the edges along +-X.
const vec3 SWORD_GLOW{1.f, 0.25f, 0.45f};
constexpr float SWORD_SCALE = 0.55f;              // sword units to camera space (a 1 m blade, framed like the guns)
constexpr float SWORD_HAND_Z = -0.055f, SWORD_OFFHAND_Z = -0.165f;   // where the hands close on the grip (sword units)
inline GunRig rigSword(const SwordLook& L) {
    GunRig r;
    const float boost = 1.f + 0.8f * L.charge + 1.2f * L.riposte;
    r.chan[C_GLOW]   = SWORD_GLOW * boost;
    r.chan[C_GEM]    = SWORD_GLOW * (L.broken ? 0.2f : 1.2f * boost);
    r.chan[C_CHARGE] = SWORD_GLOW * (L.broken ? 0.05f : 0.25f + 1.4f * L.charge + 2.2f * L.riposte);
    r.flag[F_BROKEN] = L.broken;
    r.flag[F_LOADED] = !L.broken;
    return r;
}
inline void buildSword(const SwordLook& L, std::vector<GunPart>& out) {
    Builder b{out, SWORD_GLOW};
    GunRig r = rigSword(L);
    b.rigged(r);
    const GunModel& m = gunModel(GUN_SWORD);
    for (const auto& J : m.joints) b.joint(J.id, J.parent, J.frame);
    for (const auto& p : m.parts) b.place(p);
    const float boost = 1.f + 0.8f * L.charge + 1.2f * L.riposte;
    int on = L.broken ? 0 : (int)std::ceil(std::clamp(L.guard01, 0.f, 1.f) * 8.f - 1e-4f);
    b.cells(b.W[J_CELLS], {0.f, 0.f, 0.f}, {0.f, 0.f, 0.09f}, {0.f, 0.f, 0.f}, 8, {0.018f, 0.006f, 0.066f}, 8, on, boost);
}

// The sword's frame in camera space: origin at the crossguard, +Z down the
// blade, -Y the edge (the edge kept square to the blade)
inline mat4 swordFrame(const SwordPose& P) {
    vec3 z = glm::normalize(P.blade);
    vec3 e = P.edge - z * glm::dot(P.edge, z);
    if (glm::length(e) < 1e-4f) e = std::fabs(z.y) < 0.9f ? vec3{0.f, -1.f, 0.f} : vec3{0.f, 0.f, 1.f};
    vec3 y = -glm::normalize(e - z * glm::dot(e, z));
    vec3 x = glm::cross(y, z);
    mat4 m(1.f);
    m[0] = glm::vec4(x, 0.f); m[1] = glm::vec4(y, 0.f); m[2] = glm::vec4(z, 0.f);
    m[3] = glm::vec4(P.hand - z * (SWORD_HAND_Z * SWORD_SCALE), 1.f);   // the hand closes round the grip
    return m;
}
// Where its point is (camera space)
inline vec3 swordTip(const SwordPose& P) {
    return vec3(swordFrame(P) * glm::vec4(0.f, 0.f, 0.94f * SWORD_SCALE, 1.f));
}

inline SwordPose mixPose(const SwordPose& a, const SwordPose& b, float t) {
    SwordPose o;
    o.hand = glm::mix(a.hand, b.hand, t);
    vec3 bl = glm::mix(a.blade, b.blade, t), ed = glm::mix(a.edge, b.edge, t);
    o.blade = glm::length(bl) > 1e-4f ? glm::normalize(bl) : b.blade;
    o.edge  = glm::length(ed) > 1e-4f ? glm::normalize(ed) : b.edge;
    o.twoHand = a.twoHand + (b.twoHand - a.twoHand) * t;
    o.brace   = a.brace + (b.brace - a.brace) * t;
    return o;
}

// A stroke as keyframes: a short wind-up (the cut lands as you click: Sword.h),
// the cut itself fast and easing out, the follow-through settling
struct SwordKey { float u; vec3 hand, blade, edge; float twoHand; };
inline SwordPose keyed(const SwordKey* k, int n, float u) {
    auto pose = [](const SwordKey& q) { SwordPose p; p.hand = q.hand; p.blade = glm::normalize(q.blade); p.edge = glm::normalize(q.edge); p.twoHand = q.twoHand; return p; };
    if (u <= k[0].u) return pose(k[0]);
    for (int i = 0; i + 1 < n; ++i) {
        if (u > k[i + 1].u) continue;
        float t = (u - k[i].u) / (k[i + 1].u - k[i].u);
        t = i == 0 ? smooth(t) : 1.f - (1.f - t) * (1.f - t) * (1.f - t);   // wind-up eased; the cut and after, fast then slowing
        // (two steps of nlerp: a quarter turn between keys stays even)
        SwordPose a = pose(k[i]), b = pose(k[i + 1]);
        return mixPose(a, b, t);
    }
    return pose(k[n - 1]);
}
// The rest pose: low right, the point up and in, a slow breath on it
inline SwordPose swordRest() { return SwordPose{{0.2f, -0.19f, 0.35f}, glm::normalize(vec3{-0.3f, 0.85f, 0.45f}), glm::normalize(vec3{-0.6f, 0.f, 0.8f}), 0.f, 0.f}; }

// Where the sword is through a stroke: u 0..1 through it, or -1 at rest;
// raised: across the middle, edge out, the off hand braced on the flat
inline SwordPose swordPose(int stroke, float u, bool raised, float charge01, float thrust01) {
    SwordPose rest = swordRest();
    if (raised) return SwordPose{{0.1f, -0.13f, 0.4f}, glm::normalize(vec3{-1.f, 0.2f, 0.12f}), glm::normalize(vec3{0.f, 0.25f, 1.f}), 0.f, 1.f};
    if (thrust01 > 0.f)   // driven straight out, both hands behind it
        return SwordPose{{0.05f, -0.12f, 0.36f + 0.2f * thrust01}, glm::normalize(vec3{-0.12f, 0.07f, 1.f}), {0.f, -1.f, 0.f}, 1.f, 0.f};
    if (u < 0.f) {
        if (charge01 <= 0.f) return rest;
        SwordPose cocked{{0.17f, -0.15f, 0.2f}, glm::normalize(vec3{-0.22f, 0.12f, 1.f}), {0.f, -1.f, 0.1f}, 1.f, 0.f};   // drawn back, levelled
        return mixPose(rest, cocked, smooth(std::clamp(charge01, 0.f, 1.f)));
    }
    u = std::clamp(u, 0.f, 1.f);
    if (stroke == 0) {   // forehand: cocked high over the right shoulder, cutting down and across to the left
        static const SwordKey K[] = {
            {0.f,   {0.28f, -0.06f, 0.3f},   {0.7f, 0.6f, -0.2f},    {-0.3f, 0.f, 1.f},   0.f},
            {0.1f,  {0.31f, -0.02f, 0.28f},  {0.8f, 0.55f, -0.3f},   {-0.1f, 0.f, 1.f},   0.f},
            {0.28f, {0.17f, -0.06f, 0.36f},  {0.75f, 0.35f, 0.55f},  {-0.6f, -0.2f, 0.8f}, 0.f},
            {0.4f,  {0.02f, -0.1f, 0.4f},    {0.05f, 0.2f, 0.98f},   {-1.f, -0.2f, 0.1f}, 0.f},
            {0.55f, {-0.15f, -0.14f, 0.38f}, {-0.78f, -0.05f, 0.62f}, {-0.6f, -0.1f, -0.8f}, 0.f},
            {0.75f, {-0.24f, -0.17f, 0.34f}, {-0.95f, -0.2f, 0.22f}, {-0.2f, -0.1f, -1.f}, 0.f},
            {1.f,   {-0.2f, -0.2f, 0.33f},   {-0.8f, -0.4f, 0.45f},  {-0.4f, -0.2f, -1.f}, 0.f}};
        return keyed(K, 7, u);
    }
    if (stroke == 1) {   // backhand: from low left, rising across to the right
        static const SwordKey K[] = {
            {0.f,   {-0.2f, -0.2f, 0.32f},   {-0.85f, 0.f, -0.1f},   {0.2f, 0.f, 1.f},    0.f},
            {0.1f,  {-0.24f, -0.18f, 0.3f},  {-0.9f, 0.05f, -0.3f},  {0.1f, 0.1f, 1.f},   0.f},
            {0.28f, {-0.14f, -0.15f, 0.36f}, {-0.75f, 0.12f, 0.55f}, {0.6f, 0.15f, 0.8f}, 0.f},
            {0.4f,  {0.f, -0.13f, 0.4f},     {-0.05f, 0.22f, 0.98f}, {1.f, 0.15f, 0.1f},  0.f},
            {0.55f, {0.16f, -0.11f, 0.38f},  {0.78f, 0.3f, 0.55f},   {0.6f, 0.15f, -0.8f}, 0.f},
            {0.75f, {0.25f, -0.1f, 0.34f},   {0.93f, 0.35f, 0.1f},   {0.2f, 0.1f, -1.f},  0.f},
            {1.f,   {0.23f, -0.15f, 0.34f},  {0.7f, 0.5f, 0.4f},     {0.4f, 0.f, -1.f},   0.f}};
        return keyed(K, 7, u);
    }
    // the overhead: both hands, raised high, brought down in front
    static const SwordKey K[] = {
        {0.f,  {0.08f, 0.09f, 0.28f},  {0.08f, 0.97f, -0.2f}, {0.f, 0.2f, 1.f},   1.f},
        {0.1f, {0.06f, 0.08f, 0.27f},  {0.05f, 0.93f, -0.36f}, {0.f, 0.36f, 0.93f}, 1.f},
        {0.3f, {0.04f, 0.0f, 0.44f},   {0.02f, 0.55f, 0.83f}, {0.f, 0.83f, -0.55f}, 1.f},
        {0.5f, {0.03f, -0.12f, 0.48f}, {0.f, -0.1f, 1.f},     {0.f, -1.f, -0.1f}, 1.f},
        {0.7f, {0.03f, -0.22f, 0.45f}, {0.f, -0.55f, 0.83f},  {0.f, -0.83f, -0.55f}, 1.f},
        {1.f,  {0.05f, -0.25f, 0.4f},  {0.04f, -0.6f, 0.8f},  {0.f, -0.8f, -0.6f}, 1.f}};
    return keyed(K, 6, u);
}

// ---- the arms that hold it ----
// A box from a to b (camera space), w wide, h deep, `up` roughly its top
inline void limb(Builder& b, vec3 a, vec3 c, float w, float h, vec3 up, vec3 col, float pad = 0.f) {
    vec3 z = c - a; float len = glm::length(z);
    if (len < 1e-4f) return;
    z /= len;
    vec3 x = glm::cross(up, z);
    if (glm::length(x) < 1e-4f) x = glm::cross(vec3{1.f, 0.f, 0.f}, z);
    x = glm::normalize(x);
    vec3 y = glm::cross(z, x);
    mat4 m(1.f);
    m[0] = glm::vec4(x, 0.f); m[1] = glm::vec4(y, 0.f); m[2] = glm::vec4(z, 0.f); m[3] = glm::vec4((a + c) * 0.5f, 1.f);
    b.box(m, {0.f, 0.f, 0.f}, {w, h, len + pad}, col);
}
// Two bones from the shoulder to the wrist, the elbow bent toward `pole`
inline vec3 elbowOf(vec3 sh, vec3 wr, float a, float c, vec3 pole) {
    vec3 d = wr - sh; float l = glm::length(d);
    l = std::clamp(l, std::fabs(a - c) + 1e-3f, a + c - 1e-3f);
    vec3 dir = glm::normalize(d);
    float x = (a * a - c * c + l * l) / (2.f * l), h = std::sqrt(std::max(0.f, a * a - x * x));
    vec3 p = pole - dir * glm::dot(pole, dir);
    p = glm::length(p) > 1e-4f ? glm::normalize(p) : vec3{0.f, -1.f, 0.f};
    return sh + dir * x + p * h;
}
const vec3 SLEEVE{0.13f, 0.045f, 0.06f};   // a dark crimson coat
// One arm: coat sleeve to the elbow, a blackened gauntlet with steel bands
// and a seam glowing in the sword's colour, the gloved hand closed on the grip
// (or flat on the blade: open). side: +1 right, -1 left. `fist` frames the
// hand: +Z along the grip, -Y the edge (as the sword's frame).
inline void buildArm(Builder& b, vec3 shoulder, const mat4& fist, float side, bool open, float glow) {
    const float UPPER = 0.4f, FORE = 0.36f;
    vec3 hand = vec3(fist[3]);
    vec3 gx = vec3(fist[0]), gy = vec3(fist[1]);
    vec3 wrist = hand - gx * (0.034f * side) - gy * 0.006f;   // the wrist sits beside the grip, the forearm's side
    wrist += glm::normalize(shoulder - wrist) * 0.02f;
    vec3 elbow = elbowOf(shoulder, wrist, UPPER, FORE, glm::normalize(vec3{side * 1.f, -1.3f, -0.3f}));
    vec3 up{0.f, 1.f, 0.f};
    limb(b, shoulder, elbow, 0.075f, 0.075f, up, SLEEVE, 0.03f);                        // the sleeve
    limb(b, elbow, glm::mix(elbow, wrist, 0.3f), 0.07f, 0.068f, up, SLEEVE * 1.2f);   // its cuff, turned back
    b.box(T(elbow) * glm::scale(mat4(1.f), vec3{1.f}), {0.f, 0.f, 0.f}, {0.055f, 0.055f, 0.055f}, STEEL * 0.8f);   // the elbow cop
    limb(b, glm::mix(elbow, wrist, 0.3f), wrist, 0.05f, 0.046f, up, BLACK * 1.3f);   // the gauntlet's vambrace
    for (float t : {0.42f, 0.7f, 0.94f})
        limb(b, glm::mix(elbow, wrist, t - 0.03f), glm::mix(elbow, wrist, t + 0.03f), 0.056f, 0.052f, up, STEEL);   // its bands
    {   // the seam along its top
        vec3 a = glm::mix(elbow, wrist, 0.4f), c = glm::mix(elbow, wrist, 0.92f);
        vec3 z = glm::normalize(c - a), x = glm::normalize(glm::cross(up, z)), y = glm::cross(z, x);
        b.lit(SWORD_GLOW * glow);
        limb(b, a + y * 0.024f, c + y * 0.024f, 0.008f, 0.004f, up, SWORD_GLOW);
        b.unlit();
    }
    // the hand: framed by the grip
    if (!open) {
        b.box(fist, {0.f, 0.004f, 0.f}, {0.05f, 0.054f, 0.05f}, BLACK * 1.1f);                     // fingers round the grip
        for (int k = 0; k < 4; ++k)                                                               // bone knuckle plates
            b.box(fist, {side * -0.026f, 0.006f, 0.018f - 0.012f * k}, {0.01f, 0.02f, 0.01f}, BONE);
        b.box(fist, {side * 0.02f, 0.022f, 0.016f}, {0.016f, 0.018f, 0.028f}, BLACK * 1.3f);         // the thumb over the top
        b.box(fist, {side * 0.022f, -0.004f, -0.006f}, {0.02f, 0.04f, 0.04f}, BLACK * 1.2f);         // the back of the hand
    } else {
        b.box(fist, {0.f, 0.f, 0.f}, {0.058f, 0.02f, 0.07f}, BLACK * 1.1f);                          // a flat palm
        for (int k = 0; k < 4; ++k) b.box(fist, {-0.021f + 0.014f * k, 0.003f, 0.044f}, {0.011f, 0.016f, 0.022f}, BONE);
    }
}

} // namespace gunkit
