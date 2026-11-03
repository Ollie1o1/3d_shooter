#pragma once
// =============================================================================
// Tender.h — THE TENDER, the first of the Gleaner's brood (docs/chapter-4-design.md
// §5, phase 2): the keeper of the furnace on its back. No OpenGL: the tests
// drive it.
//
// A squat 9 m furnace keeper, a crucible bigger than itself on its back and two
// long tongs for arms. It fights on the furnace roof (a flat deck, the Gleaner
// kneeling under it):
//
//   TONG SWEEP  tongs raised and glowing (the tell), then a low sweep through a
//               wide arc in front of it: jump it. Enraged: a second sweep back.
//   POUR        it turns to you, tips the crucible over its shoulder and pours:
//               a river of molten metal runs out ahead of it across the deck and
//               stays a while. While it tips and pours, the crucible's lip faces
//               you: its weak point (x3), and enough damage there spills it
//               (it staggers, the pour stops).
//   SLAG LOB    three globs flung at you (they can be punched back into it).
//   GRAB        close in, it opens its tongs wide, white-hot (the tell), and
//               lunges. PUNCH the tongs as they reach you: they jam, it
//               staggers. Caught: it lifts you up in front of its crucible,
//               tipped to take you, its lip right there: shoot it hard enough
//               (or the head) and it drops you, staggered. Too slow: it dunks
//               you and throws you out across the roof.
//
// A punched-back glob or a broken pour staggers it: everything hits harder (x2)
// for a moment. Below half health it is enraged: faster, the second sweep.
// =============================================================================
#include <glm/glm.hpp>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <utility>
#include <vector>

struct TenderEvent {
    enum Kind { SWEEP, LOB, POUR_START, POUR_END, STEP, ROAR, STAGGER, DIE, GRAB_TELL, GRAB, CATCH, DUNK, FREED, SPILL } kind;
    glm::vec3 at{0.f}, dir{0.f};
};

struct Tender {
    enum State { DORMANT, RISING, CHASE, SWEEP_WIND, SWEEP, POUR_TIP, POUR, LOB, RECOVER, STAGGER, GRAB_WIND, GRAB, HOLD, DEAD };
    static constexpr float MAX_HP = 2600.f;
    static constexpr float HEIGHT = 9.f, RADIUS = 3.4f;
    static constexpr float SWEEP_REACH = 14.f, SWEEP_ARC = 1.25f;   // half-angle (rad)
    static constexpr float RIVER_LEN = 44.f, RIVER_HALF = 2.8f, RIVER_STAYS = 6.f;
    static constexpr float LIP_BREAK = 380.f;   // damage to the lip in one pour that spills it
    static constexpr float GRAB_WIND_TIME = 0.85f, GRAB_TIME = 0.4f, HOLD_TIME = 2.2f;
    static constexpr float GRAB_REACH = 8.5f;    // the tongs' reach from its front
    static constexpr float FREE_DMG = 300.f;     // damage while it holds you that makes it drop you
    static constexpr float SPILL_BURN = 220.f;   // its own metal over it when its lip breaks

    State state = DORMANT;
    glm::vec3 pos{0.f};
    float yaw = 0.f;                 // forward = (sin yaw, 0, cos yaw)
    float hp = MAX_HP;
    float t = 0.f;                   // seconds in this state
    float clock = 0.f;
    float cooldown = 2.f;
    int   sweeps = 0, lobs = 0;
    float lipTaken = 0.f;
    float freeTaken = 0.f;           // damage taken while it holds you
    bool  holding = false;           // it has you (HOLD)
    bool  lastWasHold = false;
    float spilled = 0.f;             // seconds left of its own metal burning round its feet
    float walkPhase = 0.f;
    float flash = 0.f;               // 0..1 hit flash
    // The river it poured: from its front along where it faced
    glm::vec3 riverFrom{0.f}, riverDir{0.f, 0.f, 1.f};
    float riverLen = 0.f, riverLeft = 0.f;
    // Where it may walk (the deck)
    glm::vec2 lo{-50.f}, hi{50.f};
    float deckY = 0.f;
    uint32_t seed = 77u;
    std::vector<TenderEvent> events;

