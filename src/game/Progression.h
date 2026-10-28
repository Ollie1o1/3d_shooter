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
#include "player/Weapons.h"
#include "enemies/Enemy.h"
#include "player/StyleSystem.h"
#include "core/Persist.h"
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
        case EnemyType::ABSOLUTE: return 2000;
        case EnemyType::PACEMAKER: return 1200;
        case EnemyType::COURIER: case EnemyType::BULWARK: case EnemyType::CANTOR:
        case EnemyType::HARRIER: case EnemyType::PLUMB: case EnemyType::FOREMAN: return 300;   // the Gauntlet's mini-bosses
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

// BOSS (the main menu): every boss in the order you meet them, each fought on its own, straight
// into its fight with the upgrades a run would have by then (spent in the armory first). THE
// ABSOLUTE's first form ends as the eclipse takes him; MORNINGSTAR starts inside it
struct BossEntry { const char* name; const char* where; bool act2; int arena; bool morning; int level, points; };
constexpr int BOSS_COUNT = 6;
inline const BossEntry& bossEntry(int i) {
    static const BossEntry B[BOSS_COUNT] = {
        {"THE WARDEN",    "THE CORE",                  false, 3, false, 4, 3},
        {"THE SOVEREIGN", "THE SANCTUM",               false, 4, false, 5, 4},
        {"THE PENITENT",  "ACT II - THE DESCENT",      true,  2, false, 6, 6},
        {"THE LEVIATHAN", "ACT II - THE MAW",          true,  4, false, 6, 8},
        {"THE ABSOLUTE",  "!!!!! - HIS FIRST FORM",    true,  5, false, 6, 9},
        {"MORNINGSTAR",   "!!!!! - INSIDE THE ECLIPSE", true, 5, true,  6, 9},
    };
    return B[std::clamp(i, 0, BOSS_COUNT - 1)];
}

// Best times, kept across runs (file on desktop, localStorage on the web).
// The key carries a version: bump it when the maps change enough that old
// times no longer mean anything (v2: the Gauntlet's rooms, the Sanctum).
// A FAST room's medal for a time (Arena::medals: gold, silver, bronze, dev): 0 none .. 4 dev
inline int medalFor(const float m[4], float t) {
    if (m[3] > 0.f && t <= m[3]) return 4;
    for (int k = 0; k < 3; ++k) if (m[k] > 0.f && t <= m[k]) return 3 - k;
    return 0;
}
// The Gauntlet's layout, as far as times go. Bumped when its rooms change enough that old FAST
// times can't be compared (3: two waves, then the mini-boss, in every room); older bests, medals,
// splits, the ghost and the board's FAST times are dropped as they load
constexpr int FAST_COURSE = 3;
inline const char* medalName(int m) { static const char* N[] = {"", "BRONZE", "SILVER", "GOLD", "DEV TIME"}; return N[m < 0 || m > 4 ? 0 : m]; }
inline glm::vec3 medalColor(int m) {
    static const glm::vec3 C[] = {{0.6f, 0.6f, 0.65f}, {0.85f, 0.5f, 0.25f}, {0.8f, 0.85f, 0.95f}, {1.f, 0.82f, 0.25f}, {0.45f, 1.f, 0.9f}};
    return C[m < 0 || m > 4 ? 0 : m];
}

struct Records {
    float bestArena = 0.f;                 // full Arena run, seconds (0 = none yet)
    float bestFast  = 0.f;                 // FAST mode total
    std::vector<float> fastSplits;         // cumulative time at each section clear, from the best run
    std::vector<float> fastRooms;          // FAST: each room's best time on its own (fight start to clear; 0 = none)
    std::vector<int>   fastMedals;         // FAST: each room's best medal (0 none, 1 bronze, 2 silver, 3 gold, 4 dev)
    int   bestArenaScore = 0;              // best ARENA score (Score.h)
    int   bestEndless = 0;                 // best ENDLESS score
    bool  act2Unlocked = false;            // the Sovereign has fallen: ACT II is open
    bool  act2Cleared = false;             // the Leviathan has fallen (saved as it dies)
    float bestAct2 = 0.f, bestZenith = 0.f;   // ACT II to the Leviathan; ACT II + the Zenith (0 = none yet)
    float bestBoss[BOSS_COUNT] = {};          // BOSS: each boss's fastest kill, from its fight starting (0 = none yet)
    std::vector<float> arenaSolo, act2Solo;   // the ARENA page: each arena's best clear on its own (Act I, Act II; 0 = none yet)
    int   fastCourse = FAST_COURSE;           // the Gauntlet layout the FAST records were set on

