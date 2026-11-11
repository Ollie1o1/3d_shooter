#pragma once
// =============================================================================
// NavGrid.h — where a walker can stand in one arena, and the way to the player
// from anywhere in it. No OpenGL and no SDL (the tests build it on their own).
//
// The grid: the arena's interior cut into columns (0.8 m; larger in the widest
// rooms). Each column holds a node for every surface a body fits standing on:
// the hard floor (ramps and basins included) and every box top. Nodes are
// joined to the eight columns around them the way an enemy really moves
// (Enemy::integrate): level ground, up a ramp, down a step, off a ledge. There
// is no step-up and no jump, because the enemies have neither.
//
// The way: one flow field per kind of walker (wide or tall bodies need more
// room; ledge-aware ones refuse drops over 1.4 m), a Dijkstra pass outward from
// the player's node over the reversed links. Every chaser of that kind reads
// the same field, so a full wave costs the same as one enemy. A field is
// rebuilt only when the player has moved to another node, at most every 0.2 s,
// one field per tick. Three variants of each field price the ground a little
// differently, so a squad splits across routes that are nearly as short
// instead of filing down one.
//
// Enemies still chase directly when the straight line is walkable, and feeler
// steering (Enemy::steer) stays on top as the local layer. Flyers never use it.
// =============================================================================
#include "core/Collision.h"
#include <cstdint>
#include <queue>
#include <vector>

struct NavAgent {
    bool wide = false;    // a body wider than a doorway-sized one (radius over 0.62)
    bool tall = false;    // taller than 2.3 m
    bool drops = false;   // steps off ledges higher than 1.4 m
    int  variant = 0;     // which pricing of the ground (0: the plain shortest way)
};

struct NavStep {
    bool valid = false;        // false: no way from here (or no grid): steer as before
    glm::vec3 dir{0.f};        // flat, unit: the way to walk
    float left = 0.f;          // metres of route left to the player's node
    bool drop = false;         // this step is the route walking off a ledge on purpose
};

struct NavGrid {
    static constexpr int   VARIANTS = 3;
    static constexpr float SMALL_R = 0.6f, WIDE_R = 1.05f;   // body half-widths the grid is cut for
    static constexpr float LOW_H = 1.9f, TALL_H = 3.2f;     // ...and heights
    static constexpr float LIP = 0.06f;        // a rise this small is pushed up over (resolveAABB)
    static constexpr float STEP = 0.35f;       // a fall this small is just walking
    static constexpr float LEDGE = 1.4f;       // ledge-aware walkers step down this far (Enemy::supportedAt)
    static constexpr float RISE = 0.5f;        // the hard floor may climb this much per quarter cell (a ramp, not a cliff)
    static constexpr float REFRESH = 0.2f;     // a field follows the player at most this often
    static constexpr int   MAX_COLUMNS = 110000;
    static constexpr float MAX_CELL = 1.6f;    // coarser than this the routes mean little: the arena goes without

    enum : uint8_t { BASE = 1, FIT_TALL = 2, FIT_WIDE = 4, FIT_WIDE_TALL = 8, OPEN = 16, BURNS = 32 };
    struct Node {
        float y; int col;
        int nb[8];              // the node each direction leads to, or -1
        uint8_t flags = 0;
        uint8_t big = 0;        // bit d: that way is a drop of more than LEDGE
    };
    static constexpr int DI[8] = {1, -1, 0, 0, 1, 1, -1, -1};
    static constexpr int DK[8] = {0, 0, 1, -1, 1, -1, 1, -1};
    static constexpr int OPP[8] = {1, 0, 3, 2, 7, 6, 5, 4};

    AABB area{{0, 0, 0}, {0, 0, 0}};
    float cell = 0.8f;
    int nx = 0, nz = 0;
    bool built = false;
    unsigned revision = 0;            // bumped by every build
    double buildMs = 0.;              // (filled by whoever times it)
    std::vector<Node> nodes;
    std::vector<int> colFirst;        // column -> its first node
    std::vector<uint8_t> colCount;
    std::vector<int> revFirst;        // node b -> the links into it
    struct Rev { int from; uint8_t d; };
    std::vector<Rev> rev;

