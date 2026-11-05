// `make audit`: every section of every mode, measured (tests/MapAudit.h), with ACT I's arenas as the benchmark
#include "world/Level.h"
#include "world/LevelGauntlet.h"
#include "world/LevelAct2.h"
#include "world/LevelStory.h"
#include "world/LevelWasteland.h"
#include "world/LevelLab.h"
#include "world/LevelPlain.h"
#include "world/LevelCold.h"
#include "MapAudit.h"
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

int main(int argc, char** argv) {
    struct Lvl { const char* name; LevelData (*build)(); };
    const Lvl lv[] = {{"ACT I", buildLevel}, {"FAST", buildGauntlet}, {"ACT II", buildAct2Level}, {"CH1", buildCradle},
                      {"CH2", buildWasteland}, {"CH3", buildLab}, {"CH4", buildPlain}, {"CH5", buildCold}};
    const char* only = argc > 1 ? argv[1] : nullptr;
    std::printf("%-6s %-2s %-22s %7s %3s %5s %5s %5s %4s %3s %5s %6s %6s %6s\n", "MODE", "#", "SECTION", "FLOOR", "LV", "RISE",
                "COVER", "SIGHT", "ENMY", "KND", "DENS", "DETOUR", "DRESS", "LIGHT");
    std::printf("(fights of 3+: cover 25-50%%, 3+ levels, sight 12-30 m; every section: 5+ dressing per 100 m², detour under 1.5, indoors 0.5+ lights)\n");
    std::vector<std::string> summary;
    for (const auto& l : lv) {
        if (only && std::strcmp(only, l.name) != 0) continue;
        LevelData L = l.build();
        for (int a = 0; a < (int)L.arenas.size(); ++a) {
            if (L.arenas[a].zone.max.x - L.arenas[a].zone.min.x > 600.f) { std::printf("%-6s %-2d %-22s (too big to sample)\n", l.name, a, L.arenas[a].name); continue; }
            auto s = audit::measure(L, a);
            std::printf("%-6s %-2d %-22.22s %7.0f %3d %5.1f %4.0f%% %5.1f %4d %3d %5.2f %6.2f %6.1f %6.1f", l.name, a, s.name, s.floor, s.levels,
                        s.rise, s.cover * 100.f, s.sight, s.enemies, s.kinds, s.density, s.detour, s.dressing, s.lights);
            const std::string m = audit::misses(s);
            std::printf("%s\n", m.empty() ? "   meets the standard" : ("   " + m).c_str());
            char row[96]; std::snprintf(row, sizeof(row), "%-6s %-2d %-22.22s %s", l.name, a, s.name, m.empty() ? "ok" : m.c_str());
            summary.push_back(row);
            std::fflush(stdout);
        }
    }
    std::printf("\nagainst the standard (docs/gameplay-standard.md):\n");
    int ok = 0; for (const auto& r : summary) { std::printf("  %s\n", r.c_str()); ok += r.size() >= 2 && r.compare(r.size() - 2, 2, "ok") == 0; }
    std::printf("%d of %zu sections meet it\n", ok, summary.size());
}
