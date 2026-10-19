#pragma once
// =============================================================================
// AbsoluteFeel.h — how each of THE ABSOLUTE's moves sounds and lands: its
// wind-up (played stretched over the tell, so it peaks as the blow lands), its
// boom (hit or miss), how hard it shakes you and where the impact is. The cue
// (EnemyEvents::abCue, AB_CUE before the blow) is by colour: a TING says parry,
// a DOOM says dodge or block, a double pulse says get out. No audio or GL here:
// Gameplay_Absolute.h plays it, the tests read it.
// =============================================================================
#include "enemies/Enemy.h"
#include <algorithm>
#include <string>
#include <vector>

struct AbFeel {
    const char* wind;     // the wind-up over the tell (nullptr: none)
    float windLen;        // that sound's natural length (s): it's played stretched to the tell
    float windPitch;      // its pitch before the stretch
    const char* boom;     // the impact, hit or miss (nullptr: none - beams, areas and projectiles sound elsewhere)
    float boomPitch;
    float shake;          // 0..1: how hard the impact shakes you at 0 m (nothing by 25 m)
    glm::vec3 at;         // where the impact is, from his feet, as he faces +Z
};

inline const AbFeel& abFeel(AttackKind k) {
    using A = AttackKind;
    static const AbFeel NONE{nullptr, 0.f, 1.f, nullptr, 1.f, 0.f, {0.f, 0.f, 0.f}};
    static const AbFeel JAB{"ab_wind_jab", 0.30f, 1.f, "ab_boom_jab", 1.f, 0.35f, {0.1f, 1.6f, 1.3f}};
    static const AbFeel CROSS{"ab_wind_cross", 0.35f, 1.f, "ab_boom_cross", 1.f, 0.5f, {-0.1f, 1.6f, 1.4f}};
    static const AbFeel HOOK{"ab_wind_hook", 0.45f, 1.f, "ab_boom_hook", 1.f, 0.5f, {0.6f, 1.6f, 1.0f}};
    static const AbFeel BACKFIST{"ab_wind_spin", 0.50f, 1.15f, "ab_boom_hook", 1.1f, 0.4f, {0.7f, 1.6f, 0.9f}};
    static const AbFeel ELBOW{"ab_wind_jab", 0.30f, 0.85f, "ab_boom_jab", 0.85f, 0.45f, {-0.2f, 1.5f, 0.9f}};
    static const AbFeel ROUND{"ab_wind_spin", 0.50f, 0.9f, "ab_boom_kick", 1.f, 0.55f, {0.8f, 1.1f, 1.2f}};
    static const AbFeel UPPER{"ab_thrum", 0.80f, 1.f, "ab_boom_heavy", 1.1f, 0.7f, {-0.1f, 1.9f, 1.0f}};
    static const AbFeel HAY{"ab_thrum", 0.80f, 0.85f, "ab_boom_heavy", 0.9f, 0.9f, {-0.2f, 1.7f, 1.3f}};
    static const AbFeel DROP{"ab_thrum", 0.80f, 1.1f, "ab_boom_kick", 0.85f, 0.7f, {0.f, 0.3f, 0.f}};
    static const AbFeel AXE{"ab_thrum", 0.80f, 1.15f, "ab_boom_stone", 1.1f, 0.75f, {0.f, 0.05f, 1.6f}};
    static const AbFeel CHARGE{"ab_thrum", 0.80f, 0.95f, "ab_boom_heavy", 1.f, 0.6f, {0.f, 1.3f, 0.6f}};
    static const AbFeel LUNGE{"ab_thrum", 0.80f, 1.05f, nullptr, 1.f, 0.f, {0.f, 1.4f, 1.f}};
    static const AbFeel SKY{"ab_whistle", 0.75f, 1.f, "ab_boom_stone", 0.8f, 1.f, {0.f, 0.f, 0.f}};
    static const AbFeel METEOR{"ab_wind_meteor", 0.60f, 1.f, "ab_boom_heavy", 1.15f, 0.8f, {0.f, 1.3f, 0.6f}};
    static const AbFeel SWEEP{"ab_wind_blade", 0.45f, 1.f, "ab_boom_hook", 1.25f, 0.45f, {0.f, 1.5f, 1.5f}};   // the blade's: a sharp crack
    static const AbFeel BLADE{"ab_wind_blade", 0.45f, 1.f, nullptr, 1.f, 0.f, {0.f, 1.5f, 1.5f}};
    static const AbFeel SERPENT{"ab_wind_blade", 0.45f, 0.75f, nullptr, 1.f, 0.f, {0.f, 1.8f, 0.5f}};
    static const AbFeel RAY{"ab_ray_charge", 0.65f, 1.f, nullptr, 1.f, 0.f, {0.f, 1.6f, 0.5f}};
    static const AbFeel ZRAY{"ab_ray_charge", 0.65f, 0.75f, nullptr, 1.f, 0.f, {0.f, 0.f, 0.f}};
    static const AbFeel AREA{"ab_rumble", 0.90f, 1.f, nullptr, 1.f, 0.f, {0.f, 0.f, 0.f}};
    static const AbFeel STOMP{"ab_rumble", 0.90f, 1.15f, "ab_boom_stone", 0.9f, 0.6f, {0.3f, 0.05f, 0.3f}};
    static const AbFeel CLAP{"ab_rumble", 0.90f, 1.3f, "ab_boom_clap", 1.f, 0.6f, {0.f, 1.5f, 0.9f}};
    static const AbFeel THUNDER{"ab_zoop", 1.15f, 1.f, "ab_boom_thunder", 1.f, 1.f, {0.f, 1.5f, 0.8f}};   // up with a zoop; a clap like thunder
    // MORNINGSTAR's: his own moves' sounds, pitched down for his size (where: in his unscaled frame, scaled by the game)
    static const AbFeel MJAB{"ab_wind_jab", 0.30f, 0.8f, "ab_boom_jab", 0.75f, 0.55f, {0.1f, 1.6f, 1.3f}};
    static const AbFeel MBACK{"ab_wind_spin", 0.50f, 0.8f, "ab_boom_heavy", 0.8f, 0.85f, {0.7f, 1.6f, 0.9f}};
    static const AbFeel MGRASP{"ab_thrum", 0.80f, 0.8f, "ab_boom_stone", 0.8f, 0.8f, {0.f, 0.05f, 1.6f}};
    static const AbFeel MDROP{"ab_whistle", 0.75f, 0.8f, "ab_boom_stone", 0.6f, 1.f, {0.f, 0.f, 0.f}};
    static const AbFeel MSUNDER{"ab_rumble", 0.90f, 0.85f, "ab_boom_stone", 0.7f, 1.f, {0.f, 0.05f, 1.4f}};
    static const AbFeel MFISSURE{"ab_thrum", 0.80f, 0.9f, "ab_boom_stone", 0.85f, 0.7f, {0.f, 0.05f, 1.4f}};
    static const AbFeel MFLARES{"ab_ray_charge", 0.65f, 0.7f, "ab_boom_thunder", 0.8f, 0.6f, {0.f, 2.f, 0.f}};
    static const AbFeel MCOLLAPSE{"ab_rumble", 0.90f, 0.7f, "ab_boom_thunder", 0.6f, 1.f, {0.f, 1.5f, 0.f}};
    switch (k) {
        case A::AJAB: return JAB;            case A::ACROSS: return CROSS;      case A::AHOOK: return HOOK;
        case A::ABACKFIST: return BACKFIST;  case A::AELBOW: return ELBOW;      case A::AROUNDHOUSE: return ROUND;
        case A::AUPPER: return UPPER;        case A::AHAYMAKER: return HAY;     case A::ADROP: return DROP;
        case A::AAXE: return AXE;            case A::ACHARGE: return CHARGE;    case A::ALUNGE: return LUNGE;
        case A::ASKYFALL: return SKY;        case A::AMETEOR: return METEOR;
        case A::ASWEEP: return SWEEP;        case A::ACRESCENT: return BLADE;
        case A::ASERPENT: return SERPENT;    case A::ARAY: return RAY;          case A::AZENITH: return ZRAY;
        case A::ASEAMS: case A::AJUDGE: case A::AHALO: return AREA;
        case A::ASTOMP: return STOMP;        case A::ACLAP: return CLAP;        case A::ATHUNDER: return THUNDER;
        case A::MJAB: return MJAB;           case A::MBACKHAND: return MBACK;   case A::MGRASP: return MGRASP;
        case A::MDROP: return MDROP;         case A::MSUNDER: return MSUNDER;   case A::MFISSURE: return MFISSURE;
        case A::MFLARES: return MFLARES;     case A::MCOLLAPSE: return MCOLLAPSE;
        default: return NONE;
    }
}
// The beat to act on, by colour
inline const char* abCueSound(AbTell t) {
    return t == AbTell::WHITE ? "ab_ting" : t == AbTell::GOLD ? "ab_doom" : t == AbTell::RED ? "ab_pulse" : "";
}
inline const std::vector<AttackKind>& abAllAttacks() {
    using A = AttackKind;
    static const std::vector<AttackKind> V = {
        A::ATELEPORT, A::ADASH, A::AJAB, A::AHOOK, A::AUPPER, A::ADROP, A::AHOP, A::ARAY, A::ASTOMP, A::ASERPENT, A::AHALO,
        A::ASEAMS, A::AZENITH, A::ASWEEP, A::ALUNGE, A::ACRESCENT, A::AJUDGE, A::ATAKEOFF, A::ASKYFALL, A::AMETEOR,
        A::ACROSS, A::ABACKFIST, A::AELBOW, A::AROUNDHOUSE, A::AHAYMAKER, A::ACHARGE, A::AAXE, A::ACLAP, A::ATHUNDER,
        A::MJAB, A::MBACKHAND, A::MGRASP, A::MDROP, A::MSUNDER, A::MFISSURE, A::MFLARES, A::MCOLLAPSE};
    return V;
}
// Every sound the fight plays that isn't a gun's or the sword's: the feel table's, the cues, and the
// game's own (Gameplay_Absolute.h, Gameplay_Sword.h)
inline std::vector<std::string> abAllSounds() {
    std::vector<std::string> s = {"ab_ting", "ab_doom", "ab_pulse", "ab_ray_beam", "ab_zray_beam", "ab_takeoff", "ab_flight",
                                  "ab_shotdown", "ab_zap", "ab_clang", "ab_shatter", "ab_creak", "ab_tsk", "ab_wrong"};
    for (AttackKind k : abAllAttacks()) {
        const AbFeel& f = abFeel(k);
        if (f.wind) s.push_back(f.wind);
        if (f.boom) s.push_back(f.boom);
    }
    std::sort(s.begin(), s.end()); s.erase(std::unique(s.begin(), s.end()), s.end());
    return s;
}
