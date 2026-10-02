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
#include <algorithm>
#include <cmath>
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
#include "Display.h"

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
    double nextFrameAt = 0.0;   // frame limiter's schedule (seconds)
    bool   vsyncOn = false;

    // --bench N: render N frames flat out (no cap, no vsync), then print the
    // frame rate and exit. Pair with --fast --arena N --cam ... to time a spot.
    int benchFrames = 0, benchTotal = 0;
    int capOverride = -1;       // --cap HZ: hold this rate whatever the settings say (0 = unlimited)
    std::vector<double> benchTimes;
    Uint64 benchLast = 0;

    double seconds() const { return (double)SDL_GetPerformanceCounter() / (double)freq; }

    void readDisplayHz() {
        SDL_DisplayMode m;
        if (SDL_GetCurrentDisplayMode(SDL_GetWindowDisplayIndex(window), &m) == 0 && m.refresh_rate > 0)
            GameSettings::displayHz() = m.refresh_rate;
    }

    // Everything renders at the graphics quality's resolution (16:9 lines),
    // then present() scales that to the window
    void sizeCanvas() {
        int lines = renderOverride > 0 ? renderOverride : GameSettings::qualityLines(settings.quality);
#ifdef __EMSCRIPTEN__
        fitWebCanvas(lines);
#endif
        display::ensure(lines * 16 / 9, lines);
    }
#ifdef __EMSCRIPTEN__
    // The page's <canvas> holds the pixels the browser shows. Give it the
    // screen pixels it covers (CSS size x devicePixelRatio), no more than the
    // render resolution: below it the browser upscales, as LOW always did;
    // a higher render resolution is supersampled down into it by present().
    void fitWebCanvas(int lines) {
        double cssW = 0, cssH = 0;
        emscripten_get_element_css_size("#canvas", &cssW, &cssH);
        double dpr = emscripten_get_device_pixel_ratio();
        int h = SCREEN_H;
        if (cssW > 0 && cssH > 0) h = display::fit((int)(cssW * dpr + 0.5), (int)(cssH * dpr + 0.5)).h;
        h = std::max(360, std::min(lines, h));
        int w = h * 16 / 9, curW = 0, curH = 0;
        emscripten_get_canvas_element_size("#canvas", &curW, &curH);
        if (curW != w || curH != h) emscripten_set_canvas_element_size("#canvas", w, h);
    }