    float rnd() { seed = seed * 1664525u + 1013904223u; return (float)(seed >> 8) / 16777216.f; }
    glm::vec3 fwd() const { return {std::sin(yaw), 0.f, std::cos(yaw)}; }
    bool alive() const { return state != DEAD && state != DORMANT; }
    bool enraged() const { return hp < MAX_HP * 0.5f; }
    bool fighting() const { return alive() && state != RISING; }
    float health01() const { return std::max(0.f, hp / MAX_HP); }
    static float ease(float x) { x = std::clamp(x, 0.f, 1.f); return x * x * (3.f - 2.f * x); }

    // Out of the furnace's hatch
    void wake(glm::vec3 at, float facing, glm::vec2 deckLo, glm::vec2 deckHi, float y) {
        *this = Tender{};
        pos = at; yaw = facing; lo = deckLo; hi = deckHi; deckY = y;
        state = RISING; t = 0.f;
    }
    float riseY() const { return state == RISING ? -HEIGHT * 1.2f * (1.f - ease(t / 2.6f)) : 0.f; }

    // How far its crucible is tipped (0 upright .. 1 pouring over its shoulder) and its tongs raised (the sweep's tell)
    float tip() const {
        if (state == POUR_TIP) return ease(t / 0.9f);
        if (state == POUR) return 1.f;
        if (state == HOLD) return 0.6f * ease(t / 0.8f);   // tipped to take you
        if (state == RECOVER && lastWasPour) return 1.f - ease(t / 0.6f);
        if (state == RECOVER && lastWasHold) return 0.6f * (1.f - ease(t / 0.6f));
        return 0.f;
    }
    // The grab: how far its tongs are open and out (the tell), and where it holds you (your feet)
    float grabOpen() const { return state == GRAB_WIND ? ease(t / GRAB_WIND_TIME) : state == GRAB ? 1.f - ease(t / GRAB_TIME) : 0.f; }
    float grabReach() const { return state == GRAB_WIND ? ease(t / GRAB_WIND_TIME) : state == GRAB ? 1.f : state == HOLD ? 1.f : 0.f; }
    glm::vec3 holdPoint() const {
        float u = ease(t / 1.2f);
        return base() + fwd() * (6.5f - 1.6f * u) + glm::vec3{0.f, 4.6f + 3.4f * u, 0.f};
    }
    bool parryable() const { return (state == GRAB && t < GRAB_TIME) || (state == GRAB_WIND && t > GRAB_WIND_TIME - 0.18f); }
    float tongsUp() const { return state == SWEEP_WIND ? ease(t / windTime()) : 0.f; }
    float sweepU() const { return state == SWEEP ? std::clamp(t / 0.35f, 0.f, 1.f) : -1.f; }
    float windTime() const { return enraged() ? 0.65f : 0.9f; }
    bool  lipOpen() const { return state == POUR_TIP || state == POUR || state == HOLD; }
    bool  staggered() const { return state == STAGGER; }

    // Its body for the guns: a box for the body, one for the head, one for the crucible's lip
    struct Box { glm::vec3 lo, hi; };
    glm::vec3 base() const { return pos + glm::vec3{0.f, riseY(), 0.f}; }
    Box bodyBox() const { glm::vec3 b = base(); return {b + glm::vec3{-RADIUS, 0.f, -RADIUS}, b + glm::vec3{RADIUS, 6.4f, RADIUS}}; }
    Box headBox() const { glm::vec3 h = base() + fwd() * 2.2f + glm::vec3{0.f, 6.6f, 0.f}; return {h - glm::vec3{1.3f}, h + glm::vec3{1.3f}}; }
    glm::vec3 lipPoint() const {   // the crucible's lip: up behind its head, swinging over its shoulder to the front as it tips
        float k = tip();
        return base() + fwd() * glm::mix(-2.6f, 3.6f, k) + glm::vec3{0.f, glm::mix(10.4f, 8.2f, k), 0.f};
    }
    Box lipBox() const { glm::vec3 l = lipPoint(); return {l - glm::vec3{1.6f, 0.9f, 1.6f}, l + glm::vec3{1.6f, 0.9f, 1.6f}}; }

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
    // Where a round meets it first: zone 0 body, 1 head, 2 the lip (only while it faces you); -1 a miss
    int ray(glm::vec3 o, glm::vec3 d, float maxT, float& tOut) const {
        if (!alive()) return -1;
        int zone = -1; float best = maxT;
        float tb = rayBox(o, d, bodyBox()), th = rayBox(o, d, headBox()), tl = lipOpen() ? rayBox(o, d, lipBox()) : -1.f;
        if (tb >= 0.f && tb < best) { best = tb; zone = 0; }
        if (th >= 0.f && th <= best + 0.5f) { best = std::min(best, th); zone = 1; }
        if (tl >= 0.f && tl <= best + 0.8f) { best = std::min(best, tl); zone = 2; }
        tOut = best;
        return zone;
    }
    static float zoneMult(int zone) { return zone == 2 ? 3.f : zone == 1 ? 1.5f : 1.f; }

