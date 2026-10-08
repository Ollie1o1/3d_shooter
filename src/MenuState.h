#pragma once
#include "GameState.h"
#include "Settings.h"
#include "SettingsMenu.h"
#include "Progression.h"
#include "LeaderboardView.h"
#include "Daily.h"
#include "Level.h"
#include "LevelGauntlet.h"
#include "LevelAct2.h"
#include "UIBatch.h"
#include "Gamepad.h"
#include <SDL2/SDL.h>
#ifdef __EMSCRIPTEN__
#  include <emscripten.h>
#endif
#include "gl.h"
#include <glm/glm.hpp>
#include <functional>
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

// =============================================================================
// MenuState — main menu (ARENA / FAST / ENDLESS / DAILY / LEADERBOARD /
// SETTINGS / EXIT), the
// settings page, the leaderboard page, and the DEV level select (press ` or
// F2 on the main menu): jump straight to any arena, wave, FAST room or the
// boss, optionally in god mode. Dev runs are practice: no records.
// =============================================================================
class MenuState : public GameState {
public:
    std::function<void(GameMode, StartOptions)> onStart;
    std::function<void()>         onQuit;

    GameSettings* settings = nullptr;   // injected by main

    enum Page { MAIN, SETTINGS, BOARD, DEV };
    int   page      = MAIN;
    int   selected  = 0;
    float flashTime = 0.f;
    int   screenW, screenH;

    UIBatch      ui;
    SettingsMenu settingsMenu;
    Records      records;
    Leaderboard  board;
    Leaderboard  worldBoard;          // web: the shared board, cached by web/index.html
    bool         worldLoaded = false;
    float        boardPoll = 0.f;

    static constexpr int NUM_ITEMS = 8;
    DailyInfo daily = DailyInfo::today();
    std::vector<std::string> arenaNames;

    // DEV level select: rows are options, then every arena, then every FAST room
    struct DevRow { const char* label; std::string name; GameMode mode; int arena; int waves; };
    std::vector<DevRow> devRows;
    int  devSel = 0, devWave = 0;
    bool devGod = true;
    static constexpr int DEV_OPTIONS = 2;   // GOD MODE, START WAVE
    int act1Rows = 0;                       // ARENA rows; ACT II's go under them

    MenuState(int w, int h) : screenW(w), screenH(h), ui(w, h), settingsMenu(w, h) {
        settingsMenu.onBack = [this]() { page = MAIN; selected = 6; };
        records.load();
        board.daily = worldBoard.daily = daily.key();
        board.load();
        LevelData A = buildLevel(), F = buildGauntlet();
        for (auto& a : A.arenas) arenaNames.push_back(a.name);
        for (int i = 0; i < (int)A.arenas.size(); ++i)
            devRows.push_back({"ARENA", A.arenas[i].name, GameMode::ARENA, i, (int)A.arenas[i].waves.size()});
        for (int i = 0; i < (int)F.arenas.size(); ++i)
            devRows.push_back({"FAST", F.arenas[i].name, GameMode::FAST, i, (int)F.arenas[i].waves.size()});
        act1Rows = (int)A.arenas.size();
        LevelData A2 = buildAct2Level();
        for (int i = 0; i < (int)A2.arenas.size(); ++i)
            devRows.push_back({"ACT II", A2.arenas[i].name, GameMode::ACT2, i, (int)A2.arenas[i].waves.size()});
    }

    void openDev() { page = DEV; devSel = DEV_OPTIONS; }

