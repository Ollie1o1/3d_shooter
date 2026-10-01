#pragma once
// =============================================================================
// Enemy.h — the enemy roster: per-type stats and AI. No OpenGL in here, so the
// AI runs in the headless tests (tests/test_game.cpp). Models are built from
// the same state in EnemyModel.h.
//
// ROSTER (each one teaches the player a different answer):
//   HUSK     — humanoid rifleman. Keeps mid range, fires slow parryable orbs.
//   RIPPER   — low four-legged hound. Zig-zags in, crouches, lunges. Dash away.
//   SENTINEL — tall sniper. Paints you with a laser, then fires a fast burst.
//              Break line of sight while the laser is up.
//   RAPTOR   — bird. Circles overhead shooting, then dives at you.
//   BRUTE    — heavy. Walks you down and slams the ground; jump the shockwave.
//   MITE     — small spider bomb. Rushes and detonates; shoot it early and the
//              blast hurts its friends instead.
//   WARDEN   — the final boss: volleys, slams, and summons adds; enrages at 50%.
//
// An enemy reports what it did this tick through `ev` (shots fired, melee hit,
// slam, detonation, summons). GameplayState turns those into projectiles,
// damage, particles and sound, so the AI stays free of rendering and audio.
// =============================================================================
#include <glm/glm.hpp>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <algorithm>
#include "Player.h"  // AABB, Wall, SpatialGrid

enum class EnemyType { HUSK, RIPPER, SENTINEL, RAPTOR, BRUTE, MITE, WARDEN, COUNT };
enum class EnemyState { SPAWNING, ACTIVE, DEAD };
enum class AttackKind { NONE, SHOT, BURST, LUNGE, DIVE, SLAM, LOB, FUSE, VOLLEY, SUMMON };

struct EnemyStats {
    const char* name;
    float     health;
    float     radius, height;   // hitbox: feet at position, centred in XZ
    float     speed;            // m/s
    float     telegraph;        // default wind-up before an attack lands
    float     attackEvery;      // seconds between attacks
    bool      flying;
    glm::vec3 color;            // armour colour (also hit/death particle tint)
    glm::vec3 glow;             // eyes / core
    glm::vec3 shotColor;
    const char* hint;           // shown the first time the type appears
};

inline const EnemyStats& statsOf(EnemyType t) {
    static const EnemyStats S[] = {
        {"HUSK",     60.f, 0.45f, 1.95f, 3.6f, 0.50f, 2.4f, false,
         {0.70f,0.26f,0.18f}, {1.0f,0.78f,0.20f}, {1.0f,0.50f,0.12f},
         "HUSKS FIRE SLOW ORBS - PRESS F AS ONE ARRIVES TO PARRY IT"},
        {"RIPPER",   40.f, 0.55f, 1.10f, 7.6f, 0.34f, 1.1f, false,
         {0.80f,0.64f,0.14f}, {1.0f,0.95f,0.25f}, {1.0f,0.9f,0.3f},
         "RIPPERS CROUCH BEFORE THEY LUNGE - DASH OUT OF THE WAY"},
        {"SENTINEL", 55.f, 0.45f, 2.60f, 3.0f, 0.85f, 3.4f, false,
         {0.20f,0.32f,0.62f}, {0.25f,0.95f,1.0f}, {0.35f,0.9f,1.0f},
         "SENTINELS SNIPE - BREAK THEIR LASER BEFORE THEY FIRE"},
        {"RAPTOR",   40.f, 1.00f, 0.90f, 7.5f, 0.42f, 1.9f, true,
         {0.40f,0.16f,0.52f}, {1.0f,0.30f,0.85f}, {0.95f,0.35f,1.0f},
         "RAPTORS CIRCLE AND DIVE - WATCH THE SKY"},
        {"BRUTE",   280.f, 0.95f, 2.90f, 2.8f, 0.95f, 2.6f, false,
         {0.58f,0.24f,0.17f}, {1.0f,0.48f,0.06f}, {1.0f,0.50f,0.10f},
         "BRUTES SLAM THE GROUND - JUMP OVER THE SHOCKWAVE"},
        {"MITE",     14.f, 0.38f, 0.60f, 7.2f, 0.55f, 0.0f, false,
         {0.18f,0.30f,0.16f}, {0.40f,1.0f,0.30f}, {0.40f,1.0f,0.30f},
         "MITES EXPLODE - SHOOT THEM EARLY AND THE BLAST HITS THEIR FRIENDS"},
        {"WARDEN", 2000.f, 1.60f, 4.60f, 2.4f, 0.85f, 3.0f, false,
         {0.34f,0.27f,0.40f}, {1.0f,0.16f,0.62f}, {1.0f,0.22f,0.68f},
         "THE WARDEN"},
    };
    return S[(int)t];
}

