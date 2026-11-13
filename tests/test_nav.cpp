// Routes (world/NavGrid.h): in every arena and Story section that has a fight, walkers put down at
// many reachable places get to the player in time, nobody in a simulated wave stands wedged, and the
// grid and its fields stay inside their time budget. No OpenGL context required. `make test-nav`.
//   build/test_nav --compare   also runs each chase on feeler steering alone (what the game did before)
//   build/test_nav --only CH3  one map (ARENA, ACT2, FAST, CH1..CH6)
#include "PortableRand.h"   // (first: one rand() sequence on macOS and Linux)
#include "world/Level.h"
#include "world/LevelAct2.h"
#include "world/LevelGauntlet.h"
#include "world/LevelStory.h"
#include "world/LevelWasteland.h"
#include "world/LevelLab.h"
#include "world/LevelPlain.h"
#include "world/LevelCold.h"
#include "world/LevelRing.h"
#include "world/NavBuild.h"
#include <chrono>
#include <cstdio>
#include <cstring>
#include <string>

static int failures = 0;
#define CHECK(cond, msg) do { if (cond) std::printf("ok:   %s\n", msg); else { std::printf("FAIL: %s\n", msg); ++failures; } } while (0)
static constexpr float DT = 1.f / 60.f;

static uint32_t g_seed = 12345u;
static uint32_t rnd() { g_seed = g_seed * 1664525u + 1013904223u; return g_seed >> 8; }

struct Map { const char* tag; LevelData L; };

// The world an enemy is handed in this arena, as GameplayState::updateEnemies fills it
static EnemyWorld worldFor(const LevelData& L, int a, const SpatialGrid& grid, glm::vec3 feet, const NavGrid* nav) {
    EnemyWorld w;
    w.playerFeet = feet; w.playerEye = feet + glm::vec3{0.f, 1.7f, 0.f};
    w.walls = L.walls.data(); w.wallCount = (int)L.walls.size(); w.grid = &grid;
    w.bounds = L.arenas[a].bounds;
    w.dynWalls = L.moverWalls.data(); w.dynCount = (int)L.moverWalls.size();
    w.ramps = L.ramps.data(); w.rampCount = (int)L.ramps.size();
    w.voidUnder = arenaVoidUnder(L, a);
    w.groundTracked = arenaGroundTracked(L, a);
    w.nav = nav;
    return w;
}
static void floorUnder(const LevelData& L, int a, Enemy& e) {
    const bool tracked = arenaGroundTracked(L, a);
    e.floorY = L.enemyFloor(a, e.position, e.stats().flying, tracked ? L.groundAt(e.position.x, e.position.z, e.position.y + 0.5f) : 0.f);
}
static Enemy fresh(EnemyType t, glm::vec3 at, int uid) {
    Enemy e(t, at, at.y);
    e.uid = uid; e.spawnTimer = 0.f; e.state = EnemyState::ACTIVE;
    e.attackPace = 0.f;   // (they walk; nobody is there to hit)
    return e;
}
static NavAgent agentOf(EnemyType t, const LevelData& L, int a) {
    Enemy e(t, glm::vec3{0.f});
    NavAgent ag;
    ag.wide = e.radius() > 0.62f; ag.tall = e.height() > 2.3f;
    const bool ledgeAware = t != EnemyType::RIPPER && t != EnemyType::MITE;
    ag.drops = !arenaVoidUnder(L, a) && !ledgeAware;
    return ag;
}

struct Tally { int runs = 0, reached = 0, stuck = 0; double metres = 0., seconds = 0.; };

