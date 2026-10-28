#pragma once
// =============================================================================
// EnemyTypes.h — the names the enemy code shares with everything else: the
// roster (EnemyType), every attack (AttackKind), the Hollowed variants, and
// each type's base stats (statsOf). No behaviour here: that's Enemy.h and the
// bosses' own files.
// =============================================================================
#include <glm/glm.hpp>
#include <cstdint>
#include "core/Accessibility.h"

// CONDUIT: not a fighter but a spawner pylon (a wave objective, WaveDirector.h):
// it stands still and the wave keeps coming out of it until it's destroyed.
// CONDUCTOR: doesn't attack either; it tethers nearby allies and shields them
// (linkConductors below), so it's the one to kill first.
// The Gauntlet's (FAST): a mini-boss in each room, THE COURIER .. THE FOREMAN (EnemyMini.h), and its
// final boss, THE PACEMAKER (bosses/pacemaker). Each mini-boss moves on an older enemy's skeleton
// (rigBase) at a bigger size, with a body of its own made in Blender.
enum class EnemyType { HUSK, RIPPER, SENTINEL, RAPTOR, BRUTE, MITE, JUGGERNAUT, WARDEN, SOVEREIGN, SHIELDBEARER, CONDUIT,
                       CONDUCTOR, SERAPH, ANCHOR, PENITENT, REVENANT, WEAVER, LEVIATHAN, ABSOLUTE,
                       COURIER, BULWARK, CANTOR, HARRIER, PLUMB, FOREMAN, PACEMAKER, COUNT };
inline bool isBoss(EnemyType t) { return t == EnemyType::WARDEN || t == EnemyType::SOVEREIGN || t == EnemyType::PENITENT || t == EnemyType::LEVIATHAN ||
                                         t == EnemyType::ABSOLUTE || t == EnemyType::PACEMAKER; }
// A Gauntlet room's mini-boss: a boss bar and a death of its own, no finisher; kill it and the room is won
inline bool isMiniBoss(EnemyType t) { return t >= EnemyType::COURIER && t <= EnemyType::FOREMAN; }
// Whose skeleton (and pose, head box, joints) a type moves on
inline EnemyType rigBase(EnemyType t) {
    switch (t) {
        case EnemyType::COURIER: return EnemyType::RIPPER;
        case EnemyType::BULWARK: return EnemyType::SHIELDBEARER;
        case EnemyType::CANTOR:  return EnemyType::SERAPH;
        case EnemyType::HARRIER: return EnemyType::RAPTOR;
        case EnemyType::PLUMB:   return EnemyType::CONDUIT;
        case EnemyType::FOREMAN: return EnemyType::CONDUCTOR;
        default: return t;
    }
}
// How much bigger than its rig's enemy a mini-boss is drawn (and hit)
inline float miniScale(EnemyType t) {
    switch (t) {
        case EnemyType::COURIER: return 1.55f;
        case EnemyType::BULWARK: return 1.7f;
        case EnemyType::CANTOR:  return 1.9f;
        case EnemyType::HARRIER: return 2.3f;
        case EnemyType::PLUMB:   return 1.45f;
        case EnemyType::FOREMAN: return 1.7f;
        default: return 1.f;
    }
}

