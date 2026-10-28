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
#include "core/Collision.h"   // AABB, Wall, SpatialGrid
#include "core/Difficulty.h"
#include "enemies/EnemyTypes.h"       // EnemyType, AttackKind, statsOf
#include "enemies/HitReact.h"         // how a regular enemy takes a hit: shove, flinch, stun, finish
// Each boss's own state and events, inherited by Enemy and EnemyEvents
#include "bosses/sovereign/SovereignState.h"
#include "bosses/penitent/PenitentState.h"
#include "bosses/warden/WardenState.h"
#include "bosses/leviathan/LeviathanState.h"
#include "bosses/absolute/AbsoluteState.h"

// Everything an enemy did this tick. Cleared at the top of every update().
// The common channels are here; each boss adds its own (its *Events).
struct EnemyEvents : SovereignEvents, PenitentEvents, WardenEvents, LeviathanEvents, AbsoluteEvents {
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
    // A blade stroke landed (for its slash arc; the SOVEREIGN's, the ABSOLUTE's):
    // 0 sweep to the left, 1 sweep to the right, 2 overhead cleave, 3 a dash's cut
    int       slash = -1;
    bool      dashStarted = false;   // a dash (a lunge, a whirl, a thrust) began
    bool      dodged = false;        // it sidestepped your aim
    // Strikes marked where they'll land (the SOVEREIGN's blades from the sky,
    // kind 0, and eruptions out of the floor, kind 1; others reuse them)
    static constexpr int MAX_STRIKES = 32;
    int       strikes = 0;
    glm::vec3 strikePos[MAX_STRIKES];
    int       strikeKind[MAX_STRIKES];
    float     strikeDelay[MAX_STRIKES];
    bool      blinked = false;       // a shadow step: he vanished from blinkFrom
    glm::vec3 blinkFrom{0.f};
    // WEAVER: a wire strung between A and B
    bool      wire = false; glm::vec3 wireA{0.f}, wireB{0.f};
    // THE CANTOR's bell: a ring racing out along the floor from ringAt (jump it)
    bool      ring = false; glm::vec3 ringAt{0.f};
    // THE PACEMAKER: a phase began (1-3: it leapt to its next perch); its beacon channel began (1),
    // was broken (-1) or finished (2)
    int       pmPhaseStarted = 0, pmChannel = 0;
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
    glm::vec3 playerAim{0.f};      // where you're looking (unit; zero: unknown): gunners sidestep a crosshair held on them
    const DifficultyTuning* tune = nullptr;   // null: the default difficulty
    const int* dynWalls = nullptr;   // moving platforms (not in the grid): the Descent's cage
    int  dynCount = 0;
    glm::vec3 reactor{0.f};       // the WARDEN's power: where it goes to feed (the Core's reactor)
    bool hasReactor = false;
    bool voidUnder = false;       // the floor under the arena is a void (the Reliquary): only real ground holds you up
    // THE LEVIATHAN's lair (the Maw): the pool's centre at its surface, the
    // ring floor's height, the wells it breaches from
    bool hasLair = false;
    glm::vec3 lair{0.f}; float lairFloor = 0.f;
    const glm::vec3* wells = nullptr; int wellCount = 0;
    // THE ABSOLUTE's Zenith: the floor's centre and height, its standing
    // pillars (boxes), and whether the player stands on a shard
    bool hasZenith = false;
    glm::vec3 zenCentre{0.f}; float zenFloor = 0.f;
    const AABB* zenPillars = nullptr; int zenPillarCount = 0;
    bool onShard = false;
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

struct Enemy : SovereignState, PenitentState, WardenState, LeviathanState, AbsoluteState {
    EnemyType  type;
    EnemyState state = EnemyState::SPAWNING;
    glm::vec3  position;
    glm::vec3  velocity{0.f};
    float      yaw   = 0.f;   // radians; model faces +Z at yaw 0
    // Where it was at the start of the last physics tick: rendering blends
    // from here to `position` so it moves smoothly at any frame rate
    glm::vec3  prevPosition{0.f};
    float      prevYaw = 0.f;
    float      prevAnimPhase = 0.f;   // animPhase at the last tick: limbs drawn between ticks (above 60 fps)
    // Where it was last drawn (set by the renderer). Hitscan aims at this, so
    // a shot lands on what you saw, not on where the simulation has already
    // moved it (up to a tick ahead)
    glm::vec3  shownPos{0.f};
    bool       hasShown = false;
    float      pitch = 0.f;   // RAPTOR only: nose-down while diving
    float      health, maxHealth;
    // CONDUCTOR tethers, set every tick by linkConductors (indices into this
    // tick's enemy list)
    bool       shielded = false;     // tethered: takes no damage (takeDamage)
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

