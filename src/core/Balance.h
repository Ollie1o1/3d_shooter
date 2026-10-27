#pragma once
// =============================================================================
// Balance.h — the game's numbers in one place: every gun's stats, every
// enemy's health, speed and timing, OVERDRIVE and the style meter, and the
// four difficulties. Each is a named value pointing straight at the table the
// game already reads (weaponDefEdit, statsEdit, StyleSystem, difficultyEdit),
// so changing one changes the game at once.
//
// assets/balance.txt holds what's been changed from the code's defaults, one
// per line ("weapon.revolver.damage = 50"); the game and the tests both load
// it at startup, so the tests check the game as it ships. The balance panel
// (F1, ui/BalancePanel.h) edits these live and saves that file.
// No OpenGL: the tests use it.
// =============================================================================
#include "player/Weapons.h"
#include "player/StyleSystem.h"
#include "enemies/EnemyTypes.h"
#include "core/Difficulty.h"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

struct BalanceVar {
    std::string key;      // "weapon.revolver.damage": the file's name for it
    std::string group;    // "Weapons", "Enemies"...: the panel's section
    std::string owner;    // "REVOLVER", "HUSK"...: the panel's sub-section
    std::string label;    // "damage"
    float* f = nullptr;   // one of these two
    int*   i = nullptr;
    float lo = 0.f, hi = 1.f;
    float def = 0.f;      // the code's default
    const char* help = "";
    float get() const { return f ? *f : (float)*i; }
    void set(float v) const {
        v = std::clamp(v, lo, hi);
        if (f) *f = v; else *i = (int)std::lround(v);
    }
    bool changed() const { return std::fabs(get() - def) > 1e-6f * std::max(1.f, std::fabs(def)); }
};

inline const char* BALANCE_PATH = "assets/balance.txt";
inline bool g_balanceOpen = false;   // the panel (F1, ui/BalancePanel.h) is up: GameplayState freezes under it

inline std::string balanceSlug(const char* s) {
    std::string o;
    for (const char* p = s; *p; ++p) o += std::isalnum((unsigned char)*p) ? (char)std::tolower((unsigned char)*p) : '_';
    return o;
}

