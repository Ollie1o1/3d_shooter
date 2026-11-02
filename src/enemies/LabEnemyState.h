#pragma once
#include <glm/glm.hpp>
#include <algorithm>
#include <cmath>
struct LabEnemyState {
    static constexpr float BEAM_TIME=.8f, BEAM_RANGE=42.f, BEAM_SWEEP=.48f;
    glm::vec3 labAim{0},labFrom{0},labExit{0},labDir{0,0,1};
    float labOpen=0,labBeam=0,labCross=0;
    // The swarm's claw swipe: the strike's follow-through (drawn after the tell
    // resolves) and which hand swings next
    float labSwipe=0;int labSwipes=0;
    bool labBraced=true,labLocked=false;
    // Shared by the damaging ray and its rendered telegraph; the sweep never
    // follows the player after the final lock cue.
    glm::vec3 labBeamDirection() const {
        float angle=(1.f-std::clamp(labBeam/BEAM_TIME,0.f,1.f)-.5f)*BEAM_SWEEP;
        float c=std::cos(angle),s=std::sin(angle);
        return {labDir.x*c+labDir.z*s,labDir.y,-labDir.x*s+labDir.z*c};
    }
};