    float cx(int i) const { return area.min.x + ((float)i + 0.5f) * cell; }
    float cz(int k) const { return area.min.z + ((float)k + 0.5f) * cell; }
    int ci(float x) const { return (int)std::floor((x - area.min.x) / cell); }
    int ck(float z) const { return (int)std::floor((z - area.min.z) / cell); }
    bool inGrid(int i, int k) const { return i >= 0 && k >= 0 && i < nx && k < nz; }
    glm::vec3 at(int n) const { const Node& N = nodes[n]; return {cx(N.col / nz), N.y, cz(N.col % nz)}; }

    void clear() { built = false; nodes.clear(); colFirst.clear(); colCount.clear(); rev.clear(); revFirst.clear(); nx = nz = 0; goal_ = -1; ++revision; }

    // Cut the grid for `bounds` out of the static walls and the hard floor.
    // baseFloor(x, z): the floor's height there; at or under voidY it's no floor at all.
    // burns: boxes that hurt whoever stands in them (lava); routes cross them only when it saves a long way round.
    template <class FloorFn>
    bool build(const Wall* walls, int wallCount, const SpatialGrid* grid, const AABB& bounds, float voidY, FloorFn&& baseFloor,
               const AABB* burns = nullptr, int burnCount = 0) {
        clear();
        area = bounds;
        const float ex = bounds.max.x - bounds.min.x, ez = bounds.max.z - bounds.min.z;
        if (!(ex > 1.f) || !(ez > 1.f) || ex > 1e6f || ez > 1e6f) return false;
        cell = 0.8f;
        while ((ex / cell) * (ez / cell) > (float)MAX_COLUMNS) cell *= 1.15f;
        if (cell > MAX_CELL) return false;
        nx = std::max(1, (int)std::ceil(ex / cell)); nz = std::max(1, (int)std::ceil(ez / cell));
        const int cols = nx * nz;

        // The walls near each column (within a wide body of its centre)
        std::vector<int> candFirst((size_t)cols + 1, 0), cand, q;
        cand.reserve((size_t)cols * 4);
        for (int i = 0; i < nx; ++i) for (int k = 0; k < nz; ++k) {
            const float x = cx(i), z = cz(k);
            candFirst[(size_t)i * nz + k] = (int)cand.size();
            const AABB box{{x - WIDE_R, -1e9f, z - WIDE_R}, {x + WIDE_R, 1e9f, z + WIDE_R}};
            if (grid) grid->query(box, q);
            else { q.clear(); for (int w = 0; w < wallCount; ++w) q.push_back(w); }
            for (int w : q) {
                const Wall& W = walls[w];
                if (W.dynamic) continue;                                   // movers: not ground you can count on
                const AABB& b = W.box;
                if (b.max.y - b.min.y < 0.02f) continue;                   // (broken cover is a sliver far below)
                if (b.max.y < bounds.min.y - 4.f || b.min.y > bounds.max.y + 4.f) continue;   // another storey's
                if (box.max.x > b.min.x && box.min.x < b.max.x && box.max.z > b.min.z && box.min.z < b.max.z) cand.push_back(w);
            }
        }
        candFirst[cols] = (int)cand.size();
        auto freeAt = [&](int c, float y, float r, float h) {
            const float x = cx(c / nz), z = cz(c % nz);
            for (int j = candFirst[c]; j < candFirst[c + 1]; ++j) {
                const AABB& b = walls[cand[j]].box;
                if (x + r > b.min.x && x - r < b.max.x && z + r > b.min.z && z - r < b.max.z &&
                    y + h > b.min.y && y + LIP + 0.01f < b.max.y) return false;   // (the body Enemy::resolveAABB pushes out: a 0.3 m kerb stops it)
            }
            return true;
        };

        // Nodes: the surfaces in each column a body fits on
        colFirst.assign(cols, 0); colCount.assign(cols, 0);
        struct Surf { float y; bool base; };
        std::vector<Surf> ys;
        for (int c = 0; c < cols; ++c) {
            const float x = cx(c / nz), z = cz(c % nz);
            colFirst[c] = (int)nodes.size();
            ys.clear();
            const float fl = baseFloor(x, z);
            const float yLo = bounds.min.y - 1.5f, yHi = bounds.max.y;   // the arena's own storey: not the roof, not the room below
            if (fl > voidY && fl >= yLo && fl <= yHi) ys.push_back({fl, true});
            for (int j = candFirst[c]; j < candFirst[c + 1]; ++j) {
                const AABB& b = walls[cand[j]].box;
                if (x > b.min.x && x < b.max.x && z > b.min.z && z < b.max.z && b.max.y > voidY && b.max.y >= yLo && b.max.y <= yHi)
                    ys.push_back({b.max.y, false});
            }
            // (enemies are held a body's width inside the bounds: a column hard against the edge can't be stood in)
            if (x < bounds.min.x + 0.55f || x > bounds.max.x - 0.55f || z < bounds.min.z + 0.55f || z > bounds.max.z - 0.55f) ys.clear();
            // Nothing stands under the hard floor: it lifts whatever is there (a box top inside a ramp is no place)
            const bool hasFloor = fl > voidY && fl > -1e8f;
            if (hasFloor) ys.erase(std::remove_if(ys.begin(), ys.end(), [&](const Surf& s2) { return !s2.base && s2.y < fl - 0.05f; }), ys.end());
            std::sort(ys.begin(), ys.end(), [](const Surf& a, const Surf& b) { return a.y < b.y; });
            for (size_t s = 0; s < ys.size(); ++s) {
                if (s + 1 < ys.size() && ys[s + 1].y - ys[s].y < STEP) {   // buried just under the next: that one is the ground
                    ys[s + 1].base = ys[s + 1].base || ys[s].base;          // (a landing flush with its ramp's top is still the hard floor)
                    continue;
                }
                if (colCount[c] >= 12) break;
                const float y = ys[s].y;
                if (!freeAt(c, y, SMALL_R, LOW_H)) continue;
                Node n; n.y = y; n.col = c;
                for (int& v : n.nb) v = -1;
                n.flags = ys[s].base ? BASE : 0;
                if (freeAt(c, y, SMALL_R, TALL_H)) n.flags |= FIT_TALL;
                if (freeAt(c, y, WIDE_R, LOW_H)) {
                    n.flags |= FIT_WIDE;
                    if (freeAt(c, y, WIDE_R, TALL_H)) n.flags |= FIT_WIDE_TALL;
                }
                for (int h = 0; h < burnCount; ++h) {
                    const AABB& b = burns[h];
                    if (x > b.min.x && x < b.max.x && z > b.min.z && z < b.max.z && y < b.max.y + 0.3f && y > b.min.y - 0.5f) n.flags |= BURNS;
                }
                nodes.push_back(n); ++colCount[c];
            }
        }

        // Links, the four straight ways first
        for (int a = 0; a < (int)nodes.size(); ++a) {
            Node& A = nodes[a];
            const int i = A.col / nz, k = A.col % nz;
            const float ax = cx(i), az = cz(k);
            for (int d = 0; d < 4; ++d) {
                const int i2 = i + DI[d], k2 = k + DK[d];
                if (!inGrid(i2, k2)) continue;
                const int c2 = i2 * nz + k2;
                if (!colCount[c2]) continue;
                // The hard floor lifts whatever walks onto it: how high it carries you on the way over
                float reach = A.y;
                for (int s = 1; s <= 4; ++s) {
                    const float t = (float)s * 0.25f;
                    const float f = baseFloor(ax + (float)DI[d] * cell * t, az + (float)DK[d] * cell * t);
                    if (f <= reach) continue;
                    if (f - reach > RISE) break;
                    reach = f;
                }
                int b = -1;   // where you end up: the highest surface there at or under you
                for (int j = colFirst[c2]; j < colFirst[c2] + colCount[c2]; ++j)
                    if (nodes[j].y <= reach + LIP && (b < 0 || nodes[j].y > nodes[b].y)) b = j;
                if (b < 0) continue;
                const float fall = A.y - nodes[b].y;
                if (fall > STEP) {
                    if (!freeAt(c2, A.y, SMALL_R, LOW_H)) continue;   // a wall at your height: no way over the edge here
                    const int i3 = i2 + DI[d], k3 = k2 + DK[d];       // ...and room to walk out past it (feelers look this far ahead)
                    if (inGrid(i3, k3) && !freeAt(i3 * nz + k3, A.y, SMALL_R, LOW_H)) continue;
                    if (fall > LEDGE) A.big |= (uint8_t)(1u << d);
                }
                A.nb[d] = b;
            }
        }
        // ...then the diagonals: only across ground that's level and open on both sides (no corner cutting)
        auto dirOf = [](int di, int dk) { for (int d = 0; d < 4; ++d) if (DI[d] == di && DK[d] == dk) return d; return 0; };
        for (int a = 0; a < (int)nodes.size(); ++a) {
            Node& A = nodes[a];
            for (int d = 4; d < 8; ++d) {
                const int dx = dirOf(DI[d], 0), dz = dirOf(0, DK[d]);
                const int n1 = A.nb[dx], n2 = A.nb[dz];
                if (n1 < 0 || n2 < 0) continue;
                if (std::fabs(nodes[n1].y - A.y) > STEP || std::fabs(nodes[n2].y - A.y) > STEP) continue;
                const int b1 = nodes[n1].nb[dz], b2 = nodes[n2].nb[dx];
                if (b1 < 0 || b1 != b2 || std::fabs(nodes[b1].y - A.y) > 2.f * STEP) continue;
                A.nb[d] = b1;
            }
        }
        for (Node& A : nodes) {
            bool open = true;
            for (int d = 0; d < 8 && open; ++d) open = A.nb[d] >= 0 && std::fabs(nodes[A.nb[d]].y - A.y) <= STEP;
            if (open) A.flags |= OPEN;
        }
        // The links turned round, for the fields
        revFirst.assign(nodes.size() + 1, 0);
        for (const Node& A : nodes) for (int d = 0; d < 8; ++d) if (A.nb[d] >= 0) ++revFirst[(size_t)A.nb[d] + 1];
        for (size_t n = 0; n < nodes.size(); ++n) revFirst[n + 1] += revFirst[n];
        rev.resize(revFirst[nodes.size()]);
        std::vector<int> fill(revFirst.begin(), revFirst.end() - 1);
        for (int a = 0; a < (int)nodes.size(); ++a) for (int d = 0; d < 8; ++d) {
            const int b = nodes[a].nb[d];
            if (b >= 0) rev[fill[b]++] = {a, (uint8_t)d};
        }
        for (Field& f : fields_) { f.goal = -1; f.revision = 0; }
        built = !nodes.empty();
        return built;
    }