    void handleEvent(const SDL_Event& e) override {
        settingsMenu.s = settings;
        if (e.type == SDL_CONTROLLERBUTTONDOWN) {   // the D-pad and A/B drive every page
            // B backs out of a page; on the main page it does nothing (Escape there quits)
            SDL_Keycode k = gamepad::menuKey(e.cbutton.button, page != MAIN);
            if (page == MAIN && e.cbutton.button == SDL_CONTROLLER_BUTTON_BACK) { openDev(); return; }
            if (k != SDLK_UNKNOWN) { SDL_Event ke = gamepad::keyEvent(k); handleEvent(ke); }
            return;
        }
        if (page == SETTINGS) { settingsMenu.handleEvent(e); return; }
        if (page == BOARD) {
            if ((e.type == SDL_KEYDOWN && (e.key.keysym.sym == SDLK_ESCAPE || e.key.keysym.sym == SDLK_RETURN ||
                                          e.key.keysym.sym == SDLK_BACKSPACE)) ||
                (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT)) page = MAIN;
            return;
        }
        if (page == DEV) { handleDevEvent(e); return; }
        if (e.type == SDL_KEYDOWN && !e.key.repeat &&
            (e.key.keysym.sym == SDLK_BACKQUOTE || e.key.keysym.sym == SDLK_F2)) { openDev(); return; }
        if (e.type == SDL_KEYDOWN) {
            switch (e.key.keysym.sym) {
                case SDLK_UP:     selected = (selected + NUM_ITEMS - 1) % NUM_ITEMS; break;
                case SDLK_DOWN:   selected = (selected + 1) % NUM_ITEMS; break;
                case SDLK_RETURN:
                case SDLK_SPACE:  activate(selected); break;
                case SDLK_ESCAPE: if (onQuit) onQuit(); break;
                case SDLK_LEFT:   cycleDifficulty(-1); break;
                case SDLK_RIGHT:  cycleDifficulty(1); break;
                default: break;
            }
        }
        if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
            int i = itemAt(e.button.x, e.button.y);
            if (i >= 0) activate(i);
        }
        if (e.type == SDL_MOUSEMOTION) {
            int i = itemAt(e.motion.x, e.motion.y);
            if (i >= 0) selected = i;
        }
    }

    void update(float dt) override { flashTime += dt; }

    void render() override {
        glClearColor(0.04f, 0.03f, 0.08f, 1.f);
        glClear(GL_COLOR_BUFFER_BIT);
        ui.begin();
        // Background: vertical gradient and a slowly scrolling synthwave grid
        for (int i = 0; i < 24; ++i) {
            float t = (float)i / 24.f;
            ui.rect(0, t * screenH, screenW, screenH / 24.f + 1, {0.04f + t * 0.05f, 0.02f + t * 0.015f, 0.08f + t * 0.07f, 1.f});
        }
        float horizon = screenH * 0.62f;
        for (int i = 0; i < 14; ++i) {
            float z = std::fmod(i + flashTime * 0.6f, 14.f) / 14.f;
            float y = horizon + (screenH - horizon) * z * z;
            ui.rect(0, y, screenW, 1 + z * 2, {1.f, 0.3f, 0.6f, 0.08f + 0.3f * z});
        }
        for (int i = -12; i <= 12; ++i) {
            float x0 = screenW / 2 + i * 40.f;
            for (int k = 0; k < 16; ++k) {
                float z = k / 16.f;
                float y = horizon + (screenH - horizon) * z;
                float x = screenW / 2 + (x0 - screenW / 2) * (0.2f + 2.6f * z);
                ui.rect(x, y, 1 + z * 2, (screenH - horizon) / 16.f + 1, {1.f, 0.3f, 0.6f, 0.05f + 0.25f * z});
            }
        }
        if (page == MAIN) renderMain();
        else if (page == SETTINGS) settingsMenu.render(ui, flashTime);
        else if (page == BOARD) renderBoard();
        else renderDev();
        ui.end();
    }

