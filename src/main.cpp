#include "core/MouseCapture.h"
#include <SDL2/SDL.h>
#include "core/gl.h"
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

#include "core/GameState.h"
#include "core/Settings.h"
#include "ui/MenuState.h"
#include "game/GameplayState.h"
#include "ui/GunViewer.h"
#include "ui/EnemyViewer.h"
#include "ui/BalancePanel.h"
#include "audio/AudioSystem.h"
#include "core/Display.h"
#include "core/Gamepad.h"
#include "core/FrameTimer.h"
#include "core/Pacer.h"
#include "audio/VoiceSynth.h"

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
    enum class NextState { None, Menu, Game, Viewer };
    int viewerGun = 0;                    // --viewer [gun]: which gun the viewer opens on
    int viewerEnemy = -1;                 // --viewer husk: an enemy instead (EnemyViewer.h)
    NextState pending = NextState::Menu;
    GameMode  mode    = GameMode::ARENA;   // which game the next Game state runs

    // --shot N FILE: render N frames of gameplay, save the last one as a BMP and
    // exit (used to eyeball levels and the HUD without playing)
    // --verify-story: native integration through real Enter/E/retry/menu paths.
    bool verifyStory=false;int storyCheckStage=0,storyCheckFrames=0,verifyExit=0;
    void checkStoryRuntime() {
        if(!verifyStory)return;
        auto fail=[&](const char* reason){std::fprintf(stderr,"STORY RUNTIME FAIL: %s\n",reason);verifyExit=1;running=false;};
        auto press=[&](SDL_Keycode key){SDL_Event e{};e.type=SDL_KEYDOWN;e.key.keysym.sym=key;e.key.keysym.scancode=SDL_GetScancodeFromKey(key);currentState->handleEvent(e);};
        if(++storyCheckFrames>900){fail("timed out");return;}
        auto* g=dynamic_cast<GameplayState*>(currentState.get());
        if(storyCheckStage<7&&!g){fail("expected gameplay");return;}
        switch(storyCheckStage) {
        case 0:
            g->storyOwned=7;g->storyGrapple=true;g->enterArena(8);g->storyBossDown=true;
            g->player.position=(g->level.story.end.min+g->level.story.end.max)*.5f;
            g->player.velocity={0,0,0};++storyCheckStage;break;
        case 1:
            if(g->storyEndT>3.f){press(SDLK_RETURN);++storyCheckStage;}break;
        case 2: {
            if(!g->wasteland()||g->storyOwned!=7||!g->storyGrapple||g->storyEndT>=0||g->level.arenas.size()!=8||g->director.arena!=0) {fail("chapter handoff/inventory/world reset");return;}
            g->enterArena(3);g->player.position={-56,-6,-104};
            g->keepPlayerInZone(1.f/60.f);
            if(glm::length(g->player.position-g->level.arenas[3].respawn)>.1f){fail("span fall recovery");return;}
            g->enterArena(4);
            for(const auto& item:g->interactables)if(item.label=="TAKE THE LONGSHOT") {
                g->player.camera.position=item.position+glm::vec3{0,0,-2.5f};
                g->player.position=g->player.camera.position-glm::vec3{0,1.7f,0};
                g->player.camera.yaw=90;g->player.camera.pitch=0;press(SDLK_e);break;
            }
            if(!g->storyOwns(3)){fail("registered Longshot pickup");return;}
            // Inspect a real readable through the same explicit E key path.
            g->player.position={35,0,-72.2f};g->player.camera.position={35,1.7f,-72.2f};
            g->player.camera.yaw=90;g->player.camera.pitch=0;
            press(SDLK_e);
            if(g->storyLog!=2){fail("explicit wreck reading");return;}
            g->enterArena(6);g->player.position={30,0,-190};++storyCheckStage;break;
        }
        case 3:
            for(auto& enemy:g->enemies)if(enemy.alive&&enemy.type==EnemyType::BALLAST&&enemy.targetable()) {
                g->hurtEnemy(enemy,100000.f,enemy.position,0,0,StyleSource::ENVIRONMENT,false,true);
                if(!g->storyBossDown){fail("Ballast death script");return;}
                ++storyCheckStage;break;
            }
            break;
        case 4:
            if(g->director.arena==7) {
                if(g->level.doors[g->level.arenas[6].exitDoor].locked){fail("boss gate stayed locked");return;}
                g->enterArena(7);g->storyCheckpoint=7;g->restartHere();
                if(!g->storyBossDown||!g->storyOwns(3)){fail("post-boss retry lost progress");return;}
                g->player.position={28,0,-282};++storyCheckStage;
            }break;
        case 5:if(g->storyEndT>3.f){press(SDLK_RETURN);storyCheckStage=7;}break;
        case 7:
            if(auto* menu=dynamic_cast<MenuState*>(currentState.get())) {
                menu->selected=1;press(SDLK_RETURN);press(SDLK_DOWN);press(SDLK_RETURN);++storyCheckStage;
            }break;
        case 8:
            if(g) {
                if(!g->wasteland()||g->storyOwned!=7){fail("chapter selection did not start clean Chapter 2");return;}
                std::fprintf(stderr,"STORY RUNTIME PASS: chapter handoff, inventory, Longshot, E-read, Ballast, gate, retry, ending and menu selection\n");
                running=false;
            }break;
        }
    }
    int         shotFrames = 0;
    std::string shotPath;
    // --record FRAMES SKIP PREFIX: footage. Every frame advances 1/30 s; after
    // SKIP frames, FRAMES are saved as PREFIX_0001.bmp ...
    int         recordFrames = 0, recordSkip = 0, recordIndex = 0;
    std::string recordPrefix;

    Uint64 freq = 0, lastCounter = 0;
    double nextFrameAt = 0.0;   // frame limiter's schedule (seconds)
    Pacer  pacer;               // low-latency pacing (core/Pacer.h)
    double gpuWaitS = 0.0;      // how long this frame waited on the GPU before starting
    bool   usePacing = true;    // --nopace: off (to compare)
    bool   vsyncOn = false;

    // --bench N: render N frames flat out (no cap, no vsync), then print the
    // frame rate and exit. Pair with --fast --arena N --cam ... to time a spot.
    int benchFrames = 0, benchTotal = 0;
    int capOverride = -1;       // --cap HZ: hold this rate whatever the settings say (0 = unlimited)
    std::vector<double> benchTimes;
    Uint64 benchLast = 0;
    std::string frameTimesPath;   // --frametimes FILE: per-zone CSV of every frame, written on exit
    bool gpuTimer = false;        // --gpu-timer: also time the GPU (GL timer queries)
