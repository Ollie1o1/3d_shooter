#pragma once
#include <algorithm>

// One release per encounter attempt. The warning always completes before the
// moving collision descends; update reports the impact exactly once.
struct EncounterTrap {
    static constexpr float WARNING = 1.3f, FALL = .6f;
    float age = -1.f;
    bool landed = false;

    bool trigger() { if(age>=0.f)return false;age=0.f;return true; }
    float progress() const { return std::clamp((age-WARNING)/FALL,0.f,1.f); }
    bool update(float dt) {
        if(age<0.f || landed)return false;
        age+=std::max(0.f,dt);
        if(age<WARNING+FALL)return false;
        landed=true;return true;
    }
};
