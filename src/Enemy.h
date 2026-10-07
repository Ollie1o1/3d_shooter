#pragma once
// =============================================================================
// Enemy.h — the enemy roster: per-type stats and AI. No OpenGL in here, so the
// AI runs in the headless tests (tests/test_game.cpp). Models are built from
// the same state in EnemyModel.h.
//
// ROSTER (each one teaches the player a different answer):
//   HUSK     — humanoid rifleman. Keeps mid range, fires slow parryable orbs.
//   RIPPER   — low four-legged hound. Zig-zags in, crouches, lunges. Dash away.
//   SENTINEL — tall sniper. Paints you with a laser, then fires a fast burst.
//              Break line of sight while the laser is up.
//   RAPTOR   — bird. Circles overhead shooting, then dives at you.
//   BRUTE    — heavy. Walks you down and slams the ground; jump the shockwave.
//   MITE     — small spider bomb. Rushes and detonates; shoot it early and the
//              blast hurts its friends instead.
//   JUGGERNAUT — armored heavy (bullets do half). Fires slow siege shells and
//              smashes up close. PARRY (F) a shell to send it back for 400,
//              or punch during the smash's last moment to break it: staggered,
//              it takes double damage.
//   SHIELDBEARER — carries a tower shield that stops bullets from the front.
//              Advances slowly, fires a spread of orbs, bashes up close. Flank
//              it (it turns slowly), shoot the head over the rim, blow it up,
//              or PARRY (F) the bash to knock the shield aside.
//   WARDEN   — the Core's boss, fed by four conduits on the pillars (cut them:
//              it reels), venting its core every third attack; at 60% it feeds
//              on the reactor (rings, a sweeping lance, seekers for anyone
//              hiding), at 25% it melts down (40 s to kill it).
//   SERAPH   — winged, high and far. Charges, then sweeps a beam toward you
//              that turns slower than you run: keep moving, or break line of sight.
//   ANCHOR   — slow heavy. Inside its field you can't dash or grapple; it
//              lobs slow orbs you can parry. Kill it, or keep out of its field.
//   SOVEREIGN — the final boss, alone in the Sanctum: a 3.5 m knight with a
//              greatsword. Dashes at you (and dashes again), chains two sweeps
//              into an overhead cleave, whirls, thrusts, drives eruptions
//              through the floor, leaps onto whatever you're standing on,
//              and throws crescent slashes at range. Keep your distance and
//              he shadow-steps to you or calls blades down on you; shoot him
//              from range and his guard turns it aside. PARRY (F) a sweep,
//              thrust or cleave as it lands to break his guard for 3 s (double damage). Enrages at 50%
//              (phantoms of him join in); at 20% the Sanctum's edges burn.
//
// An enemy reports what it did this tick through `ev` (shots fired, melee hit,
// slam, detonation, summons). GameplayState turns those into projectiles,
// damage, particles and sound, so the AI stays free of rendering and audio.
// =============================================================================
#include <glm/glm.hpp>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <algorithm>
#include "Player.h"  // AABB, Wall, SpatialGrid
#include "Difficulty.h"

// CONDUIT: not a fighter but a spawner pylon (a wave objective, WaveDirector.h):
// it stands still and the wave keeps coming out of it until it's destroyed.
// CONDUCTOR: doesn't attack either; it tethers nearby allies and shields them
// (linkConductors below), so it's the one to kill first.
enum class EnemyType { HUSK, RIPPER, SENTINEL, RAPTOR, BRUTE, MITE, JUGGERNAUT, WARDEN, SOVEREIGN, SHIELDBEARER, CONDUIT,
                       CONDUCTOR, SERAPH, ANCHOR, PENITENT, COUNT };
inline bool isBoss(EnemyType t) { return t == EnemyType::WARDEN || t == EnemyType::SOVEREIGN || t == EnemyType::PENITENT; }

// Hollowed variants (Act II): a regular enemy made harder in one specific way.
//   ENRAGED  faster, shorter wind-ups, hits harder
//   TWINNED  splits in two smaller copies when it dies
//   HALOED   takes a tenth of the damage until a headshot or a parry breaks its halo
enum class Hollow { NONE, ENRAGED, TWINNED, HALOED };
inline bool canBeHollow(EnemyType t) {
    return !isBoss(t) && t != EnemyType::CONDUIT && t != EnemyType::CONDUCTOR;
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
                        WVENT, LANCE, SEEKER, WLUNGE, DETONATE };      // the WARDEN's

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

inline const EnemyStats& statsOf(EnemyType t) {
    static const EnemyStats S[] = {
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
    };
    return S[(int)t];
}

// Everything an enemy did this tick. Cleared at the top of every update().
struct EnemyEvents {
    static constexpr int MAX_SHOTS = 16;
    bool      telegraphStarted = false;
    int       shots = 0;
    glm::vec3 shotOrigin{0.f};
    glm::vec3 shotDir[MAX_SHOTS];
    float     shotSpeed = 16.f, shotDamage = 10.f, shotSize = 1.f;
    bool      shotHeavy = false;   // a JUGGERNAUT siege shell: parry it for a huge hit
    float     shotParry = 0.f;     // > 0: what these shots do when parried back
    bool      beamOn = false;      // SERAPH: its beam is sweeping, from beamFrom to beamTo
    glm::vec3 beamFrom{0.f}, beamTo{0.f};
    bool      meleeHit = false;   float meleeDamage = 0.f;
    bool      slam = false;       float slamRadius = 0.f, slamDamage = 0.f;
    bool      detonated = false;  // MITE blew itself up next to the player
    int       summonMites = 0, summonRippers = 0;
    bool      enraged = false;    // a boss crossed 50% this tick
    // SOVEREIGN: a sword stroke landed (for its slash arc). 0 sweep to his
    // left, 1 sweep to his right, 2 overhead cleave, 3 the cut at a dash's end
    int       slash = -1;
    bool      dashStarted = false, leapStarted = false;
    // SOVEREIGN: blades called down / eruptions out of the floor, marked
    // where they'll land (kind 0 a blade from the sky, 1 an eruption)
    static constexpr int MAX_STRIKES = 32;
    int       strikes = 0;
    glm::vec3 strikePos[MAX_STRIKES];
    int       strikeKind[MAX_STRIKES];
    float     strikeDelay[MAX_STRIKES];
    bool      blinked = false;       // a shadow step: he vanished from blinkFrom
    glm::vec3 blinkFrom{0.f};
    int       phantoms = 0;          // phantoms of him, set to dash at you
    glm::vec3 phantomPos[2], phantomDir[2];
    // PENITENT: a censer sweep landed (0 low: jump it, 1 high: slide under
    // it), a slam / stomp ring, a chain lash marked toward you, incense pools,
    // a ring of embers off its own back, the moment it rises, Husks called up
    int       penSweep = -1;
    bool      penSlam = false, penStomp = false, penEmbers = false, penRose = false, penLash = false;
    glm::vec3 penLashFrom{0.f}, penLashDir{0.f};
    int       penIncense = 0;
    glm::vec3 penIncensePos[3];
    int       penSummon = 0;
    // WARDEN: its phase changed (2 overload, 3 meltdown); a lance began (from,
    // start yaw, which way it sweeps); a seeker marked where you stand; its
    // vent opened; the meltdown went off
    int       wPhase = 0;
    bool      wLance = false; glm::vec3 wLanceFrom{0.f}; float wLanceYaw = 0.f, wLanceSign = 1.f;
    bool      wSeeker = false; glm::vec3 wSeekerAt{0.f};
    bool      wVent = false, wDetonate = false;
};

// What an enemy can sense each tick.
struct EnemyWorld {
    glm::vec3 playerEye{0.f};
    glm::vec3 playerFeet{0.f};
    const Wall* walls = nullptr;
    int  wallCount = 0;
    const SpatialGrid* grid = nullptr;
    AABB bounds{{-1e9f,-1e9f,-1e9f},{1e9f,1e9f,1e9f}}; // arena interior; enemies stay inside
    glm::vec3 playerVel{0.f};      // for leading shots
    const DifficultyTuning* tune = nullptr;   // null: the default difficulty
    const int* dynWalls = nullptr;   // moving platforms (not in the grid): the Descent's cage
    int  dynCount = 0;
    glm::vec3 reactor{0.f};       // the WARDEN's power: where it goes to feed (the Core's reactor)
    bool hasReactor = false;
};

// Ray vs AABB: distance along the ray to the first hit, or -1 on a miss.
inline float rayBoxHit(glm::vec3 o, glm::vec3 d, const AABB& b) {
    glm::vec3 invD{1.f/(d.x+1e-9f), 1.f/(d.y+1e-9f), 1.f/(d.z+1e-9f)};
    glm::vec3 t0 = (b.min-o)*invD, t1 = (b.max-o)*invD;
    glm::vec3 tMin = glm::min(t0,t1), tMax = glm::max(t0,t1);
    float tEnter = std::max({tMin.x,tMin.y,tMin.z});
    float tExit  = std::min({tMax.x,tMax.y,tMax.z});
    if (tEnter > tExit || tExit < 0.f) return -1.f;
    return tEnter > 0.f ? tEnter : tExit;
}

// Does the segment a→b pass through the box?
inline bool segmentHitsBox(glm::vec3 a, glm::vec3 b, const AABB& box) {
    glm::vec3 d = b - a;
    float len = glm::length(d);
    if (len < 1e-4f) return a.x >= box.min.x && a.x <= box.max.x && a.y >= box.min.y && a.y <= box.max.y &&
                            a.z >= box.min.z && a.z <= box.max.z;
    float t = rayBoxHit(a, d / len, box);
    return t >= 0.f && t <= len;
}

inline float frand(float lo, float hi) { return lo + (hi - lo) * (float)(rand() % 10001) / 10000.f; }

