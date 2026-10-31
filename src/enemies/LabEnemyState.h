#pragma once
#include <glm/glm.hpp>
struct LabEnemyState {
    glm::vec3 labAim{0},labFrom{0},labExit{0},labDir{0,0,1};
    float labOpen=0,labBeam=0,labCross=0;
    bool labBraced=true,labLocked=false;
};
