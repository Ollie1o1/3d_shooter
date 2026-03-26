#pragma once
#include <SDL2/SDL.h>
#include "gl.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "ShaderProgram.h"
#include "StyleSystem.h"
#include "PixelFont.h"
#include <algorithm>
#include <cmath>
#include <cstring>

class UIRenderer {
public:
    ShaderProgram shader;
    GLuint quadVAO = 0, quadVBO = 0;
    int screenW, screenH;

    float displayHealth = 100.f;
    float displayStyle  = 0.f;

    float overdriveFlash  = 0.f;
    float damageVignette  = 0.f;
    float hitmarkerTimer  = 0.f;
    float shootFlashTimer = 0.f;
    bool  hitmarkerKill   = false;

    int  currentFPS = 0;
    bool showFPS    = false;

    UIRenderer(int w, int h) : screenW(w), screenH(h) {
        shader.loadFiles("src/ui.vert", "src/ui.frag");
        float verts[] = { 0,0, 1,0, 1,1, 0,0, 1,1, 0,1 };
        glGenVertexArrays(1,&quadVAO);
        glGenBuffers(1,&quadVBO);
        glBindVertexArray(quadVAO);
        glBindBuffer(GL_ARRAY_BUFFER,quadVBO);
        glBufferData(GL_ARRAY_BUFFER,sizeof(verts),verts,GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0,2,GL_FLOAT,GL_FALSE,2*sizeof(float),(void*)0);
        glBindVertexArray(0);
    }

    ~UIRenderer() {
        if (quadVAO) glDeleteVertexArrays(1,&quadVAO);
        if (quadVBO) glDeleteBuffers(1,&quadVBO);
    }

    void onDamage()              { damageVignette = 0.5f; }
    void onOverdrive()           { overdriveFlash = 2.0f; }
    void onShoot()               { shootFlashTimer = 0.06f; }
    void onHit(bool kill=false)  { hitmarkerTimer = 0.18f; hitmarkerKill = kill; }

    void update(float dt, const StyleSystem& style) {
        displayHealth += (style.health - displayHealth) * std::min(1.f, dt * 8.f);
        displayStyle  += (style.style  - displayStyle ) * std::min(1.f, dt * 5.f);
        if (damageVignette  > 0.f) damageVignette  -= dt;
        if (overdriveFlash  > 0.f) overdriveFlash  -= dt;
        if (hitmarkerTimer  > 0.f) hitmarkerTimer  -= dt;
        if (shootFlashTimer > 0.f) shootFlashTimer -= dt;
    }

    void onGrenadeRefill() { /* visual flash could go here */ }

    // Damage direction indicators
    struct DamageIndicator {
        float angle;
        float timer;
    };
    DamageIndicator damageIndicators[8];
    int damageIndicatorCount = 0;

    void onDamageFrom(float angle) {
        if (damageIndicatorCount < 8) {
            damageIndicators[damageIndicatorCount++] = {angle, 1.0f};
        } else {
            // Replace oldest
            damageIndicators[0] = {angle, 1.0f};
        }
    }

    // Floating damage numbers
    struct FloatingNumber {
        float x, y;     // screen position
        float value;
        float timer;
        bool  critical;
    };
    FloatingNumber floatingNums[16];
    int floatingNumCount = 0;

    void spawnFloatingNumber(float screenX, float screenY, float value, bool crit = false) {
        int slot = floatingNumCount < 16 ? floatingNumCount++ : 0;
        floatingNums[slot] = {screenX, screenY, value, 0.8f, crit};
    }

