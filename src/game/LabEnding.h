#pragma once
// =============================================================================
// LabEnding.h — the end of Chapter 3 (docs/chapter-4-design.md §1), as a clock
// the game and the tests share. It plays in the final assessment itself, once
// the Successor is dead and you step through the doors it held shut:
//
//   0.0  third person: UNIT 24 stops in the doorway and looks back at the body
//   2.8  (cut) it comes back across the floor to the body, kneels
//   6.4  it takes the pieces off it by hand, one at a time: reaches in, tears
//        each one free, carries it to itself and seats it (the shoulder plate,
//        the forearm housing, the spine coupling); its spine lights with each
//  16.8  STATUS: REJECTED -> COMPLETE. It stands
//  18.8  the new forearm opens: a splinter of glass, once
//  20.6  (cut) out along the platform to its end, the portal over it
//  25.8  at the edge it stops and looks back
//  28.4  the portal takes it: up off the edge and into the vortex. White
//
// Hold Jump to skip (after letting go first).
// =============================================================================
#include <algorithm>
#include <cmath>

struct LabEnding {
    static constexpr float DOOR1 = 2.8f, ARRIVE0 = 2.8f, KNEEL0 = 5.f, KNEEL1 = 6.2f;
    static constexpr float PART0 = 6.4f, PART_LEN = 3.4f;
    static constexpr float REACH = 0.9f, TEAR = 0.6f, CARRY = 1.2f;   // within a part: reach, tear it free, carry it home (then it settles)
    static constexpr float COMPLETE = 16.8f, STAND0 = 17.4f, STAND1 = 18.8f, SPLINTER0 = 18.8f, SPLINTER1 = 20.4f;
    static constexpr float WALK0 = 20.6f, EDGE = 25.6f, LOOK0 = 25.8f, LOOK1 = 28.f, PULL0 = 28.4f, THROUGH = 31.4f;
    static constexpr float DURATION = 32.4f, SKIP_HOLD = 0.65f;
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

    // Which shot the camera is in (the game frames each)
    enum Shot { DOORWAY, CROSSING, KNEEL, PART, RISE, SPLINTER, PLATFORM, LOOKBACK, PORTAL };
    Shot shot() const {
        const float t = time;
        if (t < DOOR1) return DOORWAY;
        if (t < KNEEL0) return CROSSING;
        if (t < PART0) return KNEEL;
        if (t < STAND0 - 0.6f) return PART;
        if (t < SPLINTER0) return RISE;
        if (t < WALK0) return SPLINTER;
        if (t < LOOK0) return PLATFORM;
        if (t < PULL0) return LOOKBACK;
        return PORTAL;
    }
    float shotT() const {   // seconds into this shot
        static const float S[9] = {0.f, KNEEL0 - 2.2f, KNEEL0, PART0, STAND0 - 0.6f, SPLINTER0, WALK0, LOOK0, PULL0};
        return time - S[(int)shot()];
    }

    // ---- UNIT 24 ----
    float kneel() const { return sm(KNEEL0, KNEEL1, time) * (1.f - sm(STAND0, STAND1, time)); }
    // The walk back across the floor (0..1, the last few metres of it: the cut takes the rest)
    float approach() const { return sm(ARRIVE0, KNEEL0, time); }
    float walking() const { return (time > ARRIVE0 && time < KNEEL0) || (time > WALK0 && time < EDGE) ? 1.f : 0.f; }
    float platformU() const { return sm(WALK0, EDGE, time); }   // along the platform to its edge
    float lookBack() const { return (time < DOOR1 ? sm(0.9f, 1.6f, time) * (1.f - sm(2.3f, 2.8f, time)) : 0.f) + sm(LOOK0, LOOK0 + 0.7f, time) * (1.f - sm(LOOK1 - 0.4f, LOOK1, time)); }
    float pull() const { float u = std::clamp((time - PULL0) / (THROUGH - PULL0), 0.f, 1.f); return u * u * u; }   // taken: slow, then fast
    // A piece (0 the shoulder plate, 1 the forearm housing, 2 the spine coupling): where it is in being taken
    float partStart(int k) const { return PART0 + k * PART_LEN; }
    float reach(int k) const {   // the right hand going to it, and coming back with it
        const float s = partStart(k);
        return sm(s, s + REACH, time) * (1.f - sm(s + REACH + TEAR, s + REACH + TEAR + CARRY, time));
    }
    float tear(int k) const { const float s = partStart(k) + REACH; return sm(s, s + TEAR, time); }        // 0 on the body .. 1 free, in the hand
    float carry(int k) const { const float s = partStart(k) + REACH + TEAR; return sm(s, s + CARRY, time); } // 0 in the hand .. 1 seated
    bool  taken(int k) const { return time >= partStart(k) + REACH; }
    bool  seated(int k) const { return time >= partStart(k) + REACH + TEAR + CARRY; }
    float seatT(int k) const { return partStart(k) + REACH + TEAR + CARRY; }
    float tearT(int k) const { return partStart(k) + REACH + TEAR * 0.6f; }
    float spine() const { return ((seated(0) ? 1.f : 0.f) + (seated(1) ? 1.f : 0.f) + (seated(2) ? 1.f : 0.f)) / 3.f; }
    bool  complete() const {   // the nameplate: REJECTED, a flicker, COMPLETE
        if (time < COMPLETE) return false;
        if (time > COMPLETE + 0.8f) return true;
        return std::fmod(time * 9.f, 1.f) > 0.5f;
    }
    float nameplate() const { return sm(KNEEL1, KNEEL1 + 0.8f, time) * (1.f - sm(STAND1 + 0.4f, STAND1 + 1.f, time)); }
    float splinter() const { return sm(SPLINTER0, SPLINTER0 + 0.5f, time) * (1.f - sm(SPLINTER1 - 0.4f, SPLINTER1, time)); }   // the forearm open, the shard out

    float white() const { return sm(THROUGH - 0.8f, DURATION - 0.2f, time); }
    float bars() const { return sm(0.f, 0.5f, time); }
};