    // Damage (already multiplied for its zone). Returns true if this killed it
    bool hurt(float dmg, int zone) {
        if (!fighting()) return false;
        if (staggered()) dmg *= 2.f;
        hp -= dmg;
        flash = 1.f;
        if (state == HOLD && (zone == 2 || zone == 1)) {   // shot while it holds you: it drops you
            freeTaken += dmg;
            if (freeTaken >= FREE_DMG && hp > 0.f) { events.push_back({TenderEvent::FREED, holdPoint()}); holding = false; stagger(); }
        } else if (zone == 2 && lipOpen()) {
            lipTaken += dmg;
            if (lipTaken >= LIP_BREAK && hp > 0.f) {   // the lip breaks: its own metal spills down over it
                riverLeft = std::min(riverLeft, 2.f);
                hp -= SPILL_BURN; spilled = 2.4f;
                events.push_back({TenderEvent::SPILL, lipPoint()});
                if (hp > 0.f) stagger();
            }
        }
        if (hp <= 0.f) { hp = 0.f; holding = false; state = DEAD; t = 0.f; events.push_back({TenderEvent::DIE, base()}); return true; }
        return false;
    }
    void stagger() {
        if (!fighting()) return;
        if (state == POUR || state == POUR_TIP) events.push_back({TenderEvent::POUR_END, base()});
        if (state == HOLD && holding) { holding = false; events.push_back({TenderEvent::FREED, holdPoint()}); }
        state = STAGGER; t = 0.f;
        events.push_back({TenderEvent::STAGGER, base()});
    }
    // Punched as its tongs reach you: they jam, it staggers
    bool parried() {
        if (!parryable()) return false;
        stagger();
        return true;
    }

    bool inRiver(glm::vec3 p) const {
        if (riverLeft <= 0.f || riverLen <= 0.f || std::abs(p.y - deckY) > 1.2f) return false;
        glm::vec3 d = p - riverFrom; d.y = 0.f;
        float along = glm::dot(d, riverDir);
        if (along < 0.f || along > riverLen) return false;
        return glm::length(d - riverDir * along) < RIVER_HALF;
    }
    // In the sweep's arc, low enough to be hit (a jump clears it)
    bool inSweep(glm::vec3 p) const {
        glm::vec3 d = p - pos; float h = p.y - deckY; d.y = 0.f;
        float dist = glm::length(d);
        if (dist > SWEEP_REACH || dist < 0.5f || h > 1.3f) return false;
        float c = glm::dot(d / dist, fwd());
        return c > std::cos(SWEEP_ARC);
    }