// Everything an enemy did this tick. Cleared at the top of every update().
struct EnemyEvents {
    static constexpr int MAX_SHOTS = 16;
    bool      telegraphStarted = false;
    int       shots = 0;
    glm::vec3 shotOrigin{0.f};
    glm::vec3 shotDir[MAX_SHOTS];
    float     shotSpeed = 16.f, shotDamage = 10.f, shotSize = 1.f;
    bool      meleeHit = false;   float meleeDamage = 0.f;
    bool      slam = false;       float slamRadius = 0.f, slamDamage = 0.f;
    bool      detonated = false;  // MITE blew itself up next to the player
    int       summonMites = 0, summonRippers = 0;
    bool      enraged = false;    // WARDEN crossed 50% this tick
};

// What an enemy can sense each tick.
struct EnemyWorld {
    glm::vec3 playerEye{0.f};
    glm::vec3 playerFeet{0.f};
    const Wall* walls = nullptr;
    int  wallCount = 0;
    const SpatialGrid* grid = nullptr;
    AABB bounds{{-1e9f,-1e9f,-1e9f},{1e9f,1e9f,1e9f}}; // arena interior; enemies stay inside
};

// Ray vs AABB: distance along the ray to the first hit, or -1 on a miss.
inline float rayBoxHit(glm::vec3 o, glm::vec3 d, const AABB& b) {
    glm::vec3 invD{1.f/(d.x+1e-9f), 1.f/(d.y+1e-9f), 1.f/(d.z+1e-9f)};
    glm::vec3 t0 = (b.min-o)*invD, t1 = (b.max-o)*invD;
    glm::vec3 tMin = glm::min(t0,t1), tMax = glm::max(t0,t1);
    float tEnter = std::max({tMin.x,tMin.y,tMin.z});
    float tExit  = std::min({tMax.x,tMax.y,tMax.z});
    if (tEnter > tExit || tExit < 0.f) return -1.f;
    return tEnter > 0.f ? tEnter : tExit;
}

inline float frand(float lo, float hi) { return lo + (hi - lo) * (float)(rand() % 10001) / 10000.f; }

struct Enemy {
    EnemyType  type;
    EnemyState state = EnemyState::SPAWNING;
    glm::vec3  position;
    glm::vec3  velocity{0.f};
    float      yaw   = 0.f;   // radians; model faces +Z at yaw 0
    float      pitch = 0.f;   // RAPTOR only: nose-down while diving
    float      health, maxHealth;
    bool       alive = true;

    static constexpr float SPAWN_TIME = 0.9f;
    float spawnTimer = SPAWN_TIME;   // counts down; untargetable while > 0

    // Attack cycle
    AttackKind attack = AttackKind::NONE;   // the attack currently winding up / running
    float telegraphTimer = 0.f, telegraphDuration = 0.f;
    float attackTimer    = 0.f;
    int   attackCount    = 0;
    float recoverTimer   = 0.f;   // after a lunge/dive: back off before re-engaging
    int   burstLeft      = 0;
    float burstTimer     = 0.f;
    float diveTimer      = 0.f;
    glm::vec3 diveDir{0.f};

    // Movement
    float strafeTimer = 0.f, strafeDir = 1.f;
    float avoidSign   = 1.f, avoidTimer = 0.f;
    float noLosTimer  = 0.f;      // > 0: reposition to find a clear shot
    float hoverY      = 8.f;
    float orbitRadius = 12.f;