    // ---- taking a hit (HitReact.h; regular enemies only) ----------------------
    glm::vec3 knockVel{0.f};     // the shove from hits, on top of its own movement; dies away fast
    float poise      = 0.f;      // damage taken lately: past its bar, it's stunned
    float stunTimer  = 0.f;      // > 0: stunned, its attack lost
    float stunGuard  = 0.f;      // > 0: just recovered from a stun, can't be stunned yet
    bool  stunBegan  = false;    // set by takeHit the moment a stun starts (the game's cue)
    float flinchAmp  = 0.f, flinchAt = -100.f;   // the drawn flinch: its size and the clock it snapped at
    glm::vec2 flinchDir{0.f, 1.f};               // which way it tips (the shove's, flat)
    float flinchTwist = 0.f;                     // and a little turn off the hit
    glm::vec3 hitDir{0.f, 0.f, 1.f};             // the last hit: its direction, its push, a headshot?
    float hitPush = 0.f;
    bool  hitHead = false;
    float firstHitAge = -1.f;    // age at its first hit (the run log's time to kill)
    // Attack tokens (GameplayState::assignAttackers): only so many enemies wind up at once,
    // and the ones you can't see take longer to get round to it. True / 1 outside the game
    bool  mayAttack  = true;
    float attackPace = 1.f;
    float finishGlow = 0.f;      // render only: 0..1, a punch would finish it (set per frame on the drawn copy)
    // Feeling your aim on it (the light gunners): held there a moment, it sidesteps
    float aimedT = 0.f, dodgeT = 0.f, dodgeCd = 0.f;
    glm::vec3 dodgeDir{0.f};
    static constexpr float DODGE_AFTER = 0.45f, DODGE_TIME = 0.24f, DODGE_SPEED = 13.f, DODGE_CD = 2.6f;
    // A duelling boss under fire (evadeTick): this share of its health lost in about a second and it
    // breaks away sideways, at most once every EVADE_CD
    static constexpr float EVADE_AT = 0.03f, EVADE_TIME = 0.3f, EVADE_SPEED = 15.f, EVADE_CD = 5.f;
    float pressure = 0.f, evadeCd = 0.f;
    int   potionsShed = 0, potionForm = 0;   // a boss: health potions earned off it so far (every BOSS_POTION_EVERY of its bar), and on which bar   // its recent hurt (share of its health, fading over ~1 s); the evade's cooldown
    bool dodges() const { return type == EnemyType::HUSK || type == EnemyType::REVENANT || type == EnemyType::WEAVER; }
    // Yanked by the grapple (GameplayState::hookEnemy): dragged toward pullTo, stunned all the way
    glm::vec3 pullTo{0.f};
    float pullT = 0.f;
    static constexpr float PULL_SPEED = 30.f;
    // Hooked and pulled toward `to`; it arrives stunned. Returns how long the pull takes
    float yank(glm::vec3 to) {
        glm::vec3 d = to - position;
        if (!stats().flying) d.y = 0.f;
        float l = glm::length(d);
        pullTo = position + d;
        pullT = l / PULL_SPEED + 0.05f;
        knockVel = glm::vec3{0.f};
        stun(pullT + 0.45f);
        return pullT;
    }
    float floorY        = 0.f;   // hard floor under it (GameplayState sets it each tick)
    float wadeMul       = 1.f;   // wading slows its steps (set each tick)
    float age           = 0.f;

    bool  enraged       = false; // a boss's phase two

    // Shared by several bosses (each boss's own state is in its *State.h)
    bool  grounded   = true;    // standing on something (set by integrate)
    float farTimer   = 0.f;     // how long you've kept your distance (or the high ground)
    float dashTimer  = 0.f;     // > 0 while dashing (the SOVEREIGN; the WARDEN's lunge)
    bool  dashHit    = false;   // the dash already caught you
    int   comboLeft  = 0;       // blows still to come in a combo (PENITENT, LEVIATHAN, REVENANT)

    // REVENANT: how many times its soul has come back (the third death is final)
    int   reforms = 0;

    // The Gauntlet's mini-bosses (EnemyMini.h)
    glm::vec3 miniHome{0.f};   // where it hung / started: the Cantor's window, the Plumb's line, the Foreman's crane
    bool  miniHomeSet = false;
    float miniOpen  = 0.f;     // THE PLUMB LINE: > 0 its core is open
    float miniPerch = 0.f;     // THE HARRIER: > 0 skimming low after a dive
    // THE PACEMAKER (bosses/pacemaker/EnemyPacemaker.h)
    glm::vec3 pmKeep{0.f};     // the Tower's keep, at the yard's floor (set by the game when it spawns)
    int   pmPhase = 0;         // 0 the yard, 1 and 2 the balconies, 3 the roof (overclocked)
    float pmLeapT = 0.f;       // > 0 in the air between perches
    float pmChannel = 0.f;     // > 0 lighting the beacon
    float pmChannelDmg = 0.f;  // damage taken this channel (enough breaks it)
    float pmBeaconCd = 0.f;    // before it can try again
    int   pmBeacons = 0;       // tries so far (two at most)
    static constexpr float PM_CHANNEL = 8.f, PM_BREAK_SHARE = 0.04f;