#ifndef __EMSCRIPTEN__
    // Frames in flight: before a frame starts, wait until the GPU has finished the
    // frame `inFlight` back. Without it the CPU runs ahead and the driver stalls it
    // at uneven moments, so frames alternate short and long. (The browser paces itself.)
    int inFlight = 2;
    static constexpr int MAX_FENCES = 4;
    GLsync fences[MAX_FENCES] = {};
    int fenceAt = 0;
    void waitForGpu() {
        if (inFlight <= 0) return;
        GLsync& f = fences[(fenceAt + MAX_FENCES - inFlight) % MAX_FENCES];
        if (!f) return;
        glClientWaitSync(f, GL_SYNC_FLUSH_COMMANDS_BIT, 100000000);   // 100 ms at most
        glDeleteSync(f);
        f = nullptr;
    }
    void fenceFrame() {
        if (inFlight <= 0) return;
        GLsync& f = fences[fenceAt];
        if (f) glDeleteSync(f);
        f = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
        fenceAt = (fenceAt + 1) % MAX_FENCES;
    }
#endif
    Uint64 ftLast = 0;            // frame timer's start-to-start clock
#ifndef __EMSCRIPTEN__
    // GPU time per frame: a ring of GL timer queries, read back a few frames late
    static constexpr int GPU_Q = 4;
    GLuint gpuQ[GPU_Q] = {};
    int gpuQFrame[GPU_Q] = {-1, -1, -1, -1};
    int gpuQSlot = 0;
    bool gpuQPending[GPU_Q] = {};
    void collectGpu(int slot, bool wait) {
        if (!gpuQPending[slot]) return;
        GLint ready = 0;
        glGetQueryObjectiv(gpuQ[slot], GL_QUERY_RESULT_AVAILABLE, &ready);
        if (!ready && !wait) { gpuQPending[slot] = false; return; }   // still in flight: drop it rather than stall
        GLuint64 ns = 0;
        glGetQueryObjectui64v(gpuQ[slot], GL_QUERY_RESULT, &ns);
        frametimer::setGpu(gpuQFrame[slot], (double)ns / 1e6);
        gpuQPending[slot] = false;
    }
