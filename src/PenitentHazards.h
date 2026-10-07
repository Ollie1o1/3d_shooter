#pragma once
// =============================================================================
// PenitentHazards.h — what THE PENITENT throws at you besides its body: its
// censer sweeps (low: jump; high: slide under), the shockwave rings of its
// slam and stomp and the embers off its own back, the chain lash marked along
// the floor, and burning incense. No OpenGL or audio: GameplayState feeds it
// the Penitent's events, draws it, and turns its hits into damage
// (Gameplay_Penitent.h).
// =============================================================================
#include <glm/glm.hpp>
#include <vector>
#include <cmath>
#include <algorithm>

// Ripping a chain anchor out with the grapple
// Ripping a chain anchor out with the grapple: hook it and hang on for half a
// second. The hook lets go by itself as you arrive (within 2 m), so reaching
// the anchor counts as hanging on; letting go early, far from it, cancels.
// update(): hooked = the anchor the hook is on (-1: none), dist = how far you
// are from the anchor being ripped. Returns the anchor to break, or -1.
struct AnchorRip {
    static constexpr float TIME = 0.5f, REACHED = 3.5f;
    int anchor = -1; float t = 0.f; bool arrived = false;
    int update(float dt, int hooked, bool active, float dist) {
        if (active && hooked >= 0 && hooked != anchor) { anchor = hooked; t = 0.f; arrived = false; }
        if (anchor < 0) return -1;
        if (dist < REACHED) arrived = true;
        if (!active && !arrived) { anchor = -1; t = 0.f; return -1; }   // let go before getting there
        t += dt;
        if (t >= TIME) { int a = anchor; anchor = -1; t = 0.f; arrived = false; return a; }
        return -1;
    }
};

class PenitentHazards {
public:
    static constexpr float HALF_ARC = 1.7453293f;          // ±100°
    static constexpr float LOW_TOP = 1.0f, HIGH_DUCK = 1.3f, HIGH_TOP = 3.5f;
    static constexpr float SLAM_RADIUS = 14.f, STOMP_RADIUS = 6.f, RING_SPEED = 16.f, RING_HEIGHT = 1.2f, EMBER_RANGE = 30.f;
    static constexpr float LASH_WARN = 0.7f, LASH_REACH = 70.f, LASH_HALF = 1.3f, LASH_LINGER = 0.3f;   // reaches across the pit
    static constexpr float POOL_RADIUS = 3.5f, POOL_TIME = 8.f, POOL_WARN = 0.6f, POOL_DPS = 20.f, POOL_TICK = 0.25f;
    static constexpr float SWEEP_DAMAGE = 30.f, SLAM_DAMAGE = 30.f, LASH_DAMAGE = 35.f, EMBER_DAMAGE = 15.f;
    struct Arc  { glm::vec3 centre; float yaw, reach; int kind; float t = 0.f; };          // a drawn sweep trail
    struct Ring { glm::vec3 centre; float radius = 0.f, prev = 0.f, maxR; float damage; int kind; bool hit = false; }; // 0 slam/stomp, 1 embers
    struct Lash { glm::vec3 from, dir; float t = 0.f; bool yank; bool fired = false; };
    struct Pool { glm::vec3 pos; float t = 0.f; };
    struct Hit  { float damage; glm::vec3 from; bool yank; };
    std::vector<Arc> arcs; std::vector<Ring> rings; std::vector<Lash> lashes; std::vector<Pool> pools;

    // Does a sweep of `kind` from `centre`, facing `yaw` (0 = +Z), reach the
    // player (feet, height) on a floor at floorY?
    static bool sweepHits(int kind, glm::vec3 centre, float yaw, float reach, glm::vec3 feet, float height, float floorY) {
        glm::vec2 d{feet.x - centre.x, feet.z - centre.z};
        float dist = glm::length(d);
        if (dist > reach) return false;
        if (dist > 0.5f) {
            glm::vec2 f{std::sin(yaw), std::cos(yaw)};
            if (glm::dot(d / dist, f) < std::cos(HALF_ARC)) return false;
        }
        float up = feet.y - floorY;
        if (kind == 0) return up < LOW_TOP;                          // low: clear it by jumping
        return up + height > HIGH_DUCK && up < HIGH_TOP;              // high: duck under it
    }
    void addSweep(glm::vec3 centre, float yaw, float reach, int kind) { arcs.push_back({centre, yaw, reach, kind}); }
    void addRing(glm::vec3 centre, float maxR, float damage, int kind) { Ring r; r.centre = centre; r.maxR = maxR; r.damage = damage; r.kind = kind; rings.push_back(r); }
    void addLash(glm::vec3 from, glm::vec3 dir, bool yank) { Lash l; l.from = from; l.dir = dir; l.yank = yank; lashes.push_back(l); }
    void addPool(glm::vec3 pos) { Pool p; p.pos = pos; pools.push_back(p); }
    void clear() { arcs.clear(); rings.clear(); lashes.clear(); pools.clear(); poolTick = 0.f; }

    std::vector<Hit> update(float dt, glm::vec3 feet, float floorY) {
        std::vector<Hit> hits;
        for (auto& a : arcs) a.t += dt;
        arcs.erase(std::remove_if(arcs.begin(), arcs.end(), [](const Arc& a) { return a.t > 0.4f; }), arcs.end());
        float up = feet.y - floorY;
        for (auto& r : rings) {
            r.prev = r.radius; r.radius += RING_SPEED * dt;
            float d = glm::length(glm::vec2{feet.x - r.centre.x, feet.z - r.centre.z});
            if (!r.hit && up < RING_HEIGHT && d >= r.prev && d < r.radius) { r.hit = true; hits.push_back({r.damage, r.centre, false}); }
        }
        rings.erase(std::remove_if(rings.begin(), rings.end(), [](const Ring& r) { return r.radius >= r.maxR; }), rings.end());
        for (auto& l : lashes) {
            l.t += dt;
            if (!l.fired && l.t >= LASH_WARN) {
                l.fired = true;
                glm::vec3 chest = feet + glm::vec3{0.f, 0.9f, 0.f};   // a line through the air, at your chest
                float along = glm::dot(chest - l.from, l.dir);
                float side = glm::length((chest - l.from) - l.dir * along);
                if (along > 0.f && along < LASH_REACH && side < LASH_HALF)
                    hits.push_back({LASH_DAMAGE, l.from, l.yank});
            }
        }
        lashes.erase(std::remove_if(lashes.begin(), lashes.end(), [](const Lash& l) { return l.t > LASH_WARN + LASH_LINGER; }), lashes.end());
        bool inPool = false;
        for (auto& p : pools) {
            p.t += dt;
            if (p.t >= POOL_WARN && up < 0.6f && glm::length(glm::vec2{feet.x - p.pos.x, feet.z - p.pos.z}) < POOL_RADIUS) inPool = true;
        }
        pools.erase(std::remove_if(pools.begin(), pools.end(), [](const Pool& p) { return p.t > POOL_TIME; }), pools.end());
        if (inPool) {
            poolTick += dt;
            while (poolTick >= POOL_TICK) { poolTick -= POOL_TICK; hits.push_back({POOL_DPS * POOL_TICK, feet, false}); }
        } else poolTick = 0.f;
        return hits;
    }
private:
    float poolTick = 0.f;
};
