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
// DAILY:   an ENDLESS run that's the same for everyone today: the date picks
//          the arena, a modifier and the waves (Daily.h).
enum class GameMode { ARENA, FAST, ENDLESS, DAILY };

// Where a run starts. The dev level select fills it in; a normal start from
// the menu is the default (first arena, ranked).
struct StartOptions {
    int  arena = 0;          // arena / FAST room index
    int  wave  = 0;          // wave within it (ARENA)
    bool god   = false;
    bool practice = false;   // no records, no leaderboard
};