#endif

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

    bool godFromCommandLine = false;   // --god: every run, not just dev ones
    bool openDevMenu = false;          // --dev / ?dev: start on the level select

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
            setMouseCaptured(false);
            audio.setSpace(ReverbSpace::OPEN);
            auto* menu = new MenuState(SCREEN_W, SCREEN_H);
            menu->settings = &settings;
            menu->onStart  = [this](GameMode m, StartOptions o) {
                mode = m; pending = NextState::Game;
                g_startArena = o.arena; g_startWave = o.wave;
                g_storyChapter=o.storyChapter;
                g_godMode    = o.god || godFromCommandLine;
                g_practice   = o.practice;
                g_bossRun    = o.boss;
                g_roomAttack = o.room;
                g_arenaSolo  = o.solo;
            };
            menu->onQuit   = [this]() { quit(); };
            if(g_devOverlay=="chapters")menu->page=MenuState::STORYPAGE;
            if (openDevMenu) { menu->openDev(); openDevMenu = false; }
            if (g_devOverlay == "board") menu->page = MenuState::BOARD;
            if (g_devOverlay == "bosses") menu->page = MenuState::BOSS;
            if (g_devOverlay == "arenas") menu->page = MenuState::ARENAPAGE;
            currentState.reset(menu);
        } else if (next == NextState::Game) {
            setMouseCaptured(!g_devNoMouse);
            audio.masterVolume = settings.audioVolume;
            auto* game = new GameplayState(audio, &settings, mode);
            game->onReturnToMenu = [this]() { pending = NextState::Menu; };
            game->onQuit         = [this]() { quit(); };
            currentState.reset(game);
        } else if (next == NextState::Viewer) {   // --viewer: one gun on a stand, its file watched (GunViewer.h)
            setMouseCaptured(false);
            if (viewerEnemy >= 0) {
                auto* v = new EnemyViewerState(SCREEN_W, SCREEN_H, viewerEnemy);
                v->onQuit = [this]() { quit(); };
                currentState.reset(v);
            } else {
                auto* v = new GunViewerState(SCREEN_W, SCREEN_H, viewerGun);
                v->onQuit = [this]() { quit(); };
                currentState.reset(v);
            }
        }
    }

    // F1: open or close the balance panel. Opening frees the mouse (the game is frozen
    // under it); closing hands it back to a run that's being played
    void toggleBalance() {
        g_balanceOpen = !g_balanceOpen;
        if (g_balanceOpen) { setMouseCaptured(false); return; }
        if (auto* g = dynamic_cast<GameplayState*>(currentState.get()))
            if (!g->paused && !g->armoryOpen && !g->playerDead) g->captureMouse(true);
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
#ifndef __EMSCRIPTEN__
        {
            frametimer::Scope ftWait(frametimer::GPU_WAIT);
            double w0 = seconds();
            waitForGpu();
            gpuWaitS = seconds() - w0;
        }
#endif
        Uint64 frameStart = SDL_GetPerformanceCounter();
        float frameDt = (float)((double)(frameStart - lastCounter) / (double)freq);
        lastCounter = frameStart;
        if (frameDt > 0.25f) frameDt = 0.25f; // clamp huge stalls (breakpoints, window drag)
        if (g_fixedDt > 0.0) frameDt = (float)g_fixedDt;

        if (pending != NextState::None) {
            frametimer::Scope ftLoad(frametimer::LOAD);
            applyPendingState();
            lastCounter = SDL_GetPerformanceCounter(); // don't bill load time to the first frame
        }

        frametimer::Lap ftLap;
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
            if (e.type == SDL_CONTROLLERDEVICEADDED || e.type == SDL_CONTROLLERDEVICEREMOVED) gamepad::onDeviceEvent(e);
            // F1: the balance panel (BalancePanel.h): the game freezes, the mouse comes free
            if (e.type == SDL_KEYDOWN && !e.key.repeat && e.key.keysym.sym == SDLK_F1 && g_balanceAllowed) { toggleBalance(); continue; }
            if (balancepanel::handleEvent(e)) continue;
            mapMouse(e);
            if (currentState && shotFrames == 0) currentState->handleEvent(e);   // screenshot runs ignore all input
            if (pending != NextState::None) break; // remaining events go to the next state
        }
        applyFullscreen();
        sizeCanvas();
        ftLap.to(frametimer::EVENTS);
