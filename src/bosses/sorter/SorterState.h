#pragma once
// Pure state shared by AI, rendering, gameplay and the headless boss simulation.
struct SorterState {
    int sortPhase = 0, sortStep = 0;
    float sortMark = 0.f, sortScan = 0.f, sortOpen = 0.f, sortPause = 0.f;
    glm::vec3 sortAim{0.f}, sortHome{0.f};
    float sortScanYaw = 0.f, sortScanSlope = -.3f;
    bool sortHomeSet = false;
    static constexpr float SORT_STAMP_R = 3.3f, SORT_GRAB_R = 3.1f;
};
struct SorterEvents {
    int sortPhaseStarted = 0;
    bool sortStamp = false, sortGrab = false, sortMarked = false;
    glm::vec3 sortAt{0.f};
};
