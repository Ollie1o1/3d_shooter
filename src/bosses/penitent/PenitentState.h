#pragma once
// =============================================================================
// PenitentState.h — what THE PENITENT, the chained colossus at the bottom of the Descent,
// remembers between ticks, its tuning constants, and the events it reports.
//
// Enemy inherits PenitentState and EnemyEvents inherits PenitentEvents, so the
// fields read like any other enemy's (`e.anchorsLeft`, `e.ev.penSweep`). Keeping them
// here keeps Enemy.h about what every enemy shares. Behaviour: EnemyPenitent.h.
// =============================================================================
#include <glm/glm.hpp>
#include "enemies/EnemyTypes.h"

struct PenitentState {
    static constexpr float PEN_RISE_TIME = 2.f, PEN_LASH_FAR = 22.f, PEN_LASH_HIGH = 4.f, PEN_LASH_AFTER = 3.f,
                           PEN_LASH_WARN = 0.7f, PEN_INCENSE_EVERY = 12.f, PEN_SUMMON_EVERY = 25.f, PEN_SCOURGE_EVERY = 6.f;
    int   anchorsLeft  = 0;      // its chains still holding (GameplayState sets it every tick)
    bool  risen        = false;  // every chain broken: it stands and walks
    float riseTimer    = 0.f;
    bool  scourging    = false;  // under a quarter: it lashes itself, the wound on its back open
    float incenseTimer = 6.f, summonTimer = PEN_SUMMON_EVERY, scourgeTimer = 4.f;
    float scourgeRest  = 0.f;    // after its ember ring: a breath before the next blow
    int   nextSweep    = 0;      // which censer sweep comes next (low / high)
};

struct PenitentEvents {
    // PENITENT: a censer sweep landed (0 low: jump it, 1 high: slide under
    // it), a slam / stomp ring, a chain lash marked toward you, incense pools,
    // a ring of embers off its own back, the moment it rises, Husks called up
    int       penSweep = -1;
    bool      penSlam = false, penStomp = false, penEmbers = false, penRose = false, penLash = false;
    glm::vec3 penLashFrom{0.f}, penLashDir{0.f};
    int       penIncense = 0;
    glm::vec3 penIncensePos[3];
    int       penSummon = 0;
};
