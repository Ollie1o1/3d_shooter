#pragma once
// =============================================================================
// UIRenderer.h — the HUD and every in-game overlay (pause, armory, death and
// victory screens). All 2D drawing goes through UIBatch, so the whole HUD is
// a handful of draw calls; the sniper scope's lens uses its own small shader.
// =============================================================================
#include <SDL2/SDL.h>
#include "gl.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "ShaderProgram.h"
#include "StyleSystem.h"
#include "UIBatch.h"
#include "Progression.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <cstdio>
#include <string>
#include <deque>
#include <vector>

struct HudWeapon {
    const char* name = "";
    int   ammo = 0, mag = 0;
    bool  reloading = false;
    float reload01 = 1.f;
};

// Everything the HUD shows, filled in by GameplayState each frame
struct HudState {
    float health = 100.f, maxHealth = 100.f;
    int   activeWeapon = 0;
    HudWeapon weapons[WEAPON_COUNT];
    int   grenades = 0, grenadeMax = 2;
    int   level = 1, xp = 0, xpNext = 100, points = 0;
    float aim = 0.f;               // 0 hip .. 1 aimed
    bool  scoped = false;          // Longshot scope view
    glm::vec2 scopeSway{0.f};      // pixels
    float spread = 0.f;            // crosshair gap in pixels from weapon spread
    bool  hideCrosshair = false;
    bool  grappleTarget = false;   // aiming at something the grapple can hook
    bool  showTimer = false;
    float time = 0.f;
    float speed = -1.f;       // FAST: horizontal speed in m/s (< 0: hidden)
    glm::vec3 crosshairColor{1.f};
    glm::mat4 viewProj{1.f};       // for floating damage numbers
    bool  damageNumbers = true;
};

class UIRenderer {
public:
    UIBatch ui;
    int screenW, screenH;

    float displayHealth = 100.f;
    float displayStyle  = 0.f;

    float overdriveFlash  = 0.f;
    float damageVignette  = 0.f;
    float hitmarkerTimer  = 0.f;
    float shootFlashTimer = 0.f;
    bool  hitmarkerKill   = false;
    float hudTime         = 0.f;

    int  currentFPS = 0;
    bool showFPS    = false;

