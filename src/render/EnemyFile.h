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
//              WARDEN     a humanoid's, and cape and capeTip (its hem), crown (rides the head)
//              PENITENT   a humanoid's (its censers ride its fists: "tellR"/"tellL" light them)
//              REVENANT, ANCHOR  a humanoid's
//              WEAVER     body (rears to string a wire), legL1-3 / legR1-3 (hips, front to back),
//                         shinL1-3 / shinR1-3 (knees): legs up to a high knee, down to the floor
//              SERAPH     head (its ring), wingL/R (the upper pair), tipL/R (the lower pair, off its body)
//              CONDUIT    halo and crown (its two rings, turning), head (the crystal on top)
//              CONDUCTOR  body (bobbing), halo (its three fins, spinning)
//              LEVIATHAN  head (its skull, in the head rig's units: drawn x1.5), jaw (hinged under it),
//                         halo (its eye's corona, turning); body: ONE segment of its serpent body,
//                         r 2.5 m, running 2.5 m along +Z from the joint (12 m behind the head, out of
//                         its way in Blender), drawn once per segment
//                         along the curve and scaled to it ("pulse" its seam ring, "light" its fin)
//              SOVEREIGN  a humanoid's, and weapon (the greatsword, turned in his fist), cape and
//                         capeTip (its hem, lagging it), halo (turning), crown (rides the head)
//              ABSOLUTE   (and MORNINGSTAR, his own file on the same joints) hips, knees, ankles,
//                         waist, clothF/B, torso, head, crown, halo, shoulders (armL/R), elbows,
//                         ribbons, hands, fingers, tips, thumbs, thumbTips, weapon (the blade of light)
//   glow     a channel the code sets every frame, x the part's k:
//            "glow"  the enemy's glow colour, flaring through a wind-up
//            "shot"  its shot colour, blazing as the shot is about to leave
//            "pulse" a slow throb that flares in a wind-up (a BRUTE's core)
//            "dive"  lit only while it dives (a RAPTOR's talons)
//            "fuse"  a MITE's core: steady, then flashing red as its fuse burns
//            "blade" a RIPPER's blades, brightening as it crouches to lunge
//            "smash" a JUGGERNAUT's hammer fist, lit only while it winds up a smash
//            "rim"   a SHIELDBEARER's shield rim, brightening through a wind-up
//            the SOVEREIGN's: "edge" his blade's edge and runes, "halo", "cape" (smoulders, enraged)
//            THE ABSOLUTE's: "seam" "soft" "light" "eye" "core" "halo" "sword"; a limb's seams, glow
//            and runes ("tellR", "auraLegL", "runeR"...) take his tell's colour when it strikes
//            "const" k itself
//   flags    "blade" (THE ABSOLUTE's blade of light is drawn), "enraged", "broken", "open" (the
//            WARDEN's core plates drawn back while it vents and in meltdown; the PENITENT's wound
//            while it scourges itself): a part with
//            show = "enraged" is drawn only then ("!enraged": only not)
//   colours  material base colours; "glow*0.3": the (flaring) glow colour x 0.3
//
// The head hitbox doesn't read the file (headBox, EnemyModel.h): a test holds
// every head box inside it. No OpenGL.
// =============================================================================
#include "render/GunFile.h"
#include "enemies/Enemy.h"
#include <array>
#include <cstdio>
#include <string>
#include <vector>

