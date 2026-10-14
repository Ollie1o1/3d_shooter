#pragma once
// =============================================================================
// Sword.h — OATHBREAKER, the sword (only in the Zenith, Gameplay_Sword.h):
// its strokes, the charged thrust, the parry window, the block and its guard
// meter, the riposte. No OpenGL or audio: GameplayState feeds it input and
// asks it questions, so the tests drive it directly.
//
//   LMB        a 3-stroke combo (70 / 70 / 120); hold 0.5 s and release: a
//              THRUST 8 m forward (220). Slam with it out: a PLUNGE.
//   RMB tap    the first 0.15 s of a raise PARRIES: a white strike, a
//              projectile, a ray just fired. The next stroke within 1 s is a
//              RIPOSTE (x2.5). A raise within 0.4 s of a missed parry can't.
//   RMB held   BLOCK: hits from the front do 25 %, you move at 60 %. Drains
//              the guard (white 25, gold 60, a beam 15/s; refills 30/s after
//              1 s down). Empty: GUARD BREAK, stunned 1 s.
// =============================================================================
#include <glm/glm.hpp>
#include <algorithm>
#include <cmath>

struct SwordInput { bool swingPressed = false, swingHeld = false, guardPressed = false, guardHeld = false; };

struct Sword {
    static constexpr float SWING_TIME = 0.30f, COMBO_RESET = 0.6f, REACH = 3.2f, ARC_DEG = 100.f,
                           STEP_RANGE = 6.f, STEP_DIST = 2.f,
                           CHARGE_TIME = 0.5f, THRUST_DIST = 8.f, THRUST_TIME = 0.2f, THRUST_DAMAGE = 220.f,
                           PLUNGE_DAMAGE = 150.f, PLUNGE_RADIUS = 4.f,
                           PARRY_WINDOW = 0.15f, SPAM_LOCK = 0.4f, RIPOSTE_TIME = 1.f, RIPOSTE_MULT = 2.5f,
                           GUARD_MAX = 100.f, DRAIN_WHITE = 25.f, DRAIN_GOLD = 60.f, DRAIN_BEAM = 15.f,
                           REFILL = 30.f, REFILL_DELAY = 1.f, BREAK_TIME = 1.f, BLOCK_TAKEN = 0.25f, BLOCK_SPEED = 0.6f,
                           BROKEN_SPEED = 0.3f,
                           HEAL_HIT = 6.f, HEAL_PARRY = 10.f;   // a gun hit heals 2 (Gameplay_Combat.h): the sword 3x
    static constexpr float strokeBase(int s) { return s >= 2 ? 120.f : 70.f; }

    enum class Strike { WHITE, GOLD, BEAM };
    enum class Guarded { NONE, PARRIED, BLOCKED, BROKE };
    struct Out { int stroke = -1; bool thrust = false; };

    int   combo = 0;           // the next stroke: 0 right-to-left, 1 left-to-right, 2 overhead
    int   lastStroke = 0;
    float swingT = 0.f;        // > 0: a stroke is in motion
    float sinceSwing = 1e9f;   // since the last stroke began
    bool  buffered = false;    // clicked mid-stroke: the next follows straight on
    float heldFor = 0.f;       // the swing button, held
    bool  charged = false;
    float thrustT = 0.f;       // > 0: thrusting (the game moves you)
    float guard = GUARD_MAX, sinceBlock = 1e9f, brokenT = 0.f;
    bool  raised = false;
    float raisedFor = 0.f;
    bool  windowLive = false;  // this raise's parry window: not yet used or missed
    float sinceMiss = 1e9f;
    float riposteT = 0.f;

    bool  broken() const    { return brokenT > 0.f; }
    bool  parryOpen() const { return raised && windowLive && raisedFor < PARRY_WINDOW; }
    bool  blocking() const  { return raised && !broken(); }
    bool  swinging() const  { return swingT > 0.f; }
    float moveMult() const  { return broken() ? BROKEN_SPEED : blocking() ? BLOCK_SPEED : 1.f; }
    float guard01() const   { return guard / GUARD_MAX; }

    // Put away (a gun drawn, a retry): nothing carries over but the meter
    void holster() {
        float g = guard, b = brokenT, m = sinceMiss;
        *this = Sword{};
        guard = g; brokenT = b; sinceMiss = m;
    }

