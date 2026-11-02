#pragma once
// =============================================================================
// PlainMeteors.h — the meteors that never stop falling on THE BLOOD MOON PLAIN
// (Chapter 4). They are the rejected, thrown away from somewhere above.
//
// Ambient for now: each comes in from high up on a long slanted line and hits
// the ground somewhere between MIN_NEAR and MAX_FAR from the player, never
// on them. Each impact is reported once (where, how big) so the game can make
// the flash, the sound (late, at the speed of sound), the shake and the fire
// it leaves. The burning remains are kept here too, so they can be drawn and
// smoke while they cool. Phase 1's called meteors (aimed, marked) build on this.
//
// No OpenGL; its own random numbers, so a run (and a test) is repeatable.
// =============================================================================
#include <glm/glm.hpp>
#include <array>
#include <cmath>
#include <cstdint>
#include <vector>

struct Meteor {
    glm::vec3 pos{0.f}, prev{0.f}, vel{0.f};
    float size = 4.f;     // metres across the burning rock
    float age = 0.f;
    float spin = 0.f;
    bool  alive = false;
    bool  called = false;          // aimed at the player (the Gleaner called it down): marked, and it hurts
    glm::vec3 target{0.f};
};

struct MeteorImpact { glm::vec3 pos; float size; bool called = false; };

struct MeteorRemains {
    glm::vec3 pos{0.f};
    float size = 4.f;
    float age = 0.f;
    bool  alive = false;
};

struct MeteorRain {
    static constexpr int   POOL = 24;
    static constexpr int   REMAINS = 16;
    static constexpr float MIN_NEAR = 110.f;     // never closer than this to the player (ambient)
    static constexpr float MAX_FAR = 2300.f;
    static constexpr float GAP_MIN = 0.7f, GAP_MAX = 2.4f;   // seconds between falls
    static constexpr float COOL = 22.f;          // seconds a crater burns
    static constexpr float SPEED_MIN = 150.f, SPEED_MAX = 230.f;
    static constexpr float HEIGHT_MIN = 900.f, HEIGHT_MAX = 1500.f;

    std::array<Meteor, POOL> pool{};
    std::array<MeteorRemains, REMAINS> remains{};
    float next = 0.5f;
    uint32_t seed = 2024u;
    // The way they come in: from the north-east, out of the moon's side of the sky
    glm::vec3 drift{-0.35f, 0.f, 0.55f};

    float rnd() { seed = seed * 1664525u + 1013904223u; return (float)(seed >> 8) / 16777216.f; }
    float rnd(float a, float b) { return a + (b - a) * rnd(); }

    void reset(uint32_t s = 2024u) { pool = {}; remains = {}; seed = s; next = 0.5f; }
    int live() const { int n = 0; for (auto& m : pool) n += m.alive; return n; }

    // A new one, aimed to land at `target` (the caller has checked it is allowed)
    int launch(glm::vec3 target, float size) {
        int slot = -1;
        for (int i = 0; i < POOL; ++i) if (!pool[i].alive) { slot = i; break; }
        if (slot < 0) return -1;
        glm::vec3 slant = glm::normalize(glm::vec3{drift.x + rnd(-0.2f, 0.2f), -1.f, drift.z + rnd(-0.2f, 0.2f)});
        float height = rnd(HEIGHT_MIN, HEIGHT_MAX);
        float travel = height / -slant.y;
        Meteor& m = pool[slot];
        m = {};
        m.alive = true;
        m.size = size;
        m.vel = slant * rnd(SPEED_MIN, SPEED_MAX);
        m.pos = m.prev = target - slant * travel;
        m.spin = rnd(0.f, 6.283f);
        return slot;
    }

