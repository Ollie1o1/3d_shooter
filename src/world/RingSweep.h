#pragma once
// =============================================================================
// RingSweep.h — THE SWEEP (Chapter 6, docs/chapter-6-design.md section 4).
//
// The Shepherd's pulse: a wall that crosses the ring and throws everything in
// the open off it. One hit kills. The answer is always the same: be in the
// shadow of something heavy (a SweepShelter) when the wall reaches you.
//
// One object owns the flash, the countdown, the lit posts, the front and the
// kill, so they cannot disagree:
//   IDLE -> FLASH (the ridge lamps; no number yet) -> INBOUND (the countdown)
//        -> PASSING (it has gone over you; later waves and the far side) -> IDLE
//
// Kinds: OUT (a straight wall away from the Shepherd), GATHER (the same wall
// the other way: it throws you toward it, and does not kill) and CONE (the
// boss's aimed sweep: a ring out of a point, lethal only inside the cone).
//
// The countdown is the time until the visible front reaches where you stand.
// A `clocked` sweep instead fixes the moment (the boss, on the music) and
// places the front to match.
//
// No OpenGL and no GameplayState: the tests run this as it is
// (tests/test_ring.cpp). Gameplay_Ring.h kills, throws and draws.
// =============================================================================
#include "core/Collision.h"
#include <glm/glm.hpp>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

// Something heavy enough to stand behind. `post` is where its mooring post
// stands (dark until the flash).
struct SweepShelter {
    AABB      box;
    glm::vec3 post{0.f};
    bool      fixed = true;    // false: an enemy can shove it (the reach test ignores these)
    bool      small = false;   // a breaker sweep shatters it
    bool      holds = false;   // the spike alone saves you on this one (the raft, the hull): level-authored
    bool      alive = true;
    int       wall = -1;       // a level wall whose box this follows (a drifting chunk's boulder)
    int       mover = -1;      // a DRIVEN mover a HERDER can push (fixed = false)
    int       section = -1;    // the arena it belongs to (the reach test checks each on its own)
};

struct SweepSpec {
    enum class Kind { OUT, GATHER, CONE };
    Kind      kind = Kind::OUT;
    glm::vec3 dir{0.f, 0.f, 1.f};   // OUT / GATHER: the way the front travels (flat, unit). CONE: its axis
    glm::vec3 origin{0.f};          // CONE: where it fires from
    float     coneHalf = 0.42f;     // CONE: half angle (radians)
    float     warning = 20.f;       // seconds of countdown
    float     flashLead = 4.f;      // the flash comes this long before the countdown
    float     speed = 60.f;         // m/s
    int       waves = 1;            // 2: the double sweep
    float     waveGap = 3.5f;       // seconds between waves
    bool      breakers = false;     // small shelters go dark a second in, and shatter
    bool      clocked = false;      // the moment is fixed, not the front (the boss, on the beat)
    float     range = 160.f;        // how far past you the front runs before it is over
};

struct RingSweep {
    static constexpr int   MAX_WAVES = 3;
    static constexpr float SHADOW = 14.f;        // how far behind a shelter its shadow reaches
    static constexpr float CHEST = 1.f;          // the height the shadow is tested at, above the feet
    static constexpr float BREAKER_DARK = 1.f;   // small shelters' posts go out this far into a breaker's countdown

    enum class Phase { IDLE, FLASH, INBOUND, PASSING };
    struct Events {
        bool flash = false;     // the ridge lamps lit: posts on, capsule lamps down
        bool begun = false;     // the countdown started
        bool struck = false;    // a wave reached you in the open
        bool cleared = false;   // a wave went over you in shelter (the zero)
        bool ended = false;
        std::vector<int> shattered;   // shelters a breaker broke this tick
    };

    SweepSpec spec;
    Phase     phase = Phase::IDLE;
    float     t = 0.f;         // FLASH: since the flash. After: since the countdown began
    float     prevT = 0.f;
    glm::vec3 base{0.f};       // where you stood when the countdown began (OUT / GATHER measure from it)
    float     startCoord = 0.f;   // your coordinate when the countdown began
    float     youCoord = 0.f;     // ...and on the last update
    int       passed = 0;      // waves that have reached you
    float     passT[MAX_WAVES] = {}, passCoord[MAX_WAVES] = {};

    void reset() { *this = RingSweep{}; }
    bool active() const { return phase != Phase::IDLE; }
    bool lit() const { return active(); }   // the posts
    bool lethal() const { return spec.kind != SweepSpec::Kind::GATHER; }
    int  waves() const { return std::clamp(spec.waves, 1, MAX_WAVES); }
    int  wavesLeft() const { return phase == Phase::INBOUND ? waves() - passed : 0; }

