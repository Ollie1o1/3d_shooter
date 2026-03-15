#pragma once
#include <SDL2/SDL.h>

class GameState {
public:
    virtual ~GameState() = default;
    virtual void handleEvent(const SDL_Event& e) = 0;
    virtual void update(float dt) = 0;
    virtual void render() = 0;
};