namespace enemyfile {
using gunkit::GunModel;

enum Joint { E_ROOT, E_HIPL, E_HIPR, E_TORSO, E_HEAD, E_ARML, E_ARMR,
             E_BODY, E_LEG_FL, E_LEG_FR, E_LEG_BL, E_LEG_BR, E_BLADE_L, E_BLADE_R,
             E_TAIL, E_WING_L, E_WING_R, E_TIP_L, E_TIP_R,
             E_LEG_L1, E_LEG_L2, E_LEG_L3, E_LEG_R1, E_LEG_R2, E_LEG_R3, E_SHIELD,
             // the bosses: the SOVEREIGN's sword, cape and halo; THE ABSOLUTE's (and MORNINGSTAR's) whole body
             E_SWORD, E_CAPE, E_CAPE_TIP, E_HALO, E_CROWN,
             E_KNEE_L, E_KNEE_R, E_ANKLE_L, E_ANKLE_R, E_WAIST, E_CLOTH_F, E_CLOTH_B,
             E_ELBOW_L, E_ELBOW_R, E_HAND_L, E_HAND_R, E_FINGERS_L, E_FINGERS_R, E_TIPS_L, E_TIPS_R,
             E_THUMB_L, E_THUMB_R, E_THUMB_TIP_L, E_THUMB_TIP_R, E_RIBBON_L, E_RIBBON_R,
             E_JAW,   // THE LEVIATHAN's
             E_SHIN_L1, E_SHIN_L2, E_SHIN_L3, E_SHIN_R1, E_SHIN_R2, E_SHIN_R3,   // a WEAVER's knees
             E_COUNT };
inline const char* jointName(int j) {
    static const char* N[E_COUNT] = {"root", "hipL", "hipR", "torso", "head", "armL", "armR",
                                     "body", "legFL", "legFR", "legBL", "legBR", "bladeL", "bladeR",
                                     "tail", "wingL", "wingR", "tipL", "tipR",
                                     "legL1", "legL2", "legL3", "legR1", "legR2", "legR3", "shield",
                                     "weapon", "cape", "capeTip", "halo", "crown",
                                     "kneeL", "kneeR", "ankleL", "ankleR", "waist", "clothF", "clothB",
                                     "elbowL", "elbowR", "handL", "handR", "fingersL", "fingersR", "tipsL", "tipsR",
                                     "thumbL", "thumbR", "thumbTipL", "thumbTipR", "ribbonL", "ribbonR", "jaw",
                                     "shinL1", "shinL2", "shinL3", "shinR1", "shinR2", "shinR3"};
    return N[j];
}
enum Chan { C_CONST, C_GLOW, C_SHOT, C_PULSE, C_DIVE, C_FUSE, C_BLADE, C_SMASH, C_RIM,
            C_EDGE, C_CAPE, C_HALO,                                   // the SOVEREIGN's
            C_SEAM, C_SOFT, C_LIGHT, C_EYE, C_CORE,                   // THE ABSOLUTE's (and MORNINGSTAR's) ...
            C_TELL_R, C_TELL_L, C_TELL_LEG_R, C_TELL_LEG_L,           // ... a limb's seams, lit by its tell
            C_AURA_R, C_AURA_L, C_AURA_LEG_R, C_AURA_LEG_L,           // ... a limb's glow, lit by its tell
            C_RUNE_R, C_RUNE_L, C_SWORD, C_COUNT };
inline const char* chanName(int c) {
    static const char* N[C_COUNT] = {"const", "glow", "shot", "pulse", "dive", "fuse", "blade", "smash", "rim",
                                     "edge", "cape", "halo", "seam", "soft", "light", "eye", "core",
                                     "tellR", "tellL", "tellLegR", "tellLegL", "auraR", "auraL", "auraLegR", "auraLegL",
                                     "runeR", "runeL", "sword"};
    return N[c];
}
// Flags a part can show on (or, "!enraged", off): THE ABSOLUTE's blade of light (phase 3),
// the SOVEREIGN enraged (the WARDEN in meltdown), a boss broken (parried), the WARDEN's core open
enum Flag { F_BLADE, F_ENRAGED, F_BROKEN, F_OPEN, F_COUNT };
inline const char* flagName(int f) {
    static const char* N[F_COUNT] = {"blade", "enraged", "broken", "open"};
    return N[f];
}
inline const gunkit::Names& names() { static const gunkit::Names n{E_COUNT, C_COUNT, F_COUNT, jointName, chanName, flagName}; return n; }

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
        case EnemyType::WARDEN:       return "warden";
        case EnemyType::PENITENT:     return "penitent";
        case EnemyType::LEVIATHAN:    return "leviathan";
        case EnemyType::REVENANT:     return "revenant";
        case EnemyType::WEAVER:       return "weaver";
        case EnemyType::SERAPH:       return "seraph";
        case EnemyType::ANCHOR:       return "anchor";
        case EnemyType::CONDUIT:      return "conduit";
        case EnemyType::CONDUCTOR:    return "conductor";
        case EnemyType::SOVEREIGN:    return "sovereign";
        case EnemyType::ABSOLUTE:     return "absolute";
        case EnemyType::COURIER:      return "courier";     // the Gauntlet's mini-bosses (on their rigBase's joints)
        case EnemyType::BULWARK:      return "bulwark";
        case EnemyType::CANTOR:       return "cantor";
        case EnemyType::HARRIER:      return "harrier";
        case EnemyType::PLUMB:        return "plumb";
        case EnemyType::FOREMAN:      return "foreman";
        case EnemyType::PACEMAKER:    return "pacemaker";
        case EnemyType::SORTER: return "sorter";
        default: return nullptr;
    }
}
// A file's id: an EnemyType's, or MORNINGSTAR's (THE ABSOLUTE's second form has a file of its own,
// on his joints: assets/enemies/morningstar.glb)
constexpr int FILE_MORNINGSTAR = (int)EnemyType::COUNT, FILE_COUNT = FILE_MORNINGSTAR + 1;
inline const char* fileNameOf(int f) { return f == FILE_MORNINGSTAR ? "morningstar" : f >= 0 && f < FILE_MORNINGSTAR ? fileName((EnemyType)f) : nullptr; }
inline EnemyType typeOfFile(int f) { return f == FILE_MORNINGSTAR ? EnemyType::ABSOLUTE : (EnemyType)f; }
inline int fileOf(const Enemy& e) { return e.type == EnemyType::ABSOLUTE && e.morning() ? FILE_MORNINGSTAR : (int)e.type; }

