#pragma once
// Pure state shared by AI, rendering, gameplay and the headless boss simulation.
struct SorterState {
    int sortPhase = 0, sortStep = 0, sortFollowups = 0;
    float sortMark = 0.f, sortScan = 0.f, sortOpen = 0.f, sortPause = 0.f;
    glm::vec3 sortAim{0.f}, sortHome{0.f}, sortImpactAt{0.f};
    float sortImpactT = -1.f;
    bool sortLocked = false, sortDropCued = false;
    AttackKind sortImpactKind = AttackKind::NONE;
    float sortScanYaw = 0.f, sortScanSlope = -.3f;
    bool sortHomeSet = false;
    static constexpr float SORT_STAMP_R = 3.3f, SORT_GRAB_R = 3.1f;
    static constexpr float SORT_LOCK = .55f, SORT_DROP = .20f, SORT_IMPACT = .50f;
    static constexpr float SORT_PRESS_UP = 8.f, SORT_PRESS_DOWN = .4f;
    static float sortPressHeight(float remaining,float impactTime=-1.f,float raised=SORT_PRESS_UP) {
        if(impactTime>=0) {
            float u=glm::clamp((impactTime-.12f)/.38f,0.f,1.f);u=u*u*(3.f-2.f*u);
            return SORT_PRESS_DOWN+(raised-SORT_PRESS_DOWN)*u;
        }
        float u=glm::clamp(remaining/SORT_DROP,0.f,1.f);
        return SORT_PRESS_DOWN+(raised-SORT_PRESS_DOWN)*u*u;
    }
};
struct SorterEvents {
    int sortPhaseStarted = 0;
    bool sortStamp = false, sortGrab = false, sortMarked = false, sortLocked = false, sortDrop = false;
    glm::vec3 sortAt{0.f};
};
