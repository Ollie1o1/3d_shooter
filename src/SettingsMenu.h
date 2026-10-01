#pragma once
// =============================================================================
// SettingsMenu.h — the settings page, shared by the main menu and the in-game
// pause menu. Keyboard (UP/DOWN, LEFT/RIGHT, ENTER, ESC) and mouse (hover,
// click, drag the sliders) both work; every change is saved immediately.
// =============================================================================
#include "Settings.h"
#include "UIBatch.h"
#include <SDL2/SDL.h>
#include <functional>
#include <vector>
#include <cstdio>
#include <cmath>

class SettingsMenu {
public:
    GameSettings* s = nullptr;
    std::function<void()> onBack;
    int  selected = 1;          // row index (row 0 is a header)
    int  screenW, screenH;

    SettingsMenu(int w, int h) : screenW(w), screenH(h) {}

    enum class Kind { HEADER, SLIDER, TOGGLE, CYCLE, BACK };
    struct Row {
        Kind kind; const char* label;
        float* f = nullptr; float lo = 0, hi = 1, step = 0.1f; int fmt = 0;   // fmt: 0 int, 1 2dp, 2 percent, 3 multiplier
        bool*  b = nullptr;
        int*   i = nullptr; int count = 0; std::function<const char*(int)> name;
    };

    std::vector<Row> rows() const {
        std::vector<Row> r;
        auto header = [&](const char* l) { Row x; x.kind = Kind::HEADER; x.label = l; r.push_back(x); };
        auto slider = [&](const char* l, float* f, float lo, float hi, float step, int fmt) {
            Row x; x.kind = Kind::SLIDER; x.label = l; x.f = f; x.lo = lo; x.hi = hi; x.step = step; x.fmt = fmt; r.push_back(x); };
        auto toggle = [&](const char* l, bool* b) { Row x; x.kind = Kind::TOGGLE; x.label = l; x.b = b; r.push_back(x); };
        auto cycle  = [&](const char* l, int* i, int n, std::function<const char*(int)> nm) {
            Row x; x.kind = Kind::CYCLE; x.label = l; x.i = i; x.count = n; x.name = nm; r.push_back(x); };
        GameSettings* g = s;
        header("VIDEO");
        slider("FIELD OF VIEW", &g->fov, 60.f, 120.f, 1.f, 0);
        cycle ("FPS CAP", &g->fpsCap, 5, [g](int) { return g->getFPSCapLabel(); });
        toggle("SHOW FPS", &g->showFPS);
        toggle("CRT FILTER", &g->crtFilter);
        slider("SCREEN SHAKE", &g->screenShake, 0.f, 1.f, 0.05f, 2);
        toggle("VIEW BOB", &g->viewBob);
        header("CONTROLS");
        slider("MOUSE SENSITIVITY", &g->sensitivity, 0.01f, 1.0f, 0.01f, 1);
        slider("ZOOM SENSITIVITY", &g->zoomSens, 0.3f, 1.5f, 0.05f, 3);
        cycle ("GRAPPLE BUTTON", &g->grappleKey, GameSettings::GRAPPLE_KEYS, [](int i) { return GameSettings::grappleLabel(i); });
        toggle("INVERT Y", &g->invertY);
        toggle("MOUSE SPIKE FILTER", &g->mouseFilter);
        header("AUDIO");
        slider("MASTER VOLUME", &g->audioVolume, 0.f, 1.f, 0.05f, 2);
        header("GAMEPLAY");
        toggle("RUN TIMER", &g->showTimer);
        toggle("DAMAGE NUMBERS", &g->damageNumbers);
        cycle ("CROSSHAIR", &g->crosshair, GameSettings::CROSSHAIR_COLORS, [](int i) { return GameSettings::crosshairLabel(i); });
        Row back; back.kind = Kind::BACK; back.label = "BACK"; r.push_back(back);
        return r;
    }

    // ---- layout ----
    static constexpr int PANEL_W = 680, ROW_H = 27, HEAD_H = 24, TOP = 90;
    int rowY(const std::vector<Row>& r, int idx) const {
        int y = TOP;
        for (int k = 0; k < idx; ++k) y += r[k].kind == Kind::HEADER ? HEAD_H : ROW_H;
        return y;
    }
    int rowH(const Row& r) const { return r.kind == Kind::HEADER ? HEAD_H : ROW_H; }
    int barX() const { return screenW / 2 + 20; }
    static constexpr int BAR_W = 200;