// Hollowed variants (Act II): a regular enemy made harder in one specific way.
//   ENRAGED  faster, shorter wind-ups, hits harder
//   TWINNED  splits in two smaller copies when it dies
//   HALOED   takes a tenth of the damage until a headshot or a parry breaks its halo
enum class Hollow { NONE, ENRAGED, TWINNED, HALOED };
inline bool canBeHollow(EnemyType t) {
    return !isBoss(t) && !isMiniBoss(t) && t != EnemyType::CONDUIT && t != EnemyType::CONDUCTOR;
}
inline const char* hollowName(Hollow h) {
    switch (h) { case Hollow::ENRAGED: return "ENRAGED"; case Hollow::TWINNED: return "TWINNED";
                 case Hollow::HALOED: return "HALOED"; default: return ""; }
}
inline const char* hollowHint(Hollow h) {
    switch (h) {
        case Hollow::ENRAGED: return "FASTER AND MEANER - DON'T WAIT FOR ITS RHYTHM";
        case Hollow::TWINNED: return "KILL IT AND THERE ARE TWO - FINISH BOTH";
        case Hollow::HALOED:  return "SHRUGS OFF DAMAGE - A HEADSHOT OR PARRY BREAKS THE HALO";
        default: return "";
    }
}
enum class EnemyState { SPAWNING, ACTIVE, DEAD };
enum class AttackKind { NONE, SHOT, BURST, LUNGE, DIVE, SLAM, LOB, FUSE, VOLLEY, SUMMON, SHELL, SMASH,
                        DASH, SWEEP, CLEAVE, LEAP, CRESCENT,   // the SOVEREIGN's
                        BLINK, JUDGMENT, WHIRL, THRUST, RUPTURE, PHANTOMS,
                        BASH,                                  // the SHIELDBEARER's
                        BEAM,                                  // the SERAPH's
                        CENSER_LOW, CENSER_HIGH, PSLAM, PSTOMP, PLASH, SCOURGE,     // the PENITENT's
                        WVENT, LANCE, SEEKER, WLUNGE, DETONATE,        // the WARDEN's
                        RAKE, SOULBOLT, STRING,                        // the REVENANT's, the WEAVER's
                        CRASH, TORRENT, TIDE, SPIT, BREACH, SWALLOW, SUBMERGE,     // the LEVIATHAN's
                        ATELEPORT, ADASH, AJAB, AHOOK, AUPPER, ADROP, AHOP, ARAY, ASTOMP, ASERPENT,   // THE ABSOLUTE's
                        AHALO, ASEAMS, AZENITH, ASWEEP, ALUNGE, ACRESCENT, AJUDGE, ATAKEOFF, ASKYFALL, AMETEOR,
                        ACROSS, ABACKFIST, AELBOW, AROUNDHOUSE, AHAYMAKER, ACHARGE, AAXE, ACLAP, ATHUNDER,   // (his fighter's arsenal; THUNDERCLAP)
                        MJAB, MBACKHAND, MGRASP, MDROP, MSUNDER, MFISSURE, MFLARES, MCOLLAPSE };   // MORNINGSTAR's (EnemyAbsolute.h)

struct EnemyStats {
    const char* name;
    float     health;
    float     radius, height;   // hitbox: feet at position, centred in XZ
    float     speed;            // m/s
    float     telegraph;        // default wind-up before an attack lands
    float     attackEvery;      // seconds between attacks
    bool      flying;
    glm::vec3 color;            // armour colour (also hit/death particle tint)
    glm::vec3 glow;             // eyes / core
    glm::vec3 shotColor;
    const char* hint;           // shown the first time the type appears
};

