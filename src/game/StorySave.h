#pragma once
#include "game/Progression.h"
#include <iomanip>

// Save checkpoint state, never a position inside a hazard or half-finished fight.
// Practice, boss selection and recording runs never write this slot.
struct StorySave {
    static inline const char* key = "story.v1.cfg";
    int chapter = 1, checkpoint = 0, owned = 0;
    unsigned logs = 0, terminals = 0;
    bool relayIsolated = false;
    bool grapple = false, bossDown = false, completed = false, wakeSeen = false;
    Progression prog;
    float elapsed = 0, style = 0, damage = 0, peak = 0;
    int kills = 0, shots = 0, hits = 0, deaths = 0;

    std::string text() const {
        std::ostringstream out;
        out << std::setprecision(9) << "OVERDRIVE_STORY 1\n"
            << chapter << ' ' << checkpoint << ' ' << owned << ' ' << logs << ' ' << terminals << ' '
            << relayIsolated << ' ' << grapple << ' ' << bossDown << ' ' << completed << ' ' << wakeSeen << '\n'
            << prog.level << ' ' << prog.xp << ' ' << prog.points << ' ' << prog.totalXp << '\n';
        for (const auto& u : prog.up)
            out << u.tier[0] << ' ' << u.tier[1] << ' ' << u.tier[2] << ' ' << u.mod << ' ' << u.alternate << '\n';
        out << elapsed << ' ' << style << ' ' << damage << ' ' << peak << ' '
            << kills << ' ' << shots << ' ' << hits << ' ' << deaths << '\n';
        return out.str();
    }
    bool parse(const std::string& text) {
        StorySave candidate;
        std::istringstream in(text);
        std::string magic, extra; int version = 0;
        if (!(in >> magic >> version) || magic != "OVERDRIVE_STORY" || version != 1) return false;
        auto& s = candidate;
        if (!(in >> s.chapter >> s.checkpoint >> s.owned >> s.logs >> s.terminals
                 >> s.relayIsolated >> s.grapple >> s.bossDown >> s.completed >> s.wakeSeen
                 >> s.prog.level >> s.prog.xp >> s.prog.points >> s.prog.totalXp)) return false;
        for (auto& u : s.prog.up) {
            if (!(in >> u.tier[0] >> u.tier[1] >> u.tier[2] >> u.mod >> u.alternate)) return false;
            for (int tier : u.tier) if (tier < 0 || tier > MAX_TIER) return false;
        }
        if (!(in >> s.elapsed >> s.style >> s.damage >> s.peak >> s.kills >> s.shots >> s.hits >> s.deaths) || in >> extra) return false;
        if (s.chapter < 1 || s.chapter > 2 || s.checkpoint < 0 || s.checkpoint > (s.chapter == 1 ? 8 : 7)
            || s.owned < 0 || s.owned >= (1 << WEAPON_COUNT)
            || s.prog.level < 1 || s.prog.level > 10000 || s.prog.points < 0 || s.prog.points > 100000
            || s.prog.xp < 0 || s.prog.xp >= Progression::xpToNext(s.prog.level) || s.prog.totalXp < 0
            || !(s.elapsed >= 0 && s.elapsed <= 1e8f) || !(s.style >= 0 && s.style <= 1e9f)
            || !(s.damage >= 0 && s.damage <= 1e9f) || !(s.peak >= 0 && s.peak <= 100.f)
            || s.kills < 0 || s.shots < 0 || s.hits < 0 || s.deaths < 0) return false;
        *this = candidate;
        return true;
    }
    bool load() { return parse(persist::load(key)); }
    void save() const { persist::save(key, text()); }
};
