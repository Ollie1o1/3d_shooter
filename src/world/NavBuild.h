#pragma once
// =============================================================================
// NavBuild.h — one arena's NavGrid, kept true to the level. Cut when a fight
// moves to another arena, and cut again when the ground changes under it: a
// door opens, cover breaks, a load comes down, the water rises. (The check is
// a hash of the arena's static boxes, a few times a second.)
//
// Two kinds of arena go without: the one that is a moving platform more than
// ground (the Descent's cage), and anything
// too wide to cut finely (Chapter 4's plain). Enemies there steer as before.
// =============================================================================
#include "world/Level.h"
#include "world/NavGrid.h"
#include <chrono>
#include <cstring>

// Over a void every walker keeps to real ground (Enemy::canStepTo): the Blast Chillers,
// the Living Ward's decks. One place for the game and the tests to ask.
inline bool arenaVoidUnder(const LevelData& L, int a) {
    if (a < 0 || a >= (int)L.arenas.size()) return false;
    if (!L.story.built) return false;
    if (L.story.chapter == 5) return a == 5;
    if (L.story.chapter == 6) return a == 1 || a == 2;   // the Drift: islands over nothing
    if (L.story.chapter == 3) return a == 5 || a == 7 || a == 8 || a == 9;
    return false;
}
// Where an enemy's floor is whatever is really under it (LevelData::enemyFloor), not the arena's hard floor
inline bool arenaGroundTracked(const LevelData& L, int a) {
    if (a < 0 || a >= (int)L.arenas.size()) return false;
    if (L.story.built && (L.story.chapter == 3 || L.story.chapter == 5 || L.story.chapter == 6)) return true;
    return L.arenas[a].shift == ArenaShift::DESCENT;
}

struct ArenaNav {
    NavGrid grid;
    int arena = -2;
    uint32_t key = 0;
    float recheck = 0.f;
    int builds = 0;
    double worstBuildMs = 0.;

    static bool arenaTakesNav(const Arena& a) {
        return a.shift != ArenaShift::DESCENT;
    }
    // Everything a route depends on: the static boxes over the arena, and its water
    static uint32_t geometryKey(const LevelData& L, const AABB& b) {
        uint32_t h = 2166136261u;
        auto mix = [&](float v) { uint32_t u; std::memcpy(&u, &v, 4); h = (h ^ u) * 16777619u; };
        for (const Wall& w : L.walls) {
            if (w.dynamic) continue;
            const AABB& x = w.box;
            if (x.max.x < b.min.x || x.min.x > b.max.x || x.max.z < b.min.z || x.min.z > b.max.z) continue;
            mix(x.min.x); mix(x.min.y); mix(x.min.z); mix(x.max.x); mix(x.max.y); mix(x.max.z);
        }
        for (const WaterVolume& w : L.water) mix(w.level);
        return h;
    }
    void rebuild(const LevelData& L, int a, const SpatialGrid* sg) {
        arena = a; grid.clear();
        if (a < 0 || a >= (int)L.arenas.size()) return;
        const Arena& A = L.arenas[a];
        key = geometryKey(L, A.bounds);
        if (!arenaTakesNav(A)) return;
        const auto t0 = std::chrono::steady_clock::now();
        burns_.clear();
        for (const Hazard& h : L.hazards) burns_.push_back(h.box);
        grid.build(L.walls.data(), (int)L.walls.size(), sg, A.bounds, A.voidY,
                   [&](float x, float z) { return L.floorWithWater(x, z, false); }, burns_.data(), (int)burns_.size());
        grid.buildMs = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
        worstBuildMs = std::max(worstBuildMs, grid.buildMs);
        ++builds;
    }
    // Once a physics tick, before the enemies think
    void update(const LevelData& L, int a, const SpatialGrid* sg, glm::vec3 playerFeet, float dt) {
        recheck -= dt;
        if (a != arena) { rebuild(L, a, sg); recheck = 0.25f; }
        else if (recheck <= 0.f) {
            recheck = 0.25f;
            if (a >= 0 && a < (int)L.arenas.size() && geometryKey(L, L.arenas[a].bounds) != key) rebuild(L, a, sg);
        }
        grid.tick(dt, playerFeet);
    }
    const NavGrid* get() const { return grid.built ? &grid : nullptr; }
private:
    std::vector<AABB> burns_;
};