// The table (the balance panel, Balance.h, edits it live; statsOf() is how everything reads it)
inline EnemyStats& statsEdit(EnemyType t) {
    static EnemyStats S[] = {
        {"HUSK",     60.f, 0.45f, 1.95f, 3.9f, 0.45f, 1.9f, false,
         {0.70f,0.26f,0.18f}, {1.0f,0.78f,0.20f}, {1.0f,0.50f,0.12f},
         "SLOW ORBS - PRESS F AS ONE REACHES YOU TO SEND IT BACK"},
        {"RIPPER",   45.f, 0.55f, 1.10f, 8.4f, 0.32f, 1.0f, false,
         {0.80f,0.64f,0.14f}, {1.0f,0.95f,0.25f}, {1.0f,0.9f,0.3f},
         "IT CROUCHES BEFORE IT LUNGES - DASH ASIDE"},
        {"SENTINEL", 55.f, 0.45f, 2.60f, 3.0f, 0.75f, 2.9f, false,
         {0.20f,0.32f,0.62f}, {0.25f,0.95f,1.0f}, {0.35f,0.9f,1.0f},
         "THE LASER COMES FIRST - BREAK LINE OF SIGHT"},
        {"RAPTOR",   40.f, 1.00f, 0.90f, 7.5f, 0.42f, 1.9f, true,
         {0.40f,0.16f,0.52f}, {1.0f,0.30f,0.85f}, {0.95f,0.35f,1.0f},
         "THEY CIRCLE, THEN DIVE - WATCH THE SKY"},
        {"BRUTE",   340.f, 0.95f, 2.90f, 3.0f, 0.90f, 2.3f, false,
         {0.58f,0.24f,0.17f}, {1.0f,0.48f,0.06f}, {1.0f,0.50f,0.10f},
         "IT SLAMS THE GROUND - JUMP THE SHOCKWAVE"},
        {"MITE",     14.f, 0.38f, 0.60f, 7.2f, 0.55f, 0.0f, false,
         {0.18f,0.30f,0.16f}, {0.40f,1.0f,0.30f}, {0.40f,1.0f,0.30f},
         "WALKING BOMBS - POP THEM EARLY, AMONG FRIENDS"},
        {"JUGGERNAUT", 700.f, 1.05f, 3.30f, 2.3f, 1.10f, 3.2f, false,
         {0.36f,0.38f,0.44f}, {1.0f,0.72f,0.12f}, {1.0f,0.78f,0.2f},
         "HALF YOUR BULLETS BOUNCE - PARRY (F) ITS SHELLS, PUNCH ITS SMASH"},
        {"WARDEN", 3600.f, 1.60f, 4.60f, 2.4f, 0.85f, 3.0f, false,
         {0.34f,0.27f,0.40f}, {1.0f,0.16f,0.62f}, {1.0f,0.22f,0.68f},
         "THE WARDEN"},
        {"SOVEREIGN", 6400.f, 0.85f, 3.50f, 6.2f, 0.55f, 0.95f, false,
         {0.12f,0.11f,0.14f}, {1.0f,0.24f,0.14f}, {1.0f,0.82f,0.45f},
         "PARRY (F) HIS BLADE AS IT FALLS - DASH THROUGH THE REST"},
        {"SHIELDBEARER", 140.f, 0.6f, 2.20f, 3.4f, 0.5f, 2.4f, false,
         {0.34f,0.37f,0.42f}, {0.3f,1.0f,0.7f}, {0.4f,1.0f,0.75f},
         "NOTHING GETS THROUGH THE FRONT - FLANK IT OR PARRY THE BASH"},
        {"CONDUIT", 300.f, 0.8f, 3.4f, 0.f, 0.f, 0.f, false,
         {0.22f,0.2f,0.26f}, {1.0f,0.25f,0.45f}, {1.0f,0.3f,0.5f},
         "THEY KEEP THE WAVE COMING - DESTROY THEM ALL"},
        {"CONDUCTOR", 90.f, 0.75f, 1.7f, 6.f, 0.f, 0.f, true,
         {0.18f,0.3f,0.34f}, {0.3f,1.0f,0.9f}, {0.3f,1.0f,0.9f},
         "IT SHIELDS WHAT IT TETHERS - TAKE IT OUT FIRST"},
        {"SERAPH", 120.f, 0.8f, 1.6f, 5.0f, 1.0f, 3.5f, true,
         {0.82f,0.78f,0.66f}, {1.0f,0.86f,0.5f}, {1.0f,0.9f,0.6f},
         "THE BEAM TURNS SLOWER THAN YOU RUN - KEEP MOVING"},
        {"ANCHOR", 260.f, 1.0f, 2.8f, 2.2f, 1.0f, 2.8f, false,
         {0.24f,0.27f,0.32f}, {0.95f,0.25f,0.3f}, {1.0f,0.35f,0.35f},
         "INSIDE ITS FIELD: NO DASH, NO GRAPPLE"},
        {"PENITENT", 6000.f, 3.0f, 8.2f, 3.5f, 0.9f, 2.6f, false,
         {0.14f,0.13f,0.14f}, {1.3f,0.75f,0.3f}, {1.2f,0.5f,0.2f},
         "BREAK ITS CHAINS - JUMP LOW SWEEPS, SLIDE UNDER HIGH ONES"},
        {"REVENANT", 160.f, 0.5f, 2.3f, 5.4f, 0.45f, 1.6f, false,
         {0.2f,0.21f,0.26f}, {0.75f,0.9f,1.3f}, {0.6f,0.85f,1.2f},
         "ITS SOUL RUNS - CATCH IT BEFORE IT COMES BACK"},
        {"WEAVER", 120.f, 0.9f, 1.3f, 4.2f, 0.8f, 6.f, false,
         {0.16f,0.13f,0.2f}, {0.75f,0.35f,1.1f}, {0.75f,0.35f,1.1f},
         "IT WIRES THE GAPS - CUT THE NODES OR DUCK UNDER"},
        {"LEVIATHAN", 15000.f, 3.0f, 5.2f, 0.f, 1.1f, 1.9f, false,   // radius/height: the head's box
         {0.2f,0.18f,0.21f}, {1.4f,1.0f,0.6f}, {0.3f,1.1f,0.9f},
         "DASH THE CRASH - SHOOT THE EYE WHILE IT'S DOWN"},
        {"THE ABSOLUTE", 10800.f, 0.6f, 2.6f, 11.f, 0.45f, 0.9f, false,   // the encore (EnemyAbsolute.h)
         {0.95f,0.95f,0.98f}, {1.6f,1.5f,1.2f}, {1.6f,1.55f,1.4f},
         "WHITE: PARRY  GOLD: DODGE OR BLOCK  RED: GET OUT"},
        // The Gauntlet's mini-boss (radius and height: their rig's enemy's; miniScale makes them big)
        {"THE COURIER", 560.f, 0.55f, 1.10f, 9.5f, 0.45f, 3.6f, false,
         {0.78f,0.36f,0.10f}, {1.0f,0.86f,0.30f}, {1.0f,0.8f,0.3f},
         "IT HAS THE KEY - RUN IT DOWN (HOOK IT, PUNCH IT)"},
        {"THE BULWARK", 1150.f, 0.6f, 2.20f, 2.7f, 0.7f, 2.6f, false,
         {0.30f,0.36f,0.38f}, {0.3f,1.0f,0.75f}, {0.4f,1.0f,0.8f},
         "ITS GATE STOPS EVERYTHING - VAULT THE PADS, HIT ITS BACK"},
        {"THE CANTOR", 900.f, 0.8f, 1.6f, 4.0f, 1.0f, 3.0f, true,
         {0.86f,0.74f,0.52f}, {1.0f,0.75f,0.35f}, {1.0f,0.8f,0.45f},
         "CLIMB THROUGH ITS BEAM - JUMP THE BELL'S RINGS"},
        {"THE HARRIER", 760.f, 1.00f, 0.90f, 9.0f, 0.55f, 2.2f, true,
         {0.24f,0.26f,0.34f}, {0.4f,0.85f,1.0f}, {0.45f,0.85f,1.0f},
         "WHEN IT DIVES, IT LANDS - HOOK IT AND HIT IT THERE"},
        {"THE PLUMB LINE", 1000.f, 0.8f, 3.4f, 6.0f, 0.9f, 3.2f, true,
         {0.26f,0.22f,0.20f}, {1.0f,0.55f,0.2f}, {1.0f,0.6f,0.25f},
         "ARMOURED - SHOOT ITS CORE WHEN IT OPENS AFTER A VOLLEY"},
        {"THE FOREMAN", 700.f, 0.75f, 1.7f, 5.0f, 0.9f, 2.8f, true,
         {0.62f,0.48f,0.14f}, {1.0f,0.7f,0.15f}, {1.0f,0.6f,0.15f},
         "ITS CREW SHIELDS IT - CUT THE TETHERS, THEN BRING IT DOWN"},
        {"THE PACEMAKER", 5200.f, 0.55f, 2.6f, 7.0f, 0.5f, 1.0f, false,
         {0.16f,0.17f,0.20f}, {1.0f,0.55f,0.12f}, {1.0f,0.7f,0.3f},
         "WHITE: PARRY - IT RACES YOU TO THE BEACON"},
    };
    return S[(int)t];
}
inline const EnemyStats& statsOf(EnemyType t) { return statsEdit(t); }


// THE ABSOLUTE's tell colours (EnemyAbsolute.h): white parry, gold dodge or block, red get out
enum class AbTell : int8_t { NONE = -1, WHITE = 0, GOLD = 1, RED = 2 };
inline AbTell abTellOf(AttackKind k);   // (EnemyAbsolute.h)
// A gold tell's colour as shown: gold, or the blue that stands in for it (same strength)
inline glm::vec3 abGoldTell(glm::vec3 gold) {
    if (!g_tellsColourblind) return gold;
    float m = gold.x > gold.y ? (gold.x > gold.z ? gold.x : gold.z) : (gold.y > gold.z ? gold.y : gold.z);
    return glm::vec3{0.25f, 0.6f, 1.f} * m;
}
inline const char* abTellLegend() {
    return g_tellsColourblind ? "WHITE: PARRY  BLUE: DODGE OR BLOCK  RED: GET OUT" : "WHITE: PARRY  GOLD: DODGE OR BLOCK  RED: GET OUT";
}