struct Enemy {
    EnemyType  type;
    EnemyState state = EnemyState::SPAWNING;
    glm::vec3  position;
    glm::vec3  velocity{0.f};
    float      yaw   = 0.f;   // radians; model faces +Z at yaw 0
    // Where it was at the start of the last physics tick: rendering blends
    // from here to `position` so it moves smoothly at any frame rate
    glm::vec3  prevPosition{0.f};
    float      prevYaw = 0.f;
    // Where it was last drawn (set by the renderer). Hitscan aims at this, so
    // a shot lands on what you saw, not on where the simulation has already
    // moved it (up to a tick ahead)
    glm::vec3  shownPos{0.f};
    bool       hasShown = false;
    float      pitch = 0.f;   // RAPTOR only: nose-down while diving
    float      health, maxHealth;
    // CONDUCTOR tethers, set every tick by linkConductors (indices into this
    // tick's enemy list)
    bool       shielded = false;     // tethered: takes CONDUCTOR_SHIELD x damage
    int        links[3] = {-1, -1, -1};
    int        linkCount = 0;
    glm::vec3  supportAnchor{0.f};   // a CONDUCTOR's: where it wants to hover
    bool       hasAnchor = false;
    bool       alive = true;
    float      beamTimer = 0.f;      // SERAPH: > 0 while sweeping
    glm::vec3  beamPoint{0.f}, beamEnd{0.f};   // SERAPH: where it's aimed; where the beam stops
    int        uid = 0;              // stable id (GameplayState numbers them): who fired a shot
    Hollow     hollow = Hollow::NONE;
    float      scale = 1.f;          // TWINNED copies are smaller
    bool       halo = false;         // HALOED: up until a headshot or parry
    float      haloOpenTimer = 0.f;  // > 0: the halo just broke, it takes double damage
    // Damage taken from a halo: a tenth while it's up, double for 2 s once broken
    float incomingMult() const { return halo ? 0.1f : haloOpenTimer > 0.f ? 2.f : 1.f; }
    void breakHalo() {
        if (!halo) return;
        halo = false;
        haloOpenTimer = 2.f;
        stagger(0.4f);
    }
    bool  splitsOnDeath() const { return hollow == Hollow::TWINNED; }
    // SERAPH: a hit during the charge - a big one, or the head - cancels it
    bool onBeamHit(float dmg, bool head) {
        if (type != EnemyType::SERAPH || attack != AttackKind::BEAM || telegraphTimer <= 0.f) return false;
        if (dmg < 40.f && !head) return false;
        attack = AttackKind::NONE; telegraphTimer = 0.f; attackTimer = 0.f;
        return true;
    }
    float fieldRadius() const { return hollow == Hollow::ENRAGED ? 13.f : 10.f; }   // ANCHOR
    float damageMult() const { return hollow == Hollow::ENRAGED ? 1.25f : 1.f; }   // what its attacks deal
    void setHollow(Hollow h) {
        hollow = canBeHollow(type) ? h : Hollow::NONE;
        halo = hollow == Hollow::HALOED;
    }

    static constexpr float SPAWN_TIME = 0.9f;
    // The boss bar: every boss tell readable, every recovery a real window
    static constexpr float TELL_FLOOR = 0.35f, TELL_FLOOR_FOLLOW = 0.28f, RECOVER_MIN = 0.8f, PHASE_PAUSE = 0.6f;
    bool  tellFollow = false;     // the current tell follows straight on from another (a combo's 2nd stroke)
    float phaseHold  = 0.f;       // a boss's phase just changed: nothing new starts yet
    bool  ragePaused = false, lastPaused = false;
    float spawnTimer = SPAWN_TIME;   // counts down; untargetable while > 0

    // Attack cycle
    AttackKind attack = AttackKind::NONE;   // the attack currently winding up / running
    float telegraphTimer = 0.f, telegraphDuration = 0.f;
    float attackTimer    = 0.f;
    int   attackCount    = 0;
    float recoverTimer   = 0.f;   // after a lunge/dive: back off before re-engaging
    int   burstLeft      = 0;
    float burstTimer     = 0.f;
    float diveTimer      = 0.f;
    glm::vec3 diveDir{0.f};

    // Movement
    float strafeTimer = 0.f, strafeDir = 1.f;
    float avoidSign   = 1.f, avoidTimer = 0.f;
    // Stuck detection: a short look ahead can leave an enemy nudging at a wide
    // obstacle, or zig-zagging along its far side, forever. If it's heading
    // for you but hasn't got any closer for a second, it commits to going
    // round one side until the way to you opens (the other side next time).
    glm::vec3 progressFrom{0.f};
    float progressDist = 0.f;
    float progressAt = 0.f, detourUntil = -1.f, detourSign = 1.f;
    glm::vec3 detourDir{0.f};
    float noLosTimer  = 0.f;      // > 0: reposition to find a clear shot
    float hoverY      = 8.f;   // flyers: height kept above floorY
    float orbitRadius = 12.f;

    // Animation / feedback
    float animPhase     = 0.f;   // walk / flap cycle
    float moveSpeed     = 0.f;   // horizontal speed this tick
    float hitFlashTimer = 0.f;
    float staggerTimer  = 0.f;   // JUGGERNAUT, after its smash was parried
    float floorY        = 0.f;   // hard floor under it (GameplayState sets it each tick)
    float wadeMul       = 1.f;   // wading slows its steps (set each tick)
    float age           = 0.f;

    bool  enraged       = false; // a boss's phase two

    // SOVEREIGN
    int   comboStep  = 0;       // sweeps landed in the current combo
    int   chainLeft  = 0;       // dashes still to come in this chain
    int   crescentLeft = 0;
    float dashTimer  = 0.f;     // > 0 while dashing
    float dashAge    = 0.f;
    glm::vec3 dashFrom{0.f};    // where the last tick of the dash started
    bool  dashHit    = false;   // the dash already caught you
    float leapTimer  = 0.f;     // > 0 while airborne
    // PENITENT
    static constexpr float PEN_RISE_TIME = 2.f, PEN_LASH_FAR = 22.f, PEN_LASH_HIGH = 4.f, PEN_LASH_AFTER = 3.f,
                           PEN_LASH_WARN = 0.7f, PEN_INCENSE_EVERY = 12.f, PEN_SUMMON_EVERY = 25.f, PEN_SCOURGE_EVERY = 6.f;
    int   anchorsLeft  = 0;      // its chains still holding (GameplayState sets it every tick)
    bool  risen        = false;  // every chain broken: it stands and walks
    float riseTimer    = 0.f;
    bool  scourging    = false;  // under a quarter: it lashes itself, the wound on its back open
    float incenseTimer = 6.f, summonTimer = PEN_SUMMON_EVERY, scourgeTimer = 4.f;
    int   comboLeft    = 0, nextSweep = 0;
    float scourgeRest  = 0.f;    // after its ember ring: a breath before the next blow
    // WARDEN (EnemyWarden.h)
    static constexpr float W_PHASE2 = 0.6f, W_PHASE3 = 0.25f, W_VENT_TIME = 2.5f, W_LANCE_VENT = 2.f,
                           W_LANCE_TIME = 2.0943951f / 0.6108652f,   // 120 degrees at 35 a second
                           W_SEEK_AFTER = 4.f, W_SEEK_FAR = 28.f, W_MELT_TIME = 40.f, W_LUNGE_SPEED = 16.f;
    int   wardenPhase  = 1;       // 1 CHARGING, 2 OVERLOAD, 3 MELTDOWN
    int   conduitsLeft = 0;       // conduits feeding it (GameplayState sets it every tick)
    float ventTimer    = 0.f;     // > 0: its core is open
    int   sinceVent    = 0, wardenAttacks = 0;   // attacks since its last vent; its attacks that weren't vents
    float lanceTimer   = 0.f;     // > 0: a lance is sweeping
    float lanceSign    = 1.f;
    float meltClock    = 0.f;     // MELTDOWN: seconds until it goes off
    bool  atReactor    = false;
    glm::vec3 reactorSpot{0.f};
    float spotBest = 1e9f, spotAt = 0.f;   // its walk to the reactor: closest yet, and when
    bool  coreOpen() const { return type == EnemyType::WARDEN && (ventTimer > 0.f || wardenPhase == 3); }
    // A shot into an open weak point: the PENITENT's wound x3, the WARDEN's core x3 venting, x2 in meltdown
    float woundMult() const { return type == EnemyType::WARDEN ? (ventTimer > 0.f ? 3.f : 2.f) : 3.f; }

    bool  chained() const { return type == EnemyType::PENITENT && !risen; }
    float sweepReach() const { return risen ? 18.f : 16.f; }
    float swingTimer = 0.f;     // follow-through after a stroke lands (animation)
    AttackKind lastSwing = AttackKind::NONE;
    int   lastSwingStep = 0;
    bool  grounded   = true;    // standing on something (set by integrate)
    float farTimer   = 0.f;     // how long you've kept your distance (or the high ground)
    int   pursuitCount = 0;
    float whirlTimer = 0.f, whirlHitCd = 0.f;   // > 0 while spinning
    int   whirlStep  = 0;
    float thrustTimer = 0.f;    // > 0 during a thrust's lunge
    bool  thrustHit  = false;
    float blinkFlash = 5.f;     // seconds since a shadow step
    bool  riposte    = false;   // a deflected shot: answer it with a crescent
    float riposteCd  = 0.f;
    static constexpr float PURSUE_RANGE  = 22.f;   // further than this (or 3 m above him) and he comes for you
    static constexpr float DEFLECT_RANGE = 16.f;   // shots from further than this bounce off his guard
    static constexpr float THRUST_SPEED  = 30.f;
    static constexpr float SWING_TIME = 0.24f;
    static constexpr float DASH_SPEED = 34.f;
    bool  killedByBlast = false; // MITE: set when it detonated itself (not shot)

    EnemyEvents ev;

    Enemy(EnemyType t, glm::vec3 pos, float floor = 0.f) : type(t), position(pos), prevPosition(pos) {
        floorY = floor;
        const EnemyStats& s = statsOf(t);
        health = maxHealth = s.health;
        attackTimer = frand(0.f, s.attackEvery * 0.6f);   // desync the squad
        strafeDir   = (rand() & 1) ? 1.f : -1.f;
        animPhase   = frand(0.f, 6.28f);
        if (s.flying) {
            hoverY      = pos.y - floor > 2.f ? pos.y - floor : 8.f;   // above the floor under it
            orbitRadius = frand(9.f, 15.f);
        }
    }