    // Animation / feedback
    float animPhase     = 0.f;   // walk / flap cycle
    float moveSpeed     = 0.f;   // horizontal speed this tick
    float hitFlashTimer = 0.f;
    float age           = 0.f;

    bool  enraged       = false; // WARDEN phase two
    bool  killedByBlast = false; // MITE: set when it detonated itself (not shot)

    EnemyEvents ev;

    Enemy(EnemyType t, glm::vec3 pos) : type(t), position(pos) {
        const EnemyStats& s = statsOf(t);
        health = maxHealth = s.health;
        attackTimer = frand(0.f, s.attackEvery * 0.6f);   // desync the squad
        strafeDir   = (rand() & 1) ? 1.f : -1.f;
        animPhase   = frand(0.f, 6.28f);
        if (s.flying) {
            hoverY      = pos.y > 2.f ? pos.y : 8.f;
            orbitRadius = frand(9.f, 15.f);
        }
    }

    const EnemyStats& stats() const { return statsOf(type); }
    float radius() const { return stats().radius; }
    float height() const { return stats().height; }
    bool  targetable() const { return alive && state == EnemyState::ACTIVE; }
    float telegraphProgress() const {
        return telegraphTimer > 0.f && telegraphDuration > 0.f
             ? 1.f - telegraphTimer / telegraphDuration : 0.f;
    }

    AABB getAABB() const {
        float r = radius();
        return { position + glm::vec3{-r, 0.f, -r}, position + glm::vec3{r, height(), r} };
    }

    // Returns true if this hit killed the enemy.
    bool takeDamage(float dmg) {
        if (!targetable()) return false;
        health -= dmg;
        hitFlashTimer = 0.12f;
        if (health <= 0.f) {
            health = 0.f;
            alive  = false;
            state  = EnemyState::DEAD;
            return true;
        }
        if (type == EnemyType::WARDEN && !enraged && health < maxHealth * 0.5f) {
            enraged    = true;
            ev.enraged = true;
        }
        return false;
    }

    void update(float dt, const EnemyWorld& w) {
        ev = EnemyEvents{};
        if (!alive) return;
        age += dt;
        if (hitFlashTimer > 0.f) hitFlashTimer -= dt;

        if (state == EnemyState::SPAWNING) {
            spawnTimer -= dt;
            if (spawnTimer <= 0.f) { spawnTimer = 0.f; state = EnemyState::ACTIVE; }
            // Face the player while materialising
            glm::vec3 d = w.playerFeet - position;
            if (glm::length(glm::vec2(d.x, d.z)) > 0.01f) yaw = std::atan2(d.x, d.z);
            return;
        }

        bool resolve = false;   // telegraph ran out this tick
        if (telegraphTimer > 0.f) {
            telegraphTimer -= dt;
            if (telegraphTimer <= 0.f) { telegraphTimer = 0.f; resolve = true; }
        }
        if (recoverTimer > 0.f) recoverTimer -= dt;
        if (noLosTimer   > 0.f) noLosTimer   -= dt;
        if (avoidTimer   > 0.f) avoidTimer   -= dt;

        switch (type) {
            case EnemyType::HUSK:     thinkHusk(dt, w, resolve);     break;
            case EnemyType::RIPPER:   thinkRipper(dt, w, resolve);   break;
            case EnemyType::SENTINEL: thinkSentinel(dt, w, resolve); break;
            case EnemyType::RAPTOR:   thinkRaptor(dt, w, resolve);   break;
            case EnemyType::BRUTE:    thinkBrute(dt, w, resolve);    break;
            case EnemyType::MITE:     thinkMite(dt, w, resolve);     break;
            case EnemyType::WARDEN:   thinkWarden(dt, w, resolve);   break;
            default: break;
        }
        integrate(dt, w);
    }

private:
    // ---- shared helpers ------------------------------------------------------
    glm::vec3 flatTo(const glm::vec3& target) const {
        glm::vec3 d = target - position; d.y = 0.f; return d;
    }
    static glm::vec3 norm2(glm::vec3 v) {
        v.y = 0.f; float l = glm::length(v); return l > 1e-4f ? v / l : glm::vec3{0.f};
    }
    static glm::vec3 rotY(glm::vec3 v, float a) {
        float c = std::cos(a), s = std::sin(a);
        return { v.x * c + v.z * s, v.y, -v.x * s + v.z * c };
    }

