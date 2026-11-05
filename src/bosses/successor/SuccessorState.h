#pragma once
#include "enemies/EnemyTypes.h"
#include <glm/glm.hpp>
struct SuccessorState {
    static constexpr float TRACK_LOCK=.2f, PARRY_REACH=7.5f, CROSS_TIME=.22f, BREAKER_TIME=.42f;
    static constexpr int BODY_CHANNELS=18;
    int suLastMove=-1,suRepeat=0,suSeverRows=0;float suSeverT=0;
    int suPhase=0,suMove=0,suStep=0,suEpoch=0,suRotation=0,suFinalStep=-1,suDashes=0,suVolley=0,suLandedStep=0;
    float suPause=0,suOpen=0,suBurst=0,suCross=0,suNodes=0,suEchoCool=0,suAir=0,suVolleyT=0;
    AttackKind suLanded=AttackKind::NONE;
    float suSince=9.f,suGait=0;
    float suBody[BODY_CHANNELS]{},suBodyPrev[BODY_CHANNELS]{},suBodyVel[BODY_CHANNELS]{};
    bool suBodyLive=false;
    float suNodeHp[2]={35,35};
    glm::vec3 suAim{0},suDir{0,0,1},suFrom{0},suExit{0},suCentre{0},suAnchor[2]{};
    bool suLocked=false,suContact=false,suFinal=false,suCounterLine=false;
    static bool white(AttackKind k){return k==AttackKind::SU_THRUST||k==AttackKind::SU_TRANSPOSE;}
};
struct SuccessorEvents {
    int suPhaseStarted=0,suLine=-1;
    bool suLattice=false,suCascade=false;int suRows=1;
    bool suMark=false,suSuture=false,suRecall=false,suCore=false,suEcho=false,suImpact=false,suLock=false,suBreaker=false,suReap=false;
    glm::vec3 suAt{0},suTo{0};
};