    // ---- input ----
    void handleEvent(const SDL_Event& e) {
        if (!s) return;
        auto r = rows();
        int n = (int)r.size();
        if (r[selected].kind == Kind::HEADER) selected = nextSelectable(r, selected, 1);
        if (e.type == SDL_KEYDOWN) {
            switch (e.key.keysym.sym) {
                case SDLK_UP:    selected = nextSelectable(r, selected, -1); break;
                case SDLK_DOWN:  selected = nextSelectable(r, selected,  1); break;
                case SDLK_LEFT:  adjust(r[selected], -1); break;
                case SDLK_RIGHT: adjust(r[selected],  1); break;
                case SDLK_RETURN:
                case SDLK_SPACE:
                    if (r[selected].kind == Kind::BACK) { if (onBack) onBack(); }
                    else adjust(r[selected], 1);
                    break;
                case SDLK_ESCAPE: if (onBack) onBack(); break;
                default: break;
            }
        }
        if (e.type == SDL_MOUSEMOTION) {
            int idx = rowAt(r, e.motion.x, e.motion.y);
            if (dragging >= 0) setFromMouse(r[dragging], e.motion.x);
            else if (idx >= 0 && r[idx].kind != Kind::HEADER) selected = idx;
        }
        if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
            int idx = rowAt(r, e.button.x, e.button.y);
            if (idx >= 0 && r[idx].kind != Kind::HEADER) {
                selected = idx;
                const Row& row = r[idx];
                int mx = e.button.x;
                if (row.kind == Kind::BACK) { if (onBack) onBack(); }
                else if (row.kind == Kind::SLIDER) {
                    if (mx >= barX() - 10 && mx <= barX() + BAR_W + 10) { dragging = idx; setFromMouse(row, mx); }
                    else adjust(row, mx < barX() ? -1 : 1);
                } else adjust(row, mx < barX() - 24 ? -1 : 1);
            }
        }
        if (e.type == SDL_MOUSEBUTTONUP && e.button.button == SDL_BUTTON_LEFT && dragging >= 0) {
            dragging = -1;
            s->clampAll(); s->save();
        }
        if (e.type == SDL_MOUSEWHEEL && r[selected].kind != Kind::BACK) adjust(r[selected], e.wheel.y > 0 ? 1 : -1);
        (void)n;
    }

    // ---- drawing ----
    void render(UIBatch& ui, float time) {
        if (!s) return;
        auto r = rows();
        int cx = screenW / 2;
        ui.rect(cx - PANEL_W / 2 - 12, 34, PANEL_W + 24, screenH - 62, {0.03f, 0.03f, 0.06f, 0.78f});
        ui.text("SETTINGS", cx, 48, 4, {0.5f, 0.85f, 1.f, 1.f}, true);
        for (int k = 0; k < (int)r.size(); ++k) {
            const Row& row = r[k];
            int y = rowY(r, k);
            if (row.kind == Kind::HEADER) {
                ui.text(row.label, cx - PANEL_W / 2, y + 10, 2, {1.f, 0.6f, 0.15f, 0.95f});
                ui.rect(cx - PANEL_W / 2 + UIBatch::textWidth(row.label, 2) + 12, y + 16, PANEL_W - UIBatch::textWidth(row.label, 2) - 12, 2,
                        {1.f, 0.6f, 0.15f, 0.35f});
                continue;
            }
            bool sel = k == selected;
            float pulse = sel ? 0.5f + 0.5f * std::sin(time * 5.f) : 0.f;
            ui.rect(cx - PANEL_W / 2, y + 2, PANEL_W, ROW_H - 4,
                    sel ? glm::vec4{0.14f + pulse * 0.05f, 0.09f, 0.04f, 0.9f} : glm::vec4{0.08f, 0.08f, 0.11f, 0.7f});
            if (sel) ui.rect(cx - PANEL_W / 2, y + 2, 4, ROW_H - 4, {1.f, 0.6f + pulse * 0.2f, 0.1f, 1.f});
            glm::vec4 labelC = sel ? glm::vec4{1.f, 0.75f, 0.3f, 1.f} : glm::vec4{0.75f, 0.75f, 0.8f, 0.95f};
            if (row.kind == Kind::BACK) { ui.text(row.label, cx, y + 8, 2, labelC, true); continue; }
            ui.text(row.label, cx - PANEL_W / 2 + 16, y + 8, 2, labelC);
            glm::vec4 valC = sel ? glm::vec4{1.f, 0.92f, 0.5f, 1.f} : glm::vec4{0.85f, 0.85f, 0.85f, 0.9f};
            char buf[32];
            switch (row.kind) {
            case Kind::SLIDER: {
                float t = (*row.f - row.lo) / (row.hi - row.lo);
                int bx = barX(), by = y + 11;
                ui.rect(bx, by, BAR_W, 6, {0.2f, 0.2f, 0.25f, 0.9f});
                ui.rect(bx, by, BAR_W * t, 6, {1.f, 0.6f, 0.15f, 0.95f});
                ui.rect(bx + BAR_W * t - 4, by - 5, 8, 16, sel ? glm::vec4{1.f, 0.9f, 0.6f, 1.f} : glm::vec4{0.8f, 0.8f, 0.85f, 1.f});
                formatValue(row, buf, sizeof(buf));
                ui.text(buf, bx + BAR_W + 20, y + 8, 2, valC);
                break;
            }
            case Kind::TOGGLE: {
                bool on = *row.b;
                int bx = barX();
                ui.rect(bx, y + 6, 64, 18, on ? glm::vec4{0.2f, 0.75f, 0.4f, 0.9f} : glm::vec4{0.3f, 0.3f, 0.35f, 0.9f});
                ui.text(on ? "ON" : "OFF", bx + 32, y + 8, 2, {1.f, 1.f, 1.f, 0.95f}, true);
                break;
            }
            case Kind::CYCLE: {
                const char* nm = row.name(*row.i);
                int bx = barX();
                ui.text("<", bx - 4, y + 8, 2, valC);
                ui.text(nm, bx + 100, y + 8, 2, valC, true);
                ui.text(">", bx + 196, y + 8, 2, valC);
                if (row.i == &s->crosshair) {
                    float cr, cg, cb; s->crosshairColor(cr, cg, cb);
                    ui.rect(bx + 230, y + 8, 14, 14, {cr, cg, cb, 1.f});
                }
                break;
            }
            default: break;
            }
        }
        ui.text("UP/DOWN SELECT   LEFT/RIGHT OR CLICK TO CHANGE   DRAG SLIDERS   ESC BACK",
                cx, screenH - 22, 1, {0.55f, 0.55f, 0.6f, 0.85f}, true);
    }

