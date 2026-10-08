#pragma once
// =============================================================================
// VoiceTable.h — the names of every enemy voice, and which an enemy has.
//   v_<type>_spawn / _idle / _move / _hurt / _death
//   v_<type>_tell_<attack>   the wind-up      v_<type>_atk_<attack>   the release
//   plus a few specials: a halo breaking, a boss enraged, the Penitent rising
// The bank itself is built in code (VoiceSynth.h); EnemyVoice.h picks cues.
// =============================================================================
#include "Enemy.h"
#include "MixTable.h"
#include <string>
#include <vector>

enum class VoiceKind : uint8_t { SPAWN, IDLE, MOVE, TELL, ATTACK, HURT, DEATH, SPECIAL };

inline const char* voiceKey(EnemyType t) {
    static const char* K[] = {"husk", "ripper", "sentinel", "raptor", "brute", "mite", "juggernaut", "warden",
                              "sovereign", "shieldbearer", "conduit", "conductor", "seraph", "anchor", "penitent",
                              "revenant", "weaver", "leviathan"};
    static_assert(sizeof(K) / sizeof(K[0]) == (size_t)EnemyType::COUNT, "a voice key per enemy type");
    return (int)t < (int)EnemyType::COUNT ? K[(int)t] : "any";
}

inline const char* attackKey(AttackKind k) {
    switch (k) {
        case AttackKind::SHOT: return "shot";         case AttackKind::BURST: return "burst";
        case AttackKind::LUNGE: return "lunge";       case AttackKind::DIVE: return "dive";
        case AttackKind::SLAM: return "slam";         case AttackKind::LOB: return "lob";
        case AttackKind::FUSE: return "fuse";         case AttackKind::VOLLEY: return "volley";
        case AttackKind::SUMMON: return "summon";     case AttackKind::SHELL: return "shell";
        case AttackKind::SMASH: return "smash";       case AttackKind::DASH: return "dash";
        case AttackKind::SWEEP: return "sweep";       case AttackKind::CLEAVE: return "cleave";
        case AttackKind::LEAP: return "leap";         case AttackKind::CRESCENT: return "crescent";
        case AttackKind::BLINK: return "blink";       case AttackKind::JUDGMENT: return "judgment";
        case AttackKind::WHIRL: return "whirl";       case AttackKind::THRUST: return "thrust";
        case AttackKind::RUPTURE: return "rupture";   case AttackKind::PHANTOMS: return "phantoms";
        case AttackKind::BASH: return "bash";         case AttackKind::BEAM: return "beam";
        case AttackKind::CENSER_LOW: return "censer_low"; case AttackKind::CENSER_HIGH: return "censer_high";
        case AttackKind::PSLAM: return "pslam";       case AttackKind::PSTOMP: return "stomp";
        case AttackKind::PLASH: return "lash";        case AttackKind::SCOURGE: return "scourge";
        case AttackKind::WVENT: return "vent";        case AttackKind::LANCE: return "lance";
        case AttackKind::SEEKER: return "seeker";     case AttackKind::WLUNGE: return "lunge";
        case AttackKind::DETONATE: return "detonate";
        case AttackKind::RAKE: return "rake";         case AttackKind::SOULBOLT: return "soulbolt";
        case AttackKind::STRING: return "string";
        case AttackKind::CRASH: return "crash";       case AttackKind::TORRENT: return "torrent";
        case AttackKind::TIDE: return "tide";         case AttackKind::SPIT: return "spit";
        case AttackKind::BREACH: return "breach";     case AttackKind::SWALLOW: return "swallow";
        case AttackKind::SUBMERGE: return "submerge";
        default: return "none";
    }
}