// One chase: does the walker get within `near` of the player's feet in time?
static bool chase(const LevelData& L, int a, const SpatialGrid& grid, NavGrid* nav, EnemyType t, glm::vec3 from, glm::vec3 feet,
                  float route, float near, int uid, bool report, const char* tag) {
    Enemy e = fresh(t, from, uid);
    EnemyWorld w = worldFor(L, a, grid, feet, nav);
    const float speed = std::max(1.f, e.stats().speed);
    const float limit = 8.f + route / speed * 2.2f;
    const float voidY = L.arenas[a].voidY;
    for (int f = 0; f < (int)(limit * 60.f); ++f) {
        if (nav) nav->tick(DT, feet);
        floorUnder(L, a, e);
        e.update(DT, w);
        if (std::getenv("NAVFINE") && nav && f / 60.f > std::atof(std::getenv("NAVFINE")) && f / 60.f < std::atof(std::getenv("NAVFINE")) + 0.5f) {
            const NavAgent ag = e.navAgent(w); glm::vec3 v{e.velocity.x, 0.f, e.velocity.z}; const float l = glm::length(v);
            if (l > 0.1f) v /= l;
            const int n = nav->nodeAt(e.position); const NavStep st = nav->step(ag, e.position);
            std::printf("    f %d pos (%.2f %.2f %.2f) v (%.2f %.2f) node %d y %.2f d %.1f  strays %d/%d step (%.2f %.2f) routed %d floorY %.2f\n", f, e.position.x, e.position.y, e.position.z, v.x, v.z, n,
                        n >= 0 ? nav->nodes[n].y : -999.f, nav->distanceFrom(ag, n), nav->straysAt(ag, e.position, v, e.radius() + 0.6f), nav->straysAt(ag, e.position, v, 0.35f), st.dir.x, st.dir.z, e.navRouted, e.floorY, e.navWhy);
        }
        if (std::getenv("NAVTRACE") && nav && f % 15 == 0) {
            const int n = nav->nodeAt(e.position);
            std::printf("  t %.2f pos (%.1f %.1f %.1f) vel (%.1f %.1f) routed %d straight %d left %.1f nodeY %.1f\n", f / 60.f, e.position.x, e.position.y, e.position.z,
                        e.velocity.x, e.velocity.z, e.navRouted, e.navStraight, nav->distanceFrom(e.navAgent(w), n), n >= 0 ? nav->nodes[n].y : -999.f);
        }
        if (!e.alive || e.position.y < voidY) break;
        const glm::vec3 d = feet - e.position;
        if (glm::length(glm::vec2{d.x, d.z}) < near && std::fabs(d.y) < 2.6f) return true;
    }
    if (report)
        std::printf("      %s %s: %s from (%.1f %.1f %.1f) to (%.1f %.1f %.1f), a %.0f m route, stopped at (%.1f %.1f %.1f)%s\n", tag, L.arenas[a].name,
                    e.stats().name, from.x, from.y, from.z, feet.x, feet.y, feet.z, route, e.position.x, e.position.y, e.position.z,
                    e.position.y < voidY ? " in the void" : "");
    if (report && nav && std::getenv("NAVDBG")) {
        const int n = nav->nodeAt(e.position);
        const NavAgent ag = e.navAgent(w);
        std::printf("         node %d", n);
        if (n >= 0) {
            const auto& N = nav->nodes[n];
            std::printf(" y %.2f flags %d big %d left %.1f routed %d straight %d next->%d  nb:", N.y, N.flags, N.big, nav->distanceFrom(ag, n), e.navRouted, e.navStraight, nav->nextFrom(ag, n));
            for (int d = 0; d < 8; ++d) { if (N.nb[d] < 0) std::printf(" --"); else std::printf(" %.1f", nav->nodes[N.nb[d]].y); }
            const NavStep st = nav->step(ag, e.position);
            std::printf("  step %d (%.2f %.2f) vel (%.2f %.2f) floorY %.2f", st.valid, st.dir.x, st.dir.z, e.velocity.x, e.velocity.z, e.floorY);
        }
        std::printf("\n");
    }
    return false;
}

