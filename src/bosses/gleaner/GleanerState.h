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
    enum Kind { SLAM, SCREAM } kind;
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
    };

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
    float screamT = -1.f;            // seconds into a scream (-1: none)
    float clock = 0.f;
    Arm   arm[2];
    std::vector<GleanerEvent> events;   // what happened in the last tick

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
    // The fist, where it is now: planted, or partway along its arc
    glm::vec3 hand(int a) const {
        if (!arm[a].swinging) return arm[a].plant;
        float u = swingU(a);
        glm::vec3 to = home(a, lead());
        glm::vec3 p = glm::mix(arm[a].from, to, ease(u));
        p.y += LIFT * SCALE * std::sin(3.14159f * u) * (0.4f + 0.6f * ease(rise));
        return p;
    }
    float lead() const { return speed * gaitPeriod * 0.5f; }

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
        events.clear();
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
    }
};
