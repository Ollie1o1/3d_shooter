#pragma once
// =============================================================================
// PlainReveal.h — Chapter 4's opening cutscene (docs/chapter-4-design.md §1-2),
// as a deterministic clock the game and the tests share (like LabIntro.h).
//
// (What comes before it, UNIT 24 taking its pieces and going through the portal,
// is the end of Chapter 3: LabEnding.h.)
// THE ARRIVAL (first person):
//   +0.0 white: through the portal. UNIT 24 drops onto the plain and steadies
//   +1.6 looks left (a meteor comes down out there), then right
//   +7.0 ahead: something on the horizon under the blood moon
//   +8.2 it rises out of the plain. It is a back.
//  +11.6 the moon blinks shut
//  +12.4 it turns round, fist over fist
//  +16.0 the eye in its chest opens, the ribs part
//  +16.8 it screams
//  +21.6 control returns
//
// Hold Jump to skip (after letting go of the menu's Space first).
// =============================================================================
#include <algorithm>
#include <cmath>

struct PlainReveal {
    static constexpr float PRO = 0.f;   // (where the arrival starts on its clock)
    static constexpr float DURATION = PRO + 22.f, SKIP_HOLD = 0.65f;
    // The arrival's beats
    static constexpr float LAND = PRO + 0.45f, METEOR = PRO + 2.6f, RISE0 = PRO + 8.2f, RISE1 = PRO + 11.8f, BLINK0 = PRO + 11.6f, BLINK1 = PRO + 12.5f;
    static constexpr float TURN0 = PRO + 12.4f, TURN1 = PRO + 16.f, EYE0 = PRO + 15.8f, EYE1 = PRO + 16.8f, SCREAM = PRO + 16.8f;
    float time = -1.f, skip = 0.f;
    bool skipReady = false;
    bool active() const { return time >= 0.f; }
    void start() { time = 0.f; skip = 0.f; skipReady = false; }
    // True the tick it ends (played through or skipped)
    bool update(float dt, bool held) {
        if (!active()) return false;
        time += dt;
        if (!held) skipReady = true;
        skip = held && skipReady ? skip + dt : 0.f;
        if (time >= DURATION || (time > 1.f && skip >= SKIP_HOLD)) { time = -1.f; skip = 0.f; return true; }
        return false;
    }
    static float sm(float a, float b, float t) { float x = std::clamp((t - a) / (b - a), 0.f, 1.f); return x * x * (3.f - 2.f * x); }

    // ---- the arrival's camera (first person): where it looks, its height over the feet, its lens ----
    float a() const { return time - PRO; }
    float yaw() const {
        float t = a();
        return -90.f - 52.f * sm(1.6f, 3.4f, t) + 98.f * sm(3.9f, 5.9f, t) - 46.f * sm(6.2f, 7.6f, t);
    }
    float pitch() const {
        float t = a();
        float p = -38.f + 34.f * sm(0.4f, 1.8f, t);                 // up off its feet
        p += 4.f * sm(8.f, 11.f, t) + 3.f * sm(11.5f, 14.f, t);   // following it up
        p -= 2.f * sm(19.5f, 21.5f, t);
        return p;
    }
    float eye() const {   // the camera's height: a drop from the portal, the knees taking it, standing up
        float t = a();
        if (t < 0.45f) return 1.7f + 2.6f * (1.f - (t / 0.45f) * (t / 0.45f));
        float u = t - 0.45f;
        return 1.7f - 0.55f * std::exp(-u * 4.f) * std::sin(std::min(u * 5.f, 3.14159f));
    }
    float fov(float base) const {
        float t = a(); return base - (base - 62.f) * sm(12.f, 15.f, t) * (1.f - sm(19.5f, 21.6f, t)); }
    float white() const { return 1.f - sm(0.f, 0.9f, a()); }
    float bars() const { return sm(0.f, 0.6f, time) * (1.f - sm(DURATION - 1.2f, DURATION - 0.2f, time)); }

    // The Gleaner
    float rise() const { return sm(RISE0, RISE1, time); }
    float turn() const { return sm(TURN0, TURN1, time); }
    float eyeOpen() const { return sm(EYE0, EYE1, time); }
    float ribs() const { return sm(EYE0 + 0.2f, EYE1 + 0.6f, time); }
    float lid() const { return sm(BLINK0, BLINK1, time); }   // the sky's moon shutting

};