    // ---- the bosses' openings: hidden, open eyes / cores / wounds ------------
    bool  levHidden() const { return type == EnemyType::LEVIATHAN && (levStage != LevStage::FIGHT || attack == AttackKind::BREACH); }
    bool  levEyeOpen() const { return levBeached > 0.f || staggerTimer > 0.f || levPhase == 3; }
    // A hit on its eye: x3 beached or staggered, x2 in the eclipse, else as the head
    float levEyeMult() const { return levBeached > 0.f || staggerTimer > 0.f ? 3.f : levPhase == 3 ? 2.f : 1.f; }
    // Damage into its throat while it inhales: enough in one breath and it chokes
    bool  levThroatHit(float dmg) {
        if (levInhale <= 0.f) return false;
        levChoke += dmg;
        if (levChoke < LV_CHOKE) return false;
        levInhale = 0.f; levChoke = 0.f; levJaw = 0.f;
        stagger(LV_STAGGER);
        return true;
    }
    void  levEndInhale() { levInhale = 0.f; levChoke = 0.f; recoverTimer = 0.8f; }
    glm::vec3 levMouth() const { return position + glm::vec3{std::sin(yaw), 0.f, std::cos(yaw)} * 3.6f - glm::vec3{0.f, 0.9f, 0.f}; }
    bool  coreOpen() const { return type == EnemyType::WARDEN && (ventTimer > 0.f || wardenPhase == 3); }
    // A shot into an open weak point: the PENITENT's wound x3, the WARDEN's core x3 venting, x2 in meltdown
    float woundMult() const { return type == EnemyType::WARDEN ? (ventTimer > 0.f ? 3.f : 2.f) : 3.f; }

    bool  chained() const { return type == EnemyType::PENITENT && !risen; }
    float sweepReach() const { return risen ? 18.f : 16.f; }
    bool  killedByBlast = false; // MITE: set when it detonated itself (not shot)

    EnemyEvents ev;