private:
    static constexpr int ITEM_H = 44;
    int itemY(int i) const { return screenH / 2 - 150 + i * 50; }
    int itemAt(int mx, int my) const {
        for (int i = 0; i < NUM_ITEMS; ++i) {
            int y = itemY(i);
            if (mx > screenW / 2 - 200 && mx < screenW / 2 + 200 && my > y && my < y + ITEM_H) return i;
        }
        return -1;
    }

    void activate(int idx) {
        if (idx == 0 && onStart) onStart(GameMode::ARENA, StartOptions{});
        if (idx == 1 && onStart && canStartAct2(records)) onStart(GameMode::ACT2, StartOptions{});
        if (idx == 2 && onStart) onStart(GameMode::FAST, StartOptions{});
        if (idx == 3 && onStart) onStart(GameMode::ENDLESS, StartOptions{});
        if (idx == 4 && onStart) onStart(GameMode::DAILY, StartOptions{});
        if (idx == 5) {
            daily = DailyInfo::today();
            board.daily = worldBoard.daily = daily.key();
            page = BOARD; board.load(); boardPoll = 0.f;
#ifdef __EMSCRIPTEN__
            emscripten_run_script("window.overdriveBoard&&window.overdriveBoard.refresh()");
#endif
        }
        if (idx == 6) { page = SETTINGS; settingsMenu.selected = 1; }
        if (idx == 7 && onQuit) onQuit();
    }

    // ---- DEV level select ----------------------------------------------------
    int devCount() const { return DEV_OPTIONS + (int)devRows.size(); }
    float devY(int i) const {
        // options, a gap, then the levels in two columns (ARENA left, FAST right)
        if (i < DEV_OPTIONS) return 128.f + i * 34.f;
        const DevRow& r = devRows[i - DEV_OPTIONS];
        if (r.mode == GameMode::ACT2) return 232.f + (act1Rows + 1 + r.arena) * 34.f;
        return 232.f + r.arena * 34.f;
    }
    float devX(int i) const {
        if (i < DEV_OPTIONS) return screenW / 2.f - 260.f;
        return devRows[i - DEV_OPTIONS].mode != GameMode::FAST ? screenW / 2.f - 560.f : screenW / 2.f + 40.f;
    }
    static constexpr float DEV_W = 520.f;
    int devAt(int mx, int my) const {
        for (int i = 0; i < devCount(); ++i)
            if (mx > devX(i) && mx < devX(i) + DEV_W && my > devY(i) - 6 && my < devY(i) + 26) return i;
        return -1;
    }
    void devActivate(int i, int dir = 1) {
        if (i == 0) { devGod = !devGod; return; }
        if (i == 1) { devWave = (devWave + dir + 6) % 6; return; }
        if (i < DEV_OPTIONS || !onStart) return;
        const DevRow& r = devRows[i - DEV_OPTIONS];
        StartOptions o;
        o.arena = r.arena;
        o.wave  = r.mode != GameMode::FAST ? std::min(devWave, r.waves - 1) : 0;
        o.god   = devGod;
        o.practice = true;
        onStart(r.mode, o);
    }
    void handleDevEvent(const SDL_Event& e) {
        if (e.type == SDL_KEYDOWN) {
            SDL_Keycode k = e.key.keysym.sym;
            int n = devCount();
            if (k == SDLK_ESCAPE || k == SDLK_BACKQUOTE || k == SDLK_F2) { page = MAIN; return; }
            if (k == SDLK_UP)   devSel = (devSel + n - 1) % n;
            if (k == SDLK_DOWN) devSel = (devSel + 1) % n;
            if ((k == SDLK_LEFT || k == SDLK_RIGHT) && devSel >= DEV_OPTIONS) {
                // jump between the ARENA and FAST columns, keeping the row
                const DevRow& r = devRows[devSel - DEV_OPTIONS];
                GameMode want = r.mode == GameMode::FAST ? GameMode::ARENA : GameMode::FAST;
                int best = -1;
                for (int i = 0; i < (int)devRows.size(); ++i)
                    if (devRows[i].mode == want && (best < 0 || std::abs(devRows[i].arena - r.arena) < std::abs(devRows[best].arena - r.arena)))
                        best = i;
                if (best >= 0) devSel = best + DEV_OPTIONS;
            } else if (k == SDLK_LEFT || k == SDLK_RIGHT) devActivate(devSel, k == SDLK_LEFT ? -1 : 1);
            if (k == SDLK_RETURN || k == SDLK_SPACE) devActivate(devSel);
        }
        if (e.type == SDL_MOUSEMOTION) { int i = devAt(e.motion.x, e.motion.y); if (i >= 0) devSel = i; }
        if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
            int i = devAt(e.button.x, e.button.y);
            if (i >= 0) { devSel = i; devActivate(i); }
        }
    }
    void renderDev() {
        int cx = screenW / 2;
        ui.rect(0, 0, (float)screenW, (float)screenH, {0.f, 0.f, 0.02f, 0.55f});
        ui.text("DEV - LEVEL SELECT", cx, 46, 4, {0.3f, 1.f, 0.8f, 1.f}, true);
        ui.text("PRACTICE RUNS: NO RECORDS, NO LEADERBOARD", cx, 90, 1, {0.6f, 0.75f, 0.7f, 0.9f}, true);
        char buf[96];
        for (int i = 0; i < devCount(); ++i) {
            bool sel = i == devSel;
            float x = devX(i), y = devY(i);
            ui.rect(x, y - 6, DEV_W, 30, sel ? glm::vec4{0.05f, 0.2f, 0.17f, 0.95f} : glm::vec4{0.06f, 0.06f, 0.09f, 0.85f});
            if (sel) ui.rect(x, y - 6, 4, 30, {0.3f, 1.f, 0.8f, 1.f});
            glm::vec4 c = sel ? glm::vec4{0.5f, 1.f, 0.85f, 1.f} : glm::vec4{0.75f, 0.75f, 0.8f, 0.95f};
            if (i == 0) std::snprintf(buf, sizeof(buf), "GOD MODE            %s", devGod ? "ON" : "OFF");
            else if (i == 1) std::snprintf(buf, sizeof(buf), "ARENA START WAVE    < %d >", devWave + 1);
            else {
                const DevRow& r = devRows[i - DEV_OPTIONS];
                bool boss = r.mode == GameMode::ARENA && r.arena == (int)countMode(GameMode::ARENA) - 1;
                std::snprintf(buf, sizeof(buf), "%s %d  %s%s",
                              r.mode == GameMode::ARENA ? "ARENA" : r.mode == GameMode::ACT2 ? "ACT II" : "ROOM",
                              r.arena + 1, r.name.c_str(), boss ? "  (BOSS)" : "");
            }
            ui.text(buf, x + 14, y + 1, 2, c);
        }
        ui.text("ARENAS", devX(DEV_OPTIONS) + DEV_W / 2, 206, 1, {0.4f, 1.f, 0.65f, 0.9f}, true);
        ui.text("FAST ROOMS", screenW / 2.f + 40.f + DEV_W / 2, 206, 1, {1.f, 0.65f, 0.25f, 0.9f}, true);
        ui.text("ARROWS / MOUSE TO PICK   ENTER OR CLICK TO GO   LEFT/RIGHT CHANGES OPTIONS   ESC BACK",
                cx, screenH - 54, 1, {0.6f, 0.6f, 0.65f, 0.85f}, true);
        ui.text("IN A PRACTICE RUN: F5 CLEARS THE WAVE   F6 REFILLS HEALTH", cx, screenH - 36, 1,
                {0.6f, 0.6f, 0.65f, 0.85f}, true);
    }
    int countMode(GameMode m) const {
        int n = 0;
        for (auto& r : devRows) if (r.mode == m) ++n;
        return n;
    }

    // ---- LEADERBOARD page ----------------------------------------------------
    void renderBoard() {
        int cx = screenW / 2;
        // The shared board when the site's API is up (re-read as the fetch lands), else this browser's
        if ((boardPoll -= 1.f / 60.f) <= 0.f) { boardPoll = 1.f; worldLoaded = worldBoard.loadOnline(); }
        const Leaderboard& shown = worldLoaded ? worldBoard : board;
        ui.text(worldLoaded ? "WORLD LEADERBOARD" : "LEADERBOARD", cx, 34, 4, {1.f, 0.55f, 0.08f, 1.f}, true);
        const int rows = 8;
        std::string dl = "DAILY  " + daily.label();
        drawLeaderboardTable(ui, shown, Board::ARENA,   cx - 600.f, 84.f,  570.f, rows, -1, flashTime);
        drawLeaderboardTable(ui, shown, Board::FAST,    cx + 30.f,  84.f,  570.f, rows, -1, flashTime);
        drawLeaderboardTable(ui, shown, Board::ENDLESS, cx - 600.f, 338.f, 570.f, rows, -1, flashTime);
        drawLeaderboardTable(ui, shown, Board::DAILY,   cx + 30.f,  338.f, 570.f, rows, -1, flashTime, dl.c_str());
        ui.text("ARENA, ENDLESS AND DAILY RANK BY SCORE (STYLE, SPEED, DAMAGE AVOIDED, DIFFICULTY) - FAST BY TIME",
                cx, screenH - 64, 1, {0.7f, 0.7f, 0.75f, 0.9f}, true);
        ui.text("ESC, ENTER OR CLICK - BACK", cx, screenH - 40, 2, {0.6f, 0.6f, 0.65f, 0.85f}, true);
    }

    // LEFT/RIGHT on the main menu: pick the difficulty for the next run
    void cycleDifficulty(int dir) {
        if (!settings) return;
        settings->difficulty = ((settings->difficulty + dir) % DIFFICULTY_LEVELS + DIFFICULTY_LEVELS) % DIFFICULTY_LEVELS;
        settings->save();
    }

    void renderMain() {
        int cx = screenW / 2;
        ui.text("OVERDRIVE", cx + 4, screenH / 2 - 236, 7, {0.4f, 0.05f, 0.2f, 0.8f}, true);
        ui.text("OVERDRIVE", cx, screenH / 2 - 240, 7, {1.f, 0.55f, 0.08f, 1.f}, true);
        ui.rect(cx - 300, screenH / 2 - 172, 600, 3, {1.f, 0.55f, 0.05f, 0.9f});

        struct Item { const char* label; const char* sub; };
        std::string arenaSub = "5 ARENAS - WAVES - TWO BOSSES";
        std::string fastSub  = "TIME TRIAL - THE GAUNTLET";
        std::string endSub   = "THE CORE - NO EXIT, JUST THE NEXT WAVE";
        std::string daySub   = "TODAY: " + arenaNames[daily.arena] + " - " + daily.modName();
        if (records.bestArenaScore > 0) arenaSub += "   BEST " + std::to_string(records.bestArenaScore);
        if (records.bestFast  > 0.f) fastSub  += "   BEST " + formatTime(records.bestFast);
        if (records.bestEndless > 0) endSub += "   BEST " + std::to_string(records.bestEndless);
        if (records.bestDaily > 0 && records.dailyDate == daily.date) daySub += "   BEST " + std::to_string(records.bestDaily);
        bool act2 = canStartAct2(records);
        Item items[NUM_ITEMS] = {
            {"ARENA", arenaSub.c_str()},
            {"ACT II", act2 ? "PREVIEW - 4/4 ARENAS - BENEATH THE ECLIPSE" : "DEFEAT THE SOVEREIGN TO UNLOCK"},
            {"FAST",  fastSub.c_str()},
            {"ENDLESS", endSub.c_str()},
            {"DAILY", daySub.c_str()},
            {"LEADERBOARD", "EVERY MODE - THE WORLD'S BEST"},
            {"SETTINGS", "SENSITIVITY  FOV  GRAPHICS  MORE"},
            {"EXIT", ""},
        };
        for (int i = 0; i < NUM_ITEMS; ++i) {
            bool sel = selected == i;
            int y = itemY(i);
            float pulse = sel ? 0.5f + 0.5f * std::sin(flashTime * 5.f) : 0.f;
            ui.rect(cx - 200, y, 400, ITEM_H, sel ? glm::vec4{0.13f + pulse * 0.06f, 0.08f, 0.04f, 0.92f}
                                                  : glm::vec4{0.07f, 0.07f, 0.10f, 0.85f});
            glm::vec4 border = sel ? glm::vec4{1.f, 0.6f + pulse * 0.2f, 0.1f, 0.9f} : glm::vec4{0.3f, 0.3f, 0.35f, 0.6f};
            ui.rect(cx - 200, y, 400, 2, border);
            ui.rect(cx - 200, y + ITEM_H - 2, 400, 2, border);
            if (sel) ui.rect(cx - 200, y, 5, ITEM_H, {1.f, 0.6f + pulse * 0.2f, 0.1f, 1.f});
            glm::vec4 c = sel ? glm::vec4{1.f, 0.65f, 0.12f, 1.f} : glm::vec4{0.65f, 0.65f, 0.7f, 0.95f};
            if (i == 1 && !act2) c = {0.42f, 0.42f, 0.47f, 0.75f};   // locked
            bool hasSub = items[i].sub[0] != 0;
            ui.text(items[i].label, cx, y + (hasSub ? 6 : 12), 3, c, true);
            if (hasSub) ui.text(items[i].sub, cx, y + 31, 1, {0.75f, 0.72f, 0.7f, 0.9f}, true);
        }
        if (settings) {
            char d[64];
            std::snprintf(d, sizeof(d), "DIFFICULTY   <  %s  >", difficulty(settings->difficulty).name);
            glm::vec4 dc = settings->difficulty >= 2 ? glm::vec4{1.f, 0.35f, 0.25f, 0.95f} : glm::vec4{1.f, 0.8f, 0.45f, 0.95f};
            ui.text(d, cx, itemY(NUM_ITEMS - 1) + 58, 2, dc, true);
            ui.text("LEFT / RIGHT TO CHANGE", cx, itemY(NUM_ITEMS - 1) + 80, 1, {0.6f, 0.6f, 0.65f, 0.8f}, true);
        }
        float hint = 0.35f + 0.35f * std::sin(flashTime * 1.8f);
        ui.text("UP/DOWN OR MOUSE TO SELECT  -  ENTER OR CLICK TO CONFIRM  -  ` FOR DEV LEVEL SELECT",
                cx, screenH - 30, 1, {hint, hint, hint + 0.05f, 0.85f}, true);
    }
};