    // ---- who fits where -------------------------------------------------------
    static bool fits(const Node& n, const NavAgent& a) {
        if (a.wide && a.tall) return n.flags & FIT_WIDE_TALL;
        if (a.wide) return n.flags & FIT_WIDE;
        if (a.tall) return n.flags & FIT_TALL;
        return true;
    }
    bool mayGo(int a, int d, const NavAgent& ag) const {
        const Node& A = nodes[a];
        return A.nb[d] >= 0 && fits(nodes[A.nb[d]], ag) && (ag.drops || !(A.big & (1u << d)));
    }

    // The node under a body at p: its column's highest surface at or just under its feet;
    // off the grid (shoved into a blocked column, mid-fall past an edge): the nearest one around it
    int nodeAt(glm::vec3 p, float above = 0.6f) const {
        if (!built) return -1;
        const int i = ci(p.x), k = ck(p.z);
        auto inCol = [&](int ii, int kk) {
            if (!inGrid(ii, kk)) return -1;
            const int c = ii * nz + kk; int best = -1;
            for (int j = colFirst[c]; j < colFirst[c] + colCount[c]; ++j)
                if (nodes[j].y <= p.y + above && (best < 0 || nodes[j].y > nodes[best].y)) best = j;
            return best;
        };
        int n = inCol(i, k);
        if (n >= 0 && p.y - nodes[n].y <= 2.f) return n;
        // (far under its feet: it stands on something with no node of its own, a narrow top; a node beside it at its height is truer)
        const int under = n; n = -1;
        float bd = under >= 0 ? (p.y - nodes[under].y) * 0.5f : 1e9f;
        for (int r = 1; r <= 3 && n < 0; ++r)
            for (int di = -r; di <= r; ++di) for (int dk = -r; dk <= r; ++dk) {
                if (std::max(std::abs(di), std::abs(dk)) != r) continue;
                const int m = inCol(i + di, k + dk);
                if (m < 0) continue;
                const glm::vec3 q = at(m);
                const float dist = glm::length(glm::vec2{q.x - p.x, q.z - p.z}) + std::fabs(q.y - p.y) * 0.5f;
                if (dist < bd) { bd = dist; n = m; }
            }
        return n >= 0 ? n : under;
    }

