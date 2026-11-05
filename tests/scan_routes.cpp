// `make routes`: each Story chapter, section by section (tests/RouteScan.h): where the way on breaks
#include "world/LevelStory.h"
#include "world/LevelWasteland.h"
#include "world/LevelLab.h"
#include "world/LevelPlain.h"
#include "world/LevelCold.h"
#include "RouteScan.h"
#include <cstdio>

int main() {
    struct Ch { const char* name; LevelData L; };
    Ch ch[] = {{"1 CRADLE", buildCradle()}, {"2 DEAD SPAN", buildWasteland()}, {"3 LIVING WARD", buildLab()},
               {"4 PLAIN", buildPlain()}, {"5 COLD STORE", buildCold()}};
    for (auto& c : ch) {
        const LevelData& L = c.L;
        std::printf("%s\n", c.name);
        for (int a = 0; a < (int)L.arenas.size(); ++a) {
            auto s = rscan::spawn(L, a);
            std::printf("   spawn %d %-22s drop %.2f m%s%s\n", a, L.arenas[a].name, s.drop, s.headroom ? "" : "  LOW CEILING", s.embedded ? "  INSIDE A WALL" : "");
        }
        for (int a = 0; a + 1 < (int)L.arenas.size(); ++a) {
            auto r = rscan::leg(L, a);
            std::printf("   %d -> %d %-22s %s", a, a + 1, L.arenas[a + 1].name, r.reached ? "ok" : "BROKEN");
            if (!r.reached) std::printf("  gets within %.1f m, at (%.1f %.1f %.1f)", r.gap, r.closest.x, r.closest.y, r.closest.z);
            std::printf("  [%d places, start %.1f m off]\n", r.nodes, r.startMiss);
            std::fflush(stdout);
        }
    }
}
