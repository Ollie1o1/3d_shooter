#pragma once
// =============================================================================
// EnemyFile.h — an enemy's shape as a file (assets/enemies/<name>.glb, made in
// Blender), the same box-on-joints format as the guns (GunFile.h).
//
//   joints   hipL hipR torso head armL armR: the humanoid rig's frames. Where
//            a joint sits in the file is where the code turns it (the hips
//            swing, the torso sways and turns, the arms swing, aim and splay),
//            so moving a shoulder in Blender moves the arm that hangs off it.
//            Everything not under a joint rides the body (it bobs and crouches).
//   glow     "glow"  the enemy's glow colour, flaring through a wind-up
//            "shot"  its shot colour, blazing as the shot is about to leave
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

namespace enemyfile {
using gunkit::GunModel;

enum Joint { E_ROOT, E_HIPL, E_HIPR, E_TORSO, E_HEAD, E_ARML, E_ARMR, E_COUNT };
inline const char* jointName(int j) {
    static const char* N[E_COUNT] = {"body", "hipL", "hipR", "torso", "head", "armL", "armR"};
    return N[j];
}
enum Chan { C_CONST, C_GLOW, C_SHOT, C_COUNT };
inline const char* chanName(int c) {
    static const char* N[C_COUNT] = {"const", "glow", "shot"};
    return N[c];
}
inline const char* flagName(int) { return ""; }
inline const gunkit::Names& names() { static const gunkit::Names n{E_COUNT, C_COUNT, 0, jointName, chanName, flagName}; return n; }

// The enemies that have a file
inline const char* fileName(EnemyType t) {
    switch (t) {
        case EnemyType::HUSK: return "husk";
        default: return nullptr;
    }
}
inline std::string path(EnemyType t) { const char* n = fileName(t); return n ? std::string("assets/enemies/") + n + ".glb" : std::string(); }

inline GunModel& slot(EnemyType t) { static std::array<GunModel, (size_t)EnemyType::COUNT> M; return M[(size_t)t]; }
// (Re)load an enemy's file; false (and the model's error says why) if it couldn't be.
// A broken save keeps the last good model; with none, the enemy is drawn from code.
inline bool load(EnemyType t, const std::string& p) {
    GunModel m = gunkit::readGunFile(p, names());
    if (m.error.empty())
        for (int j = E_HIPL; j < E_COUNT; ++j)
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
