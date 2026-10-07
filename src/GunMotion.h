#pragma once
// =============================================================================
// GunMotion.h — how every gun moves. One grammar for the four, each gun
// supplying a weight row (how hard it kicks, how fast it settles, how far it
// rolls). No OpenGL: ViewModel reads pose() and plays nothing itself; the
// game drains `cues` and plays them, so the sounds land on the motion's beats.
//
//   FIRE     an instant kick back and up, then a critically damped settle
//   CYCLE    the pump / bolt after a shot (its sound on the beat)
//   SWITCH   the old gun drops out (0.12 s), the new one rises with a small
//            overshoot (0.2 s) while its cells boot up in sequence, a chime
//   RELOAD   three beats: OPEN (0-25 %) tilt in, action opens; FEED (25-85 %)
//            the cells relight one by one, a tick each; CLOSE (85-100 %) snap
//   INSPECT  turn it to show its side, run the cells
//   + idle breathing sway, a landing dip, a tilt on a dash, a dip on a slide
// =============================================================================
#include <glm/glm.hpp>
#include <algorithm>
#include <cmath>
#include <vector>

struct GunWeight { float kickBack, kickUp, pitchKick, roll, settle, swayAmp, reloadRoll; };
inline const GunWeight& gunWeight(int gun) {
    static const GunWeight W[4] = {
        //  back    up      pitch  roll  settle  sway  reloadRoll
        {0.045f, 0.030f,  4.f,  1.f,  0.12f, 1.0f, 32.f},   // revolver: light, snappy
        {0.070f, 0.026f,  7.f,  4.f,  0.30f, 1.3f, 55.f},   // shotgun:  heavy, short
        {0.080f, 0.020f,  6.f,  2.f,  0.26f, 1.1f, 14.f},   // lancer:   medium, long
        {0.110f, 0.030f, 10.f,  5.f,  0.45f, 1.5f, 18.f},   // longshot: heavy, long
    };
    return W[std::clamp(gun, 0, 3)];
}
constexpr float BEAT_OPEN = 0.25f, BEAT_FEED_END = 0.85f;
constexpr float SWITCH_DOWN = 0.12f, SWITCH_UP = 0.20f;

struct GunCue { const char* name; int volume; };

struct GunPose {
    glm::vec3 offset{0.f};            // camera space (right, up, forward)
    float pitch = 0.f, yaw = 0.f, roll = 0.f;   // degrees
    float reloadU = -1.f;             // 0..1 through a reload, -1: none
    float cycleU = -1.f;              // 0..1 through a pump / bolt, -1: none
    float fireU = 0.f;                // the kick: 1 as it fires .. 0 settled
    float boot = 1.f;                 // 0..1 cells booting after a switch (1: all on)
    float inspectU = -1.f;
    bool  hidden = false;             // all the way down mid-switch
};

class GunMotion {
public:
    int gun = 0;
    std::vector<GunCue> cues;

    void fire() { kickAge = 0.f; }
    void cycle(float duration) { cycleT = 0.f; cycleLen = std::max(0.15f, duration); cycleCued = false; }
    void reload(float total, int ammoBefore, int mag, int refill) {
        reloadT = 0.f; reloadLen = std::max(0.3f, total);
        rAmmo = ammoBefore; rMag = std::max(1, mag);
        rRefill = std::clamp(refill, 1, rMag);
        rWhole = gun == 0 || gun == 3;          // the revolver and the Longshot empty and refill everything
        if (rWhole) rRefill = rMag;
        cueAt = 0.f;
    }
    void switchTo(int g) {
        toGun = g; switchT = 0.f; switching = true; upCued = false;
        reloadT = -1.f; cycleT = -1.f; inspectT = -1.f;   // whatever the old gun was doing stops here
    }
    void inspect() { if (reloadT < 0.f && !switching) inspectT = 0.f; }
    void land(float fallSpeed) { landDip = std::max(landDip, std::min(1.f, fallSpeed / 20.f)); }
    void setMove(bool dashing, bool sliding) { dash = dashing; slide = sliding; }

    // Dev (screenshots): hold a reload / inspect / switch at u (call after update)
    void devReload(float u, int ammoBefore, int mag, int refill) { reload(1.f, ammoBefore, mag, refill); reloadT = u; cues.clear(); }
    void devInspect(float u) { inspectT = u * INSPECT_LEN; }
    void devSwitch(float u, int g) { gun = toGun = g; switching = true; upCued = true; switchT = SWITCH_DOWN + u * SWITCH_UP; cues.clear(); }

