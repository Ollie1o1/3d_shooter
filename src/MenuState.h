#pragma once
#include "GameState.h"
#include "Settings.h"
#include "PixelFont.h"
#include <SDL2/SDL.h>
#include "gl.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "ShaderProgram.h"
#include <functional>
#include <cmath>
#include <cstring>
#include <cstdio>

// =============================================================================
// MenuState — main menu + settings page
// page 0 = main  (START / SETTINGS / EXIT)
// page 1 = settings (FOV / SENSITIVITY / AUDIO / FPS CAP / SHOW FPS / BACK)
// =============================================================================
class MenuState : public GameState {
public:
    std::function<void()> onStart;
    std::function<void()> onQuit;

    GameSettings* settings = nullptr;   // injected by main

    int   page      = 0;   // 0=main, 1=settings
    int   selected  = 0;
    float flashTime = 0.f;

    ShaderProgram uiShader;
    GLuint quadVAO = 0, quadVBO = 0;
    int screenW, screenH;

    // Settings page items: 0=FOV 1=SENS 2=AUDIO 3=FPS_CAP 4=SHOW_FPS 5=CRT 6=BACK
    static constexpr int NUM_SETTINGS = 7;

    MenuState(int w, int h) : screenW(w), screenH(h) {
        uiShader.loadFiles("src/ui.vert", "src/ui.frag");
        float verts[] = { 0,0, 1,0, 1,1, 0,0, 1,1, 0,1 };
        glGenVertexArrays(1, &quadVAO);
        glGenBuffers(1, &quadVBO);
        glBindVertexArray(quadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2*sizeof(float), (void*)0);
        glBindVertexArray(0);
    }

    ~MenuState() {
        if (quadVAO) glDeleteVertexArrays(1, &quadVAO);
        if (quadVBO) glDeleteBuffers(1, &quadVBO);
    }

    void handleEvent(const SDL_Event& e) override {
        if (page == 0) handleMainEvent(e);
        else           handleSettingsEvent(e);
    }

    void update(float dt) override { flashTime += dt; }