    static constexpr const char* kKey = "records.v2.cfg";

    void load() { parse(persist::load(kKey)); }
    void parse(const std::string& text) {
        std::istringstream f(text);
        std::string key;
        int course = 0;
        while (f >> key) {
            if (key == "bestArena") f >> bestArena;
            else if (key == "fastCourse") f >> course;
            else if (key == "bestFast") f >> bestFast;
            else if (key == "bestArenaScore") f >> bestArenaScore;
            else if (key == "bestEndless") f >> bestEndless;
            else if (key == "bestDaily") { std::string a, b; f >> a >> b; }   // the DAILY mode's (removed): old files still carry it
            else if (key == "act2Unlocked") { int v = 0; f >> v; act2Unlocked = v != 0; }
            else if (key == "act2Cleared") { int v = 0; f >> v; act2Cleared = v != 0; }
            else if (key == "bestAct2") f >> bestAct2;
            else if (key == "bestZenith") f >> bestZenith;
            else if (key == "bestBoss") { int n = 0; f >> n; for (int i = 0; i < n; ++i) { float v = 0.f; f >> v; if (i < BOSS_COUNT) bestBoss[i] = v; } }
            else if (key == "fastSplits") {
                int n = 0; f >> n;
                fastSplits.assign(std::max(0, std::min(n, 32)), 0.f);
                for (auto& s : fastSplits) f >> s;
            } else if (key == "fastRooms") {
                int n = 0; f >> n;
                fastRooms.assign(std::max(0, std::min(n, 32)), 0.f);
                for (auto& s : fastRooms) f >> s;
            } else if (key == "arenaSolo" || key == "act2Solo") {
                auto& v = key == "arenaSolo" ? arenaSolo : act2Solo;
                int n = 0; f >> n;
                v.assign(std::max(0, std::min(n, 32)), 0.f);
                for (auto& s : v) f >> s;
            } else if (key == "fastMedals") {
                int n = 0; f >> n;
                fastMedals.assign(std::max(0, std::min(n, 32)), 0);
                for (auto& m : fastMedals) f >> m;
            } else { std::string skip; f >> skip; }
            if (!f) break;
        }
        if (course != FAST_COURSE && !text.empty()) {   // set on an older Gauntlet: not comparable
            bestFast = 0.f; fastSplits.clear(); fastRooms.clear(); fastMedals.clear();
        }
        fastCourse = FAST_COURSE;
    }
    void save() const { persist::save(kKey, serialise()); }
    std::string serialise() const {
        std::ostringstream f;
        f << "bestArena " << bestArena << "\n";
        f << "bestFast " << bestFast << "\n";
        f << "fastCourse " << fastCourse << "\n";
        f << "bestArenaScore " << bestArenaScore << "\n";
        f << "bestEndless " << bestEndless << "\n";
        f << "act2Unlocked " << (act2Unlocked ? 1 : 0) << "\n";
        f << "act2Cleared " << (act2Cleared ? 1 : 0) << "\n";
        f << "bestAct2 " << bestAct2 << "\n";
        f << "bestZenith " << bestZenith << "\n";
        f << "bestBoss " << BOSS_COUNT;
        for (float b : bestBoss) f << " " << b;
        f << "\n";
        f << "fastSplits " << fastSplits.size();
        for (float s : fastSplits) f << " " << s;
        f << "\n";
        f << "fastRooms " << fastRooms.size();
        for (float s : fastRooms) f << " " << s;
        f << "\n";
        f << "fastMedals " << fastMedals.size();
        for (int m : fastMedals) f << " " << m;
        f << "\n";
        f << "arenaSolo " << arenaSolo.size();
        for (float s : arenaSolo) f << " " << s;
        f << "\n";
        f << "act2Solo " << act2Solo.size();
        for (float s : act2Solo) f << " " << s;
        f << "\n";
        return f.str();
    }
    // A FAST room's time: kept if it's its best (and its medal if better); true if it was a new best
    bool roomTime(int room, float t, int medal) {
        if (room < 0 || room >= 32) return false;
        if ((int)fastRooms.size() <= room) fastRooms.resize(room + 1, 0.f);
        if ((int)fastMedals.size() <= room) fastMedals.resize(room + 1, 0);
        fastMedals[room] = std::max(fastMedals[room], medal);
        if (fastRooms[room] > 0.f && t >= fastRooms[room]) return false;
        fastRooms[room] = t;
        return true;
    }

