#pragma once
// =============================================================================
// Splinter.h — UNIT 24's trick-shot shards (Chapter 4, docs/chapter-4-design.md).
//
// The grenade button flicks a shard of cyan glass up and forward. It rises,
// hangs spinning at the top of its arc for about half a second (it GLINTS),
// then drops and breaks on whatever it touches. A round that hits a live
// splinter is sent on from it: to another splinter first (a chain, each link
// hitting harder), otherwise to the best weak point the splinter can see.
// Three charges, one regrowing at a time.
//
// No OpenGL and no GameplayState: the flight, charges, ray test and damage
// numbers live here so the tests can check them. Choosing what a redirected
// round hits needs the enemies, so that is in Gameplay_Splinter.h.
// =============================================================================
#include <glm/glm.hpp>
#include <array>
#include <cmath>

struct Splinter {
    glm::vec3 pos{0.f}, prev{0.f}, vel{0.f};
    float age = 0.f;
    float spin = 0.f;       // drawn rotation (radians)
    bool  alive = false;
};

struct SplinterSystem {
    static constexpr int   POOL = 6;
    static constexpr int   CHARGES = 3;
    static constexpr float REGROW = 2.5f;        // seconds for one charge to come back
    static constexpr float LIFE = 3.2f;          // seconds before an untouched splinter breaks anyway
    static constexpr float RADIUS = 0.55f;       // how close a round must pass (generous: it is small and spinning)
    static constexpr float ARMED_AFTER = 0.05f;  // a moment out of the hand before it can be shot
    static constexpr float GRAVITY = 16.f;
    static constexpr float HANG_SPEED = 2.6f;    // below this vertical speed it hangs (and glints)
    static constexpr float HANG_GRAVITY = 0.55f; // gravity scale while hanging: about half a second
    static constexpr float HANG_DRAG = 3.5f;     // horizontal drift dies off while it hangs
    static constexpr float THROW_FORWARD = 13.f, THROW_UP = 7.5f, CARRY = 0.6f;
    static constexpr int   MAX_LINKS = 4;        // splinter to splinter hops in one shot
    static constexpr float REACH = 70.f;         // how far a splinter looks for its next target

    std::array<Splinter, POOL> pool{};
    int   charges = CHARGES;
    float regrow = 0.f;

    void reset() { pool = {}; charges = CHARGES; regrow = 0.f; }
    bool canThrow() const { return charges > 0; }
    int  live() const { int n = 0; for (auto& s : pool) n += s.alive; return n; }
    float regrow01() const { return charges >= CHARGES ? 1.f : regrow / REGROW; }

    // Flick one out of the forearm. Returns its index, or -1 with no charge.
    // The oldest live one is broken to make room if the pool is full.
    int toss(glm::vec3 eye, glm::vec3 forward, glm::vec3 right, glm::vec3 playerVel) {
        if (!canThrow()) return -1;
        int slot = -1;
        for (int i = 0; i < POOL; ++i) if (!pool[i].alive) { slot = i; break; }
        if (slot < 0) {
            slot = 0;
            for (int i = 1; i < POOL; ++i) if (pool[i].age > pool[slot].age) slot = i;
        }
        glm::vec3 flat{forward.x, 0.f, forward.z};
        float fl = glm::length(flat);
        flat = fl > 1e-4f ? flat / fl : glm::vec3{0.f, 0.f, -1.f};
        // Mostly along the look, but always up into an arc so it hangs in front of you
        glm::vec3 dir = glm::normalize(flat * 0.8f + forward * 0.2f);
        Splinter& s = pool[slot];
        s = {};
        s.alive = true;
        s.pos = s.prev = eye + forward * 0.5f + right * 0.25f - glm::vec3{0.f, 0.25f, 0.f};
        s.vel = dir * THROW_FORWARD + glm::vec3{0.f, THROW_UP + std::max(0.f, forward.y) * 4.f, 0.f}
              + glm::vec3{playerVel.x, std::max(0.f, playerVel.y), playerVel.z} * CARRY;
        --charges;
        return slot;
    }

    static bool glinting(const Splinter& s) {
        return s.alive && s.age > ARMED_AFTER && std::abs(s.vel.y) < HANG_SPEED;
    }

    // Move them. `solid(from, to)` says whether the step from..to passes into
    // the world (a wall or the floor); one that does breaks. Returns a bit per
    // splinter that broke this tick, so the caller can play the break.
    template <class Solid>
    unsigned tick(float dt, Solid solid) {
        if (charges < CHARGES) {
            regrow += dt;
            if (regrow >= REGROW) { regrow = 0.f; ++charges; }
        } else regrow = 0.f;
        unsigned broke = 0;
        for (int i = 0; i < POOL; ++i) {
            Splinter& s = pool[i];
            if (!s.alive) continue;
            s.prev = s.pos;
            s.age += dt;
            bool hang = std::abs(s.vel.y) < HANG_SPEED;
            s.vel.y -= GRAVITY * (hang ? HANG_GRAVITY : 1.f) * dt;
            if (hang) {
                float k = std::exp(-HANG_DRAG * dt);
                s.vel.x *= k; s.vel.z *= k;
            }
            s.spin += dt * (hang ? 22.f : 9.f);
            glm::vec3 next = s.pos + s.vel * dt;
            if (s.age > LIFE || solid(s.pos, next)) { s.alive = false; broke |= 1u << i; continue; }
            s.pos = next;
        }
        return broke;
    }

    // The nearest armed splinter a ray passes within RADIUS of, before maxT.
    // `skip` is a bitmask of splinters already used by this shot.
    int rayHit(glm::vec3 origin, glm::vec3 dir, float maxT, float& tOut, unsigned skip = 0) const {
        int best = -1;
        float bestT = maxT;
        for (int i = 0; i < POOL; ++i) {
            const Splinter& s = pool[i];
            if (!s.alive || s.age < ARMED_AFTER || (skip & (1u << i))) continue;
            glm::vec3 oc = s.pos - origin;
            float along = glm::dot(oc, dir);
            if (along <= 0.f || along >= bestT) continue;
            float miss2 = glm::dot(oc, oc) - along * along;
            if (miss2 > RADIUS * RADIUS) continue;
            float t = along - std::sqrt(std::max(0.f, RADIUS * RADIUS - miss2));
            if (t < bestT) { bestT = std::max(t, 0.f); best = i; }
        }
        tOut = bestT;
        return best;
    }

    // The damage a redirected round does, as a multiple of the gun's: x1.5,
    // +0.5 for every splinter it already passed through, and x1.25 more if
    // this one was glinting when it was hit.
    static float damageMult(int link, bool glint) {
        return (1.5f + 0.5f * (float)link) * (glint ? 1.25f : 1.f);
    }

    // Heavy rounds get less of that bonus: a redirected round always lands on a
    // weak point, which on the bolt rifles was far too much. Guns at revolver
    // damage or below keep all of it; the LANCER (120) keeps ~70%, the LONGSHOT
    // (360) ~40%. `gunDamage` is the gun's base damage per round.
    static constexpr float FULL_BONUS_DAMAGE = 60.f;
    static float heavyRoundScale(float gunDamage) {
        return gunDamage <= FULL_BONUS_DAMAGE ? 1.f : std::sqrt(FULL_BONUS_DAMAGE / gunDamage);
    }
};
