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
        case EnemyType::SERAPH:   return 60;
        case EnemyType::ANCHOR:   return 90;
        case EnemyType::WARDEN:   return 500;
        case EnemyType::SOVEREIGN: return 1000;
        case EnemyType::REVENANT: return 45;
        case EnemyType::WEAVER:   return 50;
        case EnemyType::LEVIATHAN: return 1500;
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
    int   bestArenaScore = 0;              // best ARENA score (Score.h)
    int   bestEndless = 0;                 // best ENDLESS score
    int   bestDaily = 0, dailyDate = 0;    // best DAILY score, and the day it's for (YYYYMMDD)
    bool  act2Unlocked = false;            // the Sovereign has fallen: ACT II is open

    static constexpr const char* kKey = "records.v2.cfg";

    void load() {
        std::istringstream f(persist::load(kKey));
        std::string key;
        while (f >> key) {
            if (key == "bestArena") f >> bestArena;
            else if (key == "bestFast") f >> bestFast;
            else if (key == "bestArenaScore") f >> bestArenaScore;
            else if (key == "bestEndless") f >> bestEndless;
            else if (key == "bestDaily") f >> bestDaily >> dailyDate;
            else if (key == "act2Unlocked") { int v = 0; f >> v; act2Unlocked = v != 0; }
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
        f << "bestArenaScore " << bestArenaScore << "\n";
        f << "bestEndless " << bestEndless << "\n";
        f << "bestDaily " << bestDaily << " " << dailyDate << "\n";
        f << "act2Unlocked " << (act2Unlocked ? 1 : 0) << "\n";
        f << "fastSplits " << fastSplits.size();
        for (float s : fastSplits) f << " " << s;
        f << "\n";
        persist::save(kKey, f.str());
    }
};

// The main menu's ACT II row only starts a run once the Sovereign has fallen
inline bool canStartAct2(const Records& r) { return r.act2Unlocked; }

// Finished runs a player chose to put their name to, fastest first, kept
// per mode (the top KEEP of each). Only full runs count: not one started at
// a later arena, in god mode or from the dev level select.
// The leaderboards, one per mode. ARENA, ENDLESS and DAILY rank by score
// (Score.h), FAST (a time trial) by time. DAILY keeps only today's board.
enum class Board { ARENA, FAST, ENDLESS, DAILY, COUNT };
inline const char* boardKey(Board b) {
    static const char* K[] = {"arena", "fast", "endless", "daily"};
    return K[(int)b];
}

struct Leaderboard {
    struct Entry { std::string name; float time = 0.f; int difficulty = 1; int score = 0; int wave = 0; };
    static constexpr int KEEP = 10;
    static constexpr int MAX_NAME_LEN = 12;
    std::vector<Entry> lists[(int)Board::COUNT];
    std::string daily;      // today's DAILY key (YYYYMMDD): other days' entries are dropped
    std::string lastName;   // offered again next time

    static constexpr const char* kKey    = "leaderboard.v2.cfg";
    static constexpr const char* kOldKey = "leaderboard.cfg";   // time-only boards, before scores

    std::vector<Entry>&       list(Board b)       { return lists[(int)b]; }
    const std::vector<Entry>& list(Board b) const { return lists[(int)b]; }
    static bool byScore(Board b) { return b != Board::FAST; }
    // Does a rank above b on board `bd`?
    static bool ahead(Board bd, const Entry& a, const Entry& b) {
        return byScore(bd) ? a.score > b.score : a.time < b.time;
    }

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
    static bool valid(Board b, const Entry& e) { return byScore(b) ? e.score > 0 : e.time > 0.f; }

    // Where a run would place (0-based), or -1 if it wouldn't make the board
    int placeFor(Board b, const Entry& e) const {
        if (!valid(b, e)) return -1;
        const auto& l = list(b);
        int i = 0;
        while (i < (int)l.size() && !ahead(b, e, l[i])) ++i;
        return i < KEEP ? i : -1;
    }
    // Returns the entry's place, or -1 (no name, or not good enough)
    int add(Board b, const std::string& rawName, Entry e) {
        e.name = cleanName(rawName);
        if (e.name.empty()) return -1;
        int at = placeFor(b, e);
        if (at < 0) return -1;
        auto& l = list(b);
        l.insert(l.begin() + at, e);
        if ((int)l.size() > KEEP) l.resize(KEEP);
        lastName = e.name;
        return at;
    }