    void turnToward(glm::vec3 dir, float dt, float rate) {
        if (glm::length(glm::vec2(dir.x, dir.z)) < 1e-4f) return;
        float target = std::atan2(dir.x, dir.z);
        float diff = std::remainder(target - yaw, 6.2831853f);
        float step = rate * dt;
        yaw += glm::clamp(diff, -step, step);
    }

    void startAttack(AttackKind k, float windup) {
        attack            = k;
        telegraphDuration = windup;
        telegraphTimer    = windup;
        ev.telegraphStarted = true;
    }

    bool blockedAt(glm::vec3 p, const EnemyWorld& w) const {
        if (!w.walls) return false;
        float r = radius();
        AABB box{ p + glm::vec3{-r, 0.3f, -r}, p + glm::vec3{r, height(), r} };
        static std::vector<int> cands;
        if (w.grid) w.grid->query(box, cands);
        else { cands.clear(); for (int i = 0; i < w.wallCount; ++i) cands.push_back(i); }
        for (int i : cands) {
            const AABB& b = w.walls[i].box;
            if (box.max.x > b.min.x && box.min.x < b.max.x &&
                box.max.y > b.min.y && box.min.y < b.max.y &&
                box.max.z > b.min.z && box.min.z < b.max.z) return true;
        }
        return false;
    }

    // Gunners and Brutes hold their perch: on a raised surface they treat a
    // drop as a wall. Rippers and Mites happily leap down after you.
    bool ledgeAware() const {
        return !stats().flying && type != EnemyType::RIPPER && type != EnemyType::MITE;
    }

    // Is there something to stand on under p (within a step of its height)?
    bool supportedAt(glm::vec3 p, const EnemyWorld& w) const {
        if (p.y < 0.3f || !w.walls) return true;   // the world floor
        AABB q{p + glm::vec3{-0.05f, -1.4f, -0.05f}, p + glm::vec3{0.05f, 0.6f, 0.05f}};
        static std::vector<int> cands;
        if (w.grid) w.grid->query(q, cands);
        else { cands.clear(); for (int i = 0; i < w.wallCount; ++i) cands.push_back(i); }
        for (int i : cands) {
            const AABB& b = w.walls[i].box;
            if (p.x >= b.min.x && p.x <= b.max.x && p.z >= b.min.z && p.z <= b.max.z &&
                b.max.y >= p.y - 1.4f && b.max.y <= p.y + 0.6f) return true;
        }
        return false;
    }

    bool canStepTo(glm::vec3 p, const EnemyWorld& w) const {
        if (blockedAt(p, w)) return false;
        return !(ledgeAware() && position.y > 0.3f && !supportedAt(p, w));
    }

    // Feeler steering: if the way ahead is blocked, try turning ±45/90/135°
    // (keeping the side that worked last time) so enemies slide around cover
    // instead of grinding into it. No pathfinding: arenas are open by design.
    glm::vec3 steer(glm::vec3 want, const EnemyWorld& w) {
        want = norm2(want);
        if (glm::length(want) < 0.5f || stats().flying) return want;
        float probe = radius() + 0.9f;
        if (canStepTo(position + want * probe, w)) return want;
        static const float ANG[] = {0.785f, 1.571f, 2.356f};
        for (float a : ANG)
            for (float sgn : {avoidSign, -avoidSign}) {
                glm::vec3 d = rotY(want, a * sgn);
                if (canStepTo(position + d * probe, w)) {
                    avoidSign = sgn; avoidTimer = 0.6f;
                    return d;
                }
            }
        return glm::vec3{0.f};
    }