    // ---- the player ------------------------------------------------------------
    void tick(float dt, glm::vec3 playerFeet) {
        clock_ += dt; budget_ = 1;
        if (!built) return;
        int g = nodeAt(playerFeet);
        if (g < 0) g = nodeAt(playerFeet, 3.f);   // (feet sunk a little into something: the ground just over them)
        if (g >= 0) goal_ = g;   // (in the air over nothing: where they last stood)
    }
    int goal() const { return goal_; }

    // Does the straight line a -> b run over joined ground all the way (for this walker)?
    bool lineWalkable(int a, int b, const NavAgent& ag, float maxDist = 36.f) const {
        if (a < 0 || b < 0) return false;
        if (a == b) return true;
        const glm::vec3 A = at(a), B = at(b);
        const float len = glm::length(glm::vec2{B.x - A.x, B.z - A.z});
        if (len > maxDist) return false;
        const int steps = std::max(1, (int)std::ceil(len / (cell * 0.45f)));
        int cur = a, ci0 = nodes[a].col / nz, ck0 = nodes[a].col % nz;
        for (int s = 1; s <= steps; ++s) {
            const float t = (float)s / (float)steps;
            const int i = ci(A.x + (B.x - A.x) * t), k = ck(A.z + (B.z - A.z) * t);
            if (i == ci0 && k == ck0) continue;
            int d = -1;
            for (int e = 0; e < 8; ++e) if (ci0 + DI[e] == i && ck0 + DK[e] == k) { d = e; break; }
            if (d < 0 || !mayGo(cur, d, ag)) return false;
            if (nodes[cur].y - nodes[nodes[cur].nb[d]].y > STEP) return false;   // a drop is the route's business, not a straight chase's
            cur = nodes[cur].nb[d]; ci0 = i; ck0 = k;
        }
        return cur == b;
    }
    // Is the player straight ahead over open ground?
    bool direct(const NavAgent& ag, glm::vec3 pos) const {
        return built && goal_ >= 0 && lineWalkable(nodeAt(pos), goal_, ag);
    }

