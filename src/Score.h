#pragma once
// =============================================================================
// Score.h — a run's score, for the leaderboards. No OpenGL.
//
//   ARENA            style earned + 10 a second under par − 2 a point of damage
//                    taken, x difficulty
//   ENDLESS / DAILY  style earned + 500 a wave cleared, x difficulty
//
// "Style earned" is every point the style meter took in over the run, after
// freshness (StyleSystem::earned), so playing varied and fast scores, and
// sitting back doesn't. FAST stays a pure time trial (its board is by time).
// =============================================================================
#include <algorithm>
#include <cmath>

struct RunScore {
    int   style = 0, time = 0, damage = 0, waves = 0;   // the parts (damage is subtracted)
    float mult = 1.f;                                    // difficulty
    int   total = 0;
};

constexpr float ARENA_PAR      = 900.f;   // s: a full ARENA run in 15 minutes is par
constexpr int   TIME_POINTS    = 10;      // a second under par
constexpr int   DAMAGE_POINTS  = 2;       // a point of damage taken
constexpr int   WAVE_POINTS    = 500;     // a wave cleared (ENDLESS / DAILY)

inline float difficultyScoreMult(int d) {
    static const float M[] = {0.75f, 1.f, 1.25f, 1.5f};   // LENIENT .. BRUTAL
    return M[std::clamp(d, 0, 3)];
}

inline RunScore arenaScore(float styleEarned, float time, float damageTaken, int difficulty) {
    RunScore s;
    s.style  = (int)std::lround(styleEarned);
    s.time   = (int)std::lround(std::max(0.f, ARENA_PAR - time) * TIME_POINTS);
    s.damage = (int)std::lround(damageTaken * DAMAGE_POINTS);
    s.mult   = difficultyScoreMult(difficulty);
    s.total  = (int)std::lround(std::max(0, s.style + s.time - s.damage) * s.mult);
    return s;
}

inline RunScore endlessScore(float styleEarned, int wavesCleared, int difficulty) {
    RunScore s;
    s.style = (int)std::lround(styleEarned);
    s.waves = std::max(0, wavesCleared) * WAVE_POINTS;
    s.mult  = difficultyScoreMult(difficulty);
    s.total = (int)std::lround((s.style + s.waves) * s.mult);
    return s;
}