    const EnemyStats& stats() const { return statsOf(type); }
    float radius() const { return stats().radius * scale; }
    float height() const {
        if (type == EnemyType::PENITENT) return (risen ? 11.5f : 8.2f) * scale;   // kneeling, then standing
        return stats().height * scale;
    }
    bool  targetable() const { return alive && state == EnemyState::ACTIVE; }
    bool  staggered() const  { return staggerTimer > 0.f; }
    // The moment a melee blow can be punched back: a JUGGERNAUT's smash (its
    // last 0.4 s), a SOVEREIGN's sweep or cleave (its last quarter second)
    bool  parryWindow() const {
        // THE WARDEN's lunge: its last quarter second, or in flight until it connects (it comes into reach)
        if (type == EnemyType::WARDEN && dashTimer > 0.f && !dashHit) return true;
        if (telegraphTimer <= 0.f) return false;
        if (type == EnemyType::WARDEN) return attack == AttackKind::WLUNGE && telegraphTimer < 0.25f;

        if (type == EnemyType::JUGGERNAUT) return attack == AttackKind::SMASH && telegraphTimer < 0.4f;
        if (type == EnemyType::SHIELDBEARER) return attack == AttackKind::BASH && telegraphTimer < 0.3f;
        if (type == EnemyType::SOVEREIGN)
            return (attack == AttackKind::SWEEP || attack == AttackKind::CLEAVE || attack == AttackKind::THRUST) &&
                   telegraphTimer < 0.25f;
        if (type == EnemyType::PENITENT)
            return (attack == AttackKind::CENSER_LOW || attack == AttackKind::CENSER_HIGH ||
                    attack == AttackKind::PSLAM || attack == AttackKind::PSTOMP) && telegraphTimer < 0.25f;
        return false;
    }
    // SOVEREIGN: does his guard turn aside a shot travelling along dir, fired
    // from `dist` away? From range, from the front, while he isn't mid-attack:
    // his wind-ups, strokes and recoveries are the openings
    bool deflects(glm::vec3 dir, float dist) const {
        if (type != EnemyType::SOVEREIGN || !alive || staggered() || dist < DEFLECT_RANGE) return false;
        if (telegraphTimer > 0.f || recoverTimer > 0.f || dashTimer > 0.f || leapTimer > 0.f ||
            whirlTimer > 0.f || thrustTimer > 0.f) return false;
        glm::vec2 d{-dir.x, -dir.z};
        float l = glm::length(d);
        if (l < 1e-4f) return false;
        return glm::dot(d / l, glm::vec2{std::sin(yaw), std::cos(yaw)}) > 0.5f;
    }
    void onDeflect() { if (riposteCd <= 0.f) { riposte = true; riposteCd = 1.4f; } }
    // How long a parry leaves it broken
    float staggerTime() const { return type == EnemyType::SOVEREIGN ? 3.f : type == EnemyType::WARDEN ? 2.f : type == EnemyType::SHIELDBEARER ? 2.2f : 2.5f; }
    // SHIELDBEARER: does its shield stop a shot travelling along dir? (From
    // the front, while it's standing; a broken one has its shield knocked aside)
    bool blocks(glm::vec3 dir) const {
        if (type != EnemyType::SHIELDBEARER || staggered() || !alive) return false;
        glm::vec2 d{-dir.x, -dir.z};
        float l = glm::length(d);
        if (l < 1e-4f) return false;
        return glm::dot(d / l, glm::vec2{std::sin(yaw), std::cos(yaw)}) > 0.42f;   // within ~65 degrees of its facing
    }
    // Damage multiplier from armor: the JUGGERNAUT shrugs off half unless
    // broken; a broken SOVEREIGN takes half again
    float armorMult() const {
        if (type == EnemyType::WARDEN) return (conduitsLeft > 0 && ventTimer <= 0.f ? 0.5f : 1.f) * (staggered() ? 2.f : 1.f);
        if (type == EnemyType::PENITENT) return (anchorsLeft > 0 ? 0.25f : 1.f) * (staggered() ? 2.f : 1.f);
        if (type == EnemyType::SOVEREIGN) return staggered() ? 2.f : 1.f;
        if (type != EnemyType::JUGGERNAUT) return 1.f;
        return staggered() ? 2.f : 0.5f;
    }
    void stagger(float t) {
        comboLeft = 0;
        staggerTimer = t; attack = AttackKind::NONE; telegraphTimer = 0.f; attackTimer = 0.f;
        comboStep = 0; chainLeft = 0; crescentLeft = 0; dashTimer = 0.f;
        whirlTimer = 0.f; thrustTimer = 0.f; riposte = false;
        if (leapTimer <= 0.f) { velocity.x = velocity.z = 0.f; }
    }
    float telegraphProgress() const {
        return telegraphTimer > 0.f && telegraphDuration > 0.f
             ? 1.f - telegraphTimer / telegraphDuration : 0.f;
    }

    AABB getAABB() const {
        float r = radius();
        return { position + glm::vec3{-r, 0.f, -r}, position + glm::vec3{r, height(), r} };
    }

    // Returns true if this hit killed the enemy.
    bool takeDamage(float dmg) {
        if (!targetable()) return false;
        health -= dmg;
        hitFlashTimer = 0.12f;
        if (health <= 0.f) {
            health = 0.f;
            alive  = false;
            state  = EnemyState::DEAD;
            return true;
        }
        if (isBoss(type) && type != EnemyType::WARDEN && !enraged && health < maxHealth * 0.5f) {
            enraged    = true;
            ev.enraged = true;
        }
        return false;
    }

    void update(float dt, const EnemyWorld& w) {
        ev = EnemyEvents{};
        if (!alive) return;
        tune_    = w.tune ? w.tune : &difficulty(DIFFICULTY_DEFAULT);
        if (hollow == Hollow::ENRAGED) {   // on top of the difficulty
            hollowTune_ = *tune_;
            hollowTune_.moveSpeed  *= 1.3f;
            hollowTune_.windup     *= 0.65f;
            hollowTune_.attackRate /= 0.75f;
            tune_ = &hollowTune_;
        }
        leadVel_ = w.playerVel * tune_->lead;
        age += dt;
        if (hitFlashTimer > 0.f) hitFlashTimer -= dt;
        if (haloOpenTimer > 0.f) haloOpenTimer = std::max(0.f, haloOpenTimer - dt);

        if (state == EnemyState::SPAWNING) {
            spawnTimer -= dt;
            if (spawnTimer <= 0.f) { spawnTimer = 0.f; state = EnemyState::ACTIVE; }
            // Face the player while materialising
            glm::vec3 d = w.playerFeet - position;
            if (glm::length(glm::vec2(d.x, d.z)) > 0.01f) yaw = std::atan2(d.x, d.z);
            return;
        }

        bool resolve = false;   // telegraph ran out this tick
        if (telegraphTimer > 0.f) {
            telegraphTimer -= dt;
            if (telegraphTimer <= 0.f) { telegraphTimer = 0.f; resolve = true; }
        }
        if (recoverTimer > 0.f) recoverTimer -= dt;
        if (staggerTimer > 0.f) {   // broken: stands there, open to punishment
            staggerTimer -= dt;
            velocity.x = velocity.z = 0.f;
            integrate(dt, w);
            return;
        }
        if (noLosTimer   > 0.f) noLosTimer   -= dt;
        if (avoidTimer   > 0.f) avoidTimer   -= dt;

        switch (type) {
            case EnemyType::HUSK:     thinkHusk(dt, w, resolve);     break;
            case EnemyType::RIPPER:   thinkRipper(dt, w, resolve);   break;
            case EnemyType::SENTINEL: thinkSentinel(dt, w, resolve); break;
            case EnemyType::RAPTOR:   thinkRaptor(dt, w, resolve);   break;
            case EnemyType::BRUTE:    thinkBrute(dt, w, resolve);    break;
            case EnemyType::MITE:     thinkMite(dt, w, resolve);     break;
            case EnemyType::JUGGERNAUT: thinkJuggernaut(dt, w, resolve); break;
            case EnemyType::WARDEN:   thinkWarden(dt, w, resolve);   break;
            case EnemyType::SOVEREIGN: thinkSovereign(dt, w, resolve); break;
            case EnemyType::SHIELDBEARER: thinkShieldbearer(dt, w, resolve); break;
            case EnemyType::CONDUCTOR: thinkConductor(dt, w); break;
            case EnemyType::SERAPH:   thinkSeraph(dt, w, resolve);   break;
            case EnemyType::ANCHOR:   thinkAnchor(dt, w, resolve);   break;
            case EnemyType::PENITENT: thinkPenitent(dt, w, resolve); break;
            default: break;
        }
        integrate(dt, w);
    }

private:
    const DifficultyTuning* tune_ = &difficulty(DIFFICULTY_DEFAULT);   // this tick's difficulty
    DifficultyTuning hollowTune_{};   // ENRAGED: the difficulty, sharpened
    glm::vec3 leadVel_{0.f};      // how far ahead to aim: player velocity × lead

    // ---- shared helpers ------------------------------------------------------
    glm::vec3 flatTo(const glm::vec3& target) const {
        glm::vec3 d = target - position; d.y = 0.f; return d;
    }
    static glm::vec3 norm2(glm::vec3 v) {
        v.y = 0.f; float l = glm::length(v); return l > 1e-4f ? v / l : glm::vec3{0.f};
    }
    static glm::vec3 rotY(glm::vec3 v, float a) {
        float c = std::cos(a), s = std::sin(a);
        return { v.x * c + v.z * s, v.y, -v.x * s + v.z * c };
    }

    void turnToward(glm::vec3 dir, float dt, float rate) {
        if (glm::length(glm::vec2(dir.x, dir.z)) < 1e-4f) return;
        float target = std::atan2(dir.x, dir.z);
        float diff = std::remainder(target - yaw, 6.2831853f);
        float step = rate * dt;
        yaw += glm::clamp(diff, -step, step);
    }

    void startAttack(AttackKind k, float windup, bool followUp = false) {
        windup *= tune_->windup;
        if (type == EnemyType::SOVEREIGN || type == EnemyType::WARDEN)   // never too quick to read, whatever sped it up
            windup = std::max(windup, followUp ? TELL_FLOOR_FOLLOW : TELL_FLOOR);
        attack            = k;
        tellFollow        = followUp;
        telegraphDuration = windup;
        telegraphTimer    = windup;
        ev.telegraphStarted = true;
    }
    // A boss's recovery: never shorter than RECOVER_MIN (the window to punish it)
    void recover(float t) { recoverTimer = std::max(t, RECOVER_MIN); }

    bool blockedAt(glm::vec3 p, const EnemyWorld& w) const {
        if (!w.walls) return false;
        float r = radius();
        AABB box{ p + glm::vec3{-r, 0.3f, -r}, p + glm::vec3{r, height(), r} };
        static std::vector<int> cands;
        if (w.grid) w.grid->query(box, cands);
        else { cands.clear(); for (int i = 0; i < w.wallCount; ++i) cands.push_back(i); }
        for (int i : cands) {
            const AABB& b = w.walls[i].box;
            if (box.max.x > b.min.x && box.min.x < b.max.x &&
                box.max.y > b.min.y && box.min.y < b.max.y &&
                box.max.z > b.min.z && box.min.z < b.max.z) return true;
        }
        return false;
    }