#endif
    int renderOverride = 0;     // --res H: render at H lines (16:9) whatever the window (dev)

    // Mouse positions arrive in window points; menus think in the virtual 1280x720
    void mapMouse(SDL_Event& e) {
        int ww = SCREEN_W, wh = SCREEN_H;
        SDL_GetWindowSize(window, &ww, &wh);
        if (e.type == SDL_MOUSEMOTION) display::toVirtual(ww, wh, e.motion.x, e.motion.y);
        if (e.type == SDL_MOUSEBUTTONDOWN || e.type == SDL_MOUSEBUTTONUP) display::toVirtual(ww, wh, e.button.x, e.button.y);
    }

    bool fullscreenOn = false;
    void applyFullscreen() {
#ifndef __EMSCRIPTEN__
        if (settings.fullscreen == fullscreenOn) return;
        fullscreenOn = settings.fullscreen;
        SDL_SetWindowFullscreen(window, fullscreenOn ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
        readDisplayHz();
#endif
    }

    void applyVsync() {
        bool want = settings.vsync && benchFrames == 0;
        if (want == vsyncOn) return;
        vsyncOn = want;
        // Adaptive vsync (tears instead of stuttering when a frame is late) where the driver has it
        if (!want) SDL_GL_SetSwapInterval(0);
        else if (SDL_GL_SetSwapInterval(-1) != 0) SDL_GL_SetSwapInterval(1);
    }

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

    // Saves the canvas (the frame at render resolution, before scaling to the window)
    void saveScreenshot(const std::string& path) {
        int W = display::renderW(), H = display::renderH();
        std::vector<unsigned char> px(W * H * 3);
        glBindFramebuffer(GL_READ_FRAMEBUFFER, display::canvas().fbo);
        glPixelStorei(GL_PACK_ALIGNMENT, 1);
        glReadPixels(0, 0, W, H, GL_RGB, GL_UNSIGNED_BYTE, px.data());
        SDL_Surface* surf = SDL_CreateRGBSurfaceWithFormat(0, W, H, 24, SDL_PIXELFORMAT_RGB24);
        if (!surf) return;
        for (int y = 0; y < H; ++y)   // GL rows are bottom-up
            std::memcpy((unsigned char*)surf->pixels + y * surf->pitch, &px[(H - 1 - y) * W * 3], W * 3);
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
            if (e.type == SDL_WINDOWEVENT && (e.window.event == SDL_WINDOWEVENT_MOVED || e.window.event == SDL_WINDOWEVENT_DISPLAY_CHANGED))
                readDisplayHz();
            // F11 or Alt+Enter: fullscreen on / off
            if (e.type == SDL_KEYDOWN && !e.key.repeat && (e.key.keysym.sym == SDLK_F11 ||
                (e.key.keysym.sym == SDLK_RETURN && (e.key.keysym.mod & KMOD_ALT)))) {
                settings.fullscreen = !settings.fullscreen;
                settings.save();
                continue;
            }
            mapMouse(e);
            if (currentState && shotFrames == 0) currentState->handleEvent(e);   // screenshot runs ignore all input
            if (pending != NextState::None) break; // remaining events go to the next state
        }
        applyFullscreen();
        sizeCanvas();
        if (currentState) {
            currentState->update(frameDt);
            display::bind();
            currentState->render();
        }
        if (dynamic_cast<MenuState*>(currentState.get())) {   // the menu's music: a groove, no fight
            audio.masterVolume = settings.audioVolume;
            audio.setMusicVolume(settings.musicVolume);
            audio.music.setTrack(0);
            audio.music.setIntensity(0.55f);
            audio.music.setMuffle(false);
        }
        if (shotFrames > 0 && --shotFrames == 0) {
            saveScreenshot(shotPath); running = false;
            if (auto* g = dynamic_cast<GameplayState*>(currentState.get()))
                std::fprintf(stderr, "shot: feet (%.2f %.2f %.2f) yaw %.1f pitch %.1f arena %d\n", g->player.position.x,
                             g->player.position.y, g->player.position.z, g->player.camera.yaw, g->player.camera.pitch, g->director.arena);
        }
        {
            int dw = SCREEN_W, dh = SCREEN_H;
#ifdef __EMSCRIPTEN__
            // SDL reports its fixed window size here; the real pixels are the canvas's
            emscripten_get_canvas_element_size("#canvas", &dw, &dh);
#else
            SDL_GL_GetDrawableSize(window, &dw, &dh);
#endif
            display::present(dw, dh);
        }
        SDL_GL_SwapWindow(window);

#ifndef __EMSCRIPTEN__
        applyVsync();
        // Frame limiter (the browser paces the web build itself). It keeps an
        // absolute schedule and sleeps most of each gap, then spins the last
        // 1.5 ms: millisecond sleeps alone can't hold 144 or 240 Hz evenly.
        int capValue = capOverride >= 0 ? capOverride : (benchFrames > 0 || vsyncOn) ? 0 : settings.getFPSCapValue();
        if (capValue > 0) {
            double period = 1.0 / capValue;
            if (nextFrameAt == 0.0 || seconds() - nextFrameAt > period * 2.0) nextFrameAt = seconds();   // fell behind: start over
            nextFrameAt += period;
            for (;;) {
                double left = nextFrameAt - seconds();
                if (left <= 0.0) break;
                if (left > 0.002) SDL_Delay((Uint32)((left - 0.0015) * 1000.0));
            }
        } else {
            nextFrameAt = 0.0;
        }
        if (benchFrames > 0) {
            Uint64 nowC = SDL_GetPerformanceCounter();
            if (benchLast) benchTimes.push_back((double)(nowC - benchLast) / (double)freq);   // start-to-start: what the screen sees
            benchLast = nowC;
            if (--benchFrames == 0) { reportBench(); running = false; }
        }
#endif
    }

    void reportBench() {
        if (benchTimes.size() < 10) return;
        std::vector<double> t(benchTimes.begin() + benchTimes.size() / 10, benchTimes.end());   // skip warm-up
        double sum = 0; for (double v : t) sum += v;
        std::sort(t.begin(), t.end());
        double avg = sum / t.size();
        double p99 = t[(size_t)(t.size() * 0.99)];
        double var = 0; for (double v : t) var += (v - avg) * (v - avg);
        std::fprintf(stderr, "bench: %d frames  avg %.2f ms (%.0f fps)  1%% low %.2f ms (%.0f fps)  worst %.2f ms  jitter %.3f ms\n",
                     (int)t.size(), avg * 1000.0, 1.0 / avg, p99 * 1000.0, 1.0 / p99, t.back() * 1000.0,
                     std::sqrt(var / t.size()) * 1000.0);
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
#ifdef __EMSCRIPTEN__
        SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
#else
        // Resizable, full-resolution on Retina/HiDPI screens: the 3D scene
        // renders at the window's real pixel size (times the resolution scale)
        SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
#endif
    if (!app->window) throw std::runtime_error(SDL_GetError());
    SDL_SetWindowMinimumSize(app->window, 640, 360);

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
        "potion", "barrier", "split", "upgrade", "clank", "punch", "step1", "step2", "step3", "step4",
        "door", "door_close", "boost", "cyl_open", "cyl_close", "eject", "shell_in", "pump",
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
        if (arg == "--spawn" && i + 1 < argc) g_devSpawn = std::atoi(argv[++i]);
        if (arg == "--overlay" && i + 1 < argc) g_devOverlay = argv[++i];
        if (arg == "--bench" && i + 1 < argc) { app->benchFrames = std::atoi(argv[++i]); g_devNoMouse = true; }
        if (arg == "--cap" && i + 1 < argc) app->capOverride = std::atoi(argv[++i]);
        if (arg == "--res" && i + 1 < argc) app->renderOverride = std::atoi(argv[++i]);
        if (arg == "--shot" && i + 2 < argc) {
            app->shotFrames = std::atoi(argv[i + 1]); app->shotPath = argv[i + 2]; i += 2;
            g_devNoMouse = true;
        }
    }

    app->freq        = SDL_GetPerformanceFrequency();
    app->lastCounter = SDL_GetPerformanceCounter();
    app->readDisplayHz();

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
