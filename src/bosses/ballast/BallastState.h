#pragma once
// A road grader with a failing brake and a turbine-driven clearance blower.
// Shared by native gameplay, the model and deterministic boss simulation.
struct BallastState {
    int ballPhase = 0, ballStep = 0;
    AttackKind ballFollow = AttackKind::NONE;
    float ballPause = 0.f, ballOpen = 0.f, ballDrive = 0.f, ballRotor = 0.f;
    glm::vec3 ballAim{0.f}, ballFrom{0.f}, ballDir{0,0,1};
    bool ballContact = false, ballReverse = false;
    static constexpr float BALL_ANCHOR_R = 2.8f, BALL_CUT_HEIGHT = 1.4f;
};
struct BallastEvents {
    int ballPhaseStarted = 0;
    bool ballGust = false, ballAnchor = false, ballGrade = false;
    glm::vec3 ballFrom{0.f}, ballTo{0.f};
};