    // The way to walk from pos (see the header). Looks a few nodes down the
    // route and heads for the farthest it can walk straight to, so a path is
    // corners, not a staircase of cells.
    NavStep step(const NavAgent& ag, glm::vec3 pos) const {
        NavStep out;
        if (!built || goal_ < 0) return out;
        const int n = nodeAt(pos);
        if (n < 0) return out;
        const Field& f = field(ag);
        if (f.dist.empty()) return out;
        if (f.dist[n] >= 1e8f) {
            // Standing somewhere the grid has no way from (a wide body squeezed into a narrow spot, a kerb's
            // top): step to the ground beside it that does lead on
            int best = -1;
            for (int d = 0; d < 8; ++d) {
                const int m = nodes[n].nb[d];
                if (m >= 0 && f.dist[m] < 1e8f && (best < 0 || f.dist[m] < f.dist[best])) best = m;
            }
            if (best < 0) return out;
            glm::vec3 to = at(best) - pos; to.y = 0.f;
            const float len = glm::length(to);
            if (len < 1e-3f) return out;
            out.dir = to / len; out.left = f.dist[best]; out.valid = true;
            return out;
        }
        out.left = f.dist[n];
        {   // The next link is a drop: walk off the edge that way and keep going (a body is held up until it is
            // a whole radius past the lip, so aiming at the column just over it would stall on the brink)
            const int d0 = f.next[n];
            if (d0 >= 0 && nodes[n].nb[d0] >= 0 && nodes[n].y - nodes[nodes[n].nb[d0]].y > STEP && pos.y > nodes[n].y - 0.3f) {
                out.dir = glm::normalize(glm::vec3{(float)DI[d0], 0.f, (float)DK[d0]});
                out.valid = true; out.drop = true;
                return out;
            }
        }
        int target = n, cur = n;
        for (int s = 0; s < 7; ++s) {
            const int d = f.next[cur];
            if (d < 0) break;
            const int nx2 = nodes[cur].nb[d];
            if (nx2 < 0) break;
            cur = nx2;
            if (s == 0 || lineWalkable(n, cur, ag, 12.f)) target = cur;
            else break;
        }
        glm::vec3 to = at(target) - pos; to.y = 0.f;
        const float len = glm::length(to);
        if (target == n || len < 0.05f) {   // on the last node: straight at where the field ends
            to = at(f.goal) - pos; to.y = 0.f;
            const float l2 = glm::length(to);
            if (l2 < 1e-3f) return out;
            out.dir = to / l2; out.valid = true;
            return out;
        }
        out.dir = to / len; out.valid = true;
        return out;
    }