// The joints a type's rig turns (a file must have them all)
inline std::vector<int> neededJoints(EnemyType t) {
    switch (rigBase(t)) {
        case EnemyType::SORTER:return {E_TORSO,E_HEAD,E_ARML,E_ARMR,E_ELBOW_L,E_ELBOW_R,E_HAND_L,E_HAND_R};   // (a mini-boss: its rig's enemy's)
        case EnemyType::RIPPER: return {E_BODY, E_HEAD, E_LEG_FL, E_LEG_FR, E_LEG_BL, E_LEG_BR, E_BLADE_L, E_BLADE_R};
        case EnemyType::RAPTOR: return {E_HEAD, E_TAIL, E_WING_L, E_WING_R, E_TIP_L, E_TIP_R};
        case EnemyType::MITE:   return {E_BODY, E_LEG_L1, E_LEG_L2, E_LEG_L3, E_LEG_R1, E_LEG_R2, E_LEG_R3};
        case EnemyType::SHIELDBEARER: return {E_HIPL, E_HIPR, E_TORSO, E_HEAD, E_ARML, E_ARMR, E_SHIELD};
        case EnemyType::LEVIATHAN: return {E_HEAD, E_JAW, E_HALO, E_BODY};
        case EnemyType::WEAVER: return {E_BODY, E_LEG_L1, E_LEG_L2, E_LEG_L3, E_LEG_R1, E_LEG_R2, E_LEG_R3,
                                        E_SHIN_L1, E_SHIN_L2, E_SHIN_L3, E_SHIN_R1, E_SHIN_R2, E_SHIN_R3};
        case EnemyType::SERAPH: return {E_HEAD, E_WING_L, E_WING_R, E_TIP_L, E_TIP_R};
        case EnemyType::CONDUIT: return {E_HALO, E_CROWN, E_HEAD};
        case EnemyType::CONDUCTOR: return {E_BODY, E_HALO};
        case EnemyType::WARDEN: return {E_HIPL, E_HIPR, E_TORSO, E_HEAD, E_ARML, E_ARMR, E_CAPE, E_CAPE_TIP, E_CROWN};
        case EnemyType::SOVEREIGN: return {E_HIPL, E_HIPR, E_TORSO, E_HEAD, E_ARML, E_ARMR, E_SWORD, E_CAPE, E_CAPE_TIP, E_HALO, E_CROWN};
        case EnemyType::ABSOLUTE:
            return {E_HIPL, E_HIPR, E_TORSO, E_HEAD, E_ARML, E_ARMR, E_HALO, E_CROWN, E_SWORD,
                    E_KNEE_L, E_KNEE_R, E_ANKLE_L, E_ANKLE_R, E_WAIST, E_CLOTH_F, E_CLOTH_B,
                    E_ELBOW_L, E_ELBOW_R, E_HAND_L, E_HAND_R, E_FINGERS_L, E_FINGERS_R, E_TIPS_L, E_TIPS_R,
                    E_THUMB_L, E_THUMB_R, E_THUMB_TIP_L, E_THUMB_TIP_R, E_RIBBON_L, E_RIBBON_R};
        default:                return {E_HIPL, E_HIPR, E_TORSO, E_HEAD, E_ARML, E_ARMR};
    }
}
inline std::string pathOf(int f) { const char* n = fileNameOf(f); return n ? std::string("assets/enemies/") + n + ".glb" : std::string(); }
inline std::string path(EnemyType t) { return pathOf((int)t); }