    // Gunners and Brutes hold their perch: on a raised surface they treat a
    // drop as a wall. Rippers and Mites happily leap down after you.
    bool ledgeAware() const {
        return !stats().flying && type != EnemyType::RIPPER && type != EnemyType::MITE && type != EnemyType::SOVEREIGN;
    }

    // Is there something to stand on under p (within a step of its height)?
    bool supportedAt(glm::vec3 p, const EnemyWorld& w) const {
        if (p.y < floorY + 0.3f || !w.walls) return true;   // the floor under it (Y 0, or a basin's)
        AABB q{p + glm::vec3{-0.05f, -1.4f, -0.05f}, p + glm::vec3{0.05f, 0.6f, 0.05f}};
        static std::vector<int> cands;
        if (w.grid) w.grid->query(q, cands);
        else { cands.clear(); for (int i = 0; i < w.wallCount; ++i) cands.push_back(i); }
        for (int i : cands) {
            const AABB& b = w.walls[i].box;
            if (p.x >= b.min.x && p.x <= b.max.x && p.z >= b.min.z && p.z <= b.max.z &&
                b.max.y >= p.y - 1.4f && b.max.y <= p.y + 0.6f) return true;
        }
        for (int k = 0; k < w.dynCount; ++k) {   // moving platforms (the Descent's cage) aren't in the grid
            const AABB& b = w.walls[w.dynWalls[k]].box;
            if (p.x >= b.min.x && p.x <= b.max.x && p.z >= b.min.z && p.z <= b.max.z &&
                b.max.y >= p.y - 1.4f && b.max.y <= p.y + 0.6f) return true;
        }
        return false;
    }

    bool canStepTo(glm::vec3 p, const EnemyWorld& w) const {
        if (blockedAt(p, w)) return false;
        return !(ledgeAware() && position.y > floorY + 0.3f && !supportedAt(p, w));
    }

    // Feeler steering: if the way ahead is blocked, try turning ±45/90/135°
    // (keeping the side that worked last time) so enemies slide around cover
    // instead of grinding into it. No pathfinding: arenas are open by design.
    glm::vec3 steer(glm::vec3 want, const EnemyWorld& w) {
        want = norm2(want);
        if (glm::length(want) < 0.5f || stats().flying) return want;
        float probe = radius() + 0.9f;
        if (canStepTo(position + want * probe, w)) return want;
        static const float ANG[] = {0.785f, 1.571f, 2.356f};
        for (float a : ANG)
            for (float sgn : {avoidSign, -avoidSign}) {
                glm::vec3 d = rotY(want, a * sgn);
                if (canStepTo(position + d * probe, w)) {
                    avoidSign = sgn; avoidTimer = 0.6f;
                    return d;
                }
            }
        return glm::vec3{0.f};
    }

    void setMove(glm::vec3 dir, float speed, const EnemyWorld& w) {
        glm::vec3 want = norm2(dir);
        glm::vec3 toP = flatTo(w.playerFeet);
        float distP = glm::length(toP);
        bool approaching = distP > 4.f && glm::dot(want, toP / std::max(distP, 1e-3f)) > 0.3f;
        if (!stats().flying && speed > 1.f && glm::length(want) > 0.5f && (approaching || age < detourUntil)) {
            if (age - progressAt > 1.f) {
                bool noCloser = progressDist - distP < 1.f;
                if (progressAt > 0.f && approaching && noCloser && age >= detourUntil) {
                    // Stuck again straight after a detour: that side's blocked too, try the other
                    detourSign = age < detourUntil + 0.5f ? -detourSign : avoidSign;
                    detourDir = rotY(toP / std::max(distP, 1e-3f), 1.571f * detourSign);
                    detourUntil = age + 1.4f;
                }
                progressFrom = position; progressAt = age; progressDist = distP;
            }
            if (age < detourUntil) {
                dir = detourDir + want * 0.25f;
                avoidSign = detourSign;   // and slide round obstacles on the same side
                // Round the corner: done as soon as the straight way opens up
                // (a few metres of it, so a corner isn't cut straight back into);
                // still blocked when the time's up, keep going
                glm::vec3 straight = toP / std::max(distP, 1e-3f);
                bool open = canStepTo(position + straight * (radius() + 1.2f), w) &&
                            !blockedAt(position + straight * (radius() + 3.f), w);   // walls only: a ledge further on is fine
                if (open && age > detourUntil - 1.1f) detourUntil = age;
                else if (!open && detourUntil - age < 0.1f) detourUntil = age + 0.5f;
            }
        }
        glm::vec3 d = steer(dir, w);
        speed *= tune_->moveSpeed;
        velocity.x = d.x * speed;
        velocity.z = d.z * speed;
    }

    bool lineOfSight(const glm::vec3& from, const EnemyWorld& w) const {
        glm::vec3 d = w.playerEye - from;
        float dist = glm::length(d);
        if (dist < 1e-3f || !w.walls) return true;
        d /= dist;
        for (int i = 0; i < w.wallCount; ++i) {
            float t = rayBoxHit(from, d, w.walls[i].box);
            if (t > 0.f && t < dist) return false;
        }
        return true;
    }

    glm::vec3 eyePos() const { return position + glm::vec3{0.f, height() * 0.85f, 0.f}; }

    // Fire n shots fanned across `spread` radians. Aimed ahead of a moving
    // player by the difficulty's lead (where they'll be when the shot lands).
    void fireAt(const glm::vec3& target, int n, float spread, float speed, float dmg, float size = 1.f) {
        speed *= tune_->shotSpeed;
        ev.shotOrigin = eyePos();
        ev.shotSpeed  = speed;
        ev.shotDamage = dmg;
        ev.shotSize   = size;
        glm::vec3 aim = target;
        if (speed > 1.f) {
            float flight = glm::length(target - ev.shotOrigin) / speed;
            aim += leadVel_ * std::min(flight, 1.2f);
        }
        glm::vec3 base = aim - ev.shotOrigin;
        float len = glm::length(base);
        base = len > 1e-4f ? base / len : glm::vec3{0,0,1};
        n = std::min(n, EnemyEvents::MAX_SHOTS);
        for (int i = 0; i < n; ++i) {
            float a = (n == 1) ? 0.f : -spread * 0.5f + spread * (float)i / (float)(n - 1);
            ev.shotDir[i] = glm::normalize(rotY(base, a));
        }
        ev.shots = n;
    }

    // Strafe-and-hold-range movement used by the gunners.
    void rangedMove(float dt, const EnemyWorld& w, float nearR, float farR, float speed) {
        glm::vec3 to = flatTo(w.playerFeet);
        float d = glm::length(to);
        glm::vec3 dir = norm2(to);
        glm::vec3 side{-dir.z, 0.f, dir.x};
        strafeTimer -= dt;
        if (strafeTimer <= 0.f) { strafeTimer = frand(1.6f, 3.2f); strafeDir = -strafeDir; }
        glm::vec3 mv;
        if (noLosTimer > 0.f)  mv = side * strafeDir + dir * 0.35f;      // sidestep out from behind cover
        else if (d < nearR)    mv = -dir + side * strafeDir * 0.7f;
        else if (d > farR)     mv = dir + side * strafeDir * 0.3f;
        else                   mv = side * strafeDir;
        setMove(mv, speed, w);
    }

    // Ticks the attack clock; returns true when it's time to start a new attack.
    bool attackReady(float dt) {
        if (telegraphTimer > 0.f || burstLeft > 0 || diveTimer > 0.f || beamTimer > 0.f) return false;
        attackTimer += dt * (enraged ? 1.5f : 1.f) * tune_->attackRate;
        if (attackTimer < stats().attackEvery) return false;
        attackTimer = 0.f;
        return true;
    }

    // No clear shot: sidestep for a moment and look again soon, rather than
    // waiting out a whole attack cycle behind cover.
    void blockedShot() {
        noLosTimer  = 1.2f;
        attackTimer = stats().attackEvery - 0.35f;
    }

    // ---- per-type behaviour --------------------------------------------------
    void thinkHusk(float dt, const EnemyWorld& w, bool resolve) {
        float speed = stats().speed * (telegraphTimer > 0.f ? 0.25f : 1.f);
        rangedMove(dt, w, 8.f, 17.f, speed);
        turnToward(flatTo(w.playerFeet), dt, 6.f);
        if (resolve && attack == AttackKind::SHOT) {
            fireAt(w.playerEye, 1, 0.f, 21.f, 12.f);
            attack = AttackKind::NONE;
        }
        if (attackReady(dt)) {
            if (lineOfSight(eyePos(), w)) startAttack(AttackKind::SHOT, stats().telegraph);
            else blockedShot();
        }
    }

    void thinkRipper(float dt, const EnemyWorld& w, bool resolve) {
        glm::vec3 to = flatTo(w.playerFeet);
        float d = glm::length(to);
        glm::vec3 dir = norm2(to);
        glm::vec3 side{-dir.z, 0.f, dir.x};
        strafeTimer -= dt;
        if (strafeTimer <= 0.f) { strafeTimer = frand(0.6f, 1.2f); strafeDir = -strafeDir; }

        if (attack == AttackKind::LUNGE) {
            // Crouch almost still, then spring on the last tenth of a second
            float spd = telegraphTimer < 0.1f ? 15.f : 0.8f;
            velocity.x = dir.x * spd; velocity.z = dir.z * spd;
            if (resolve) {
                attack = AttackKind::NONE;
                float dy = w.playerFeet.y - position.y;
                if (d < 2.7f && std::fabs(dy) < 2.5f) { ev.meleeHit = true; ev.meleeDamage = 18.f; }
                recoverTimer = 0.45f;
            }
        } else if (recoverTimer > 0.f) {
            setMove(-dir + side * strafeDir, stats().speed * 0.7f, w);   // hit and run
        } else {
            float zig = d < 5.f ? 0.15f : 0.75f;
            setMove(dir + side * strafeDir * zig, stats().speed, w);
            attackTimer += dt * tune_->attackRate;
            if (attackTimer >= stats().attackEvery && d < 4.2f) {
                attackTimer = 0.f;
                startAttack(AttackKind::LUNGE, stats().telegraph);
            }
        }
        turnToward(to, dt, 10.f);
    }

    void thinkSentinel(float dt, const EnemyWorld& w, bool resolve) {
        float speed = (telegraphTimer > 0.f || burstLeft > 0) ? 0.f : stats().speed;
        rangedMove(dt, w, 15.f, 26.f, speed);
        turnToward(flatTo(w.playerFeet), dt, 4.f);
        if (resolve && attack == AttackKind::BURST) {
            attack = AttackKind::NONE;
            // The laser warned you: if you broke line of sight, the shot is lost.
            if (lineOfSight(eyePos(), w)) { burstLeft = 3; burstTimer = 0.f; }
            else blockedShot();
        }
        if (burstLeft > 0) {
            burstTimer -= dt;
            if (burstTimer <= 0.f) {
                fireAt(w.playerEye, 1, 0.f, 34.f, 9.f, 0.8f);
                --burstLeft; burstTimer = 0.12f;
            }
        }
        if (attackReady(dt)) {
            if (lineOfSight(eyePos(), w)) startAttack(AttackKind::BURST, stats().telegraph);
            else blockedShot();
        }
    }

