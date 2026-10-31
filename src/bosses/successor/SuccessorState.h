#pragma once
#include "enemies/EnemyTypes.h"
#include <glm/glm.hpp>
struct SuccessorState {
    int suPhase=0,suMove=0,suStep=0,suEpoch=0,suRotation=0,suFinalStep=-1;
    float suPause=0,suOpen=0,suBurst=0,suCross=0,suNodes=0,suEchoCool=0;
    float suNodeHp[2]={35,35};
    glm::vec3 suAim{0},suDir{0,0,1},suFrom{0},suExit{0},suCentre{0},suAnchor[2]{};
    bool suLocked=false,suContact=false,suFinal=false,suCounterLine=false;
    static bool white(AttackKind k){return k==AttackKind::SU_THRUST||k==AttackKind::SU_TRANSPOSE;}
};
struct SuccessorEvents {
    int suPhaseStarted=0,suLine=-1;
    bool suMark=false,suSuture=false,suRecall=false,suCore=false,suEcho=false,suImpact=false,suLock=false;
    glm::vec3 suAt{0},suTo{0};
};