int main(int argc, char** argv) {
    bool compare = false; const char* only = nullptr;
    for (int i = 1; i < argc; ++i) {
        if (!std::strcmp(argv[i], "--compare")) compare = true;
        if (!std::strcmp(argv[i], "--only") && i + 1 < argc) only = argv[++i];
    }
    Map maps[] = {{"ARENA", buildLevel()}, {"ACT2", buildAct2Level()}, {"FAST", buildGauntlet()}, {"CH1", buildCradle()},
                  {"CH2", buildWasteland()}, {"CH3", buildLab()}, {"CH4", buildPlain()}, {"CH5", buildCold()}, {"CH6", buildRing()}};
    // Who walks: a light chaser that drops off ledges, a small one, a wide tall one that holds its perch
    const EnemyType walkers[] = {EnemyType::RIPPER, EnemyType::MITE, EnemyType::BRUTE};
    const float nearOf[] = {4.f, 2.8f, 6.5f};

    Tally nav, old;
    int arenasWith = 0, arenasWithout = 0, wedged = 0, waveEnemies = 0;
    double worstBuild = 0., worstField = 0., worstStep = 0., sumBuild = 0.;
    double closeNav = 0., closeOld = 0., shotsNav = 0., shotsOld = 0.;
    size_t mostNodes = 0;
    for (Map& m : maps) {
        if (only && std::strcmp(only, m.tag)) continue;
        const LevelData& L = m.L;
        SpatialGrid grid; grid.build(L.walls);
        for (int a = 0; a < (int)L.arenas.size(); ++a) {
            const Arena& A = L.arenas[a];
            if (A.waves.empty()) continue;
            ArenaNav an; an.rebuild(L, a, &grid);
            if (!an.get()) { ++arenasWithout; std::printf("      %s %-28s no grid (%s)\n", m.tag, A.name, ArenaNav::arenaTakesNav(A) ? "too wide" : "moving ground"); continue; }
            ++arenasWith;
            NavGrid& G = an.grid;
            worstBuild = std::max(worstBuild, G.buildMs); sumBuild += G.buildMs; mostNodes = std::max(mostNodes, G.nodes.size());

            // Where the player stands: the section's start, then places picked across the grid that the start can be walked to from
            // (a start behind its own shut gate, or out in the approach, doesn't count: then somewhere most of the room can walk to)
            g_seed = 977u * (uint32_t)(a + 1) + (uint32_t)m.tag[0];
            const NavAgent plain{};
            int open = 0;
            for (const auto& N : G.nodes) open += (N.flags & NavGrid::OPEN) ? 1 : 0;
            glm::vec3 base = A.playerStart;
            for (int tries = 0; tries < 40; ++tries) {
                for (int k = 0; k < 14; ++k) G.tick(DT, base);
                int reach = 0;
                if (G.goal() >= 0) for (int n = 0; n < (int)G.nodes.size(); ++n) reach += ((G.nodes[n].flags & NavGrid::OPEN) && G.distanceFrom(plain, n) < 1e8f) ? 1 : 0;
                if (reach * 4 > open) break;
                int n = (int)(rnd() % (uint32_t)G.nodes.size());
                while (!(G.nodes[n].flags & NavGrid::OPEN)) n = (n + 1) % (int)G.nodes.size();
                base = G.at(n);
            }
            std::vector<glm::vec3> stands{base};
            // (the others: open ground the first stand can walk to and back from, so not a pocket you can only drop out of)
            std::vector<int> cands;
            for (int k = 0; k < 14; ++k) G.tick(DT, base);
            const int baseNode = G.goal();
            for (int tries = 0; tries < 400 && cands.size() < 12; ++tries) {
                const int n = (int)(rnd() % (uint32_t)G.nodes.size());
                if (!(G.nodes[n].flags & NavGrid::OPEN) || G.distanceFrom(plain, n) > 1e8f) continue;
                bool spread = true;
                for (int c : cands) spread &= glm::length(G.at(n) - G.at(c)) > 10.f;
                if (spread && glm::length(G.at(n) - base) > 10.f) cands.push_back(n);
            }
            for (int c : cands) {
                if (stands.size() >= 4) break;
                for (int k = 0; k < 14; ++k) G.tick(DT, G.at(c));
                if (G.goal() == c && baseNode >= 0 && G.distanceFrom(plain, baseNode) < 1e8f) stands.push_back(G.at(c));
            }
            Tally an_nav, an_old;
            for (size_t s = 0; s < stands.size(); ++s) {
                // (time passes between stands, so each field may follow the player)
                for (int k = 0; k < 20; ++k) G.tick(DT, stands[s]);
                if (G.goal() < 0) continue;
                const glm::vec3 feet = G.at(G.goal());
                // (Chapter 6 has its own walkers: the STRAY, which bounds, and the PORTER, wide and tall)
                const bool ring6 = !std::strcmp(m.tag, "CH6");
                const EnemyType ringWalkers[] = {EnemyType::STRAY, EnemyType::STRAY, EnemyType::PORTER};
                for (int wk = 0; wk < 3; ++wk) {
                    const EnemyType who = ring6 ? ringWalkers[wk] : walkers[wk];
                    NavAgent ag = agentOf(who, L, a);
                    for (int k = 0; k < 14; ++k) G.tick(DT, feet);   // (one field a tick: let this walker's follow the player here)
                    auto t0 = std::chrono::steady_clock::now();
                    (void)G.distanceFrom(ag, 0);
                    worstField = std::max(worstField, std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count());
                    int placed = 0;
                    for (int tries = 0; tries < 600 && placed < 5; ++tries) {
                        const int n = (int)(rnd() % (uint32_t)G.nodes.size());
                        const NavGrid::Node& N = G.nodes[n];
                        const float route = G.distanceFrom(ag, n);
                        if (!(N.flags & NavGrid::OPEN) || !NavGrid::fits(N, ag) || route > 1e8f || route < 8.f) continue;
                        ++placed;
                        const glm::vec3 from = G.at(n);
                        t0 = std::chrono::steady_clock::now();
                        for (int q = 0; q < 50; ++q) (void)G.step(ag, from);
                        worstStep = std::max(worstStep, std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - t0).count() / 50.);
                        srand(7u + (unsigned)n);
                        ++an_nav.runs; an_nav.metres += route;
                        if (chase(L, a, grid, &G, who, from, feet, route, nearOf[wk], 100 + n, true, m.tag)) ++an_nav.reached;
                        if (compare) {
                            srand(7u + (unsigned)n);
                            ++an_old.runs;
                            if (chase(L, a, grid, nullptr, who, from, feet, route, nearOf[wk], 100 + n, false, m.tag)) ++an_old.reached;
                        }
                    }
                }
            }
            nav.runs += an_nav.runs; nav.reached += an_nav.reached; nav.metres += an_nav.metres; old.runs += an_old.runs; old.reached += an_old.reached;

            // A wave: eight walkers from the arena's own spawns while the player moves between the stands.
            // Wedged: has a route of 6 m or more left, and hasn't moved a metre in three seconds.
            // Pressure (--compare): enemy-seconds spent within 6 m of the player, and gunner-seconds with a clear
            // shot inside 20 m, with routes and on feelers alone: how much harder the same wave leans on you.
            auto runWave = [&](bool routed, double& close, double& shots) {
                std::vector<Enemy> wave;
                const auto& spawns = A.groundSpawns;
                srand(99u + (unsigned)a);
                const EnemyType kinds[] = {EnemyType::RIPPER, EnemyType::HUSK, EnemyType::RIPPER, EnemyType::BRUTE,
                                           EnemyType::MITE, EnemyType::HUSK, EnemyType::RIPPER, EnemyType::SHIELDBEARER};
                for (int k = 0; k < 8 && !spawns.empty(); ++k) {
                    glm::vec3 p = spawns[(size_t)k % spawns.size()];
                    const int n = G.nodeAt(p, 1.5f);
                    if (n < 0) continue;
                    p = G.at(n) + glm::vec3{0.2f * (float)(k % 3), 0.f, 0.15f * (float)(k % 2)};
                    wave.push_back(fresh(kinds[k], p, 500 + k));
                }
                std::vector<glm::vec3> mark(wave.size()); std::vector<float> since(wave.size(), 0.f); std::vector<char> was(wave.size(), 0);
                for (size_t k = 0; k < wave.size(); ++k) mark[k] = wave[k].position;
                for (int f = 0; f < 60 * 30; ++f) {
                    const glm::vec3 stand = stands[(size_t)(f / (60 * 10)) % stands.size()];
                    G.tick(DT, stand);
                    if (f % (60 * 10) == 0) for (float& t : since) t = 0.f;   // (the player just moved: standing by where they were isn't wedged)
                    const glm::vec3 feet = G.goal() >= 0 ? G.at(G.goal()) : stand;
                    EnemyWorld w = worldFor(L, a, grid, feet, routed ? &G : nullptr);
                    for (size_t k = 0; k < wave.size(); ++k) {
                        Enemy& e = wave[k];
                        if (!e.alive || e.position.y < A.voidY) continue;
                        floorUnder(L, a, e);
                        e.update(DT, w);
                        const glm::vec3 d = feet - e.position;
                        const float flat = glm::length(glm::vec2{d.x, d.z});
                        if (flat < 6.f && std::fabs(d.y) < 2.6f && e.type != EnemyType::HUSK) close += DT;
                        if (e.type == EnemyType::HUSK && flat < 20.f && f % 6 == 0) {
                            const glm::vec3 from = e.position + glm::vec3{0.f, e.height() * 0.85f, 0.f};
                            glm::vec3 dir = w.playerEye - from; const float len = glm::length(dir);
                            bool clear = len > 1e-3f;
                            if (clear) { dir /= len; for (const Wall& wl : L.walls) { const float t = rayBoxHit(from, dir, wl.box); if (t > 0.f && t < len) { clear = false; break; } } }
                            if (clear) shots += DT * 6.f;
                        }
                        if (!routed) continue;
                        since[k] += DT;
                        if (glm::length(e.position - mark[k]) > 1.f) { mark[k] = e.position; since[k] = 0.f; }
                        const float left = G.distanceFrom(e.navAgent(w), G.nodeAt(e.position));
                        const bool holdsRange = e.type == EnemyType::HUSK;   // (a gunner standing off at its range isn't wedged)
                        if (since[k] > 3.f && left > 6.f && left < 1e8f && e.navRouted && !was[k] && !holdsRange) {
                            was[k] = 1; ++wedged;
                            std::printf("      %s %s: a %s wedged at (%.1f %.1f %.1f), %.0f m of route left\n", m.tag, A.name, e.stats().name,
                                        e.position.x, e.position.y, e.position.z, left);
                        }
                    }
                    for (size_t i = 0; i < wave.size(); ++i) for (size_t j = i + 1; j < wave.size(); ++j) {   // (the game's soft separation)
                        Enemy& x = wave[i]; Enemy& y = wave[j];
                        glm::vec3 d = y.position - x.position;
                        if (std::fabs(d.y) > 1.5f) continue;
                        d.y = 0.f;
                        const float len = glm::length(d), minD = x.radius() + y.radius();
                        if (len >= minD || len < 1e-4f) continue;
                        const glm::vec3 n = d / len * (minD - len);
                        x.position -= n * (y.radius() / minD); y.position += n * (x.radius() / minD);
                    }
                }
                if (routed) waveEnemies += (int)wave.size();
            };
            runWave(true, closeNav, shotsNav);
            if (compare) runWave(false, closeOld, shotsOld);
            std::printf("      %s %-28s %6zu nodes, %.2f m cells, cut in %5.1f ms; routes %d/%d", m.tag, A.name, G.nodes.size(), G.cell, G.buildMs,
                        an_nav.reached, an_nav.runs);
            if (compare) std::printf("  (feelers alone %d/%d)", an_old.reached, an_old.runs);
            std::printf("\n");
        }
    }
    std::printf("      %d arenas with a grid, %d without; %d chases, mean route %.0f m\n", arenasWith, arenasWithout, nav.runs,
                nav.runs ? nav.metres / nav.runs : 0.);
    if (compare) {
        std::printf("      feeler steering alone reached the player in %d of %d of the same chases\n", old.reached, old.runs);
        std::printf("      pressure over the simulated waves (a player who stands still for 10 s at a time, nobody shooting back):\n");
        std::printf("        melee enemy-seconds within 6 m: %.0f with routes, %.0f on feelers alone (%+.0f%%)\n", closeNav, closeOld, closeOld > 0. ? 100. * (closeNav / closeOld - 1.) : 0.);
        std::printf("        gunner-seconds with a clear shot inside 20 m: %.0f with routes, %.0f on feelers alone (%+.0f%%)\n", shotsNav, shotsOld, shotsOld > 0. ? 100. * (shotsNav / shotsOld - 1.) : 0.);
    }
    std::printf("      worst: grid %.1f ms (%zu nodes), a field %.2f ms, a step %.2f us; grids cut in %.0f ms altogether\n", worstBuild, mostNodes,
                worstField, worstStep, sumBuild);
    CHECK(arenasWith >= 30, "every fight on standing ground has a grid");
    std::printf("      routes reached the player in %d of %d chases (%.2f%%)\n", nav.reached, nav.runs, nav.runs ? 100. * nav.reached / nav.runs : 0.);
    // Not every one: a handful of spots remain where the grid's 0.8 m columns and the real boxes disagree
    // (each failure is printed above with where it stopped). The floor is 99%; feeler steering alone manages 89%.
    CHECK(nav.runs > 500 && nav.reached * 100 >= nav.runs * 99, "walkers put down anywhere reachable get to the player in time (99% of chases)");
    CHECK(waveEnemies > 100 && wedged == 0, "nobody in a simulated wave stands wedged with a route left to walk");
    CHECK(worstBuild < 250., "the largest grid is cut in under a quarter second (at arena load)");
    CHECK(worstField < 12., "a flow field for the whole arena costs under 12 ms (one at most per tick, five a second)");
    CHECK(worstStep < 60., "reading the route for one enemy costs under 60 microseconds");
    std::printf("%d nav failures\n", failures);
    return failures ? 1 : 0;
}