    // One the Gleaner calls down on the player: steep and quick (about CALLED_FALL seconds), aimed at
    // `target`, which is marked on the ground while it comes
    static constexpr float CALLED_FALL = 2.4f;
    int launchCalled(glm::vec3 target, float size, float groundY = 0.f) {   // (groundY: where it lands, if not the plain: the Gleaner's back)
        int slot = -1;
        for (int i = 0; i < POOL; ++i) if (!pool[i].alive) { slot = i; break; }
        if (slot < 0) return -1;
        glm::vec3 slant = glm::normalize(glm::vec3{drift.x * 0.45f + rnd(-0.1f, 0.1f), -1.f, drift.z * 0.45f + rnd(-0.1f, 0.1f)});
        float speed = 210.f;
        Meteor& m = pool[slot];
        m = {};
        m.alive = true; m.called = true; m.size = size; m.target = target; m.target.y = groundY;
        m.vel = slant * speed;
        m.pos = m.prev = m.target - m.vel * CALLED_FALL;
        m.spin = rnd(0.f, 6.283f);
        return slot;
    }
    // 0..1: how close a called one is to landing (its marker tightens)
    static float calledIn(const Meteor& m) { return m.called && m.vel.y < 0.f ? std::clamp(1.f - ((m.pos.y - m.target.y) / -m.vel.y) / CALLED_FALL, 0.f, 1.f) : 0.f; }

    // Somewhere on the ground the player can see but is not standing on
    glm::vec3 pickTarget(glm::vec3 player) {
        // Most fall far off; now and then one comes down near enough to feel
        float u = rnd();
        float d = u < 0.12f ? rnd(MIN_NEAR, 350.f) : u < 0.5f ? rnd(350.f, 1000.f) : rnd(1000.f, MAX_FAR);
        float a = rnd(0.f, 6.283f);
        return {player.x + std::cos(a) * d, 0.f, player.z + std::sin(a) * d};
    }

    // Move them; fills `hits` with the ones that reached the ground this tick
    void tick(float dt, glm::vec3 player, std::vector<MeteorImpact>& hits) {
        hits.clear();
        next -= dt;
        if (next <= 0.f) {
            next = rnd(GAP_MIN, GAP_MAX);
            glm::vec3 t = pickTarget(player);
            float big = rnd();
            launch(t, big > 0.9f ? rnd(9.f, 14.f) : rnd(3.f, 7.f));
        }
        for (auto& m : pool) {
            if (!m.alive) continue;
            // While it is still high, lean it away from where the player has got to:
            // ambient meteors never land on you, however far you have run since
            if (!m.called && m.pos.y > 200.f && m.vel.y < 0.f) {
                float tLand = m.pos.y / -m.vel.y;
                glm::vec3 land = m.pos + m.vel * tLand;
                glm::vec2 away{land.x - player.x, land.z - player.z};
                float d = glm::length(away);
                if (d < MIN_NEAR) {
                    glm::vec2 dir = d > 1.f ? away / d : glm::vec2{1.f, 0.f};
                    glm::vec2 push = dir * (MIN_NEAR - d + 5.f) / tLand;
                    float most = 25.f * dt;                       // a gentle lean, unseen that high up
                    float len = glm::length(push);
                    if (len > most) push *= most / len;
                    m.vel.x += push.x; m.vel.z += push.y;
                }
            }
            m.prev = m.pos;
            m.age += dt;
            m.spin += dt * 1.7f;
            m.pos += m.vel * dt;
            const float ground = m.called ? m.target.y : 0.f;
            if (m.pos.y <= ground) {
                // Back along its line to the ground exactly
                float back = m.vel.y < 0.f ? (m.pos.y - ground) / m.vel.y : 0.f;
                glm::vec3 at = m.pos - m.vel * back;
                at.y = ground;
                hits.push_back({at, m.size, m.called});
                m.alive = false;
                addRemains(at, m.size);
            }
        }
        for (auto& r : remains) {
            if (!r.alive) continue;
            r.age += dt;
            if (r.age > COOL) r.alive = false;
        }
    }

    void addRemains(glm::vec3 at, float size) {
        int slot = 0;
        for (int i = 0; i < REMAINS; ++i) {
            if (!remains[i].alive) { slot = i; break; }
            if (remains[i].age > remains[slot].age) slot = i;
        }
        remains[slot] = {at, size, 0.f, true};
    }

    // 1 just landed .. 0 cold
    static float heat(const MeteorRemains& r) { return r.alive ? 1.f - r.age / COOL : 0.f; }
};
