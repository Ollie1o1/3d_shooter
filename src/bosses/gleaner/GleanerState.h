#pragma once
// =============================================================================
// GleanerState.h — THE GLEANER (Chapter 4, docs/chapter-4-design.md): how the
// colossus moves. No OpenGL, so the tests can drive it.
//
// It is about 240 m from its knuckles to its dragging feet. It crawls on two
// huge arms (knuckle-walking: one arm swings while the other bears it, the
// body dipping and rolling onto each planted fist), its withered legs trail,
// the furnace on its hunched back breathes. Every fist that lands is a SLAM
// event; a SCREAM opens its jaw for several seconds. The reveal drives `rise`
// (0 lying curled with its back to you, the ridge on the horizon, 1 up on its
// knuckles), the turn, the eye and the ribs.
//
// Its frame: +Z forward (where the head is), +X its right, +Y up. Facing yaw:
// forward = (sin yaw, 0, cos yaw), so yaw 0 faces +Z (south, toward the arrival).
// =============================================================================
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <cmath>
#include <vector>

struct GleanerEvent {
    enum Kind { SLAM, SCREAM, SWAT_RAISE, SWAT } kind;   // SWAT: a fist come down on its own back (the fight on it)
    glm::vec3 at;
    float size;
};

struct Gleaner {
    // ---- its build (its own frame, in units SCALE metres across: the numbers below are
    // its proportions, SCALE its size) ----
    static constexpr float SCALE = 2.4f;
    static constexpr float SHOULDER_X = 44.f, SHOULDER_Y = 118.f, SHOULDER_Z = 28.f;
    static constexpr float UPPER = 80.f, FORE = 86.f;       // arm bones
    static constexpr float PLANT_X = 66.f, PLANT_Z = 84.f;   // where a fist rests, standing still
    static constexpr float LIFT = 30.f;                      // how high a fist rises as it swings
    static constexpr float SCREAM_LEN = 4.6f;
    static glm::vec3 eyeLocal() { return {0.f, 64.f, 42.f}; }   // low in the chest, under the head
    static glm::vec3 headLocal() { return {0.f, 108.f, 96.f}; }   // thrust out ahead of the shoulders

    struct Arm {
        glm::vec3 plant{0.f}, from{0.f};
        bool  swinging = false;
        float sinceLanded = 9.f;   // seconds since this fist came down (the body dips onto it)
        // A SWAT: kneeling, it lifts this fist up over its own back and brings it down on you there.
        // swatT seconds into it (-1: none); swatAt where the fist comes down (its knuckles' ground)
        float swatT = -1.f;
        glm::vec3 swatAt{0.f};
        bool  dead = false;        // its brood member is dead: the arm hangs, dark, and will not swat
    };
    // The swat's beats: up and over (the tell: its shadow on the roof), down, held there, back to the plain
    static constexpr float SWAT_RAISE = 2.9f, SWAT_FALL = 0.32f, SWAT_HOLD = 2.4f, SWAT_BACK = 2.6f;
    static constexpr float SWAT_LIFT = 95.f;   // how far over its knuckles' mark it holds the fist (metres)

    glm::vec3 pos{0.f};
    float yaw = 0.f, targetYaw = 0.f;
    float speed = 0.f, wantSpeed = 0.f;
    float turnRate = 0.07f;          // rad/s at most
    float gaitT = 0.f;               // 0..1 through a stride: arm 0 swings in [0, .42), arm 1 in [.5, .92)
    float gaitPeriod = 8.f;          // seconds a full stride takes
    float rise = 1.f;                // 0 lying curled .. 1 up on its knuckles
    float kneel = 0.f;               // 0 up .. 1 sunk on its knuckles, holding still (its back a battlefield)
    float eyeOpen = 0.f, ribs = 0.f; // 0..1
    float jaw = 0.f, headLift = 0.f; // 0..1 (the scream)
    float headHold = 0.f;            // 0..1 its head reared up and held still, its crown level with its shoulders (the Caller's ground)
    float screamT = -1.f;            // seconds into a scream (-1: none)
    float clock = 0.f;
    Arm   arm[2];
    std::vector<GleanerEvent> events;   // what happened in the last tick (and a scream() called since)
    size_t ticked = 0;                  // how many of `events` the last tick made (the next tick clears only those)

    glm::vec3 fwd() const { return {std::sin(yaw), 0.f, std::cos(yaw)}; }
    glm::vec3 right() const { return {std::cos(yaw), 0.f, -std::sin(yaw)}; }
    static float side(int a) { return a == 0 ? -1.f : 1.f; }   // arm 0 is its left
    static float ease(float x) { x = std::clamp(x, 0.f, 1.f); return x * x * (3.f - 2.f * x); }

