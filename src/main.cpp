#include <SDL2/SDL.h>
#include "gl.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <functional>
#include <string>
#include <cstdlib>
#include <cstring>
#include <vector>
#ifdef _WIN32
#  include <direct.h>
#  define chdir _chdir
#else
#  include <unistd.h>
#endif

#include "GameState.h"
#include "Settings.h"
#include "MenuState.h"
#include "GameplayState.h"
#include "AudioSystem.h"

#ifdef __EMSCRIPTEN__
#  include <emscripten.h>
#  include <emscripten/html5.h>
#endif

// Everything the main loop needs, heap-allocated so the web build (whose
// main() returns before the first frame) can keep running from a callback.
struct App {
    SDL_Window*   window = nullptr;
    SDL_GLContext ctx    = nullptr;
    AudioSystem   audio;
    GameSettings  settings;   // shared settings object — persists for the process lifetime
    std::unique_ptr<GameState> currentState;
    bool running = true;

    // State changes are requested from inside a state's own handleEvent/update,
    // so they're deferred to the top of the next frame: destroying the active
    // state while one of its methods is still on the stack is a use-after-free.
    enum class NextState { None, Menu, Game };
    NextState pending = NextState::Menu;
    GameMode  mode    = GameMode::ARENA;   // which game the next Game state runs

    // --shot N FILE: render N frames of gameplay, save the last one as a BMP and
    // exit (used to eyeball levels and the HUD without playing)
    int         shotFrames = 0;
    std::string shotPath;

    Uint64 freq = 0, lastCounter = 0;

    void quit() {
#ifndef __EMSCRIPTEN__
        running = false;   // a browser tab can't exit; the menu's EXIT is a no-op there
#endif
    }

    void applyPendingState() {
        NextState next = pending;
        pending = NextState::None;
        if (next == NextState::Menu) {
            SDL_SetRelativeMouseMode(SDL_FALSE);
            auto* menu = new MenuState(SCREEN_W, SCREEN_H);
            menu->settings = &settings;
            menu->onStart  = [this](GameMode m) { mode = m; pending = NextState::Game; };
            menu->onQuit   = [this]() { quit(); };
            currentState.reset(menu);
        } else if (next == NextState::Game) {
            SDL_SetRelativeMouseMode(g_devNoMouse ? SDL_FALSE : SDL_TRUE);
            audio.masterVolume = settings.audioVolume;
            auto* game = new GameplayState(audio, &settings, mode);
            game->onReturnToMenu = [this]() { pending = NextState::Menu; };
            game->onQuit         = [this]() { quit(); };
            currentState.reset(game);
        }
    }

    void saveScreenshot(const std::string& path) {
        std::vector<unsigned char> px(SCREEN_W * SCREEN_H * 3);
        glPixelStorei(GL_PACK_ALIGNMENT, 1);
        glReadPixels(0, 0, SCREEN_W, SCREEN_H, GL_RGB, GL_UNSIGNED_BYTE, px.data());
        SDL_Surface* surf = SDL_CreateRGBSurfaceWithFormat(0, SCREEN_W, SCREEN_H, 24, SDL_PIXELFORMAT_RGB24);
        if (!surf) return;
        for (int y = 0; y < SCREEN_H; ++y)   // GL rows are bottom-up
            std::memcpy((unsigned char*)surf->pixels + y * surf->pitch, &px[(SCREEN_H - 1 - y) * SCREEN_W * 3], SCREEN_W * 3);
        SDL_SaveBMP(surf, path.c_str());
        SDL_FreeSurface(surf);
    }

    void frame() {
        Uint64 frameStart = SDL_GetPerformanceCounter();
        float frameDt = (float)((double)(frameStart - lastCounter) / (double)freq);
        lastCounter = frameStart;
        if (frameDt > 0.25f) frameDt = 0.25f; // clamp huge stalls (breakpoints, window drag)

        if (pending != NextState::None) {
            applyPendingState();
            lastCounter = SDL_GetPerformanceCounter(); // don't bill load time to the first frame
        }

        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) { running = false; break; }
            if (currentState) currentState->handleEvent(e);
            if (pending != NextState::None) break; // remaining events go to the next state
        }
        if (currentState) {
            currentState->update(frameDt);
            currentState->render();
        }
        if (shotFrames > 0 && --shotFrames == 0) {
            saveScreenshot(shotPath); running = false;
            if (auto* g = dynamic_cast<GameplayState*>(currentState.get()))
                std::fprintf(stderr, "shot: feet (%.2f %.2f %.2f) yaw %.1f pitch %.1f arena %d\n", g->player.position.x,
                             g->player.position.y, g->player.position.z, g->player.camera.yaw, g->player.camera.pitch, g->director.arena);
        }
        SDL_GL_SwapWindow(window);

#ifndef __EMSCRIPTEN__
        // FPS cap — sleep the remainder of the frame budget (the browser paces the web build)
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
#endif
    }
};