#ifndef __EMSCRIPTEN__
        const bool gpuTiming = gpuTimer && frametimer::enabled();
        if (gpuTiming) {
            if (!gpuQ[0]) glGenQueries(GPU_Q, gpuQ);
            collectGpu(gpuQSlot, false);
            glBeginQuery(GL_TIME_ELAPSED, gpuQ[gpuQSlot]);
        }
#endif
        if (currentState) {
            currentState->update(frameDt);
            checkStoryRuntime();
            ftLap.to(frametimer::UPDATE);
            display::bind();
            currentState->render();
            ftLap.to(frametimer::RENDER);
        }
        if (dynamic_cast<MenuState*>(currentState.get())) {   // the menu's music: a groove, no fight
            audio.masterVolume = settings.audioVolume;
            audio.setMusicVolume(settings.musicVolume);
            audio.music.setTrack(0);
            audio.music.setIntensity(0.55f);
            audio.music.setMuffle(false);
        }
        if (recordFrames > 0) {   // --record: skip the warm-up, then save every frame
            ++recordIndex;
            if (recordIndex > recordSkip) {
                char path[512];
                std::snprintf(path, sizeof(path), "%s_%04d.bmp", recordPrefix.c_str(), recordIndex - recordSkip);
                saveScreenshot(path);
                if (recordIndex - recordSkip >= recordFrames) running = false;
            }
            g_recProgress = recordIndex > recordSkip ? (float)(recordIndex - recordSkip) / (float)recordFrames : 0.f;
        }
        if (audio.dumpDone()) { audio.writeDump(); running = false; }
        if (shotFrames > 0 && --shotFrames == 0) {
            if (g_balanceOpen) balancepanel::draw(display::canvas().fbo, display::renderW(), display::renderH());   // (--overlay balance: in the shot)
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
            ftLap = frametimer::Lap{};
            display::present(dw, dh);
            balancepanel::draw(0, dw, dh);   // (F1) over the frame, at the window's own resolution
            ftLap.to(frametimer::PRESENT);
        }
#ifndef __EMSCRIPTEN__
        if (gpuTiming) { frametimer::Scope ftq(frametimer::GPU_SYNC); glEndQuery(GL_TIME_ELAPSED); }
#endif
        {
            frametimer::Scope ftSwap(frametimer::SWAP);
            SDL_GL_SwapWindow(window);
        }
#ifndef __EMSCRIPTEN__
        fenceFrame();
#endif

