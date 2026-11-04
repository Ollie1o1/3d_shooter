#pragma once
// =============================================================================
// Caller.h — THE CALLER, the third of the Gleaner's brood (docs/chapter-4-design.md
// §5, phase 2), over the crown of its reared-up head. No OpenGL: the tests drive it.
//
// A cracked iron bell the size of a house, flying on ragged wings of chain and
// cloth. Where its clapper should hang there is a single red lamp: its weak
// point (x3), and you only see it from below, up into the bell's mouth.
//
//   TOLL  it stops, swings and rings: the sky answers. A meteor is called down
//         on you (marked, it hurts); enraged, three.
//   DIVE  it tips its mouth at you (the lamp flares white: the tell, and the lamp
//         open to you from the front), then dives in a straight committed line
//         through where you stood and climbs away. Punch it as it arrives: it
//         is thrown down onto the crown, on its side, the lamp open from
//         everywhere (x2) until it gets up.
//
// Its shell turns rounds aside (x0.4, a clang) unless they come up into its
// mouth, or off a SPLINTER. It circles high: splinters are the main answer.
//
//   RING IT  every round that clangs off its shell sets it ringing (it glows
//          brighter, its note climbs). Ring it all the way and it cannot stop:
//          OVERRUNG, it sags low over its ground, reeling, tolling wildly, its
//          lamp showing from every side, and the sky answers its own bell: a
//          meteor comes down on it. Struck, it crashes onto the crown on its
//          side like a parried dive (x2). The shots that do nothing are the
//          way to bring it down.
// =============================================================================
#include <glm/glm.hpp>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <utility>
#include <vector>

struct CallerEvent {
    enum Kind { TOLL, CALL, DIVE_TELL, DIVE, PARRIED, LAND, ROAR, DIE, WINGBEAT, OVERRING, SELF_CALL, STRUCK } kind;
    glm::vec3 at{0.f};
};

struct Caller {
    enum State { DORMANT, ARRIVING, CIRCLE, TOLL, DIVE_AIM, DIVE, CLIMB, STUNNED, OVERRUNG, DEAD };
    static constexpr float MAX_HP = 2200.f;
    static constexpr float BELL_R = 3.4f, BELL_H = 7.f;       // its mouth's radius, its height
    static constexpr float ALT = 24.f, ORBIT_R = 30.f;        // where it circles, over its ground
    static constexpr float DIVE_SPEED = 40.f, PARRY_REACH = 8.5f, HIT_R = 4.2f;
    static constexpr float MOUTH = 0.5f;                       // how square up into its mouth a round must come
    static constexpr float ARRIVE_TIME = 4.5f;
    static constexpr float RING_DMG = 150.f;       // shell damage (after its x0.4) that rings it all the way
    static constexpr float OVERRUN_TIME = 2.4f;    // reeling under its own meteor (MeteorRain::CALLED_FALL)
    static constexpr float STRIKE_DMG = 520.f;     // its own meteor on it
    static constexpr float SAG = 9.f;              // how low over its ground it reels

    State state = DORMANT;
    glm::vec3 pos{0.f}, vel{0.f};
    glm::vec3 axis{0.f, 1.f, 0.f};    // up through the bell (its mouth faces -axis)
    float hp = MAX_HP;
    float t = 0.f, clock = 0.f, cooldown = 3.f;
    float flash = 0.f;
    float orbitA = 0.f;
    float swing = 0.f;                // 0..1 the toll's swing
    glm::vec3 diveTo{0.f}, diveDir{0.f, -1.f, 0.f}, diveFrom{0.f};
    bool  hitDone = false;            // this dive has already struck (or been parried)
    int   dives = 0;
    float flap = 0.f;                 // wing phase
    glm::vec3 centre{0.f};            // its ground's middle
    float floorY = 0.f;
    glm::vec3 arriveFrom{0.f};
    float resonance = 0.f;            // 0..1 how hard it is ringing from your shots
    glm::vec3 selfAt{0.f};            // where its own meteor will land (under it)
    glm::vec3 overFrom{0.f};
    uint32_t seed = 313u;
    std::vector<CallerEvent> events;

