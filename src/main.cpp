#include <SDL2/SDL.h>
#include "gl.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <functional>

#include "GameState.h"
#include "Settings.h"
#include "MenuState.h"
#include "GameplayState.h"
#include "AudioSystem.h"

int main(int /*argc*/, char* /*argv*/[]) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0)
        throw std::runtime_error(SDL_GetError());

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE,   24);

    SDL_Window* window = SDL_CreateWindow(
        "OVERDRIVE",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        SCREEN_W, SCREEN_H,
        SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
    if (!window) throw std::runtime_error(SDL_GetError());

    SDL_GLContext ctx = SDL_GL_CreateContext(window);
    if (!ctx) throw std::runtime_error(SDL_GetError());

#ifndef __APPLE__
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK)
        throw std::runtime_error("Failed to initialize GLEW");
#endif

    SDL_GL_SetSwapInterval(0); // vsync off — we do our own frame cap
    glViewport(0, 0, SCREEN_W, SCREEN_H);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    AudioSystem audio;
    audio.loadSound("jump",        "assets/sfx/jump.wav");
    audio.loadSound("land",        "assets/sfx/land.wav");
    audio.loadSound("dash",        "assets/sfx/dash.wav");
    audio.loadSound("slam",        "assets/sfx/slam.wav");
    audio.loadSound("revolver",    "assets/sfx/revolver.wav");
    audio.loadSound("shotgun",     "assets/sfx/shotgun.wav");
    audio.loadSound("reload",      "assets/sfx/reload.wav");
    audio.loadSound("grapple_fire","assets/sfx/grapple_fire.wav");
    audio.loadSound("hit",         "assets/sfx/hit.wav");
    audio.loadSound("enemy_death", "assets/sfx/enemy_death.wav");
    audio.loadSound("player_hit",  "assets/sfx/player_hit.wav");
    audio.loadSound("parry",       "assets/sfx/parry.wav");
    audio.loadSound("telegraph",   "assets/sfx/telegraph.wav");
    audio.loadSound("explosion",   "assets/sfx/explosion.wav");

    GameSettings settings; // shared settings object — persists for the process lifetime

    std::unique_ptr<GameState> currentState;
    bool running = true;

    std::function<void()> goToMenu;
    goToMenu = [&]() {
        SDL_SetRelativeMouseMode(SDL_FALSE);
        auto* menu = new MenuState(SCREEN_W, SCREEN_H);
        menu->settings = &settings;
        menu->onStart = [&]() {
            SDL_SetRelativeMouseMode(SDL_TRUE);
            auto* game = new GameplayState(audio);
            game->settings         = &settings;
            game->onReturnToMenu   = [&]() { goToMenu(); };
            game->onQuit           = [&]() { running = false; };
            currentState.reset(game);
        };
        menu->onQuit = [&]() { running = false; };
        currentState.reset(menu);
    };

    goToMenu();

    Uint64 freq      = SDL_GetPerformanceFrequency();

    while (running) {
        Uint64 frameStart = SDL_GetPerformanceCounter();

        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) { running = false; break; }
            if (currentState) currentState->handleEvent(e);
        }
        if (currentState) {
            currentState->update(1.f / 60.f);
            currentState->render();
        }
        SDL_GL_SwapWindow(window);

        // FPS cap — sleep the remainder of the frame budget
        int capValue = settings.getFPSCapValue();
        if (capValue > 0) {
            Uint64 frameEnd    = SDL_GetPerformanceCounter();
            double elapsed     = (double)(frameEnd - frameStart) / (double)freq;
            double targetTime  = 1.0 / (double)capValue;
            if (elapsed < targetTime) {
                Uint32 sleepMs = (Uint32)((targetTime - elapsed) * 1000.0);
                if (sleepMs > 0) SDL_Delay(sleepMs);
            }
        }
    }

    currentState.reset();
    SDL_GL_DeleteContext(ctx);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
