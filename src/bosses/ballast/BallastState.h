#pragma once
#include "bosses/StoryBossTools.h"
// A road grader with a failing brake and a turbine-driven clearance blower.
// Shared by native gameplay, the model and deterministic boss simulation.
struct BallastState {
    glm::vec3 ballObservedVel{0};
    int ballPhase = 0, ballStep = 0;
    AttackKind ballFollow = AttackKind::NONE,ballLast=AttackKind::NONE;
    float ballRushCd=0,ballWindCd=0,ballWinchCd=0,ballRipCd=0,ballThrowCd=0,ballSweepCd=0;
    float ballTether=0,ballCableShow=0,ballSwing=0,ballVolleyT=0;
    int ballPasses=0,ballVolley=0;
    bool ballLocked=false,ballHaul=false;
    glm::vec3 ballCableAt{0},ballVolleyAim{0},ballSwingDir{0,0,1};
    float ballPause = 0.f, ballOpen = 0.f, ballDrive = 0.f, ballRotor = 0.f;
    glm::vec3 ballAim{0.f}, ballFrom{0.f}, ballDir{0,0,1};
    bool ballContact = false, ballReverse = false;
    static constexpr float BALL_ANCHOR_R = 2.8f, BALL_CUT_HEIGHT = 1.4f;
};
struct BallastEvents {
    int ballPhaseStarted = 0;
    bool ballGust = false, ballAnchor = false, ballGrade = false;
    bool ballRip=false,ballWinch=false,ballYank=false,ballSweep=false,ballLocked=false;
    glm::vec3 ballFrom{0.f}, ballTo{0.f};
};