    bool reloading() const { return reloadT >= 0.f; }
    bool switchingNow() const { return switching; }
    int  shownGun() const { return switching && switchT < SWITCH_DOWN ? gun : (switching ? toGun : gun); }
    float kickAmount() const {   // critically damped: (1 + w t) e^-wt, ~2 % left at `settle`
        float w = 5.83f / gunWeight(gun).settle;
        float x = w * kickAge;
        return (1.f + x) * std::exp(-x);
    }

    void update(float dt) {
        clock += dt;
        kickAge += dt;
        landDip = std::max(0.f, landDip - dt * 3.3f);
        dashAmt += ((dash ? 1.f : 0.f) - dashAmt) * std::min(1.f, dt * 12.f);
        slideAmt += ((slide ? 1.f : 0.f) - slideAmt) * std::min(1.f, dt * 10.f);
        if (switching) {
            switchT += dt;
            if (switchT >= SWITCH_DOWN && !upCued) {   // it's down: the new gun comes up
                gun = toGun; upCued = true; kickAge = 10.f;
                cues.push_back({SWITCH_CUE[std::clamp(gun, 0, 3)], 90});
            }
            if (switchT >= SWITCH_DOWN + SWITCH_UP) switching = false;
        }
        if (cycleT >= 0.f) {
            float before = cycleT / cycleLen;
            cycleT += dt;
            float now = cycleT / cycleLen;
            if (!cycleCued) {
                float at = gun == 1 ? 0.f : 0.24f;
                if (before <= at && now >= at) { cues.push_back({gun == 1 ? "pump" : "bolt", gun == 1 ? 80 : 110}); cycleCued = true; }
            }
            if (cycleT >= cycleLen) cycleT = -1.f;
        }
        if (inspectT >= 0.f) { inspectT += dt; if (inspectT >= INSPECT_LEN) inspectT = -1.f; }
        if (reloadT >= 0.f) {
            float before = reloadT / reloadLen;
            reloadT += dt;
            float now = std::min(1.f, reloadT / reloadLen);
            reloadCues(before, now);
            if (reloadT >= reloadLen) reloadT = -1.f;
        }
    }

    // Cells to light now: during a reload, the beats decide; booting after a
    // switch, they come on one by one; otherwise the ammo the game reports
    int litCells(int ammo, int mag) const {
        mag = std::max(1, mag);
        int lit = std::clamp(ammo, 0, mag);
        if (reloadT >= 0.f) {
            float u = reloadT / reloadLen;
            int base = rWhole ? (u < 0.12f ? rAmmo : 0) : rAmmo;
            int fed = (int)std::floor(rRefill * seg(u, BEAT_OPEN, BEAT_FEED_END) + 1e-4f);
            lit = std::min(rMag, base + fed);
        }
        if (switching || bootT() < 1.f) lit = std::min(lit, (int)std::floor(mag * bootT() + 1e-4f));
        return lit;
    }