    float rnd() { seed = seed * 1664525u + 1013904223u; return (float)(seed >> 8) / 16777216.f; }
    bool alive() const { return state != DEAD && state != DORMANT; }
    bool fighting() const { return alive() && state != ARRIVING; }
    bool enraged() const { return hp < MAX_HP * 0.5f; }
    float health01() const { return std::max(0.f, hp / MAX_HP); }
    static float ease(float x) { x = std::clamp(x, 0.f, 1.f); return x * x * (3.f - 2.f * x); }
    bool stunned() const { return state == STUNNED; }
    bool grounded() const { return state == STUNNED || state == DEAD; }

    // Down out of the sky over its ground
    void wake(glm::vec3 ground, float y, float fromAngle) {
        *this = Caller{};
        centre = ground; centre.y = y; floorY = y;
        orbitA = fromAngle;
        arriveFrom = orbitPoint() + glm::vec3{std::cos(fromAngle) * 120.f, 170.f, std::sin(fromAngle) * 120.f};
        pos = arriveFrom;
        state = ARRIVING; t = 0.f;
    }
    glm::vec3 orbitPoint() const {
        return centre + glm::vec3{std::cos(orbitA) * ORBIT_R, ALT + 3.f * std::sin(clock * 0.7f), std::sin(orbitA) * ORBIT_R};
    }
    glm::vec3 lamp() const { return pos - axis * 1.3f; }
    float lampFlare() const { return state == DIVE_AIM ? ease(t / 0.4f) : state == DIVE ? 1.f : 0.f; }
    float aimTime() const { return enraged() ? 0.8f : 1.05f; }

    struct Box { glm::vec3 lo, hi; };
    Box shellBox() const { float r = BELL_R + 0.3f; return {pos - glm::vec3{r, BELL_H * 0.5f, r}, pos + glm::vec3{r, BELL_H * 0.5f, r}}; }
    Box lampBox() const { glm::vec3 l = lamp(); return {l - glm::vec3{1.f}, l + glm::vec3{1.f}}; }
    static float rayBox(glm::vec3 o, glm::vec3 d, const Box& b) {
        float t0 = 0.f, t1 = 1e9f;
        for (int a = 0; a < 3; ++a) {
            if (std::abs(d[a]) < 1e-8f) { if (o[a] < b.lo[a] || o[a] > b.hi[a]) return -1.f; continue; }
            float i0 = (b.lo[a] - o[a]) / d[a], i1 = (b.hi[a] - o[a]) / d[a];
            if (i0 > i1) std::swap(i0, i1);
            t0 = std::max(t0, i0); t1 = std::min(t1, i1);
            if (t0 > t1) return -1.f;
        }
        return t0;
    }
    // Can a round on this line see the lamp? Up into its mouth, or it is down on its side, or a splinter
    bool lampSeen(glm::vec3 d, bool splinter) const { return splinter || stunned() || state == OVERRUNG || glm::dot(d, axis) > MOUTH; }
    // Where a round meets it: zone 0 the shell, 2 the lamp; -1 a miss
    int ray(glm::vec3 o, glm::vec3 d, float maxT, float& tOut, bool splinter = false) const {
        if (!fighting()) return -1;
        float ts = rayBox(o, d, shellBox());
        if (ts < 0.f || ts >= maxT) return -1;
        if (lampSeen(d, splinter)) {
            float tl = rayBox(o, d, lampBox());
            if (tl >= 0.f && tl < maxT) { tOut = tl; return 2; }
        }
        tOut = ts;
        return 0;
    }
    static float zoneMult(int zone) { return zone == 2 ? 3.f : 0.4f; }

