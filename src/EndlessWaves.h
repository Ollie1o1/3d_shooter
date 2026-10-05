#pragma once
// =============================================================================
// EndlessWaves.h — generates ENDLESS (and DAILY) waves, one at a time, from a
// seed, so the same seed always gives the same run, on any machine. No OpenGL.
//
// Each wave has a budget that grows with its number and spends it on enemies
// that have unlocked by then (Husks and Rippers from the start, Sentinels and
// Mites next, Raptors, Shieldbearers (often with a Sentinel), Brutes,
// Conductors, Juggernauts with Mites at their feet). Every 4th wave is an
// objective wave borrowed from the arena's own (hold, conduits, survive);
// every 10th is a heavy wave: the Warden in the Core, two Juggernauts elsewhere.
// =============================================================================
#include "Level.h"
#include <random>
#include <cstdint>
#include <vector>
#include <algorithm>

class EndlessWaves {
public:
    float budgetScale = 1.f;        // DAILY's SWARM: bigger waves
    bool  bossArena   = false;      // the Core: its 10th waves bring the Warden

    void begin(uint32_t seed, std::vector<WaveGoal> arenaGoals, bool core) {
        rng.seed(seed);
        goals.clear();
        for (auto& g : arenaGoals) if (g.kind != WaveGoal::KILL_ALL) goals.push_back(g);
        bossArena = core;
    }

    static float budgetFor(int n) { return 10.f + 3.2f * n; }

    // Wave n (0-based): its enemies and its goal
    std::pair<std::vector<WaveEntry>, WaveGoal> wave(int n) {
        std::vector<WaveEntry> out;
        WaveGoal goal;
        if (n % 10 == 9) {   // heavy wave
            if (bossArena) out.push_back({EnemyType::WARDEN, 1});
            else out.push_back(WaveEntry(EnemyType::JUGGERNAUT, 2).with({EnemyType::MITE, EnemyType::MITE}));
            spend(out, n, budgetFor(n) * 0.4f * budgetScale);
            return {merge(out), goal};
        }
        if (n % 4 == 3) {    // objective wave
            if (!goals.empty()) goal = goals[(n / 4) % goals.size()];
            else goal = WaveGoal::survive("SURVIVE", 30.f);
            if (goal.kind == WaveGoal::SURVIVE) goal.seconds = std::min(30.f + n, 60.f);
        }
        spend(out, n, budgetFor(n) * budgetScale);
        return {merge(out), goal};
    }

private:
    std::mt19937 rng;
    std::vector<WaveGoal> goals;

    struct Kind { EnemyType type; int cost; int from; int maxPer; };   // maxPer 0: no cap
    static const std::vector<Kind>& kinds() {
        static const std::vector<Kind> K = {
            {EnemyType::HUSK, 2, 0, 0},       {EnemyType::RIPPER, 2, 0, 0},
            {EnemyType::MITE, 1, 1, 0},       {EnemyType::SENTINEL, 3, 1, 4},
            {EnemyType::RAPTOR, 3, 2, 4},     {EnemyType::SHIELDBEARER, 4, 3, 3},
            {EnemyType::BRUTE, 6, 4, 2},      {EnemyType::CONDUCTOR, 5, 5, 2},
            {EnemyType::JUGGERNAUT, 10, 7, 2},
        };
        return K;
    }

    void spend(std::vector<WaveEntry>& out, int n, float budget) {
        std::vector<int> count(kinds().size(), 0);
        for (int guard = 0; budget >= 1.f && guard < 400; ++guard) {
            std::vector<int> ok;
            for (int i = 0; i < (int)kinds().size(); ++i) {
                const Kind& k = kinds()[i];
                if (n < k.from || k.cost > budget) continue;
                if (k.maxPer && count[i] >= k.maxPer + n / 10) continue;
                ok.push_back(i);
            }
            if (ok.empty()) break;
            int i = ok[pick((int)ok.size())];
            const Kind& k = kinds()[i];
            ++count[i];
            budget -= (float)k.cost;
            // Some arrive as squads
            if (k.type == EnemyType::SHIELDBEARER && budget >= 3.f && coin(0.5f)) {
                out.push_back(WaveEntry(k.type, 1).with({EnemyType::SENTINEL})); budget -= 3.f;
            } else if (k.type == EnemyType::JUGGERNAUT && budget >= 2.f) {
                out.push_back(WaveEntry(k.type, 1).with({EnemyType::MITE, EnemyType::MITE})); budget -= 2.f;
            } else {
                out.push_back({k.type, 1});
            }
        }
    }
    // Straight from the generator's raw output (which the standard fixes), not
    // std's distributions (which differ between libraries): a DAILY must be
    // the same run on every machine
    int  pick(int n) { return (int)(rng() % (uint32_t)n); }
    bool coin(float p) { return (rng() % 10000u) < (uint32_t)(p * 10000.f); }

    // Fold single entries of the same type together (squads stay separate)
    static std::vector<WaveEntry> merge(const std::vector<WaveEntry>& in) {
        std::vector<WaveEntry> out;
        for (auto& e : in) {
            bool done = false;
            if (e.escort.empty())
                for (auto& o : out)
                    if (o.type == e.type && o.escort.empty() && o.at.empty()) { o.count += e.count; done = true; break; }
            if (!done) out.push_back(e);
        }
        return out;
    }
};