    // (events pile up from tick() and from hurt() between ticks; whoever reads them clears them)
    void tick(float dt, glm::vec3 player) {
        clock += dt; t += dt;
        flash = std::max(0.f, flash - dt * 5.f);
        if (riverLeft > 0.f) riverLeft -= dt;
        if (spilled > 0.f) spilled -= dt;
        if (state == DEAD || state == DORMANT) return;
        glm::vec3 to = player - pos; to.y = 0.f;
        float dist = glm::length(to);
        float want = std::atan2(to.x, to.z);
        auto turnTo = [&](float rate) {
            float d = std::fmod(want - yaw + 9.42477f, 6.28319f) - 3.14159f;
            yaw += std::clamp(d, -rate * dt, rate * dt);
        };
        const float pace = enraged() ? 1.35f : 1.f;
        switch (state) {
        case RISING:
            if (t > 2.6f) { state = CHASE; t = 0.f; cooldown = 1.2f; events.push_back({TenderEvent::ROAR, base()}); }
            break;
        case CHASE: {
            turnTo(1.7f * pace);
            if (dist > 9.f) {
                float before = walkPhase;
                glm::vec3 step = fwd() * (4.6f * pace * dt);
                pos.x = std::clamp(pos.x + step.x, lo.x + RADIUS, hi.x - RADIUS);
                pos.z = std::clamp(pos.z + step.z, lo.y + RADIUS, hi.y - RADIUS);
                walkPhase += dt * 1.6f * pace;
                if (std::floor(before * 2.f) != std::floor(walkPhase * 2.f)) events.push_back({TenderEvent::STEP, base()});
            }
            cooldown -= dt;
            if (cooldown <= 0.f) {
                float r = rnd();
                if (dist < 12.f && r < 0.42f) { state = GRAB_WIND; events.push_back({TenderEvent::GRAB_TELL, base()}); }
                else if (dist < 12.f) { state = SWEEP_WIND; sweeps = 0; }
                else if (dist < 40.f && r < 0.6f) state = POUR_TIP;
                else if (dist < 40.f || r < 0.45f) { state = LOB; lobs = 0; }
                else { cooldown = 1.2f; break; }   // far off: it keeps coming
                t = 0.f;
                if (state == POUR_TIP) { lipTaken = 0.f; }
            }
            break;
        }
        case SWEEP_WIND:
            turnTo(2.4f * pace);
            if (t >= windTime()) { state = SWEEP; t = 0.f; events.push_back({TenderEvent::SWEEP, base(), fwd()}); }
            break;
        case SWEEP:
            if (t >= 0.35f) {
                ++sweeps;
                if (enraged() && sweeps < 2) { state = SWEEP_WIND; t = windTime() * 0.4f; }   // and back again, quicker
                else { state = RECOVER; t = 0.f; lastWasPour = false; }
            }
            break;
        case POUR_TIP:
            if (t < 0.4f) turnTo(3.f);   // it aims, then commits
            if (t >= 0.9f) {
                state = POUR; t = 0.f;
                riverFrom = pos + fwd() * 3.5f; riverFrom.y = deckY; riverDir = fwd(); riverLen = 0.f; riverLeft = 3.2f + RIVER_STAYS;
                events.push_back({TenderEvent::POUR_START, riverFrom, riverDir});
            }
            break;
        case POUR:
            riverLen = std::min(RIVER_LEN, riverLen + dt * 22.f);
            if (t >= 3.2f) { state = RECOVER; t = 0.f; lastWasPour = true; events.push_back({TenderEvent::POUR_END, base()}); }
            break;
        case LOB:
            turnTo(2.f);
            if (lobs < 3 && t >= 0.45f + lobs * 0.4f) {
                ++lobs;
                events.push_back({TenderEvent::LOB, base() + glm::vec3{0.f, 9.f, 0.f} - fwd() * 1.5f, fwd()});
            }
            if (t >= 1.9f) { state = RECOVER; t = 0.f; lastWasPour = false; }
            break;
        case GRAB_WIND:
            turnTo(2.6f * pace);
            if (t >= GRAB_WIND_TIME) { state = GRAB; t = 0.f; events.push_back({TenderEvent::GRAB, base(), fwd()}); }
            break;
        case GRAB: {   // the lunge: it comes forward, the tongs snap shut on whatever is in front of it
            glm::vec3 step = fwd() * (9.f * dt);
            pos.x = std::clamp(pos.x + step.x, lo.x + RADIUS, hi.x - RADIUS);
            pos.z = std::clamp(pos.z + step.z, lo.y + RADIUS, hi.y - RADIUS);
            glm::vec3 d = player - pos; float h = player.y - deckY; d.y = 0.f;
            float l = glm::length(d);
            if (t > GRAB_TIME * 0.6f && l < GRAB_REACH + RADIUS && l > 0.5f && glm::dot(d / l, fwd()) > 0.55f && h < 2.5f) {
                state = HOLD; t = 0.f; holding = true; freeTaken = 0.f; lipTaken = 0.f;
                events.push_back({TenderEvent::CATCH, player});
                break;
            }
            if (t >= GRAB_TIME) { state = RECOVER; t = 0.f; lastWasPour = false; lastWasHold = false; }
            break;
        }
        case HOLD:
            if (t >= HOLD_TIME) {   // into the crucible with you, and out across the roof
                holding = false;
                events.push_back({TenderEvent::DUNK, holdPoint(), fwd()});
                state = RECOVER; t = 0.f; lastWasPour = false; lastWasHold = true;
            }
            break;
        case RECOVER:
            if (t >= 0.9f / pace) { state = CHASE; t = 0.f; cooldown = (enraged() ? 0.8f : 1.5f) + rnd() * 0.8f; lastWasHold = false; }
            break;
        case STAGGER:
            if (t >= 2.2f) { state = CHASE; t = 0.f; cooldown = 1.f; }
            break;
        default: break;
        }
    }
    bool lastWasPour = false;
};
