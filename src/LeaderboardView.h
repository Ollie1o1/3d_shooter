#pragma once
// =============================================================================
// LeaderboardView.h — draws a mode's leaderboard (Progression.h) as a table.
// Shared by the main menu's LEADERBOARD page and the victory screens.
// =============================================================================
#include "UIBatch.h"
#include "Progression.h"
#include "Difficulty.h"
#include <cstdio>
#include <cmath>

// A panel at (x, y), w wide: a title, then up to `rows` entries. `highlight`
// marks one row (the run just saved) in gold. Score boards show the score,
// with the time (ARENA) or the wave reached (ENDLESS, DAILY) beside it.
inline void drawLeaderboardTable(UIBatch& ui, const Leaderboard& lb, Board bd, float x, float y, float w,
                                 int rows = Leaderboard::KEEP, int highlight = -1, float time = 0.f,
                                 const char* title = nullptr) {
    static const char* TITLES[] = {"ARENA - FULL RUN", "FAST - THE GAUNTLET", "ENDLESS", "DAILY CHALLENGE"};
    static const glm::vec4 ACCENT[] = {{0.3f, 1.f, 0.6f, 0.9f}, {1.f, 0.6f, 0.2f, 0.9f},
                                       {1.f, 0.3f, 0.45f, 0.9f}, {0.4f, 0.8f, 1.f, 0.9f}};
    const auto& l = lb.list(bd);
    glm::vec4 acc = ACCENT[(int)bd];
    ui.rect(x, y, w, 34 + rows * 24 + 12, {0.03f, 0.03f, 0.06f, 0.82f});
    ui.rect(x, y, w, 2, acc);
    ui.text(title ? title : TITLES[(int)bd], x + w / 2, y + 12, 2, {acc.r, acc.g, acc.b, 1.f}, true);
    float ry = y + 40;
    if (l.empty()) {
        ui.text(Leaderboard::byScore(bd) ? "NO SCORES YET" : "NO TIMES YET", x + w / 2, ry + 4, 2, {0.55f, 0.55f, 0.6f, 0.8f}, true);
        return;
    }
    char buf[48];
    for (int i = 0; i < rows && i < (int)l.size(); ++i, ry += 24) {
        const auto& e = l[i];
        bool hi = i == highlight;
        if (hi) ui.rect(x + 4, ry - 4, w - 8, 22, {1.f, 0.75f, 0.15f, 0.14f + 0.08f * std::sin(time * 6.f)});
        glm::vec4 c = hi ? glm::vec4{1.f, 0.85f, 0.25f, 1.f}
                    : i == 0 ? glm::vec4{1.f, 0.95f, 0.8f, 0.95f} : glm::vec4{0.82f, 0.82f, 0.86f, 0.9f};
        std::snprintf(buf, sizeof(buf), "%d", i + 1);
        ui.textRight(buf, x + 34, ry, 2, c);
        ui.text(e.name.c_str(), x + 48, ry, 2, c);
        glm::vec4 dim{0.6f, 0.6f, 0.66f, 0.85f};
        if (Leaderboard::byScore(bd)) {
            std::snprintf(buf, sizeof(buf), "%d", e.score);
            ui.textRight(buf, x + w - 72, ry, 2, c);
            if (bd == Board::ARENA) { std::string t = formatTime(e.time); ui.text(t.c_str(), x + w - 64, ry - 2, 1, dim); }
            else { std::snprintf(buf, sizeof(buf), "WAVE %d", e.wave); ui.text(buf, x + w - 64, ry - 2, 1, dim); }
            ui.text(difficulty(e.difficulty).name, x + w - 64, ry + 8, 1, dim);
        } else {
            std::string t = formatTime(e.time);
            ui.textRight(t.c_str(), x + w - 72, ry, 2, c);
            ui.text(difficulty(e.difficulty).name, x + w - 64, ry + 4, 1, dim);
        }
    }
}