    // Would walking `reach` metres along dir from pos take this walker off an edge, or up onto a ramp from
    // its side, to somewhere farther from the player (or with no way back)? A route along a bank is not a
    // reason to fall off it.
    bool strays(const NavAgent& ag, glm::vec3 pos, glm::vec3 dir, float reach) const {
        return straysAt(ag, pos, dir, reach) || straysAt(ag, pos, dir, 0.35f);   // (ahead of the body, and the very next step)
    }
    bool straysAt(const NavAgent& ag, glm::vec3 pos, glm::vec3 dir, float reach) const {
        if (!built || goal_ < 0) return false;
        const int a = nodeAt(pos);
        if (a < 0) return false;
        const glm::vec3 q = pos + dir * reach;
        int b = nodeAt(q, 0.6f);
        // (the hard floor lifts whatever walks onto it: stepping sideways into a ramp's footprint puts you up on the ramp)
        const int i = ci(q.x), k = ck(q.z);
        if (inGrid(i, k)) {
            const int c = i * nz + k;
            for (int j = colFirst[c]; j < colFirst[c] + colCount[c]; ++j)
                if ((nodes[j].flags & BASE) && nodes[j].y > pos.y + STEP) b = j;
        }
        if (b < 0 || b == a || std::fabs(nodes[a].y - nodes[b].y) <= STEP) return false;
        const Field& f = field(ag);
        if (f.dist.empty()) return false;
        return f.dist[b] > f.dist[a] + 0.5f;
    }

    // A body holds on to a ledge until it is a whole radius past the lip. Is this one hanging over an edge
    // that way (its centre already past it), over ground that leads away from the player? Then: the node
    // to pull back toward, else -1.
    int hanging(const NavAgent& ag, glm::vec3 pos) const {
        if (!built || goal_ < 0) return -1;
        const int i = ci(pos.x), k = ck(pos.z);
        if (!inGrid(i, k)) return -1;
        const int c = i * nz + k;
        int under = -1;
        for (int j = colFirst[c]; j < colFirst[c] + colCount[c]; ++j)
            if (nodes[j].y <= pos.y + 0.6f && (under < 0 || nodes[j].y > nodes[under].y)) under = j;
        if (under >= 0 && pos.y - nodes[under].y <= STEP) return -1;   // its own column holds it
        const int a = nodeAt(pos);
        if (a < 0 || a == under || std::fabs(nodes[a].y - pos.y) > 0.6f) return -1;
        const Field& f = field(ag);
        if (f.dist.empty()) return -1;
        if (under >= 0 && f.dist[under] <= f.dist[a] + 0.5f) return -1;   // down is the way: let it go
        return a;
    }

