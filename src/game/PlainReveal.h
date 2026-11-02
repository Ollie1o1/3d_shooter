#pragma once
// =============================================================================
// PlainReveal.h — Chapter 4's opening cutscene (docs/chapter-4-design.md §1-2),
// as a deterministic clock the game and the tests share (like LabIntro.h).
//
// THE PROLOGUE (third person, in the void where the Successor fell):
//   0.0  UNIT 24 kneels over the Successor's body. STATUS: REJECTED
//   4.6  the pieces come to it, one by one, and its spine lights
//   6.8  STATUS: COMPLETE
//   7.0  it stands, turns to the portal, walks
//  10.4  it stops and looks back at the body
//  11.6  it goes through
// THE ARRIVAL (first person, PRO seconds in):
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
    static constexpr float PRO = 13.f;   // the prologue's length
    static constexpr float DURATION = PRO + 22.f, SKIP_HOLD = 0.65f;
    // The prologue's beats
    static constexpr float PART0 = 4.6f, PART_GAP = 1.f, PART_FLY = 0.7f, COMPLETE = 6.8f;
    static constexpr float STAND0 = 7.f, STAND1 = 8.2f, TURN_U0 = 8.2f, TURN_U1 = 8.9f, WALK0 = 8.9f, STOP = 10.4f, GO = 11.6f, THROUGH = 12.3f;
    // The arrival's beats (absolute)
    static constexpr float LAND = PRO + 0.45f, METEOR = PRO + 2.6f, RISE0 = PRO + 8.2f, RISE1 = PRO + 11.8f, BLINK0 = PRO + 11.6f, BLINK1 = PRO + 12.5f;
    static constexpr float TURN0 = PRO + 12.4f, TURN1 = PRO + 16.f, EYE0 = PRO + 15.8f, EYE1 = PRO + 16.8f, SCREAM = PRO + 16.8f;
    float time = -1.f, skip = 0.f;
    bool skipReady = false;
    bool active() const { return time >= 0.f; }
    bool prologue() const { return active() && time < PRO; }
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

    // ---- the prologue (positions in the void set's own frame: the body at the origin, the portal at +Z) ----
    float kneel() const { return 1.f - sm(STAND0, STAND1, time); }            // 1 kneeling .. 0 standing
    float facing() const { return 3.14159265f * (1.f - sm(TURN_U0, TURN_U1, time)); }   // its yaw: to the body, then the portal
    float partIn(int k) const { return sm(PART0 + k * PART_GAP, PART0 + k * PART_GAP + PART_FLY, time); }   // 0..1 a piece's flight
    float spine() const { return (partIn(0) + partIn(1) + partIn(2)) / 3.f; } // its spine lighting, piece by piece
    float unitZ() const {   // walking to the portal: on, a stop to look back, then through
        return 1.5f + 8.5f * sm(WALK0, STOP, time) + 7.5f * sm(GO, PRO, time);
    }
    float walking() const { return (time > WALK0 && time < STOP + 0.2f) || time > GO ? 1.f : 0.f; }
    float lookBack() const { return sm(STOP, STOP + 0.5f, time) * (1.f - sm(GO - 0.3f, GO + 0.1f, time)); }
    bool  complete() const {   // the nameplate: REJECTED, a flicker, COMPLETE
        if (time < COMPLETE) return false;
        if (time > COMPLETE + 0.8f) return true;
        return std::fmod(time * 9.f, 1.f) > 0.5f;
    }
    float portalWhite() const { return sm(THROUGH, PRO, time); }

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
        if (prologue()) return 50.f;   // a long lens on the two of them
        float t = a(); return base - (base - 62.f) * sm(12.f, 15.f, t) * (1.f - sm(19.5f, 21.6f, t)); }
    float white() const { return prologue() ? portalWhite() : 1.f - sm(0.f, 0.9f, a()); }
    float bars() const { return sm(0.f, 0.6f, time) * (1.f - sm(DURATION - 1.2f, DURATION - 0.2f, time)); }

    // The Gleaner
    float rise() const { return sm(RISE0, RISE1, time); }
    float turn() const { return sm(TURN0, TURN1, time); }
    float eyeOpen() const { return sm(EYE0, EYE1, time); }
    float ribs() const { return sm(EYE0 + 0.2f, EYE1 + 0.6f, time); }
    float lid() const { return sm(BLINK0, BLINK1, time); }   // the sky's moon shutting

    const char* caption() const {
        float t = a();
        if (t > 17.f && t < 21.6f) return "UNIT ?  /  GLEANER  /  PURPOSE: RECLAIM";
        return "";
    }
};