inline std::vector<BalanceVar>& balanceVars() {
    static std::vector<BalanceVar> V;
    static bool built = false;
    if (built) return V;
    built = true;
    auto addF = [&](std::string key, std::string group, std::string owner, std::string label, float& v, float lo, float hi, const char* help) {
        BalanceVar b; b.key = key; b.group = group; b.owner = owner; b.label = label; b.f = &v; b.lo = lo; b.hi = hi; b.def = v; b.help = help;
        V.push_back(b);
    };
    auto addI = [&](std::string key, std::string group, std::string owner, std::string label, int& v, int lo, int hi, const char* help) {
        BalanceVar b; b.key = key; b.group = group; b.owner = owner; b.label = label; b.i = &v; b.lo = (float)lo; b.hi = (float)hi; b.def = (float)v; b.help = help;
        V.push_back(b);
    };
    // ---- the guns ----
    for (int w = 0; w < WEAPON_COUNT; ++w) {
        WeaponDef& d = weaponDefEdit((WeaponId)w);
        std::string k = "weapon." + balanceSlug(d.name) + ".", G = "Weapons", O = d.name;
        addF(k + "damage", G, O, "damage", d.damage, 0.f, std::max(50.f, d.damage * 4.f), "per bullet / pellet");
        addI(k + "pellets", G, O, "pellets", d.pellets, 1, 30, "bullets per shot");
        addF(k + "cooldown", G, O, "time between shots", d.cooldown, 0.03f, 3.f, "seconds (the bolt cycle on the rifles)");
        addI(k + "mag", G, O, "magazine", d.magSize, 1, 30, "rounds (applies from the next reload)");
        addF(k + "reload", G, O, "reload time", d.reloadTime, 0.2f, 6.f, "seconds");
        addF(k + "spread_hip", G, O, "spread (hip)", d.spreadHip, 0.f, 0.4f, "max offset from the aim line");
        addF(k + "spread_aimed", G, O, "spread (aimed)", d.spreadAds, 0.f, 0.4f, "same, fully aimed");
        addF(k + "headshot", G, O, "headshot x", d.headMult, 1.f, 6.f, "headshot multiplier");
        addI(k + "pierce", G, O, "pierce", d.pierce, 0, 8, "extra enemies a shot passes through");
        addF(k + "recoil", G, O, "recoil", d.recoil, 0.f, 10.f, "camera kick, degrees");
        addF(k + "range", G, O, "range", d.range, 5.f, 400.f, "metres");
    }
    // ---- the enemies, then the bosses ----
    for (int pass = 0; pass < 2; ++pass)
    for (int t = 0; t < (int)EnemyType::COUNT; ++t) {
        if (isBoss((EnemyType)t) != (pass == 1)) continue;
        EnemyStats& s = statsEdit((EnemyType)t);
        std::string k = "enemy." + balanceSlug(s.name) + ".", G = isBoss((EnemyType)t) ? "Bosses" : "Enemies", O = s.name;
        addF(k + "health", G, O, "health", s.health, 1.f, std::max(200.f, s.health * 4.f), "applies to new spawns");
        addF(k + "speed", G, O, "speed", s.speed, 0.f, std::max(20.f, s.speed * 3.f), "m/s");
        addF(k + "windup", G, O, "wind-up", s.telegraph, 0.f, 3.f, "seconds of warning before an attack lands (0 on a type that doesn't attack)");
        addF(k + "attack_every", G, O, "attack every", s.attackEvery, 0.f, 12.f, "seconds between attacks (0 on a type that doesn't attack)");
    }
    // ---- OVERDRIVE and the style meter ----
    {
        std::string G = "OVERDRIVE & style", O = "OVERDRIVE";
        addF("overdrive.duration", G, O, "duration", StyleSystem::OVERDRIVE_DURATION, 1.f, 20.f, "seconds it lasts");
        addF("overdrive.max", G, O, "max (with kills)", StyleSystem::OVERDRIVE_MAX, 1.f, 30.f, "what kills can stretch it to");
        addF("overdrive.after", G, O, "meter after", StyleSystem::OVERDRIVE_AFTER, 0.f, 100.f, "where the meter lands when it burns out");
        addF("overdrive.cooldown", G, O, "cooldown", StyleSystem::OVERDRIVE_COOL, 0.f, 120.f, "seconds before it can start again (raise it if it comes too often)");
        O = "STYLE METER";
        addF("style.meter_gain", G, O, "meter gain", StyleSystem::METER_GAIN, 0.f, 2.f, "how fast style fills the meter");
        addF("style.hit_meter", G, O, "hit meter", StyleSystem::HIT_METER, 0.f, 2.f, "meter from landing hits");
    }
    // ---- the difficulties ----
    for (int l = 0; l < DIFFICULTY_LEVELS; ++l) {
        DifficultyTuning& d = difficultyEdit(l);
        std::string k = "difficulty." + balanceSlug(d.name) + ".", G = "Difficulty", O = d.name;
        addF(k + "damage", G, O, "enemy damage x", d.damage, 0.f, 4.f, "");
        addF(k + "health", G, O, "enemy health x", d.health, 0.1f, 4.f, "");
        addF(k + "attack_rate", G, O, "attack rate x", d.attackRate, 0.1f, 4.f, "");
        addF(k + "shot_speed", G, O, "shot speed x", d.shotSpeed, 0.1f, 4.f, "");
        addF(k + "lead", G, O, "lead", d.lead, 0.f, 1.f, "0: shoot where you are, 1: where you'll be");
        addF(k + "windup", G, O, "wind-up x", d.windup, 0.2f, 3.f, "shorter = less time to react");
        addF(k + "move", G, O, "move speed x", d.moveSpeed, 0.1f, 3.f, "");
        addF(k + "heal", G, O, "healing x", d.heal, 0.f, 3.f, "health back from hits and kills");
        addF(k + "drops", G, O, "health drops x", d.drops, 0.f, 3.f, "");
        addF(k + "wave_size", G, O, "wave size x", d.waveSize, 0.2f, 4.f, "ARENA: enemies per wave");
        addI(k + "max_alive_bonus", G, O, "+ alive at once", d.maxAliveBonus, 0, 16, "ARENA");
    }
    return V;
}
inline BalanceVar* balanceFind(const std::string& key) {
    for (auto& b : balanceVars()) if (b.key == key) return &b;
    return nullptr;
}