    // How far the body sinks and leans while it lies curled, and its dip onto each fist
    float bob() const {
        float dip = 0.f;
        for (const auto& a : arm) dip += 5.f * SCALE * std::exp(-a.sinceLanded * 2.5f) * (1.f - std::exp(-a.sinceLanded * 18.f));
        return -dip - 128.f * SCALE * (1.f - ease(rise)) - 26.f * SCALE * ease(kneel);   // lying, half sunk in the plain: a mountain range
    }
    float lean() const { return 0.3f * (1.f - ease(rise)); }   // nose down while it lies
    float roll() const {   // onto the planted fist while the other swings
        float r = 0.f;
        for (int a = 0; a < 2; ++a) if (arm[a].swinging) r += -side(a) * 0.035f * std::sin(3.14159f * swingU(a));
        return r;
    }
    float breath() const { return std::sin(clock * 0.55f); }   // -1..1, about 11 s a breath

    // Its body: its own frame into the world (bob, lean, roll and breath applied)
    glm::mat4 body() const {
        // (kneeling it holds its breath: its back must not move under the fight on it)
        glm::mat4 m = glm::translate(glm::mat4(1.f), pos + glm::vec3{0.f, bob() + 1.2f * SCALE * breath() * (1.f - ease(kneel)), 0.f});
        m = glm::rotate(m, yaw, glm::vec3{0, 1, 0});
        m = glm::rotate(m, lean(), glm::vec3{1, 0, 0});
        m = glm::rotate(m, roll(), glm::vec3{0, 0, 1});
        return glm::scale(m, glm::vec3{SCALE});
    }
    glm::vec3 toWorld(glm::vec3 local) const { return glm::vec3(body() * glm::vec4(local, 1.f)); }
    glm::vec3 shoulder(int a) const { return toWorld({side(a) * SHOULDER_X, SHOULDER_Y, SHOULDER_Z}); }
    glm::vec3 eye() const { return toWorld(eyeLocal()); }

    // Where a fist goes: out to its side and ahead, `lead` metres further for the stride.
    // Lying curled, the fists are tucked under its chest.
    glm::vec3 home(int a, float lead) const {
        float r = ease(rise);
        float x = glm::mix(26.f, PLANT_X, r) * SCALE, z = glm::mix(30.f, PLANT_Z, r) * SCALE + lead * r;
        glm::vec3 p = pos + right() * (side(a) * x) + fwd() * z;
        p.y = 0.f;
        return p;
    }
    float swingU(int a) const {
        float start = a == 0 ? 0.f : 0.5f, len = 0.42f;
        return std::clamp((gaitT - start) / len, 0.f, 1.f);
    }
    // The fist, where it is now: planted, or partway along its arc (or a swat's)
    glm::vec3 hand(int a) const {
        if (arm[a].swatT >= 0.f) return swatHand(a);
        if (!arm[a].swinging) return arm[a].plant;
        float u = swingU(a);
        glm::vec3 to = home(a, lead());
        glm::vec3 p = glm::mix(arm[a].from, to, ease(u));
        p.y += LIFT * SCALE * std::sin(3.14159f * u) * (0.4f + 0.6f * ease(rise));
        return p;
    }
    float lead() const { return speed * gaitPeriod * 0.5f; }

    // ---- the swat ----
    bool swatting(int a) const { return arm[a].swatT >= 0.f; }
    bool swattingAny() const { return swatting(0) || swatting(1); }
    // 0..1 how far through the tell (raised over its mark): its shadow darkening, the ring tightening
    float swatTell(int a) const { return swatting(a) ? std::clamp(arm[a].swatT / (SWAT_RAISE + SWAT_FALL), 0.f, 1.f) : 0.f; }
    bool swatDown(int a) const { float t = arm[a].swatT; return t >= SWAT_RAISE + SWAT_FALL && t < SWAT_RAISE + SWAT_FALL + SWAT_HOLD; }
    glm::vec3 swatHand(int a) const {
        const Arm& r = arm[a];
        const float t = r.swatT;
        const glm::vec3 over = r.swatAt + glm::vec3{0.f, SWAT_LIFT, 0.f} - right() * (side(a) * 18.f);   // (held out over its side a little)
        if (t < SWAT_RAISE) {   // up off the plain and over, in an arc out to its side
            float u = ease(t / SWAT_RAISE);
            glm::vec3 p = glm::mix(r.plant, over, u);
            p += right() * (side(a) * 40.f * std::sin(3.14159f * u));
            p.y += 0.f;
            return p + glm::vec3{0.f, 1.5f * std::sin(clock * 9.f), 0.f} * u;   // (straining)
        }
        if (t < SWAT_RAISE + SWAT_FALL) { float u = (t - SWAT_RAISE) / SWAT_FALL; return glm::mix(over, r.swatAt, u * u); }
        if (t < SWAT_RAISE + SWAT_FALL + SWAT_HOLD) return r.swatAt;
        float u = ease((t - SWAT_RAISE - SWAT_FALL - SWAT_HOLD) / SWAT_BACK);
        glm::vec3 p = glm::mix(r.swatAt, r.plant, u);
        p.y += 40.f * std::sin(3.14159f * u);
        return p;
    }
    // Bring fist `a` down on `at` (its knuckles' mark, on its back). False if it can't now
    bool swat(int a, glm::vec3 at) {
        if (arm[a].dead || swattingAny() || arm[a].swinging) return false;
        arm[a].swatT = 0.f; arm[a].swatAt = at;
        events.push_back({GleanerEvent::SWAT_RAISE, at, 1.f});
        return true;
    }