    Enemy(EnemyType t, glm::vec3 pos, float floor = 0.f) : type(t), position(pos), prevPosition(pos) {
        floorY = floor;
        const EnemyStats& s = statsOf(t);
        health = maxHealth = s.health;
        scale = miniScale(t);   // (1 for everything but the Gauntlet's mini-bosses)
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
    bool  targetable() const { return alive && state == EnemyState::ACTIVE && !levHidden() && !(type == EnemyType::ABSOLUTE && (abArrive > 0.f || abSwallow > 0.f)); }
    bool  staggered() const  { return staggerTimer > 0.f; }
    // The moment a melee blow can be punched back: a JUGGERNAUT's smash (its
    // last 0.4 s), a SOVEREIGN's sweep or cleave (its last quarter second)
    bool  parryWindow() const {
        // THE WARDEN's lunge: its last quarter second, or in flight until it connects (it comes into reach)
        if ((type == EnemyType::WARDEN || type == EnemyType::PACEMAKER) && dashTimer > 0.f && !dashHit) return true;   // (and THE PACEMAKER's dash)
        if (telegraphTimer <= 0.f) return false;
        if (type == EnemyType::WARDEN) return attack == AttackKind::WLUNGE && telegraphTimer < 0.25f;

        if (type == EnemyType::JUGGERNAUT) return attack == AttackKind::SMASH && telegraphTimer < 0.4f;
        if (type == EnemyType::LEVIATHAN) return attack == AttackKind::CRASH && telegraphTimer < 0.25f;   // (and you within LV_PARRY_REACH of where it lands)
        if (type == EnemyType::ABSOLUTE) return abMeleeWhite(attack) && !abParriedThis && telegraphTimer < 0.25f;   // white strikes only
        if (type == EnemyType::SHIELDBEARER || type == EnemyType::BULWARK) return attack == AttackKind::BASH && telegraphTimer < 0.3f;
        if (type == EnemyType::PACEMAKER) return (attack == AttackKind::DASH || attack == AttackKind::SWEEP || attack == AttackKind::THRUST) && telegraphTimer < 0.25f;
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
        if (type == EnemyType::ABSOLUTE)   // all round, from beyond 14 m, while he isn't winding up, recovering or broken
            return alive && !staggered() && dist >= AB_DEFLECT && telegraphTimer <= 0.f && recoverTimer <= 0.f && abArrive <= 0.f && !abAirborne();   // (aloft: open)
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
    float staggerTime() const { return type == EnemyType::ABSOLUTE ? AB_BREAK : type == EnemyType::LEVIATHAN ? LV_STAGGER : type == EnemyType::SOVEREIGN ? 3.f : type == EnemyType::WARDEN ? 2.f : type == EnemyType::SHIELDBEARER ? 2.2f : type == EnemyType::BULWARK ? 2.8f : type == EnemyType::PACEMAKER ? 2.6f : 2.5f; }
    // SHIELDBEARER: does its shield stop a shot travelling along dir? (From
    // the front, while it's standing; a broken one has its shield knocked aside)
    bool blocks(glm::vec3 dir) const {
        if ((type != EnemyType::SHIELDBEARER && type != EnemyType::BULWARK) || staggered() || !alive) return false;
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
        if (type == EnemyType::ABSOLUTE) return staggered() ? (abReeling ? 1.5f : 2.f) : 1.f;
        if (type == EnemyType::SOVEREIGN || type == EnemyType::LEVIATHAN || type == EnemyType::PACEMAKER) return staggered() ? 2.f : 1.f;
        if (isMiniBoss(type)) return miniArmor();
        if (type != EnemyType::JUGGERNAUT) return 1.f;
        return staggered() ? 2.f : 0.5f;
    }
    void stagger(float t) {
        comboLeft = 0;
        staggerTimer = t; attack = AttackKind::NONE; telegraphTimer = 0.f; attackTimer = 0.f;
        comboStep = 0; chainLeft = 0; crescentLeft = 0; dashTimer = 0.f;
        whirlTimer = 0.f; thrustTimer = 0.f; riposte = false;
        abCur = -1; abStep = 0;   // THE ABSOLUTE: the string is over (a reel resumes it: abParried)
        if (leapTimer <= 0.f) { velocity.x = velocity.z = 0.f; }
    }
    float telegraphProgress() const {
        return telegraphTimer > 0.f && telegraphDuration > 0.f
             ? 1.f - telegraphTimer / telegraphDuration : 0.f;
    }

    // ---- taking a hit --------------------------------------------------------
    bool reacts() const { return hitreact::reacts(type); }
    bool stunned() const { return stunTimer > 0.f; }
    // A punch would finish it: a regular enemy low enough (or stunned, or you in OVERDRIVE)
    bool finishable(bool overdrive) const {
        if (!reacts() || !targetable() || shielded || type == EnemyType::MITE || isMiniBoss(type)) return false;
        return health <= maxHealth * hitreact::finishShare(type, stunned(), overdrive);
    }
    // A hit lands: shoved along `dir` by `push` (m/s on a 1-mass body), flinched,
    // and `dmg * poiseMul` against its poise. `clock` is the game clock (the drawn
    // flinch). True if this hit stunned it: its attack is lost
    bool takeHit(glm::vec3 dir, float push, float dmg, float poiseMul, bool head, float clock) {
        hitDir = dir; hitPush = push; hitHead = head;
        if (firstHitAge < 0.f) firstHitAge = age;
        if (alive && hitreact::bossFlinches(type)) {   // it feels it, a little (HitReact.h): no shove, no stun
            float now = hitreact::flinchAt(flinchAmp, clock - flinchAt);
            flinchAmp = std::min(hitreact::BOSS_FLINCH_MAX, now + push * hitreact::BOSS_FLINCH_PER_PUSH + (head ? hitreact::BOSS_FLINCH_HEAD : 0.f));
            flinchAt = clock;
            glm::vec2 fd{dir.x, dir.z};
            if (glm::length(fd) > 1e-4f) flinchDir = glm::normalize(fd);
            flinchTwist = 0.f;
        }
        if (!reacts() || !alive) return false;
        const hitreact::Body& b = hitreact::bodyOf(type);
        float mass = b.mass * scale * scale;
        float shove = push / mass;
        glm::vec3 d = dir;
        if (!stats().flying) { d.y = 0.f; float l = glm::length(d); d = l > 1e-4f ? d / l : glm::vec3{0.f}; }
        knockVel += d * shove;
        float kl = glm::length(knockVel);
        if (kl > 16.f) knockVel *= 16.f / kl;
        if (!stats().flying && grounded && shove > 6.f) velocity.y = std::max(velocity.y, std::min(5.5f, (shove - 6.f) * 0.7f));   // a big one lifts it off its feet
        // The drawn flinch: snaps over from wherever it is now, a little further for each hit
        float now = hitreact::flinchAt(flinchAmp, clock - flinchAt);
        flinchAmp = std::min(hitreact::FLINCH_MAX, now + shove * hitreact::FLINCH_PER_PUSH + (head ? hitreact::FLINCH_HEAD : 0.f));
        flinchAt = clock;
        glm::vec2 fd{dir.x, dir.z};
        if (glm::length(fd) > 1e-4f) flinchDir = glm::normalize(fd);
        flinchTwist = 0.18f * std::sin(clock * 37.f + (float)uid * 2.3f) * std::min(1.f, shove / 5.f);   // (no rand(): the sim stays repeatable)
        // Poise: enough, close enough together, breaks its footing
        poise += dmg * poiseMul;
        if (poise >= b.poise && stunGuard <= 0.f && stunTimer <= 0.f && b.stun > 0.f) { stun(b.stun); return true; }
        return false;
    }
    void stun(float t) {
        stunTimer = t; stunGuard = t + hitreact::STUN_GUARD; poise = 0.f; stunBegan = true;
        attack = AttackKind::NONE; telegraphTimer = 0.f; burstLeft = 0; comboLeft = 0;
        if (diveTimer > 0.f) { diveTimer = 0.f; recoverTimer = std::max(recoverTimer, 0.6f); }
        if (beamTimer > 0.f) { beamTimer = 0.f; attackTimer = 0.f; }
        attackTimer = std::min(attackTimer, stats().attackEvery * 0.5f);   // it has to work back up to it
    }

    AABB getAABB() const {
        if (type == EnemyType::LEVIATHAN) {   // its head, round its centre
            glm::vec3 h{3.0f, 2.6f, 3.0f};
            return {position - h, position + h};
        }
        float r = radius();
        return { position + glm::vec3{-r, 0.f, -r}, position + glm::vec3{r, height(), r} };
    }

    // Returns true if this hit killed the enemy.
    // An enemy tethered to a CONDUCTOR takes none: kill the Conductor first.
    bool takeDamage(float dmg) {
        if (!targetable() || shielded) return false;
        health -= dmg;
        hitFlashTimer = 0.12f;
        if (isBoss(type)) pressure += dmg / std::max(1.f, maxHealth);   // (evadeTick)
        if (health <= 0.f && type == EnemyType::ABSOLUTE && abForm == 0) {   // not yet: the eclipse takes him (MORNINGSTAR)
            health = 0.f;
            abBeginSwallow();
            return false;
        }
        if (health <= 0.f) {
            health = 0.f;
            alive  = false;
            state  = EnemyState::DEAD;
            return true;
        }
        if (type == EnemyType::PACEMAKER && pmChannel > 0.f) pmChannelDmg += dmg;   // into its beacon channel: enough breaks it
        if (isBoss(type) && type != EnemyType::WARDEN && type != EnemyType::LEVIATHAN && type != EnemyType::ABSOLUTE && type != EnemyType::PACEMAKER && !enraged && health < maxHealth * 0.5f) {
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
        pressure *= std::exp(-dt);
        if (evadeCd > 0.f) evadeCd -= dt;
        if (haloOpenTimer > 0.f) haloOpenTimer = std::max(0.f, haloOpenTimer - dt);

        // THE ABSOLUTE's entrance is his descent (abArrive): no materialising on the floor first
        if (state == EnemyState::SPAWNING && type == EnemyType::ABSOLUTE) { spawnTimer = 0.f; state = EnemyState::ACTIVE; }
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
            const float before = telegraphTimer;
            telegraphTimer -= dt;
            if (type == EnemyType::ABSOLUTE && abTellOf(attack) != AbTell::NONE &&
                (before > AB_CUE || before >= telegraphDuration - 1e-5f) && telegraphTimer <= AB_CUE)
                ev.abCue = true;   // the beat to act on: AB_CUE before the blow (a tell no longer than that cues on its first tick)
            if (telegraphTimer <= 0.f) { telegraphTimer = 0.f; resolve = true; }
        }
        if (recoverTimer > 0.f) recoverTimer -= dt;
        if (staggerTimer > 0.f) {   // broken: stands there, open to punishment
            staggerTimer -= dt;
            velocity.x = velocity.z = 0.f;
            if (type == EnemyType::LEVIATHAN) { levReel(dt); return; }   // its head down on the ring
            if (type == EnemyType::ABSOLUTE) {   // he stands where he was struck (or falls there), unless a parry drives him back
                const glm::vec3 was = position;
                abShoveTick(dt, w); abFly(dt, w); abGaitTick(dt, was);
                if (staggerTimer <= 0.f) { abReeling = false; abShove = glm::vec3{0.f}; }
                return;
            }   // he stands where he was struck (or falls there)
            integrate(dt, w);
            return;
        }
        if (reacts()) {   // poise drains; a stun holds it where it was hit (the shove still carries it)
            poise = std::max(0.f, poise - hitreact::bodyOf(type).poise * hitreact::POISE_DRAIN * dt);
            if (stunGuard > 0.f) stunGuard -= dt;
            if (stunTimer > 0.f) {
                stunTimer = std::max(0.f, stunTimer - dt);
                velocity.x = velocity.z = 0.f;
                if (pullT > 0.f) {   // on the end of the rope: dragged in, walls still in the way
                    pullT -= dt;
                    glm::vec3 d = pullTo - position;
                    if (!stats().flying) d.y = 0.f;
                    float l = glm::length(d), step = PULL_SPEED * dt;
                    if (l <= step || pullT <= 0.f) { pullT = 0.f; if (l > 1e-4f) knockVel = d / l * 4.f; }   // (it lands with a little carry)
                    else position += d / l * step;
                    if (stats().flying) velocity.y = 0.f;
                }
                if (stats().flying) velocity.y = -1.5f;   // a flyer sags
                if (type == EnemyType::RAPTOR) pitch = glm::mix(pitch, 0.f, std::min(1.f, dt * 6.f));
                integrate(dt, w);
                return;
            }
        }
        if (noLosTimer   > 0.f) noLosTimer   -= dt;
        if (avoidTimer   > 0.f) avoidTimer   -= dt;
        if (dodges() && dodgeTick(dt, w)) { integrate(dt, w); return; }

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
            case EnemyType::REVENANT: thinkRevenant(dt, w, resolve); break;
            case EnemyType::WEAVER:   thinkWeaver(dt, w, resolve);   break;
            case EnemyType::LEVIATHAN: thinkLeviathan(dt, w, resolve); return;   // it moves its head itself
            case EnemyType::ABSOLUTE: thinkAbsolute(dt, w, resolve); return;     // he places himself (EnemyAbsolute.h)
            case EnemyType::COURIER: case EnemyType::BULWARK: case EnemyType::CANTOR:
            case EnemyType::HARRIER: case EnemyType::PLUMB: case EnemyType::FOREMAN:
                thinkMini(dt, w, resolve); break;                                 // the Gauntlet's (EnemyMini.h)
            case EnemyType::PACEMAKER:                                            // (EnemyPacemaker.h; mid-leap it flies its arc)
                thinkPacemaker(dt, w, resolve);
                if (pmLeapT <= 0.f) integrate(dt, w);
                return;
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
        if (type == EnemyType::SOVEREIGN || type == EnemyType::WARDEN || type == EnemyType::LEVIATHAN || type == EnemyType::ABSOLUTE)   // never too quick to read, whatever sped it up
            windup = std::max(windup, followUp ? (type == EnemyType::ABSOLUTE ? AB_FOLLOW_FLOOR : TELL_FLOOR_FOLLOW) : TELL_FLOOR);
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
        if (!w.walls) return true;
        if (p.y < floorY + 0.3f && !w.voidUnder) return true;   // the floor under it (Y 0, or a basin's); over a void, only real ground counts
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
        if (w.voidUnder) return stats().flying || supportedAt(p, w);   // over a void every walker keeps to real ground
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

    glm::vec3 eyePos() const {
        if (type == EnemyType::LEVIATHAN) return levMouth();
        return position + glm::vec3{0.f, height() * 0.85f, 0.f};
    }

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
        // Its own bearing round you (golden-angle spaced by uid): drift round toward it, so a squad
        // spreads out and comes at you from several sides rather than all from the front
        float want = (float)uid * 2.39996f, have = std::atan2(-to.x, -to.z);
        float off = std::remainder(want - have, 6.2831853f);
        glm::vec3 spread = side * glm::clamp(off * 1.2f, -1.f, 1.f) * (d > nearR ? 0.6f : 0.3f);
        // On a perch what hides you is usually the perch's own lip: walk to the
        // edge (the ledge check stops it there) and shoot down from it
        bool perched = ledgeAware() && position.y > floorY + 0.3f;
        if (noLosTimer > 0.f && perched) mv = dir + side * strafeDir * 0.3f;
        else if (noLosTimer > 0.f) mv = side * strafeDir + dir * 0.35f;      // sidestep out from behind cover
        else if (d < nearR)    mv = -dir + side * strafeDir * 0.7f + spread;
        else if (d > farR)     mv = dir + side * strafeDir * 0.3f + spread;
        else                   mv = side * strafeDir * 0.6f + spread * 1.4f;
        setMove(mv, speed, w);
    }

    // Ticks the attack clock; returns true when it's time to start a new attack.
    bool attackReady(float dt) {
        if (telegraphTimer > 0.f || burstLeft > 0 || diveTimer > 0.f || beamTimer > 0.f) return false;
        attackTimer += dt * (enraged ? 1.5f : 1.f) * tune_->attackRate * attackPace;
        if (attackTimer < stats().attackEvery) return false;
        if (!mayAttack) { attackTimer = stats().attackEvery * 0.8f; return false; }   // ready, but someone else has the floor
        attackTimer = 0.f;
        return true;
    }

    // Your crosshair held on it a moment (and it isn't mid-attack): a quick sidestep, the side that's
    // clear. True while dodging (the caller just integrates)
    bool dodgeTick(float dt, const EnemyWorld& w) {
        if (dodgeCd > 0.f) dodgeCd -= dt;
        if (dodgeT > 0.f) {
            dodgeT -= dt;
            velocity.x = dodgeDir.x * DODGE_SPEED * tune_->moveSpeed; velocity.z = dodgeDir.z * DODGE_SPEED * tune_->moveSpeed;
            animPhase += dt * 12.f;
            return true;
        }
        if (glm::dot(w.playerAim, w.playerAim) < 0.5f) return false;
        glm::vec3 c = position + glm::vec3{0.f, height() * 0.6f, 0.f}, to = c - w.playerEye;
        float t = glm::dot(to, w.playerAim), dist = glm::length(to);
        bool onIt = t > 6.f && dist < 22.f && glm::length(to - w.playerAim * t) < radius() + 0.35f;
        aimedT = onIt ? aimedT + dt : std::max(0.f, aimedT - dt * 2.f);
        if (aimedT < DODGE_AFTER || dodgeCd > 0.f || telegraphTimer > 0.f || burstLeft > 0) return false;
        glm::vec3 across = norm2(glm::cross(w.playerAim, glm::vec3{0.f, 1.f, 0.f}));
        float sgn = (uid + (int)(age * 3.f)) % 2 ? 1.f : -1.f;
        for (int k = 0; k < 2; ++k, sgn = -sgn)
            if (canStepTo(position + across * sgn * (radius() + 1.6f), w)) {
                dodgeDir = across * sgn; dodgeT = DODGE_TIME; dodgeCd = DODGE_CD; aimedT = 0.f;
                ev.dodged = true;
                return true;
            }
        dodgeCd = 0.6f;   // boxed in: look again in a moment
        return false;
    }

    // A duelling boss, free to move (not winding up or recovering): hurt hard enough, fast enough, it breaks
    // sideways across your aim, the side that's clear, and keeps strafing that way. True while it's breaking away
    bool evadeTick(float dt, const EnemyWorld& w) {
        if (dodgeT > 0.f) {
            dodgeT -= dt;
            velocity.x = dodgeDir.x * EVADE_SPEED * tune_->moveSpeed; velocity.z = dodgeDir.z * EVADE_SPEED * tune_->moveSpeed;
            animPhase += dt * 12.f;
            return true;
        }
        if (pressure < EVADE_AT || evadeCd > 0.f || staggered()) return false;
        glm::vec3 aim = glm::dot(w.playerAim, w.playerAim) > 0.5f ? w.playerAim : position - w.playerEye;
        glm::vec3 across = norm2(glm::cross(aim, glm::vec3{0.f, 1.f, 0.f}));
        float sgn = strafeDir < 0.f ? 1.f : -1.f;   // against the way it was going: your aim was leading it
        for (int k = 0; k < 2; ++k, sgn = -sgn)
            if (canStepTo(position + across * sgn * (radius() + 2.5f), w)) {
                dodgeDir = across * sgn; dodgeT = EVADE_TIME; evadeCd = EVADE_CD; pressure = 0.f;
                strafeTimer = std::max(strafeTimer, 0.8f);
                ev.dodged = true;
                return true;
            }
        evadeCd = 1.f;   // boxed in: look again in a moment
        return false;
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
            attackTimer += dt * tune_->attackRate * attackPace;
            if (attackTimer >= stats().attackEvery && d < 4.2f && mayAttack) {
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
        } else if (noLosTimer > 0.f && telegraphTimer <= 0.f && burstLeft <= 0 && mayAttack && lineOfSight(eyePos(), w)) {
            // Moving to find a shot that was already due: take it the moment the
            // way opens (a gap at the corner of a lip can be gone before the
            // next re-check)
            noLosTimer = 0.f; attackTimer = 0.f;
            startAttack(AttackKind::BURST, stats().telegraph);
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

    // ---- the SERAPH -----------------------------------------------------------
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

    // ---- the ANCHOR -----------------------------------------------------------
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

    // ---- the CONDUCTOR --------------------------------------------------------
    // Hovers behind the allies it shields (supportAnchor), weaving so it isn't
    // a sitting target, and backs off if you close in. No attack of its own.
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

    // ---- helpers the bosses share --------------------------------------------

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
    void thinkLeviathan(float dt, const EnemyWorld& w, bool resolve);  // EnemyLeviathan.h
public:
    int  abParried(bool bySword);                                     // EnemyAbsolute.h
    void abShoveBack(float v);
    void abBeginSwallow();                                            // EnemyAbsolute.h
    void abBecomeMorning(const EnemyWorld& w, glm::vec3 feet);
private:
    void thinkAbsolute(float dt, const EnemyWorld& w, bool resolve);   // EnemyAbsolute.h
    void thinkAbsoluteMind(float dt, const EnemyWorld& w, bool resolve);
    void abFly(float dt, const EnemyWorld& w);
    void abGaitTick(float dt, glm::vec3 was);
    void abShoveTick(float dt, const EnemyWorld& w);
    void abOrbit(float dt, const EnemyWorld& w, glm::vec3 feet);
    void abDowned();
    void abPhaseCheck(float hp);
    int  abChoose(float d, const EnemyWorld& w);
    int  abCampAnswer(const EnemyWorld& w) const;
    void abBegin(int step, bool follow, float quick, const EnemyWorld& w, glm::vec3 feet);
    void abResolve(const EnemyWorld& w, glm::vec3 feet, float d);
    void abResolveLate(const EnemyWorld& w, glm::vec3 feet, float d, float damage);
    glm::vec3 abGround(const EnemyWorld& w, glm::vec3 p) const;
    void levReel(float dt);                                            // EnemyLeviathan.h
    void aimCrash(const EnemyWorld& w, float windup, bool followUp);
    void levPose(float dt, const EnemyWorld& w);
    void thinkRevenant(float dt, const EnemyWorld& w, bool resolve);   // EnemyRelic.h
    void thinkWeaver(float dt, const EnemyWorld& w, bool resolve);     // EnemyRelic.h

    void thinkSovereign(float dt, const EnemyWorld& w, bool resolve);  // EnemySovereign.h
    // The Gauntlet's mini-bosses (EnemyMini.h)
    void thinkMini(float dt, const EnemyWorld& w, bool resolve);
    void thinkCourier(float dt, const EnemyWorld& w, bool resolve);
    void thinkBulwark(float dt, const EnemyWorld& w, bool resolve);
    void thinkCantor(float dt, const EnemyWorld& w, bool resolve);
    void thinkHarrier(float dt, const EnemyWorld& w, bool resolve);
    void thinkPlumb(float dt, const EnemyWorld& w, bool resolve);
    void thinkForeman(float dt, const EnemyWorld& w, bool resolve);
    bool miniBeam(float dt, const EnemyWorld& w, float speed);
    void thinkPacemaker(float dt, const EnemyWorld& w, bool resolve);   // bosses/pacemaker/EnemyPacemaker.h
    void pmLeapTo(glm::vec3 p);
public:
    glm::vec3 pmPerch(int phase) const;
    float miniArmor() const;
private:

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
        if (knockVel.x != 0.f || knockVel.y != 0.f || knockVel.z != 0.f) {   // the shove from hits: fast, and gone in a quarter second
            position += knockVel * dt;
            knockVel *= std::exp(-dt * 9.f);
            if (glm::dot(knockVel, knockVel) < 0.01f) knockVel = glm::vec3{0.f};
        }
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
        t.reforms = p.reforms;   // a REVENANT's twins have the lives it had left
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
// conductor). Tethered enemies take no damage (Enemy::takeDamage). Each conductor
// is also given a spot to hover: above and behind its allies, as seen from
// the player. Call once per tick, after dead enemies are removed.
static constexpr float CONDUCTOR_RANGE  = 14.f;
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

#include "bosses/sovereign/EnemySovereign.h"  // THE SOVEREIGN's mind (Enemy::thinkSovereign)
#include "bosses/penitent/EnemyPenitent.h"   // THE PENITENT's mind (Enemy::thinkPenitent)
#include "bosses/leviathan/EnemyLeviathan.h"  // THE LEVIATHAN's mind (Enemy::thinkLeviathan)
#include "bosses/warden/EnemyWarden.h"     // THE WARDEN's mind (Enemy::thinkWarden)
#include "enemies/EnemyRelic.h"      // the Reliquary's REVENANT and WEAVER
#include "enemies/EnemyMini.h"       // the Gauntlet's mini-bosses
#include "bosses/pacemaker/EnemyPacemaker.h"   // THE PACEMAKER's mind (Enemy::thinkPacemaker)
#include "bosses/absolute/EnemyAbsolute.h"   // THE ABSOLUTE's mind (Enemy::thinkAbsolute)