#ifndef __EMSCRIPTEN__
        applyVsync();
        // Frame limiter (the browser paces the web build itself). It keeps an
        // absolute schedule and sleeps most of each gap, then spins the last
        // 1.5 ms: millisecond sleeps alone can't hold 144 or 240 Hz evenly.
        frametimer::Lap ftLimit;
        int capValue = capOverride >= 0 ? capOverride : (benchFrames > 0 || vsyncOn) ? 0 : settings.getFPSCapValue();
        // Low-latency pacing (core/Pacer.h): never let the CPU queue frames up behind a busy GPU (--nopace: off)
        const bool pace = usePacing && benchFrames == 0 && !vsyncOn;
        if (pace) pacer.onWait(gpuWaitS);
        else pacer.reset();
        double capPeriod = capValue > 0 ? 1.0 / capValue : 0.0;
        double period = std::max(capPeriod, pacer.period);
        if (period > 0.0) {
            double now = seconds();
            if (nextFrameAt == 0.0 || now - nextFrameAt > period * 2.0) nextFrameAt = now;   // fell behind: start again from now (no extra wait on top of a late frame)
            else nextFrameAt += period;
            for (;;) {
                double left = nextFrameAt - seconds();
                if (left <= 0.0) break;
                if (left > 0.002) SDL_Delay((Uint32)((left - 0.0015) * 1000.0));
            }
        } else {
            nextFrameAt = 0.0;
        }
        ftLimit.to(frametimer::LIMIT);
#endif
        if (frametimer::enabled()) {
            Uint64 nowC = SDL_GetPerformanceCounter();
            int idx = ftLast ? frametimer::endFrame(frametimer::ms(ftLast, nowC)) : (frametimer::cur() = frametimer::Frame{}, -1);
            ftLast = nowC;
#ifndef __EMSCRIPTEN__
            if (gpuTiming) {
                gpuQFrame[gpuQSlot] = idx;
                gpuQPending[gpuQSlot] = idx >= 0;
                gpuQSlot = (gpuQSlot + 1) % GPU_Q;
            }
#endif
        }
#ifndef __EMSCRIPTEN__
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
#ifndef __EMSCRIPTEN__
        for (int i = 0; i < GPU_Q; ++i) collectGpu(i, true);
#endif
        std::vector<double> t(benchTimes.begin() + benchTimes.size() / 10, benchTimes.end());   // skip warm-up
        double sum = 0; for (double v : t) sum += v;
        std::sort(t.begin(), t.end());
        double avg = sum / t.size();
        double p99 = t[(size_t)(t.size() * 0.99)];
        double var = 0; for (double v : t) var += (v - avg) * (v - avg);
        std::fprintf(stderr, "bench: %d frames  avg %.2f ms (%.0f fps)  1%% low %.2f ms (%.0f fps)  worst %.2f ms  jitter %.3f ms\n",
                     (int)t.size(), avg * 1000.0, 1.0 / avg, p99 * 1000.0, 1.0 / p99, t.back() * 1000.0,
                     std::sqrt(var / t.size()) * 1000.0);
        frametimer::report(frametimer::history().size() / 10);
    }
};

