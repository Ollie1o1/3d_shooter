#pragma once
// =============================================================================
// StoryRank.h — a chapter's rank on its completion card, and the best kept for
// it on the Story page. S: no deaths, under the chapter's S time, with style to
// spare; A: under three deaths and under its A time; B: under six deaths; C.
// The times are first guesses from each chapter's length: tune them from play
// (runs.jsonl's sectionSeconds, tools/runlog.py). No OpenGL.
// =============================================================================
#include <algorithm>

namespace storyrank {

constexpr int CHAPTERS = 5;
struct Par { float sSeconds, aSeconds, sStyle; };
inline const Par& par(int chapter) {
    static const Par P[CHAPTERS] = {
        {15 * 60.f, 20 * 60.f, 5000.f},   // 1 THE CRADLE: nine rooms and the Sorter
        {15 * 60.f, 20 * 60.f, 5000.f},   // 2 THE DEAD SPAN: eight stretches and Ballast
        {22 * 60.f, 30 * 60.f, 7000.f},   // 3 THE LIVING WARD: ten chambers and the Successor
        {18 * 60.f, 25 * 60.f, 6000.f},   // 4 THE BLOOD MOON PLAIN: one long fight, inside and out
        {22 * 60.f, 30 * 60.f, 7000.f},   // 5 THE COLD STORE: ten sections, 24-K and the Restorer
    };
    return P[std::clamp(chapter, 1, CHAPTERS) - 1];
}

// 1 C, 2 B, 3 A, 4 S (0: none yet)
inline int rank(int chapter, float seconds, int deaths, float style) {
    const Par& p = par(chapter);
    if (deaths == 0 && seconds <= p.sSeconds && style >= p.sStyle) return 4;
    if (deaths < 3 && seconds <= p.aSeconds) return 3;
    if (deaths < 6) return 2;
    return 1;
}
inline const char* letter(int r) { return r >= 4 ? "S" : r == 3 ? "A" : r == 2 ? "B" : r == 1 ? "C" : "-"; }

}  // namespace storyrank