    void render(const StyleSystem& style, int activeWeapon,
                int shotgunAmmo, int shotgunAmmoMax,
                bool shotgunReloading, float shotgunReloadProgress,
                int revolverAmmo, int revolverAmmoMax,
                bool reloading, float reloadProgress,
                bool nearInteractable = false,
                bool showWin = false, bool showDeath = false,
                int kills = 0, int shots = 0, int hits = 0,
                float gameTime = 0.f, float peakStyle = 0.f,
                int waveNum = 0, float waveBanner = 0.f) {
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        shader.use();
        glm::mat4 ortho = glm::ortho(0.f,(float)screenW,(float)screenH,0.f);
        shader.setMat4("projection", ortho);
        glBindVertexArray(quadVAO);

        if (damageVignette > 0.f) {
            float alpha = damageVignette * 0.6f;
            drawRect(0, 0, screenW, screenH, {0.5f,0.f,0.f,alpha});
        }

        int bx=20, by=screenH-50, bw=200, bh=18;
        drawRect(bx-2,by-2,bw+4,bh+4,{0.1f,0.1f,0.1f,0.9f});
        drawRect(bx,by,bw,bh,{0.25f,0.05f,0.05f,0.8f});
        float hFill = displayHealth / style.maxHealth;
        drawRect(bx,by,(int)(bw*hFill),bh,{0.9f,0.15f,0.15f,0.95f});

        StyleRank rank = style.getRank();
        glm::vec4 rankColor = getRankColor(rank, style);
        int sx=(screenW-200)/2, sy=screenH-50;
        drawRect(sx-2,sy-2,204,22,{0.1f,0.1f,0.1f,0.9f});
        drawRect(sx,sy,200,18,{0.1f,0.1f,0.15f,0.8f});
        float sFill = displayStyle / style.maxStyle;
        drawRect(sx,sy,(int)(200*sFill),18,rankColor);

        // ---- Weapon slots (bottom-right) ----
        // Slot 1: Revolver  Slot 2: Shotgun   G key: Grenade (not shown as a slot)
        int slotW = 56, slotH = 64, slotPad = 8;
        int slotBaseX = screenW - 2*(slotW+slotPad) - 12;
        int slotBaseY = screenH - slotH - 12;

        for (int s = 0; s < 2; ++s) {
            int sx = slotBaseX + s*(slotW+slotPad);
            bool active = (s == activeWeapon);
            // Slot background
            glm::vec4 bgCol = active ? glm::vec4{0.25f,0.25f,0.3f,0.95f}
                                     : glm::vec4{0.1f,0.1f,0.12f,0.8f};
            drawRect(sx-2, slotBaseY-2, slotW+4, slotH+4,
                     active ? glm::vec4{0.85f,0.7f,0.2f,0.9f}  // gold border
                            : glm::vec4{0.3f,0.3f,0.35f,0.5f});
            drawRect(sx, slotBaseY, slotW, slotH, bgCol);

            // Slot number pip
            drawRect(sx+3, slotBaseY+3, 8, 8,
                     active ? glm::vec4{1,0.85f,0.2f,1} : glm::vec4{0.5f,0.5f,0.5f,0.7f});

            if (s == 0) {
                // Revolver icon — barrel + grip bars
                glm::vec4 ic = {0.9f,0.85f,0.6f,0.9f};
                drawRect(sx+8, slotBaseY+12, 36, 5,  ic);  // barrel
                drawRect(sx+8, slotBaseY+20, 22, 9,  ic);  // grip

                // Ammo pips — 2 columns of 4 bullets (8 total)
                // Left col: rounds 1-4, right col: rounds 5-8
                for (int pip = 0; pip < revolverAmmoMax; ++pip) {
                    int col   = pip / 4;
                    int row   = pip % 4;
                    int px    = sx + 7 + col * 12;
                    int py    = slotBaseY + 33 + row * 7;
                    bool live = pip < revolverAmmo;
                    glm::vec4 pc = live ? glm::vec4{0.95f,0.85f,0.30f,0.9f}
                                        : glm::vec4{0.30f,0.28f,0.20f,0.5f};
                    drawRect(px, py, 8, 5, pc);
                }

                // Reload wheel overlay — shown while reloading
                if (reloading) {
                    int   dots   = 12;
                    float cx_f   = sx + slotW * 0.5f;
                    float cy_f   = slotBaseY + slotH * 0.5f;
                    float radius = 20.f;
                    // dim background ring
                    for (int d = 0; d < dots; ++d) {
                        float angle = (float)d / (float)dots * 6.2832f - 1.5708f;
                        float dx    = cosf(angle) * radius;
                        float dy    = sinf(angle) * radius;
                        drawRect((int)(cx_f+dx-3),(int)(cy_f+dy-3),6,6,{0.15f,0.15f,0.15f,0.7f});
                    }
                    // filled progress arc
                    for (int d = 0; d < dots; ++d) {
                        float angle = (float)d / (float)dots * 6.2832f - 1.5708f;
                        float dx    = cosf(angle) * radius;
                        float dy    = sinf(angle) * radius;
                        float lit   = ((float)d / (float)dots < reloadProgress) ? 0.95f : 0.0f;
                        if (lit > 0.f)
                            drawRect((int)(cx_f+dx-3),(int)(cy_f+dy-3),6,6,
                                     {0.95f,0.80f,0.30f,lit});
                    }
                }
            } else {
                // Shotgun icon — wide receiver body + barrel
                glm::vec4 ic = shotgunAmmo > 0 ? glm::vec4{0.75f,0.6f,0.45f,0.9f}
                                               : glm::vec4{0.4f,0.35f,0.3f,0.5f};
                drawRect(sx+6,  slotBaseY+12, 40, 6,  ic);  // barrel (long)
                drawRect(sx+6,  slotBaseY+20, 28, 8,  ic);  // receiver body
                drawRect(sx+10, slotBaseY+28, 16, 12, ic);  // grip

                // Shell pips — one large rect per shell
                for (int sh = 0; sh < shotgunAmmoMax; ++sh) {
                    bool live = sh < shotgunAmmo;
                    glm::vec4 sc = live ? glm::vec4{0.95f,0.75f,0.20f,0.95f}
                                       : glm::vec4{0.30f,0.26f,0.18f,0.4f};
                    drawRect(sx + 8 + sh * 20, slotBaseY + slotH - 16, 16, 10, sc);
                }

                // Reload arc overlay
                if (shotgunReloading) {
                    int   dots   = 12;
                    float cx_f   = sx + slotW * 0.5f;
                    float cy_f   = slotBaseY + slotH * 0.5f;
                    float radius = 20.f;
                    for (int d = 0; d < dots; ++d) {
                        float angle = (float)d / (float)dots * 6.2832f - 1.5708f;
                        float dx    = cosf(angle) * radius;
                        float dy    = sinf(angle) * radius;
                        drawRect((int)(cx_f+dx-3),(int)(cy_f+dy-3),6,6,{0.15f,0.15f,0.15f,0.7f});
                    }
                    for (int d = 0; d < dots; ++d) {
                        float angle = (float)d / (float)dots * 6.2832f - 1.5708f;
                        float dx    = cosf(angle) * radius;
                        float dy    = sinf(angle) * radius;
                        float lit   = ((float)d / (float)dots < shotgunReloadProgress) ? 0.95f : 0.0f;
                        if (lit > 0.f)
                            drawRect((int)(cx_f+dx-3),(int)(cy_f+dy-3),6,6,
                                     {0.95f,0.70f,0.20f,lit});
                    }
                }
            }
        }

        if (overdriveFlash > 0.f || style.overdrive) {
            float alpha = style.overdrive ? 0.9f : (overdriveFlash * 0.6f);
            drawRect(screenW/2-120, 40, 240, 36, {0.9f,0.5f,0.05f,alpha*0.3f});
            drawRect(screenW/2-120, 40, 240, 4, {1.f,0.7f,0.1f,alpha});
            drawRect(screenW/2-120, 72, 240, 4, {1.f,0.7f,0.1f,alpha});
        }

        int cx=screenW/2, cy=screenH/2;

        // Shoot flash — brief bright dot at crosshair on fire
        if (shootFlashTimer > 0.f) {
            float a = (shootFlashTimer / 0.06f) * 0.75f;
            drawRect(cx-6, cy-6, 12, 12, {1.f, 0.9f, 0.5f, a});
        }

        // Hitmarker — big orange/white X when a shot connects
        if (hitmarkerTimer > 0.f) {
            float a = (hitmarkerTimer / 0.18f);
            glm::vec4 hc = hitmarkerKill ? glm::vec4{1.f,0.35f,0.f,a} : glm::vec4{1.f,1.f,1.f,a};
            // diagonal / top-left to bottom-right
            drawRect(cx-14, cy-14, 8, 4, hc); drawRect(cx+6,  cy+10, 8, 4, hc);
            drawRect(cx+6,  cy-14, 8, 4, hc); drawRect(cx-14, cy+10, 8, 4, hc);
        }

        // Crosshair — standard + shape; turns gold when near an interactable
        glm::vec4 chCol = nearInteractable
            ? glm::vec4{1.0f, 0.85f, 0.15f, 0.95f}
            : glm::vec4{1.0f, 1.0f,  1.0f,  0.85f};
        // Extra dot in center when looking at interactable (interaction prompt)
        if (nearInteractable)
            drawRect(cx-3, cy-3, 6, 6, {1.0f, 0.85f, 0.15f, 0.5f});
        drawRect(cx-8,cy-1,6,2,chCol);
        drawRect(cx+2,cy-1,6,2,chCol);
        drawRect(cx-1,cy-8,2,6,chCol);
        drawRect(cx-1,cy+2,2,6,chCol);

        // --- Damage direction indicators ---
        for (int i = 0; i < damageIndicatorCount; ++i) {
            auto& di = damageIndicators[i];
            if (di.timer <= 0.f) continue;
            di.timer -= 1.f / 60.f;  // approximate dt
            float alpha = di.timer * 0.7f;
            float a = di.angle;
            // Normalize angle
            while (a >  3.14159f) a -= 6.28318f;
            while (a < -3.14159f) a += 6.28318f;
            // Position on screen edge
            float edgeDist = 80.f;
            float ix = cx + sinf(a) * edgeDist;
            float iy = cy - cosf(a) * edgeDist;
            drawRect((int)ix - 6, (int)iy - 6, 12, 12, {0.9f, 0.1f, 0.1f, alpha});
            drawRect((int)ix - 4, (int)iy - 4, 8, 8, {1.0f, 0.2f, 0.1f, alpha * 0.7f});
        }
        // Compact expired indicators
        int w = 0;
        for (int r = 0; r < damageIndicatorCount; ++r)
            if (damageIndicators[r].timer > 0.f) damageIndicators[w++] = damageIndicators[r];
        damageIndicatorCount = w;

        // --- Floating damage numbers ---
        for (int i = 0; i < floatingNumCount; ++i) {
            auto& fn = floatingNums[i];
            if (fn.timer <= 0.f) continue;
            fn.timer -= 1.f / 60.f;
            fn.y -= 40.f / 60.f;  // float upward
            float alpha = fn.timer / 0.8f;
            char numBuf[16];
            snprintf(numBuf, sizeof(numBuf), "%d", (int)fn.value);
            glm::vec4 col = fn.critical ? glm::vec4{1.f, 0.85f, 0.2f, alpha}
                                        : glm::vec4{1.f, 1.f, 1.f, alpha};
            drawText(numBuf, (int)fn.x, (int)fn.y, 2, col, true);
        }
        w = 0;
        for (int r = 0; r < floatingNumCount; ++r)
            if (floatingNums[r].timer > 0.f) floatingNums[w++] = floatingNums[r];
        floatingNumCount = w;

        // --- Wave banner ---
        if (waveBanner > 0.f) {
            float alpha = std::min(waveBanner, 1.f);
            char waveBuf[32];
            snprintf(waveBuf, sizeof(waveBuf), "WAVE %d", waveNum);
            drawRect(screenW/2 - 120, screenH/2 - 40, 240, 50, {0.1f, 0.1f, 0.15f, alpha * 0.7f});
            drawText(waveBuf, screenW/2, screenH/2 - 20, 3, {1.f, 0.9f, 0.3f, alpha}, true);
        }

        // --- Win screen ---
        if (showWin) {
            drawRect(0, 0, screenW, screenH, {0.0f, 0.02f, 0.05f, 0.65f});
            drawText("ARENA CLEARED", screenW/2, screenH/2 - 100, 4, {0.2f, 1.f, 0.4f, 0.95f}, true);

            char buf[64];
            int statY = screenH/2 - 30;
            snprintf(buf, sizeof(buf), "TIME  %d:%02d", (int)gameTime / 60, (int)gameTime % 60);
            drawText(buf, screenW/2, statY, 2, {0.9f,0.9f,0.9f,0.9f}, true);
            snprintf(buf, sizeof(buf), "KILLS  %d", kills);
            drawText(buf, screenW/2, statY + 24, 2, {0.9f,0.9f,0.9f,0.9f}, true);
            float acc = shots > 0 ? (float)hits / (float)shots * 100.f : 0.f;
            snprintf(buf, sizeof(buf), "ACCURACY  %d%%", (int)acc);
            drawText(buf, screenW/2, statY + 48, 2, {0.9f,0.9f,0.9f,0.9f}, true);

            const char* gradeStr = "D";
            float score = (float)kills * 10.f + acc * 2.f + peakStyle + std::max(0.f, 300.f - gameTime);
            if (score > 800.f) gradeStr = "S";
            else if (score > 600.f) gradeStr = "A";
            else if (score > 400.f) gradeStr = "B";
            else if (score > 200.f) gradeStr = "C";
            drawText(gradeStr, screenW/2, statY + 80, 5, {1.f, 0.85f, 0.2f, 0.95f}, true);

            drawText("R - RESTART    ESC - MENU", screenW/2, statY + 130, 2, {0.6f,0.6f,0.6f,0.8f}, true);
        }

        // --- Death screen ---
        if (showDeath) {
            float vigAlpha = 0.5f;
            drawRect(0, 0, screenW, screenH, {0.3f, 0.0f, 0.0f, vigAlpha});
            drawText("YOU DIED", screenW/2, screenH/2 - 80, 4, {0.9f, 0.15f, 0.1f, 0.95f}, true);

            char buf[64];
            int statY = screenH/2 - 10;
            snprintf(buf, sizeof(buf), "TIME  %d:%02d", (int)gameTime / 60, (int)gameTime % 60);
            drawText(buf, screenW/2, statY, 2, {0.8f,0.7f,0.7f,0.85f}, true);
            snprintf(buf, sizeof(buf), "KILLS  %d", kills);
            drawText(buf, screenW/2, statY + 24, 2, {0.8f,0.7f,0.7f,0.85f}, true);

            drawText("R - RESTART    ESC - MENU", screenW/2, statY + 70, 2, {0.6f,0.5f,0.5f,0.8f}, true);
        }

        // --- FPS counter (bottom of screen) ----------------------------------
        if (showFPS) {
            char buf[16];
            snprintf(buf, sizeof(buf), "FPS %d", currentFPS);
            drawText(buf, screenW/2, screenH - 18, 2, {0.9f,0.9f,0.2f,0.85f}, true);
        }

        glDisable(GL_BLEND);
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
    }

private:
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
                    if (bits & (0x10 >> col)) {
                        int rx = cx + col * scale;
                        int ry = py + row * scale;
                        drawRect(rx, ry, scale, scale, color);
                    }
                }
            }
            cx += charW;
        }
    }
    void drawRect(int x, int y, int w, int h, glm::vec4 color) {
        glm::mat4 model = glm::translate(glm::mat4(1.f), glm::vec3((float)x,(float)y,0.f));
        model = glm::scale(model, glm::vec3((float)w,(float)h,1.f));
        shader.setMat4("model", model);
        shader.setVec3("uColor", {color.r,color.g,color.b});
        shader.setFloat("uAlpha", color.a);
        glDrawArrays(GL_TRIANGLES, 0, 6);
    }

    glm::vec4 getRankColor(StyleRank r, const StyleSystem& s) {
        (void)s;
        switch (r) {
            case StyleRank::D:   return {0.4f,0.4f,0.4f,0.85f};
            case StyleRank::C:   return {0.2f,0.3f,0.9f,0.85f};
            case StyleRank::B:   return {0.1f,0.8f,0.2f,0.85f};
            case StyleRank::A:   return {0.9f,0.85f,0.1f,0.85f};
            case StyleRank::S:   return {0.95f,0.5f,0.05f,0.85f};
            case StyleRank::SSS: {
                float pulse = 0.5f+0.5f*sinf((float)SDL_GetTicks()*0.006f);
                return {0.9f,0.05f+pulse*0.3f,0.05f,0.95f};
            }
        }
        return {1,1,1,1};
    }
};
