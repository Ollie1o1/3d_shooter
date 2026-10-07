#pragma once
// =============================================================================
// WardenHazards.h — what THE WARDEN throws at you besides its body and its
// orbs: the lance (a beam sweeping toward you that a pillar stops), the
// seeker (an orb arcing over cover onto where you stood), the steam off its
// open vents; and the clock its conduits re-attach by. No OpenGL or audio:
// GameplayState feeds it the Warden's events and turns hits into damage
// (Gameplay_Warden.h).
// =============================================================================
#include "Player.h"   // AABB, Wall
#include <glm/glm.hpp>
#include <algorithm>
#include <cmath>
#include <vector>

// All four conduits cut: 20 s on, they re-attach one every 5 s. update()
// returns how many should be attached again by now (0..4)
struct ConduitClock {
    static constexpr float AFTER = 20.f, GAP = 5.f;
    float t = -1.f;
    void allCut() { t = 0.f; }
    void reset()  { t = -1.f; }
    bool running() const { return t >= 0.f; }
    int update(float dt) {
        if (t < 0.f) return 0;
        t += dt;
        if (t < AFTER) return 0;
        return std::min(4, 1 + (int)((t - AFTER) / GAP));
    }
};

class WardenHazards {
public:
    static constexpr float LANCE_SPEED = 0.6108652f, LANCE_ARC = 2.0943951f;   // 35 degrees a second, 120 degrees
    static constexpr float LANCE_TIME = LANCE_ARC / LANCE_SPEED;
    static constexpr float LANCE_REACH = 45.f, LANCE_HALF = 1.0f, LANCE_BAND = 2.2f, LANCE_DPS = 25.f, LANCE_TICK = 0.2f;
    static constexpr float SEEK_WARN = 1.f, SEEK_RADIUS = 4.f, SEEK_DAMAGE = 35.f;
    static constexpr float STEAM_RADIUS = 4.f, STEAM_DPS = 15.f, STEAM_TICK = 0.25f;
    static constexpr float DETONATE_DAMAGE = 60.f;
    struct Lance  { glm::vec3 from; float yaw0, sign, t = 0.f;
                    float yaw() const { return yaw0 + sign * LANCE_SPEED * std::min(t, LANCE_TIME); } };
    struct Seeker { glm::vec3 at; float t = 0.f; bool fired = false; };
    struct Hit    { float damage; glm::vec3 from; };
    std::vector<Lance> lances; std::vector<Seeker> seekers;

    void addLance(glm::vec3 from, float yaw0, float sign) { lances.push_back({from, yaw0, sign}); }
    void addSeeker(glm::vec3 at) { seekers.push_back({at}); }
    void clear() { lances.clear(); seekers.clear(); lanceTick = steamTick = 0.f; }

    // Is the player (feet, height) on the beam fired from `from` along yaw
    // (0 = +Z)? A line through the air in a band round the beam's height,
    // within LANCE_HALF of it sideways, and no wall in between
    static bool lanceTouches(glm::vec3 from, float yaw, glm::vec3 feet, float height, const Wall* walls, int n) {
        glm::vec3 dir{std::sin(yaw), 0.f, std::cos(yaw)};
        glm::vec2 rel{feet.x - from.x, feet.z - from.z};
        float along = rel.x * dir.x + rel.y * dir.z;
        if (along <= 0.f || along > LANCE_REACH) return false;
        float side = std::fabs(rel.x * dir.z - rel.y * dir.x);
        if (side > LANCE_HALF) return false;
        if (feet.y > from.y + 0.5f || feet.y + height < from.y - LANCE_BAND) return false;   // above it, or well below
        glm::vec3 o{from.x, std::min(from.y, feet.y + height * 0.6f), from.z};
        glm::vec3 seg = glm::vec3{feet.x, o.y, feet.z} - o;
        for (int i = 0; i < n; ++i) if (segHits(o, seg, walls[i].box)) return false;
        return true;
    }

    std::vector<Hit> update(float dt, glm::vec3 feet, float height, const Wall* walls, int n, glm::vec3 warden, bool venting) {
        std::vector<Hit> hits;
        bool burning = false;
        for (auto& l : lances) {
            l.t += dt;
            if (l.t <= LANCE_TIME && lanceTouches(l.from, l.yaw(), feet, height, walls, n)) burning = true;
        }
        lances.erase(std::remove_if(lances.begin(), lances.end(), [](const Lance& l) { return l.t > LANCE_TIME; }), lances.end());
        if (burning) {
            lanceTick += dt;
            while (lanceTick >= LANCE_TICK) { lanceTick -= LANCE_TICK; hits.push_back({LANCE_DPS * LANCE_TICK, warden}); }
        } else lanceTick = LANCE_TICK;   // the first touch burns at once
        for (auto& s : seekers) {
            s.t += dt;
            if (!s.fired && s.t >= SEEK_WARN) {
                s.fired = true;
                if (glm::length(glm::vec2{feet.x - s.at.x, feet.z - s.at.z}) < SEEK_RADIUS && std::fabs(feet.y - s.at.y) < 2.5f)
                    hits.push_back({SEEK_DAMAGE, s.at});
            }
        }
        seekers.erase(std::remove_if(seekers.begin(), seekers.end(), [](const Seeker& s) { return s.t > SEEK_WARN + 0.4f; }), seekers.end());
        if (venting && glm::length(glm::vec2{feet.x - warden.x, feet.z - warden.z}) < STEAM_RADIUS && std::fabs(feet.y - warden.y) < 3.f) {
            steamTick += dt;
            while (steamTick >= STEAM_TICK) { steamTick -= STEAM_TICK; hits.push_back({STEAM_DPS * STEAM_TICK, warden}); }
        } else steamTick = 0.f;
        return hits;
    }
private:
    float lanceTick = LANCE_TICK, steamTick = 0.f;
    // Does the segment o -> o+seg pass through box b? (slab test)
    static bool segHits(glm::vec3 o, glm::vec3 seg, const AABB& b) {
        float t0 = 0.f, t1 = 1.f;
        for (int k = 0; k < 3; ++k) {
            if (std::fabs(seg[k]) < 1e-6f) { if (o[k] < b.min[k] || o[k] > b.max[k]) return false; continue; }
            float a = (b.min[k] - o[k]) / seg[k], c = (b.max[k] - o[k]) / seg[k];
            if (a > c) std::swap(a, c);
            t0 = std::max(t0, a); t1 = std::min(t1, c);
            if (t0 > t1) return false;
        }
        return true;
    }
};

// A parried orb's flight toward the Warden's core: turn up to 6 rad/s toward
// target, same speed
inline glm::vec3 steerParried(glm::vec3 pos, glm::vec3 vel, glm::vec3 target, float dt) {
    float speed = glm::length(vel);
    glm::vec3 want = target - pos;
    if (speed < 1e-3f || glm::length(want) < 1e-3f) return vel;
    glm::vec3 a = vel / speed, b = glm::normalize(want);
    float ang = std::acos(std::clamp(glm::dot(a, b), -1.f, 1.f)), turn = 6.f * dt;
    if (ang <= turn) return b * speed;
    glm::vec3 axis = glm::cross(a, b);
    if (glm::length(axis) < 1e-5f) return vel;
    axis = glm::normalize(axis);
    glm::vec3 r = a * std::cos(turn) + glm::cross(axis, a) * std::sin(turn) + axis * glm::dot(axis, a) * (1.f - std::cos(turn));
    return glm::normalize(r) * speed;
}
// Conduits are only targets while the Warden lives and feeds (phase 1)
inline bool conduitShootable(bool wardenAlive, int phase) { return wardenAlive && phase == 1; }
