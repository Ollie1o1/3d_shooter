#pragma once
#include <algorithm>
#include <cmath>

// A boss loses health immediately; the small bright damage chunk holds for
// 120 ms, then catches up. Healing/new encounters reset it immediately.
struct DamageTrail {
    float value = 1.f, health = 1.f, hold = 0.f;
    bool initialized = false;
    void reset(float hp) { value = health = std::clamp(hp, 0.f, 1.f); hold = 0.f; initialized = true; }
    void observe(float hp) {
        hp = std::clamp(hp, 0.f, 1.f);
        if (!initialized || hp > health + 0.0001f) { reset(hp); return; }
        if (hp < health - 0.0001f) { value = std::max(value, health); hold = .12f; }
        health = hp;
    }
    void tick(float dt) {
        if (!initialized || dt <= 0.f) return;
        float wait = std::min(dt, hold); hold -= wait; dt -= wait;
        value = health + (value - health) * std::exp(-dt * 8.f);
        if (value - health < .0001f) value = health;
    }
};
