#pragma once
#include <SDL2/SDL.h>

class GameState {
public:
    virtual ~GameState() = default;
    virtual void handleEvent(const SDL_Event& e) = 0;
    virtual void update(float dt) = 0;
    virtual void render() = 0;
};

// ARENA: the four-arena wave run ending with the boss.
// FAST:  the Descent, a time trial through hand-placed fights (see LevelDescent.h).
enum class GameMode { ARENA, FAST };