    // One entry per line. FAST: "fast TIME DIFF NAME"; the others:
    // "arena|endless SCORE TIME DIFF WAVE NAME", "daily DATE SCORE TIME DIFF WAVE NAME"
    static bool parseScored(std::istringstream& ls, Entry& e) {
        if (!(ls >> e.score >> e.time >> e.difficulty >> e.wave)) return false;
        std::string rest; std::getline(ls, rest);
        e.name = cleanName(rest);
        return !e.name.empty();
    }
    static bool parseTimed(std::istringstream& ls, Entry& e) {
        if (!(ls >> e.time >> e.difficulty)) return false;
        std::string rest; std::getline(ls, rest);
        e.name = cleanName(rest);
        return !e.name.empty() && e.time > 0.f;
    }
    void clear() { for (auto& l : lists) l.clear(); }
    void sortAll() {
        for (int b = 0; b < (int)Board::COUNT; ++b) {
            Board bd = (Board)b;
            std::stable_sort(lists[b].begin(), lists[b].end(), [bd](const Entry& x, const Entry& y) { return ahead(bd, x, y); });
            if ((int)lists[b].size() > KEEP) lists[b].resize(KEEP);
        }
    }

    void load() {
        clear();
        std::string text = persist::load(kKey);
        bool migrate = text.empty();
        if (migrate) text = persist::load(kOldKey);   // keep the FAST times from before scores
        std::istringstream f(text);
        std::string line;
        while (std::getline(f, line)) {
            std::istringstream ls(line);
            std::string key; ls >> key;
            Entry e;
            if (key == "fast") { if (parseTimed(ls, e)) list(Board::FAST).push_back(e); }
            else if (key == "arena" && !migrate) { if (parseScored(ls, e)) list(Board::ARENA).push_back(e); }
            else if (key == "endless") { if (parseScored(ls, e)) list(Board::ENDLESS).push_back(e); }
            else if (key == "daily") {
                std::string date; ls >> date;
                if (date == daily && parseScored(ls, e)) list(Board::DAILY).push_back(e);
            } else if (key == "lastName") {
                std::string rest; std::getline(ls, rest);
                lastName = cleanName(rest);
            }
        }
        sortAll();
    }

    // The shared (online) board, as the web page caches it: per mode, lines of
    // "TIME DIFF NAME" (FAST) or "SCORE TIME DIFF WAVE NAME". Empty off the web
    // or with no API: callers then show this browser's own board.
    static constexpr const char* kOnlineKey = "leaderboard.online.";
    bool loadOnline() {
        clear();
        bool any = false;
        for (int b = 0; b < (int)Board::COUNT; ++b) {
            Board bd = (Board)b;
            std::string key = std::string(kOnlineKey) + boardKey(bd) + (bd == Board::DAILY ? "." + daily : "");
            std::istringstream f(persist::load(key));
            std::string line;
            while (std::getline(f, line) && (int)list(bd).size() < KEEP) {
                std::istringstream ls(line);
                Entry e;
                if (byScore(bd) ? parseScored(ls, e) : parseTimed(ls, e)) { list(bd).push_back(e); any = true; }
            }
        }
        return any;
    }

    void save() const {
        std::ostringstream f;
        for (auto& e : list(Board::FAST)) f << "fast " << e.time << " " << e.difficulty << " " << e.name << "\n";
        for (Board b : {Board::ARENA, Board::ENDLESS, Board::DAILY})
            for (auto& e : list(b)) {
                f << boardKey(b) << " ";
                if (b == Board::DAILY) f << daily << " ";
                f << e.score << " " << e.time << " " << e.difficulty << " " << e.wave << " " << e.name << "\n";
            }
        if (!lastName.empty()) f << "lastName " << lastName << "\n";
        // Other days' DAILY entries go: only today's board is kept
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