    // Route length from a node (1e9: none), for tests and the overlay
    float distanceFrom(const NavAgent& ag, int n) const {
        if (!built || n < 0 || goal_ < 0) return 1e9f;
        const Field& f = field(ag);
        return f.dist.empty() ? 1e9f : f.dist[n];
    }
    int nextFrom(const NavAgent& ag, int n) const {
        if (!built || n < 0 || goal_ < 0) return -1;
        const Field& f = field(ag);
        if (f.next.empty() || f.next[n] < 0) return -1;
        return nodes[n].nb[f.next[n]];
    }
    int fieldsBuilt = 0;          // since the grid was cut (the overlay and the tests read these)
    mutable double lastFieldMs = 0.;

private:
    struct Field {
        int goal = -1; unsigned revision = 0; float at = -1e9f;
        std::vector<float> dist; std::vector<int8_t> next;
    };
    mutable Field fields_[8 * VARIANTS];
    mutable int budget_ = 1;
    float clock_ = 0.f;
    int goal_ = -1;

    static int keyOf(const NavAgent& a) {
        return ((a.wide ? 1 : 0) | (a.tall ? 2 : 0) | (a.drops ? 4 : 0)) + 8 * glm::clamp(a.variant, 0, VARIANTS - 1);
    }
    // A variant's price for ground: blocks of five cells, each up to 60% dearer, different per variant
    float priceAt(const Node& n, int variant) const {
        float p = (n.flags & OPEN) ? 0.f : 0.3f;   // hugging a wall or an edge costs a little: routes keep to the open
        if (n.flags & BURNS) p += 2.f;
        if (variant > 0) {
            const int i = (n.col / nz) / 5, k = (n.col % nz) / 5;
            uint32_t h = (uint32_t)i * 73856093u ^ (uint32_t)k * 19349663u ^ (uint32_t)variant * 83492791u;
            h ^= h >> 13; h *= 0x5bd1e995u; h ^= h >> 15;
            p += 0.6f * (float)(h & 1023u) / 1023.f;
        }
        return p;
    }
    const Field& field(const NavAgent& ag) const {
        Field& f = fields_[keyOf(ag)];
        const bool stale = f.revision != revision || f.dist.size() != nodes.size();
        if (stale || (f.goal != goal_ && clock_ - f.at >= REFRESH && budget_ > 0)) {
            if (!stale) --budget_;   // (a field that has never been built can't wait its turn)
            compute(f, ag);
        }
        return f;
    }
    void compute(Field& f, const NavAgent& ag) const {
        f.goal = goal_; f.revision = revision; f.at = clock_;
        f.dist.assign(nodes.size(), 1e9f); f.next.assign(nodes.size(), (int8_t)-1);
        ++const_cast<NavGrid*>(this)->fieldsBuilt;
        if (goal_ < 0) return;
        using Item = std::pair<float, int>;
        std::priority_queue<Item, std::vector<Item>, std::greater<Item>> open;
        f.dist[goal_] = 0.f; open.push({0.f, goal_});
        while (!open.empty()) {
            const Item it = open.top(); open.pop();
            const int b = it.second;
            if (it.first > f.dist[b]) continue;
            for (int j = revFirst[b]; j < revFirst[b + 1]; ++j) {
                const int a = rev[j].from, d = rev[j].d;
                const Node& A = nodes[a];
                if (!fits(A, ag)) continue;
                if (!ag.drops && (A.big & (1u << d))) continue;
                float cost = (d < 4 ? 1.f : 1.4142f) * cell * (1.f + priceAt(A, ag.variant));
                if (A.y - nodes[b].y > STEP) cost += 0.6f;   // a drop: fine, but not for nothing
                const float nd = it.first + cost;
                if (nd < f.dist[a]) { f.dist[a] = nd; f.next[a] = (int8_t)d; open.push({nd, a}); }
            }
        }
    }
};