private:
    int dragging = -1;

    int nextSelectable(const std::vector<Row>& r, int from, int dir) const {
        int n = (int)r.size();
        int k = from;
        for (int guard = 0; guard < n; ++guard) {
            k = (k + dir + n) % n;
            if (r[k].kind != Kind::HEADER) return k;
        }
        return from;
    }

    int rowAt(const std::vector<Row>& r, int mx, int my) const {
        int cx = screenW / 2;
        if (mx < cx - PANEL_W / 2 || mx > cx + PANEL_W / 2) return -1;
        for (int k = 0; k < (int)r.size(); ++k) {
            int y = rowY(r, k);
            if (my >= y && my < y + rowH(r[k])) return k;
        }
        return -1;
    }

    void adjust(const Row& row, int dir) {
        switch (row.kind) {
        case Kind::SLIDER: *row.f = std::round((*row.f + dir * row.step) / row.step) * row.step; break;
        case Kind::TOGGLE: *row.b = !*row.b; break;
        case Kind::CYCLE:  *row.i = ((*row.i + dir) % row.count + row.count) % row.count; break;
        default: return;
        }
        s->clampAll();
        s->save();
    }

    void setFromMouse(const Row& row, int mx) {
        if (row.kind != Kind::SLIDER) return;
        float t = std::min(1.f, std::max(0.f, (mx - barX()) / (float)BAR_W));
        float v = row.lo + t * (row.hi - row.lo);
        *row.f = std::round(v / row.step) * row.step;
        s->clampAll();
    }

    static void formatValue(const Row& row, char* buf, size_t n) {
        float v = *row.f;
        switch (row.fmt) {
            case 0: std::snprintf(buf, n, "%d", (int)std::round(v)); break;
            case 1: std::snprintf(buf, n, "%.2f", v); break;
            case 2: std::snprintf(buf, n, "%d%%", (int)std::round(v * 100.f)); break;
            default: std::snprintf(buf, n, "%.2fX", v); break;
        }
    }
};