    // Elbow by two-bone IK: bent out to the side and up
    glm::vec3 elbow(int a) const {
        glm::vec3 s = shoulder(a), h = hand(a);
        glm::vec3 d = h - s;
        const float up = UPPER * SCALE, fore = FORE * SCALE;
        float len = std::clamp(glm::length(d), 1.f, (up + fore) * 0.999f);
        glm::vec3 dir = glm::normalize(d);
        float along = (up * up - fore * fore + len * len) / (2.f * len);
        float hgt = std::sqrt(std::max(0.f, up * up - along * along));
        glm::vec3 pole = right() * side(a) + glm::vec3{0.f, 0.6f, 0.f} - fwd() * 0.3f;
        pole -= dir * glm::dot(pole, dir);
        float pl = glm::length(pole);
        pole = pl > 1e-4f ? pole / pl : glm::vec3{0.f, 1.f, 0.f};
        return s + dir * along + pole * hgt;
    }

    // Put it down somewhere, fists planted where they belong
    void place(glm::vec3 p, float facing, float risen) {
        pos = p; pos.y = 0.f; yaw = targetYaw = facing; rise = risen; speed = wantSpeed = 0.f; gaitT = 0.95f;   // (the first stride starts with its left fist)
        for (int a = 0; a < 2; ++a) { arm[a] = Arm{}; arm[a].plant = home(a, 0.f); }
    }
    void scream() {
        screamT = 0.f;
        events.push_back({GleanerEvent::SCREAM, toWorld(headLocal()), 1.f});
    }
    bool screaming() const { return screamT >= 0.f; }

    static float angleTo(float from, float to) {
        float d = std::fmod(to - from + 3.14159265f, 6.2831853f);
        if (d < 0.f) d += 6.2831853f;
        return d - 3.14159265f;
    }

    void tick(float dt) {
        // (only what the last tick made: a scream() called between ticks, by the fight or the reveal,
        // stays for whoever reads this tick's events. Clearing them all here silenced every roar.)
        events.erase(events.begin(), events.begin() + (long)std::min(ticked, events.size()));
        clock += dt;
        // Turn and crawl
        float dy = angleTo(yaw, targetYaw);
        float turn = std::clamp(dy, -turnRate * dt, turnRate * dt);
        yaw += turn;
        speed += (wantSpeed - speed) * std::min(1.f, dt * 0.4f);
        pos += fwd() * (speed * dt);
        // A fist left behind (it turned or crawled on): the stride goes on until both are home
        bool needs = speed > 0.2f || std::abs(dy) > 0.02f;
        for (int a = 0; a < 2 && !needs; ++a)
            needs = glm::length(arm[a].plant - home(a, 0.f)) > 18.f * SCALE && rise > 0.5f;
        for (auto& a : arm) a.sinceLanded += dt;
        for (int a = 0; a < 2; ++a) {   // a swat under way
            Arm& r = arm[a];
            if (r.swatT < 0.f) continue;
            const float was = r.swatT;
            r.swatT += dt;
            const float hit = SWAT_RAISE + SWAT_FALL;
            if (was < hit && r.swatT >= hit) events.push_back({GleanerEvent::SWAT, r.swatAt, 1.f});
            if (r.swatT >= hit + SWAT_HOLD + SWAT_BACK) { r.swatT = -1.f; r.sinceLanded = 0.f; }
        }
        if (needs || arm[0].swinging || arm[1].swinging) {
            float was = gaitT;
            gaitT += dt / gaitPeriod;
            if (gaitT >= 1.f) { gaitT -= 1.f; was -= 1.f; }   // (round the stride: arm 0 starts again at 0)
            for (int a = 0; a < 2; ++a) {
                float start = a == 0 ? 0.f : 0.5f, end = start + 0.42f;
                if (was < start && gaitT >= start && (needs || arm[a].swinging)) { arm[a].swinging = true; arm[a].from = arm[a].plant; }
                if (arm[a].swinging && gaitT >= end) {
                    arm[a].swinging = false;
                    arm[a].plant = home(a, lead());
                    arm[a].sinceLanded = 0.f;
                    events.push_back({GleanerEvent::SLAM, arm[a].plant, 0.6f + 0.4f * ease(rise)});
                }
            }
        }
        // While it lies curled its fists stay tucked under it
        if (rise < 0.999f)
            for (int a = 0; a < 2; ++a) if (!arm[a].swinging) arm[a].plant = glm::mix(arm[a].plant, home(a, 0.f), std::min(1.f, dt * 2.f));
        // The scream: the head comes up, the jaw drops and holds, then both close
        if (screamT >= 0.f) {
            screamT += dt;
            float open = ease(screamT / 0.45f) * (1.f - ease((screamT - SCREAM_LEN + 0.7f) / 0.7f));
            jaw = open;
            headLift = ease(screamT / 0.8f) * (1.f - ease((screamT - SCREAM_LEN + 0.4f) / 1.2f));
            if (screamT > SCREAM_LEN + 0.8f) { screamT = -1.f; jaw = headLift = 0.f; }
        }
        ticked = events.size();
    }
};
