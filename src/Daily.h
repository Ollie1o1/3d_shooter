#pragma once
// =============================================================================
// Daily.h — the daily challenge: one ENDLESS run a day, the same for everyone.
// The date (UTC) picks the arena, the modifier and the seed that generates
// the waves, and names the day's leaderboard. No OpenGL.
// =============================================================================
#include <cstdint>
#include <ctime>
#include <cstdio>
#include <string>

enum class DailyMod { GLASS_CANNON, SWARM, MARKSMAN, GROUNDED, COUNT };

struct DailyInfo {
    int      date = 0;          // YYYYMMDD
    uint32_t seed = 0;
    int      arena = 0;         // 0..3: the Yard, the Foundry, the Spire, the Core
    DailyMod mod = DailyMod::GLASS_CANNON;

    static const char* modName(DailyMod m) {
        static const char* N[] = {"GLASS CANNON", "SWARM", "MARKSMAN", "GROUNDED"};
        return N[(int)m];
    }
    static const char* modHint(DailyMod m) {
        static const char* H[] = {"YOU DEAL DOUBLE DAMAGE - AND TAKE IT",
                                  "WAVES HALF AGAIN AS BIG - EACH ENEMY 60% AS TOUGH",
                                  "THE KAR98 ONLY",
                                  "NO GRAPPLE"};
        return H[(int)m];
    }
    const char* modName() const { return modName(mod); }
    const char* modHint() const { return modHint(mod); }
    std::string key() const { char b[16]; std::snprintf(b, sizeof(b), "%08d", date); return b; }
    std::string label() const {   // e.g. 2026-10-05
        char b[16]; std::snprintf(b, sizeof(b), "%04d-%02d-%02d", date / 10000, date / 100 % 100, date % 100); return b;
    }

    static DailyInfo forDate(int yyyymmdd) {
        DailyInfo d;
        d.date = yyyymmdd;
        // A small integer hash of the date, so neighbouring days differ
        uint32_t h = (uint32_t)yyyymmdd * 2654435761u;
        h ^= h >> 16; h *= 2246822519u; h ^= h >> 13;
        d.seed  = h;
        d.arena = (int)(h % 4u);
        d.mod   = (DailyMod)((h / 4u) % (uint32_t)DailyMod::COUNT);
        return d;
    }
    static int todayUtc() {
        std::time_t now = std::time(nullptr);
        std::tm g{};
#ifdef _WIN32
        gmtime_s(&g, &now);
#else
        gmtime_r(&now, &g);
#endif
        return (g.tm_year + 1900) * 10000 + (g.tm_mon + 1) * 100 + g.tm_mday;
    }
    static DailyInfo today() { return forDate(todayUtc()); }
};
