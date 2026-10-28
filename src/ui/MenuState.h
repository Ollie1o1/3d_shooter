#pragma once
#include "core/GameState.h"
#include "core/Settings.h"
#include "ui/SettingsMenu.h"
#include "game/Progression.h"
#include "ui/LeaderboardView.h"
#include "world/Level.h"
#include "world/LevelGauntlet.h"
#include "world/LevelAct2.h"
#include "ui/UIBatch.h"
#include "ui/MenuArt.h"
#include "core/Gamepad.h"
#include <SDL2/SDL.h>
#ifdef __EMSCRIPTEN__
#  include <emscripten.h>
#endif
#include "core/gl.h"
#include <glm/glm.hpp>
#include <functional>
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

// =============================================================================
// MenuState — main menu (ARENA / STORY / FAST / ENDLESS / BOSS / LEADERBOARD /
// SETTINGS / EXIT), the ARENA page (the whole of Act I, the whole of Act II once
// Act I is beaten, or any arena of either on its own), the BOSS page (every boss in order, fought on its own), the
// settings page, the leaderboard page, and the DEV level select (press ` or
// F2 on the main menu): jump straight to any arena, wave, FAST room or the
// boss, optionally in god mode. Dev runs are practice: no records.
// =============================================================================
class MenuState : public GameState {
public:
    std::function<void(GameMode, StartOptions)> onStart;
    std::function<void()>         onQuit;

    GameSettings* settings = nullptr;   // injected by main

    enum Page { MAIN, SETTINGS, BOARD, DEV, BOSS, FASTPAGE, ARENAPAGE };
    // The ARENA page: two columns, each the whole act then its arenas one by one
    struct ArenaRow { std::string name, subtitle; };
    std::vector<ArenaRow> act1Arenas, act2Arenas;
    int   arenaSel = 0;
    float storyNudge = 0.f;   // STORY was picked: it isn't here yet (the row flashes COMING SOON)
    // The pictures behind the pages (MenuArt.h): an arena, fading to the next; the BOSS page's boss
    menuart::Fade backdrop, portrait;
    float slideT = 0.f;       // the main menu's slideshow of the arenas
    int   fastSel = 0;   // the FAST page: 0 the whole Gauntlet, 1.. a room on its own (ROOM ATTACK)
    struct FastRoom { std::string name; float medals[4]; };
    std::vector<FastRoom> fastRooms;
    int   bossSel = 0;
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
    WorldBest    bossWorld[BOSS_COUNT];   // web: each boss's world record (the site's board), as the page caches it
    float        bossPoll = 0.f;
    // Re-read the world records every second (the fetch lands a moment after the page opens)
    void pollBossWorld() {
        if ((bossPoll -= 1.f / 60.f) > 0.f) return;
        bossPoll = 1.f;
        for (int i = 0; i < BOSS_COUNT; ++i) bossWorld[i] = loadBossWorld(i);
    }
    static void refreshWorld() {
#ifdef __EMSCRIPTEN__
        emscripten_run_script("window.overdriveBoard&&window.overdriveBoard.refresh()");
#endif
    }

