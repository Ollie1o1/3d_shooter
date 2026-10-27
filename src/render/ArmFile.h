#pragma once
// =============================================================================
// ArmFile.h — your arms in first person (both on the sword, the left that
// parries) as a file: assets/guns/arm.glb, made in Blender
// (tools/blender/arm_build.py), the same format as the guns (GunFile.h).
//
//   joints   each framed by the code every frame (the shoulder-elbow-wrist
//            chain buildArm has always worked out):
//              upper  at the shoulder, +Z down the upper arm to the elbow
//              fore   at the elbow, +Z down the forearm to the wrist
//              fist   the hand: +Z along the grip (or the punch), +Y the back
//                     of the hand, +X the thumb's side
//            Parts on upper / fore are laid out for an upper arm UPPER long and
//            a forearm FORE long and stretched to the real length. It's a right
//            arm: the left is the same file, mirrored.
//   glow     "seam" the vambrace's seam; "flare" the gauntlet's studs, "rune"
//            its rune line and cuff ring (both blazing on a parry)
//   show     "open" a flat palm (on the blade, in a guard) / "!open" a closed
//            hand; "gauntlet" the parrying gauntlet / "!gauntlet" without it
//
// With no file (or a broken one) the arms are drawn from code (GunKit.h). No OpenGL.
// =============================================================================
#include "render/GunKit.h"
#include "render/GunFile.h"
#include <string>

namespace armfile {
using glm::vec3; using glm::mat4;

enum Joint { A_ROOT, A_UPPER, A_FORE, A_FIST, A_COUNT };
inline const char* jointName(int j) { static const char* N[A_COUNT] = {"root", "upper", "fore", "fist"}; return N[j]; }
enum Chan { AC_CONST, AC_SEAM, AC_FLARE, AC_RUNE, AC_COUNT };
inline const char* chanName(int c) { static const char* N[AC_COUNT] = {"const", "seam", "flare", "rune"}; return N[c]; }
enum Flag { AF_OPEN, AF_GAUNTLET, AF_COUNT };
inline const char* flagName(int f) { static const char* N[AF_COUNT] = {"open", "gauntlet"}; return N[f]; }
inline const gunkit::Names& names() { static const gunkit::Names n{A_COUNT, AC_COUNT, AF_COUNT, jointName, chanName, flagName}; return n; }

constexpr float UPPER = 0.4f, FORE = 0.36f;   // buildArm's bones
inline const char* path() { return "assets/guns/arm.glb"; }

inline gunkit::GunModel& slot() { static gunkit::GunModel m; return m; }
// (Re)load the file; false (and the model's error says why) if it couldn't be. A broken save keeps the last good one.
inline bool load(const std::string& p = path()) {
    gunkit::GunModel m = gunkit::readGunFile(p, names());
    if (m.error.empty())
        for (int j : {A_UPPER, A_FORE, A_FIST})
            if (!m.has(j)) { m.error = p + ": no '" + jointName(j) + "' joint"; break; }
    bool ok = m.error.empty();
    if (ok || slot().parts.empty()) slot() = std::move(m);
    else slot().error = m.error;
    return ok;
}
inline const gunkit::GunModel* model() {
    static bool tried = false;
    if (!tried) { tried = true; load(); }
    return slot().parts.empty() || !slot().error.empty() ? nullptr : &slot();
}

// How one arm looks this frame
struct ArmLook {
    bool  open = false;        // a flat palm (on the blade)
    bool  gauntlet = false;    // the parrying gauntlet over the hand
    float seam = 0.8f;         // the vambrace seam's strength (x the sword's glow colour)
    vec3  glow{1.f};           // the gauntlet's colour (the gun's, or the sword's)
    float flare = 0.f;         // 0..1: the gauntlet blazing as it lands a parry
};

// A frame at a, +Z toward c (stretched by |c - a| / rest), +Y as near `up` as it can be (limb()'s frame)
inline mat4 along(vec3 a, vec3 c, float rest, vec3 up = {0.f, 1.f, 0.f}) {
    vec3 z = c - a; float len = glm::length(z);
    z = len > 1e-5f ? z / len : vec3{0.f, 0.f, 1.f};
    vec3 x = glm::cross(up, z);
    if (glm::length(x) < 1e-4f) x = glm::cross(vec3{1.f, 0.f, 0.f}, z);
    x = glm::normalize(x);
    vec3 y = glm::cross(z, x);
    mat4 m(1.f);
    m[0] = glm::vec4(x, 0.f); m[1] = glm::vec4(y, 0.f); m[2] = glm::vec4(z * (len / rest), 0.f); m[3] = glm::vec4(a, 1.f);
    return m;
}

// The shoulder-elbow-wrist chain for a hand at `fist` (buildArm's): side +1 right, -1 left
inline void chain(vec3 shoulder, const mat4& fist, float side, vec3& elbow, vec3& wrist) {
    vec3 hand = vec3(fist[3]), gx = vec3(fist[0]), gy = vec3(fist[1]);
    wrist = hand - gx * (0.034f * side) - gy * 0.006f;   // the wrist sits beside the grip, the forearm's side
    wrist += glm::normalize(shoulder - wrist) * 0.02f;
    elbow = gunkit::elbowOf(shoulder, wrist, UPPER, FORE, glm::normalize(vec3{side * 1.f, -1.3f, -0.3f}));
}

// One arm from the file into b; false if there's no file (draw it from code then)
inline bool draw(gunkit::Builder& b, vec3 shoulder, const mat4& fist, float side, const ArmLook& L) {
    const gunkit::GunModel* m = model();
    if (!m) return false;
    vec3 elbow, wrist;
    chain(shoulder, fist, side, elbow, wrist);
    const mat4 mirror = side < 0.f ? glm::scale(mat4(1.f), vec3{-1.f, 1.f, 1.f}) : mat4(1.f);
    mat4 W[A_COUNT];
    W[A_ROOT] = mat4(1.f);
    W[A_UPPER] = along(shoulder, elbow, UPPER) * mirror;
    W[A_FORE] = along(elbow, wrist, FORE) * mirror;
    W[A_FIST] = fist * mirror;
    vec3 chan[AC_COUNT];
    chan[AC_CONST] = vec3{1.f};
    chan[AC_SEAM] = gunkit::SWORD_GLOW * L.seam;
    chan[AC_FLARE] = L.glow * (0.7f + 3.5f * L.flare) + vec3{1.f, 0.9f, 0.7f} * (2.f * L.flare);
    chan[AC_RUNE] = L.glow * (1.f + 3.f * L.flare);
    const bool flag[AF_COUNT] = {L.open, L.gauntlet};
    for (const auto& p : m->parts) {
        if (p.flag >= 0 && p.flag < AF_COUNT && flag[p.flag] != p.want) continue;
        if (p.joint <= A_ROOT || p.joint >= A_COUNT) continue;
        vec3 col = p.glowScale >= 0.f ? L.glow * p.glowScale : p.color;
        vec3 emi = p.chan >= 0 && p.chan < AC_COUNT ? chan[p.chan] * p.k : vec3{0.f};
        b.out.push_back({W[p.joint] * p.rel, col, emi, 0, p.mesh});
    }
    return true;
}

} // namespace armfile

namespace gunkit {
// An arm: from its file, or (no file) from code
inline void drawArm(Builder& b, vec3 shoulder, const mat4& fist, float side, const armfile::ArmLook& L) {
    if (armfile::draw(b, shoulder, fist, side, L)) return;
    buildArm(b, shoulder, fist, side, L.open, L.seam);
    if (L.gauntlet) buildGauntlet(b, fist, L.glow, L.flare);
}
} // namespace gunkit