    void thinkRaptor(float dt, const EnemyWorld& w, bool resolve) {
        animPhase += dt * (diveTimer > 0.f ? 5.f : 9.f);
        glm::vec3 to3 = w.playerEye - position;
        glm::vec3 to  = flatTo(w.playerFeet);
        float d = glm::length(to);
        glm::vec3 dir = norm2(to);
        glm::vec3 side{-dir.z, 0.f, dir.x};

        if (diveTimer > 0.f) {
            diveTimer -= dt;
            velocity = diveDir * 17.f;
            pitch = glm::mix(pitch, 0.7f, std::min(1.f, dt * 8.f));
            if (glm::length(w.playerEye - position) < 1.8f) {
                ev.meleeHit = true; ev.meleeDamage = 16.f;
                diveTimer = 0.f; recoverTimer = 1.2f;
            }
            if (diveTimer <= 0.f) recoverTimer = std::max(recoverTimer, 1.0f);
            turnToward(diveDir, dt, 8.f);
            return;
        }
        pitch = glm::mix(pitch, 0.f, std::min(1.f, dt * 4.f));

        strafeTimer -= dt;
        if (strafeTimer <= 0.f) { strafeTimer = frand(2.5f, 4.5f); strafeDir = -strafeDir; }
        // Orbit: tangent plus a radial correction toward the preferred radius
        float radial = glm::clamp((d - orbitRadius) * 0.25f, -1.f, 1.f);
        glm::vec3 mv = norm2(side * strafeDir + dir * radial);
        velocity.x = mv.x * stats().speed;
        velocity.z = mv.z * stats().speed;
        float targetY = floorY + (recoverTimer > 0.f ? hoverY + 2.f : hoverY);
        velocity.y = glm::clamp((targetY - position.y) * 3.f, -8.f, 8.f);
        turnToward(glm::vec3{velocity.x, 0.f, velocity.z}, dt, 5.f);

        if (resolve) {
            if (attack == AttackKind::SHOT) fireAt(w.playerEye, 1, 0.f, 18.f, 9.f);
            else if (attack == AttackKind::DIVE) {
                float l = glm::length(to3);
                diveDir   = l > 1e-3f ? to3 / l : glm::vec3{0,-1,0};
                diveTimer = 1.3f;
            }
            attack = AttackKind::NONE;
        }
        if (recoverTimer <= 0.f && attackReady(dt)) {
            ++attackCount;
            if (attackCount % 3 == 0) startAttack(AttackKind::DIVE, 0.55f);
            else if (lineOfSight(eyePos(), w)) startAttack(AttackKind::SHOT, stats().telegraph);
        }
    }

    void thinkBrute(float dt, const EnemyWorld& w, bool resolve) {
        glm::vec3 to = flatTo(w.playerFeet);
        float d = glm::length(to);
        if (telegraphTimer > 0.f) {
            velocity.x = velocity.z = 0.f;
        } else {
            setMove(to, stats().speed * (d > 16.f ? 1.35f : 1.f), w);
            animPhase += dt * 4.f;
        }
        turnToward(to, dt, telegraphTimer > 0.f ? 1.f : 3.f);
        if (resolve) {
            if (attack == AttackKind::SLAM) {
                ev.slam = true; ev.slamRadius = 8.f; ev.slamDamage = 32.f;
            } else if (attack == AttackKind::LOB) {
                fireAt(w.playerEye, 1, 0.f, 15.f, 18.f, 2.2f);
            }
            attack = AttackKind::NONE;
        }
        if (attackReady(dt)) {
            if (d < 7.f) startAttack(AttackKind::SLAM, stats().telegraph);
            else if (d > 12.f && lineOfSight(eyePos(), w)) startAttack(AttackKind::LOB, 0.7f);
            else attackTimer = stats().attackEvery * 0.7f;  // close the gap, re-check soon
        }
    }

    void thinkMite(float dt, const EnemyWorld& w, bool resolve) {
        glm::vec3 to = flatTo(w.playerFeet);
        float d = glm::length(to);
        glm::vec3 dir = norm2(to);
        glm::vec3 side{-dir.z, 0.f, dir.x};
        animPhase += dt * 14.f;
        if (attack == AttackKind::FUSE) {
            setMove(dir, 2.5f, w);
            if (resolve) {
                ev.detonated  = true;
                killedByBlast = true;
                alive = false; state = EnemyState::DEAD; health = 0.f;
                return;
            }
        } else {
            float wobble = std::sin(age * 7.f + animPhase) * 0.35f;
            setMove(dir + side * wobble, stats().speed, w);
            if (d < 2.6f && std::fabs(w.playerFeet.y - position.y) < 2.f)
                startAttack(AttackKind::FUSE, stats().telegraph);
        }
        turnToward(to, dt, 12.f);
    }

    void thinkJuggernaut(float dt, const EnemyWorld& w, bool resolve) {
        glm::vec3 to = flatTo(w.playerFeet);
        float d = glm::length(to);
        if (telegraphTimer > 0.f) {
            velocity.x = velocity.z = 0.f;   // planted for the wind-up
        } else {
            // Advance steadily; it wants you close enough to smash
            glm::vec3 dir = norm2(to), side{-dir.z, 0.f, dir.x};
            strafeTimer -= dt;
            if (strafeTimer <= 0.f) { strafeTimer = frand(2.5f, 4.f); strafeDir = -strafeDir; }
            setMove(d > 4.f ? dir + side * strafeDir * 0.25f : side * strafeDir, stats().speed, w);
            animPhase += dt * 3.f;
        }
        turnToward(to, dt, telegraphTimer > 0.f ? 1.4f : 2.5f);
        if (resolve) {
            if (attack == AttackKind::SHELL) {
                fireAt(w.playerEye, 1, 0.f, 13.f, 45.f, 3.0f);
                ev.shotHeavy = true;
            } else if (attack == AttackKind::SMASH) {
                float dy = w.playerFeet.y - position.y;
                if (d < 4.6f && std::fabs(dy) < 2.5f) { ev.meleeHit = true; ev.meleeDamage = 40.f; }
                ev.slam = true; ev.slamRadius = 3.5f; ev.slamDamage = 0.f;   // the dust ring, no extra damage
            }
            attack = AttackKind::NONE;
            recoverTimer = 0.5f;
        }
        if (recoverTimer <= 0.f && attackReady(dt)) {
            if (d < 5.f) startAttack(AttackKind::SMASH, 0.95f);
            else if (lineOfSight(eyePos(), w)) startAttack(AttackKind::SHELL, stats().telegraph);
            else blockedShot();
        }
    }

    void thinkWarden(float dt, const EnemyWorld& w, bool resolve);   // EnemyWarden.h

    // ---- the SHIELDBEARER ------------------------------------------------------
    // Walks you down behind its shield, turning slowly (that's the opening:
    // get round it). Mid range it fires a spread of three orbs; close in it
    // winds up a shield bash you can parry.
    void thinkShieldbearer(float dt, const EnemyWorld& w, bool resolve) {
        glm::vec3 to = flatTo(w.playerFeet);
        float d = glm::length(to);
        glm::vec3 dir = norm2(to);
        if (telegraphTimer > 0.f) {
            float spd = attack == AttackKind::BASH && d > 1.8f ? 1.5f : 0.f;
            velocity.x = dir.x * spd; velocity.z = dir.z * spd;
        } else {
            glm::vec3 side{-dir.z, 0.f, dir.x};
            strafeTimer -= dt;
            if (strafeTimer <= 0.f) { strafeTimer = frand(2.f, 3.5f); strafeDir = -strafeDir; }
            glm::vec3 mv = d > 2.5f ? dir + side * strafeDir * 0.2f : side * strafeDir;
            if (noLosTimer > 0.f && d > 2.5f) mv = side * strafeDir + dir * 0.35f;   // sidestep out from behind cover
            setMove(mv, stats().speed, w);
        }
        turnToward(to, dt, 1.8f);   // slow to turn: flank it
        if (resolve) {
            if (attack == AttackKind::BASH) {
                float dy = w.playerFeet.y - position.y;
                glm::vec3 fwd{std::sin(yaw), 0.f, std::cos(yaw)};
                if (d < 3.4f && std::fabs(dy) < 2.f && (d < 1.f || glm::dot(fwd, to / d) > 0.5f)) {
                    ev.meleeHit = true; ev.meleeDamage = 20.f;
                }
            } else if (attack == AttackKind::SHOT) {
                fireAt(w.playerEye, 3, 0.35f, 17.f, 9.f);
            }
            attack = AttackKind::NONE;
        }
        if (attackReady(dt)) {
            if (d < 3.2f) startAttack(AttackKind::BASH, 0.6f);
            else if (d < 16.f && lineOfSight(eyePos(), w)) startAttack(AttackKind::SHOT, stats().telegraph);
            else if (d < 16.f) blockedShot();   // in range but behind cover: step out
            else attackTimer = stats().attackEvery * 0.6f;
        }
        if (d < 3.2f && telegraphTimer <= 0.f && attackTimer > stats().attackEvery * 0.5f)
            startAttack(AttackKind::BASH, 0.6f);   // too close to wait out the clock
    }