    static constexpr int NUM_ITEMS = 8;
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
        board.load();
        LevelData A = buildLevel(), F = buildGauntlet();
        for (auto& a : A.arenas) { arenaNames.push_back(a.name); act1Arenas.push_back({a.name ? a.name : "", a.subtitle ? a.subtitle : ""}); }
        for (int i = 0; i < (int)A.arenas.size(); ++i)
            devRows.push_back({"ARENA", A.arenas[i].name, GameMode::ARENA, i, (int)A.arenas[i].waves.size()});
        for (int i = 0; i < (int)F.arenas.size(); ++i) {
            devRows.push_back({"FAST", F.arenas[i].name, GameMode::FAST, i, (int)F.arenas[i].waves.size()});
            FastRoom r{F.arenas[i].name, {}};
            for (int k = 0; k < 4; ++k) r.medals[k] = F.arenas[i].medals[k];
            fastRooms.push_back(r);
        }
        act1Rows = (int)A.arenas.size();
        LevelData A2 = buildAct2Level();
        for (auto& a : A2.arenas) act2Arenas.push_back({a.name ? a.name : "", a.subtitle ? a.subtitle : ""});
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
        if (page == BOSS) { handleBossEvent(e); return; }
        if (page == FASTPAGE) { handleFastEvent(e); return; }
        if (page == ARENAPAGE) { handleArenaEvent(e); return; }
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
            else if (int d = difficultyClick(e.button.x, e.button.y, itemY(NUM_ITEMS - 1) + 58.f)) cycleDifficulty(d);
        }
        if (e.type == SDL_MOUSEMOTION) {
            int i = itemAt(e.motion.x, e.motion.y);
            if (i >= 0) selected = i;
        }
    }

    void update(float dt) override {
        flashTime += dt; storyNudge = std::max(0.f, storyNudge - dt);
        slideT += dt;
        backdrop.update(dt, page == MAIN ? 1.6f : 0.35f);
        portrait.update(dt, 0.25f);
    }

    void render() override {
        glClearColor(0.04f, 0.03f, 0.08f, 1.f);
        glClear(GL_COLOR_BUFFER_BIT);
        ui.begin();
        // Background: vertical gradient and a slowly scrolling synthwave grid
        for (int i = 0; i < 24; ++i) {
            float t = (float)i / 24.f;
            ui.rect(0, t * screenH, screenW, screenH / 24.f + 1, {0.04f + t * 0.05f, 0.02f + t * 0.015f, 0.08f + t * 0.07f, 1.f});
        }
        drawBackdrop();
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
        else if (page == BOSS) renderBoss();
        else if (page == FASTPAGE) renderFast();
        else if (page == ARENAPAGE) renderArena();
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
        if (idx == 0) { page = ARENAPAGE; arenaSel = 0; }
        if (idx == 1) storyNudge = 1.2f;   // STORY: not yet
        if (idx == 2) { page = FASTPAGE; fastSel = 0; }
        if (idx == 3 && onStart) onStart(GameMode::ENDLESS, StartOptions{});
        if (idx == 4) { page = BOSS; bossSel = 0; bossPoll = 0.f; refreshWorld(); }
        if (idx == 5) {
            page = BOARD; board.load(); boardPoll = 0.f; bossPoll = 0.f;
#ifdef __EMSCRIPTEN__
            emscripten_run_script("window.overdriveBoard&&window.overdriveBoard.refresh()");
#endif
        }
        if (idx == 6) { page = SETTINGS; settingsMenu.selected = 1; settingsMenu.bindsPage = false; }
        if (idx == 7 && onQuit) onQuit();
    }

    // ---- the pictures behind the pages ---------------------------------------
    // The main menu: every arena in turn, slowly; the ARENA page: the arena picked; the BOSS page: the
    // boss's arena. Faint, so the page stays the thing you read
    void drawBackdrop() {
        const menuart::Pic* want = nullptr;
        float alpha = 0.f;
        if (page == MAIN) {
            const int n1 = (int)act1Arenas.size(), n = n1 + (int)act2Arenas.size();
            const int k = n ? (int)(slideT / 7.f) % n : 0;
            want = &menuart::arena(k >= n1, k >= n1 ? k - n1 : k);
            alpha = 0.22f;
        } else if (page == ARENAPAGE) {
            int col, row; arenaCell(arenaSel, col, row);
            want = &menuart::arena(col == 1, std::max(0, row - 1));
            alpha = 0.34f;
        } else if (page == BOSS) {
            const BossEntry& b = bossEntry(bossSel);
            want = &menuart::arena(b.act2, b.arena);
            alpha = 0.3f;
        }
        backdrop.show(want && want->tex ? want : nullptr);
        auto draw = [&](const menuart::Pic* p, float a) {
            if (!p || !p->tex || a <= 0.f) return;
            // cover the screen, cropped to its shape
            float sx = (float)screenW / p->w, sy = (float)screenH / p->h, k = std::max(sx, sy);
            float w = p->w * k, h = p->h * k;
            ui.image(p->tex, (screenW - w) * 0.5f, (screenH - h) * 0.5f, w, h, {1.f, 1.f, 1.f, a});
        };
        float t = backdrop.t * backdrop.t * (3.f - 2.f * backdrop.t);
        draw(backdrop.was, alpha * (1.f - t));
        draw(backdrop.now, alpha * t);
    }

    // ---- ARENA: the whole of Act I, the whole of Act II (once Act I is beaten), or any arena on its own ----
    // Rows: column 0 is ACT I then its arenas, column 1 ACT II then its arenas (Up/Down walk both in turn)
    static constexpr float ARENA_COL_W = 600.f;
    int arenaRows() const { return 2 + (int)act1Arenas.size() + (int)act2Arenas.size(); }
    // which column (0 Act I, 1 Act II) and which row in it (0 the whole act, 1.. an arena)
    void arenaCell(int i, int& col, int& row) const {
        const int n0 = 1 + (int)act1Arenas.size();
        col = i < n0 ? 0 : 1; row = i < n0 ? i : i - n0;
    }
    glm::vec4 arenaRect(int i) const {   // x, y, w, h
        int col, row; arenaCell(i, col, row);
        const float x = screenW / 2.f + (col == 0 ? -ARENA_COL_W - 10.f : 10.f);
        if (row == 0) return {x, 124.f, ARENA_COL_W, 62.f};
        return {x, 222.f + (row - 1) * 50.f, ARENA_COL_W, 44.f};
    }
    float arenaBottom() const {
        return 222.f + (float)std::max(act1Arenas.size(), act2Arenas.size()) * 50.f;
    }
    int arenaAt(int mx, int my) const {
        for (int i = 0; i < arenaRows(); ++i) {
            glm::vec4 r = arenaRect(i);
            if (mx > r.x && mx < r.x + r.z && my > r.y && my < r.y + r.w) return i;
        }
        return -1;
    }
    void startArena(int i) {
        if (!onStart) return;
        int col, row; arenaCell(i, col, row);
        const GameMode m = col == 0 ? GameMode::ARENA : GameMode::ACT2;
        if (row == 0) {   // the whole act: Act II only once Act I is beaten
            if (col == 1 && !canStartAct2(records)) return;
            onStart(m, StartOptions{});
            return;
        }
        StartOptions o;   // one arena on its own: always open
        o.arena = o.solo = row - 1;
        onStart(m, o);
    }
    void handleArenaEvent(const SDL_Event& e) {
        if (e.type == SDL_KEYDOWN) {
            SDL_Keycode k = e.key.keysym.sym;
            const int n = arenaRows();
            if (k == SDLK_ESCAPE || k == SDLK_BACKSPACE) { page = MAIN; selected = 0; return; }
            if (k == SDLK_UP)   arenaSel = (arenaSel + n - 1) % n;
            if (k == SDLK_DOWN) arenaSel = (arenaSel + 1) % n;
            if (k == SDLK_LEFT || k == SDLK_RIGHT) cycleDifficulty(k == SDLK_LEFT ? -1 : 1);
            if (k == SDLK_RETURN || k == SDLK_SPACE) startArena(arenaSel);
        }
        if (e.type == SDL_MOUSEMOTION) { int i = arenaAt(e.motion.x, e.motion.y); if (i >= 0) arenaSel = i; }
        if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
            int i = arenaAt(e.button.x, e.button.y);
            if (i >= 0) { arenaSel = i; startArena(i); }
            else if (int d = difficultyClick(e.button.x, e.button.y, arenaBottom() + 8.f)) cycleDifficulty(d);
        }
    }
    void renderArena() {
        const int cx = screenW / 2;
        ui.text("ARENA", cx, 40, 5, {1.f, 0.55f, 0.08f, 1.f}, true);
        ui.text("A WHOLE ACT, START TO FINISH - OR ANY ARENA ON ITS OWN", cx, 96, 1, {0.75f, 0.72f, 0.7f, 0.9f}, true);
        const bool act2Open = canStartAct2(records);
        char buf[96];
        for (int i = 0; i < arenaRows(); ++i) {
            int col, row; arenaCell(i, col, row);
            const glm::vec4 r = arenaRect(i);
            const float x = r.x, y = r.y, w = r.z, h = r.w;
            const bool sel = i == arenaSel, locked = col == 1 && row == 0 && !act2Open;
            const float pulse = sel ? 0.5f + 0.5f * std::sin(flashTime * 5.f) : 0.f;
            const glm::vec3 accent = col == 0 ? glm::vec3{1.f, 0.6f, 0.1f} : glm::vec3{0.35f, 0.95f, 0.9f};
            ui.rect(x, y, w, h, sel ? glm::vec4{0.13f + pulse * 0.06f, 0.08f, 0.04f + col * 0.05f, 0.92f} : glm::vec4{0.07f, 0.07f, 0.10f, 0.85f});
            if (sel) ui.rect(x, y, 5, h, {accent.x, accent.y + pulse * 0.1f, accent.z, 1.f});
            glm::vec4 c = locked ? glm::vec4{0.42f, 0.42f, 0.47f, 0.75f}
                        : sel ? glm::vec4{accent, 1.f} : glm::vec4{0.8f, 0.8f, 0.85f, 0.95f};
            const glm::vec4 dim{0.7f, 0.68f, 0.66f, 0.85f}, gold{1.f, 0.85f, 0.45f, 0.9f};
            if (row == 0) {   // the whole act
                ui.text(col == 0 ? "ACT I" : "ACT II", x + 22, y + 10, 3, c);
                if (col == 0) {
                    ui.text("EVERY ARENA IN A ROW - TWO BOSSES - RANKED", x + 22, y + 40, 1, dim);
                    if (records.bestArenaScore > 0) {
                        std::snprintf(buf, sizeof(buf), "BEST %d", records.bestArenaScore);
                        ui.textRight(buf, (int)(x + w - 18), (int)y + 12, 2, gold);
                    }
                } else {
                    std::string sub = act2Subtitle(records, act2Open);
                    if (!act2Open) sub = "CLEAR ACT I TO RUN THE WHOLE ACT";
                    ui.text(sub.substr(0, 90).c_str(), x + 22, y + 40, 1, locked ? glm::vec4{1.f, 0.55f, 0.3f, 0.9f} : dim);
                    if (locked) ui.textRight("LOCKED", (int)(x + w - 18), (int)y + 12, 2, {1.f, 0.55f, 0.3f, 0.9f});
                }
                continue;
            }
            const int a = row - 1;
            const ArenaRow& ar = col == 0 ? act1Arenas[a] : act2Arenas[a];
            std::snprintf(buf, sizeof(buf), "%d", row);
            ui.text(buf, x + 18, y + 10, 3, {0.5f, 0.5f, 0.55f, 0.8f});
            ui.text(ar.name.c_str(), x + 56, y + 5, 2, c);
            ui.text(ar.subtitle.substr(0, 60).c_str(), x + 56, y + 28, 1, dim);
            const float best = records.soloBest(col == 1, a);
            ui.textRight(best > 0.f ? formatTime(best).c_str() : "-", (int)(x + w - 18), (int)y + 12, 2, gold);
        }
        ui.text("ACT I ARENAS", cx - (int)(ARENA_COL_W / 2) - 10, 204, 1, {1.f, 0.65f, 0.25f, 0.9f}, true);
        ui.text("ACT II ARENAS - ALWAYS OPEN", cx + (int)(ARENA_COL_W / 2) + 10, 204, 1, {0.35f, 0.95f, 0.9f, 0.9f}, true);
        if (settings) {
            std::snprintf(buf, sizeof(buf), "DIFFICULTY   <  %s  >", difficulty(settings->difficulty).name);
            ui.text(buf, cx, arenaBottom() + 8, 2, {1.f, 0.8f, 0.45f, 0.95f}, true);
        }
        ui.text("ONE ARENA ON ITS OWN: THE UPGRADES A RUN WOULD HAVE BY THEN, YOUR BEST CLEAR TIME KEPT",
                cx, screenH - 58, 1, {0.6f, 0.6f, 0.65f, 0.85f}, true);
        ui.text("UP/DOWN OR MOUSE TO PICK   ENTER OR CLICK TO PLAY   LEFT/RIGHT DIFFICULTY   ESC BACK",
                cx, screenH - 36, 1, {0.6f, 0.6f, 0.65f, 0.85f}, true);
    }

    // ---- FAST: the whole Gauntlet, or ROOM ATTACK (a room on its own, for its medal) ----
    int fastRows() const { return 1 + (int)fastRooms.size(); }
    float fastY(int i) const { return i == 0 ? 140.f : 214.f + (i - 1) * 54.f; }
    int fastAt(int mx, int my) const {
        for (int i = 0; i < fastRows(); ++i)
            if (mx > screenW / 2 - BOSS_W / 2 && mx < screenW / 2 + BOSS_W / 2 && my > fastY(i) && my < fastY(i) + (i == 0 ? 60 : 48)) return i;
        return -1;
    }
    void startFast(int i) {
        if (!onStart) return;
        StartOptions o;
        if (i > 0) { o.room = i - 1; o.arena = i - 1; }
        onStart(GameMode::FAST, o);
    }
    void handleFastEvent(const SDL_Event& e) {
        if (e.type == SDL_KEYDOWN) {
            SDL_Keycode k = e.key.keysym.sym;
            if (k == SDLK_ESCAPE || k == SDLK_BACKSPACE) { page = MAIN; selected = 2; return; }
            if (k == SDLK_UP)   fastSel = (fastSel + fastRows() - 1) % fastRows();
            if (k == SDLK_DOWN) fastSel = (fastSel + 1) % fastRows();
            if (k == SDLK_LEFT || k == SDLK_RIGHT) cycleDifficulty(k == SDLK_LEFT ? -1 : 1);
            if (k == SDLK_RETURN || k == SDLK_SPACE) startFast(fastSel);
        }
        if (e.type == SDL_MOUSEMOTION) { int i = fastAt(e.motion.x, e.motion.y); if (i >= 0) fastSel = i; }
        if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
            int i = fastAt(e.button.x, e.button.y);
            if (i >= 0) { fastSel = i; startFast(i); }
            else if (int d = difficultyClick(e.button.x, e.button.y, fastY(fastRows()) + 4.f)) cycleDifficulty(d);
        }
    }
    void renderFast() {
        int cx = screenW / 2;
        ui.text("FAST", cx, 46, 5, {1.f, 0.55f, 0.08f, 1.f}, true);
        ui.text("THE WHOLE GAUNTLET AGAINST THE CLOCK - OR ONE ROOM ON ITS OWN FOR ITS MEDAL", cx, 104, 1, {0.75f, 0.72f, 0.7f, 0.9f}, true);
        char buf[96];
        int golds = 0;
        for (int m : records.fastMedals) golds += m >= 3;
        for (int i = 0; i < fastRows(); ++i) {
            bool sel = i == fastSel;
            float x = cx - BOSS_W / 2, y = fastY(i), h = i == 0 ? 60.f : 48.f;
            float pulse = sel ? 0.5f + 0.5f * std::sin(flashTime * 5.f) : 0.f;
            ui.rect(x, y, BOSS_W, h, sel ? glm::vec4{0.13f + pulse * 0.06f, 0.08f, 0.04f, 0.92f} : glm::vec4{0.07f, 0.07f, 0.10f, 0.85f});
            if (sel) ui.rect(x, y, 5, h, {1.f, 0.6f + pulse * 0.2f, 0.1f, 1.f});
            glm::vec4 c = sel ? glm::vec4{1.f, 0.65f, 0.12f, 1.f} : glm::vec4{0.8f, 0.8f, 0.85f, 0.95f};
            if (i == 0) {
                ui.text("THE GAUNTLET", x + 22, y + 10, 3, c);
                ui.text("SEVEN ROOMS, A MINI-BOSS IN EACH, THEN THE PACEMAKER - SPLITS, GHOST, RANK", x + 22, y + 40, 1, {0.7f, 0.68f, 0.66f, 0.85f});
                std::snprintf(buf, sizeof(buf), "BEST %s", records.bestFast > 0.f ? formatTime(records.bestFast).c_str() : "-");
                ui.textRight(buf, (int)(x + BOSS_W - 18), (int)y + 12, 2, {1.f, 0.85f, 0.45f, 0.9f});
                std::snprintf(buf, sizeof(buf), "GOLD IN %d/%d ROOMS", golds, (int)fastRooms.size());
                ui.textRight(buf, (int)(x + BOSS_W - 18), (int)y + 38, 1, {1.f, 0.82f, 0.25f, 0.85f});
                continue;
            }
            const int r = i - 1;
            const FastRoom& fr = fastRooms[r];
            std::snprintf(buf, sizeof(buf), "%d", i);
            ui.text(buf, x + 22, y + 12, 3, {0.5f, 0.5f, 0.55f, 0.8f});
            ui.text(fr.name.c_str(), x + 62, y + 6, 2, c);
            std::snprintf(buf, sizeof(buf), "GOLD %s  SILVER %s  BRONZE %s", formatTime(fr.medals[0], false).c_str(),
                          formatTime(fr.medals[1], false).c_str(), formatTime(fr.medals[2], false).c_str());
            ui.text(buf, x + 62, y + 30, 1, {0.7f, 0.68f, 0.66f, 0.85f});
            const float best = r < (int)records.fastRooms.size() ? records.fastRooms[r] : 0.f;
            const int medal = r < (int)records.fastMedals.size() ? records.fastMedals[r] : 0;
            glm::vec3 mc = medalColor(medal);
            std::snprintf(buf, sizeof(buf), "%s", best > 0.f ? formatTime(best).c_str() : "-");
            ui.textRight(buf, (int)(x + BOSS_W - 18), (int)y + 6, 2, {1.f, 0.85f, 0.45f, 0.9f});
            if (medal) ui.textRight(medalName(medal), (int)(x + BOSS_W - 18), (int)y + 30, 1, {mc.x, mc.y, mc.z, 1.f});
        }
        if (settings) {
            std::snprintf(buf, sizeof(buf), "DIFFICULTY   <  %s  >", difficulty(settings->difficulty).name);
            ui.text(buf, cx, fastY(fastRows()) + 4, 2, {1.f, 0.8f, 0.45f, 0.95f}, true);
        }
        ui.text("UP/DOWN OR MOUSE TO PICK   ENTER OR CLICK TO RUN   LEFT/RIGHT DIFFICULTY   ESC BACK",
                cx, screenH - 36, 1, {0.6f, 0.6f, 0.65f, 0.85f}, true);
    }

    // ---- BOSS: every boss in the order you meet them ---------------------------
    float bossY(int i) const { return 150.f + i * 66.f; }
    static constexpr float BOSS_W = 640.f;
    int bossAt(int mx, int my) const {
        for (int i = 0; i < BOSS_COUNT; ++i)
            if (mx > screenW / 2 - BOSS_W / 2 && mx < screenW / 2 + BOSS_W / 2 && my > bossY(i) && my < bossY(i) + 56) return i;
        return -1;
    }
    void startBoss(int i) {
        if (!onStart || !bossOpen(records, i)) return;
        StartOptions o;
        o.boss = i; o.arena = bossEntry(i).arena;
        onStart(bossEntry(i).act2 ? GameMode::ACT2 : GameMode::ARENA, o);
    }
    void handleBossEvent(const SDL_Event& e) {
        if (e.type == SDL_KEYDOWN) {
            SDL_Keycode k = e.key.keysym.sym;
            if (k == SDLK_ESCAPE || k == SDLK_BACKSPACE) { page = MAIN; selected = 4; return; }
            if (k == SDLK_UP)   bossSel = (bossSel + BOSS_COUNT - 1) % BOSS_COUNT;
            if (k == SDLK_DOWN) bossSel = (bossSel + 1) % BOSS_COUNT;
            if (k == SDLK_LEFT || k == SDLK_RIGHT) cycleDifficulty(k == SDLK_LEFT ? -1 : 1);
            if (k == SDLK_RETURN || k == SDLK_SPACE) startBoss(bossSel);
        }
        if (e.type == SDL_MOUSEMOTION) { int i = bossAt(e.motion.x, e.motion.y); if (i >= 0) bossSel = i; }
        if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
            int i = bossAt(e.button.x, e.button.y);
            if (i >= 0) { bossSel = i; startBoss(i); }
            else if (int d = difficultyClick(e.button.x, e.button.y, bossY(BOSS_COUNT) + 10.f)) cycleDifficulty(d);
        }
    }
    void renderBoss() {
        int cx = screenW / 2;
        pollBossWorld();
        {   // the boss picked, beside the list (its render: MenuArt.h)
            const menuart::Pic& pic = menuart::boss(bossSel);
            portrait.show(pic.tex ? &pic : nullptr);
            const float x = cx + BOSS_W / 2 + 16.f, w = screenW - x - 16.f, h = w * 1.25f, y = 150.f;
            float t = portrait.t * portrait.t * (3.f - 2.f * portrait.t);
            if (portrait.was && portrait.was->tex) ui.image(portrait.was->tex, x, y, w, h, {1.f, 1.f, 1.f, 1.f - t});
            if (portrait.now && portrait.now->tex) ui.image(portrait.now->tex, x, y, w, h, {1.f, 1.f, 1.f, t});
        }
        ui.text("BOSS", cx, 46, 5, {1.f, 0.55f, 0.08f, 1.f}, true);
        ui.text("EVERY BOSS IN ORDER, ON ITS OWN - WITH THE UPGRADES A RUN WOULD HAVE BY THEN", cx, 104, 1, {0.75f, 0.72f, 0.7f, 0.9f}, true);
        char buf[96];
        for (int i = 0; i < BOSS_COUNT; ++i) {
            const BossEntry& b = bossEntry(i);
            bool sel = i == bossSel, open = bossOpen(records, i);
            float x = cx - BOSS_W / 2, y = bossY(i);
            float pulse = sel ? 0.5f + 0.5f * std::sin(flashTime * 5.f) : 0.f;
            ui.rect(x, y, BOSS_W, 56, sel ? glm::vec4{0.13f + pulse * 0.06f, 0.08f, 0.04f, 0.92f} : glm::vec4{0.07f, 0.07f, 0.10f, 0.85f});
            if (sel) ui.rect(x, y, 5, 56, {1.f, 0.6f + pulse * 0.2f, 0.1f, 1.f});
            glm::vec4 c = !open ? glm::vec4{0.42f, 0.42f, 0.47f, 0.75f} : sel ? glm::vec4{1.f, 0.65f, 0.12f, 1.f} : glm::vec4{0.8f, 0.8f, 0.85f, 0.95f};
            std::snprintf(buf, sizeof(buf), "%d", i + 1);
            ui.text(buf, x + 22, y + 16, 3, {0.5f, 0.5f, 0.55f, 0.8f});
            ui.text(b.name, x + 62, y + 8, 3, c);
            ui.text(b.where, x + 62, y + 36, 1, {0.7f, 0.68f, 0.66f, 0.85f});
            // your best kill, and the world's (who holds it) when the site's board has one
            const WorldBest& w = bossWorld[i];
            std::snprintf(buf, sizeof(buf), "YOU %s", records.bestBoss[i] > 0.f ? formatTime(records.bestBoss[i]).c_str() : "-");
            ui.textRight(buf, (int)(x + BOSS_W - 18), (int)y + (w.time > 0.f ? 9 : 20), 2, {1.f, 0.85f, 0.45f, 0.9f});
            if (w.time > 0.f) {
                const bool mine = records.bestBoss[i] > 0.f && std::fabs(records.bestBoss[i] - w.time) < 0.02f;
                std::snprintf(buf, sizeof(buf), "WORLD %s  %s", formatTime(w.time).c_str(), w.name.c_str());
                ui.textRight(buf, (int)(x + BOSS_W - 18), (int)y + 34, 1,
                             mine ? glm::vec4{0.45f, 1.f, 0.6f, 0.95f} : glm::vec4{0.55f, 0.85f, 1.f, 0.9f});
            }
        }
        if (settings) {
            std::snprintf(buf, sizeof(buf), "DIFFICULTY   <  %s  >", difficulty(settings->difficulty).name);
            ui.text(buf, cx, bossY(BOSS_COUNT) + 10, 2, {1.f, 0.8f, 0.45f, 0.95f}, true);
        }
        ui.text("UP/DOWN OR MOUSE TO PICK   ENTER OR CLICK TO FIGHT   LEFT/RIGHT DIFFICULTY   ESC BACK",
                cx, screenH - 36, 1, {0.6f, 0.6f, 0.65f, 0.85f}, true);
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
        drawLeaderboardTable(ui, shown, Board::ARENA,   cx - 600.f, 84.f,  570.f, rows, -1, flashTime);
        drawLeaderboardTable(ui, shown, Board::FAST,    cx + 30.f,  84.f,  570.f, rows, -1, flashTime);
        drawLeaderboardTable(ui, shown, Board::ENDLESS, cx - 600.f, 338.f, 570.f, rows, -1, flashTime);
        {   // BOSS: each boss's world record (who holds it) and your own best kill
            pollBossWorld();
            float x = cx + 30.f, y = 338.f;
            ui.rect(x, y, 570.f, 32.f + rows * 22.f, {0.06f, 0.06f, 0.09f, 0.85f});
            ui.rect(x, y, 570.f, 3.f, {1.f, 0.6f, 0.25f, 0.9f});
            ui.text("BOSS  -  FASTEST KILLS", x + 285.f, y + 8, 2, {1.f, 0.6f, 0.25f, 1.f}, true);
            ui.textRight("WORLD", (int)(x + 300), (int)y + 30, 1, {0.55f, 0.85f, 1.f, 0.8f});
            ui.textRight("YOU", (int)(x + 546), (int)y + 30, 1, {1.f, 0.85f, 0.45f, 0.8f});
            char buf[64];
            for (int i = 0; i < BOSS_COUNT; ++i) {
                float ry = y + 44.f + i * 22.f;
                ui.text(bossEntry(i).name, x + 24, ry, 2, {0.8f, 0.8f, 0.85f, 0.95f});
                const WorldBest& w = bossWorld[i];
                if (w.time > 0.f) {
                    std::snprintf(buf, sizeof(buf), "%s", formatTime(w.time).c_str());
                    ui.textRight(buf, (int)(x + 300), (int)ry, 2, {0.55f, 0.85f, 1.f, 0.95f});
                    ui.text(w.name.c_str(), x + 310, ry + 4, 1, {0.55f, 0.75f, 0.9f, 0.8f});
                } else ui.textRight("-", (int)(x + 300), (int)ry, 2, {0.55f, 0.85f, 1.f, 0.6f});
                if (records.bestBoss[i] > 0.f) std::snprintf(buf, sizeof(buf), "%s", formatTime(records.bestBoss[i]).c_str());
                else std::snprintf(buf, sizeof(buf), "-");
                ui.textRight(buf, (int)(x + 546), (int)ry, 2, {1.f, 0.85f, 0.45f, 0.9f});
            }
        }
        ui.text("ARENA AND ENDLESS RANK BY SCORE (STYLE, SPEED, DAMAGE AVOIDED, DIFFICULTY) - FAST BY TIME",
                cx, screenH - 64, 1, {0.7f, 0.7f, 0.75f, 0.9f}, true);
        ui.text("ESC, ENTER OR CLICK - BACK", cx, screenH - 40, 2, {0.6f, 0.6f, 0.65f, 0.85f}, true);
    }

    // The DIFFICULTY  <  NAME  > line (drawn centred at lineY, scale 2): a click on its left half (the < side,
    // up to the middle of the name) goes down a level, on its right half up one. 0: the click missed it
    int difficultyClick(int mx, int my, float lineY) const {
        if (!settings) return 0;
        char buf[64];
        std::snprintf(buf, sizeof(buf), "DIFFICULTY   <  %s  >", difficulty(settings->difficulty).name);
        const float w = (float)UIBatch::textWidth(buf, 2), x0 = screenW / 2.f - w / 2.f;
        if (my < lineY - 8.f || my > lineY + 24.f || mx < x0 - 12.f || mx > x0 + w + 12.f) return 0;
        const float lt = x0 + 13 * 12.f;   // the "<" (13 characters in, 12 px each at scale 2)
        if (mx < lt - 40.f) return 0;      // the word DIFFICULTY itself: not a button
        const float nameMid = lt + (3.f + std::strlen(difficulty(settings->difficulty).name) * 0.5f) * 12.f;
        return mx < nameMid ? -1 : 1;
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
        std::string arenaSub = "ACT I - ACT II - OR ANY ARENA ON ITS OWN";
        std::string fastSub  = "TIME TRIAL - THE GAUNTLET";
        std::string endSub   = "THE CORE - NO EXIT, JUST THE NEXT WAVE";
        int bossBest = 0;
        for (float b : records.bestBoss) bossBest += b > 0.f;
        std::string bossSub  = "EVERY BOSS IN ORDER - FIGHT ANY ONE ON ITS OWN";
        if (bossBest > 0) bossSub = "EVERY BOSS IN ORDER   " + std::to_string(bossBest) + "/" + std::to_string(BOSS_COUNT) + " BEATEN";
        if (records.bestArenaScore > 0) arenaSub += "   BEST " + std::to_string(records.bestArenaScore);
        if (records.bestFast  > 0.f) fastSub  += "   BEST " + formatTime(records.bestFast);
        if (records.bestEndless > 0) endSub += "   BEST " + std::to_string(records.bestEndless);
        const bool nudge = storyNudge > 0.f;
        Item items[NUM_ITEMS] = {
            {"ARENA", arenaSub.c_str()},
            {"STORY", nudge ? "COMING SOON - WE'RE STILL WRITING IT" : "COMING SOON"},
            {"FAST",  fastSub.c_str()},
            {"ENDLESS", endSub.c_str()},
            {"BOSS", bossSub.c_str()},
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
            if (i == 1) c = nudge ? glm::vec4{1.f, 0.55f, 0.3f, 0.6f + 0.4f * std::sin(flashTime * 12.f)}
                                  : glm::vec4{0.42f, 0.42f, 0.47f, 0.75f};   // STORY: not yet
            bool hasSub = items[i].sub[0] != 0;
            ui.text(items[i].label, cx, y + (hasSub ? 6 : 12), 3, c, true);
            if (hasSub) ui.text(items[i].sub, cx, y + 31, 1, {0.75f, 0.72f, 0.7f, 0.9f}, true);
        }
        if (settings) {
            char d[64];
            std::snprintf(d, sizeof(d), "DIFFICULTY   <  %s  >", difficulty(settings->difficulty).name);
            glm::vec4 dc = settings->difficulty >= 2 ? glm::vec4{1.f, 0.35f, 0.25f, 0.95f} : glm::vec4{1.f, 0.8f, 0.45f, 0.95f};
            ui.text(d, cx, itemY(NUM_ITEMS - 1) + 58, 2, dc, true);
            ui.text("LEFT / RIGHT OR CLICK TO CHANGE", cx, itemY(NUM_ITEMS - 1) + 80, 1, {0.6f, 0.6f, 0.65f, 0.8f}, true);
        }
        float hint = 0.35f + 0.35f * std::sin(flashTime * 1.8f);
        ui.text("UP/DOWN OR MOUSE TO SELECT  -  ENTER OR CLICK TO CONFIRM  -  ` FOR DEV LEVEL SELECT",
                cx, screenH - 30, 1, {hint, hint, hint + 0.05f, 0.85f}, true);
    }
};
