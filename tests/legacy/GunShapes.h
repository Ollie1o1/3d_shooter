#pragma once
// TEMPORARY (gun-files migration): the four guns' shapes as C++, split from the
// legacy recipes (GunKitLegacy.h) - every box on its joint, glow channel and
// flag. tests/export_guns.cpp records them into assets/guns/*.glb; the
// migration test proves rig + file + extras draw exactly what the recipes did.
#include "render/GunKit.h"

namespace gunkit {

// The rifles' bolt: a lever on the bolt joint, its knob
inline void boltShape(Builder& b, float len, vec3 knob) {
    JM m = b.in(J_BOLT);
    b.box(m, {len * 0.5f, 0.f, 0.f}, {len, 0.011f, 0.011f}, STEEL);
    b.box(m, {len, -0.004f, 0.f}, {0.016f, 0.016f, 0.016f}, knob);
}

inline void shapeRevolver(Builder& b) {
    const float CY = 0.018f, CZ = 0.036f;
    const float FTOP = 0.051f;
    const vec3 pivot{-0.024f, -0.022f, CZ};
    b.joint(J_CRANE, J_ROOT, T(pivot));
    b.joint(J_CYLINDER, J_CRANE, T({0.f, CY, CZ}));
    b.joint(J_HAMMER, J_ROOT, T({0.f, 0.040f, -0.068f}) * R(-28.f, {1.f, 0.f, 0.f}));
    b.joint(J_TRIGGER, J_ROOT, T({0.f, -0.042f, 0.f}));
    b.joint(J_NEEDLE, J_ROOT, T(vec3{-0.0225f, 0.010f, 0.104f} + vec3{-0.0018f, 0.f, 0.f}) * R(60.f, {1.f, 0.f, 0.f}));
    b.joint(J_MUZZLE, J_ROOT, T({0.f, 0.026f, 0.392f}));
    b.joint(J_CELLS, J_ROOT, T({-0.0285f, 0.018f, -0.054f}));
    const JM base = b.in(J_ROOT);

    // ---- the frame behind the cylinder: slab sides, bolts, levers ----
    b.box(base, {0.f, 0.008f, -0.036f}, {0.052f, 0.086f, 0.064f}, BLACK);
    b.box(base, {0.f, 0.050f, -0.036f}, {0.044f, 0.004f, 0.062f}, EDGE * 0.8f);
    for (float sx : {-1.f, 1.f}) {
        b.box(base, {sx * 0.0262f, 0.006f, -0.036f}, {0.002f, 0.066f, 0.056f}, STEEL * 0.8f);
        b.glowOn(C_STRIPE, 1.f);
        b.box(base, {sx * 0.0275f, 0.044f, -0.036f}, {0.001f, 0.003f, 0.054f}, b.glow);
        b.glowOff();
        for (vec3 at : {vec3{0.f, 0.034f, -0.062f}, vec3{0.f, 0.034f, -0.010f}, vec3{0.f, -0.022f, -0.062f}}) {
            b.box(base, at + vec3{sx * 0.0275f, 0.f, 0.f}, {0.002f, 0.006f, 0.006f}, BRASS);
            b.box(base, at + vec3{sx * 0.0283f, 0.f, 0.f}, {0.001f, 0.0025f, 0.0025f}, BRASS * 0.5f);
        }
    }
    {
        const float FZ0 = -0.068f, FZ1 = -0.004f;
        for (float sx : {-1.f, 1.f}) {
            float x = sx * 0.0274f;
            b.box(base, {x, 0.010f, -0.036f}, {0.001f, 0.032f, 0.044f}, BLACK * 0.55f);
            b.box(base, {x + sx * 0.0004f, 0.0265f, -0.036f}, {0.001f, 0.0015f, 0.046f}, EDGE * 0.9f);
            b.box(base, {x + sx * 0.0004f, -0.0065f, -0.036f}, {0.001f, 0.0015f, 0.046f}, EDGE * 0.6f);
            for (float z : {-0.0585f, -0.0135f}) b.box(base, {x + sx * 0.0004f, 0.010f, z}, {0.001f, 0.034f, 0.0015f}, EDGE * 0.75f);
            // the vent grille and seal: on the left, only while an upgraded mag's cells aren't there
            if (sx < 0.f) b.when(F_BIGMAG, false);
            for (int k = 0; k < 5; ++k) {
                float z = -0.052f + k * 0.0075f;
                b.box(base, {x + sx * 0.0003f, 0.012f, z}, {0.001f, 0.022f, 0.0035f}, BLACK * 0.25f);
                b.glowOn(C_BREATH, 0.55f + 0.15f * k);
                b.box(base, {x + sx * 0.0006f, 0.012f, z}, {0.0005f, 0.016f, 0.0012f}, b.glow * 0.5f);
                b.glowOff();
            }
            vec3 c{x + sx * 0.0003f, 0.012f, -0.0215f};
            b.box(base, c, {0.001f, 0.011f, 0.011f}, STEEL * 0.9f);
            b.box(base * T(c) * R(45.f, {1.f, 0.f, 0.f}), {}, {0.001f, 0.011f, 0.011f}, STEEL * 0.9f);
            b.box(base, c + vec3{sx * 0.0004f, 0.f, 0.f}, {0.001f, 0.008f, 0.008f}, BLACK * 0.4f);
            b.glowOn(C_BREATH, 1.f);
            b.box(base, c + vec3{sx * 0.0008f, 0.f, 0.f}, {0.0005f, 0.0062f, 0.0011f}, b.glow * 0.7f);
            b.box(base, c + vec3{sx * 0.0008f, 0.0016f, 0.0016f}, {0.0005f, 0.0011f, 0.0034f}, b.glow * 0.7f);
            b.box(base, c + vec3{sx * 0.0008f, -0.0016f, -0.0016f}, {0.0005f, 0.0011f, 0.0034f}, b.glow * 0.7f);
            b.glowOff();
            b.always();
            b.box(base, {x, -0.019f, -0.044f}, {0.001f, 0.009f, 0.026f}, STEEL);
            for (int r = 0; r < 2; ++r)
                for (int k = 0; k < 9; ++k)
                    if ((k * 7 + r * 3) % 5 != 0)
                        b.box(base, {x + sx * 0.0005f, -0.0168f - r * 0.0042f, -0.0545f + k * 0.0026f}, {0.0005f, 0.0018f, 0.0012f}, BLACK * 0.6f);
            for (float z : {FZ0 + 0.001f, FZ1 - 0.001f})
                b.box(base * T({sx * 0.0255f, 0.008f, z}) * R(45.f, {0.f, 1.f, 0.f}), {}, {0.003f, 0.084f, 0.003f}, EDGE * 0.7f);
        }
        b.box(base, {-0.0285f, 0.004f, -0.0075f}, {0.003f, 0.010f, 0.008f}, STEEL);
        for (int k = 0; k < 4; ++k) b.box(base, {-0.0302f, 0.0005f + k * 0.0024f, -0.0075f}, {0.0006f, 0.001f, 0.0085f}, EDGE);
        b.box(base, {-0.0245f, -0.031f, 0.010f}, {0.003f, 0.003f, 0.11f}, BRASS * 0.85f);
        for (float z : {-0.040f, 0.010f, 0.058f}) b.box(base, {-0.0247f, -0.031f, z}, {0.004f, 0.005f, 0.004f}, STEEL);
        b.box(base, {-0.0245f, -0.031f, -0.046f}, {0.004f, 0.004f, 0.004f}, BRASS);
        b.box(base, {0.f, 0.002f, FZ0 - 0.0005f}, {0.040f, 0.060f, 0.001f}, STEEL * 0.7f);
        for (float sx : {-1.f, 1.f}) {
            b.box(base, {sx * 0.015f, -0.022f, FZ0 - 0.0012f}, {0.005f, 0.005f, 0.001f}, BRASS);
            b.box(base, {sx * 0.015f, -0.022f, FZ0 - 0.0018f}, {0.0045f, 0.001f, 0.0005f}, BRASS * 0.4f);
            for (int k = 0; k < 4; ++k) b.box(base, {sx * 0.0155f, 0.008f + k * 0.0055f, FZ0 - 0.0012f}, {0.008f, 0.0015f, 0.001f}, BLACK * 0.5f);
        }
        b.box(base, {0.f, 0.000f, FZ0 - 0.0012f}, {0.005f, 0.026f, 0.001f}, BLACK * 0.3f);
        b.glowOn(C_BREATH, 0.9f);
        b.box(base, {0.f, 0.000f, FZ0 - 0.0018f}, {0.0018f, 0.020f, 0.0005f}, b.glow * 0.6f);
        b.glowOff();
        for (int k = 0; k < 2; ++k) b.box(base, {0.f, 0.0515f, -0.046f + k * 0.018f}, {0.020f, 0.002f, 0.006f}, BLACK * 0.6f);
        b.glowOn(C_GLOW, 1.1f);
        for (float sx : {-1.f, 1.f}) b.box(base, {sx * 0.008f, FTOP + 0.0125f, -0.0645f}, {0.0025f, 0.0025f, 0.001f}, b.glow);
        b.glowOff();
        // a loaded-chamber pin that stands up while there's a round left
        b.when(F_LOADED, true);
        b.box(base, {0.f, FTOP + (0.004f + 0.004f) * 0.5f - 0.001f, -0.010f}, {0.004f, 0.004f + 0.004f, 0.004f}, BRASS);
        b.when(F_LOADED, false);
        b.box(base, {0.f, FTOP + 0.004f * 0.5f - 0.001f, -0.010f}, {0.004f, 0.004f, 0.004f}, STEEL * 0.6f);
        b.always();
        for (int k = 0; k < 4; ++k) b.box(base * T({0.f, CY, -0.0035f}) * R(k * 45.f, {0.f, 0.f, 1.f}), {}, {0.024f, 0.058f, 0.002f}, STEEL * 0.75f);
        b.box(base, {0.f, CY, -0.0045f}, {0.006f, 0.006f, 0.002f}, EDGE);
    }
    b.box(base, {0.0275f, -0.004f, -0.020f}, {0.003f, 0.012f, 0.016f}, STEEL);
    b.box(base, {0.0285f, 0.002f, -0.020f}, {0.002f, 0.004f, 0.020f}, EDGE);
    b.box(base, {-0.0272f, 0.050f, -0.066f}, {0.004f, 0.006f, 0.010f}, STEEL);
    b.box(base, {0.f, 0.056f, CZ}, {0.040f, 0.014f, 0.082f}, BLACK);
    b.box(base, {0.f, 0.0635f, CZ}, {0.024f, 0.002f, 0.080f}, EDGE * 0.8f);
    b.box(base, {0.f, -0.026f, CZ}, {0.046f, 0.020f, 0.080f}, BLACK);
    b.box(base, {0.f, 0.022f, 0.078f}, {0.046f, 0.070f, 0.014f}, BLACK);
    b.box(base, {0.f, 0.022f, 0.0715f}, {0.050f, 0.074f, 0.002f}, STEEL * 0.6f);
    b.box(base, {0.f, FTOP + 0.002f, -0.058f}, {0.024f, 0.004f, 0.014f}, STEEL * 0.7f);
    for (float sx : {-1.f, 1.f}) b.box(base, {sx * 0.008f, FTOP + 0.010f, -0.058f}, {0.007f, 0.012f, 0.012f}, BLACK);

    // ---- the barrel ----
    const float BZ = 0.21f, BL = 0.25f;
    b.box(base, {0.f, 0.010f, BZ}, {0.044f, 0.040f, BL}, BLACK);
    b.box(base, {0.f, 0.042f, BZ}, {0.030f, 0.026f, BL}, BLACK);
    for (float sx : {-1.f, 1.f}) {
        JM bev = base * T({sx * 0.0185f, 0.0315f, BZ}) * R(sx * 38.f, {0.f, 0.f, 1.f});
        b.box(bev, {}, {0.014f, 0.004f, BL}, STEEL * 0.9f);
        b.box(base, {sx * 0.0222f, 0.029f, BZ}, {0.001f, 0.002f, BL - 0.01f}, EDGE);
    }
    b.box(base, {0.f, 0.0585f, BZ}, {0.020f, 0.005f, BL - 0.01f}, STEEL);
    for (int k = 0; k < 10; ++k) b.box(base, {0.f, 0.0605f, 0.100f + k * 0.024f}, {0.022f, 0.002f, 0.009f}, BLACK);
    for (float sx : {-1.f, 1.f})
        for (int k = 0; k < 3; ++k) {
            float z = 0.150f + k * 0.052f;
            b.box(base, {sx * 0.0221f, 0.013f, z}, {0.002f, 0.014f, 0.040f}, BLACK * 0.3f);
            b.glowOn(C_BREATH, 0.8f);
            b.box(base, {sx * 0.0226f, 0.013f, z}, {0.001f, 0.004f, 0.030f}, b.glow * 0.6f);
            b.glowOff();
            for (int f = -1; f <= 1; f += 2) b.box(base, {sx * 0.0226f, 0.013f + f * 0.0045f, z}, {0.001f, 0.001f, 0.032f}, BLACK * 0.5f);
        }
    static const float GLYPH[6][3][4] = {
        {{0, 0, 7, 1}, {3, 2, 1, 5}, {-3, 2, 1, 5}},
        {{0, -2, 7, 1}, {0, 2, 7, 1}, {0, 0, 1, 5}},
        {{3, 0, 1, 6}, {0, 0, 1, 6}, {-3, 0, 1, 6}},
        {{0, 0, 7, 1}, {3, 2, 1, 4}, {0, 2, 1, 4}},
        {{2, -1, 3, 1}, {-2, 1, 3, 1}, {0, 0, 1, 6}},
        {{0, 0, 1, 6}, {2, 0, 3, 1}, {-2, 0, 3, 1}}};
    for (float sx : {-1.f, 1.f})
        for (int g = 0; g < 6; ++g) {
            float z0 = 0.118f + g * 0.011f + (g >= 3 ? 0.006f : 0.f);
            b.glowOn(C_BREATH, 1.f); b.pulse(g * 0.6f);
            for (auto& s : GLYPH[g]) b.box(base, {sx * 0.0223f, -0.003f + s[0] * 0.0008f, z0 + s[1] * 0.0008f},
                                           {0.001f, s[2] * 0.0008f + 0.0006f, s[3] * 0.0008f + 0.0006f}, b.glow * 0.7f);
            b.glowOff();
        }
    {   // the heat gauge
        vec3 c{-0.0225f, 0.010f, 0.104f};
        b.box(base, c, {0.002f, 0.018f, 0.018f}, EDGE);
        b.box(base * T(c) * R(45.f, {1.f, 0.f, 0.f}), {}, {0.002f, 0.018f, 0.018f}, EDGE);
        b.box(base, c + vec3{-0.0008f, 0.f, 0.f}, {0.002f, 0.015f, 0.015f}, BLACK * 0.4f);
        for (int t = 0; t < 5; ++t) {
            float a = glm::radians(-60.f + t * 30.f);
            b.box(base, c + vec3{-0.0016f, std::cos(a) * 0.006f, std::sin(a) * 0.006f}, {0.001f, 0.0012f, 0.0012f}, t == 4 ? vec3{0.6f, 0.15f, 0.1f} : BONE);
        }
        b.glowOn(C_NEEDLE, 1.f);
        b.box(b.in(J_NEEDLE), {0.f, 0.0028f, 0.f}, {0.001f, 0.0056f, 0.0008f}, b.glow);
        b.glowOff();
        b.box(base, c + vec3{-0.0019f, 0.f, 0.f}, {0.001f, 0.0018f, 0.0018f}, BRASS);
    }
    b.box(base, {0.f, -0.018f, BZ + 0.005f}, {0.034f, 0.018f, BL - 0.02f}, BLACK);
    b.box(base, {0.f, -0.0275f, BZ + 0.005f}, {0.028f, 0.002f, BL - 0.03f}, STEEL * 0.7f);
    for (float sx : {-1.f, 1.f}) {
        for (int k = 0; k < 14; ++k) {
            JM st = base * T({sx * 0.0172f, -0.018f, 0.130f + k * 0.009f}) * R(40.f, {1.f, 0.f, 0.f});
            if (k % 2) { b.glowOn(C_GLOW, 0.25f); b.box(st, {}, {0.001f, 0.012f, 0.003f}, b.glow * 0.5f); b.glowOff(); }
            else b.box(st, {}, {0.001f, 0.012f, 0.003f}, BLACK * 0.5f);
        }
        for (int k = 0; k < 3; ++k) b.box(base, {sx * 0.0173f, -0.018f, 0.272f + k * 0.018f}, {0.002f, 0.004f, 0.004f}, BRASS);
    }
    const float MZ = 0.356f;
    b.box(base, {0.f, 0.020f, MZ}, {0.048f, 0.068f, 0.042f}, BLACK);
    b.box(base, {0.f, 0.020f, MZ + 0.0215f}, {0.040f, 0.060f, 0.002f}, STEEL * 0.7f);
    b.box(base, {0.f, 0.026f, MZ + 0.0222f}, {0.016f, 0.016f, 0.002f}, BLACK * 0.15f);
    for (float sx : {-1.f, 1.f})
        for (int k = 0; k < 3; ++k) {
            b.box(base, {sx * 0.0242f, 0.024f, MZ - 0.012f + k * 0.012f}, {0.002f, 0.042f, 0.006f}, BLACK * 0.2f);
            b.glowOn(C_PORT, 1.f);
            b.box(base, {sx * 0.0236f, 0.024f, MZ - 0.012f + k * 0.012f}, {0.001f, 0.034f, 0.003f}, b.glow * 0.4f);
            b.glowOff();
        }
    for (int k = 0; k < 2; ++k) {
        b.box(base, {0.f, 0.0545f, MZ - 0.006f + k * 0.013f}, {0.030f, 0.002f, 0.006f}, BLACK * 0.2f);
        b.glowOn(C_PORT, 1.f); b.box(base, {0.f, 0.0538f, MZ - 0.006f + k * 0.013f}, {0.024f, 0.001f, 0.003f}, b.glow * 0.4f); b.glowOff();
    }
    b.box(base, {0.f, 0.063f, 0.320f}, {0.006f, 0.012f, 0.020f}, BLACK);
    b.glowOn(C_GLOW, 1.4f);
    b.box(base, {0.f, 0.067f, 0.326f}, {0.004f, 0.004f, 0.004f}, b.glow);
    b.glowOff();

    // ---- hammer ----
    JM hm = b.in(J_HAMMER);
    for (float sx : {-1.f, 1.f}) b.box(hm, {sx * 0.0045f, 0.012f, -0.004f}, {0.003f, 0.024f, 0.010f}, BLACK);
    b.box(hm, {0.f, 0.025f, -0.006f}, {0.012f, 0.005f, 0.014f}, STEEL);
    for (int k = 0; k < 3; ++k) b.box(hm, {0.f, 0.0278f, -0.011f + k * 0.004f}, {0.012f, 0.001f, 0.0015f}, BLACK);

    // ---- the cylinder on its crane ----
    JM crane = b.at(J_CRANE);
    b.box(crane, {-0.012f, -0.022f, CZ + 0.044f}, {0.012f, 0.008f, 0.02f}, STEEL);
    JM cm = b.in(J_CYLINDER);
    for (int k = 0; k < 8; ++k)
        b.box(cm * R(k * 22.5f, {0.f, 0.f, 1.f}), {}, {0.0123f, 0.062f, 0.070f}, k % 2 ? STEEL * 0.85f : STEEL);
    for (int k = 0; k < 8; ++k) {
        JM f = cm * R(22.5f + k * 45.f, {0.f, 0.f, 1.f});
        b.box(f, {0.f, 0.0305f, 0.f}, {0.006f, 0.002f, 0.052f}, BLACK * 0.6f);
    }
    for (float z : {0.034f, -0.034f})
        for (int k = 0; k < 4; ++k) b.box(cm * R(k * 45.f, {0.f, 0.f, 1.f}), {0.f, 0.f, z}, {0.0245f, 0.0595f, 0.003f}, EDGE);
    b.glowOn(C_RIM, 1.f);
    for (int k = 0; k < 4; ++k) b.box(cm * R(k * 45.f, {0.f, 0.f, 1.f}), {0.f, 0.f, 0.03f}, {0.027f, 0.0645f, 0.003f}, b.glow);
    b.glowOff();
    for (int k = 0; k < 8; ++k) {
        float a = glm::radians(90.f + k * 45.f);
        vec3 c{std::cos(a) * 0.021f, std::sin(a) * 0.021f, 0.f};
        for (float z : {0.0365f, -0.0365f}) b.box(cm, c + vec3{0.f, 0.f, z}, {0.011f, 0.011f, 0.003f}, BLACK * 0.5f);
    }
    b.box(crane, {0.f, CY, CZ + 0.05f}, {0.006f, 0.006f, 0.03f}, EDGE);
    b.box(crane, {0.f, CY, CZ}, {0.012f, 0.012f, 0.076f}, EDGE);

    // ---- the guard, the trigger ----
    b.box(base, {0.f, -0.058f, 0.004f}, {0.016f, 0.007f, 0.070f}, BLACK);
    b.box(base, {0.f, -0.042f, 0.037f}, {0.016f, 0.034f, 0.007f}, BLACK);
    b.box(base, {0.f, -0.061f, 0.044f}, {0.016f, 0.010f, 0.012f}, BLACK);
    b.box(base, {0.f, -0.0545f, 0.004f}, {0.010f, 0.001f, 0.060f}, EDGE * 0.7f);
    JM tr = b.at(J_TRIGGER);
    b.box(tr, {0.f, -0.036f, -0.002f}, {0.007f, 0.028f, 0.008f}, EDGE);
    b.box(tr, {0.f, -0.049f, 0.000f}, {0.008f, 0.004f, 0.010f}, EDGE);

    // ---- the grip ----
    JM gm = base * T({0.f, -0.034f, -0.052f}) * R(-20.f, {1.f, 0.f, 0.f});
    b.box(gm, {0.f, -0.056f, -0.004f}, {0.044f, 0.116f, 0.054f}, BLACK);
    b.box(gm, {0.f, 0.006f, -0.034f}, {0.040f, 0.012f, 0.026f}, BLACK);
    for (int k = 0; k < 3; ++k) b.box(gm, {0.f, -0.040f - k * 0.024f, 0.024f}, {0.040f, 0.010f, 0.006f}, BLACK * 1.4f);
    for (float sx : {-1.f, 1.f}) {
        b.box(gm, {sx * 0.0225f, -0.060f, -0.004f}, {0.004f, 0.094f, 0.046f}, BONE);
        for (int r = 0; r < 6; ++r)
            for (int c = 0; c < 3; ++c)
                if ((r + c) % 2 == 0) b.box(gm, {sx * 0.0246f, -0.094f + r * 0.010f, -0.016f + c * 0.012f}, {0.001f, 0.004f, 0.004f}, BONE * 0.62f);
        b.glowOn(C_CONST, BRASS * 0.3f);
        b.box(gm, {sx * 0.0248f, -0.026f, -0.004f}, {0.002f, 0.014f, 0.014f}, BRASS);
        b.glowOff();
        b.glowOn(C_BREATH, 0.6f);
        b.box(gm, {sx * 0.0251f, -0.026f, -0.004f}, {0.001f, 0.006f, 0.002f}, b.glow * 0.5f);
        b.box(gm, {sx * 0.0251f, -0.026f, -0.004f}, {0.001f, 0.002f, 0.006f}, b.glow * 0.5f);
        b.glowOff();
    }
    b.box(gm, {0.f, -0.118f, -0.004f}, {0.050f, 0.014f, 0.062f}, STEEL);
    b.box(gm, {0.f, -0.1255f, -0.004f}, {0.040f, 0.002f, 0.050f}, BLACK);
    b.box(gm, {0.f, -0.122f, -0.036f}, {0.010f, 0.008f, 0.006f}, EDGE);
}

inline void shapeShotgun(Builder& b) {
    b.joint(J_PUMP, J_ROOT, T({0.f, -0.005f, 0.08f}));
    b.joint(J_MUZZLE, J_ROOT, T({0.f, 0.028f, 0.30f}));
    b.joint(J_CELLS, J_ROOT, T({-0.007f, 0.0465f, -0.09f}));
    const JM base = b.in(J_ROOT);
    b.box(base, {0.f, 0.028f, 0.12f}, {0.048f, 0.048f, 0.32f}, STEEL);
    b.glowOn(C_ACCENT, 1.f);
    b.box(base, {0.f, 0.053f, 0.12f}, {0.006f, 0.003f, 0.30f}, b.glow);
    b.glowOff();
    b.box(base, {0.f, -0.018f, 0.14f}, {0.032f, 0.032f, 0.26f}, BLACK);
    b.box(base, {0.f, 0.005f, -0.04f}, {0.065f, 0.075f, 0.14f}, BLACK);
    b.box(base, {0.f, 0.042f, -0.04f}, {0.04f, 0.004f, 0.12f}, EDGE);
    b.box(base, {0.f, 0.044f, -0.04f}, {0.03f, 0.003f, 0.13f}, STEEL);
    JM pump = b.at(J_PUMP);
    b.box(pump, {0.f, -0.005f, 0.08f}, {0.050f, 0.044f, 0.10f}, BONE);
    for (int k = 0; k < 4; ++k) b.box(pump, {0.f, -0.005f, 0.044f + k * 0.024f}, {0.052f, 0.046f, 0.006f}, BONE * 0.7f);
    b.box(base, {-0.036f, 0.004f, -0.04f}, {0.008f, 0.05f, 0.11f}, BLACK);
    for (int k = 0; k < 4; ++k) {
        b.box(base, {-0.044f, 0.004f, -0.08f + k * 0.026f}, {0.012f, 0.044f, 0.02f}, BLACK * 1.6f);
        b.box(base, {-0.044f, -0.022f, -0.08f + k * 0.026f}, {0.013f, 0.009f, 0.021f}, BRASS);
    }
    b.box(base, {0.f, -0.046f, -0.02f}, {0.026f, 0.004f, 0.05f}, {0.05f, 0.05f, 0.05f});
    b.box(base, {0.f, -0.035f, -0.16f}, {0.048f, 0.065f, 0.10f}, BLACK);
    b.box(base, {0.f, -0.060f, -0.22f}, {0.040f, 0.055f, 0.06f}, BLACK);
    b.box(base, {0.f, -0.050f, -0.04f}, {0.020f, 0.022f, 0.070f}, BLACK);
    b.box(base, {0.f, -0.120f, -0.08f}, {0.044f, 0.100f, 0.055f}, BLACK);
    for (float sx : {-1.f, 1.f}) b.box(base, {sx * 0.023f, -0.120f, -0.08f}, {0.003f, 0.085f, 0.045f}, BONE);
}

inline void shapeLancer(Builder& b) {
    b.joint(J_BOLT, J_ROOT, T({0.020f, 0.022f, -0.035f}));
    b.joint(J_MUZZLE, J_ROOT, T({0.f, 0.028f, 0.68f}));
    b.joint(J_CELLS, J_ROOT, T({-0.0215f, 0.026f, -0.03f}));
    const JM base = b.in(J_ROOT);
    b.box(base, {0.f, -0.040f, -0.150f}, {0.050f, 0.066f, 0.30f}, BLACK);
    b.box(base, {0.f, -0.080f, -0.270f}, {0.052f, 0.100f, 0.07f}, BLACK);
    b.box(base, {0.f, -0.082f, -0.308f}, {0.054f, 0.104f, 0.012f}, STEEL);
    b.box(base, {0.f, -0.068f, -0.040f}, {0.040f, 0.070f, 0.07f}, BONE);
    b.box(base, {0.f, -0.012f, -0.170f}, {0.052f, 0.012f, 0.12f}, STEEL);
    b.box(base, {0.f, 0.014f, 0.020f}, {0.040f, 0.044f, 0.17f}, BLACK);
    b.box(base, {0.f, -0.034f, 0.050f}, {0.034f, 0.026f, 0.08f}, BLACK);
    b.box(base, {0.f, -0.056f, -0.005f}, {0.016f, 0.020f, 0.05f}, BLACK);
    b.box(base, {0.f, -0.004f, 0.250f}, {0.044f, 0.040f, 0.32f}, BLACK);
    b.glowOn(C_ACCENT, 1.f);
    b.box(base, {0.0225f, -0.004f, 0.25f}, {0.002f, 0.006f, 0.30f}, b.glow);
    b.glowOff();
    b.box(base, {0.f, 0.004f, 0.200f}, {0.048f, 0.050f, 0.012f}, EDGE);
    b.box(base, {0.f, 0.004f, 0.380f}, {0.048f, 0.050f, 0.012f}, EDGE);
    b.box(base, {0.f, 0.028f, 0.360f}, {0.020f, 0.020f, 0.58f}, STEEL);
    b.box(base, {-0.010f, 0.044f, 0.110f}, {0.008f, 0.024f, 0.012f}, EDGE);
    b.box(base, { 0.010f, 0.044f, 0.110f}, {0.008f, 0.024f, 0.012f}, EDGE);
    b.box(base, { 0.f,    0.036f, 0.110f}, {0.028f, 0.008f, 0.016f}, EDGE);
    b.glowOn(C_GLOW, 0.8f);
    b.box(base, { 0.f, 0.044f, 0.630f}, {0.004f, 0.016f, 0.008f}, glm::mix(b.glow, vec3{1.f}, 0.5f));
    b.glowOff();
    b.box(base, {-0.011f, 0.046f, 0.630f}, {0.004f, 0.024f, 0.014f}, BLACK);
    b.box(base, { 0.011f, 0.046f, 0.630f}, {0.004f, 0.024f, 0.014f}, BLACK);
    boltShape(b, 0.036f, EDGE);
}

inline void shapeLongshot(Builder& b) {
    b.joint(J_MAG, J_ROOT, T({0.f, -0.070f, 0.060f}));
    b.joint(J_BOLT, J_ROOT, T({0.034f, 0.025f, -0.060f}));
    b.joint(J_MUZZLE, J_ROOT, T({0.f, 0.026f, 0.84f}));
    b.joint(J_CELLS, J_ROOT, T({-0.0235f, LONG_SCOPE_Y, -0.04f}));
    const JM base = b.in(J_ROOT);
    b.box(base, {0.f, -0.030f, -0.200f}, {0.050f, 0.050f, 0.20f}, BLACK);
    b.box(base, {0.f, -0.090f, -0.250f}, {0.046f, 0.030f, 0.14f}, BLACK);
    b.box(base, {0.f, 0.020f, -0.210f}, {0.044f, 0.030f, 0.14f}, BONE);
    b.box(base, {0.f, -0.040f, -0.315f}, {0.056f, 0.140f, 0.030f}, STEEL);
    b.box(base, {0.f, 0.012f, 0.020f}, {0.066f, 0.066f, 0.25f}, BLACK);
    b.box(base, {0.f, -0.085f, -0.060f}, {0.040f, 0.110f, 0.050f}, BLACK);
    for (float sx : {-1.f, 1.f}) b.box(base, {sx * 0.021f, -0.085f, -0.060f}, {0.003f, 0.095f, 0.04f}, BONE);
    b.box(b.at(J_MAG), {0.f, -0.070f, 0.060f}, {0.046f, 0.090f, 0.070f}, STEEL);
    b.box(base, {0.f, 0.026f, 0.440f}, {0.034f, 0.034f, 0.62f}, BLACK);
    b.box(base, {0.f, 0.026f, 0.300f}, {0.040f, 0.012f, 0.30f}, STEEL);
    b.glowOn(C_ACCENT, 1.f);
    b.box(base, {0.f, 0.044f, 0.30f}, {0.006f, 0.003f, 0.30f}, b.glow);
    b.glowOff();
    b.box(base, {0.f, 0.026f, 0.770f}, {0.062f, 0.046f, 0.075f}, STEEL);
    b.box(base, {0.f, 0.026f, 0.770f}, {0.070f, 0.016f, 0.030f}, BLACK);
    b.box(base, {-0.016f, -0.012f, 0.380f}, {0.010f, 0.010f, 0.24f}, EDGE);
    b.box(base, { 0.016f, -0.012f, 0.380f}, {0.010f, 0.010f, 0.24f}, EDGE);
    b.box(base, { 0.f, 0.000f, 0.260f}, {0.050f, 0.018f, 0.030f}, BLACK);
    b.box(base, {0.f, 0.058f, -0.020f}, {0.030f, 0.030f, 0.022f}, BLACK);
    b.box(base, {0.f, 0.058f,  0.110f}, {0.030f, 0.030f, 0.022f}, BLACK);
    b.box(base, {0.f, LONG_SCOPE_Y, 0.040f}, {0.044f, 0.044f, 0.30f}, STEEL);
    b.box(base, {0.f, LONG_SCOPE_Y, 0.215f}, {0.066f, 0.066f, 0.070f}, STEEL);
    b.box(base, {0.f, LONG_SCOPE_Y, -0.135f}, {0.056f, 0.056f, 0.060f}, STEEL);
    b.box(base, {0.f, LONG_SCOPE_Y + 0.030f, 0.045f}, {0.022f, 0.022f, 0.026f}, EDGE);
    b.box(base, {0.030f, LONG_SCOPE_Y, 0.045f}, {0.022f, 0.022f, 0.026f}, EDGE);
    b.glowOn(C_GLOW, 0.4f);
    b.box(base, {0.f, LONG_SCOPE_Y, 0.251f}, {0.054f, 0.054f, 0.004f}, BLACK);
    b.box(base, {0.f, LONG_SCOPE_Y, -0.166f}, {0.044f, 0.044f, 0.004f}, BLACK);
    b.glowOff();
    boltShape(b, 0.055f, STEEL);
}

inline void shapeOf(int gun, Builder& b) {
    switch (gun) {
        case 0: shapeRevolver(b); break;
        case 1: shapeShotgun(b);  break;
        case 2: shapeLancer(b);   break;
        default: shapeLongshot(b); break;
    }
}
// The gun as the C++ shapes draw it (the migration's middle step)
inline void buildGunFromShapes(int gun, const GunLook& L, const GunPose& P, std::vector<GunPart>& out) {
    Builder b{out, glowOf(gun)};
    GunRig r = rigOf(gun, L, P);
    b.rigged(r);
    shapeOf(gun, b);
    extrasOf(gun, b, L, P);
}
// Its shape, recorded for the exporter
inline GunModel recordGun(int gun) {
    std::vector<GunPart> sink;
    Builder b{sink, glowOf(gun)};
    GunRig r = rigOf(gun, GunLook{}, GunPose{});
    b.rigged(r);
    GunModel m;
    b.rec = &m;
    shapeOf(gun, b);
    return m;
}
} // namespace gunkit
