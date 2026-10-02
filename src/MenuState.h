#pragma once
#include "GameState.h"
#include "Settings.h"
#include "SettingsMenu.h"
#include "Progression.h"
#include "UIBatch.h"
#include <SDL2/SDL.h>
#include "gl.h"
#include <glm/glm.hpp>
#include <functional>
#include <cmath>
#include <cstdio>
#include <string>

// =============================================================================
// MenuState — main menu (ARENA / FAST / SETTINGS / EXIT) and the settings page
// =============================================================================
class MenuState : public GameState {
public:
    std::function<void(GameMode)> onStart;
    std::function<void()>         onQuit;

    GameSettings* settings = nullptr;   // injected by main

    int   page      = 0;   // 0=main, 1=settings
    int   selected  = 0;
    float flashTime = 0.f;
    int   screenW, screenH;

    UIBatch      ui;
    SettingsMenu settingsMenu;
    Records      records;

    static constexpr int NUM_ITEMS = 4;

    MenuState(int w, int h) : screenW(w), screenH(h), ui(w, h), settingsMenu(w, h) {
        settingsMenu.onBack = [this]() { page = 0; selected = 2; };
        records.load();
    }

    void handleEvent(const SDL_Event& e) override {
        settingsMenu.s = settings;
        if (page == 1) { settingsMenu.handleEvent(e); return; }
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
        if (page == 0) renderMain(); else settingsMenu.render(ui, flashTime);
        ui.end();
    }

private:
    int itemY(int i) const { return screenH / 2 - 92 + i * 66; }
    int itemAt(int mx, int my) const {
        for (int i = 0; i < NUM_ITEMS; ++i) {
            int y = itemY(i);
            if (mx > screenW / 2 - 200 && mx < screenW / 2 + 200 && my > y && my < y + 54) return i;
        }
        return -1;
    }

    void activate(int idx) {
        if (idx == 0 && onStart) onStart(GameMode::ARENA);
        if (idx == 1 && onStart) onStart(GameMode::FAST);
        if (idx == 2) { page = 1; settingsMenu.selected = 1; }
        if (idx == 3 && onQuit) onQuit();
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
        std::string arenaSub = "4 ARENAS - WAVES - THE WARDEN";
        std::string fastSub  = "TIME TRIAL - THE GAUNTLET";
        if (records.bestArena > 0.f) arenaSub += "   BEST " + formatTime(records.bestArena);
        if (records.bestFast  > 0.f) fastSub  += "   BEST " + formatTime(records.bestFast);
        Item items[NUM_ITEMS] = {
            {"ARENA", arenaSub.c_str()},
            {"FAST",  fastSub.c_str()},
            {"SETTINGS", "SENSITIVITY  FOV  AUDIO  MORE"},
            {"EXIT", ""},
        };
        for (int i = 0; i < NUM_ITEMS; ++i) {
            bool sel = selected == i;
            int y = itemY(i);
            float pulse = sel ? 0.5f + 0.5f * std::sin(flashTime * 5.f) : 0.f;
            ui.rect(cx - 200, y, 400, 54, sel ? glm::vec4{0.13f + pulse * 0.06f, 0.08f, 0.04f, 0.92f}
                                              : glm::vec4{0.07f, 0.07f, 0.10f, 0.85f});
            glm::vec4 border = sel ? glm::vec4{1.f, 0.6f + pulse * 0.2f, 0.1f, 0.9f} : glm::vec4{0.3f, 0.3f, 0.35f, 0.6f};
            ui.rect(cx - 200, y, 400, 2, border);
            ui.rect(cx - 200, y + 52, 400, 2, border);
            if (sel) ui.rect(cx - 200, y, 5, 54, {1.f, 0.6f + pulse * 0.2f, 0.1f, 1.f});
            glm::vec4 c = sel ? glm::vec4{1.f, 0.65f, 0.12f, 1.f} : glm::vec4{0.65f, 0.65f, 0.7f, 0.95f};
            bool hasSub = items[i].sub[0] != 0;
            ui.text(items[i].label, cx, y + (hasSub ? 9 : 16), 3, c, true);
            if (hasSub) ui.text(items[i].sub, cx, y + 37, 1, {0.75f, 0.72f, 0.7f, 0.9f}, true);
        }
        if (settings) {
            char d[64];
            std::snprintf(d, sizeof(d), "DIFFICULTY   <  %s  >", difficulty(settings->difficulty).name);
            glm::vec4 dc = settings->difficulty >= 2 ? glm::vec4{1.f, 0.35f, 0.25f, 0.95f} : glm::vec4{1.f, 0.8f, 0.45f, 0.95f};
            ui.text(d, cx, itemY(NUM_ITEMS - 1) + 74, 2, dc, true);
            ui.text("LEFT / RIGHT TO CHANGE", cx, itemY(NUM_ITEMS - 1) + 98, 1, {0.6f, 0.6f, 0.65f, 0.8f}, true);
        }
        float hint = 0.35f + 0.35f * std::sin(flashTime * 1.8f);
        ui.text("UP/DOWN OR MOUSE TO SELECT  -  ENTER OR CLICK TO CONFIRM",
                cx, screenH - 30, 1, {hint, hint, hint + 0.05f, 0.85f}, true);
    }
};
