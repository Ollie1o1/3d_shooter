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
#include <algorithm>

inline int xpForKill(EnemyType t) {
    switch (t) {
        case EnemyType::HUSK:     return 20;
        case EnemyType::RIPPER:   return 15;
        case EnemyType::SENTINEL: return 30;
        case EnemyType::RAPTOR:   return 25;
        case EnemyType::BRUTE:    return 80;
        case EnemyType::MITE:     return 8;
        case EnemyType::JUGGERNAUT: return 120;
        case EnemyType::SHIELDBEARER: return 40;
        case EnemyType::CONDUIT:  return 50;
        case EnemyType::CONDUCTOR: return 45;
        case EnemyType::WARDEN:   return 500;
        case EnemyType::SOVEREIGN: return 1000;
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
// The key carries a version: bump it when the maps change enough that old
// times no longer mean anything (v2: the Gauntlet's rooms, the Sanctum).
struct Records {
    float bestArena = 0.f;                 // full Arena run, seconds (0 = none yet)
    float bestFast  = 0.f;                 // FAST mode total
    std::vector<float> fastSplits;         // cumulative time at each section clear, from the best run

    static constexpr const char* kKey = "records.v2.cfg";

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

// Finished runs a player chose to put their name to, fastest first, kept
// per mode (the top KEEP of each). Only full runs count: not one started at
// a later arena, in god mode or from the dev level select.
struct Leaderboard {
    struct Entry { std::string name; float time; int difficulty; };
    static constexpr int KEEP = 10;
    static constexpr int MAX_NAME_LEN = 12;
    std::vector<Entry> arena, fast;
    std::string lastName;   // offered again next time

    static constexpr const char* kKey = "leaderboard.cfg";

    std::vector<Entry>&       list(bool f)       { return f ? fast : arena; }
    const std::vector<Entry>& list(bool f) const { return f ? fast : arena; }

    // Letters, digits, space and - . _ only (what the pixel font draws), upper case
    static bool nameChar(char c) {
        return (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == ' ' || c == '-' || c == '.' || c == '_';
    }
    static std::string cleanName(const std::string& in) {
        std::string o;
        for (char c : in) {
            if (c >= 'a' && c <= 'z') c = (char)(c - 'a' + 'A');
            if (nameChar(c) && (int)o.size() < MAX_NAME_LEN) o += c;
        }
        size_t a = o.find_first_not_of(' '), b = o.find_last_not_of(' ');
        return a == std::string::npos ? std::string() : o.substr(a, b - a + 1);
    }

    // Where a time would place (0-based), or -1 if it wouldn't make the board
    int placeFor(bool f, float t) const {
        const auto& l = list(f);
        int i = 0;
        while (i < (int)l.size() && l[i].time <= t) ++i;
        return i < KEEP ? i : -1;
    }
    // Returns the entry's place, or -1 (no name, or not fast enough)
    int add(bool f, const std::string& rawName, float t, int difficulty) {
        std::string name = cleanName(rawName);
        if (name.empty() || t <= 0.f) return -1;
        int at = placeFor(f, t);
        if (at < 0) return -1;
        auto& l = list(f);
        l.insert(l.begin() + at, Entry{name, t, difficulty});
        if ((int)l.size() > KEEP) l.resize(KEEP);
        lastName = name;
        return at;
    }

    void load() {
        arena.clear(); fast.clear();
        std::istringstream f(persist::load(kKey));
        std::string line;
        while (std::getline(f, line)) {
            std::istringstream ls(line);
            std::string key; ls >> key;
            if (key == "arena" || key == "fast") {
                Entry e{"", 0.f, 0}; ls >> e.time >> e.difficulty;
                std::string rest; std::getline(ls, rest);
                e.name = cleanName(rest);
                if (!e.name.empty() && e.time > 0.f && (int)list(key == "fast").size() < KEEP)
                    list(key == "fast").push_back(e);
            } else if (key == "lastName") {
                std::string rest; std::getline(ls, rest);
                lastName = cleanName(rest);
            }
        }
        auto byTime = [](const Entry& a, const Entry& b) { return a.time < b.time; };
        std::stable_sort(arena.begin(), arena.end(), byTime);
        std::stable_sort(fast.begin(), fast.end(), byTime);
    }
    // The shared (online) board, as the web page caches it: per mode, lines
    // of "time difficulty name". Empty off the web or with no API: callers
    // then show this browser's own board.
    static constexpr const char* kOnlineKey = "leaderboard.online.";
    bool loadOnline() {
        arena.clear(); fast.clear();
        for (bool fm : {false, true}) {
            std::istringstream f(persist::load(std::string(kOnlineKey) + (fm ? "fast" : "arena")));
            std::string line;
            while (std::getline(f, line) && (int)list(fm).size() < KEEP) {
                std::istringstream ls(line);
                Entry e{"", 0.f, 0};
                if (!(ls >> e.time >> e.difficulty)) continue;
                std::string rest; std::getline(ls, rest);
                e.name = cleanName(rest);
                if (!e.name.empty() && e.time > 0.f) list(fm).push_back(e);
            }
        }
        return !arena.empty() || !fast.empty();
    }

    void save() const {
        std::ostringstream f;
        for (bool fm : {false, true})
            for (auto& e : list(fm)) f << (fm ? "fast " : "arena ") << e.time << " " << e.difficulty << " " << e.name << "\n";
        if (!lastName.empty()) f << "lastName " << lastName << "\n";
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