    void setMove(glm::vec3 dir, float speed, const EnemyWorld& w) {
        glm::vec3 d = steer(dir, w);
        velocity.x = d.x * speed;
        velocity.z = d.z * speed;
    }

    bool lineOfSight(const glm::vec3& from, const EnemyWorld& w) const {
        glm::vec3 d = w.playerEye - from;
        float dist = glm::length(d);
        if (dist < 1e-3f || !w.walls) return true;
        d /= dist;
        for (int i = 0; i < w.wallCount; ++i) {
            float t = rayBoxHit(from, d, w.walls[i].box);
            if (t > 0.f && t < dist) return false;
        }
        return true;
    }

    glm::vec3 eyePos() const { return position + glm::vec3{0.f, height() * 0.85f, 0.f}; }

    void fireAt(const glm::vec3& target, int n, float spread, float speed, float dmg, float size = 1.f) {
        ev.shotOrigin = eyePos();
        ev.shotSpeed  = speed;
        ev.shotDamage = dmg;
        ev.shotSize   = size;
        glm::vec3 base = target - ev.shotOrigin;
        float len = glm::length(base);
        base = len > 1e-4f ? base / len : glm::vec3{0,0,1};
        n = std::min(n, EnemyEvents::MAX_SHOTS);
        for (int i = 0; i < n; ++i) {
            float a = (n == 1) ? 0.f : -spread * 0.5f + spread * (float)i / (float)(n - 1);
            ev.shotDir[i] = glm::normalize(rotY(base, a));
        }
        ev.shots = n;
    }

    // Strafe-and-hold-range movement used by the gunners.
    void rangedMove(float dt, const EnemyWorld& w, float nearR, float farR, float speed) {
        glm::vec3 to = flatTo(w.playerFeet);
        float d = glm::length(to);
        glm::vec3 dir = norm2(to);
        glm::vec3 side{-dir.z, 0.f, dir.x};
        strafeTimer -= dt;
        if (strafeTimer <= 0.f) { strafeTimer = frand(1.6f, 3.2f); strafeDir = -strafeDir; }
        glm::vec3 mv;
        if (noLosTimer > 0.f)  mv = side * strafeDir + dir * 0.35f;      // sidestep out from behind cover
        else if (d < nearR)    mv = -dir + side * strafeDir * 0.7f;
        else if (d > farR)     mv = dir + side * strafeDir * 0.3f;
        else                   mv = side * strafeDir;
        setMove(mv, speed, w);
    }

    // Ticks the attack clock; returns true when it's time to start a new attack.
    bool attackReady(float dt) {
        if (telegraphTimer > 0.f || burstLeft > 0 || diveTimer > 0.f) return false;
        attackTimer += dt * (enraged ? 1.5f : 1.f);
        if (attackTimer < stats().attackEvery) return false;
        attackTimer = 0.f;
        return true;
    }

    // No clear shot: sidestep for a moment and look again soon, rather than
    // waiting out a whole attack cycle behind cover.
    void blockedShot() {
        noLosTimer  = 1.2f;
        attackTimer = stats().attackEvery - 0.35f;
    }

    // ---- per-type behaviour --------------------------------------------------
    void thinkHusk(float dt, const EnemyWorld& w, bool resolve) {
        float speed = stats().speed * (telegraphTimer > 0.f ? 0.25f : 1.f);
        rangedMove(dt, w, 8.f, 17.f, speed);
        turnToward(flatTo(w.playerFeet), dt, 6.f);
        if (resolve && attack == AttackKind::SHOT) {
            fireAt(w.playerEye, 1, 0.f, 17.f, 10.f);
            attack = AttackKind::NONE;
        }
        if (attackReady(dt)) {
            if (lineOfSight(eyePos(), w)) startAttack(AttackKind::SHOT, stats().telegraph);
            else blockedShot();
        }
    }

