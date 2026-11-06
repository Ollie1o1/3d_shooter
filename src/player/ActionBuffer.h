#pragma once

// A short-lived press belongs to the weapon/action the player requested.
// Absolute time makes expiration independent of render rate and physics ticks.
struct ActionBuffer {
    static constexpr float WINDOW = 0.10f;
    int context = -1;
    float until = -1.f;
    void press(int action, float now) { context = action; until = now + WINDOW; }
    bool pending(int action, float now) const { return context == action && now <= until; }
    void clear() { context = -1; until = -1.f; }
};
