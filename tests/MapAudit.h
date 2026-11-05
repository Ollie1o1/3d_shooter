#pragma once
// =============================================================================
// MapAudit.h — what a section is like to play, in numbers, so maps can be held
// to a standard (docs/gameplay-standard.md) instead of judged by eye alone.
// For one section (its zone and extra zones), from the route graph
// (BoundsScan.h, with leaps):
//   floor     m² you can stand on
//   levels    standing layers of 20 m² or more (verticality)
//   cover     share of the floor within 2.5 m of something you can hide behind
//   sight     mean open distance at eye height (8 directions, to 60 m)
//   enemies   bodies across its waves, per 100 m² of floor, and kinds
//   detour    walked route to the next section's start / the straight line
//   dressing  props, models and shapes per 100 m²; lights per 100 m²
// No OpenGL. `make audit` prints every mode's sections side by side.
// =============================================================================
#include "BoundsScan.h"
#include "RouteScan.h"
#include "core/Collision.h"
#include <set>
#include <string>

namespace audit {

struct Section {
    const char* name = "";
    float floor = 0.f;
    int   levels = 0;
    float rise = 0.f;          // 10th to 90th percentile of standing height
    float cover = 0.f;         // 0..1
    float sight = 0.f;         // metres
    int   enemies = 0, kinds = 0, waves = 0;
    float density = 0.f;       // enemies per 100 m²
    float detour = 0.f;        // 0: no next section
    float dressing = 0.f, lights = 0.f;
    bool  indoor = false;      // a ceiling within 40 m: lit by its own lights, not the sky
};

inline bool inZones(const std::vector<AABB>& zones, glm::vec3 p) {
    for (const auto& z : zones)
        if (p.x >= z.min.x && p.x <= z.max.x && p.z >= z.min.z && p.z <= z.max.z && p.y >= z.min.y - 0.5f && p.y <= z.max.y) return true;
    return false;
}

inline Section measure(const LevelData& L, int a) {
    const Arena& A = L.arenas[a];
    Section S; S.name = A.name;
    S.indoor = A.zone.max.y - A.playerStart.y < 40.f;
    const std::vector<AABB> zones = bscan::zonesOf(A);
    std::vector<int> doors = bscan::doorWalls(L);
    bscan::Graph G = bscan::buildGraph(L, zones, A.voidY, &doors, bscan::JUMP_REACH, rscan::LEAP);
    if (G.nodes.empty()) return S;
    const float cellA = bscan::CELL * bscan::CELL;

    // Floor and layers: each column's surfaces; heights binned by half metres
    std::set<long long> cols;
    std::vector<float> ys;
    std::map<int, float> layer;   // half-metre bin -> m²
    for (const auto& n : G.nodes) {
        cols.insert((long long)n.i * G.nz + n.k);
        ys.push_back(n.y);
        layer[(int)std::floor(n.y * 2.f)] += cellA;
    }
    S.floor = (float)G.nodes.size() * cellA;
    std::sort(ys.begin(), ys.end());
    S.rise = ys[ys.size() * 9 / 10] - ys[ys.size() / 10];
    {   // layers: runs of bins of 20 m² or more, a gap between them
        int prev = -1000;
        for (const auto& [bin, m2] : layer) if (m2 >= 20.f) { if (bin > prev + 1) ++S.levels; prev = bin; }
    }

    // Cover and sightlines: sampled every 4th node
    SpatialGrid grid; grid.build(L.walls);
    std::vector<int> cands;
    int samples = 0, covered = 0; double sightSum = 0.0;
    const int step = std::max(1, (int)G.nodes.size() / 1500);
    for (int idx = 0; idx < (int)G.nodes.size(); idx += step) {
        const glm::vec3 p = G.at(idx);
        ++samples;
        // Cover: a solid within 2.5 m that rises at least 1 m above your feet and starts below your waist
        AABB near{p - glm::vec3{2.5f, 0.f, 2.5f}, p + glm::vec3{2.5f, 2.f, 2.5f}};
        grid.query(near, cands);
        bool c = false;
        for (int w : cands) {
            const AABB& b = L.walls[w].box;
            if (L.walls[w].dynamic) continue;
            if (b.max.y < p.y + 1.f || b.min.y > p.y + 0.6f) continue;
            if (b.max.x < near.min.x || b.min.x > near.max.x || b.max.z < near.min.z || b.min.z > near.max.z) continue;
            if (p.x > b.min.x && p.x < b.max.x && p.z > b.min.z && p.z < b.max.z) continue;   // (the thing you stand on)
            c = true; break;
        }
        covered += c;
        // Sight: eight rays at eye height, to the first wall or 60 m (or the zone's edge)
        const glm::vec3 eye = p + glm::vec3{0.f, 1.6f, 0.f};
        for (int d = 0; d < 8; ++d) {
            const float ang = d * 0.785398f;
            const glm::vec3 dir{std::cos(ang), 0.f, std::sin(ang)};
            float t = 60.f;
            AABB ray{glm::min(eye, eye + dir * 60.f) - glm::vec3{0.1f}, glm::max(eye, eye + dir * 60.f) + glm::vec3{0.1f}};
            grid.query(ray, cands);
            for (int w : cands) { float h = rayBoxHit(eye, dir, L.walls[w].box); if (h > 0.f && h < t) t = h; }
            for (const auto& z : zones) {   // the zone's wall counts as the end of the view
                float ex = dir.x > 0 ? (z.max.x - eye.x) / dir.x : dir.x < 0 ? (z.min.x - eye.x) / dir.x : 1e9f;
                float ez = dir.z > 0 ? (z.max.z - eye.z) / dir.z : dir.z < 0 ? (z.min.z - eye.z) / dir.z : 1e9f;
                if (eye.x >= z.min.x && eye.x <= z.max.x && eye.z >= z.min.z && eye.z <= z.max.z) t = std::min(t, std::max(0.f, std::min(ex, ez)));
            }
            sightSum += t;
        }
    }
    S.cover = samples ? (float)covered / samples : 0.f;
    S.sight = samples ? (float)(sightSum / (samples * 8.0)) : 0.f;

    // Enemies across its waves
    std::set<int> kinds;
    for (const auto& wave : A.waves) {
        ++S.waves;
        for (const auto& e : wave) {
            S.enemies += std::max({1, e.count, (int)e.at.size()}) + (int)e.escort.size();   // (story waves list their spawn points)
            kinds.insert((int)e.type);
            for (auto t : e.escort) kinds.insert((int)t);
        }
    }
    S.kinds = (int)kinds.size();
    S.density = S.floor > 0.f ? S.enemies * 100.f / S.floor : 0.f;

    // The route on: walked distance from this start to the next section's, against the straight line
    if (a + 1 < (int)L.arenas.size()) {
        std::vector<AABB> both = zones; auto zb = bscan::zonesOf(L.arenas[a + 1]); both.insert(both.end(), zb.begin(), zb.end());
        bscan::Graph R = bscan::buildGraph(L, both, std::min(A.voidY, L.arenas[a + 1].voidY), &doors, bscan::JUMP_REACH, rscan::LEAP);
        int s = R.nearest(A.playerStart), t = R.nearest(L.arenas[a + 1].playerStart);
        if (s >= 0 && t >= 0) {
            std::vector<std::vector<int>> out(R.nodes.size());
            for (int b = 0; b < (int)R.into.size(); ++b) for (int x : R.into[b]) out[x].push_back(b);
            std::vector<float> dist(R.nodes.size(), 1e9f);
            std::deque<int> q{s}; dist[s] = 0.f;   // (breadth first over a grid: near enough to the shortest walk)
            while (!q.empty()) {
                int x = q.front(); q.pop_front();
                for (int b : out[x]) {
                    float d = dist[x] + glm::length(R.at(b) - R.at(x));
                    if (d < dist[b] - 1e-3f) { dist[b] = d; q.push_back(b); }
                }
            }
            const float straight = glm::length(L.arenas[a + 1].playerStart - A.playerStart);
            if (dist[t] < 1e8f && straight > 1.f) S.detour = dist[t] / straight;
        }
    }

    // Dressing and light inside it
    auto inside = [&](const AABB& b) { return inZones(zones, (b.min + b.max) * 0.5f); };
    int dressing = 0, lights = 0;
    for (const auto& p : L.props) dressing += inside(p.box);
    for (const auto& m : L.models) dressing += inside(m.fit) ? 4 : 0;   // (a Blender model is worth a few boxes)
    for (const auto& sh : L.shapes) dressing += inZones(zones, glm::vec3(sh.xf[3])) ? 1 : 0;
    for (const auto& n : L.neon) lights += inside(n.box);
    S.dressing = S.floor > 0.f ? dressing * 100.f / S.floor : 0.f;
    S.lights = S.floor > 0.f ? lights * 100.f / S.floor : 0.f;
    return S;
}

// The standard (docs/gameplay-standard.md): what a fight section should measure. Act I's arenas, the
// most-tuned spaces in the game, set most of these; the Thaw Room meets all of them.
struct Targets {
    static constexpr float COVER_MIN = 0.25f, COVER_MAX = 0.50f;   // somewhere to duck within a dash, not a maze
    static constexpr int   LEVELS_MIN = 3;                          // floor, a step up, a high ground
    static constexpr float SIGHT_MIN = 12.f, SIGHT_MAX = 30.f;      // room to shoot, not a firing range
    static constexpr float LIGHTS_MIN = 0.5f;                       // light that says where things are (per 100 m²)
    static constexpr float DRESSING_MIN = 5.f;                      // something to read the space by (per 100 m²)
    static constexpr float DETOUR_MAX = 1.5f;                       // the way on is plain
};
// What a section misses, as short words ("" for none). Fights (3+ enemies) are held to all of it; quiet sections,
// bosses and duels to the way on, light (indoors) and dressing.
inline std::string misses(const Section& s) {
    std::string m;
    auto add = [&](const char* w) { if (!m.empty()) m += ' '; m += w; };
    const bool fight = s.enemies >= 3;   // (a boss or a duel alone keeps its own open ground)
    if (fight && s.cover < Targets::COVER_MIN) add("cover-");
    if (fight && s.cover > Targets::COVER_MAX) add("cover+");
    if (fight && s.levels < Targets::LEVELS_MIN) add("levels-");
    if (fight && s.sight > Targets::SIGHT_MAX) add("open");
    if (fight && s.sight < Targets::SIGHT_MIN) add("cramped");
    if (s.indoor && s.lights < Targets::LIGHTS_MIN) add("dark");
    if (s.dressing < Targets::DRESSING_MIN) add("bare");
    if (s.detour > Targets::DETOUR_MAX) add("winding");
    return m;
}

}  // namespace audit