inline GunModel& slotOf(int f) { static std::array<GunModel, (size_t)FILE_COUNT> M; return M[(size_t)f]; }
inline GunModel& slot(EnemyType t) { return slotOf((int)t); }
// (Re)load an enemy's file; false (and the model's error says why) if it couldn't be.
// A broken save keeps the last good model; with none, the enemy is drawn from code.
inline bool loadFile(int f, const std::string& p) {
    GunModel m = gunkit::readGunFile(p, names());
    if (m.error.empty())
        for (int j : neededJoints(typeOfFile(f)))
            if (!m.has(j)) { m.error = p + ": no '" + jointName(j) + "' joint"; break; }
    if (!m.error.empty()) std::fprintf(stderr, "enemy: %s\n", m.error.c_str());
    bool ok = m.error.empty();
    if (ok || slotOf(f).parts.empty()) slotOf(f) = std::move(m);
    else slotOf(f).error = m.error;
    return ok;
}
inline bool load(EnemyType t, const std::string& p) { return loadFile((int)t, p); }
// The model for a file: nullptr if there's no such file (or it never loaded)
inline const GunModel* modelOf(int f) {
    if (!fileNameOf(f)) return nullptr;
    static std::array<bool, (size_t)FILE_COUNT> tried{};
    if (!tried[(size_t)f]) { tried[(size_t)f] = true; loadFile(f, pathOf(f)); }
    const GunModel& m = slotOf(f);
    return m.parts.empty() ? nullptr : &m;
}
inline const GunModel* model(EnemyType t) { return modelOf((int)t); }

// Each joint at rest relative to its parent joint (from the file)
inline void restLocals(const GunModel& m, glm::mat4 local[E_COUNT], int parent[E_COUNT]) {
    glm::mat4 rest[E_COUNT];
    for (int j = 0; j < E_COUNT; ++j) { rest[j] = local[j] = glm::mat4(1.f); parent[j] = E_ROOT; }
    for (const auto& J : m.joints) { rest[J.id] = J.frame; parent[J.id] = J.parent; }
    for (int j = 1; j < E_COUNT; ++j) local[j] = glm::inverse(rest[parent[j]]) * rest[j];
}

} // namespace enemyfile