    UIRenderer(int w, int h) : ui(w, h), screenW(w), screenH(h) {
        scopeShader.loadFiles("src/postprocess.vert", "src/scope.frag");
        float verts[] = { -1,-1,0,0,  1,-1,1,0,  1,1,1,1,  -1,-1,0,0,  1,1,1,1,  -1,1,0,1 };
        glGenVertexArrays(1, &quadVAO);
        glGenBuffers(1, &quadVBO);
        glBindVertexArray(quadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
        glBindVertexArray(0);
    }
    ~UIRenderer() {
        if (quadVAO) glDeleteVertexArrays(1, &quadVAO);
        if (quadVBO) glDeleteBuffers(1, &quadVBO);
    }

    void onDamage()              { damageVignette = 0.5f; }
    void onOverdrive()           { overdriveFlash = 2.0f; }
    void onShoot()               { shootFlashTimer = 0.06f; }
    void onHit(bool kill=false)  { hitmarkerTimer = 0.18f; hitmarkerKill = kill; }
    void onParry()               { parryFlash = 0.15f; }
    float parryFlash = 0.f;
    void onGrenadeRefill()       { feed("+1 GRENADE", {0.5f, 1.f, 0.3f}); }

    // ---- damage direction indicators ----
    struct DamageIndicator { float angle, timer; };
    DamageIndicator damageIndicators[8];
    int damageIndicatorCount = 0;
    void onDamageFrom(float angle) {
        if (damageIndicatorCount < 8) damageIndicators[damageIndicatorCount++] = {angle, 1.0f};
        else damageIndicators[0] = {angle, 1.0f};
    }
    void clearIndicators() { damageIndicatorCount = 0; floating.clear(); feedLines.clear(); toasts.clear(); }

    // ---- floating damage numbers (world space, re-projected every frame) ----
    struct FloatingNumber { glm::vec3 pos; float value, timer; bool crit; };
    std::deque<FloatingNumber> floating;
    void spawnDamageNumber(glm::vec3 worldPos, float value, bool crit) {
        // Merge rapid hits on the same spot (shotgun pellets) into one number
        for (auto& f : floating)
            if (f.timer > 0.6f && glm::length(f.pos - worldPos) < 1.2f) { f.value += value; f.crit |= crit; f.timer = 0.8f; return; }
        floating.push_back({worldPos, value, 0.8f, crit});
        if (floating.size() > 24) floating.pop_front();
    }

    // ---- feed (right side): kills, pickups, bonuses ----
    struct FeedLine { std::string text; glm::vec3 color; float timer; };
    std::deque<FeedLine> feedLines;
    void feed(const std::string& text, glm::vec3 color) {
        feedLines.push_back({text, color, 2.8f});
        if (feedLines.size() > 6) feedLines.pop_front();
    }

    // ---- toasts (centre, under the crosshair): level up, quickscope ----
    struct Toast { std::string title, sub; glm::vec3 color; float timer, dur; };
    std::deque<Toast> toasts;
    void toast(const std::string& title, const std::string& sub, glm::vec3 color, float dur = 2.2f) {
        if (!toasts.empty() && toasts.back().title == title) { toasts.back().timer = 0.f; toasts.back().sub = sub; return; }
        toasts.push_back({title, sub, color, 0.f, dur});
        if (toasts.size() > 3) toasts.pop_front();
    }

    // ---- split (under the timer) ----
    std::string splitText;
    glm::vec3   splitColor{1.f};
    float       splitTimer = 0.f;
    void showSplit(const std::string& t, glm::vec3 c) { splitText = t; splitColor = c; splitTimer = 4.f; }

    void update(float dt, const StyleSystem& style) {
        hudTime += dt;
        displayHealth += (style.health - displayHealth) * std::min(1.f, dt * 8.f);
        displayStyle  += (style.style  - displayStyle ) * std::min(1.f, dt * 5.f);
        if (damageVignette  > 0.f) damageVignette  -= dt;
        if (overdriveFlash  > 0.f) overdriveFlash  -= dt;
        if (hitmarkerTimer  > 0.f) hitmarkerTimer  -= dt;
        if (shootFlashTimer > 0.f) shootFlashTimer -= dt;
        if (parryFlash      > 0.f) parryFlash      -= dt;
        if (splitTimer      > 0.f) splitTimer      -= dt;
        for (int i = 0; i < damageIndicatorCount; ++i) damageIndicators[i].timer -= dt;
        int w = 0;
        for (int r = 0; r < damageIndicatorCount; ++r)
            if (damageIndicators[r].timer > 0.f) damageIndicators[w++] = damageIndicators[r];
        damageIndicatorCount = w;
        for (auto& f : floating) { f.timer -= dt; f.pos.y += dt * 1.2f; }
        while (!floating.empty() && floating.front().timer <= 0.f) floating.pop_front();
        for (auto& l : feedLines) l.timer -= dt;
        while (!feedLines.empty() && feedLines.front().timer <= 0.f) feedLines.pop_front();
        if (!toasts.empty()) { toasts.front().timer += dt; if (toasts.front().timer >= toasts.front().dur) toasts.pop_front(); }
    }

    // =========================================================================
    // The HUD proper
    // =========================================================================
    void render(const StyleSystem& style, const HudState& h) {
        if (h.scoped) drawScopeLens(h);
        ui.begin();
        int cx = screenW / 2, cy = screenH / 2;

        drawVignettes(style, h);
        if (parryFlash > 0.f)   // a bright gold flash the instant a parry lands
            ui.rect(0, 0, screenW, screenH, {1.f, 0.9f, 0.6f, parryFlash / 0.15f * 0.35f});
        if (h.scoped) drawReticle(h);

        drawHealthPanel(style, h);
        drawStyleMeter(style);
        drawWeaponSlots(h);
        drawCrosshair(h, cx, cy);

        // Damage direction indicators
        for (int i = 0; i < damageIndicatorCount; ++i) {
            auto& di = damageIndicators[i];
            float alpha = std::min(1.f, di.timer) * 0.75f;
            float a = di.angle;
            for (int k = 0; k < 5; ++k) {   // a short arc of blocks pointing at the source
                float ak = a + (k - 2) * 0.09f;
                float r = 92.f + (k == 2 ? 6.f : 0.f);
                float ix = cx + std::sin(ak) * r, iy = cy - std::cos(ak) * r;
                ui.rect(ix - 4, iy - 4, 8, 8, {1.f, 0.15f, 0.1f, alpha * (k == 2 ? 1.f : 0.7f)});
            }
        }

        // Floating damage numbers
        if (h.damageNumbers) {
            for (auto& f : floating) {
                glm::vec4 c = h.viewProj * glm::vec4(f.pos, 1.f);
                if (c.w <= 0.1f) continue;
                float sx = (c.x / c.w * 0.5f + 0.5f) * screenW, sy = (1.f - (c.y / c.w * 0.5f + 0.5f)) * screenH;
                float a = std::min(1.f, f.timer / 0.3f);
                char nb[16]; std::snprintf(nb, sizeof(nb), "%d", (int)std::round(f.value));
                ui.textShadow(nb, sx, sy, f.crit ? 3 : 2, f.crit ? glm::vec4{1.f, 0.8f, 0.15f, a} : glm::vec4{1.f, 1.f, 1.f, a}, true);
            }
        }

        // Feed
        int fy = 150;
        for (auto it = feedLines.rbegin(); it != feedLines.rend(); ++it, fy += 22) {
            float a = std::min(1.f, it->timer / 0.5f);
            ui.textShadow(it->text.c_str(), screenW - 24 - UIBatch::textWidth(it->text.c_str(), 2), fy, 2,
                          {it->color.r, it->color.g, it->color.b, a});
        }

        // Toasts
        if (!toasts.empty()) {
            const Toast& t = toasts.front();
            float a = std::min({1.f, t.timer * 5.f, (t.dur - t.timer) * 3.f});
            float pop = 1.f + std::max(0.f, 0.25f - t.timer) * 2.f;
            int sc = pop > 1.2f ? 4 : 3;
            ui.textShadow(t.title.c_str(), cx, cy + 70, sc, {t.color.r, t.color.g, t.color.b, a}, true);
            if (!t.sub.empty()) ui.textShadow(t.sub.c_str(), cx, cy + 102, 2, {0.95f, 0.92f, 0.85f, a * 0.95f}, true);
        }

        // Timer and split
        if (h.showTimer) {
            std::string ts = formatTime(h.time);
            ui.textShadow("TIME", screenW - 24 - UIBatch::textWidth("TIME", 1), 18, 1, {0.8f, 0.8f, 0.85f, 0.8f});
            ui.textShadow(ts.c_str(), screenW - 24 - UIBatch::textWidth(ts.c_str(), 3), 30, 3, {1.f, 0.95f, 0.85f, 0.95f});
            if (splitTimer > 0.f) {
                float a = std::min(1.f, splitTimer);
                ui.textShadow(splitText.c_str(), screenW - 24 - UIBatch::textWidth(splitText.c_str(), 2), 60, 2,
                              {splitColor.r, splitColor.g, splitColor.b, a});
            }
        }
        // Speed: grey at a walk, cyan when you're flying
        if (h.speed >= 0.f) {
            char sp[24];
            std::snprintf(sp, sizeof(sp), "%d M/S", (int)std::round(h.speed));
            float k = std::min(1.f, std::max(0.f, (h.speed - 7.f) / 20.f));
            glm::vec4 c = glm::mix(glm::vec4{0.7f, 0.7f, 0.75f, 0.7f}, glm::vec4{0.35f, 0.95f, 1.f, 0.95f}, k);
            ui.textShadow(sp, screenW - 24 - UIBatch::textWidth(sp, 2), splitTimer > 0.f ? 84 : 62, 2, c);
        }

        if (showFPS) {
            char buf[16];
            std::snprintf(buf, sizeof(buf), "FPS %d", currentFPS);
            ui.text(buf, 16, 16, 2, {0.9f, 0.9f, 0.2f, 0.85f});
        }
        ui.end();
    }

    // =========================================================================
    // Overlays
    // =========================================================================
    void begin2D() { ui.begin(); }
    void end2D()   { ui.end(); }

    // One status line at the top: arena, wave, hostiles left (or what to do next).
    void renderObjective(const char* text, glm::vec3 accent) {
        begin2D();
        int w = std::max(260, UIBatch::textWidth(text, 2) + 40);
        ui.rect(screenW / 2 - w / 2, 10, w, 28, {0.04f, 0.04f, 0.07f, 0.6f});
        ui.rect(screenW / 2 - w / 2, 36, w, 2, {accent.r, accent.g, accent.b, 0.8f});
        ui.text(text, screenW / 2, 17, 2, {0.95f, 0.92f, 0.85f, 0.95f}, true);
        end2D();
    }

    // Big centred title card with a smaller line under it (arena names, waves,
    // new-enemy tips). alpha fades it in and out.
    void renderBanner(const char* title, const char* subtitle, glm::vec3 color, float alpha) {
        if (alpha <= 0.f) return;
        begin2D();
        int y = screenH / 2 - 150;
        int tw = UIBatch::textWidth(title, 4) + 60;
        int sw = UIBatch::textWidth(subtitle, 2) + 40;
        int bw = std::max(std::max(tw, sw), 320);
        ui.rect(screenW / 2 - bw / 2, y - 16, bw, subtitle[0] ? 88 : 62, {0.03f, 0.02f, 0.05f, 0.7f * alpha});
        ui.rect(screenW / 2 - bw / 2, y - 16, bw, 3, {color.r, color.g, color.b, alpha});
        ui.text(title, screenW / 2, y, 4, {color.r, color.g, color.b, alpha}, true);
        if (subtitle[0]) ui.text(subtitle, screenW / 2, y + 42, 2, {0.92f, 0.9f, 0.88f, alpha}, true);
        end2D();
    }

    void renderBossBar(const char* name, float fill, bool enraged) {
        begin2D();
        int w = 560, x = screenW / 2 - w / 2, y = 62;
        glm::vec4 col = enraged ? glm::vec4{1.f, 0.15f, 0.25f, 0.95f} : glm::vec4{1.f, 0.2f, 0.65f, 0.95f};
        ui.text(name, screenW / 2, y - 16, 2, col, true);
        ui.rect(x - 3, y - 3, w + 6, 18, {0.05f, 0.03f, 0.06f, 0.85f});
        ui.rect(x, y, w * glm::clamp(fill, 0.f, 1.f), 12, col);
        for (int i = 1; i < 4; ++i) ui.rect(x + w * i / 4, y, 2, 12, {0.f, 0.f, 0.f, 0.5f});
        end2D();
    }

    // A diamond marker at a screen position; when the target is off-screen the
    // caller passes onScreen=false and the marker sits on the screen edge.
    void renderMarker(float sx, float sy, bool onScreen, glm::vec3 color, const char* label) {
        begin2D();
        glm::vec4 c{color.r, color.g, color.b, 0.9f};
        int s = onScreen ? 7 : 9;
        for (int i = -s; i <= s; i += 2) {
            int half = s - std::abs(i);
            ui.rect(sx - half, sy + i, half * 2 + 1, 2, c);
        }
        if (label && label[0]) ui.textShadow(label, sx, sy + s + 6, 1, c, true);
        end2D();
    }

    // Fading control legend shown for the first few seconds of a run.
    void renderControlHint(float alpha, const char* grappleKey) {
        if (alpha <= 0.f) return;
        begin2D();
        const char* l1 = "WASD MOVE  SPACE JUMP  SHIFT DASH  CTRL SLIDE/SLAM  LMB FIRE  F PARRY  G GRENADE";
        char l2[128];
        std::snprintf(l2, sizeof(l2), "%s GRAPPLE  RMB AIM (RIFLES)  1-4 WEAPONS  TAB ARMORY  BKSP RESTART  ESC PAUSE", grappleKey);
        int baseY = screenH - 150;
        ui.rect(screenW / 2 - 360, baseY - 10, 720, 54, {0.05f, 0.05f, 0.08f, 0.55f * alpha});
        ui.text(l1, screenW / 2, baseY,      1, {0.88f, 0.88f, 0.92f, 0.9f * alpha}, true);
        ui.text(l2, screenW / 2, baseY + 22, 1, {0.88f, 0.88f, 0.92f, 0.9f * alpha}, true);
        end2D();
    }

    // FAST mode countdown: 3, 2, 1, GO
    void renderCountdown(float t) {
        begin2D();
        int n = (int)std::ceil(t);
        const char* txt = n >= 3 ? "3" : n == 2 ? "2" : n == 1 ? "1" : "GO";
        float frac = t - std::floor(t);
        float a = n > 0 ? 0.4f + 0.6f * frac : 1.f;
        ui.textShadow(txt, screenW / 2, screenH / 2 - 120, 10, {1.f, 0.75f, 0.2f, a}, true);
        end2D();
    }

    // Pause overlay — drawn on top of the (frozen) gameplay frame.
    static constexpr int PAUSE_ITEMS = 5;
    int pauseButtonY(int i) const { return screenH / 2 - 84 + i * 54; }
    void renderPause(int selected, const char* const labels[PAUSE_ITEMS], const char* modeLine) {
        begin2D();
        ui.rect(0, 0, screenW, screenH, {0.0f, 0.0f, 0.02f, 0.6f});
        ui.text("PAUSED", screenW / 2, screenH / 2 - 170, 5, {0.9f, 0.9f, 0.95f, 0.95f}, true);
        ui.text(modeLine, screenW / 2, screenH / 2 - 120, 2, {0.7f, 0.7f, 0.75f, 0.9f}, true);
        for (int i = 0; i < PAUSE_ITEMS; ++i) {
            bool sel = selected == i;
            int by = pauseButtonY(i);
            ui.rect(screenW / 2 - 150, by, 300, 46, sel ? glm::vec4{0.15f, 0.1f, 0.05f, 0.92f} : glm::vec4{0.08f, 0.08f, 0.10f, 0.85f});
            glm::vec4 b = sel ? glm::vec4{1.f, 0.6f, 0.1f, 0.9f} : glm::vec4{0.3f, 0.3f, 0.35f, 0.5f};
            ui.rect(screenW / 2 - 150, by, 300, 2, b);
            ui.rect(screenW / 2 - 150, by + 44, 300, 2, b);
            ui.text(labels[i], screenW / 2, by + 16, 2, sel ? glm::vec4{1.f, 0.7f, 0.15f, 1.f} : glm::vec4{0.75f, 0.75f, 0.8f, 0.9f}, true);
        }
        ui.text("ESC TO RESUME   UP/DOWN + ENTER OR CLICK   BACKSPACE IN GAME RESTARTS THE ROOM", screenW / 2, screenH / 2 + 196, 1, {0.55f, 0.55f, 0.6f, 0.85f}, true);
        end2D();
    }

    // ---- ARMORY (TAB): spend upgrade points ----
    static constexpr int ARM_COL_W = 270, ARM_ROW_H = 74, ARM_TOP = 150;
    int armoryX(int w) const { return screenW / 2 - 2 * ARM_COL_W + w * ARM_COL_W; }
    int armoryY(int s) const { return ARM_TOP + 46 + s * ARM_ROW_H; }
    bool armoryCellAt(int mx, int my, int& w, int& s) const {
        for (int i = 0; i < WEAPON_COUNT; ++i) for (int j = 0; j < UPGRADE_STATS; ++j) {
            int x = armoryX(i) + 8, y = armoryY(j);
            if (mx >= x && mx < x + ARM_COL_W - 16 && my >= y && my < y + ARM_ROW_H - 8) { w = i; s = j; return true; }
        }
        return false;
    }

    void renderArmory(const Progression& p, int selW, int selS) {
        begin2D();
        ui.rect(0, 0, screenW, screenH, {0.01f, 0.02f, 0.04f, 0.82f});
        ui.text("ARMORY", screenW / 2, 40, 5, {0.4f, 0.9f, 1.f, 1.f}, true);
        char buf[96];
        std::snprintf(buf, sizeof(buf), "LEVEL %d   -   %d POINT%s TO SPEND", p.level, p.points, p.points == 1 ? "" : "S");
        ui.text(buf, screenW / 2, 92, 2, p.points > 0 ? glm::vec4{1.f, 0.85f, 0.3f, 1.f} : glm::vec4{0.7f, 0.7f, 0.75f, 0.9f}, true);
        std::snprintf(buf, sizeof(buf), "XP %d / %d TO NEXT LEVEL", p.xp, Progression::xpToNext(p.level));
        ui.text(buf, screenW / 2, 116, 1, {0.6f, 0.65f, 0.7f, 0.9f}, true);

        static const char* STAT[] = {"DAMAGE", "FIRE RATE", "MAGAZINE", "MOD"};
        for (int w = 0; w < WEAPON_COUNT; ++w) {
            WeaponId wid = (WeaponId)w;
            const WeaponDef& d = weaponDef(wid);
            const WeaponUpgrades& u = p.up[w];
            int x = armoryX(w);
            ui.rect(x + 4, ARM_TOP, ARM_COL_W - 8, 36, {0.08f, 0.1f, 0.14f, 0.95f});
            std::snprintf(buf, sizeof(buf), "%d %s", w + 1, d.name);
            ui.text(buf, x + ARM_COL_W / 2, ARM_TOP + 11, 2, {0.9f, 0.9f, 0.95f, 1.f}, true);
            for (int s = 0; s < UPGRADE_STATS; ++s) {
                UpgradeStat st = (UpgradeStat)s;
                int y = armoryY(s);
                bool sel = selW == w && selS == s;
                int tier = p.tierOf(wid, st), maxT = Progression::maxTier(st);
                bool maxed = tier >= maxT, afford = p.canBuy(wid, st);
                glm::vec4 bg = sel ? glm::vec4{0.16f, 0.12f, 0.05f, 0.95f} : glm::vec4{0.06f, 0.07f, 0.1f, 0.9f};
                ui.rect(x + 8, y, ARM_COL_W - 16, ARM_ROW_H - 8, bg);
                if (sel) ui.frame(x + 8, y, ARM_COL_W - 16, ARM_ROW_H - 8, 2, {1.f, 0.65f, 0.15f, 1.f});
                glm::vec4 lc = maxed ? glm::vec4{0.4f, 1.f, 0.6f, 1.f} : afford ? glm::vec4{1.f, 0.85f, 0.35f, 1.f} : glm::vec4{0.6f, 0.6f, 0.65f, 0.9f};
                ui.text(s == 3 ? d.modName : STAT[s], x + 18, y + 8, s == 3 ? 1 : 2, lc);
                // Tier pips
                for (int k = 0; k < maxT; ++k)
                    ui.rect(x + 18 + k * 20, y + 30, 16, 8, k < tier ? glm::vec4{1.f, 0.7f, 0.2f, 1.f} : glm::vec4{0.25f, 0.25f, 0.3f, 1.f});
                // Value preview
                statPreview(wid, u, st, buf, sizeof(buf));
                ui.text(buf, x + 18, y + 48, 1, {0.75f, 0.78f, 0.82f, 0.95f});
                const char* costTxt = maxed ? "MAX" : (Progression::cost(st) == 2 ? "2 PTS" : "1 PT");
                ui.textRight(costTxt, x + ARM_COL_W - 20, y + 30, 1, maxed ? glm::vec4{0.4f, 1.f, 0.6f, 1.f} : lc);
            }
        }
        // Description of the selected cell
        const WeaponDef& d = weaponDef((WeaponId)selW);
        const char* desc = selS == 3 ? d.modDesc
                         : selS == 0 ? "+20% DAMAGE PER TIER"
                         : selS == 1 ? "15% LESS TIME BETWEEN SHOTS PER TIER"
                                     : "BIGGER MAGAZINE AND 12% FASTER RELOAD PER TIER";
        ui.text(desc, screenW / 2, armoryY(UPGRADE_STATS) + 8, 2, {0.9f, 0.9f, 0.9f, 0.95f}, true);
        ui.text("CLICK OR ENTER TO BUY   ARROWS TO MOVE   TAB / ESC TO CLOSE   LEVEL UP BY KILLING IN STYLE",
                screenW / 2, screenH - 30, 1, {0.55f, 0.55f, 0.6f, 0.85f}, true);
        end2D();
    }

    void renderVictoryArena(int kills, int shots, int hits, int deaths, float gameTime, float peakStyle,
                            int level, float best, bool newRecord) {
        begin2D();
        ui.rect(0, 0, screenW, screenH, {0.0f, 0.02f, 0.05f, 0.72f});
        ui.text("ALL ARENAS CLEARED", screenW / 2, screenH / 2 - 170, 4, {0.2f, 1.f, 0.6f, 0.95f}, true);
        ui.text("THE WARDEN IS DOWN", screenW / 2, screenH / 2 - 128, 2, {0.8f, 0.9f, 0.85f, 0.85f}, true);
        char buf[64];
        int y = screenH / 2 - 84;
        glm::vec4 sc{0.9f, 0.9f, 0.9f, 0.9f};
        std::snprintf(buf, sizeof(buf), "TIME  %s", formatTime(gameTime).c_str());
        ui.text(buf, screenW / 2, y, 2, sc, true);
        if (newRecord) ui.text("NEW RECORD", screenW / 2, y + 22, 2, {1.f, 0.85f, 0.2f, 0.5f + 0.5f * std::sin(hudTime * 6.f)}, true);
        else if (best > 0.f) { std::snprintf(buf, sizeof(buf), "BEST  %s", formatTime(best).c_str()); ui.text(buf, screenW / 2, y + 22, 2, {0.6f, 0.6f, 0.65f, 0.85f}, true); }
        float acc = shots > 0 ? (float)hits / (float)shots * 100.f : 0.f;
        std::snprintf(buf, sizeof(buf), "KILLS %d    ACCURACY %d%%    DEATHS %d    LEVEL %d", kills, (int)acc, deaths, level);
        ui.text(buf, screenW / 2, y + 52, 2, sc, true);
        float score = acc * 3.f + peakStyle + std::max(0.f, 1200.f - gameTime) - deaths * 60.f;
        const char* grade = score > 1150.f ? "S" : score > 900.f ? "A" : score > 650.f ? "B" : score > 400.f ? "C" : "D";
        ui.text(grade, screenW / 2, y + 90, 7, {1.f, 0.85f, 0.2f, 0.95f}, true);
        ui.text("ENTER - NEW RUN    ESC - MENU", screenW / 2, y + 170, 2, {0.6f, 0.6f, 0.6f, 0.8f}, true);
        end2D();
    }

    void renderVictoryFast(float time, float best, bool newRecord, const char* rank, int kills, int shots, int hits,
                           int deaths, const std::vector<float>& splits, const std::vector<float>& bestSplits) {
        begin2D();
        ui.rect(0, 0, screenW, screenH, {0.03f, 0.0f, 0.02f, 0.75f});
        ui.text("GAUNTLET COMPLETE", screenW / 2, 70, 4, {1.f, 0.6f, 0.2f, 0.95f}, true);
        std::string ts = formatTime(time);
        ui.text(ts.c_str(), screenW / 2, 120, 6, {1.f, 0.95f, 0.85f, 1.f}, true);
        char buf[96];
        if (newRecord) ui.text("NEW RECORD", screenW / 2, 172, 3, {1.f, 0.85f, 0.2f, 0.5f + 0.5f * std::sin(hudTime * 6.f)}, true);
        else if (best > 0.f) {
            std::snprintf(buf, sizeof(buf), "BEST %s   (+%.2f)", formatTime(best).c_str(), time - best);
            ui.text(buf, screenW / 2, 176, 2, {0.75f, 0.55f, 0.5f, 0.9f}, true);
        }
        // Splits table
        int y = 216;
        for (size_t i = 0; i < splits.size(); ++i, y += 22) {
            std::snprintf(buf, sizeof(buf), "LEVEL %d", (int)i + 1);
            ui.text(buf, screenW / 2 - 220, y, 2, {0.75f, 0.75f, 0.8f, 0.9f});
            std::string st = formatTime(splits[i]);
            ui.text(st.c_str(), screenW / 2 + 10, y, 2, {0.95f, 0.95f, 0.95f, 0.95f});
            if (i < bestSplits.size()) {
                float d = splits[i] - bestSplits[i];
                std::snprintf(buf, sizeof(buf), "%+.2f", d);
                ui.text(buf, screenW / 2 + 140, y, 2, d <= 0.f ? glm::vec4{0.3f, 1.f, 0.5f, 1.f} : glm::vec4{1.f, 0.4f, 0.35f, 1.f});
            }
        }
        float acc = shots > 0 ? (float)hits / (float)shots * 100.f : 0.f;
        std::snprintf(buf, sizeof(buf), "KILLS %d    ACCURACY %d%%    DEATHS %d", kills, (int)acc, deaths);
        ui.text(buf, screenW / 2, y + 16, 2, {0.9f, 0.9f, 0.9f, 0.9f}, true);
        ui.text(rank, screenW / 2, y + 48, 7, {1.f, 0.85f, 0.2f, 0.95f}, true);
        ui.text("ENTER - RUN IT AGAIN    ESC - MENU", screenW / 2, screenH - 40, 2, {0.6f, 0.6f, 0.6f, 0.85f}, true);
        end2D();
    }

    void renderDeath(const char* where, int kills, float gameTime, bool fast) {
        begin2D();
        ui.rect(0, 0, screenW, screenH, {0.3f, 0.0f, 0.0f, 0.5f});
        ui.text("YOU DIED", screenW / 2, screenH / 2 - 90, 5, {0.95f, 0.15f, 0.1f, 0.95f}, true);
        ui.text(where, screenW / 2, screenH / 2 - 34, 2, {0.95f, 0.8f, 0.75f, 0.9f}, true);
        char buf[64];
        std::snprintf(buf, sizeof(buf), "TIME %s   KILLS %d", formatTime(gameTime, fast).c_str(), kills);
        ui.text(buf, screenW / 2, screenH / 2 - 6, 2, {0.8f, 0.7f, 0.7f, 0.85f}, true);
        ui.text(fast ? "R - RESTART ROOM (CLOCK KEEPS RUNNING)" : "R - RETRY THIS ARENA (UPGRADES KEPT)",
                screenW / 2, screenH / 2 + 40, 2, {1.f, 0.85f, 0.5f, 0.95f}, true);
        ui.text("ENTER - NEW RUN    ESC - MENU", screenW / 2, screenH / 2 + 68, 2, {0.6f, 0.5f, 0.5f, 0.8f}, true);
        end2D();
    }

private:
    ShaderProgram scopeShader;
    GLuint quadVAO = 0, quadVBO = 0;

    void drawScopeLens(const HudState& h) {
        glDisable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        scopeShader.use();
        scopeShader.setVec2("uRes", {(float)screenW, (float)screenH});
        // uCenter is in GL window coordinates (origin bottom-left)
        scopeShader.setVec2("uCenter", {screenW * 0.5f + h.scopeSway.x, screenH * 0.5f + h.scopeSway.y});
        scopeShader.setFloat("uRadius", screenH * 0.47f);
        scopeShader.setFloat("uAlpha", glm::clamp((h.aim - 0.82f) / 0.18f, 0.f, 1.f));
        glBindVertexArray(quadVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);
        glDisable(GL_BLEND);
        glEnable(GL_DEPTH_TEST);
    }

    // Mil-dot reticle: thick outer posts, thin centre lines, a red dot
    void drawReticle(const HudState& h) {
        float a = glm::clamp((h.aim - 0.82f) / 0.18f, 0.f, 1.f);
        float cx = screenW * 0.5f + h.scopeSway.x, cy = screenH * 0.5f - h.scopeSway.y;
        float R = screenH * 0.47f;
        glm::vec4 k{0.02f, 0.02f, 0.02f, 0.95f * a};
        ui.rect(cx - R, cy - 2, R * 0.55f, 4, k);           // thick posts
        ui.rect(cx + R * 0.45f, cy - 2, R * 0.55f, 4, k);
        ui.rect(cx - 2, cy + R * 0.45f, 4, R * 0.55f, k);
        ui.rect(cx - 2, cy - R, 4, R * 0.55f, k);
        ui.rect(cx - R * 0.45f, cy - 0.5f, R * 0.9f, 1, k); // thin lines
        ui.rect(cx - 0.5f, cy - R * 0.45f, 1, R * 0.9f, k);
        for (int i = 1; i <= 4; ++i) {                      // mil dots
            float d = i * R * 0.1f;
            ui.rect(cx + d - 2, cy - 2, 4, 4, k); ui.rect(cx - d - 2, cy - 2, 4, 4, k);
            ui.rect(cx - 2, cy + d - 2, 4, 4, k); ui.rect(cx - 2, cy - d - 2, 4, 4, k);
        }
        ui.rect(cx - 2, cy - 2, 4, 4, {1.f, 0.1f, 0.05f, a});
        // Anything outside the lens circle is already black (scope.frag)
    }

    void drawVignettes(const StyleSystem& style, const HudState& h) {
        float low = h.health < 30.f ? (0.5f + 0.5f * std::sin(hudTime * 6.f)) * (1.f - h.health / 30.f) : 0.f;
        float dmg = std::max(0.f, damageVignette) * 1.2f;
        float a = std::min(0.85f, dmg + low * 0.5f);
        if (a <= 0.01f) return;
        for (int i = 0; i < 10; ++i) {
            float t = 1.f - i / 10.f;
            float w = 9.f;
            glm::vec4 c{0.7f, 0.f, 0.f, a * t * t * 0.55f};
            ui.rect(0, i * w, screenW, w, c);
            ui.rect(0, screenH - (i + 1) * w, screenW, w, c);
            ui.rect(i * w, (i + 1) * w, w, screenH - 2 * (i + 1) * w, c);
            ui.rect(screenW - (i + 1) * w, (i + 1) * w, w, screenH - 2 * (i + 1) * w, c);
        }
        (void)style;
    }

    void drawHealthPanel(const StyleSystem& style, const HudState& h) {
        int x = 22, y = screenH - 104;
        ui.rect(x - 8, y - 10, 318, 108, {0.02f, 0.02f, 0.05f, 0.55f});
        // Level + XP
        char buf[48];
        std::snprintf(buf, sizeof(buf), "LV %d", h.level);
        ui.textShadow(buf, x, y, 2, {0.45f, 0.9f, 1.f, 1.f});
        int xbx = x + 66, xbw = 230;
        float xpf = h.xpNext > 0 ? (float)h.xp / (float)h.xpNext : 0.f;
        ui.rect(xbx, y + 4, xbw, 6, {0.1f, 0.15f, 0.2f, 0.9f});
        ui.rect(xbx, y + 4, xbw * xpf, 6, {0.35f, 0.85f, 1.f, 0.95f});
        if (h.points > 0) {
            float p = 0.6f + 0.4f * std::sin(hudTime * 5.f);
            std::snprintf(buf, sizeof(buf), "%d UPGRADE%s - PRESS TAB", h.points, h.points == 1 ? "" : "S");
            ui.textShadow(buf, x, y - 22, 2, {1.f, 0.85f, 0.25f, p});
        }
        // Big number
        float hp = std::max(0.f, h.health);
        bool lowHp = hp < 30.f;
        glm::vec4 numC = lowHp ? glm::vec4{1.f, 0.3f + 0.2f * std::sin(hudTime * 8.f), 0.25f, 1.f}
                               : hp < 60.f ? glm::vec4{1.f, 0.8f, 0.3f, 1.f} : glm::vec4{1.f, 1.f, 1.f, 1.f};
        std::snprintf(buf, sizeof(buf), "%d", (int)std::ceil(hp));
        ui.textShadow(buf, x, y + 22, 6, numC);
        int nw = UIBatch::textWidth(buf, 6);
        std::snprintf(buf, sizeof(buf), "/%d HP", (int)h.maxHealth);
        ui.textShadow(buf, x + nw + 10, y + 50, 2, {0.75f, 0.75f, 0.8f, 0.9f});
        // Bar with a trailing "damage taken" segment
        int bx = x, by = y + 74, bw = 296, bh = 12;
        ui.rect(bx - 2, by - 2, bw + 4, bh + 4, {0.05f, 0.05f, 0.07f, 0.9f});
        ui.rect(bx, by, bw, bh, {0.22f, 0.05f, 0.05f, 0.85f});
        float trail = displayHealth / style.maxHealth, now = hp / style.maxHealth;
        ui.rect(bx, by, bw * std::max(trail, now), bh, {1.f, 0.85f, 0.75f, 0.7f});
        glm::vec4 barC = lowHp ? glm::vec4{0.95f, 0.15f, 0.12f, 1.f} : glm::vec4{0.2f, 0.85f, 0.4f, 0.95f};
        ui.rect(bx, by, bw * std::min(trail, now), bh, barC);
        for (int i = 1; i < 4; ++i) ui.rect(bx + bw * i / 4, by, 2, bh, {0.f, 0.f, 0.f, 0.45f});
    }

    void drawStyleMeter(const StyleSystem& style) {
        StyleRank rank = style.getRank();
        glm::vec4 rc = rankColor(rank);
        int sx = (screenW - 220) / 2, sy = screenH - 40;
        ui.rect(sx - 2, sy - 2, 224, 16, {0.05f, 0.05f, 0.07f, 0.85f});
        ui.rect(sx, sy, 220, 12, {0.1f, 0.1f, 0.15f, 0.8f});
        ui.rect(sx, sy, 220 * displayStyle / style.maxStyle, 12, rc);
        static const char* NAMES[] = {"D", "C", "B", "A", "S", "SSS"};
        if (displayStyle > 1.f) ui.textShadow(NAMES[(int)rank], screenW / 2, sy - 40, 4, rc, true);
        if (overdriveFlash > 0.f || style.overdrive) {
            float alpha = style.overdrive ? 0.9f : (overdriveFlash * 0.6f);
            int oy = screenH - 120;
            ui.rect(screenW / 2 - 120, oy, 240, 30, {0.9f, 0.5f, 0.05f, alpha * 0.3f});
            ui.rect(screenW / 2 - 120, oy, 240, 3, {1.f, 0.7f, 0.1f, alpha});
            ui.rect(screenW / 2 - 120, oy + 27, 240, 3, {1.f, 0.7f, 0.1f, alpha});
            ui.text("OVERDRIVE", screenW / 2, oy + 8, 2, {1.f, 0.75f, 0.2f, alpha}, true);
        }
    }

    void drawWeaponSlots(const HudState& h) {
        int slotW = 70, slotH = 64, pad = 6;
        int x0 = screenW - WEAPON_COUNT * (slotW + pad) - 14;
        int y0 = screenH - slotH - 14;
        char buf[24];
        for (int s = 0; s < WEAPON_COUNT; ++s) {
            const HudWeapon& w = h.weapons[s];
            int x = x0 + s * (slotW + pad);
            bool active = s == h.activeWeapon;
            ui.rect(x - 2, y0 - 2, slotW + 4, slotH + 4, active ? glm::vec4{1.f, 0.7f, 0.2f, 0.95f} : glm::vec4{0.3f, 0.3f, 0.35f, 0.5f});
            ui.rect(x, y0, slotW, slotH, active ? glm::vec4{0.16f, 0.15f, 0.2f, 0.95f} : glm::vec4{0.06f, 0.06f, 0.08f, 0.85f});
            std::snprintf(buf, sizeof(buf), "%d", s + 1);
            ui.text(buf, x + 4, y0 + 4, 1, active ? glm::vec4{1.f, 0.8f, 0.3f, 1.f} : glm::vec4{0.55f, 0.55f, 0.6f, 0.9f});
            ui.text(w.name, x + slotW / 2 + 3, y0 + 4, 1, {0.85f, 0.85f, 0.9f, active ? 1.f : 0.6f}, true);
            drawWeaponIcon(s, x + 6, y0 + 16, active);
            if (w.reloading) {
                ui.rect(x + 4, y0 + slotH - 8, (slotW - 8) * w.reload01, 4, {1.f, 0.75f, 0.25f, 0.95f});
                ui.text("RELOAD", x + slotW / 2, y0 + slotH - 22, 1, {1.f, 0.75f, 0.25f, 0.6f + 0.4f * std::sin(hudTime * 10.f)}, true);
            } else {
                std::snprintf(buf, sizeof(buf), "%d/%d", w.ammo, w.mag);
                glm::vec4 ac = w.ammo == 0 ? glm::vec4{1.f, 0.3f, 0.2f, 1.f} : glm::vec4{1.f, 0.95f, 0.8f, active ? 1.f : 0.65f};
                ui.text(buf, x + slotW / 2, y0 + slotH - 18, 2, ac, true);
            }
        }
        // Grenades
        int gx = x0 + WEAPON_COUNT * (slotW + pad) - pad;
        for (int i = 0; i < h.grenadeMax; ++i)
            ui.rect(gx - (i + 1) * 16, y0 - 18, 12, 10, i < h.grenades ? glm::vec4{0.45f, 1.f, 0.25f, 0.95f} : glm::vec4{0.2f, 0.25f, 0.2f, 0.6f});
        ui.textRight("G", gx - h.grenadeMax * 16 - 8, y0 - 17, 1, {0.7f, 0.9f, 0.6f, 0.9f});
    }

    void drawWeaponIcon(int s, int x, int y, bool active) {
        glm::vec4 c = active ? glm::vec4{0.95f, 0.88f, 0.65f, 0.95f} : glm::vec4{0.6f, 0.58f, 0.5f, 0.6f};
        switch (s) {
            case 0: ui.rect(x + 14, y + 2, 30, 4, c); ui.rect(x + 14, y + 6, 14, 8, c); ui.rect(x + 18, y + 14, 6, 6, c); break;
            case 1: ui.rect(x + 4, y + 2, 50, 4, c); ui.rect(x + 4, y + 6, 30, 5, c); ui.rect(x + 34, y + 6, 8, 3, c); ui.rect(x + 4, y + 11, 10, 6, c); break;
            case 2: ui.rect(x, y + 4, 58, 3, c); ui.rect(x, y + 7, 26, 5, c); ui.rect(x, y + 12, 8, 5, c); ui.rect(x + 24, y + 2, 3, 3, c); break;
            case 3: ui.rect(x, y + 6, 58, 4, c); ui.rect(x + 14, y, 22, 5, c); ui.rect(x, y + 10, 18, 7, c); ui.rect(x + 52, y + 4, 6, 8, c); break;
        }
    }

    void drawCrosshair(const HudState& h, int cx, int cy) {
        glm::vec4 col{h.crosshairColor.r, h.crosshairColor.g, h.crosshairColor.b, 0.9f};
        if (shootFlashTimer > 0.f && !h.hideCrosshair)
            ui.rect(cx - 5, cy - 5, 10, 10, {1.f, 0.9f, 0.5f, (shootFlashTimer / 0.06f) * 0.6f});
        if (hitmarkerTimer > 0.f) {
            float a = hitmarkerTimer / 0.18f;
            glm::vec4 hc = hitmarkerKill ? glm::vec4{1.f, 0.35f, 0.f, a} : glm::vec4{1.f, 1.f, 1.f, a};
            int g = h.scoped ? 10 : 6;
            for (int i = 0; i < 4; ++i) {   // diagonal ticks
                float sx = (i & 1) ? 1.f : -1.f, sy = (i & 2) ? 1.f : -1.f;
                for (int k = 0; k < 4; ++k)
                    ui.rect(cx + sx * (g + k * 2) - 1.5f, cy + sy * (g + k * 2) - 1.5f, 3, 3, hc);
            }
        }
        if (h.hideCrosshair) return;
        if (h.grappleTarget) {   // cyan brackets: a moving platform you can hook
            glm::vec4 gc{0.3f, 1.f, 1.f, 0.85f};
            int r = 16;
            for (int sx : {-1, 1}) for (int sy : {-1, 1}) {
                ui.rect(cx + sx * r - (sx < 0 ? 0 : 6), cy + sy * r - (sy < 0 ? 0 : 2), 6, 2, gc);
                ui.rect(cx + sx * r - (sx < 0 ? 0 : 2), cy + sy * r - (sy < 0 ? 0 : 6), 2, 6, gc);
            }
        }
        float gap = 2.f + h.spread;
        ui.rect(cx - gap - 6, cy - 1, 6, 2, col);
        ui.rect(cx + gap,     cy - 1, 6, 2, col);
        ui.rect(cx - 1, cy - gap - 6, 2, 6, col);
        ui.rect(cx - 1, cy + gap,     2, 6, col);
        ui.rect(cx - 1, cy - 1, 2, 2, {col.r, col.g, col.b, 0.6f});
    }

    glm::vec4 rankColor(StyleRank r) const {
        switch (r) {
            case StyleRank::D:   return {0.5f, 0.5f, 0.5f, 0.9f};
            case StyleRank::C:   return {0.3f, 0.45f, 1.f, 0.9f};
            case StyleRank::B:   return {0.2f, 0.9f, 0.3f, 0.9f};
            case StyleRank::A:   return {1.f, 0.9f, 0.15f, 0.9f};
            case StyleRank::S:   return {1.f, 0.55f, 0.1f, 0.95f};
            case StyleRank::SSS: {
                float pulse = 0.5f + 0.5f * std::sin(hudTime * 6.f);
                return {1.f, 0.1f + pulse * 0.3f, 0.1f, 1.f};
            }
        }
        return {1, 1, 1, 1};
    }

    void statPreview(WeaponId w, const WeaponUpgrades& u, UpgradeStat s, char* buf, size_t n) const {
        WeaponUpgrades next = u;
        bool maxed = false;
        if (s == UpgradeStat::MOD) { std::snprintf(buf, n, "%s", u.mod ? "INSTALLED" : "SPECIAL PERK"); return; }
        if (next.tier[(int)s] < MAX_TIER) ++next.tier[(int)s]; else maxed = true;
        switch (s) {
            case UpgradeStat::DAMAGE:
                if (maxed) std::snprintf(buf, n, "%d DMG", (int)weaponDamage(w, u));
                else std::snprintf(buf, n, "%d > %d DMG", (int)weaponDamage(w, u), (int)weaponDamage(w, next));
                break;
            case UpgradeStat::RATE:
                if (maxed) std::snprintf(buf, n, "%.2fS / SHOT", weaponCooldown(w, u));
                else std::snprintf(buf, n, "%.2f > %.2fS / SHOT", weaponCooldown(w, u), weaponCooldown(w, next));
                break;
            default:
                if (maxed) std::snprintf(buf, n, "%d RDS  %.1fS", weaponMag(w, u), weaponReload(w, u));
                else std::snprintf(buf, n, "%d > %d RDS  %.1fS", weaponMag(w, u), weaponMag(w, next), weaponReload(w, next));
                break;
        }
    }
};