    void thinkRipper(float dt, const EnemyWorld& w, bool resolve) {
        glm::vec3 to = flatTo(w.playerFeet);
        float d = glm::length(to);
        glm::vec3 dir = norm2(to);
        glm::vec3 side{-dir.z, 0.f, dir.x};
        strafeTimer -= dt;
        if (strafeTimer <= 0.f) { strafeTimer = frand(0.6f, 1.2f); strafeDir = -strafeDir; }

        if (attack == AttackKind::LUNGE) {
            // Crouch almost still, then spring on the last tenth of a second
            float spd = telegraphTimer < 0.1f ? 15.f : 0.8f;
            velocity.x = dir.x * spd; velocity.z = dir.z * spd;
            if (resolve) {
                attack = AttackKind::NONE;
                float dy = w.playerFeet.y - position.y;
                if (d < 2.7f && std::fabs(dy) < 2.5f) { ev.meleeHit = true; ev.meleeDamage = 13.f; }
                recoverTimer = 0.45f;
            }
        } else if (recoverTimer > 0.f) {
            setMove(-dir + side * strafeDir, stats().speed * 0.7f, w);   // hit and run
        } else {
            float zig = d < 5.f ? 0.15f : 0.75f;
            setMove(dir + side * strafeDir * zig, stats().speed, w);
            attackTimer += dt;
            if (attackTimer >= stats().attackEvery && d < 4.2f) {
                attackTimer = 0.f;
                startAttack(AttackKind::LUNGE, stats().telegraph);
            }
        }
        turnToward(to, dt, 10.f);
    }

    void thinkSentinel(float dt, const EnemyWorld& w, bool resolve) {
        float speed = (telegraphTimer > 0.f || burstLeft > 0) ? 0.f : stats().speed;
        rangedMove(dt, w, 15.f, 26.f, speed);
        turnToward(flatTo(w.playerFeet), dt, 4.f);
        if (resolve && attack == AttackKind::BURST) {
            attack = AttackKind::NONE;
            // The laser warned you: if you broke line of sight, the shot is lost.
            if (lineOfSight(eyePos(), w)) { burstLeft = 3; burstTimer = 0.f; }
            else blockedShot();
        }
        if (burstLeft > 0) {
            burstTimer -= dt;
            if (burstTimer <= 0.f) {
                fireAt(w.playerEye, 1, 0.f, 32.f, 7.f, 0.8f);
                --burstLeft; burstTimer = 0.12f;
            }
        }
        if (attackReady(dt)) {
            if (lineOfSight(eyePos(), w)) startAttack(AttackKind::BURST, stats().telegraph);
            else blockedShot();
        }
    }

    void thinkRaptor(float dt, const EnemyWorld& w, bool resolve) {
        animPhase += dt * (diveTimer > 0.f ? 5.f : 9.f);
        glm::vec3 to3 = w.playerEye - position;
        glm::vec3 to  = flatTo(w.playerFeet);
        float d = glm::length(to);
        glm::vec3 dir = norm2(to);
        glm::vec3 side{-dir.z, 0.f, dir.x};

        if (diveTimer > 0.f) {
            diveTimer -= dt;
            velocity = diveDir * 17.f;
            pitch = glm::mix(pitch, 0.7f, std::min(1.f, dt * 8.f));
            if (glm::length(w.playerEye - position) < 1.8f) {
                ev.meleeHit = true; ev.meleeDamage = 12.f;
                diveTimer = 0.f; recoverTimer = 1.2f;
            }
            if (diveTimer <= 0.f) recoverTimer = std::max(recoverTimer, 1.0f);
            turnToward(diveDir, dt, 8.f);
            return;
        }
        pitch = glm::mix(pitch, 0.f, std::min(1.f, dt * 4.f));

        strafeTimer -= dt;
        if (strafeTimer <= 0.f) { strafeTimer = frand(2.5f, 4.5f); strafeDir = -strafeDir; }
        // Orbit: tangent plus a radial correction toward the preferred radius
        float radial = glm::clamp((d - orbitRadius) * 0.25f, -1.f, 1.f);
        glm::vec3 mv = norm2(side * strafeDir + dir * radial);
        velocity.x = mv.x * stats().speed;
        velocity.z = mv.z * stats().speed;
        float targetY = recoverTimer > 0.f ? hoverY + 2.f : hoverY;
        velocity.y = glm::clamp((targetY - position.y) * 3.f, -8.f, 8.f);
        turnToward(glm::vec3{velocity.x, 0.f, velocity.z}, dt, 5.f);

        if (resolve) {
            if (attack == AttackKind::SHOT) fireAt(w.playerEye, 1, 0.f, 18.f, 9.f);
            else if (attack == AttackKind::DIVE) {
                float l = glm::length(to3);
                diveDir   = l > 1e-3f ? to3 / l : glm::vec3{0,-1,0};
                diveTimer = 1.3f;
            }
            attack = AttackKind::NONE;
        }
        if (recoverTimer <= 0.f && attackReady(dt)) {
            ++attackCount;
            if (attackCount % 3 == 0) startAttack(AttackKind::DIVE, 0.55f);
            else if (lineOfSight(eyePos(), w)) startAttack(AttackKind::SHOT, stats().telegraph);
        }
    }

