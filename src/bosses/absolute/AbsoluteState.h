#pragma once
// =============================================================================
// AbsoluteState.h — what THE ABSOLUTE and his second form MORNINGSTAR, in the Zenith,
// remembers between ticks, its tuning constants, and the events it reports.
//
// Enemy inherits AbsoluteState and EnemyEvents inherits AbsoluteEvents, so the
// fields read like any other enemy's (`e.abForm`, `e.ev.abCue`). Keeping them
// here keeps Enemy.h about what every enemy shares. Behaviour: EnemyAbsolute.h.
// =============================================================================
#include <glm/glm.hpp>
#include "enemies/EnemyTypes.h"

struct AbsoluteState {
    // THE ABSOLUTE (EnemyAbsolute.h): he fights in strings (combos) from a table
    static constexpr float AB_PHASE2 = 0.60f, AB_PHASE3 = 0.25f, AB_JUDGE_AT = 0.08f, AB_ARRIVE = 2.f, AB_REEL = 1.25f,
                           AB_BREAK = 3.f, AB_SPENT = 4.f, AB_DEFLECT = 14.f, AB_CAMP = 3.f, AB_FAR = 30.f, AB_RUN = 11.f,
                           AB_DASH = 32.f, AB_REACH = 2.8f, AB_FOLLOW_FLOOR = 0.22f, AB_QUICK = 0.75f, AB_RANGE = 20.f,
                           AB_BLADE_BACK = 3.f, AB_HALO_TIME = 2.f, AB_JUDGE_WARN = 2.f,
                           AB_FLOOR_R = 42.f, AB_DAIS_R = 9.f, AB_DAIS_UP = 0.15f, AB_HALO_UP = 40.f;
    // his flight (EnemyAbsolute.h): cruising height, climb / dive / fall speeds, a flight's limit, the share of
    // his health that shoots him down and how long he lies there, how far he circles you, the height that clears pillars
    static constexpr float AB_CRUISE_LO = 9.f, AB_CRUISE_HI = 14.f, AB_CLIMB = 18.f, AB_DIVE_V = 45.f, AB_FALL_V = 30.f,
                           AB_AIR_MAX = 5.f, AB_SHOOTDOWN = 0.06f, AB_DOWNED = 1.5f, AB_ORBIT_LO = 14.f, AB_ORBIT_HI = 20.f,
                           AB_OVER = 15.f, AB_ARRIVE_UP = 12.f, AB_ASCEND_UP = 12.f, AB_SKY_R = 3.5f, AB_MARK_T = 0.7f, AB_CUE = 0.22f;   // (AB_CUE: the cue sounds this long before a blow lands)
    // his arsenal: a kick's reach, the shoulder charge's run, the clap's cone (6 m, 35 degrees either side)
    static constexpr float AB_KICK_REACH = 4.f, AB_CHARGE_LEN = 14.f, AB_CLAP_R = 6.f, AB_CLAP_COS = 0.819f;
    // THUNDERCLAP: how high he rises before the clap (he hangs there, open to fire, not a flight: it can't shoot him down)
    static constexpr float AB_THUNDER_UP = 7.f;
    float abAlt = 0.f, abAltWant = 0.f;   // his height over the floor under him, and where it's going
    float abAirT = 0.f;                   // this flight's time in the air
    float abFlightHp = -1.f;              // his health as this flight began (< 0: on the ground)
    bool  abDiving = false;               // coming down fast (a dive, a crash, a landing)
    int   abOrbitDir = 1;                 // which way he circles you
    bool  abAirborne() const { return abAlt > 1.5f; }
    int   abPhase = 1;
    int   abCur = -1, abStep = 0;      // the string running (an AB_* id), the move in it
    int   abForce = -1;                // tests / dev: the next string
    int   abParries = 0;               // white strikes parried in this string
    bool  abReeling = false;           // this stagger is a reel (x1.5), not a break (x2)
    glm::vec3 abShove{0.f};            // a parry's shove: he's driven back on his heels while staggered (abShoveTick)
    bool  abParriedThis = false;       // the current strike has been parried
    bool  abStruck = false;            // a strike has landed in this string (the next tell is a follow-up)
    float abArrive = AB_ARRIVE;        // descending onto the Dais: untouchable
    float abCamp = 0.f;                // how long you've camped (a shard, far off, one pillar)
    int   abCampPillar = -1;
    int   abRot = 0;                   // where he is in the phase's rotation
    bool  abBlade = false; float abBladeGone = 0.f;   // the blade of light (phase 3); > 0: shattered, redrawing
    bool  abJudged = false;            // the last judgment has happened
    float abLate = 0.f;                // a delayed strike: how long after its flash it lands
    // MORNINGSTAR (EnemyAbsolute.h): his lethal blow in phase 3 doesn't kill him. He kneels, the eclipse
    // swallows the Zenith, he rises into the black and comes down as MORNINGSTAR: MS_SCALE times the size,
    // a fresh health bar, phases 4 (MORNINGSTAR), 5 (RISING, aftershocks) and 6 (TOTALITY)
    static constexpr float MS_SCALE = 1.65f, MS_HP = 11000.f, MS_SWALLOW = 5.f, MS_RISE = 1.4f, MS_UP = 30.f,
                           MS_PHASE5 = 0.55f, MS_TOTALITY = 0.12f, MS_ENTRY = 2.6f, MS_DROP_R = 5.5f, MS_DROP_UP = 22.f;
    // his reach (a jab's, the backhand's), GRASP's hand (how far it reaches, how wide it closes), his run
    static constexpr float MS_REACH = 4.6f, MS_BACK_REACH = 6.5f, MS_GRASP_REACH = 8.f, MS_GRASP_R = 3.2f, MS_RUN = 12.f;
    int   abForm = 0;                  // 0 THE ABSOLUTE, 1 MORNINGSTAR
    float abSwallow = 0.f;             // > 0: the swallow running (he kneels, then rises): untouchable
    bool  abSwallowFresh = false;      // the swallow began since his last tick (its event goes out then)
    bool  morning() const { return abForm == 1; }
    glm::vec3 abAim{0.f};              // where a ray / dash / drop-kick / lunge is headed
    glm::vec3 abCentre{0.f};           // the Zenith's centre (from the world, each tick)
    AttackKind lastAbAttack = AttackKind::NONE;   // the strike that just resolved (the game reads its colour)
    // his body in motion (AbsoluteRig.h tickAbsoluteBody): the drawn pose, one spring per joint, chasing
    // where the rig says he's going; the blow that last landed and how long ago (its follow-through)
    static constexpr int AB_BODY = 26;
    float abBody[AB_BODY] = {}, abBodyV[AB_BODY] = {}, abBodyPrev[AB_BODY] = {};
    bool  abBodyLive = false;
    float abGait = 0.f;                // his feet's speed over the floor, smoothed (+ forward, - backing off): his steps (abGaitTick)
    AttackKind abBodyAtk = AttackKind::NONE, abBodyLanded = AttackKind::NONE;
    float abBodyTT = 0.f, abBodySince = 9.f;
    // A white strike from his body or blade: the ones you can parry
    static bool abMeleeWhite(AttackKind k) {
        return k == AttackKind::AJAB || k == AttackKind::AHOOK || k == AttackKind::ASWEEP || k == AttackKind::AMETEOR ||
               k == AttackKind::ACROSS || k == AttackKind::ABACKFIST || k == AttackKind::AELBOW || k == AttackKind::AROUNDHOUSE ||
               k == AttackKind::MJAB || k == AttackKind::MBACKHAND;
    }
};

