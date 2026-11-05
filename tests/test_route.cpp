// Story routes (tests/RouteScan.h): every chapter, section by section, can be walked and jumped through
// (with the doors open) except where a known aid carries you: a lift, the grapple. Every checkpoint's
// spawn stands on something with room over its head, and a real Player put there stays put.
// No OpenGL context required. `make test-route`; `make routes` prints the whole picture.
#include "world/LevelStory.h"
#include "world/LevelWasteland.h"
#include "world/LevelLab.h"
#include "world/LevelPlain.h"
#include "world/LevelCold.h"
#include "world/Stuck.h"
#include "player/Player.h"
#include "RouteScan.h"
#include <cstdio>
#include <cstring>

static int failures = 0;
#define CHECK(cond, msg) do { if (cond) std::printf("ok:   %s\n", msg); else { std::printf("FAIL: %s\n", msg); ++failures; } } while (0)
static constexpr float DT = 1.f / 60.f;

// Legs (section a -> a + 1) that need more than your feet, and what carries you
struct Aid { int chapter, leg; const char* what; };
static const Aid AIDS[] = {
    {1, 2, "the Ducts' service lift up to the Assembly Hall"},
    {1, 6, "the Elevator Shaft's freight deck"},
    {5, 1, "the Hook Line: the grapple, hook to hook"},
};
static const Aid* aidFor(int chapter, int leg) {
    for (const Aid& a : AIDS) if (a.chapter == chapter && a.leg == leg) return &a;
    return nullptr;
}

int main() {
    struct Ch { int n; const char* name; LevelData L; };
    Ch ch[] = {{1, "THE CRADLE", buildCradle()}, {2, "THE DEAD SPAN", buildWasteland()}, {3, "THE LIVING WARD", buildLab()},
               {4, "THE BLOOD MOON PLAIN", buildPlain()}, {5, "THE COLD STORE", buildCold()}};
    int legs = 0, broken = 0, aided = 0, stale = 0, badSpawns = 0, drifted = 0;
    for (auto& c : ch) {
        const LevelData& L = c.L;
        for (int a = 0; a + 1 < (int)L.arenas.size(); ++a) {
            auto r = rscan::leg(L, a); ++legs;
            const Aid* aid = aidFor(c.n, a);
            if (!r.reached && aid) { ++aided; continue; }
            if (!r.reached) {
                ++broken;
                std::printf("      chapter %d, %s -> %s: no way on foot; you get within %.1f m, at (%.1f %.1f %.1f)\n", c.n, L.arenas[a].name,
                            L.arenas[a + 1].name, r.gap, r.closest.x, r.closest.y, r.closest.z);
            } else if (aid) {
                ++stale;
                std::printf("      chapter %d leg %d is walkable now: take \"%s\" off the list of aids\n", c.n, a, aid->what);
            }
        }
        // Each section's start (a checkpoint's spawn): solid under it, room above, and a real Player stays there
        SpatialGrid g; g.build(L.walls);
        for (int a = 0; a < (int)L.arenas.size(); ++a) {
            const Arena& A = L.arenas[a];
            auto s = rscan::spawn(L, a);
            if (s.drop > 0.6f || !s.headroom || s.embedded) {
                ++badSpawns;
                std::printf("      chapter %d, %s: spawn %s\n", c.n, A.name, !s.headroom ? "under a low ceiling" : s.embedded ? "inside a wall" : "over a drop");
            }
            Player p(A.playerStart);
            p.dynWalls = L.moverWalls.data(); p.dynCount = (int)L.moverWalls.size();
            Uint8 k[SDL_NUM_SCANCODES]; std::memset(k, 0, sizeof(k));
            bool stuck = false;
            for (int i = 0; i < 120; ++i) {
                p.floorY = L.baseFloor(p.position.x, p.position.z);
                p.update(DT, k, L.walls.data(), (int)L.walls.size(), false, &g);
                stuck |= bodyEmbedded(L.walls, L.moverWalls, g, p.position, p.radius, p.height);
            }
            if (!p.onGround || stuck || glm::length(p.position - A.playerStart) > 0.6f || p.position.y < A.voidY) {
                ++drifted;
                std::printf("      chapter %d, %s: a player spawned there %s (moved %.2f m)\n", c.n, A.name,
                            stuck ? "is stuck in something" : !p.onGround ? "never lands" : "slides away", glm::length(p.position - A.playerStart));
            }
        }
    }
    std::printf("      %d legs: %d walkable, %d by a known aid, %d broken\n", legs, legs - aided - broken, aided, broken);
    CHECK(broken == 0, "every Story chapter can be walked and jumped through, section to section (doors open), but for its known lifts and grapples");
    CHECK(stale == 0, "...and every known aid is still needed (the list stays true)");
    CHECK(badSpawns == 0, "every checkpoint spawn stands on something, with room over its head");
    CHECK(drifted == 0, "...and a real player spawned at each lands there and stays put, stuck in nothing");

    {   // The scan does catch a break: lift the Scrap Pit's exit ledge out of reach and the leg to the Ducts fails
        LevelData L = buildCradle();
        for (Wall& w : L.walls) if (std::fabs(w.box.min.x - 26.f) < 0.01f && std::fabs(w.box.max.y + 27.4f) < 0.01f) { w.box.min.y += 30.f; w.box.max.y += 30.f; }
        CHECK(!rscan::leg(L, 1).reached, "...and it catches a route that breaks (the Scrap Pit's exit ledge moved out of reach)");
    }

    std::printf(failures ? "\n%d FAILED\n" : "\nALL PASSED (0 failures)\n", failures);
    return failures ? 1 : 0;
}