    GunPose pose(bool sway = true) const {
        GunPose p;
        const GunWeight& W = gunWeight(gun);
        float k = kickAmount();
        p.fireU = k;
        p.offset += glm::vec3{0.f, W.kickUp * k, -W.kickBack * k};
        p.pitch += W.pitchKick * k;
        p.roll  += W.roll * k;
        // Switch: down and out to the right, then up with a small overshoot
        if (switching) {
            if (switchT < SWITCH_DOWN) {
                float d = smooth(switchT / SWITCH_DOWN);
                p.offset += glm::vec3{0.05f * d, -0.26f * d, 0.f};
                p.roll += -12.f * d;
                p.hidden = d > 0.98f;
            } else {
                float u = std::min(1.f, (switchT - SWITCH_DOWN) / SWITCH_UP);
                float rise = backOut(u);                    // overshoots a touch, settles at 1
                p.offset += glm::vec3{0.02f * (1.f - u), -0.26f * (1.f - rise), 0.f};
                p.roll += 8.f * (1.f - u);
            }
        }
        p.boot = bootT();
        // Reload: OPEN tilt in, CLOSE snap back
        if (reloadT >= 0.f) {
            float u = std::min(1.f, reloadT / reloadLen);
            p.reloadU = u;
            float tilt = smooth(seg(u, 0.f, 0.12f)) * (1.f - smooth(seg(u, BEAT_FEED_END, 0.95f)));
            float snap = std::sin(seg(u, BEAT_FEED_END, 1.f) * 3.14159265f);
            p.roll  += -tilt * W.reloadRoll + snap * 6.f;
            p.pitch += tilt * 8.f;
            p.yaw   += -tilt * 14.f;
            p.offset += glm::vec3{-0.06f * tilt, 0.04f * tilt, -0.02f * snap};
        }
        if (cycleT >= 0.f) {
            p.cycleU = cycleT / cycleLen;
            float w = std::sin(p.cycleU * 3.14159265f);
            p.roll += w * (gun == 1 ? 4.f : 9.f);
            p.offset.y -= w * 0.015f;
        }
        if (inspectT >= 0.f) {
            float u = inspectT / INSPECT_LEN;
            p.inspectU = u;
            float k2 = smooth(seg(u, 0.f, 0.22f)) * (1.f - smooth(seg(u, 0.8f, 1.f)));
            float tilt = std::sin(seg(u, 0.3f, 0.75f) * 3.14159265f);
            p.yaw += k2 * 62.f; p.roll += -tilt * 25.f * k2; p.pitch += k2 * 6.f + tilt * 10.f;
            p.offset += glm::vec3{-0.11f * k2, 0.06f * k2, 0.05f * k2};
        }
        // Breathing sway, a landing dip, a dash tilt, a slide dip
        if (sway) p.offset += glm::vec3{std::sin(clock * 1.1f) * 0.003f, std::sin(clock * 0.65f) * 0.002f, 0.f} * W.swayAmp;
        p.offset.y -= 0.045f * landDip;
        p.roll += 6.f * dashAmt + 10.f * slideAmt;
        p.offset.y -= 0.03f * slideAmt;
        return p;
    }

private:
    static constexpr float INSPECT_LEN = 2.2f;
    static constexpr const char* SWITCH_CUE[4] = {"switch_up0", "switch_up1", "switch_up2", "switch_up3"};
    float clock = 0.f, kickAge = 10.f, landDip = 0.f, dashAmt = 0.f, slideAmt = 0.f;
    bool  dash = false, slide = false;
    bool  switching = false, upCued = false; int toGun = 0; float switchT = 0.f;
    float cycleT = -1.f, cycleLen = 0.5f; bool cycleCued = false;
    float inspectT = -1.f;
    float reloadT = -1.f, reloadLen = 1.f, cueAt = 0.f;
    int   rAmmo = 0, rMag = 1, rRefill = 1; bool rWhole = false;

    static float seg(float u, float a, float b) { return std::clamp((u - a) / (b - a), 0.f, 1.f); }
    static float smooth(float t) { return t * t * (3.f - 2.f * t); }
    static float backOut(float t) { const float c = 1.4f; t -= 1.f; return 1.f + (c + 1.f) * t * t * t + c * t * t; }
    float bootT() const {
        if (!switching) return 1.f;
        if (switchT < SWITCH_DOWN) return 1.f;   // the old gun, as it was
        return std::min(1.f, (switchT - SWITCH_DOWN) / SWITCH_UP);
    }
    void cue(float before, float now, float at, const char* name, int vol) { if (before < at && now >= at) cues.push_back({name, vol}); }
    void reloadCues(float b, float n) {
        static const char* OPEN[4]  = {"cyl_open", "reload", "cyl_open", "eject"};
        static const char* CLOSE[4] = {"cyl_close", "pump", "bolt", "bolt"};
        cue(b, n, 0.02f, OPEN[gun], 110);
        if (gun == 0 || gun == 2) cue(b, n, 0.12f, "eject", 95);
        cue(b, n, BEAT_OPEN + 0.01f, "shell_in", 115);
        for (int i = 0; i < rRefill; ++i)                 // a tick as each cell relights
            cue(b, n, BEAT_OPEN + (BEAT_FEED_END - BEAT_OPEN) * (i + 1.f) / rRefill - 1e-4f, "cell", 55);
        cue(b, n, BEAT_FEED_END + 0.01f, CLOSE[gun], 120);
    }
};
