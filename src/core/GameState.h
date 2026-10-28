#pragma once
#include <SDL2/SDL.h>

class GameState {
public:
    virtual ~GameState() = default;
    virtual void handleEvent(const SDL_Event& e) = 0;
    virtual void update(float dt) = 0;
    virtual void render() = 0;
};

// ARENA:   the wave run through every arena, ending with the boss.
// FAST:    the Gauntlet, a time trial through seven rooms of hand-placed fights (LevelGauntlet.h).
// ENDLESS: generated waves in the Core until you die (EndlessWaves.h).
// ACT2:    Act II, beneath the eclipse (LevelAct2.h). Unlocked by killing the Sovereign.
enum class GameMode { ARENA, FAST, ENDLESS, ACT2 };

// Where a run starts. The dev level select fills it in; a normal start from
// the menu is the default (first arena, ranked).
struct StartOptions {
    int  arena = 0;          // arena / FAST room index
    int  wave  = 0;          // wave within it (ARENA)
    bool god   = false;
    bool practice = false;   // no records, no leaderboard
    int  boss = -1;          // BOSS: which boss (bossEntry, Progression.h), fought on its own
    int  room = -1;          // FAST: ROOM ATTACK, one room on its own (-1: the whole Gauntlet)
    int  solo = -1;          // ARENA / ACT II: one arena on its own (the ARENA page; -1: the whole act)
};