    void thinkBrute(float dt, const EnemyWorld& w, bool resolve) {
        glm::vec3 to = flatTo(w.playerFeet);
        float d = glm::length(to);
        if (telegraphTimer > 0.f) {
            velocity.x = velocity.z = 0.f;
        } else {
            setMove(to, stats().speed * (d > 16.f ? 1.35f : 1.f), w);
            animPhase += dt * 4.f;
        }
        turnToward(to, dt, telegraphTimer > 0.f ? 1.f : 3.f);
        if (resolve) {
            if (attack == AttackKind::SLAM) {
                ev.slam = true; ev.slamRadius = 8.f; ev.slamDamage = 26.f;
            } else if (attack == AttackKind::LOB) {
                fireAt(w.playerEye, 1, 0.f, 15.f, 18.f, 2.2f);
            }
            attack = AttackKind::NONE;
        }
        if (attackReady(dt)) {
            if (d < 7.f) startAttack(AttackKind::SLAM, stats().telegraph);
            else if (d > 12.f && lineOfSight(eyePos(), w)) startAttack(AttackKind::LOB, 0.7f);
            else attackTimer = stats().attackEvery * 0.7f;  // close the gap, re-check soon
        }
    }

    void thinkMite(float dt, const EnemyWorld& w, bool resolve) {
        glm::vec3 to = flatTo(w.playerFeet);
        float d = glm::length(to);
        glm::vec3 dir = norm2(to);
        glm::vec3 side{-dir.z, 0.f, dir.x};
        animPhase += dt * 14.f;
        if (attack == AttackKind::FUSE) {
            setMove(dir, 2.5f, w);
            if (resolve) {
                ev.detonated  = true;
                killedByBlast = true;
                alive = false; state = EnemyState::DEAD; health = 0.f;
                return;
            }
        } else {
            float wobble = std::sin(age * 7.f + animPhase) * 0.35f;
            setMove(dir + side * wobble, stats().speed, w);
            if (d < 2.6f && std::fabs(w.playerFeet.y - position.y) < 2.f)
                startAttack(AttackKind::FUSE, stats().telegraph);
        }
        turnToward(to, dt, 12.f);
    }

