#pragma once
// =============================================================================
// GunModel.h — a gun's shape as data (assets/guns/*.glb, made in Blender),
// split from how it behaves (the rig, in GunKit.h).
//
//   joints   named frames the rig moves: the revolver's crane swings, its
//            cylinder spins, the shotgun's pump slides... A joint that never
//            moves is a socket: where code hangs things on the gun (the
//            muzzle flash, the ammo cells).
//   parts    boxes, each riding one joint. A part may glow on a channel (its
//            strength set by the rig every frame: breath, heat, accent...)
//            and may show only while a flag holds ("loaded", "!bigmag").
//
// The rig, for each frame, gives every joint a motion (in the joint's own
// frame), every channel a colour and every flag a value; pose() turns the
// model into the boxes ViewModel draws. No OpenGL.
// =============================================================================
#include <glm/glm.hpp>
#include <string>
#include <vector>
#include <cstring>
#include <cmath>

namespace gunkit {

// The joints a rig knows. ROOT is the gun itself (never named in a file).
enum Joint { J_ROOT, J_CRANE, J_CYLINDER, J_HAMMER, J_TRIGGER, J_NEEDLE, J_PUMP, J_BOLT, J_MAG,
             J_MUZZLE, J_CELLS, J_COUNT };
inline const char* jointName(int j) {
    static const char* N[J_COUNT] = {"root", "crane", "cylinder", "hammer", "trigger", "needle", "pump", "bolt", "mag",
                                     "muzzle", "cells"};
    return N[j];
}
// What a glowing part glows with (its emissive = channel x the part's k):
//   const    k itself, a fixed colour        glow    the gun's glow colour
//   accent   brighter the fuller the gun, flaring as it fires
//   breath   the revolver's slow breath      stripe  its heat stripe
//   rim      its cylinder's rims             needle  its heat gauge's needle
//   port     its brake's ports (heat and flash)
enum Chan { C_CONST, C_GLOW, C_ACCENT, C_BREATH, C_STRIPE, C_RIM, C_NEEDLE, C_PORT, C_COUNT };
inline const char* chanName(int c) {
    static const char* N[C_COUNT] = {"const", "glow", "accent", "breath", "stripe", "rim", "needle", "port"};
    return N[c];
}
// Flags a part can show on: "loaded" (a round left), "bigmag" (an upgraded mag past eight)
enum Flag { F_LOADED, F_BIGMAG, F_COUNT };
inline const char* flagName(int f) {
    static const char* N[F_COUNT] = {"loaded", "bigmag"};
    return N[f];
}
template <int N>
inline int lookup(const char* (*name)(int), const std::string& s) {
    for (int i = 0; i < N; ++i) if (s == name(i)) return i;
    return -1;
}

// One frame's rig state
struct GunRig {
    glm::mat4 motion[J_COUNT];   // each joint's move, in its own frame (identity: at rest)
    glm::vec3 chan[C_COUNT];
    bool      flag[F_COUNT] = {};
    float     wave = 0.f;        // the phase a part's `phase` runs on (the revolver's glyphs)
    GunRig() { for (auto& m : motion) m = glm::mat4(1.f); for (auto& c : chan) c = glm::vec3(0.f); chan[C_CONST] = glm::vec3(1.f); }
};

struct ModelJoint {
    int id, parent;              // Joint ids
    glm::mat4 frame;             // where it is at rest, in gun space
};
struct ModelPart {
    int joint = J_ROOT;
    glm::mat4 rel{1.f};          // the unit box (-0.5..0.5) into the joint's frame
    glm::vec3 color{0.5f};
    float glowScale = -1.f;      // >= 0: its colour is the gun's glow x this (material "glow*0.5")
    int chan = -1;               // glows on this channel (-1: doesn't)
    glm::vec3 k{0.f};
    bool hasPhase = false;       // its glow pulses: x (0.75 + 0.25 sin(wave + phase))
    float phase = 0.f;
    int flag = -1;               // shown only while flag == want (-1: always)
    bool want = true;
};
struct GunModel {
    std::vector<ModelJoint> joints;   // parents before children
    std::vector<ModelPart> parts;
    std::string error;                // why it couldn't be loaded ("" if it was)
    bool has(int j) const { if (j == J_ROOT) return true; for (auto& J : joints) if (J.id == j) return true; return false; }
};

inline glm::vec3 emission(const GunRig& r, const ModelPart& p) {
    if (p.chan < 0) return glm::vec3(0.f);
    glm::vec3 e = r.chan[p.chan] * p.k;
    if (p.hasPhase) e *= 0.75f + 0.25f * std::sin(r.wave + p.phase);
    return e;
}

} // namespace gunkit