    // The ARENA page: an arena cleared on its own; kept if it's its best (true: a new best)
    bool soloTime(bool act2, int arena, float t) {
        auto& v = act2 ? act2Solo : arenaSolo;
        if (arena < 0 || arena >= 32 || t <= 0.f) return false;
        if ((int)v.size() <= arena) v.resize(arena + 1, 0.f);
        if (v[arena] > 0.f && t >= v[arena]) return false;
        v[arena] = t;
        return true;
    }
    float soloBest(bool act2, int arena) const {
        const auto& v = act2 ? act2Solo : arenaSolo;
        return arena >= 0 && arena < (int)v.size() ? v[arena] : 0.f;
    }
};

// The ARENA page's single arenas: the level and points a whole run would have by the
// arena (as the BOSS page's fights have), spent in the armory before it starts
struct SoloStart { int level, points; };
inline SoloStart soloStart(bool act2, int arena) {
    arena = std::max(0, arena);
    if (!act2) return {1 + arena, arena};
    return {6, std::min(5 + std::max(0, arena - 1), 9)};   // (Act II starts at 6 with 5 to spend)
}

// ACT II and the Zenith keep times only for a run from the act's start
// (not a level-select or --arena start, not god mode, not practice)
inline bool act2RunRecords(bool fromStart, bool god, bool practice) { return fromStart && !god && !practice; }
// ACT II's clear, saved the moment the Leviathan dies (whatever happens above)
inline void recordAct2Clear(Records& r, float t) {
    r.act2Cleared = true;
    if (r.bestAct2 <= 0.f || t < r.bestAct2) r.bestAct2 = t;
}
// ACT II + the Zenith: true when it's a new best
inline bool recordZenith(Records& r, float t) {
    if (r.bestZenith > 0.f && t >= r.bestZenith) return false;
    r.bestZenith = t; return true;
}
// The arena just cleared is the Maw, and the Zenith waits above it
inline bool zenithHandoff(bool act2, int clearedArena, int arenaCount) {
    return act2 && arenaCount == 6 && clearedArena == 4;
}

// The ARENA page's ACT II card only starts a whole-act run once the Sovereign has fallen in an Act I run
inline bool canStartAct2(const Records& r) { return r.act2Unlocked; }
// BOSS: every boss is open from the start (Oliver's call: no need to have reached it)
inline bool bossOpen(const Records&, int i) { return i >= 0 && i < BOSS_COUNT; }
// BOSS on the world board (the site's API, one board per boss, fastest first): its key, and its
// world record as the web page caches it ("TIME DIFFICULTY NAME" lines, best first). Off the web,
// or before the fetch lands, there's none (time 0)
#ifdef __EMSCRIPTEN__
constexpr bool WORLD_BOARD = true;    // the web build posts to the site's boards
#else
constexpr bool WORLD_BOARD = false;   // (desktop has none)
#endif
inline std::string bossBoardKey(int i) { return "boss" + std::to_string(std::clamp(i, 0, BOSS_COUNT - 1)); }
struct WorldBest { float time = 0.f; int difficulty = 0; std::string name; };
inline WorldBest parseWorldBest(const std::string& text) {
    WorldBest w;
    std::istringstream f(text);
    std::string line;
    if (std::getline(f, line)) {
        std::istringstream ls(line);
        float t = 0.f; int d = 0; std::string rest;
        if (ls >> t >> d && t > 0.f) { std::getline(ls, rest); w.time = t; w.difficulty = d; w.name = rest.empty() ? "" : rest.substr(1); }
    }
    return w;
}
inline WorldBest loadBossWorld(int i) { return parseWorldBest(persist::load("leaderboard.online." + bossBoardKey(i))); }