    // ---- the CONDUCTOR --------------------------------------------------------
    // Hovers behind the allies it shields (supportAnchor), weaving so it isn't
    // a sitting target, and backs off if you close in. No attack of its own.
    // Drifts high and far; charges (1 s), then sweeps a beam whose ground point
    // starts 5 m to one side and turns toward you at 8 m/s for 3 s. Walls cut it.
    void thinkSeraph(float dt, const EnemyWorld& w, bool resolve) {
        animPhase += dt * 4.f;
        glm::vec3 to = flatTo(w.playerFeet);
        float d = glm::length(to);
        glm::vec3 dir = norm2(to), side{-dir.z, 0.f, dir.x};
        bool busy = telegraphTimer > 0.f || beamTimer > 0.f;
        if (busy) { velocity.x = velocity.z = 0.f; }
        else {
            strafeTimer -= dt;
            if (strafeTimer <= 0.f) { strafeTimer = frand(3.f, 5.f); strafeDir = -strafeDir; }
            float radial = d < 18.f ? -1.f : d > 28.f ? 1.f : 0.f;
            glm::vec3 mv = norm2(side * strafeDir + dir * radial);
            velocity.x = mv.x * stats().speed * tune_->moveSpeed;
            velocity.z = mv.z * stats().speed * tune_->moveSpeed;
        }
        float targetY = floorY + glm::clamp(hoverY, 10.f, 14.f);
        velocity.y = glm::clamp((targetY - position.y) * 2.f, -6.f, 6.f);
        turnToward(to, dt, 3.f);
        glm::vec3 feet = w.playerFeet;
        if (resolve && attack == AttackKind::BEAM) { beamTimer = 3.f; attack = AttackKind::NONE; }
        if (beamTimer > 0.f) {
            beamTimer -= dt;
            glm::vec3 gap = feet - beamPoint;
            float l = glm::length(gap), step = 8.f * dt;
            beamPoint = l <= step ? feet : beamPoint + gap / l * step;
            glm::vec3 from = eyePos(), seg = beamPoint - from;
            float len = glm::length(seg);
            beamEnd = beamPoint;
            if (w.walls && len > 1e-3f) {
                glm::vec3 u = seg / len;
                AABB q{glm::min(from, beamPoint) - glm::vec3{0.5f}, glm::max(from, beamPoint) + glm::vec3{0.5f}};
                static std::vector<int> cands;
                if (w.grid) w.grid->query(q, cands);
                else { cands.clear(); for (int i = 0; i < w.wallCount; ++i) cands.push_back(i); }
                float best = len;
                for (int i : cands) { float t = rayBoxHit(from, u, w.walls[i].box); if (t > 0.f && t < best) best = t; }
                beamEnd = from + u * best;
            }
            ev.beamOn = true; ev.beamFrom = from; ev.beamTo = beamEnd;
            if (beamTimer <= 0.f) { beamTimer = 0.f; attackTimer = 0.f; }
            return;
        }
        if (attackReady(dt) && lineOfSight(eyePos(), w)) {
            // Start 5 m off: behind where you're heading, or to one side if you're still
            glm::vec2 v{w.playerVel.x, w.playerVel.z};
            glm::vec3 off = glm::length(v) > 1.f ? -glm::vec3{v.x, 0.f, v.y} / glm::length(v) : side * strafeDir;
            beamPoint = feet + off * 5.f;
            startAttack(AttackKind::BEAM, 1.f);
        }
    }

    // Walks in until you're 8 m off (inside its field), holds, and lobs a
    // telegraphed volley of three slow orbs that parry back for 120
    void thinkAnchor(float dt, const EnemyWorld& w, bool resolve) {
        glm::vec3 to = flatTo(w.playerFeet);
        float d = glm::length(to);
        if (telegraphTimer > 0.f || d < 8.f) velocity.x = velocity.z = 0.f;
        else { setMove(to, stats().speed, w); animPhase += dt * 3.f; }
        turnToward(to, dt, 1.5f);
        if (resolve && attack == AttackKind::VOLLEY) {
            fireAt(w.playerEye, 3, 0.12f, 11.f, 16.f, 1.8f);
            ev.shotParry = 120.f;
            attack = AttackKind::NONE;
        }
        if (attackReady(dt)) {
            if (lineOfSight(eyePos(), w)) startAttack(AttackKind::VOLLEY, 1.f);
            else attackTimer = stats().attackEvery * 0.6f;
        }
    }

    void thinkConductor(float dt, const EnemyWorld& w) {
        animPhase += dt * 3.f;
        glm::vec3 toP = flatTo(w.playerFeet);
        float dP = glm::length(toP);
        glm::vec3 target = hasAnchor ? supportAnchor
                         : w.playerFeet - norm2(toP) * 18.f + glm::vec3{0.f, hoverY, 0.f};
        glm::vec3 to = target - position;
        glm::vec3 flat{to.x, 0.f, to.z};
        glm::vec3 mv = glm::length(flat) > 0.5f ? norm2(flat) * std::min(1.f, glm::length(flat) / 3.f) : glm::vec3{0.f};
        glm::vec3 side{-toP.z, 0.f, toP.x};
        mv += norm2(side) * std::sin(age * 1.7f) * 0.6f;               // weave
        if (dP < 9.f) mv -= norm2(toP) * (9.f - dP) / 3.f;             // too close: back off
        velocity.x = mv.x * stats().speed;
        velocity.z = mv.z * stats().speed;
        float ty = std::max(target.y, w.playerFeet.y + 3.5f) + std::sin(age * 2.3f) * 0.4f;
        velocity.y = glm::clamp((ty - position.y) * 2.5f, -6.f, 6.f);
        turnToward(toP, dt, 3.f);
    }

    // ---- the SOVEREIGN ---------------------------------------------------------
    // A duelist. Every stroke is telegraphed (blade raised, eyes flaring) and
    // every one can be dodged; what makes him hard is that they come in
    // strings. Out of a dash he dashes again or goes straight into a combo;
    // a combo is two sweeps and an overhead cleave whose shockwave you jump.
    // Standing on a platform doesn't save you: he leaps up to you. Enraged
    // (below half health) he winds up faster, chains longer and throws two
    // crescents at a time.
    static constexpr float SWEEP_REACH  = 5.2f;
    static constexpr float CLEAVE_REACH = 5.8f;

    // Is the player inside the arc in front of him (half-angle in radians)?
    bool inArc(const EnemyWorld& w, float reach, float halfAngle) const {
        glm::vec3 to = flatTo(w.playerFeet);
        float d = glm::length(to);
        if (d > reach || std::fabs(w.playerFeet.y - position.y) > 2.6f) return false;
        if (d < 1.2f) return true;
        glm::vec3 fwd{std::sin(yaw), 0.f, std::cos(yaw)};
        return glm::dot(fwd, to / d) > std::cos(halfAngle);
    }

    void addStrike(glm::vec3 p, int kind, float delay) {
        if (ev.strikes >= EnemyEvents::MAX_STRIKES) return;
        ev.strikePos[ev.strikes] = p; ev.strikeKind[ev.strikes] = kind; ev.strikeDelay[ev.strikes] = delay;
        ++ev.strikes;
    }
    bool insideBounds(glm::vec3 p, const EnemyWorld& w, float margin) const {
        return p.x > w.bounds.min.x + margin && p.x < w.bounds.max.x - margin &&
               p.z > w.bounds.min.z + margin && p.z < w.bounds.max.z - margin;
    }

    void thinkPenitent(float dt, const EnemyWorld& w, bool resolve);   // EnemyPenitent.h

