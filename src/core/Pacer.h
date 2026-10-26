#pragma once
// =============================================================================
// Pacer.h — low-latency frame pacing (main.cpp's limiter). If a frame had to
// wait for the GPU before it could start, the CPU had run ahead: the frame on
// screen was built from input a couple of frames old, and a GPU flat out
// starves the window compositor (hitches). So the pacer slows the schedule by
// part of what was waited, and when there's no wait it creeps faster again. It
// settles just above the GPU's own pace: the GPU's queue stays empty and every
// frame starts from fresh input. Below what the GPU can do, it does nothing.
// =============================================================================
#include <algorithm>

struct Pacer {
    double period = 0.0;          // seconds a frame; 0 = no limit of its own
    double kUp = 0.3, kDown = 0.99;
    static constexpr double WAIT_MIN = 0.0002;   // a wait shorter than this is noise
    void reset() { period = 0.0; }
    // After each frame's GPU wait: how long it waited (seconds)
    void onWait(double waited) {
        if (waited > WAIT_MIN) period = std::min(0.1, period + waited * kUp);
        else period *= kDown;
    }
};
