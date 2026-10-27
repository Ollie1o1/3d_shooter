#pragma once
// =============================================================================
// EnemyFile.h — an enemy's shape as a file (assets/enemies/<name>.glb, made in
// Blender), the same box-on-joints format as the guns (GunFile.h).
//
//   joints   named frames the code turns, each where the file puts it (so
//            moving a shoulder in Blender moves the arm that hangs off it).
//            Each rig uses its own (neededJoints):
//              humanoids  hipL hipR torso head armL armR; the rest rides the body (bob, crouch)
//                         (a SHIELDBEARER's shield too: braced, drawn back to bash, knocked aside)
//              RIPPER     body head legFL legFR legBL legBR bladeL bladeR
//              RAPTOR     head tail wingL wingR tipL tipR (tips ride the wings)
//              MITE       body legL1-3 legR1-3
//   glow     a channel the code sets every frame, x the part's k:
//            "glow"  the enemy's glow colour, flaring through a wind-up
//            "shot"  its shot colour, blazing as the shot is about to leave
//            "pulse" a slow throb that flares in a wind-up (a BRUTE's core)
//            "dive"  lit only while it dives (a RAPTOR's talons)
//            "fuse"  a MITE's core: steady, then flashing red as its fuse burns
//            "blade" a RIPPER's blades, brightening as it crouches to lunge
//            "smash" a JUGGERNAUT's hammer fist, lit only while it winds up a smash
//            "rim"   a SHIELDBEARER's shield rim, brightening through a wind-up
//            "const" k itself
//   colours  material base colours; "glow*0.3": the (flaring) glow colour x 0.3
//
// The head hitbox doesn't read the file (headBox, EnemyModel.h): a test holds
// every head box inside it. No OpenGL.
// =============================================================================
#include "render/GunFile.h"
#include "enemies/EnemyTypes.h"
#include <array>
#include <cstdio>
#include <string>
#include <vector>

namespace enemyfile {
using gunkit::GunModel;

enum Joint { E_ROOT, E_HIPL, E_HIPR, E_TORSO, E_HEAD, E_ARML, E_ARMR,
             E_BODY, E_LEG_FL, E_LEG_FR, E_LEG_BL, E_LEG_BR, E_BLADE_L, E_BLADE_R,
             E_TAIL, E_WING_L, E_WING_R, E_TIP_L, E_TIP_R,
             E_LEG_L1, E_LEG_L2, E_LEG_L3, E_LEG_R1, E_LEG_R2, E_LEG_R3, E_SHIELD, E_COUNT };
inline const char* jointName(int j) {
    static const char* N[E_COUNT] = {"root", "hipL", "hipR", "torso", "head", "armL", "armR",
                                     "body", "legFL", "legFR", "legBL", "legBR", "bladeL", "bladeR",
                                     "tail", "wingL", "wingR", "tipL", "tipR",
                                     "legL1", "legL2", "legL3", "legR1", "legR2", "legR3", "shield"};
    return N[j];
}
enum Chan { C_CONST, C_GLOW, C_SHOT, C_PULSE, C_DIVE, C_FUSE, C_BLADE, C_SMASH, C_RIM, C_COUNT };
inline const char* chanName(int c) {
    static const char* N[C_COUNT] = {"const", "glow", "shot", "pulse", "dive", "fuse", "blade", "smash", "rim"};
    return N[c];
}
inline const char* flagName(int) { return ""; }
inline const gunkit::Names& names() { static const gunkit::Names n{E_COUNT, C_COUNT, 0, jointName, chanName, flagName}; return n; }

// The enemies that have a file
inline const char* fileName(EnemyType t) {
    switch (t) {
        case EnemyType::HUSK:     return "husk";
        case EnemyType::SENTINEL: return "sentinel";
        case EnemyType::BRUTE:    return "brute";
        case EnemyType::RIPPER:   return "ripper";
        case EnemyType::RAPTOR:   return "raptor";
        case EnemyType::MITE:     return "mite";
        case EnemyType::JUGGERNAUT:   return "juggernaut";
        case EnemyType::SHIELDBEARER: return "shieldbearer";
        default: return nullptr;
    }
}
// The joints a type's rig turns (a file must have them all)
inline std::vector<int> neededJoints(EnemyType t) {
    switch (t) {
        case EnemyType::RIPPER: return {E_BODY, E_HEAD, E_LEG_FL, E_LEG_FR, E_LEG_BL, E_LEG_BR, E_BLADE_L, E_BLADE_R};
        case EnemyType::RAPTOR: return {E_HEAD, E_TAIL, E_WING_L, E_WING_R, E_TIP_L, E_TIP_R};
        case EnemyType::MITE:   return {E_BODY, E_LEG_L1, E_LEG_L2, E_LEG_L3, E_LEG_R1, E_LEG_R2, E_LEG_R3};
        case EnemyType::SHIELDBEARER: return {E_HIPL, E_HIPR, E_TORSO, E_HEAD, E_ARML, E_ARMR, E_SHIELD};
        default:                return {E_HIPL, E_HIPR, E_TORSO, E_HEAD, E_ARML, E_ARMR};
    }
}
inline std::string path(EnemyType t) { const char* n = fileName(t); return n ? std::string("assets/enemies/") + n + ".glb" : std::string(); }

inline GunModel& slot(EnemyType t) { static std::array<GunModel, (size_t)EnemyType::COUNT> M; return M[(size_t)t]; }
// (Re)load an enemy's file; false (and the model's error says why) if it couldn't be.
// A broken save keeps the last good model; with none, the enemy is drawn from code.
inline bool load(EnemyType t, const std::string& p) {
    GunModel m = gunkit::readGunFile(p, names());
    if (m.error.empty())
        for (int j : neededJoints(t))
            if (!m.has(j)) { m.error = p + ": no '" + jointName(j) + "' joint"; break; }
    if (!m.error.empty()) std::fprintf(stderr, "enemy: %s\n", m.error.c_str());
    bool ok = m.error.empty();
    if (ok || slot(t).parts.empty()) slot(t) = std::move(m);
    else slot(t).error = m.error;
    return ok;
}
// The model for a type: nullptr if it has no file (or never loaded one)
inline const GunModel* model(EnemyType t) {
    if (!fileName(t)) return nullptr;
    static std::array<bool, (size_t)EnemyType::COUNT> tried{};
    if (!tried[(size_t)t]) { tried[(size_t)t] = true; load(t, path(t)); }
    const GunModel& m = slot(t);
    return m.parts.empty() ? nullptr : &m;
}

// Each joint at rest relative to its parent joint (from the file)
inline void restLocals(const GunModel& m, glm::mat4 local[E_COUNT], int parent[E_COUNT]) {
    glm::mat4 rest[E_COUNT];
    for (int j = 0; j < E_COUNT; ++j) { rest[j] = local[j] = glm::mat4(1.f); parent[j] = E_ROOT; }
    for (const auto& J : m.joints) { rest[J.id] = J.frame; parent[J.id] = J.parent; }
    for (int j = 1; j < E_COUNT; ++j) local[j] = glm::inverse(rest[parent[j]]) * rest[j];
}

} // namespace enemyfile
