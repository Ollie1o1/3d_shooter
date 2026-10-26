// `make scan`: every arena's traps, in detail (tests/BoundsScan.h). `make test`
// checks the same thing and fails on any trap; this prints where they are.
#include "world/Level.h"
#include "world/LevelGauntlet.h"
#include "world/LevelAct2.h"
#include "BoundsScan.h"
#include <cstdio>

int main() {
    struct Lvl { const char* name; LevelData L; };
    Lvl lv[] = {{"ACT I", buildLevel()}, {"FAST", buildGauntlet()}, {"ACT II", buildAct2Level()}};
    int total = 0;
    auto report = [&](const char* what, const bscan::Result& r) {
        total += r.traps;
        std::printf("%-34s surfaces %6d traps %5d (start %.1f m off)", what, r.nodes, r.traps, r.startMiss);
        if (r.traps) std::printf("  in (%.1f %.1f %.1f)-(%.1f %.1f %.1f)", r.trapBox.min.x, r.trapBox.min.y, r.trapBox.min.z,
                                 r.trapBox.max.x, r.trapBox.max.y, r.trapBox.max.z);
        std::printf("\n");
        for (auto& t : r.samples) std::printf("      at (%.1f %.1f %.1f)\n", t.at.x, t.at.y, t.at.z);
    };
    for (auto& l : lv) {
        std::vector<int> doors = bscan::doorWalls(l.L);
        char buf[96];
        for (int a = 0; a < (int)l.L.arenas.size(); ++a) {
            const Arena& ar = l.L.arenas[a];
            std::snprintf(buf, sizeof(buf), "%s %d %s", l.name, a, ar.name);
            report(buf, bscan::scan(l.L, bscan::zonesOf(ar), ar.playerStart, ar.voidY, {ar.bounds}, &doors));
        }
        // A cleared arena: its zone, the corridor on and the next arena's, all open at once
        for (int a = 0; a + 1 < (int)l.L.arenas.size() && a < (int)l.L.corridors.size(); ++a) {
            const Arena& A = l.L.arenas[a]; const Arena& B = l.L.arenas[a + 1];
            std::vector<AABB> zones = bscan::zonesOf(A), zb = bscan::zonesOf(B);
            zones.insert(zones.end(), zb.begin(), zb.end());
            zones.push_back(l.L.corridors[a]);
            AABB cor = l.L.corridors[a]; cor.max.y = cor.min.y + 6.f;   // its floor, not its roof
            std::snprintf(buf, sizeof(buf), "%s corridor %d -> %d", l.name, a, a + 1);
            report(buf, bscan::scan(l.L, zones, A.playerStart, std::min(A.voidY, B.voidY), {A.bounds, B.bounds, cor}, &doors));
        }
    }
    std::printf("total traps %d\n", total);
    return total ? 1 : 0;
}