    void render() override {
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glClearColor(0.04f, 0.03f, 0.08f, 1.f);
        glClear(GL_COLOR_BUFFER_BIT);

        uiShader.use();
        glm::mat4 ortho = glm::ortho(0.f, (float)screenW, (float)screenH, 0.f);
        uiShader.setMat4("projection", ortho);
        glBindVertexArray(quadVAO);

        // Background gradient
        for (int i = 0; i < 16; ++i) {
            float t = (float)i / 16.f;
            int   y = (int)(t * screenH);
            int   h2 = screenH / 16 + 1;
            drawRect(0, y, screenW, h2, {0.04f+t*0.03f, 0.02f+t*0.02f, 0.08f+t*0.08f, 1.f});
        }

        if (page == 0) renderMain();
        else           renderSettings();

        glDisable(GL_BLEND);
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
    }

private:
    // =========================================================================
    // MAIN PAGE
    // =========================================================================
    void handleMainEvent(const SDL_Event& e) {
        if (e.type == SDL_KEYDOWN) {
            switch (e.key.keysym.sym) {
                case SDLK_UP:     selected = (selected + 2) % 3; break;
                case SDLK_DOWN:   selected = (selected + 1) % 3; break;
                case SDLK_RETURN:
                case SDLK_SPACE:  activateMain(selected); break;
                case SDLK_ESCAPE: if (onQuit) onQuit(); break;
                default: break;
            }
        }
        if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
            int mx = e.button.x, my = e.button.y;
            for (int i = 0; i < 3; ++i) {
                int by = mainButtonY(i);
                if (mx > screenW/2-120 && mx < screenW/2+120 && my > by && my < by+50)
                    activateMain(i);
            }
        }
        if (e.type == SDL_MOUSEMOTION) {
            int mx = e.motion.x, my = e.motion.y;
            for (int i = 0; i < 3; ++i) {
                int by = mainButtonY(i);
                if (mx > screenW/2-120 && mx < screenW/2+120 && my > by && my < by+50)
                    selected = i;
            }
        }
    }

    void activateMain(int idx) {
        if (idx == 0 && onStart) onStart();
        if (idx == 1) { page = 1; selected = 0; }
        if (idx == 2 && onQuit)  onQuit();
    }

    int mainButtonY(int i) const { return screenH/2 - 80 + i * 70; }

    void renderMain() {
        int titleBottom = screenH/2 - 148;
        drawRect(screenW/2-260, titleBottom,   520, 3, {1.f, 0.55f, 0.05f, 0.9f});
        drawRect(screenW/2-260, titleBottom+6, 520, 1, {1.f, 0.55f, 0.05f, 0.4f});
        drawText("OVERDRIVE", screenW/2, screenH/2 - 210, 5, {1.f, 0.55f, 0.08f, 1.f}, true);

        const char* labels[3] = { "START", "SETTINGS", "EXIT" };
        glm::vec4 selColor  = {0.9f, 0.6f, 0.1f, 1.f};
        glm::vec4 normColor = {0.55f, 0.55f, 0.6f, 0.9f};

        for (int i = 0; i < 3; ++i) {
            bool  isSel = (selected == i);
            int   by    = mainButtonY(i);
            float pulse = isSel ? 0.5f + 0.5f*sinf(flashTime*5.f) : 0.f;

            glm::vec4 bg = isSel
                ? glm::vec4(0.12f+pulse*0.08f, 0.08f+pulse*0.04f, 0.04f, 0.92f)
                : glm::vec4(0.08f, 0.08f, 0.10f, 0.85f);
            drawRect(screenW/2-120, by, 240, 50, bg);
            if (isSel)
                drawRect(screenW/2-120, by, 5, 50, {1.f, 0.6f+pulse*0.2f, 0.1f, 1.f});
            glm::vec4 borderC = isSel
                ? glm::vec4(1.f, 0.6f+pulse*0.2f, 0.1f, 0.9f)
                : glm::vec4(0.3f, 0.3f, 0.35f, 0.6f);
            drawRect(screenW/2-120, by,    240, 2, borderC);
            drawRect(screenW/2-120, by+48, 240, 2, borderC);
            drawText(labels[i], screenW/2, by+15, 3, isSel ? selColor : normColor, true);
        }

        int   ay = mainButtonY(selected) + 18;
        float ap = 0.5f + 0.5f*sinf(flashTime*5.f);
        drawText(">", screenW/2 - 140, ay, 3, {1.f, 0.6f+ap*0.2f, 0.1f, ap*0.8f+0.2f}, false);

        float hint = 0.35f + 0.35f*sinf(flashTime*1.8f);
        drawText("UP/DOWN or mouse to select  |  ENTER or click to confirm",
                 screenW/2, screenH - 30, 1, {hint, hint, hint+0.05f, 0.8f}, true);
    }

    // =========================================================================
    // SETTINGS PAGE
    // Items: 0=FOV  1=SENS  2=AUDIO  3=FPS_CAP  4=SHOW_FPS  5=BACK
    // LEFT/RIGHT arrows change value; UP/DOWN navigate; ENTER on BACK returns
    // =========================================================================
    void handleSettingsEvent(const SDL_Event& e) {
        if (e.type == SDL_KEYDOWN) {
            switch (e.key.keysym.sym) {
                case SDLK_UP:
                    selected = (selected + NUM_SETTINGS - 1) % NUM_SETTINGS; break;
                case SDLK_DOWN:
                    selected = (selected + 1) % NUM_SETTINGS; break;
                case SDLK_LEFT:  adjustSetting(selected, -1); break;
                case SDLK_RIGHT: adjustSetting(selected,  1); break;
                case SDLK_RETURN:
                case SDLK_SPACE:
                    if (selected == 6) { page = 0; selected = 1; }
                    else adjustSetting(selected, 1);
                    break;
                case SDLK_ESCAPE:
                    page = 0; selected = 1; break;
                default: break;
            }
        }
    }

    void adjustSetting(int item, int dir) {
        if (!settings) return;
        switch (item) {
            case 0: // FOV 60..120 step 5
                settings->fov = glm::clamp(settings->fov + dir * 5.f, 60.f, 120.f);
                break;
            case 1: // Sensitivity 0.02..0.50 step 0.02
                settings->sensitivity = glm::clamp(settings->sensitivity + dir * 0.02f, 0.02f, 0.50f);
                break;
            case 2: // Audio 0..1 step 0.1
                settings->audioVolume = glm::clamp(settings->audioVolume + dir * 0.1f, 0.f, 1.f);
                break;
            case 3: // FPS cap cycle
                settings->fpsCap = (settings->fpsCap + dir + 5) % 5;
                break;
            case 4: // Show FPS toggle
                settings->showFPS = !settings->showFPS;
                break;
            case 5: // CRT filter toggle
                settings->crtFilter = !settings->crtFilter;
                break;
            default: break;
        }
    }

    void renderSettings() {
        // Title
        drawRect(screenW/2-200, screenH/2 - 190, 400, 3, {0.5f, 0.8f, 1.f, 0.8f});
        drawText("SETTINGS", screenW/2, screenH/2 - 220, 4, {0.5f, 0.85f, 1.f, 1.f}, true);

        struct Item { const char* label; char value[32]; };
        Item items[NUM_SETTINGS];

        if (settings) {
            snprintf(items[0].value, 32, "%d", (int)settings->fov);
            snprintf(items[1].value, 32, "%.2f", settings->sensitivity);
            snprintf(items[2].value, 32, "%.0f%%", settings->audioVolume * 100.f);
            snprintf(items[3].value, 32, "%s", settings->getFPSCapLabel());
            snprintf(items[4].value, 32, "%s", settings->showFPS ? "ON" : "OFF");
            snprintf(items[5].value, 32, "%s", settings->crtFilter ? "ON" : "OFF");
        } else {
            for (int i = 0; i < NUM_SETTINGS; ++i) strcpy(items[i].value, "-");
        }
        items[0].label = "FOV";
        items[1].label = "SENSITIVITY";
        items[2].label = "AUDIO";
        items[3].label = "FPS CAP";
        items[4].label = "SHOW FPS";
        items[5].label = "CRT FILTER";
        items[6].label = "BACK";
        strcpy(items[6].value, "");

        int baseY = screenH/2 - 155;
        for (int i = 0; i < NUM_SETTINGS; ++i) {
            bool   isSel = (selected == i);
            int    iy    = baseY + i * 58;
            float  pulse = isSel ? 0.5f + 0.5f*sinf(flashTime*5.f) : 0.f;
            glm::vec4 bg = isSel
                ? glm::vec4(0.12f+pulse*0.08f, 0.08f+pulse*0.04f, 0.04f, 0.92f)
                : glm::vec4(0.08f, 0.08f, 0.10f, 0.85f);
            drawRect(screenW/2-220, iy, 440, 48, bg);
            if (isSel)
                drawRect(screenW/2-220, iy, 4, 48, {1.f, 0.6f+pulse*0.2f, 0.1f, 1.f});
            glm::vec4 borderC = isSel
                ? glm::vec4(1.f, 0.6f+pulse*0.2f, 0.1f, 0.7f)
                : glm::vec4(0.3f, 0.3f, 0.35f, 0.4f);
            drawRect(screenW/2-220, iy,    440, 2, borderC);
            drawRect(screenW/2-220, iy+46, 440, 2, borderC);

            glm::vec4 labelC = isSel ? glm::vec4{0.9f,0.6f,0.1f,1.f} : glm::vec4{0.6f,0.6f,0.65f,0.9f};
            drawText(items[i].label, screenW/2 - 200, iy + 16, 2, labelC, false);

            if (i < 6) {
                // arrows + value
                if (isSel) {
                    drawText("<", screenW/2 + 60, iy + 16, 2, {1.f,0.6f,0.1f,0.9f}, false);
                    drawText(">", screenW/2 + 160, iy + 16, 2, {1.f,0.6f,0.1f,0.9f}, false);
                }
                glm::vec4 valC = isSel ? glm::vec4{1.f,0.9f,0.4f,1.f} : glm::vec4{0.7f,0.7f,0.7f,0.8f};
                drawText(items[i].value, screenW/2 + 110, iy + 16, 2, valC, true);
            }
        }

        drawText("LEFT/RIGHT to change  |  ESC or BACK to return",
                 screenW/2, screenH - 30, 1, {0.4f,0.4f,0.45f,0.7f}, true);
    }

    // =========================================================================
    // Shared drawing helpers
    // =========================================================================
    void drawText(const char* text, int px, int py, int scale, glm::vec4 color, bool centred) {
        int len   = (int)strlen(text);
        int charW = 5 * scale + scale;
        int totalW = len * charW - scale;
        int startX = centred ? px - totalW / 2 : px;
        int cx = startX;
        for (int ci = 0; ci < len; ++ci) {
            char ch = text[ci];
            if ((unsigned char)ch < 32 || (unsigned char)ch > 127) { cx += charW; continue; }
            const uint8_t* glyph = PIXEL_FONT[(unsigned char)(ch - ' ')];
            for (int row = 0; row < 7; ++row) {
                uint8_t bits = glyph[row];
                for (int col = 0; col < 5; ++col) {
                    if (bits & (0x10 >> col))
                        drawRect(cx + col*scale, py + row*scale, scale, scale, color);
                }
            }
            cx += charW;
        }
    }

    void drawRect(int x, int y, int w, int h, glm::vec4 c) {
        glm::mat4 m = glm::translate(glm::mat4(1.f), {(float)x, (float)y, 0.f});
        m = glm::scale(m, {(float)w, (float)h, 1.f});
        uiShader.setMat4("model", m);
        uiShader.setVec3("uColor", {c.r, c.g, c.b});
        uiShader.setFloat("uAlpha", c.a);
        glDrawArrays(GL_TRIANGLES, 0, 6);
    }
};
