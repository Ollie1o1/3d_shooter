#pragma once
// =============================================================================
// LeviathanState.h — what THE LEVIATHAN, the serpent in the Maw,
// remembers between ticks, its tuning constants, and the events it reports.
//
// Enemy inherits LeviathanState and EnemyEvents inherits LeviathanEvents, so the
// fields read like any other enemy's (`e.levBeached`, `e.ev.lvCrash`). Keeping them
// here keeps Enemy.h about what every enemy shares. Behaviour: EnemyLeviathan.h.
// =============================================================================
#include <glm/glm.hpp>
#include "enemies/EnemyTypes.h"

struct LeviathanState {
    // LEVIATHAN (EnemyLeviathan.h): `position` is its head's centre; the body
    // curves up to it out of its root (the pool, or a well while it hunts)
    enum class LevStage { RISE, FIGHT, HIDDEN };
    static constexpr float LV_PHASE2 = 0.65f, LV_PHASE3 = 0.30f, LV_RISE = 3.f, LV_BEACHED = 2.2f, LV_HIDDEN = 1.4f,
                           LV_SPIT_AFTER = 4.f, LV_SPIT_FAR = 34.f, LV_SPIT_HIGH = 4.f, LV_REACH = 44.f, LV_INHALE = 3.f,
                           LV_CHOKE = 450.f, LV_STAGGER = 4.f, LV_PARRY_REACH = 6.f, LV_POOL = 13.f;
    int   levPhase = 1;
    LevStage levStage = LevStage::RISE;
    bool  levInit = false;
    glm::vec3 levRoot{0.f}, levHome{0.f}, levTarget{0.f}, levWakeFrom{0.f};
    float levFloor = 0.f;          // the ring floor's Y
    float levStageT = 0.f;         // RISE / HIDDEN: time left
    float levBeached = 0.f;        // > 0: its head lies on the ring after a crash, the eye open
    float levInhale = 0.f;         // > 0: inhaling (SWALLOW), the throat open
    float levChoke = 0.f;          // damage into the throat this inhale
    float levPitch = 0.f, levJaw = 0.f;   // the head's tilt (+ up) and how far its jaw is open, 0..1
    int   levSiteAttacks = 0;      // attacks made from this root (THE HUNT: two, then it dives)
    int   levSeq = 0;              // where it is in its rotation
    bool  levAtPool() const { return glm::length(glm::vec2{levRoot.x - levHome.x, levRoot.z - levHome.z}) < 1.f; }
};

struct LeviathanEvents {
    // LEVIATHAN (EnemyLeviathan.h): its phase changed (2 the hunt, 3 the
    // eclipse); a crash marked from its root to where you stood, then landing
    // there; a tide rolling out from its root; a spit marked where you'll be;
    // a well boiling, then the breach out of it; an inhale began; it rose
    int       lvPhase = 0;
    bool      lvCrashMark = false, lvCrash = false; glm::vec3 lvFrom{0.f}, lvTo{0.f};   // lvTo: where a crash landed
    glm::vec3 lvMarkTo{0.f};   // where a crash just marked will land (the eclipse marks the second as the first lands)
    bool      lvTide = false; glm::vec3 lvTideAt{0.f}; float lvTideR = 0.f;
    bool      lvSpit = false; glm::vec3 lvSpitAt{0.f};
    bool      lvBreachTell = false, lvBreach = false; glm::vec3 lvSite{0.f};
    bool      lvInhale = false, lvRise = false, lvBeached = false;
};