    bool hurt(float dmg, int zone) {
        if (!fighting()) return false;
        if (stunned()) dmg *= 2.f;
        hp -= dmg;
        flash = 1.f;
        if (zone == 0 && state != OVERRUNG && !grounded() && hp > 0.f) {   // it rings
            resonance = std::min(1.f, resonance + dmg / RING_DMG);
            if (resonance >= 1.f) overring();
        }
        if (hp <= 0.f) {
            hp = 0.f; state = DEAD; t = 0.f; landed = false;
            vel = glm::vec3{vel.x * 0.3f, std::max(vel.y, 4.f), vel.z * 0.3f};
            events.push_back({CallerEvent::DIE, pos});
            return true;
        }
        return false;
    }
    // Punched as it dives: thrown down onto its ground, on its side
    bool parryable() const { return state == DIVE && !hitDone; }
    void parried(glm::vec3 away) {
        if (!parryable()) return;
        hitDone = true; landed = false;
        state = STUNNED; t = 0.f;
        away.y = 0.f;
        float l = glm::length(away);
        away = l > 0.01f ? away / l : glm::vec3{0.f, 0.f, 1.f};
        vel = away * 14.f + glm::vec3{0.f, 9.f, 0.f};
        events.push_back({CallerEvent::PARRIED, pos});
    }
    bool onGroundNow() const { return pos.y <= floorY + BELL_R + 0.05f; }
    // Rung all the way: it sags over its ground, reeling, and its own sky comes down on it
    void overring() {
        if (!fighting() || grounded() || state == OVERRUNG) return;
        state = OVERRUNG; t = 0.f; resonance = 1.f; hitDone = true;
        overFrom = pos;
        glm::vec2 off{pos.x - centre.x, pos.z - centre.z};
        const float lim = ORBIT_R * 0.45f;
        if (glm::length(off) > lim) off *= lim / glm::length(off);
        selfAt = {centre.x + off.x, floorY, centre.z + off.y};
        events.push_back({CallerEvent::OVERRING, pos});
        events.push_back({CallerEvent::SELF_CALL, selfAt});
    }
    float wobble() const { return state == OVERRUNG ? 1.f : 0.f; }

