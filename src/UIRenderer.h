#pragma once
#include <SDL2/SDL.h>
#include <OpenGL/gl3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "ShaderProgram.h"
#include "StyleSystem.h"
#include <algorithm>
#include <cmath>

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

    void render(const StyleSystem& style, int activeWeapon,
                int grenadeCount, int grenadeMax,
                int revolverAmmo, int revolverAmmoMax,
                bool reloading, float reloadProgress,
                bool nearInteractable = false) {
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
        // Slot 1: Revolver (infinite ammo)
        // Slot 2: Grenades (count shown as squares)
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
                // Grenade icon — oval body
                glm::vec4 gc = grenadeCount > 0 ? glm::vec4{0.3f,0.9f,0.15f,0.9f}
                                                : glm::vec4{0.4f,0.4f,0.4f,0.5f};
                drawRect(sx+14, slotBaseY+14, 24, 30, gc);  // body
                drawRect(sx+20, slotBaseY+8,  12, 10, {0.6f,0.6f,0.6f,0.8f}); // cap
                // Grenade count dots
                for (int g = 0; g < grenadeMax; ++g) {
                    float da = (g < grenadeCount) ? 0.95f : 0.2f;
                    drawRect(sx+8 + g*18, slotBaseY+slotH-14, 14, 8, {0.3f,0.9f,0.15f,da});
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

        glDisable(GL_BLEND);
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
    }

private:
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