    void thinkSovereign(float dt, const EnemyWorld& w, bool resolve) {
        const float rage = enraged ? 1.f : 0.f;
        const bool  last = health < maxHealth * 0.2f;     // the last stand: everything comes quicker
        const float quick = last ? 0.8f : 1.f;            // wind-up multiplier
        glm::vec3 to  = flatTo(w.playerFeet);
        float d       = glm::length(to);
        glm::vec3 dir = norm2(to);
        float dy      = w.playerFeet.y - position.y;
        if (swingTimer > 0.f) swingTimer -= dt;
        if (riposteCd > 0.f) riposteCd -= dt;
        if (blinkFlash < 5.f) blinkFlash += dt;
        // A phase change (enraged at half, the last stand at a fifth): a breath before anything new
        if (enraged && !ragePaused) { ragePaused = true; phaseHold = PHASE_PAUSE; }
        if (last && !lastPaused)    { lastPaused = true; phaseHold = PHASE_PAUSE; }
        if (phaseHold > 0.f) phaseHold -= dt;

        // Keeping your distance (or the high ground) doesn't last: the
        // longer you do it, the sooner he comes for you
        bool away = d > PURSUE_RANGE || dy > 3.f;
        farTimer = away ? farTimer + dt : std::max(0.f, farTimer - 2.f * dt);

        // ---- in the air (LEAP): gravity does the work; land with a slam ----
        if (leapTimer > 0.f) {
            leapTimer -= dt;
            turnToward(to, dt, 4.f);
            bool landed = grounded && velocity.y <= 0.f && leapTimer < 1.2f;
            if (landed || leapTimer <= 0.f) {
                leapTimer = 0.f;
                velocity.x = velocity.z = 0.f;
                ev.slam = true; ev.slamRadius = 8.f; ev.slamDamage = 24.f;
                ev.slash = 2; lastSwing = AttackKind::CLEAVE; swingTimer = SWING_TIME;
                recover(0.65f - 0.25f * rage);
            }
            return;
        }

        // ---- dashing: a straight line at 34 m/s, cutting at the end --------
        if (dashTimer > 0.f) {
            // How far the last tick actually carried him (walls stop a dash)
            float made = dashAge > 0.f ? glm::length(glm::vec2(position.x - dashFrom.x, position.z - dashFrom.z)) / dt : DASH_SPEED;
            dashFrom = position;
            dashTimer -= dt; dashAge += dt;
            velocity.x = diveDir.x * DASH_SPEED;
            velocity.z = diveDir.z * DASH_SPEED;
            turnToward(diveDir, dt, 20.f);
            if (!dashHit && d < 2.6f && std::fabs(dy) < 2.6f) {
                dashHit = true; ev.meleeHit = true; ev.meleeDamage = 22.f;
                // Caught you: pull up just past instead of carrying on through you
                dashTimer = std::min(dashTimer, 0.06f);
            }
            // Stopped by a wall (barely moved last tick) or out of distance
            bool stalled = dashAge > 0.1f && made < 6.f;
            if (dashTimer <= 0.f || stalled) {
                dashTimer = 0.f;
                velocity.x = velocity.z = 0.f;
                ev.slash = 3; lastSwing = AttackKind::DASH; swingTimer = SWING_TIME;
                if (!dashHit && inArc(w, 4.4f, 1.2f)) { ev.meleeHit = true; ev.meleeDamage = 18.f; }
                attack = AttackKind::NONE;
                if (chainLeft > 0) {                       // and again
                    --chainLeft;
                    // Enraged, the last link of a chain can be a shadow step behind you
                    if (enraged && chainLeft == 0 && attackCount % 2 == 0) startAttack(AttackKind::BLINK, 0.3f * quick, true);
                    else startAttack(AttackKind::DASH, (0.34f - 0.08f * rage) * quick, true);
                } else if (d < SWEEP_REACH + 0.8f) {       // straight into a combo
                    comboStep = 0;
                    startAttack(AttackKind::SWEEP, (0.36f - 0.06f * rage) * quick, true);
                } else {
                    recover(0.55f - 0.2f * rage);    // the opening: punish it
                }
            }
            return;
        }

        // ---- thrusting: a short, straight lunge, sword first ---------------
        if (thrustTimer > 0.f) {
            thrustTimer -= dt;
            velocity.x = diveDir.x * THRUST_SPEED;
            velocity.z = diveDir.z * THRUST_SPEED;
            if (!thrustHit && d < 2.9f && std::fabs(dy) < 2.4f) {
                thrustHit = true; ev.meleeHit = true; ev.meleeDamage = 24.f;
                thrustTimer = std::min(thrustTimer, 0.04f);
            }
            if (thrustTimer <= 0.f) {
                thrustTimer = 0.f;
                velocity.x = velocity.z = 0.f;
                ev.slash = 3; lastSwing = AttackKind::THRUST; swingTimer = SWING_TIME;
                recover(0.6f - 0.2f * rage);
            }
            return;
        }

        // ---- the whirlwind: spinning in on you, blade out ------------------
        // Back off, or jump it: the blade passes at waist height
        if (whirlTimer > 0.f) {
            whirlTimer -= dt; whirlHitCd -= dt;
            yaw += dt * 13.f;
            setMove(to, d > 1.5f ? 6.5f + 2.f * rage : 0.f, w);
            if (whirlHitCd <= 0.f) {
                whirlHitCd = 0.3f;
                ev.slash = whirlStep % 2; lastSwing = AttackKind::SWEEP; lastSwingStep = whirlStep; swingTimer = SWING_TIME;
                ++whirlStep;
                if (d < 4.4f && dy > -1.f && dy < 1.2f) { ev.meleeHit = true; ev.meleeDamage = 12.f; }
            }
            if (whirlTimer <= 0.f) {
                whirlTimer = 0.f;
                velocity.x = velocity.z = 0.f;
                recover(0.9f - 0.3f * rage);        // dizzy: the opening
            }
            return;
        }

        // ---- on foot ----
        if (telegraphTimer > 0.f) {
            // Planted for most wind-ups; creeps in behind a sweep or cleave
            bool creep = attack == AttackKind::SWEEP || attack == AttackKind::CLEAVE;
            float spd = creep && d > 2.5f ? 2.2f : 0.f;
            velocity.x = dir.x * spd; velocity.z = dir.z * spd;
            turnToward(to, dt, attack == AttackKind::DASH || attack == AttackKind::THRUST ? 7.f : creep ? 3.5f : 2.5f);
        } else if (recoverTimer > 0.f) {
            velocity.x = velocity.z = 0.f;
            turnToward(to, dt, 1.5f);
        } else {
            glm::vec3 side{-dir.z, 0.f, dir.x};
            strafeTimer -= dt;
            if (strafeTimer <= 0.f) { strafeTimer = frand(1.2f, 2.4f); strafeDir = -strafeDir; }
            glm::vec3 mv = d > 7.f ? dir + side * strafeDir * 0.35f : side * strafeDir + dir * 0.25f;
            setMove(mv, stats().speed * (1.f + 0.25f * rage), w);
            turnToward(to, dt, 5.f);
            // A shot turned aside comes straight back as a crescent
            if (riposte) {
                riposte = false;
                ev.slash = 0; lastSwing = AttackKind::SWEEP; lastSwingStep = 0; swingTimer = SWING_TIME;
                fireAt(w.playerEye, 3, 0.25f, 34.f, 12.f, 1.3f);
            }
        }

        if (resolve) {
            AttackKind k = attack;
            attack = AttackKind::NONE;
            switch (k) {
            case AttackKind::DASH: {
                // Aim where you're going to be, and carry on a few metres past
                glm::vec3 aim = w.playerFeet + leadVel_ * 0.35f;
                glm::vec3 a = flatTo(aim);
                float len = glm::length(a);
                diveDir = len > 0.1f ? a / len : glm::vec3{std::sin(yaw), 0.f, std::cos(yaw)};
                float dist = glm::clamp(len + 4.f, 7.f, 24.f);
                dashTimer = dist / DASH_SPEED;
                dashAge = 0.f; dashHit = false;
                ev.dashStarted = true;
                break;
            }
            case AttackKind::SWEEP: {
                ev.slash = comboStep % 2; lastSwing = AttackKind::SWEEP; lastSwingStep = comboStep;
                swingTimer = SWING_TIME;
                if (inArc(w, SWEEP_REACH, 1.35f)) { ev.meleeHit = true; ev.meleeDamage = 18.f; }
                ++comboStep;
                if (comboStep < 2) startAttack(AttackKind::SWEEP, (0.3f - 0.06f * rage) * quick, true);
                else               startAttack(AttackKind::CLEAVE, (0.48f - 0.08f * rage) * quick, true);
                break;
            }
            case AttackKind::CLEAVE: {
                ev.slash = 2; lastSwing = AttackKind::CLEAVE; swingTimer = SWING_TIME;
                if (inArc(w, CLEAVE_REACH, 0.6f)) { ev.meleeHit = true; ev.meleeDamage = 28.f; }
                ev.slam = true; ev.slamRadius = 7.f; ev.slamDamage = 16.f;   // jump the shockwave
                comboStep = 0;
                if (rage > 0.f && d > 4.f) { chainLeft = 0; startAttack(AttackKind::DASH, 0.38f * quick, true); }
                else recover(0.85f - 0.3f * rage);
                break;
            }
            case AttackKind::LEAP: {
                // A ballistic arc onto where you are (gravity 24, as in integrate)
                glm::vec3 target = w.playerFeet + leadVel_ * 0.25f;
                glm::vec3 flat = flatTo(target);
                float T  = glm::clamp(0.8f + glm::length(flat) / 40.f, 0.85f, 1.6f);
                float up = target.y - position.y;
                velocity.x = flat.x / T; velocity.z = flat.z / T;
                velocity.y = up / T + 0.5f * 24.f * T;
                leapTimer = T + 0.8f;
                grounded = false;
                ev.leapStarted = true;
                break;
            }
            case AttackKind::CRESCENT: {
                ev.slash = 0; lastSwing = AttackKind::SWEEP; lastSwingStep = 0; swingTimer = SWING_TIME;
                fireAt(w.playerEye, enraged ? 7 : 5, enraged ? 1.2f : 0.9f, 26.f, 10.f, 1.5f);
                if (crescentLeft > 0) { --crescentLeft; startAttack(AttackKind::CRESCENT, 0.32f, true); }
                else recover(0.4f);
                break;
            }
            case AttackKind::BLINK: {
                // Gone in smoke, and out again just past you - on the ground
                // or up on whatever you're standing on - already raising the
                // blade. Prefer a spot with a floor under it.
                glm::vec3 beyond = d > 0.1f ? dir : glm::vec3{std::sin(yaw), 0.f, std::cos(yaw)};
                glm::vec3 dest{0.f}; bool found = false;
                static const float ANG[] = {0.f, 1.2f, -1.2f, 2.4f, -2.4f, 3.1416f};
                for (int pass = 0; pass < 2 && !found; ++pass)
                    for (float a : ANG) {
                        glm::vec3 p = w.playerFeet + rotY(beyond, a) * 3.2f;
                        p.y = w.playerFeet.y;
                        if (!insideBounds(p, w, radius() + 0.5f) || blockedAt(p, w)) continue;
                        if (pass == 0 && !supportedAt(p, w)) continue;
                        dest = p; found = true; break;
                    }
                if (!found) { recover(0.3f); break; }
                ev.blinked = true; ev.blinkFrom = position;
                position = prevPosition = dest;
                velocity = glm::vec3{0.f};
                glm::vec3 back = flatTo(w.playerFeet);
                if (glm::length(back) > 0.01f) yaw = prevYaw = std::atan2(back.x, back.z);
                blinkFlash = 0.f; farTimer = 0.f;
                comboStep = 0;
                startAttack(AttackKind::CLEAVE, (0.46f - 0.08f * rage) * quick, true);   // parry it as it falls
                break;
            }
            case AttackKind::JUDGMENT: {
                // Blades out of the sky: one where you stand (and where you're
                // heading), a ring round it, and enraged a wider ring after
                glm::vec3 c = w.playerFeet + leadVel_ * 0.5f;
                addStrike(w.playerFeet, 0, 0.95f);
                if (glm::length(leadVel_) > 1.f) addStrike(c, 0, 1.0f);
                int ring = enraged ? 8 : 6;
                for (int i = 0; i < ring; ++i) {
                    float a = i * 6.2832f / ring + age;
                    addStrike(c + glm::vec3{std::cos(a) * 4.4f, 0.f, std::sin(a) * 4.4f}, 0, 1.05f + 0.05f * i);
                }
                if (enraged || last)
                    for (int i = 0; i < 10; ++i) {
                        float a = i * 0.6283f + age * 0.5f;
                        addStrike(c + glm::vec3{std::cos(a) * 8.5f, 0.f, std::sin(a) * 8.5f}, 0, 1.6f + 0.04f * i);
                    }
                ev.slash = 2; lastSwing = AttackKind::CLEAVE; swingTimer = SWING_TIME;
                recover(0.45f);
                break;
            }
            case AttackKind::WHIRL:
                whirlTimer = 1.7f + 0.6f * rage; whirlHitCd = 0.f;
                ev.dashStarted = true;
                break;
            case AttackKind::THRUST: {
                glm::vec3 a = flatTo(w.playerFeet + leadVel_ * 0.2f);
                float len = glm::length(a);
                diveDir = len > 0.1f ? a / len : glm::vec3{std::sin(yaw), 0.f, std::cos(yaw)};
                thrustTimer = glm::clamp((len + 1.5f) / THRUST_SPEED, 0.12f, 0.32f);
                thrustHit = false;
                ev.dashStarted = true;
                break;
            }
            case AttackKind::RUPTURE: {
                // The blade driven into the floor: eruptions run out along it
                // toward you (enraged, three lines fanned out). Sidestep or jump.
                ev.slash = 2; lastSwing = AttackKind::CLEAVE; swingTimer = SWING_TIME;
                ev.slam = true; ev.slamRadius = 3.5f; ev.slamDamage = 14.f;
                glm::vec3 fwd = d > 0.1f ? dir : glm::vec3{std::sin(yaw), 0.f, std::cos(yaw)};
                int lines = enraged ? 3 : 1, per = enraged ? 9 : 12;
                for (int l = 0; l < lines; ++l) {
                    glm::vec3 ld = rotY(fwd, (l - (lines - 1) * 0.5f) * 0.42f);
                    for (int i = 1; i <= per; ++i) {
                        glm::vec3 p = position + ld * (1.2f + 2.2f * i);
                        if (!insideBounds(p, w, 0.5f)) break;
                        addStrike(p, 1, 0.12f + 0.065f * i);
                    }
                }
                recover(0.7f - 0.2f * rage);
                break;
            }
            case AttackKind::PHANTOMS: {
                // Two of him, from either side of you, dashing in at once;
                // he follows them in himself
                for (int s = 0; s < 2; ++s) {
                    glm::vec3 side = rotY(d > 0.1f ? dir : glm::vec3{0, 0, 1}, s == 0 ? 1.5708f : -1.5708f);
                    glm::vec3 p = w.playerFeet - side * 14.f;
                    p.x = glm::clamp(p.x, w.bounds.min.x + 2.f, w.bounds.max.x - 2.f);
                    p.z = glm::clamp(p.z, w.bounds.min.z + 2.f, w.bounds.max.z - 2.f);
                    glm::vec3 pd = w.playerFeet - p; pd.y = 0.f;
                    float l = glm::length(pd);
                    ev.phantomPos[s] = p;
                    ev.phantomDir[s] = l > 0.1f ? pd / l : side;
                }
                ev.phantoms = 2;
                chainLeft = 0;
                startAttack(AttackKind::DASH, 0.5f, true);
                break;
            }
            default: break;
            }
        }

        if (recoverTimer <= 0.f && phaseHold <= 0.f && attackReady(dt)) {
            ++attackCount;
            float pursueAfter = last ? 1.2f : enraged ? 1.8f : 2.6f;
            if (farTimer > pursueAfter) {
                // Pursuit: a shadow step to you, blades on you, or (if you're
                // up high) a leap from wherever he is
                farTimer = 0.f;
                int pick = pursuitCount++ % 3;
                if (pick == 1)                 startAttack(AttackKind::JUDGMENT, 0.65f * quick);
                else if (pick == 2 && dy > 2.f) startAttack(AttackKind::LEAP, (0.55f - 0.1f * rage) * quick);
                else                           startAttack(AttackKind::BLINK, 0.5f * quick);
            }
            else if (dy > 3.f && d < 30.f)    startAttack(AttackKind::LEAP, (0.55f - 0.1f * rage) * quick);
            else if (d < SWEEP_REACH) {
                // The sweep-sweep-cleave combo most often; a whirlwind, a
                // thrust, a rupture under your feet (enraged, every other
                // time the phantoms instead) to keep you guessing
                int v = attackCount % (enraged ? 5 : 6);
                if (v == 1)      startAttack(AttackKind::WHIRL, (0.5f - 0.1f * rage) * quick);
                else if (v == 2) startAttack(enraged && (attackCount / 5) % 2 ? AttackKind::PHANTOMS : AttackKind::RUPTURE,
                                             (0.6f - 0.1f * rage) * quick);
                else if (v == 3) startAttack(AttackKind::THRUST, (0.45f - 0.05f * rage) * quick);
                else { comboStep = 0; startAttack(AttackKind::SWEEP, (0.42f - 0.08f * rage) * quick); }
            }
            else if (enraged && attackCount % 5 == 0 && d < 30.f) startAttack(AttackKind::PHANTOMS, 0.6f * quick);
            else if (d < 12.f && attackCount % 3 == 0)            startAttack(AttackKind::RUPTURE, (0.6f - 0.1f * rage) * quick);
            else if (d < 11.f && attackCount % 3 == 1)            startAttack(AttackKind::THRUST, (0.5f - 0.08f * rage) * quick);
            else if (d < 18.f || attackCount % 2 == 0) {
                chainLeft = rage > 0.f ? 2 : (attackCount % 3 == 0 ? 0 : 1);
                startAttack(AttackKind::DASH, (0.55f - 0.12f * rage) * quick);
            } else {
                crescentLeft = rage > 0.f ? 1 : 0;
                startAttack(AttackKind::CRESCENT, (0.6f - 0.1f * rage) * quick);
            }
        }
    }

