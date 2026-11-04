#pragma once
// =============================================================================
// Organs.h — inside THE GLEANER (docs/chapter-4-design.md §5, phase 3): the
// three organs of its foundry, one room each, climbing up through it. No
// OpenGL: the tests drive them.
//
//   THE CRUCIBLE HEART  hangs over the floor and beats. Every few beats it
//                       swells (the floor glows: the tell) and floods the floor
//                       with slag; stand on the islands. While it floods, its
//                       three valves are open: its weak points (x3). It spits
//                       slag at you between floods (punch it back).
//   THE BELLOWS LUNGS   two great bellows on the far wall breathe in turn. In:
//                       the throat opens (its weak point, x3). Hold: a hiss.
//                       Out: a gust down the room that throws you toward the
//                       burning grate at its end, unless a pillar shelters you.
//   THE SORTING CORE    a drum of hooks turning over the floor, sorting bodies.
//                       It throws them at you, three at a time (punch them back
//                       into it); then it opens its lens (x3) while it sorts.
//
// Damage comes in already multiplied for its zone; each returns true when it dies.
// =============================================================================
#include <glm/glm.hpp>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <utility>
#include <vector>

namespace organ {
struct Box { glm::vec3 lo, hi; };
inline float rayBox(glm::vec3 o, glm::vec3 d, const Box& b) {
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
inline Box around(glm::vec3 c, glm::vec3 h) { return {c - h, c + h}; }
inline float ease(float x) { x = std::clamp(x, 0.f, 1.f); return x * x * (3.f - 2.f * x); }
struct Event { enum Kind { BEAT, SWELL, FLOOD, DRAIN, SPIT, INHALE, HISS, GUST, THROW, OPEN, CLOSE, ROAR, DIE } kind; glm::vec3 at{0.f}; int who = 0; };
}

// ---------------------------------------------------------------------------
struct CrucibleHeart {
    enum State { DORMANT, BEATING, SWELL, FLOOD, DEAD };
    static constexpr float MAX_HP = 2400.f, R = 5.5f;
    static constexpr float SWELL_T = 2.f, FLOOD_T = 4.6f, RISE = 0.8f, FLOOD_H = 1.2f;
    State state = DORMANT;
    glm::vec3 pos{0.f};          // its middle (it hangs)
    float floorY = 0.f;
    float hp = MAX_HP, t = 0.f, clock = 0.f, flash = 0.f, beatT = 0.f, restFor = 5.f, spitCd = 3.f;
    int floods = 0;
    uint32_t seed = 41u;
    std::vector<organ::Event> events;
    float rnd() { seed = seed * 1664525u + 1013904223u; return (float)(seed >> 8) / 16777216.f; }
    bool alive() const { return state != DORMANT && state != DEAD; }
    bool enraged() const { return hp < MAX_HP * 0.5f; }
    float health01() const { return std::max(0.f, hp / MAX_HP); }
    void wake(glm::vec3 at, float floor) { *this = CrucibleHeart{}; pos = at; floorY = floor; state = BEATING; restFor = 4.f; }
    float beatRate() const { return state == SWELL ? 2.6f : enraged() ? 1.6f : 1.15f; }   // beats a second
    float pulse() const { float u = std::fmod(beatT, 1.f); return std::exp(-u * 7.f); }
    float swell() const { return state == SWELL ? organ::ease(t / SWELL_T) : state == FLOOD ? 1.f - organ::ease((t - FLOOD_T + RISE) / RISE) : 0.f; }
    float level() const {   // the slag over the floor: up, held, drained
        if (state != FLOOD) return 0.f;
        return FLOOD_H * organ::ease(t / RISE) * (1.f - organ::ease((t - FLOOD_T + RISE) / RISE));
    }
    bool valvesOpen() const { return state == FLOOD; }
    glm::vec3 valve(int k) const { float a = k * 2.0944f + clock * 0.2f; return pos + glm::vec3{std::cos(a) * (R + 0.9f), -1.5f, std::sin(a) * (R + 0.9f)}; }
    bool inSlag(glm::vec3 feet) const { return level() > 0.15f && feet.y < floorY + level() + 0.05f; }
    // Zone 0 its armoured body, 2 a valve (only while it floods); -1 a miss
    int ray(glm::vec3 o, glm::vec3 d, float maxT, float& tOut, bool splinter = false) const {
        if (!alive()) return -1;
        float best = maxT; int zone = -1;
        if (valvesOpen() || splinter)
            for (int k = 0; k < 3; ++k) { float tv = organ::rayBox(o, d, organ::around(valve(k), glm::vec3{1.3f})); if (tv >= 0.f && tv < best) { best = tv; zone = 2; } }
        float tb = organ::rayBox(o, d, organ::around(pos, glm::vec3{R * 0.78f, R * 0.9f, R * 0.78f}));   // (round: its box inside its outline, so the valves stand clear of it)
        if (tb >= 0.f && tb < best - 0.6f) { best = tb; zone = 0; }
        tOut = best; return zone;
    }
    static float zoneMult(int z) { return z == 2 ? 3.f : 0.3f; }
    bool hurt(float dmg) {
        if (!alive()) return false;
        hp -= dmg; flash = 1.f;
        if (hp <= 0.f) { hp = 0.f; state = DEAD; t = 0.f; events.push_back({organ::Event::DIE, pos}); return true; }
        return false;
    }
    void tick(float dt) {
        clock += dt; t += dt; flash = std::max(0.f, flash - dt * 5.f);
        if (!alive()) return;
        float was = beatT; beatT += dt * beatRate();
        if (std::floor(was) != std::floor(beatT)) events.push_back({organ::Event::BEAT, pos});
        switch (state) {
        case BEATING:
            spitCd -= dt;
            if (spitCd <= 0.f) { events.push_back({organ::Event::SPIT, pos - glm::vec3{0.f, R * 0.6f, 0.f}}); spitCd = (enraged() ? 1.6f : 2.6f) + rnd(); }
            if (t >= restFor) { state = SWELL; t = 0.f; events.push_back({organ::Event::SWELL, pos}); }
            break;
        case SWELL:
            if (t >= SWELL_T) { state = FLOOD; t = 0.f; ++floods; events.push_back({organ::Event::FLOOD, pos}); }
            break;
        case FLOOD:
            if (t >= FLOOD_T) { state = BEATING; t = 0.f; restFor = (enraged() ? 3.5f : 5.f) + rnd() * 1.5f; spitCd = 1.f; events.push_back({organ::Event::DRAIN, pos}); }
            break;
        default: break;
        }
    }
};

// ---------------------------------------------------------------------------
struct BellowsLungs {
    enum Phase { IN, HOLD, OUT, REST };
    static constexpr float MAX_HP = 1500.f;   // each
    static constexpr float IN_T = 2.4f, HOLD_T = 0.8f, OUT_T = 1.6f, REST_T = 1.4f;
    static constexpr float GUST = 34.f;       // m/s^2 at the gust's heart
    struct Bellows { glm::vec3 pos{0.f}; float hp = MAX_HP, t = 0.f, flash = 0.f; Phase ph = REST; bool dead = false; };
    Bellows b[2];
    glm::vec3 blow{0.f, 0.f, 1.f};            // down the room
    float floorY = 0.f, clock = 0.f;
    bool woke = false;
    std::vector<organ::Event> events;
    static float len(Phase p) { return p == IN ? IN_T : p == HOLD ? HOLD_T : p == OUT ? OUT_T : REST_T; }
    void wake(glm::vec3 left, glm::vec3 right, glm::vec3 dir, float floor) {
        *this = BellowsLungs{};
        b[0].pos = left; b[1].pos = right; blow = dir; floorY = floor; woke = true;
        b[0].ph = IN; b[1].ph = OUT; b[1].t = 0.5f;   // (they breathe in turn)
    }
    bool alive() const { return woke && (!b[0].dead || !b[1].dead); }
    bool fighting() const { return alive(); }
    float health01() const { return std::max(0.f, (b[0].hp + b[1].hp) / (2.f * MAX_HP)); }
    // How full it is (0 empty .. 1 drawn in), for the rig
    float fill(int k) const {
        const Bellows& w = b[k];
        if (w.dead) return 0.2f;
        return w.ph == IN ? organ::ease(w.t / IN_T) : w.ph == HOLD ? 1.f : w.ph == OUT ? 1.f - organ::ease(w.t / (OUT_T * 0.6f)) : 0.f;
    }
    bool throatOpen(int k) const { return !b[k].dead && (b[k].ph == IN || b[k].ph == HOLD); }
    bool gusting(int k) const { return !b[k].dead && b[k].ph == OUT && b[k].t < OUT_T * 0.85f; }
    glm::vec3 throat(int k) const { return b[k].pos + blow * 4.2f + glm::vec3{0.f, -2.f, 0.f}; }
    int ray(glm::vec3 o, glm::vec3 d, float maxT, float& tOut, int& who, bool splinter = false) const {
        float best = maxT; int zone = -1;
        for (int k = 0; k < 2; ++k) {
            if (b[k].dead) continue;
            if (throatOpen(k) || splinter) { float tt = organ::rayBox(o, d, organ::around(throat(k), glm::vec3{1.4f})); if (tt >= 0.f && tt < best) { best = tt; zone = 2; who = k; } }
            float tb = organ::rayBox(o, d, organ::around(b[k].pos, glm::vec3{4.5f, 5.f, 3.5f}));
            if (tb >= 0.f && tb < best - 0.5f) { best = tb; zone = 0; who = k; }
        }
        tOut = best; return zone;
    }
    static float zoneMult(int z) { return z == 2 ? 3.f : 0.25f; }
    bool hurt(int k, float dmg) {
        Bellows& w = b[k];
        if (w.dead || !woke) return false;
        w.hp -= dmg; w.flash = 1.f;
        if (w.hp <= 0.f) { w.hp = 0.f; w.dead = true; events.push_back({organ::Event::DIE, w.pos, k}); return true; }
        return false;
    }
    void tick(float dt) {
        clock += dt;
        for (int k = 0; k < 2; ++k) {
            Bellows& w = b[k];
            w.flash = std::max(0.f, w.flash - dt * 5.f);
            if (w.dead || !woke) continue;
            const bool alone = b[1 - k].dead;   // the other dead: it breathes faster
            w.t += dt * (alone ? 1.3f : 1.f);
            if (w.t >= len(w.ph)) {
                w.t = 0.f;
                w.ph = (Phase)((w.ph + 1) % 4);
                events.push_back({w.ph == IN ? organ::Event::INHALE : w.ph == HOLD ? organ::Event::HISS : w.ph == OUT ? organ::Event::GUST : organ::Event::CLOSE, w.pos, k});
            }
        }
    }
    // The push a gust gives at p (the caller decides whether a pillar shelters you)
    glm::vec3 gustAt(int k, glm::vec3 p) const {
        if (!gusting(k)) return glm::vec3{0.f};
        glm::vec3 d = p - b[k].pos; d.y = 0.f;
        float along = glm::dot(d, blow);
        if (along < 0.f) return glm::vec3{0.f};
        glm::vec3 side = d - blow * along;
        float spread = 6.f + along * 0.35f;
        float k2 = std::exp(-glm::dot(side, side) / (spread * spread)) * (1.f - organ::ease(b[k].t / (OUT_T * 0.85f)) * 0.5f);
        return blow * (GUST * k2);
    }
};

// ---------------------------------------------------------------------------
struct SortingCore {
    enum State { DORMANT, SORT, THROW, OPEN, DEAD };
    static constexpr float MAX_HP = 3000.f, R = 6.f;
    static constexpr float SORT_T = 3.6f, OPEN_T = 2.8f;
    State state = DORMANT;
    glm::vec3 pos{0.f};
    float hp = MAX_HP, t = 0.f, clock = 0.f, flash = 0.f, spin = 0.f;
    int thrown = 0;
    uint32_t seed = 59u;
    std::vector<organ::Event> events;
    float rnd() { seed = seed * 1664525u + 1013904223u; return (float)(seed >> 8) / 16777216.f; }
    bool alive() const { return state != DORMANT && state != DEAD; }
    bool enraged() const { return hp < MAX_HP * 0.5f; }
    float health01() const { return std::max(0.f, hp / MAX_HP); }
    void wake(glm::vec3 at) { *this = SortingCore{}; pos = at; state = SORT; t = 1.5f; }
    bool lensOpen() const { return state == OPEN; }
    float lensU() const { return state == OPEN ? organ::ease(t / 0.4f) * (1.f - organ::ease((t - OPEN_T + 0.4f) / 0.4f)) : 0.f; }
    glm::vec3 lens() const { return pos + glm::vec3{0.f, -R * 0.85f, 0.f}; }   // (under the drum: you look up into it)
    void open() { if (alive()) { state = OPEN; t = 0.f; events.push_back({organ::Event::OPEN, lens()}); } }
    int ray(glm::vec3 o, glm::vec3 d, float maxT, float& tOut, bool splinter = false) const {
        if (!alive()) return -1;
        float best = maxT; int zone = -1;
        if (lensOpen() || splinter) { float tl = organ::rayBox(o, d, organ::around(lens(), glm::vec3{1.8f})); if (tl >= 0.f && tl < best) { best = tl; zone = 2; } }
        float tb = organ::rayBox(o, d, organ::around(pos, glm::vec3{R, R * 0.7f, R}));
        if (tb >= 0.f && tb < best - 0.8f) { best = tb; zone = 0; }
        tOut = best; return zone;
    }
    static float zoneMult(int z) { return z == 2 ? 3.f : 0.25f; }
    bool hurt(float dmg) {
        if (!alive()) return false;
        if (lensOpen()) dmg *= 1.f;
        hp -= dmg; flash = 1.f;
        if (hp <= 0.f) { hp = 0.f; state = DEAD; t = 0.f; events.push_back({organ::Event::DIE, pos}); return true; }
        return false;
    }
    void tick(float dt) {
        clock += dt; t += dt; flash = std::max(0.f, flash - dt * 5.f);
        spin += dt * (state == SORT ? 0.9f : state == THROW ? 1.8f : state == OPEN ? 0.15f : 0.f);
        if (!alive()) return;
        switch (state) {
        case SORT:
            if (t >= (enraged() ? SORT_T * 0.7f : SORT_T)) { state = THROW; t = 0.f; thrown = 0; events.push_back({organ::Event::ROAR, pos}); }
            break;
        case THROW: {
            int n = enraged() ? 4 : 3;
            if (thrown < n && t >= 0.5f + thrown * 0.55f) {
                float a = spin * 2.f + thrown * 1.7f;
                events.push_back({organ::Event::THROW, pos + glm::vec3{std::cos(a) * R, -1.f, std::sin(a) * R}});
                ++thrown;
            }
            if (t >= 0.9f + n * 0.55f) open();
            break;
        }
        case OPEN:
            if (t >= OPEN_T) { state = SORT; t = 0.f; events.push_back({organ::Event::CLOSE, pos}); }
            break;
        default: break;
        }
    }
};
