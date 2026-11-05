#pragma once
// =============================================================================
// RouteScan.h — can you get through a Story chapter on foot? For each section
// and the next, with every door open: from the section's start, walking,
// dropping and jumping (a double jump: BoundsScan's reach), with no grapple,
// lift, belt or pad, do you reach the next section's start? A route that needs
// one of those says so in the chapter's list of known aids (tests/test_route.cpp);
// any other break is a bug. Also checks every section start (each checkpoint's
// spawn) is on something you can stand on, with room over your head.
// =============================================================================
#include "BoundsScan.h"

namespace rscan {

constexpr float LEAP = 3.f;   // a running double jump goes ~8 m: 3 m across a gap is well within it

struct Leg {
    int from = 0;            // section a -> a + 1
    bool reached = false;
    float startMiss = 0.f;   // the start's distance from the nearest standing place
    float gap = 0.f;         // unreached: how close you get to the next start
    glm::vec3 closest{0.f};  // ...and where that is
    int nodes = 0;
};

// One leg: section a's start to section a + 1's
inline Leg leg(const LevelData& L, int a) {
    const Arena& A = L.arenas[a]; const Arena& B = L.arenas[a + 1];
    // Story keeps every section you've reached open, and the next (Gameplay_Tick.h keepPlayerInZone);
    // the one before this is enough for a route (a section's start may sit at the end of the last)
    std::vector<AABB> zones;
    for (int s = std::max(0, a - 1); s <= a + 1; ++s) { auto z = bscan::zonesOf(L.arenas[s]); zones.insert(zones.end(), z.begin(), z.end()); }
    if (a < (int)L.corridors.size()) zones.push_back(L.corridors[a]);
    std::vector<int> doors = bscan::doorWalls(L);
    bscan::Graph G = bscan::buildGraph(L, zones, std::min(A.voidY, B.voidY), &doors, bscan::JUMP_REACH, LEAP);
    Leg r; r.from = a; r.nodes = (int)G.nodes.size();
    int s = G.nearest(A.playerStart, &r.startMiss);
    float toMiss = 0.f; int t = G.nearest(B.playerStart, &toMiss);
    auto seen = bscan::reachFrom(G, s);
    r.reached = t >= 0 && seen[t];
    if (!r.reached) {
        r.gap = 1e9f;
        for (int n = 0; n < (int)G.nodes.size(); ++n) if (seen[n]) {
            float d = glm::length(G.at(n) - B.playerStart);
            if (d < r.gap) { r.gap = d; r.closest = G.at(n); }
        }
    }
    return r;
}

// A section start you spawn at: is there a floor (or box top) under it, close, and room to stand?
struct Spawn { int section = 0; float drop = 0.f; bool headroom = true, embedded = false; };
inline Spawn spawn(const LevelData& L, int a) {
    const Arena& A = L.arenas[a];
    const glm::vec3 p = A.playerStart;
    float top = L.baseFloor(p.x, p.z);
    if (top <= A.voidY) top = -1e9f;
    Spawn s; s.section = a;
    const AABB body{p + glm::vec3{-bscan::R + 0.06f, 0.08f, -bscan::R + 0.06f}, p + glm::vec3{bscan::R - 0.06f, bscan::H, bscan::R - 0.06f}};
    for (const Wall& w : L.walls) {
        if (w.dynamic) continue;
        const AABB& b = w.box;
        const bool under = p.x + bscan::R > b.min.x && p.x - bscan::R < b.max.x && p.z + bscan::R > b.min.z && p.z - bscan::R < b.max.z;
        if (under && b.max.y <= p.y + 0.3f) top = std::max(top, b.max.y);
        if (bscan::overlaps(body, b)) {
            if (b.min.y > p.y + 0.5f) s.headroom = false; else s.embedded = true;
        }
    }
    s.drop = p.y - top;
    return s;
}

}  // namespace rscan