    // ---- physics -------------------------------------------------------------
    void integrate(float dt, const EnemyWorld& w) {
        bool flying = stats().flying;
        if (!flying) {
            velocity.y -= 24.f * dt;
            moveSpeed = glm::length(glm::vec2(velocity.x, velocity.z));
            if (type != EnemyType::RAPTOR && type != EnemyType::MITE &&
                type != EnemyType::BRUTE && type != EnemyType::WARDEN)
                animPhase += dt * std::min(moveSpeed, 9.f) * (type == EnemyType::SOVEREIGN ? 1.2f : 1.9f);
        } else {
            moveSpeed = glm::length(velocity);
        }
        float slow = flying ? 1.f : wadeMul;
        position += glm::vec3{velocity.x * slow, velocity.y, velocity.z * slow} * dt;
        grounded = false;

        if (position.y < floorY) { position.y = floorY; if (velocity.y < 0.f) velocity.y = 0.f; grounded = true; }

        if (w.walls) {
            static std::vector<int> cands;
            float r = radius();
            AABB eb{ position + glm::vec3{-r-0.1f, -0.1f, -r-0.1f},
                     position + glm::vec3{ r+0.1f, height()+0.1f, r+0.1f} };
            if (w.grid) w.grid->query(eb, cands);
            else { cands.clear(); for (int i = 0; i < w.wallCount; ++i) cands.push_back(i); }
            for (int idx : cands) resolveAABB(w.walls[idx].box);
        }

        // Stay inside the arena
        float r = radius();
        position.x = glm::clamp(position.x, w.bounds.min.x + r, w.bounds.max.x - r);
        position.z = glm::clamp(position.z, w.bounds.min.z + r, w.bounds.max.z - r);
        if (flying) position.y = glm::clamp(position.y, w.bounds.min.y + 1.5f, w.bounds.max.y - height());
    }

    void resolveAABB(const AABB& wall) {
        float r = radius(), h = height();
        glm::vec3 pMin = position + glm::vec3{-r, 0.f, -r};
        glm::vec3 pMax = position + glm::vec3{ r, h,   r};
        if (pMax.x <= wall.min.x || pMin.x >= wall.max.x) return;
        if (pMax.y <= wall.min.y || pMin.y >= wall.max.y) return;
        if (pMax.z <= wall.min.z || pMin.z >= wall.max.z) return;
        float ox = std::min(pMax.x - wall.min.x, wall.max.x - pMin.x);
        float oy = std::min(pMax.y - wall.min.y, wall.max.y - pMin.y);
        float oz = std::min(pMax.z - wall.min.z, wall.max.z - pMin.z);
        if (ox <= oy && ox <= oz) {
            position.x += (position.x < (wall.min.x + wall.max.x) * 0.5f) ? -ox : ox;
        } else if (oy <= ox && oy <= oz) {
            bool up = position.y + h * 0.5f > (wall.min.y + wall.max.y) * 0.5f;
            position.y += up ? oy : -oy;
            if (up) grounded = true;
            if (up && velocity.y < 0.f) velocity.y = 0.f;
            if (!up && velocity.y > 0.f) velocity.y = 0.f;
        } else {
            position.z += (position.z < (wall.min.z + wall.max.z) * 0.5f) ? -oz : oz;
        }
    }
};

// A TWINNED enemy's two copies: plain, 35% of its health, three-quarter size,
// either side of where it fell, ready to fight at once
inline std::vector<Enemy> twinsOf(const Enemy& p) {
    std::vector<Enemy> out;
    glm::vec3 side{std::cos(p.yaw), 0.f, -std::sin(p.yaw)};
    for (float s : {-1.f, 1.f}) {
        Enemy t(p.type, p.position + side * (1.2f * s), p.floorY);
        t.maxHealth = t.health = p.maxHealth * 0.35f;
        t.scale = 0.75f;
        t.yaw = t.prevYaw = p.yaw;
        t.state = EnemyState::ACTIVE; t.spawnTimer = 0.f;
        out.push_back(t);
    }
    return out;
}

// Are the player's feet inside an ANCHOR's field? (A cylinder on its feet,
// 3 m up and down: grapple high over it and you're free.)
inline bool inAnchorField(const Enemy& a, glm::vec3 feet) {
    if (a.type != EnemyType::ANCHOR || !a.alive || a.state != EnemyState::ACTIVE) return false;
    if (std::fabs(feet.y - a.position.y) > 3.f) return false;
    glm::vec2 d{feet.x - a.position.x, feet.z - a.position.z};
    return glm::dot(d, d) <= a.fieldRadius() * a.fieldRadius();
}

// CONDUCTORs: each tethers up to three allies within CONDUCTOR_RANGE (the
// nearest that aren't already tethered; never a boss, a conduit or another
// conductor). Tethered enemies take CONDUCTOR_SHIELD x damage. Each conductor
// is also given a spot to hover: above and behind its allies, as seen from
// the player. Call once per tick, after dead enemies are removed.
static constexpr float CONDUCTOR_RANGE  = 14.f;
static constexpr float CONDUCTOR_SHIELD = 0.4f;
inline void linkConductors(std::vector<Enemy>& es, glm::vec3 player) {
    for (auto& e : es) { e.shielded = false; e.linkCount = 0; e.hasAnchor = false; }
    for (auto& c : es) {
        if (c.type != EnemyType::CONDUCTOR || !c.targetable()) continue;
        for (int n = 0; n < 3; ++n) {
            int best = -1; float bestD = CONDUCTOR_RANGE;
            for (int i = 0; i < (int)es.size(); ++i) {
                const Enemy& o = es[i];
                if (!o.targetable() || o.shielded || isBoss(o.type) || o.type == EnemyType::CONDUIT ||
                    o.type == EnemyType::CONDUCTOR) continue;
                float d = glm::length(o.position - c.position);
                if (d < bestD) { bestD = d; best = i; }
            }
            if (best < 0) break;
            es[best].shielded = true;
            c.links[c.linkCount++] = best;
        }
        if (c.linkCount == 0) continue;
        glm::vec3 mid{0.f};
        for (int k = 0; k < c.linkCount; ++k) mid += es[c.links[k]].position;
        mid /= (float)c.linkCount;
        glm::vec3 away{mid.x - player.x, 0.f, mid.z - player.z};
        float l = glm::length(away);
        away = l > 0.01f ? away / l : glm::vec3{0.f, 0.f, -1.f};
        c.supportAnchor = mid + away * 4.f + glm::vec3{0.f, 4.5f, 0.f};
        c.hasAnchor = true;
    }
}

#include "EnemyPenitent.h"   // THE PENITENT's mind (Enemy::thinkPenitent)
#include "EnemyWarden.h"     // THE WARDEN's mind (Enemy::thinkWarden)