// A boss killed in t seconds: true when it's that boss's new best
inline bool recordBoss(Records& r, int i, float t) {
    if (i < 0 || i >= BOSS_COUNT || t <= 0.f) return false;
    if (r.bestBoss[i] > 0.f && t >= r.bestBoss[i]) return false;
    r.bestBoss[i] = t; return true;
}

// Finished runs a player chose to put their name to, fastest first, kept
// per mode (the top KEEP of each). Only full runs count: not one started at
// a later arena, in god mode or from the dev level select.
// The leaderboards, one per mode. ARENA and ENDLESS rank by score
// (Score.h), FAST (a time trial) by time.
enum class Board { ARENA, FAST, ENDLESS, COUNT };
inline const char* boardKey(Board b) {
    static const char* K[] = {"arena", "fast", "endless"};
    return K[(int)b];
}

struct Leaderboard {
    struct Entry { std::string name; float time = 0.f; int difficulty = 1; int score = 0; int wave = 0; };
    static constexpr int KEEP = 10;
    static constexpr int MAX_NAME_LEN = 12;
    std::vector<Entry> lists[(int)Board::COUNT];
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
    // "arena|endless SCORE TIME DIFF WAVE NAME" (old files' "daily" lines are skipped)
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
        int course = 0;
        while (std::getline(f, line)) {
            std::istringstream ls(line);
            std::string key; ls >> key;
            Entry e;
            if (key == "fastCourse") ls >> course;
            else if (key == "fast") { if (parseTimed(ls, e)) list(Board::FAST).push_back(e); }
            else if (key == "arena" && !migrate) { if (parseScored(ls, e)) list(Board::ARENA).push_back(e); }
            else if (key == "endless") { if (parseScored(ls, e)) list(Board::ENDLESS).push_back(e); }
            else if (key == "lastName") {
                std::string rest; std::getline(ls, rest);
                lastName = cleanName(rest);
            }
        }
        if (course != FAST_COURSE) list(Board::FAST).clear();   // times from an older Gauntlet
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
            std::string key = std::string(kOnlineKey) + boardKey(bd);
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
        f << "fastCourse " << FAST_COURSE << "\n";
        for (auto& e : list(Board::FAST)) f << "fast " << e.time << " " << e.difficulty << " " << e.name << "\n";
        for (Board b : {Board::ARENA, Board::ENDLESS})
            for (auto& e : list(b)) {
                f << boardKey(b) << " ";
                f << e.score << " " << e.time << " " << e.difficulty << " " << e.wave << " " << e.name << "\n";
            }
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

// The ARENA page's ACT II card: locked, the act, then its best times (the act to
// the Leviathan; with the Zenith once it's cleared)
inline std::string act2Subtitle(const Records& r, bool unlocked) {
    if (!unlocked) return "DEFEAT THE SOVEREIGN TO UNLOCK";
    std::string s = r.bestZenith > 0.f ? "5 ARENAS - AND WHAT WAITS ABOVE" : "5 ARENAS - BENEATH THE ECLIPSE";
    if (r.bestAct2 > 0.f) s += "   BEST " + formatTime(r.bestAct2);
    if (r.bestZenith > 0.f) s += "   !!!!! " + formatTime(r.bestZenith);   // (the encore's name: ZENITH_NAME)
    return s;
}
