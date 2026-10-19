#pragma once
// =============================================================================
// SovereignState.h — what THE SOVEREIGN, the Sanctum's duellist,
// remembers between ticks, its tuning constants, and the events it reports.
//
// Enemy inherits SovereignState and EnemyEvents inherits SovereignEvents, so the
// fields read like any other enemy's (`e.whirlTimer`, `e.ev.phantoms`). Keeping them
// here keeps Enemy.h about what every enemy shares. Behaviour: EnemySovereign.h.
// =============================================================================
#include <glm/glm.hpp>
#include "enemies/EnemyTypes.h"

struct SovereignState {
    // His reach: a sweep's, the overhead cleave's
    static constexpr float SWEEP_REACH  = 5.2f;
    static constexpr float CLEAVE_REACH = 5.8f;
    int   comboStep  = 0;       // sweeps landed in the current combo
    int   chainLeft  = 0;       // dashes still to come in this chain
    int   crescentLeft = 0;
    float dashAge    = 0.f;
    glm::vec3 dashFrom{0.f};    // where the last tick of the dash started
    float leapTimer  = 0.f;     // > 0 while airborne
    float swingTimer = 0.f;     // follow-through after a stroke lands (animation)
    AttackKind lastSwing = AttackKind::NONE;
    int   lastSwingStep = 0;
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
};

struct SovereignEvents {
    int       phantoms = 0;          // phantoms of him, set to dash at you
    glm::vec3 phantomPos[2], phantomDir[2];
    bool      leapStarted = false;   // he leapt (onto whatever you stand on)
};