    Out update(float dt, const SwordInput& in) {
        Out o;
        sinceSwing += dt; sinceMiss += dt;
        riposteT = std::max(0.f, riposteT - dt);
        thrustT  = std::max(0.f, thrustT - dt);
        brokenT  = std::max(0.f, brokenT - dt);
        // ---- the guard ----
        bool was = raised;
        raised = in.guardHeld && !broken();
        if (raised && (!was || in.guardPressed)) { raisedFor = 0.f; windowLive = sinceMiss >= SPAM_LOCK; }
        else if (raised) raisedFor += dt;
        if (windowLive && (!raised || raisedFor >= PARRY_WINDOW)) { windowLive = false; sinceMiss = 0.f; }   // missed
        if (raised) sinceBlock = 0.f; else sinceBlock += dt;
        if (!raised && !broken() && sinceBlock >= REFILL_DELAY) guard = std::min(GUARD_MAX, guard + REFILL * dt);
        // ---- strokes ----
        swingT = std::max(0.f, swingT - dt);
        if (raised || broken()) { heldFor = 0.f; charged = false; buffered = false; return o; }
        if (in.swingPressed) { if (swingT > 0.f) buffered = true; else startStroke(o); }
        else if (buffered && swingT <= 0.f) { buffered = false; startStroke(o); }
        if (in.swingHeld) { heldFor += dt; if (heldFor >= CHARGE_TIME) charged = true; }
        else {
            if (charged) { o.thrust = true; thrustT = THRUST_TIME; }
            heldFor = 0.f; charged = false;
        }
        return o;
    }

    // A strike reaching you. Only a white one in the window is parried; from
    // the front while raised, anything else is blocked.
    Guarded receive(Strike k, bool fromFront, float dt = 0.f) {
        if (!fromFront || !blocking()) return Guarded::NONE;
        if (k == Strike::WHITE && parryOpen()) { windowLive = false; riposteT = RIPOSTE_TIME; return Guarded::PARRIED; }
        guard -= k == Strike::WHITE ? DRAIN_WHITE : k == Strike::GOLD ? DRAIN_GOLD : DRAIN_BEAM * dt;
        if (guard <= 0.f) { guard = 0.f; brokenT = BREAK_TIME; raised = false; windowLive = false; return Guarded::BROKE; }
        return Guarded::BLOCKED;
    }
    // A projectile or a ray caught in the window
    bool tryParry() {
        if (!parryOpen()) return false;
        windowLive = false; riposteT = RIPOSTE_TIME;
        return true;
    }
    float strokeDamage(int stroke) { return spendRiposte(strokeBase(std::clamp(stroke, 0, 2))); }
    float thrustDamage() { return spendRiposte(THRUST_DAMAGE); }

    // ---- hit tests ----
    // Is a target (centre c, radius r) inside a stroke's arc from the eye?
    static bool inArc(glm::vec3 eye, glm::vec3 fwd, glm::vec3 c, float r) {
        glm::vec3 d = c - eye;
        float dist = glm::length(d);
        if (dist - r > REACH || std::fabs(d.y) > 2.5f + r) return false;
        glm::vec2 f{fwd.x, fwd.z}, dd{d.x, d.z};
        if (glm::length(f) < 1e-4f || glm::length(dd) < 0.3f) return true;
        float half = glm::radians(ARC_DEG * 0.5f) + std::atan2(r, std::max(glm::length(dd), 0.1f));
        return glm::dot(glm::normalize(f), glm::normalize(dd)) >= std::cos(std::min(half, 3.1415f));
    }
    // Does a thrust from `from` to `to` pass through a target at c?
    static bool thrustHits(glm::vec3 from, glm::vec3 to, glm::vec3 c, float r) {
        glm::vec3 ab = to - from;
        float t = glm::clamp(glm::dot(c - from, ab) / std::max(glm::dot(ab, ab), 1e-6f), 0.f, 1.f);
        glm::vec3 q = from + ab * t;
        return glm::length(glm::vec2{c.x - q.x, c.z - q.z}) < r + 1.f && std::fabs(c.y - q.y) < 2.5f;
    }
    // A stroke's lock-on step: up to STEP_DIST toward a target within
    // STEP_RANGE and 60 degrees of the crosshair, stopping 1 m short of it
    static glm::vec3 lockStep(glm::vec3 feet, glm::vec3 fwd, glm::vec3 target, float r) {
        glm::vec2 d{target.x - feet.x, target.z - feet.z}, f{fwd.x, fwd.z};
        float l = glm::length(d);
        if (l < 1e-3f || l > STEP_RANGE || glm::length(f) < 1e-4f) return glm::vec3{0.f};
        if (glm::dot(d / l, glm::normalize(f)) < 0.5f) return glm::vec3{0.f};
        float step = std::clamp(l - r - 1.f, 0.f, STEP_DIST);
        return glm::vec3{d.x / l, 0.f, d.y / l} * step;
    }

private:
    void startStroke(Out& o) {
        if (sinceSwing > SWING_TIME + COMBO_RESET) combo = 0;
        o.stroke = lastStroke = combo;
        combo = (combo + 1) % 3;
        swingT = SWING_TIME; sinceSwing = 0.f;
    }
    float spendRiposte(float d) { if (riposteT > 0.f) { riposteT = 0.f; return d * RIPOSTE_MULT; } return d; }
};

// Key 5: the sword only in the Zenith (or anywhere with --sword, for testing)
inline bool swordKeyAllowed(bool devSword, bool act2, int arena, int zenithArena) {
    return devSword || (act2 && arena == zenithArena);
}
