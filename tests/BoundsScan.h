#pragma once
// =============================================================================
// BoundsScan.h — can you get stuck somewhere? For one arena, every place a
// player could stand inside its zone (on the floor or on any box top, the body
// fitting under whatever is above and under the zone's ceiling) is treated as
// reachable: players find a way up anything (pads, the grapple, a dash). From
// each, the scan asks whether you can get back by yourself: walk, drop, or jump
// up at most JUMP_REACH (a double jump with some margin), with no grapple, pad
// or moving platform. Back means the arena's start, or a fall into the void
// (which puts you back at the checkpoint). Whatever can't is a trap.
// =============================================================================
#include "world/Level.h"
#include "core/Collision.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <deque>
#include <vector>

namespace bscan {

constexpr float CELL = 0.4f;          // grid step (m)
constexpr float R = 0.4f, H = 1.8f;   // the player's half-width and height
constexpr float JUMP_REACH = 3.2f;    // a double jump peaks ~3.6 m: a margin for timing and edges

struct Trap { glm::vec3 at; };
struct Result {
    int nodes = 0, traps = 0;
    float startMiss = 0.f;   // how far the start point was from the surface taken as the start
    int outsideHome = 0;     // surfaces (trap or not) outside home
    AABB outBox{glm::vec3{1e9f}, glm::vec3{-1e9f}};
    AABB trapBox{glm::vec3{1e9f}, glm::vec3{-1e9f}};
    std::vector<Trap> samples;   // a few of them
    std::vector<Trap> all;       // every one
};

inline bool overlaps(const AABB& a, const AABB& b) {
    return a.min.x < b.max.x && a.max.x > b.min.x && a.min.y < b.max.y && a.max.y > b.min.y &&
           a.min.z < b.max.z && a.max.z > b.min.z;
}

// Every place the body fits standing (on the floor or any box top, under the zones' ceilings), and who can
// get to whom by walking, dropping or jumping up at most `reach` (static walls only; skipWalls: left out)
struct Graph {
    AABB all{glm::vec3{1e9f}, glm::vec3{-1e9f}};
    int nx = 0, nz = 0;
    struct Node { int i, k; float y; bool start = false, voidEdge = false; };
    std::vector<Node> nodes;
    std::vector<std::vector<int>> col;    // each column's nodes
    std::vector<std::vector<int>> into;   // into[b]: the nodes that can get to b
    float cx(int i) const { return all.min.x + i * CELL; }
    float cz(int k) const { return all.min.z + k * CELL; }
    glm::vec3 at(int n) const { return {cx(nodes[n].i), nodes[n].y, cz(nodes[n].k)}; }
    int nearest(glm::vec3 p, float* dist = nullptr) const {
        int best = -1; float bd = 1e9f;
        for (int a = 0; a < (int)nodes.size(); ++a) { float d = glm::length(at(a) - p); if (d < bd) { bd = d; best = a; } }
        if (dist) *dist = bd;
        return best;
    }
};

inline Graph buildGraph(const LevelData& L, const std::vector<AABB>& zones, float voidY,
                        const std::vector<int>* skipWalls = nullptr, float reach = JUMP_REACH, float leap = 0.f) {
    std::vector<const AABB*> walls;
    for (int i = 0; i < (int)L.walls.size(); ++i) {
        if (L.walls[i].dynamic) continue;
        if (skipWalls && std::find(skipWalls->begin(), skipWalls->end(), i) != skipWalls->end()) continue;
        walls.push_back(&L.walls[i].box);
    }
    Graph G;
    AABB& all = G.all;
    for (auto& z : zones) { all.min = glm::min(all.min, z.min); all.max = glm::max(all.max, z.max); }
    const int nx = G.nx = (int)std::ceil((all.max.x - all.min.x) / CELL) + 1, nz = G.nz = (int)std::ceil((all.max.z - all.min.z) / CELL) + 1;
    auto cx = [&](int i) { return all.min.x + i * CELL; };
    auto cz = [&](int k) { return all.min.z + k * CELL; };
    auto ceilAt = [&](float x, float z) {   // the highest ceiling of a zone over (x, z); -1e9 outside them all
        float c = -1e9f;
        for (auto& zn : zones) if (x >= zn.min.x && x <= zn.max.x && z >= zn.min.z && z <= zn.max.z) c = std::max(c, zn.max.y);
        return c;
    };
    // Walls near each column, once
    std::vector<std::vector<const AABB*>> near((size_t)nx * nz);
    for (auto* w : walls) {
        int i0 = std::max(0, (int)std::floor((w->min.x - R - all.min.x) / CELL)), i1 = std::min(nx - 1, (int)std::ceil((w->max.x + R - all.min.x) / CELL));
        int k0 = std::max(0, (int)std::floor((w->min.z - R - all.min.z) / CELL)), k1 = std::min(nz - 1, (int)std::ceil((w->max.z + R - all.min.z) / CELL));
        for (int i = i0; i <= i1; ++i) for (int k = k0; k <= k1; ++k) near[(size_t)i * nz + k].push_back(w);
    }
    auto bodyFree = [&](int i, int k, float y) {   // the body standing at y in this column hits nothing
        float x = cx(i), z = cz(k);
        AABB b{{x - R + 0.02f, y + 0.02f, z - R + 0.02f}, {x + R - 0.02f, y + H, z + R - 0.02f}};
        for (auto* w : near[(size_t)i * nz + k]) if (overlaps(b, *w)) return false;
        return y + H <= ceilAt(x, z) + 1e-3f;
    };
    // Surfaces per column: the floor (if it isn't the void) and every box top the body overlaps
    auto& nodes = G.nodes;
    auto& col = G.col; col.assign((size_t)nx * nz, {});
    for (int i = 0; i < nx; ++i) for (int k = 0; k < nz; ++k) {
        float x = cx(i), z = cz(k);
        if (ceilAt(x, z) < -1e8f) continue;
        std::vector<float> ys;
        float fl = L.baseFloor(x, z);
        if (fl > voidY) ys.push_back(fl);
        for (auto* w : near[(size_t)i * nz + k])
            if (x + R > w->min.x && x - R < w->max.x && z + R > w->min.z && z - R < w->max.z) ys.push_back(w->max.y);
        std::sort(ys.begin(), ys.end());
        ys.erase(std::unique(ys.begin(), ys.end(), [](float a, float b) { return std::fabs(a - b) < 0.05f; }), ys.end());
        for (float y : ys) if (bodyFree(i, k, y)) {
            col[(size_t)i * nz + k].push_back((int)nodes.size());
            nodes.push_back({i, k, y});
        }
    }
    // Where you land dropping off a ledge into column (i, k) from height y: its highest surface at or under y
    auto landing = [&](int i, int k, float y) {
        int best = -1; float by = -1e9f;
        for (int n : col[(size_t)i * nz + k]) if (nodes[n].y <= y + 0.05f && nodes[n].y > by) { by = nodes[n].y; best = n; }
        return best;
    };
    // Edges, reversed: who can get to me
    auto& into = G.into; into.assign(nodes.size(), {});
    const int DI[8] = {1, -1, 0, 0, 1, 1, -1, -1}, DK[8] = {0, 0, 1, -1, 1, -1, 1, -1};
    for (int a = 0; a < (int)nodes.size(); ++a) {
        const Graph::Node& A = nodes[a];
        for (int d = 0; d < 8; ++d) {
            int i = A.i + DI[d], k = A.k + DK[d];
            if (i < 0 || k < 0 || i >= nx || k >= nz) continue;
            if (ceilAt(cx(i), cz(k)) < -1e8f) continue;   // the zone stops you
            // Step or drop: move over at your height, then fall to whatever is under you
            if (bodyFree(i, k, A.y)) {
                int b = landing(i, k, A.y);
                if (b >= 0) into[b].push_back(a);
                else if (L.baseFloor(cx(i), cz(k)) <= voidY) nodes[a].voidEdge = true;   // nothing under: the void
            }
            // Jump up onto a surface next door (room to rise in your own column, and to stand there)
            for (int b : col[(size_t)i * nz + k]) {
                const Graph::Node& B = nodes[b];
                if (B.y <= A.y + 0.05f || B.y > A.y + reach) continue;
                if (bodyFree(A.i, A.k, B.y)) into[b].push_back(a);   // (nothing over your head on the way up)
            }
        }
    }
    // Leaps (routes only, leap > 0): a running jump off an edge across a gap, up to `leap` metres, onto a
    // surface no higher than `reach` above you, the air above the gap clear of walls all the way
    if (leap > 0.f) {
        const int span = (int)std::ceil(leap / CELL);
        auto sameHeightNext = [&](const Graph::Node& A, int i, int k) {
            for (int n : col[(size_t)i * nz + k]) if (std::fabs(nodes[n].y - A.y) < 0.3f) return true;
            return false;
        };
        for (int a = 0; a < (int)nodes.size(); ++a) {
            const Graph::Node& A = nodes[a];
            bool edge = false;   // only from an edge: somewhere a neighbour isn't level with you
            for (int d = 0; d < 8 && !edge; ++d) {
                int i = A.i + DI[d], k = A.k + DK[d];
                if (i < 0 || k < 0 || i >= nx || k >= nz) continue;
                edge = !sameHeightNext(A, i, k);
            }
            if (!edge) continue;
            for (int di = -span; di <= span; ++di) for (int dk = -span; dk <= span; ++dk) {
                if (std::abs(di) <= 1 && std::abs(dk) <= 1) continue;
                if ((di * di + dk * dk) * CELL * CELL > leap * leap) continue;
                int i = A.i + di, k = A.k + dk;
                if (i < 0 || k < 0 || i >= nx || k >= nz) continue;
                for (int b : col[(size_t)i * nz + k]) {
                    const Graph::Node& B = nodes[b];
                    if (B.y > A.y + reach || B.y < A.y - 12.f) continue;
                    // The arc's top: over the higher end, with a little room, clear at every column on the way
                    float top = std::max(A.y, B.y) + 0.3f;
                    bool clear = bodyFree(A.i, A.k, top);
                    int steps = std::max(std::abs(di), std::abs(dk));
                    for (int t = 1; t <= steps && clear; ++t) {
                        int ci = A.i + (int)std::lround((float)di * t / steps), ck = A.k + (int)std::lround((float)dk * t / steps);
                        clear = bodyFree(ci, ck, top);
                    }
                    if (clear) into[b].push_back(a);
                }
            }
        }
    }
    return G;
}

// zones: where the player may be (Arena::zone + extraZones); start: where "back" is
// homes: the arena's interior (Arena::bounds, and a corridor's): standing anywhere in one (XZ) counts as back
inline Result scan(const LevelData& L, const std::vector<AABB>& zones, glm::vec3 start, float voidY,
                   const std::vector<AABB>& homes, const std::vector<int>* skipWalls = nullptr) {
    Graph G = buildGraph(L, zones, voidY, skipWalls);
    auto& nodes = G.nodes;
    auto cx = [&](int i) { return G.cx(i); };
    auto cz = [&](int k) { return G.cz(k); };
    float startMiss = 1e9f;
    {   // the start: the surface nearest the arena's start point (it may stand on a box or in water)
        int best = G.nearest(start, &startMiss);
        if (best >= 0) nodes[best].start = true;
    }
    // Back from anywhere that reaches the start or the void
    std::vector<char> ok(nodes.size(), 0);
    std::deque<int> q;
    auto inHome = [&](const Graph::Node& n) {
        float x = cx(n.i), z = cz(n.k);
        for (auto& home : homes)
            if (x > home.min.x + 0.5f && x < home.max.x - 0.5f && z > home.min.z + 0.5f && z < home.max.z - 0.5f && n.y < home.max.y) return true;
        return false;
    };
    for (int a = 0; a < (int)nodes.size(); ++a) if (nodes[a].start || nodes[a].voidEdge || inHome(nodes[a])) { ok[a] = 1; q.push_back(a); }
    // into[b] lists who can reach b; ok spreads to them
    while (!q.empty()) {
        int b = q.front(); q.pop_front();
        for (int a : G.into[b]) if (!ok[a]) { ok[a] = 1; q.push_back(a); }
    }
    Result r; r.nodes = (int)nodes.size(); r.startMiss = startMiss;
    for (int a = 0; a < (int)nodes.size(); ++a) if (!inHome(nodes[a])) {
        ++r.outsideHome;
        glm::vec3 p = G.at(a);
        r.outBox.min = glm::min(r.outBox.min, p); r.outBox.max = glm::max(r.outBox.max, p);
    }
    for (int a = 0; a < (int)nodes.size(); ++a) {
        if (ok[a]) continue;
        ++r.traps;
        glm::vec3 p = G.at(a);
        r.all.push_back({p});
        r.trapBox.min = glm::min(r.trapBox.min, p); r.trapBox.max = glm::max(r.trapBox.max, p);
        if (r.samples.size() < 6 && (r.samples.empty() || glm::length(r.samples.back().at - p) > 3.f)) r.samples.push_back({p});
    }
    return r;
}

// Everywhere you can get to from `from` (walking, dropping, jumping), for routes: out[n] set if reached
inline std::vector<char> reachFrom(const Graph& G, int from) {
    std::vector<std::vector<int>> out(G.nodes.size());
    for (int b = 0; b < (int)G.into.size(); ++b) for (int a : G.into[b]) out[a].push_back(b);
    std::vector<char> seen(G.nodes.size(), 0);
    if (from < 0) return seen;
    std::deque<int> q{from}; seen[from] = 1;
    while (!q.empty()) { int a = q.front(); q.pop_front(); for (int b : out[a]) if (!seen[b]) { seen[b] = 1; q.push_back(b); } }
    return seen;
}

// An arena's zones (where the player may be) and the level's door halves (left out: they open)
inline std::vector<AABB> zonesOf(const Arena& a) {
    std::vector<AABB> z{a.zone};
    z.insert(z.end(), a.extraZones.begin(), a.extraZones.end());
    return z;
}
inline std::vector<int> doorWalls(const LevelData& L) {
    std::vector<int> d;
    for (auto& door : L.doors) { if (door.wall >= 0) d.push_back(door.wall); if (door.wall2 >= 0) d.push_back(door.wall2); }
    return d;
}

}  // namespace bscan