// Every attack each type winds up (the test runs each enemy to prove it)
inline std::vector<AttackKind> attacksOf(EnemyType t) {
    using A = AttackKind;
    switch (t) {
        case EnemyType::HUSK:         return {A::SHOT};
        case EnemyType::RIPPER:       return {A::LUNGE};
        case EnemyType::SENTINEL:     return {A::BURST};
        case EnemyType::RAPTOR:       return {A::SHOT, A::DIVE};
        case EnemyType::BRUTE:        return {A::SLAM, A::LOB};
        case EnemyType::MITE:         return {A::FUSE};
        case EnemyType::JUGGERNAUT:   return {A::SHELL, A::SMASH};
        case EnemyType::WARDEN:       return {A::VOLLEY, A::SLAM, A::SUMMON, A::WVENT, A::LANCE, A::SEEKER, A::WLUNGE, A::DETONATE};
        case EnemyType::SOVEREIGN:    return {A::DASH, A::SWEEP, A::CLEAVE, A::LEAP, A::CRESCENT, A::BLINK,
                                              A::JUDGMENT, A::WHIRL, A::THRUST, A::RUPTURE, A::PHANTOMS};
        case EnemyType::SHIELDBEARER: return {A::SHOT, A::BASH};
        case EnemyType::SERAPH:       return {A::BEAM};
        case EnemyType::ANCHOR:       return {A::VOLLEY};
        case EnemyType::REVENANT:     return {A::RAKE, A::SOULBOLT};
        case EnemyType::WEAVER:       return {A::STRING};
        case EnemyType::PENITENT:     return {A::CENSER_LOW, A::CENSER_HIGH, A::PSLAM, A::PSTOMP, A::PLASH, A::SCOURGE};
        case EnemyType::LEVIATHAN:    return {A::CRASH, A::TORRENT, A::TIDE, A::SPIT, A::BREACH, A::SWALLOW, A::SUBMERGE};
        default:                      return {};
    }
}

// Does the attack get a release sound of its own? (Slams and smashes, the
// Sovereign's strokes and the Penitent's blows already sound in the game.)
// BEAM's is held while the beam is on.
inline bool hasRelease(AttackKind k) {
    switch (k) {
        case AttackKind::SHOT: case AttackKind::BURST: case AttackKind::LUNGE: case AttackKind::DIVE:
        case AttackKind::LOB: case AttackKind::SHELL: case AttackKind::VOLLEY: case AttackKind::BASH:
        case AttackKind::BEAM: case AttackKind::RAKE: case AttackKind::SOULBOLT: return true;
        default: return false;
    }
}

// Steps (or wingbeats) a second at full speed; 0: it doesn't move
inline float moveCadence(EnemyType t) {
    static const float C[] = {2.2f, 7.f, 1.5f, 3.f, 1.4f, 9.f, 1.2f, 1.f, 2.4f, 1.8f, 0.f, 2.f, 1.6f, 1.f, 0.7f, 2.6f, 5.f, 0.f};
    static_assert(sizeof(C) / sizeof(C[0]) == (size_t)EnemyType::COUNT, "a cadence per enemy type");
    return C[(int)t];
}

// How long a tell sounds: about the wind-up it announces
inline float tellDur(EnemyType t, AttackKind a) {
    switch (a) {
        case AttackKind::LUNGE: return 0.32f;   case AttackKind::BURST: return 0.75f;
        case AttackKind::DIVE: return 0.55f;    case AttackKind::LOB: return 0.7f;
        case AttackKind::FUSE: return 0.55f;    case AttackKind::SHELL: return 1.1f;
        case AttackKind::SMASH: return 0.95f;   case AttackKind::SUMMON: return 1.2f;
        case AttackKind::BASH: return 0.6f;     case AttackKind::BEAM: return 1.f;
        case AttackKind::CENSER_LOW: case AttackKind::CENSER_HIGH: return 0.9f;
        case AttackKind::PSLAM: return 1.f;     case AttackKind::PSTOMP: return 0.8f;
        case AttackKind::PLASH: return 0.7f;    case AttackKind::SCOURGE: return 0.8f;
        case AttackKind::JUDGMENT: return 0.65f; case AttackKind::PHANTOMS: return 0.6f;
        case AttackKind::LEAP: return 0.55f;    case AttackKind::RUPTURE: return 0.6f;
        case AttackKind::WVENT: return 0.6f;    case AttackKind::LANCE: return 1.f;
        case AttackKind::SEEKER: return 1.f;    case AttackKind::WLUNGE: return 0.8f;
        case AttackKind::DETONATE: return 1.5f;
        case AttackKind::RAKE: return 0.45f;    case AttackKind::SOULBOLT: return 0.6f;
        case AttackKind::STRING: return 0.8f;
        case AttackKind::CRASH: return 1.1f;    case AttackKind::TORRENT: return 0.8f;
        case AttackKind::TIDE: return 0.9f;     case AttackKind::SPIT: return 1.f;
        case AttackKind::BREACH: return 1.2f;   case AttackKind::SWALLOW: return 0.9f;
        case AttackKind::SUBMERGE: return 0.8f;
        case AttackKind::DASH: case AttackKind::SWEEP: case AttackKind::CLEAVE: case AttackKind::CRESCENT:
        case AttackKind::BLINK: case AttackKind::WHIRL: case AttackKind::THRUST: return 0.4f;
        default: return statsOf(t).telegraph > 0.f ? statsOf(t).telegraph : 0.6f;
    }
}