    void thinkWarden(float dt, const EnemyWorld& w, bool resolve) {
        glm::vec3 to = flatTo(w.playerFeet);
        float d = glm::length(to);
        float speed = stats().speed * (enraged ? 1.4f : 1.f);
        if (telegraphTimer > 0.f) { velocity.x = velocity.z = 0.f; }
        else {
            glm::vec3 dir = norm2(to);
            glm::vec3 side{-dir.z, 0.f, dir.x};
            strafeTimer -= dt;
            if (strafeTimer <= 0.f) { strafeTimer = frand(3.f, 5.f); strafeDir = -strafeDir; }
            glm::vec3 mv = d > 11.f ? dir + side * strafeDir * 0.4f : side * strafeDir;
            setMove(mv, speed, w);
            animPhase += dt * 3.f;
        }
        turnToward(to, dt, 2.f);

        if (resolve) {
            switch (attack) {
                case AttackKind::SLAM:
                    ev.slam = true; ev.slamRadius = 11.f; ev.slamDamage = 30.f; break;
                case AttackKind::VOLLEY:
                    fireAt(w.playerEye, enraged ? 11 : 7, enraged ? 1.4f : 0.9f, 18.f, 12.f, 1.6f);
                    break;
                case AttackKind::SUMMON:
                    ev.summonMites   = enraged ? 2 : 3;
                    ev.summonRippers = enraged ? 2 : 0;
                    break;
                default: break;
            }
            attack = AttackKind::NONE;
        }
        if (attackReady(dt)) {
            ++attackCount;
            if (d < 9.f)                   startAttack(AttackKind::SLAM, 1.0f);
            else if (attackCount % 4 == 0) startAttack(AttackKind::SUMMON, 1.2f);
            else                           startAttack(AttackKind::VOLLEY, stats().telegraph);
        }
    }

    // ---- physics -------------------------------------------------------------
    void integrate(float dt, const EnemyWorld& w) {
        bool flying = stats().flying;
        if (!flying) {
            velocity.y -= 24.f * dt;
            moveSpeed = glm::length(glm::vec2(velocity.x, velocity.z));
            if (type != EnemyType::RAPTOR && type != EnemyType::MITE &&
                type != EnemyType::BRUTE && type != EnemyType::WARDEN)
                animPhase += dt * moveSpeed * 1.9f;
        } else {
            moveSpeed = glm::length(velocity);
        }
        position += velocity * dt;

        if (position.y < 0.f) { position.y = 0.f; if (velocity.y < 0.f) velocity.y = 0.f; }

        if (w.walls) {
            static std::vector<int> cands;
            float r = radius();
            AABB eb{ position + glm::vec3{-r-0.1f, -0.1f, -r-0.1f},
                     position + glm::vec3{ r+0.1f, height()+0.1f, r+0.1f} };
            if (w.grid) w.grid->query(eb, cands);
            else { cands.clear(); for (int i = 0; i < w.wallCount; ++i) cands.push_back(i); }
            for (int idx : cands) resolveAABB(w.walls[idx].box);
        }

        // Stay inside the arena
        float r = radius();
        position.x = glm::clamp(position.x, w.bounds.min.x + r, w.bounds.max.x - r);
        position.z = glm::clamp(position.z, w.bounds.min.z + r, w.bounds.max.z - r);
        if (flying) position.y = glm::clamp(position.y, w.bounds.min.y + 1.5f, w.bounds.max.y - height());
    }

    void resolveAABB(const AABB& wall) {
        float r = radius(), h = height();
        glm::vec3 pMin = position + glm::vec3{-r, 0.f, -r};
        glm::vec3 pMax = position + glm::vec3{ r, h,   r};
        if (pMax.x <= wall.min.x || pMin.x >= wall.max.x) return;
        if (pMax.y <= wall.min.y || pMin.y >= wall.max.y) return;
        if (pMax.z <= wall.min.z || pMin.z >= wall.max.z) return;
        float ox = std::min(pMax.x - wall.min.x, wall.max.x - pMin.x);
        float oy = std::min(pMax.y - wall.min.y, wall.max.y - pMin.y);
        float oz = std::min(pMax.z - wall.min.z, wall.max.z - pMin.z);
        if (ox <= oy && ox <= oz) {
            position.x += (position.x < (wall.min.x + wall.max.x) * 0.5f) ? -ox : ox;
        } else if (oy <= ox && oy <= oz) {
            bool up = position.y + h * 0.5f > (wall.min.y + wall.max.y) * 0.5f;
            position.y += up ? oy : -oy;
            if (up && velocity.y < 0.f) velocity.y = 0.f;
            if (!up && velocity.y > 0.f) velocity.y = 0.f;
        } else {
            position.z += (position.z < (wall.min.z + wall.max.z) * 0.5f) ? -oz : oz;
        }
    }
};