    void tick(float dt, glm::vec3 player, glm::vec3 eye) {
        clock += dt; t += dt;
        flash = std::max(0.f, flash - dt * 5.f);
        if (state != OVERRUNG) resonance = std::max(0.f, resonance - dt * 0.05f);   // (it dies away, slowly)
        float before = flap;
        float flapRate = state == DIVE ? 0.2f : state == CLIMB ? 1.6f : state == TOLL ? 0.5f : grounded() ? 0.f : 1.f;
        flap += dt * flapRate;
        if (std::floor(before) != std::floor(flap)) events.push_back({CallerEvent::WINGBEAT, pos});
        glm::vec3 wantAxis{0.f, 1.f, 0.f};
        auto toward = [&](glm::vec3 goal, float k) { pos += (goal - pos) * std::min(1.f, dt * k); };
        const float wspeed = enraged() ? 0.5f : 0.34f;
        switch (state) {
        case DORMANT: return;
        case ARRIVING: {
            float u = ease(t / ARRIVE_TIME);
            pos = glm::mix(arriveFrom, orbitPoint(), u) + glm::vec3{0.f, 30.f * std::sin(3.14159f * u), 0.f};
            if (t >= ARRIVE_TIME) { state = CIRCLE; t = 0.f; cooldown = 1.6f; events.push_back({CallerEvent::TOLL, pos}); events.push_back({CallerEvent::ROAR, pos}); }
            break;
        }
        case CIRCLE:
            orbitA += wspeed * dt;
            toward(orbitPoint(), 1.6f);
            cooldown -= dt;
            if (cooldown <= 0.f) {
                if (rnd() < 0.42f) { state = TOLL; swing = 0.f; }
                else { state = DIVE_AIM; dives = 0; events.push_back({CallerEvent::DIVE_TELL, pos}); }
                t = 0.f;
            }
            break;
        case TOLL:
            swing = std::sin(std::min(t / 1.6f, 1.f) * 3.14159f);
            if (t - dt < 1.0f && t >= 1.0f) {
                events.push_back({CallerEvent::TOLL, pos});
                int n = enraged() ? 3 : 1;
                for (int k = 0; k < n; ++k) {
                    glm::vec3 at = player;
                    if (k > 0) { float a = rnd() * 6.283f; at += glm::vec3{std::cos(a), 0.f, std::sin(a)} * (6.f + 6.f * rnd()); }
                    at.y = floorY;
                    events.push_back({CallerEvent::CALL, at});
                }
            }
            if (t >= 1.9f) { state = CIRCLE; t = 0.f; swing = 0.f; cooldown = (enraged() ? 1.6f : 2.6f) + rnd(); }
            break;
        case DIVE_AIM: {
            // Hold, and tip the mouth at you
            glm::vec3 d = eye - pos; float l = glm::length(d);
            if (t < aimTime() - 0.25f) { diveTo = eye - glm::vec3{0.f, 0.3f, 0.f}; }
            wantAxis = l > 0.1f ? -(diveTo - pos) / glm::length(diveTo - pos) : wantAxis;
            pos += glm::vec3{0.f, -0.5f * dt, 0.f};
            if (t >= aimTime()) {
                state = DIVE; t = 0.f; hitDone = false;
                diveFrom = pos;
                diveDir = glm::normalize(diveTo - pos);
                vel = diveDir * DIVE_SPEED;
                events.push_back({CallerEvent::DIVE, pos});
            }
            break;
        }
        case DIVE: {
            wantAxis = -diveDir;
            pos += vel * dt;
            float along = glm::dot(pos - diveTo, diveDir);
            if (pos.y < floorY + BELL_R + 1.5f) { pos.y = floorY + BELL_R + 1.5f; vel.y = std::max(vel.y, 0.f); }   // skimming its ground
            if (along > 16.f || t > 3.5f) {
                ++dives;
                if (enraged() && dives < 2) { state = DIVE_AIM; t = aimTime() * 0.35f; events.push_back({CallerEvent::DIVE_TELL, pos}); }
                else { state = CLIMB; t = 0.f; }
            }
            break;
        }
        case CLIMB:
            orbitA += wspeed * dt;
            toward(orbitPoint(), 1.3f);
            if (t >= 1.8f) { state = CIRCLE; t = 0.f; cooldown = (enraged() ? 1.4f : 2.4f) + rnd() * 1.2f; }
            break;
        case OVERRUNG: {   // down over its mark, reeling and tolling, until its meteor arrives
            float u = ease(t / 0.9f);
            pos = glm::mix(overFrom, selfAt + glm::vec3{0.f, SAG, 0.f}, u) + glm::vec3{std::sin(clock * 11.f), 0.4f * std::sin(clock * 17.f), std::cos(clock * 9.f)} * 0.6f;
            wantAxis = glm::normalize(glm::vec3{std::sin(clock * 7.f) * 0.5f, 1.f, std::cos(clock * 6.f) * 0.5f});
            swing = 0.8f;
            if (std::fmod(t, 0.6f) < dt) events.push_back({CallerEvent::TOLL, pos});
            if (t >= OVERRUN_TIME) {   // struck by it: down onto the crown, on its side
                hp -= STRIKE_DMG; flash = 1.f; resonance = 0.f; swing = 0.f;
                events.push_back({CallerEvent::STRUCK, pos});
                if (hp <= 0.f) { hp = 0.f; state = DEAD; t = 0.f; landed = false; vel = {0.f, -6.f, 0.f}; events.push_back({CallerEvent::DIE, pos}); break; }
                state = STUNNED; t = 0.f; landed = false;
                vel = glm::vec3{std::sin(clock) * 6.f, -14.f, std::cos(clock) * 6.f};
            }
            break;
        }
        case STUNNED:
        case DEAD: {
            // Falling (thrown or dead) onto its ground, onto its side
            vel.y -= 24.f * dt;
            pos += vel * dt;
            glm::vec3 side = glm::vec3{vel.x, 0.f, vel.z};
            if (glm::length(side) > 0.5f) tipDir = glm::normalize(side);
            wantAxis = glm::normalize(tipDir + glm::vec3{0.f, 0.15f, 0.f});
            if (onGroundNow()) {
                if (!landed) { landed = true; events.push_back({CallerEvent::LAND, pos}); }
                pos.y = floorY + BELL_R;
                vel = glm::vec3{0.f};
            }
            // (it lies inside its ground; it cannot slide off it)
            glm::vec2 off{pos.x - centre.x, pos.z - centre.z};
            if (glm::length(off) > ORBIT_R * 0.55f) { off *= ORBIT_R * 0.55f / glm::length(off); pos.x = centre.x + off.x; pos.z = centre.z + off.y; }
            if (state == STUNNED && t >= 3.4f) { state = CLIMB; t = 0.f; landed = false; }
            break;
        }
        }
        axis = glm::normalize(glm::mix(axis, wantAxis, std::min(1.f, dt * (state == DIVE_AIM ? 7.f : 4.f))));
    }
    glm::vec3 tipDir{1.f, 0.f, 0.f};
    bool landed = false;
};
