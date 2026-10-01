#pragma once
// =============================================================================
// Progression.h — XP, levels and weapon upgrades for one run, plus the best
// times kept between runs. No OpenGL.
//
// Kills give XP by enemy type, multiplied by your style rank (D x1 … SSS x2),
// so playing aggressively levels you faster. Each level is one upgrade point,
// spent in the ARMORY (TAB). Dying and retrying an arena keeps your upgrades;
// a new run starts from level 1.
// =============================================================================
#include "Weapons.h"
#include "Enemy.h"
#include "StyleSystem.h"
#include "Persist.h"
#include <array>
#include <cstdio>
#include <sstream>
#include <string>
#include <vector>

inline int xpForKill(EnemyType t) {
    switch (t) {
        case EnemyType::HUSK:     return 20;
        case EnemyType::RIPPER:   return 15;
        case EnemyType::SENTINEL: return 30;
        case EnemyType::RAPTOR:   return 25;
        case EnemyType::BRUTE:    return 80;
        case EnemyType::MITE:     return 8;
        case EnemyType::WARDEN:   return 500;
        default:                  return 10;
    }
}

inline float styleXpMultiplier(StyleRank r) {
    switch (r) {
        case StyleRank::D:   return 1.0f;
        case StyleRank::C:   return 1.2f;
        case StyleRank::B:   return 1.4f;
        case StyleRank::A:   return 1.6f;
        case StyleRank::S:   return 1.8f;
        case StyleRank::SSS: return 2.0f;
    }
    return 1.f;
}

struct Progression {
    int level  = 1;
    int xp     = 0;      // progress toward the next level
    int points = 0;      // unspent upgrade points
    int totalXp = 0;
    std::array<WeaponUpgrades, WEAPON_COUNT> up{};

    static int xpToNext(int lvl) { return 60 + 40 * lvl; }

    // Returns the number of levels gained.
    int addXp(int amount) {
        if (amount <= 0) return 0;
        xp += amount; totalXp += amount;
        int gained = 0;
        while (xp >= xpToNext(level)) { xp -= xpToNext(level); ++level; ++points; ++gained; }
        return gained;
    }

    static int cost(UpgradeStat s) { return s == UpgradeStat::MOD ? 2 : 1; }

    int tierOf(WeaponId w, UpgradeStat s) const {
        const WeaponUpgrades& u = up[(int)w];
        return s == UpgradeStat::MOD ? (u.mod ? 1 : 0) : u.tier[(int)s];
    }
    static int maxTier(UpgradeStat s) { return s == UpgradeStat::MOD ? 1 : MAX_TIER; }

    bool canBuy(WeaponId w, UpgradeStat s) const {
        return tierOf(w, s) < maxTier(s) && points >= cost(s);
    }
    bool buy(WeaponId w, UpgradeStat s) {
        if (!canBuy(w, s)) return false;
        points -= cost(s);
        WeaponUpgrades& u = up[(int)w];
        if (s == UpgradeStat::MOD) u.mod = true; else ++u.tier[(int)s];
        return true;
    }
};

// Best times, kept across runs (file on desktop, localStorage on the web).
struct Records {
    float bestArena = 0.f;                 // full Arena run, seconds (0 = none yet)
    float bestFast  = 0.f;                 // FAST mode total
    std::vector<float> fastSplits;         // cumulative time at each section clear, from the best run

    static constexpr const char* kKey = "records.cfg";

    void load() {
        std::istringstream f(persist::load(kKey));
        std::string key;
        while (f >> key) {
            if (key == "bestArena") f >> bestArena;
            else if (key == "bestFast") f >> bestFast;
            else if (key == "fastSplits") {
                int n = 0; f >> n;
                fastSplits.assign(std::max(0, std::min(n, 32)), 0.f);
                for (auto& s : fastSplits) f >> s;
            } else { std::string skip; f >> skip; }
            if (!f) break;
        }
    }
    void save() const {
        std::ostringstream f;
        f << "bestArena " << bestArena << "\n";
        f << "bestFast " << bestFast << "\n";
        f << "fastSplits " << fastSplits.size();
        for (float s : fastSplits) f << " " << s;
        f << "\n";
        persist::save(kKey, f.str());
    }
};

// "1:23.45"
inline std::string formatTime(float t, bool hundredths = true) {
    if (t < 0.f) t = 0.f;
    int m = (int)(t / 60.f);
    float s = t - m * 60.f;
    char buf[32];
    if (hundredths) std::snprintf(buf, sizeof(buf), "%d:%05.2f", m, s);
    else            std::snprintf(buf, sizeof(buf), "%d:%02d", m, (int)s);
    return buf;
}