    void start(const SweepSpec& s) {
        reset();
        spec = s;
        glm::vec3 d{s.dir.x, 0.f, s.dir.z};
        float n = glm::length(d);
        spec.dir = n > 1e-4f ? d / n : glm::vec3{0.f, 0.f, 1.f};
        phase = Phase::FLASH;
    }

    // How far along the sweep a point is: metres down the wall's path (OUT /
    // GATHER), or out from the origin (CONE). The front has one too.
    float coord(glm::vec3 p) const {
        if (spec.kind == SweepSpec::Kind::CONE) { glm::vec3 d = p - spec.origin; d.y = 0.f; return glm::length(d); }
        return glm::dot(p - base, spec.dir);
    }
    // The unit way from p back to where the wall comes from
    glm::vec3 toSource(glm::vec3 p) const {
        if (spec.kind != SweepSpec::Kind::CONE) return -spec.dir;
        glm::vec3 d = spec.origin - p; d.y = 0.f;
        float n = glm::length(d);
        return n > 1e-4f ? d / n : -spec.dir;
    }
    bool inPath(glm::vec3 p) const {
        if (spec.kind != SweepSpec::Kind::CONE) return true;
        glm::vec3 d = p - spec.origin; d.y = 0.f;
        float n = glm::length(d);
        return n < 0.5f || glm::dot(d / n, spec.dir) >= std::cos(spec.coneHalf);
    }

    // Wave k's front at time `at` into the countdown
    float frontAt(int k, float at) const {
        if (k < passed) return passCoord[k] + (at - passT[k]) * spec.speed;        // gone over you: on it goes
        const float due = spec.warning + k * spec.waveGap;                         // when it reaches you
        if (spec.clocked) return youCoord - (due - at) * spec.speed;
        return startCoord - (due - at) * spec.speed;
    }
    float front(int k = 0) const { return frontAt(k, t); }

    // Seconds until the next wave reaches `feet`; -1 when no number should show
    float countdown(glm::vec3 feet) const {
        if (phase != Phase::INBOUND || passed >= waves()) return -1.f;
        return std::max(0.f, (coord(feet) - front(passed)) / spec.speed);
    }

    bool shelterLit(const SweepShelter& s) const {
        if (!active() || !s.alive) return false;
        return !(spec.breakers && s.small && phase != Phase::FLASH && t > BREAKER_DARK);
    }
    // Would a shelter save something standing at `feet`? (In a breaker, the
    // small ones don't.)
    template <class Shelters>
    bool sheltered(glm::vec3 feet, const Shelters& shelters) const {
        const glm::vec3 o = feet + glm::vec3{0.f, CHEST, 0.f}, d = toSource(feet);
        for (const SweepShelter& s : shelters) {
            if (!s.alive || (spec.breakers && s.small)) continue;
            const float hit = rayBoxHit(o, d, s.box);
            if (hit > 0.f && hit <= SHADOW) return true;
        }
        return false;
    }
    template <class Shelters>
    bool exposed(glm::vec3 feet, const Shelters& shelters) const { return inPath(feet) && !sheltered(feet, shelters); }

    // Did a front go over `feet`, in the open, on the last update? (enemies,
    // loose things)
    template <class Shelters>
    bool sweptNow(glm::vec3 feet, const Shelters& shelters) const {
        if (phase == Phase::IDLE || phase == Phase::FLASH) return false;
        const float c = coord(feet);
        for (int k = 0; k < waves(); ++k)
            if (frontAt(k, prevT) < c && c <= frontAt(k, t)) return exposed(feet, shelters);
        return false;
    }

    template <class Shelters>
    Events update(float dt, glm::vec3 feet, Shelters& shelters) {
        Events ev;
        if (phase == Phase::IDLE) return ev;
        if (phase == Phase::FLASH) {
            if (t == 0.f) ev.flash = true;
            t += dt;
            if (t < spec.flashLead) return ev;
            phase = Phase::INBOUND; ev.begun = true;
            base = feet; t = prevT = 0.f;
            startCoord = youCoord = coord(feet);
            return ev;
        }
        prevT = t; t += dt;
        youCoord = coord(feet);
        while (passed < waves() && front(passed) >= youCoord) {
            const int k = passed;
            // (set after reading the front: frontAt() switches on `passed`)
            passCoord[k] = youCoord; passT[k] = t; ++passed;
            if (exposed(feet, shelters)) ev.struck = true; else ev.cleared = true;
        }
        if (spec.breakers)
            for (int i = 0; i < (int)shelters.size(); ++i) {
                SweepShelter& s = shelters[i];
                if (!s.alive || !s.small) continue;
                const glm::vec3 c = (s.box.min + s.box.max) * 0.5f;
                if (inPath(c) && front(0) >= coord(c)) { s.alive = false; ev.shattered.push_back(i); }
            }
        if (passed >= waves()) {
            phase = Phase::PASSING;
            if (front(waves() - 1) > youCoord + spec.range) { phase = Phase::IDLE; ev.ended = true; }
        }
        return ev;
    }
};