inline std::string voiceName(EnemyType t, VoiceKind k, AttackKind a = AttackKind::NONE) {
    const std::string n = std::string("v_") + voiceKey(t) + "_";
    switch (k) {
        case VoiceKind::SPAWN:  return n + "spawn";
        case VoiceKind::IDLE:   return n + "idle";
        case VoiceKind::MOVE:   return n + "move";
        case VoiceKind::HURT:   return n + "hurt";
        case VoiceKind::DEATH:  return n + "death";
        case VoiceKind::TELL:   return n + "tell_" + attackKey(a);
        case VoiceKind::ATTACK: return n + "atk_" + attackKey(a);
        default:                return "";
    }
}

inline const char* const VOICE_HALO_BREAK   = "v_halo_break";
inline const char* const VOICE_HALO_SHIMMER = "v_halo_shimmer";

struct VoiceSpec {
    std::string name;
    EnemyType   type;      // COUNT: shared by all
    VoiceKind   kind;
    AttackKind  attack;
    int         variants;
    MixClass    cls;
    const char* special = "";   // SPECIAL: which recipe
};

inline const std::vector<VoiceSpec>& voiceBank() {
    static const std::vector<VoiceSpec> bank = [] {
        std::vector<VoiceSpec> v;
        for (int i = 0; i < (int)EnemyType::COUNT; ++i) {
            const EnemyType t = (EnemyType)i;
            v.push_back({voiceName(t, VoiceKind::SPAWN), t, VoiceKind::SPAWN, AttackKind::NONE, 1, MixClass::WORLD});
            v.push_back({voiceName(t, VoiceKind::IDLE),  t, VoiceKind::IDLE,  AttackKind::NONE, 2, MixClass::CHATTER});
            if (moveCadence(t) > 0.f)
                v.push_back({voiceName(t, VoiceKind::MOVE), t, VoiceKind::MOVE, AttackKind::NONE, 3, MixClass::CHATTER});
            v.push_back({voiceName(t, VoiceKind::HURT),  t, VoiceKind::HURT,  AttackKind::NONE, 3, MixClass::FOLEY});
            v.push_back({voiceName(t, VoiceKind::DEATH), t, VoiceKind::DEATH, AttackKind::NONE, 2, MixClass::ACTION});
            for (AttackKind a : attacksOf(t)) {
                v.push_back({voiceName(t, VoiceKind::TELL, a), t, VoiceKind::TELL, a, 2, MixClass::TELL});
                if (hasRelease(a))
                    v.push_back({voiceName(t, VoiceKind::ATTACK, a), t, VoiceKind::ATTACK, a,
                                 a == AttackKind::BEAM ? 1 : 2, a == AttackKind::BEAM ? MixClass::TELL : MixClass::ACTION});
            }
        }
        auto sp = [&](const char* name, EnemyType t, int n, MixClass c, const char* key) {
            v.push_back({name, t, VoiceKind::SPECIAL, AttackKind::NONE, n, c, key});
        };
        sp(VOICE_HALO_BREAK,     EnemyType::COUNT,     1, MixClass::ACTION,  "halo_break");
        sp(VOICE_HALO_SHIMMER,   EnemyType::COUNT,     1, MixClass::CHATTER, "halo_shimmer");
        sp("v_warden_enrage",    EnemyType::WARDEN,    1, MixClass::TELL,    "enrage");
        sp("v_sovereign_enrage", EnemyType::SOVEREIGN, 1, MixClass::TELL,    "enrage");
        sp("v_penitent_rise",    EnemyType::PENITENT,  1, MixClass::TELL,    "rise");
        sp("v_conductor_link",   EnemyType::CONDUCTOR, 2, MixClass::FOLEY,   "link");
        sp("v_revenant_soul",    EnemyType::REVENANT,  1, MixClass::TELL,    "soul");
        sp("v_revenant_reform",  EnemyType::REVENANT,  1, MixClass::TELL,    "reform");
        sp("v_weaver_twang",     EnemyType::WEAVER,    2, MixClass::ACTION,  "twang");
        sp("v_leviathan_rise",   EnemyType::LEVIATHAN, 1, MixClass::TELL,    "rise");
        sp("v_leviathan_enrage", EnemyType::LEVIATHAN, 1, MixClass::TELL,    "enrage");
        sp("v_leviathan_choke",  EnemyType::LEVIATHAN, 1, MixClass::ACTION,  "choke");
        return v;
    }();
    return bank;
}