// Apply a balance file's text: "key = value" lines, '#' comments. Unknown keys
// and bad lines are listed in `problems` (and skipped); values are clamped to
// the slider's range. Returns how many were set.
inline int balanceApply(const std::string& text, std::vector<std::string>* problems = nullptr) {
    int set = 0, lineNo = 0;
    std::istringstream in(text);
    std::string line;
    while (std::getline(in, line)) {
        ++lineNo;
        size_t hash = line.find('#');
        if (hash != std::string::npos) line.resize(hash);
        auto trim = [](std::string s) {
            size_t a = s.find_first_not_of(" \t\r"), b = s.find_last_not_of(" \t\r");
            return a == std::string::npos ? std::string() : s.substr(a, b - a + 1);
        };
        line = trim(line);
        if (line.empty()) continue;
        size_t eq = line.find('=');
        std::string key = eq == std::string::npos ? line : trim(line.substr(0, eq));
        BalanceVar* b = balanceFind(key);
        char* end = nullptr;
        std::string val = eq == std::string::npos ? "" : trim(line.substr(eq + 1));
        float v = val.empty() ? 0.f : std::strtof(val.c_str(), &end);
        if (!b) { if (problems) problems->push_back("line " + std::to_string(lineNo) + ": no '" + key + "'"); continue; }
        if (val.empty() || !end || *end) { if (problems) problems->push_back("line " + std::to_string(lineNo) + ": '" + val + "' isn't a number"); continue; }
        if (v < b->lo || v > b->hi) { if (problems) problems->push_back("line " + std::to_string(lineNo) + ": " + key + " " + val + " is outside " + std::to_string(b->lo) + "..." + std::to_string(b->hi) + " (clamped)"); }
        b->set(v);
        ++set;
    }
    return set;
}
inline void balanceResetAll() { for (auto& b : balanceVars()) b.set(b.def); }

// Load the file over the code's defaults (missing file: just the defaults)
inline bool balanceLoad(const std::string& path = BALANCE_PATH, std::vector<std::string>* problems = nullptr) {
    balanceVars();   // (the defaults are captured before anything changes them)
    std::ifstream f(path);
    if (!f) return false;
    std::stringstream ss; ss << f.rdbuf();
    balanceResetAll();
    balanceApply(ss.str(), problems);
    return true;
}
// The file: everything that differs from the code, grouped, each with its default beside it
inline std::string balanceText() {
    std::string o = "# OVERDRIVE balance: what differs from the code's defaults (src/core/Balance.h).\n"
                    "# Loaded at startup by the game and the tests. Edit here or in the game (F1).\n";
    std::string group;
    for (const auto& b : balanceVars()) {
        if (!b.changed()) continue;
        if (b.group != group) { group = b.group; o += "\n# " + group + "\n"; }
        char buf[200];
        if (b.i) std::snprintf(buf, sizeof buf, "%s = %d   # default %d\n", b.key.c_str(), *b.i, (int)b.def);
        else     std::snprintf(buf, sizeof buf, "%s = %g   # default %g\n", b.key.c_str(), *b.f, b.def);
        o += buf;
    }
    return o;
}
inline bool balanceSave(const std::string& path = BALANCE_PATH) {
    std::ofstream f(path);
    if (!f) return false;
    f << balanceText();
    return (bool)f;
}
// Once, at startup (the game's main and each test's main)
inline void balanceInit() {
    static bool done = false;
    if (done) return;
    done = true;
    std::vector<std::string> problems;
    balanceLoad(BALANCE_PATH, &problems);
    for (auto& p : problems) std::fprintf(stderr, "balance: %s: %s\n", BALANCE_PATH, p.c_str());
}
