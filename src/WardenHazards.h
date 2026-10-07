#pragma once
// =============================================================================
// WardenHazards.h — what THE WARDEN throws at you besides its body and its
// orbs: the lance (a beam sweeping toward you that a pillar stops), the
// seeker (an orb arcing over cover onto where you stood), the steam off its
// open vents; and the clock its conduits re-attach by. No OpenGL or audio:
// GameplayState feeds it the Warden's events and turns hits into damage
// (Gameplay_Warden.h).
// =============================================================================
#include "Player.h"   // AABB, Wall
#include <glm/glm.hpp>
#include <algorithm>
#include <cmath>
#include <vector>

// All four conduits cut: 20 s on, they re-attach one every 5 s. update()
// returns how many should be attached again by now (0..4)
struct ConduitClock {
    static constexpr float AFTER = 20.f, GAP = 5.f;
    float t = -1.f;
    void allCut() { t = 0.f; }
    void reset()  { t = -1.f; }
    bool running() const { return t >= 0.f; }
    int update(float dt) {
        if (t < 0.f) return 0;
        t += dt;
        if (t < AFTER) return 0;
        return std::min(4, 1 + (int)((t - AFTER) / GAP));
    }
};