#ifdef __EMSCRIPTEN__
// The browser paces the web build off requestAnimationFrame: the page can only
// show what the screen refreshes, and frames made faster than that (setImmediate)
// pile up in the browser's GPU process - seconds of lag at "unlimited". So a cap
// at or above the screen's rate runs at the screen's rate; a cap under it skips
// frames. The screen's rate is measured from rAF's own spacing.
static void webFrame(void* p) {
    App* app = static_cast<App*>(p);
    static double nextAt = 0.0, lastRaf = 0.0, rafMs = 1000.0 / 60.0;
    static int rafSamples = 0;
    double now = emscripten_get_now();
    int hz = GameSettings::frameCapHz(app->settings.frameCap);
    if (lastRaf > 0.0) {   // learn the screen's rate (a running average that ignores hitches)
        double d = now - lastRaf;
        if (d > 2.0 && d < 60.0 && (rafSamples < 10 || std::fabs(d - rafMs) < rafMs * 0.3)) {
            rafMs += (d - rafMs) * (rafSamples < 10 ? 0.5 : 0.05);
            if (++rafSamples >= 10) GameSettings::displayHz() = std::max(30, (int)std::lround(1000.0 / rafMs));
        }
    }
    lastRaf = now;
    static int frames = 0; static double since = 0.0;
    if (g_devFpsLog && now - since > 2000.0) {   // --fps / ?fps=: the rate this loop really runs at
        if (since > 0.0) std::printf("web fps %.0f (cap %d, display %d)\n", frames * 1000.0 / (now - since), hz, GameSettings::displayHz());
        since = now; frames = 0;
    }
    int cap = (hz <= 0 || hz >= GameSettings::displayHz() - 2) ? 0 : hz;   // DISPLAY / UNLIMITED / above the screen: every rAF
    if (cap > 0) {
        double period = 1000.0 / cap, slack = rafMs * 0.5;
        if (nextAt == 0.0 || now - nextAt > period * 2.0) nextAt = now;   // fell behind: start over
        if (now + slack < nextAt) return;
        nextAt += period;
    } else nextAt = 0.0;
    ++frames;
    app->frame();
}

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
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMECONTROLLER) != 0)
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
    balanceInit();                               // the game's numbers, as assets/balance.txt has them (core/Balance.h)
    GameplayState::sharedWorldTex();             // the world's textures, made now (behind the loading screen), not as a run starts
    balancepanel::init(app->window, app->ctx);   // F1

    for (const MixEntry& m : mixTable())   // every sound file has its place in the mix (MixTable.h)
        app->audio.loadSound(m.name, std::string("assets/sfx/") + m.name + ".wav");
    if (app->audio.initialized) {   // a sound that didn't load just plays nothing: say so
        int missing = 0;
        for (const MixEntry& m : mixTable())
            if (!app->audio.sfx.sample(m.name, 0)) { std::fprintf(stderr, "sfx: %s did not load\n", m.name); ++missing; }
        std::fprintf(stderr, "sfx: %d of %d sounds loaded\n", (int)mixTable().size() - missing, (int)mixTable().size());
    }
    if (app->audio.initialized) {   // the enemy voices, built in code (VoiceSynth.h)
        const Uint32 t0 = SDL_GetTicks();
        float secs = 0.f;
        for (const VoiceSpec& v : voiceBank())
            for (int k = 0; k < v.variants; ++k) {
                auto b = VoiceSynth::build(v, k);
                secs += b.size() / VoiceSynth::RATE;
                app->audio.addBuffer(v.name, std::move(b), VoiceSynth::RATE);
            }
        if (std::getenv("OVERDRIVE_SFXLIST")) std::fprintf(stderr, "voices: %.1f s of audio built in %u ms\n", secs, SDL_GetTicks() - t0);
    }
    if (std::getenv("OVERDRIVE_SFXLIST")) app->audio.sfx.dumpLengths(stderr);
    for (int i = 1; i < argc; ++i) if (std::string(argv[i]) == "--mixreport") {   // dev: every sound's level, then quit
        auto row = [&](const std::string& n, MixClass c, float trim) {
            const std::vector<float>* smp = app->audio.sfx.sample(n, 0);
            if (!smp) { std::printf("%-24s missing\n", n.c_str()); return; }
            float file = shortTermDb(smp->data(), smp->size(), app->audio.sfx.sampleRateOf(n));
            std::printf("%-24s class %d  file %6.1f  trim %5.1f  level %6.1f  target %6.1f\n", n.c_str(), (int)c, file, trim, file + trim, mixTargetDb(c));
        };
        for (const MixEntry& m : mixTable()) row(m.name, m.cls, m.trimDb);
        for (const VoiceSpec& v : voiceBank()) row(v.name, v.cls, 0.f);
        SDL_GL_DeleteContext(app->ctx); SDL_DestroyWindow(app->window);
        delete app; SDL_Quit();   // the audio callback unhooks before anything goes away
        return 0;
    }
    app->audio.start();   // the bank is complete: the audio thread may mix from here on

    app->settings.load();  // restore every option (settings.cfg on desktop, localStorage on the web)

    // --play skips the main menu and drops straight into an ARENA run; --fast
    // into the FAST time trial, --endless an ENDLESS run, --act2 ACT II. --arena N starts at a later arena / section
    // (implies --play), --wave N skips to a wave within it, --god disables
    // damage (for footage), --dev opens the level select. Dev: --cam X Y Z
    // YAW PITCH, --shot FRAMES FILE.BMP, --audiodump FILE.WAV SECONDS (record the mix, then quit),
    // --sword (the sword on key 5 in any arena)
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--play") app->pending = App::NextState::Game;
        if (arg == "--fast") { app->mode = GameMode::FAST; app->pending = App::NextState::Game; }
        if (arg == "--room" && i + 1 < argc) {   // --room N: FAST's ROOM ATTACK, room N (1-7) on its own
            app->mode = GameMode::FAST; app->pending = App::NextState::Game;
            g_roomAttack = std::max(0, std::atoi(argv[++i]) - 1); g_startArena = g_roomAttack;
        }
        if (arg == "--act2") { app->mode = GameMode::ACT2; app->pending = App::NextState::Game; }
        if (arg == "--solo" && i + 1 < argc) {   // --solo N: the ARENA page's arena N on its own (with --act2 first: Act II's)
            if (app->mode != GameMode::ACT2) app->mode = GameMode::ARENA;
            app->pending = App::NextState::Game;
            g_arenaSolo = std::max(0, std::atoi(argv[++i]) - 1); g_startArena = g_arenaSolo;
        }
        if (arg == "--endless") { app->mode = GameMode::ENDLESS; app->pending = App::NextState::Game; }
        if (arg == "--story") { app->mode = GameMode::STORY; app->pending = App::NextState::Game; }   // STORY: CHAPTER 1 (the wake-up first)
        if(arg=="--verify-story") {app->verifyStory=true;g_storyChapter=1;app->mode=GameMode::STORY;app->pending=App::NextState::Game;g_fixedDt=1.0/30;g_devNoMouse=true;g_godMode=true;}
        if(arg=="--story2") {g_storyChapter=2;app->mode=GameMode::STORY;app->pending=App::NextState::Game;}
        if(arg=="--chapter" && i+1<argc)g_storyChapter=std::clamp(std::atoi(argv[++i]),1,2);
        if (arg == "--boss" && i + 1 < argc) {   // --boss N: BOSS N (1 the Warden .. 6 MORNINGSTAR), on its own
            g_bossRun = std::clamp(std::atoi(argv[++i]) - 1, 0, BOSS_COUNT - 1);
            app->mode = bossEntry(g_bossRun).act2 ? GameMode::ACT2 : GameMode::ARENA; app->pending = App::NextState::Game;
        }
        if (arg == "--god")  { g_godMode = true; app->godFromCommandLine = true; }
        if (arg == "--dev")  app->openDevMenu = true;
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
        if (arg == "--sword") g_devSword = true;   // the sword (key 5) anywhere, for testing it
        if (arg == "--swordorbit" && i + 1 < argc) g_devSwordOrbit = (float)std::atof(argv[++i]);
        if (arg == "--spawn" && i + 1 < argc) g_devSpawns.push_back(std::atoi(argv[++i]));
        if (arg == "--hollow" && i + 1 < argc) g_devHollow = std::atoi(argv[++i]) & 3;
        if (arg == "--overlay" && i + 1 < argc) g_devOverlay = argv[++i];
        if (arg == "--bench" && i + 1 < argc) { app->benchFrames = std::atoi(argv[++i]); g_devNoMouse = true; frametimer::enabled() = true; }