#ifdef __EMSCRIPTEN__
static void webFrame(void* app) { static_cast<App*>(app)->frame(); }

// The browser consumes Escape to release the pointer lock, so the game never
// sees the key. Treat losing the lock mid-run as a pause instead.
static bool onPointerLockChange(int, const EmscriptenPointerlockChangeEvent* e, void* app) {
    auto* a = static_cast<App*>(app);
    if (!e->isActive)
        if (auto* game = dynamic_cast<GameplayState*>(a->currentState.get())) game->pause();
    return false;
}
#endif

int main(int argc, char* argv[]) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0)
        throw std::runtime_error(SDL_GetError());

#ifndef __EMSCRIPTEN__
    // Shaders and assets are loaded by relative path (src/, assets/). Run from
    // the binary's directory so launching from Finder or another cwd works.
    if (char* base = SDL_GetBasePath()) {
        if (chdir(base) != 0) std::cerr << "warning: could not chdir to " << base << "\n";
        SDL_free(base);
    }
#endif

#ifdef __EMSCRIPTEN__
    // WebGL2 = OpenGL ES 3.0
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
#else
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
#endif
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE,   24);

    auto* app = new App();
    app->window = SDL_CreateWindow(
        "OVERDRIVE",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        SCREEN_W, SCREEN_H,
        SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
    if (!app->window) throw std::runtime_error(SDL_GetError());

    app->ctx = SDL_GL_CreateContext(app->window);
    if (!app->ctx) throw std::runtime_error(SDL_GetError());

#if !defined(__APPLE__) && !defined(__EMSCRIPTEN__)
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK)
        throw std::runtime_error("Failed to initialize GLEW");
#endif

    SDL_GL_SetSwapInterval(0); // vsync off — we do our own frame cap
    glViewport(0, 0, SCREEN_W, SCREEN_H);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    static const char* SOUNDS[] = {
        "jump", "land", "dash", "slam", "revolver", "shotgun", "reload", "grapple_fire",
        "hit", "enemy_death", "player_hit", "parry", "telegraph", "explosion",
        "wave", "spawn", "pickup", "kar", "longshot", "bolt", "scope", "levelup",
        "potion", "barrier", "split", "upgrade",
    };
    for (const char* name : SOUNDS)
        app->audio.loadSound(name, std::string("assets/sfx/") + name + ".wav");

    app->settings.load();  // restore every option (settings.cfg on desktop, localStorage on the web)

    // --play skips the main menu and drops straight into an ARENA run; --fast
    // into the FAST time trial. --arena N starts at a later arena / section
    // (implies --play), --wave N skips to a wave within it, --god disables
    // damage (for footage). Dev: --cam X Y Z YAW PITCH, --shot FRAMES FILE.BMP
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--play") app->pending = App::NextState::Game;
        if (arg == "--fast") { app->mode = GameMode::FAST; app->pending = App::NextState::Game; }
        if (arg == "--god")  g_godMode = true;
        if (arg == "--wave" && i + 1 < argc) g_startWave = std::atoi(argv[++i]) - 1;
        if (arg == "--arena" && i + 1 < argc) {
            g_startArena = std::atoi(argv[++i]) - 1;
            app->pending = App::NextState::Game;
        }
        if (arg == "--cam" && i + 5 < argc) {
            g_devCam = true;
            g_devCamPos = {(float)std::atof(argv[i + 1]), (float)std::atof(argv[i + 2]), (float)std::atof(argv[i + 3])};
            g_devCamYaw = (float)std::atof(argv[i + 4]); g_devCamPitch = (float)std::atof(argv[i + 5]);
            i += 5;
        }
        if (arg == "--weapon" && i + 1 < argc) g_devWeapon = std::atoi(argv[++i]) - 1;
        if (arg == "--aim") g_devAim = true;
        if (arg == "--overlay" && i + 1 < argc) g_devOverlay = argv[++i];
        if (arg == "--shot" && i + 2 < argc) {
            app->shotFrames = std::atoi(argv[i + 1]); app->shotPath = argv[i + 2]; i += 2;
            g_devNoMouse = true;
        }
    }

    app->freq        = SDL_GetPerformanceFrequency();
    app->lastCounter = SDL_GetPerformanceCounter();

#ifdef __EMSCRIPTEN__
    emscripten_set_pointerlockchange_callback(EMSCRIPTEN_EVENT_TARGET_DOCUMENT, app, false, onPointerLockChange);
    emscripten_set_main_loop_arg(webFrame, app, 0, false);  // 0 = requestAnimationFrame
    return 0;
#else
    while (app->running) app->frame();

    app->currentState.reset();
    SDL_GL_DeleteContext(app->ctx);
    SDL_DestroyWindow(app->window);
    delete app;
    SDL_Quit();
    return 0;
#endif
}