// When the next one comes. Random inside a window, and never unfair: not just
// after a checkpoint or respawn, not while something else has the stage.
struct SweepSchedule {
    static constexpr float GRACE = 10.f;   // after a checkpoint, a respawn or a block lifting

    float    gapMin = 90.f, gapMax = 150.f;
    float    timer = 120.f;
    float    hold = 0.f;
    uint32_t rng = 0x9E3779B9u;

    float roll() {
        rng = rng * 1664525u + 1013904223u;
        return gapMin + (gapMax - gapMin) * ((rng >> 8) & 0xFFFF) / 65535.f;
    }
    void arm(float lo, float hi, uint32_t seed) { gapMin = lo; gapMax = hi; rng = seed ? seed : 1u; timer = roll(); hold = GRACE; }
    void checkpoint() { hold = std::max(hold, GRACE); }

    // True on the tick a sweep should start. `active`: one is under way.
    // `blocked`: a cutscene, a quiet room, another lethal attack pending.
    // `rate`: 1, or 2 while a TALLY lives (the gap halves; a warning never changes).
    bool tick(float dt, bool active, bool blocked, float rate = 1.f) {
        if (blocked) hold = std::max(hold, GRACE);
        else hold = std::max(0.f, hold - dt);
        if (active) return false;
        timer -= dt * rate;
        if (timer > 0.f || blocked || hold > 0.f) return false;
        timer = roll();
        return true;
    }
};

// The crossing's three distances (section 4's table) and the boss's cone
namespace sweeptier {
struct Tier { SweepSpec spec; float gapMin, gapMax; };
inline Tier far(glm::vec3 dir)  { SweepSpec s; s.dir = dir; s.warning = 20.f; s.flashLead = 4.f; return {s, 90.f, 150.f}; }
inline Tier mid(glm::vec3 dir)  { SweepSpec s; s.dir = dir; s.warning = 10.f; s.flashLead = 2.5f; s.waves = 2; return {s, 70.f, 120.f}; }
inline Tier near(glm::vec3 dir) { SweepSpec s; s.dir = dir; s.warning = 6.f; s.flashLead = 1.5f; s.breakers = true; return {s, 50.f, 90.f}; }
inline SweepSpec aimed(glm::vec3 origin, glm::vec3 at, float warning = 4.f) {
    SweepSpec s; s.kind = SweepSpec::Kind::CONE; s.origin = origin; s.dir = at - origin;
    s.warning = warning; s.flashLead = 0.f; s.clocked = true; s.range = 60.f;
    return s;
}
inline SweepSpec gather(glm::vec3 towardShepherd, float warning = 8.f) {
    SweepSpec s; s.kind = SweepSpec::Kind::GATHER; s.dir = towardShepherd; s.warning = warning; s.flashLead = 1.5f;
    return s;
}
}

// The level-design rule: from anywhere you can stand, a fixed shelter's shadow
// is reachable inside the warning. Straight-line, at `speed`; the level tests
// add a real walk where the ground is not open.
namespace sweepreach {
// The nearest point to `from` that is in a fixed shelter's shadow from a wall
// travelling along `dir`; false if there is none
template <class Shelters>
inline bool nearestShadow(glm::vec3 from, glm::vec3 dir, const Shelters& shelters, glm::vec3& out, bool breakers = false) {
    RingSweep probe; probe.spec.dir = dir; probe.spec.breakers = breakers; probe.base = from;
    const glm::vec3 side{-dir.z, 0.f, dir.x};
    float best = 1e30f; bool found = false;
    for (const SweepShelter& s : shelters) {
        if (!s.alive || !s.fixed || (breakers && s.small)) continue;
        const glm::vec3 c = (s.box.min + s.box.max) * 0.5f, h = (s.box.max - s.box.min) * 0.5f;
        const float along = std::fabs(dir.x) * h.x + std::fabs(dir.z) * h.z;    // the box's reach down the path
        const float across = std::fabs(side.x) * h.x + std::fabs(side.z) * h.z; // ...and across it
        for (int i = -3; i <= 3; ++i)
            for (float back : {1.2f, 4.f, 8.f}) {
                glm::vec3 p = c + dir * (along + back) + side * (across * 0.8f * i / 3.f);
                p.y = from.y;
                if (!probe.sheltered(p, shelters)) continue;
                const float d = glm::length(glm::vec2{p.x - from.x, p.z - from.z});
                if (d < best) { best = d; out = p; found = true; }
            }
    }
    return found;
}
template <class Shelters>
inline float secondsToShelter(glm::vec3 from, glm::vec3 dir, const Shelters& shelters, float speed, bool breakers = false) {
    glm::vec3 at;
    if (!nearestShadow(from, dir, shelters, at, breakers)) return 1e30f;
    return glm::length(glm::vec2{at.x - from.x, at.z - from.z}) / speed;
}
}