#ifndef __EMSCRIPTEN__
        if (arg == "--inflight" && i + 1 < argc) app->inFlight = std::max(0, std::min(3, std::atoi(argv[++i])));   // dev: frames the CPU may run ahead (0 = no fence)
#endif
        if (arg == "--aa" && i + 1 < argc) g_devAA = std::atoi(argv[++i]);
        if (arg == "--skip" && i + 1 < argc) g_devSkip = argv[++i];
        if (arg == "--gpu-timer") { app->gpuTimer = true; frametimer::enabled() = true; }
        if (arg == "--frametimes" && i + 1 < argc) { app->frameTimesPath = argv[++i]; frametimer::enabled() = true; }
        if (arg == "--cap" && i + 1 < argc) app->capOverride = std::atoi(argv[++i]);
        if (arg == "--nopace") app->usePacing = false;
        if (arg == "--fps" && i + 1 < argc) {   // dev: pick the FRAME RATE setting by its Hz (0 = UNLIMITED, -1 = DISPLAY); the web build logs its rate
            int want = std::atoi(argv[++i]);
            for (int k = 0; k < GameSettings::FRAME_CAPS; ++k) if (GameSettings::frameCapHz(k) == want) app->settings.frameCap = k;
            g_devFpsLog = true;
        }
        if (arg == "--ambient" && i + 1 < argc) app->settings.ambientDetail = std::clamp(std::atoi(argv[++i]), 0, 3);
        if (arg == "--res" && i + 1 < argc) app->renderOverride = std::atoi(argv[++i]);
        if (arg == "--balance") g_balanceAllowed = true;
        if (g_devOverlay.rfind("balance", 0) == 0) {   // --overlay balance[:search]: start with it open (screenshots)
            g_balanceOpen = true;
            if (g_devOverlay.size() > 8) std::snprintf(balancepanel::st().filter, sizeof balancepanel::st().filter, "%s", g_devOverlay.c_str() + 8);
        }   // the balance panel (F1) in the web build (?balance); desktop always has it
        if (arg == "--viewer") {   // --viewer [revolver|shotgun|lancer|longshot|sword|1-5|husk]: the gun (or enemy) viewer (work on assets/ in Blender)
            app->pending = App::NextState::Viewer;
            if (i + 1 < argc && argv[i + 1][0] != '-') {
                std::string g = argv[++i];
                for (int k = 0; k < gunkit::GUN_FILES; ++k) if (g == gunkit::gunName(k) || g == std::to_string(k + 1)) app->viewerGun = k;
                for (int k = 0; k < enemyfile::FILE_COUNT; ++k)   // (an enemy's file id: morningstar too)
                    if (const char* n = enemyfile::fileNameOf(k); n && g == n) app->viewerEnemy = k;
            }
        }
        if (arg == "--record" && i + 3 < argc) {
            app->recordFrames = std::atoi(argv[i + 1]); app->recordSkip = std::atoi(argv[i + 2]);
            app->recordPrefix = argv[i + 3]; i += 3;
            g_fixedDt = 1.0 / 30.0; g_devNoMouse = true;
        }
        if (arg == "--campath" && i + 5 < argc) {
            g_devCamPath = true;
            g_devCamPos2 = {(float)std::atof(argv[i + 1]), (float)std::atof(argv[i + 2]), (float)std::atof(argv[i + 3])};
            g_devCamYaw2 = (float)std::atof(argv[i + 4]); g_devCamPitch2 = (float)std::atof(argv[i + 5]);
            i += 5;
        }
        if (arg == "--autoaim") g_devAutoAim = true;
        if (arg == "--clean") g_devClean = true;
        if (arg == "--kite") g_devKite = true;
        if (arg == "--audiodump" && i + 2 < argc) {   // dev: record the final mix to a WAV, then quit
            app->audio.startDump(argv[i + 1], (float)std::atof(argv[i + 2])); i += 2;
            g_devNoMouse = true;
        }
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
    if (!app->frameTimesPath.empty()) {
        for (int i = 0; i < App::GPU_Q; ++i) app->collectGpu(i, true);
        if (frametimer::writeCsv(app->frameTimesPath)) std::fprintf(stderr, "frametimes: %zu frames -> %s\n", frametimer::history().size(), app->frameTimesPath.c_str());
        else std::fprintf(stderr, "frametimes: could not write %s\n", app->frameTimesPath.c_str());
    }

    app->currentState.reset();
    SDL_GL_DeleteContext(app->ctx);
    SDL_DestroyWindow(app->window);
    int exitCode=app->verifyExit;
    delete app;
    SDL_Quit();
    return exitCode;
#endif
}
