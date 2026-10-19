#pragma once
// =============================================================================
// WardenState.h — what THE WARDEN, the Core's reactor boss,
// remembers between ticks, its tuning constants, and the events it reports.
//
// Enemy inherits WardenState and EnemyEvents inherits WardenEvents, so the
// fields read like any other enemy's (`e.ventTimer`, `e.ev.wLance`). Keeping them
// here keeps Enemy.h about what every enemy shares. Behaviour: EnemyWarden.h.
// =============================================================================
#include <glm/glm.hpp>
#include "enemies/EnemyTypes.h"

struct WardenState {
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
    glm::vec3 roundTo{0.f}; bool rounding = false;   // meltdown: the corner it's going round (EnemyWarden.h)
};

struct WardenEvents {
    // WARDEN: its phase changed (2 overload, 3 meltdown); a lance began (from,
    // start yaw, which way it sweeps); a seeker marked where you stand; its
    // vent opened; the meltdown went off
    int       wPhase = 0;
    bool      wLance = false; glm::vec3 wLanceFrom{0.f}; float wLanceYaw = 0.f, wLanceSign = 1.f;
    bool      wSeeker = false; glm::vec3 wSeekerAt{0.f};
    bool      wVent = false, wDetonate = false;
};