struct AbsoluteEvents {
    // THE ABSOLUTE (EnemyAbsolute.h): the colour of the strike winding up (0
    // white, 1 gold, 2 red), a phase change, a line to show, a ray fired from
    // abFrom toward abTo, a stomp ring, a serpent, the halo sweep, the seams
    // (which parity burns first), a zenith ray, mirages, the blade drawn or
    // shattered, the last judgment marked / landing, an uppercut's launch,
    // the shards falling
    int  abTell = -1, abPhase = 0, abLine = -1, abSeamFirst = 0, abMirages = 0;
    bool abRay = false, abStomp = false, abSerpent = false, abHalo = false, abSeams = false, abZenith = false,
         abBladeDrawn = false, abBladeBroke = false, abJudgeMark = false, abJudge = false, abLaunch = false, abShards = false;
    bool abTakeoff = false, abShotDown = false;   // he left the floor; he was shot out of the sky
    bool abLanded = false;                        // his entrance: down on the Dais
    bool abCue = false, abImpact = false; AttackKind abImpactKind = AttackKind::NONE;   // the beat to act on; a blow landing (hit or miss) and what it was
    bool abHeavy = false, abCrack = false;   // a haymaker landed (a big knockback); an axe kick's crack (at abTo)
    bool abFan = false;                      // THUNDERCLAP: a fan of fire rolls out from the floor under him (abFrom) toward abTo
    bool abMark = false, abCrater = false, abFlare = false, abShardBreak = false;   // SKYFALL's circle (at abTo) and its crater (a shard under it breaks); a METEOR's flare
    glm::vec3 abFrom{0.f}, abTo{0.f};
    // MORNINGSTAR (his second form): the eclipse swallows the Zenith; he comes back as MORNINGSTAR
    bool msSwallow = false, msForm = false;
    // his moves landing: SUNDER's ring (from abFrom), a FISSURE (abFrom toward abTo), the Eclipse drop's
    // crater (abTo), a GRASP's hand (abTo, caught or not), the ring of fire's flares (lanes across the floor,
    // set by abTo), the corona collapsing; msAfter: an aftershock follows (RISING on); msFlaresUnder: the
    // ring fires under the string he's starting
    bool msSunder = false, msFissure = false, msDrop = false, msGrasp = false, msFlares = false, msCollapse = false;
    bool msAfter = false, msFlaresUnder = false;
};
