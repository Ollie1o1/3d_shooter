#pragma once
// =============================================================================
// Recall.h — UNIT 24's RECALL (Chapter 5, docs/chapter-5-design.md section 4).
//
// Press once: a frozen statue of you stays where you stand. Press again within
// WINDOW seconds: you snap back to it, and it shatters, hurting everything
// close to it. Left alone, it crumbles harmlessly. For its first LURE seconds
// nearby enemies take the statue for you.
//
// It moves your position only, never the view (the Chapter 3 camera rule).
//
// No OpenGL and no GameplayState: the timings and state live here so the tests
// can check them. Gameplay_Recall.h moves the player, lures and draws.
// =============================================================================
#include <glm/glm.hpp>
#include <algorithm>

inline const glm::vec3 RECALL_FROST{0.72f, 0.86f, 0.95f};   // the statue's frost, and RECALL's colour

struct RecallSystem {
    static constexpr float WINDOW = 8.f;          // how long the statue waits for you
    static constexpr float MIN_AGE = 0.25f;       // a double tap does not snap you back in place
    static constexpr float COOLDOWN = 3.f;        // after a recall or a crumble, before the next statue
    static constexpr float LURE = 1.75f;          // enemies near it take it for you this long
    static constexpr float LURE_RANGE = 22.f;     // ...if they are within this of it
    static constexpr float SHATTER_RADIUS = 5.f;
    static constexpr float SHATTER_DAMAGE = 65.f;

    enum class Press { NONE, PLACED, RECALLED };

    bool      placed = false;
    glm::vec3 feet{0.f};     // where the statue stands (your feet when you left it)
    float     yaw = 0.f;     // the way it faces (radians, model convention: forward = {sin, 0, cos})
    float     age = 0.f;
    float     cooldown = 0.f;

    void reset() { *this = RecallSystem{}; }

    bool ready() const { return !placed && cooldown <= 0.f; }
    bool luring() const { return placed && age < LURE; }
    float left01() const { return placed ? std::clamp(1.f - age / WINDOW, 0.f, 1.f) : 0.f; }
    float cooldown01() const { return std::clamp(1.f - cooldown / COOLDOWN, 0.f, 1.f); }

    // The button. PLACED: a statue at `at`. RECALLED: snap to `feet` (read it
    // before the next press) and shatter it there.
    Press press(glm::vec3 at, float faceYaw) {
        if (placed) {
            if (age < MIN_AGE) return Press::NONE;
            placed = false; cooldown = COOLDOWN;
            return Press::RECALLED;
        }
        if (cooldown > 0.f) return Press::NONE;
        placed = true; feet = at; yaw = faceYaw; age = 0.f;
        return Press::PLACED;
    }

    // True on the tick the statue crumbles, unused
    bool tick(float dt) {
        cooldown = std::max(0.f, cooldown - dt);
        if (!placed) return false;
        age += dt;
        if (age < WINDOW) return false;
        placed = false; cooldown = COOLDOWN;
        return true;
    }
};
