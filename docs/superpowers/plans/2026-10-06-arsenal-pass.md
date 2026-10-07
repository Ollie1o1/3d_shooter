# The Arsenal Pass Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Make the four guns (and the fist) one arsenal: one look with a glow colour per gun and ammo cells you can read, one motion grammar, one family of layered gunshots and mechanical sounds; rename the Kar98 the LANCER.

**Architecture:** Two new GL-free headers carry the rules so tests can check them: `src/GunMotion.h` (weights, kick spring, switch, the three reload beats, sway/landing, and timed sound cues) and `src/GunKit.h` (materials, glow table, cell builder, and each gun's recipe → a list of boxes in gun space). `src/ViewModel.h` keeps its public trigger API but becomes framing + "ask GunMotion for the pose, ask GunKit for the parts, draw them". GameplayState plays GunMotion's cues instead of its hand-placed reload cue table. A new `tools/gen_arsenal.py` builds the sounds.

**Tech Stack:** C++17, OpenGL 3.3 / WebGL2, glm, Python 3 (stdlib only) for sound generation, `tests/test_game.cpp`.

**Spec:** `docs/superpowers/specs/2026-10-06-arsenal-pass-design.md`

## Global Constraints

- Glow (emissive): revolver amber (1.0, 0.68, 0.22); shotgun ember (1.0, 0.28, 0.10); Lancer cyan (0.30, 0.90, 1.00); Longshot violet (0.70, 0.35, 1.00).
- Materials: blackened gunmetal (0.10, 0.10, 0.11); worn steel (0.32, 0.32, 0.34); edge steel (0.46, 0.46, 0.49); bone (0.76, 0.71, 0.61); brass only for small details (0.70, 0.52, 0.24).
- One ammo cell per round of the current magazine size (upgrades add cells); lit = rounds left.
- Reload beats: OPEN 0–25 %, FEED 25–85 % (cells relight evenly), CLOSE 85–100 %.
- Switch: down 0.12 s + up 0.20 s (≈0.32 s), cells boot in sequence, a chime in the gun's pitch.
- Gameplay, stats, upgrades and balance unchanged. `WeaponId::KAR` stays as the internal id; the shown name is "LANCER".
- No GLSL changes. `make web` must build. New headers go in the Makefile `HEADERS` (and `test:` deps when tests include them).
- Commit messages: no Co-Authored-By / Claude trailer. `$SCRATCH` = the session scratchpad.

## Review Focus

1. **Switching guns mid-reload, mid-bolt, or twice quickly** → the new gun comes up clean (no reload pose carried over, no stale cues); a second switch during the first restarts cleanly. Test in Task 1.
2. **Magazine upgrades bigger than the cell layout** (revolver 8 → 14 with upgrades, shotgun 2 → 5) → every round still has its own cell, laid out without overlapping the gun body badly. Test in Task 2 (cells == mag for mag 1…16).
3. **Firing while empty / holding fire during a reload** → a dry click, rate-limited (not a click every frame). Task 4 (game wiring; by inspection + audiodump).
4. **Aiming the rifles** → iron sights / scope still sit on the crosshair (KAR_SIGHT_Y / LONG_SCOPE_Y unchanged relationships). Screenshot check in Task 6; framing test excludes the aimed pose.
5. **Reload interrupted** (switch away, die, retry) → cues stop, the pose returns to rest, nothing keeps ticking. Test in Task 1 (`switchTo` cancels the reload state and its cues).

---

### Task 1: GunMotion — one motion grammar

**Files:**
- Create: `src/GunMotion.h`
- Modify: `tests/test_game.cpp`, `Makefile`

**Interfaces:**
- Produces:

```cpp
struct GunWeight { float kickBack, kickUp, pitchKick, roll, settle, swayAmp, reloadRoll; };
const GunWeight& gunWeight(int gun);                 // 0 revolver, 1 shotgun, 2 lancer, 3 longshot
constexpr float BEAT_OPEN = 0.25f, BEAT_FEED_END = 0.85f, SWITCH_DOWN = 0.12f, SWITCH_UP = 0.20f;
struct GunCue  { const char* name; int volume; };
struct GunPose { glm::vec3 offset; float pitch, yaw, roll, reloadU, cycleU, fireU, boot, inspectU; bool hidden; };
class GunMotion {
public:
    int gun = 0;
    std::vector<GunCue> cues;                         // appended by update(); the game drains it
    void fire();
    void cycle(float duration);                       // pump / bolt after a shot
    void reload(float total, int ammoBefore, int mag, int refill);
    void switchTo(int g);
    void inspect();
    void land(float fallSpeed);
    void setMove(bool dashing, bool sliding);
    void update(float dt);
    GunPose pose(bool sway = true) const;            // sway=false: without the idle breathing (tests)
    int  shownGun() const;                            // the gun to draw (the old one until it's down)
    bool switchingNow() const;
    int  litCells(int ammo, int mag) const;           // cells to light now (reload / boot aware)
    bool reloading() const;
    float kickAmount() const;                         // 1 the instant it fires .. 0 settled
};
```

- [ ] **Step 1: Write the failing tests**

Add `#include "../src/GunMotion.h"` to the test includes and, before the mouse-filter block:

```cpp
    // ---------------------------------------------------------------- the arsenal: one motion grammar
    {
        // Kick: instant, then settles within each gun's `settle`
        bool settles = true;
        for (int g = 0; g < 4; ++g) {
            GunMotion m; m.gun = g; m.fire();
            float at0 = m.kickAmount();
            float t = 0.f;
            while (t < gunWeight(g).settle) { m.update(DT * 0.25f); t += DT * 0.25f; }
            settles &= at0 > 0.99f && m.kickAmount() < 0.03f;
        }
        CHECK(settles, "every gun kicks at once and settles within its weight's time");
        CHECK(gunWeight(0).settle < gunWeight(1).settle && gunWeight(1).settle <= gunWeight(3).settle &&
              gunWeight(2).settle < gunWeight(3).settle, "light guns snap back sooner than heavy ones");
        // Switch: the old gun until it's down, the new one rising, ~0.32 s, one chime
        GunMotion s; s.gun = 0;
        s.switchTo(2);
        float t = 0.f; bool oldFirst = true; int chimes = 0; float done = -1.f;
        for (int i = 0; i < 240 && done < 0.f; ++i) {
            s.update(DT * 0.25f); t += DT * 0.25f;
            if (t < SWITCH_DOWN - 0.005f) oldFirst &= s.shownGun() == 0;
            for (auto& c : s.cues) chimes += std::string(c.name) == "switch_up2";
            s.cues.clear();
            GunPose p = s.pose(false);
            if (t > SWITCH_DOWN + SWITCH_UP && glm::length(p.offset) < 1e-3f) done = t;
        }
        CHECK(oldFirst && s.shownGun() == 2 && chimes == 1 && done > 0.3f && done < 0.36f,
              "a switch lowers the old gun, raises the new one with one chime, in about 0.32 s");
        // Reload: three beats; cells go dark on OPEN (revolver ejects), relight evenly during FEED, all lit at CLOSE
        GunMotion r; r.gun = 0; r.reload(1.2f, 0, 8, 8);
        std::vector<float> relit; int lastLit = r.litCells(0, 8), maxLit = 0; float tt = 0.f;
        std::vector<std::string> order;
        while (r.reloading()) {
            r.update(DT * 0.25f); tt += DT * 0.25f;
            int lit = r.litCells(0, 8);
            if (lit > lastLit) relit.push_back(tt / 1.2f);
            lastLit = lit; maxLit = std::max(maxLit, lit);
            for (auto& c : r.cues) order.push_back(c.name);
            r.cues.clear();
        }
        bool even = relit.size() == 8 && relit.front() >= BEAT_OPEN - 0.01f && relit.back() <= BEAT_FEED_END + 0.01f;
        for (size_t i = 2; i < relit.size() && even; ++i)
            even &= std::fabs((relit[i] - relit[i - 1]) - (relit[1] - relit[0])) < 0.02f;
        CHECK(even, "a reload relights its cells one by one, evenly, inside the FEED beat");
        CHECK(maxLit == 8 && r.litCells(0, 8) == 0, "all eight relight; after the reload the gun shows what the game says (ammo)");
        int opens = 0, closes = 0, cells = 0; bool inOrder = !order.empty() && order.front() == "cyl_open" && order.back() == "cyl_close";
        for (auto& n : order) { opens += n == "cyl_open"; closes += n == "cyl_close"; cells += n == "cell"; }
        CHECK(inOrder && opens == 1 && closes == 1 && cells == 8, "reload cues: open first, a tick per cell, close last, each once");
        // A switch cancels a reload: no more cues, back to rest
        GunMotion c; c.gun = 1; c.reload(1.4f, 0, 2, 2);
        for (int i = 0; i < 20; ++i) c.update(DT);
        c.cues.clear(); c.switchTo(3);
        bool quiet = true;
        for (int i = 0; i < 60; ++i) { c.update(DT); for (auto& q : c.cues) quiet &= std::string(q.name) == "switch_up3"; c.cues.clear(); }
        CHECK(quiet && !c.reloading() && c.pose().reloadU < 0.f, "switching mid-reload drops the reload and its sounds");
        // A second switch mid-switch restarts cleanly to the newest gun
        GunMotion d; d.gun = 0; d.switchTo(1);
        for (int i = 0; i < 4; ++i) d.update(DT);
        d.switchTo(2);
        for (int i = 0; i < 40; ++i) d.update(DT);
        CHECK(d.shownGun() == 2 && glm::length(d.pose(false).offset) < 1e-3f, "switching again mid-switch lands on the newest gun");
        // Cycling cues: the shotgun pumps, the rifles work the bolt
        GunMotion b; b.gun = 2; b.cycle(0.7f);
        int bolts = 0; for (int i = 0; i < 60; ++i) { b.update(DT); for (auto& q : b.cues) bolts += std::string(q.name) == "bolt"; b.cues.clear(); }
        CHECK(bolts == 1, "after a rifle shot the bolt sounds once, on its beat");
    }
```

Add `src/GunMotion.h` to the Makefile `HEADERS` and `test:` deps.

- [ ] **Step 2: Run to verify it fails**

Run: `make test 2>&1 | grep error | head -3`
Expected: `GunMotion.h` not found.

- [ ] **Step 3: Implement `src/GunMotion.h`**

```cpp
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
```

(The per-cell tick time equals the moment `litCells` increments: cell `i+1` lights at `BEAT_OPEN + span·(i+1)/n`.)

- [ ] **Step 4: Run to verify it passes**

Run: `make test 2>&1 | grep -E "FAIL|ALL PASSED|FAILED"`
Expected: `ALL PASSED`. Tune only the weight table if `settles`/ordering is marginal; never the beat constants.

- [ ] **Step 5: Commit**

```bash
git add src/GunMotion.h tests/test_game.cpp Makefile
git commit -m "GunMotion: one motion grammar for every gun - kick, switch, three reload beats, timed cues"
```

---

### Task 2: GunKit — one look, a recipe per gun

**Files:**
- Create: `src/GunKit.h`
- Modify: `tests/test_game.cpp`, `Makefile`

**Interfaces:**
- Consumes: `GunPose` (Task 1) fields `reloadU`, `cycleU`, `fireU`, `boot`.
- Produces:

```cpp
struct GunPart { glm::mat4 xf; glm::vec3 color, emissive; int tag; };   // tag: 0 body, 1 lit cell, 2 dark cell
struct GunLook { int ammo = 8, mag = 8, lit = 8; float flash = 0.f, heat = 0.f, cylSpin = 0.f; int refill = 1; float aim = 0.f; };
namespace gunkit {
    glm::vec3 glowOf(int gun);
    void buildGun(int gun, const GunLook& look, const GunPose& pose, std::vector<GunPart>& out);   // gun space
    void buildFist(glm::vec3 glow, float punch, bool hit, std::vector<GunPart>& out);             // fist space
}
```

- [ ] **Step 1: Write the failing tests**

```cpp
    // ---------------------------------------------------------------- the arsenal: one look
    {
        GunPose rest;
        bool cellsOk = true, litOk = true, glowOk = true;
        for (int g = 0; g < 4; ++g)
            for (int mag : {1, 2, 4, 5, 8, 10, 14, 16})
                for (int ammo : {0, mag / 2, mag}) {
                    GunLook L; L.mag = mag; L.ammo = ammo; L.lit = ammo;
                    std::vector<GunPart> parts; gunkit::buildGun(g, L, rest, parts);
                    int lit = 0, dark = 0;
                    for (auto& p : parts) { lit += p.tag == 1; dark += p.tag == 2; }
                    if (lit + dark != mag) { std::printf("      gun %d mag %d: %d cells\n", g, mag, lit + dark); cellsOk = false; }
                    litOk &= lit == ammo;
                    if (ammo > 0) {
                        bool any = false;
                        for (auto& p : parts)
                            if (p.tag == 1) any |= glm::length(glm::normalize(p.emissive) - glm::normalize(gunkit::glowOf(g))) < 0.01f;
                        glowOk &= any;
                    }
                }
        CHECK(cellsOk, "every gun shows one cell per round of its magazine (1-16, upgrades included)");
        CHECK(litOk, "the lit cells are the rounds left");
        CHECK(glowOk, "each gun's cells glow its own colour");
        CHECK(glm::length(gunkit::glowOf(0) - glm::vec3{1.f, 0.68f, 0.22f}) < 1e-4f && glm::length(gunkit::glowOf(2) - glm::vec3{0.3f, 0.9f, 1.f}) < 1e-4f,
              "amber revolver, cyan Lancer");
        // Framing: at the hip, every gun's parts project into the lower-right of a 16:9 screen
        glm::mat4 proj = glm::perspective(glm::radians(65.f), 16.f / 9.f, 0.03f, 10.f);
        bool framed = true; float areas[4];
        for (int g = 0; g < 4; ++g) {
            GunLook L; std::vector<GunPart> parts; gunkit::buildGun(g, L, rest, parts);
            glm::vec3 hip = gunkit::hipOf(g);
            float x0 = 9, x1 = -9, y0 = 9, y1 = -9;
            for (auto& p : parts) {
                glm::vec3 c = glm::vec3(p.xf[3]) + hip;
                glm::vec4 clip = proj * glm::vec4(c.x, c.y, -c.z, 1.f);    // camera looks down -Z
                glm::vec2 ndc{clip.x / clip.w, clip.y / clip.w};
                x0 = std::min(x0, ndc.x); x1 = std::max(x1, ndc.x); y0 = std::min(y0, ndc.y); y1 = std::max(y1, ndc.y);
            }
            areas[g] = (std::min(x1, 1.f) - std::max(x0, -1.f)) * (std::min(y1, 0.3f) - std::max(y0, -1.f));
            if (x0 < -0.25f || y1 > 0.3f || x1 < 0.2f) { std::printf("      gun %d frame x %.2f..%.2f y %.2f..%.2f\n", g, x0, x1, y0, y1); framed = false; }
        }
        CHECK(framed, "at the hip every gun sits in the lower right, its muzzle toward the crosshair");
        bool similar = true;
        for (int g = 1; g < 4; ++g) similar &= areas[g] > areas[0] * 0.5f && areas[g] < areas[0] * 3.f;
        CHECK(similar, "the four guns fill a similar share of the screen");
        std::vector<GunPart> fist; gunkit::buildFist(gunkit::glowOf(1), 1.f, true, fist);
        bool seam = false; for (auto& p : fist) seam |= glm::length(p.emissive) > 0.1f;
        CHECK(fist.size() >= 8 && seam, "the gauntlet: a fist with a seam glowing in the current gun's colour");
        CHECK(std::string(weaponDef(WeaponId::KAR).name) == "LANCER" && weaponDef(WeaponId::KAR).damage == 120.f &&
              weaponDef(WeaponId::KAR).magSize == 5 && std::fabs(weaponDef(WeaponId::KAR).reloadTime - 2.0f) < 1e-4f,
              "the Kar98 is now the LANCER, with the same stats");
    }
```

(Add `gunkit::hipOf(int)` to the Interfaces: the shared hip, `glm::vec3{0.17f, -0.15f, 0.30f}` for every gun — one framing rule; each recipe positions its own model about its origin so its visual mass sits there.)

- [ ] **Step 2: Run to verify it fails**

Run: `make test 2>&1 | grep error | head -2`
Expected: `GunKit.h` not found.

- [ ] **Step 3: Implement the kit (`src/GunKit.h`)**

Framework (exact):

```cpp
#pragma once
// =============================================================================
// GunKit.h — the arsenal's look: one body for every gun (blackened gunmetal,
// worn steel, bone grips, a little brass), one glow colour per gun, ammo cells
// you can read on the gun, and each gun's recipe built from those parts. No
// OpenGL: buildGun() returns boxes in gun space (+X right, +Y up, +Z along the
// barrel) that ViewModel draws, so tests can count cells and check framing.
// =============================================================================
#include "GunMotion.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <cmath>
#include <algorithm>

struct GunPart { glm::mat4 xf; glm::vec3 color, emissive; int tag; };   // tag 1: lit cell, 2: dark cell
struct GunLook { int ammo = 8, mag = 8, lit = 8; float flash = 0.f, heat = 0.f, cylSpin = 0.f; int refill = 1; float aim = 0.f; };

namespace gunkit {
using glm::vec3; using glm::mat4;
const vec3 BLACK{0.10f, 0.10f, 0.11f}, STEEL{0.32f, 0.32f, 0.34f}, EDGE{0.46f, 0.46f, 0.49f},
           BONE{0.76f, 0.71f, 0.61f}, BRASS{0.70f, 0.52f, 0.24f};
inline vec3 glowOf(int gun) {
    static const vec3 G[4] = {{1.f, 0.68f, 0.22f}, {1.f, 0.28f, 0.10f}, {0.30f, 0.90f, 1.f}, {0.70f, 0.35f, 1.f}};
    return G[std::clamp(gun, 0, 3)];
}
inline vec3 hipOf(int) { return {0.17f, -0.15f, 0.30f}; }   // one framing rule for every gun
inline mat4 T(vec3 v) { return glm::translate(mat4(1.f), v); }
inline mat4 R(float deg, vec3 axis) { return glm::rotate(mat4(1.f), glm::radians(deg), axis); }
inline float seg(float u, float a, float b) { return std::clamp((u - a) / (b - a), 0.f, 1.f); }
inline float smooth(float t) { return t * t * (3.f - 2.f * t); }

struct Builder {
    std::vector<GunPart>& out;
    vec3 glow;
    vec3 emi{0.f};
    void box(const mat4& base, vec3 c, vec3 size, vec3 col) { out.push_back({base * T(c) * glm::scale(mat4(1.f), size), col, emi, 0}); }
    void lit(vec3 e) { emi = e; }
    void unlit() { emi = vec3{0.f}; }
    // n ammo cells laid out from `start` stepping `step` (wrapping into rows of
    // `perRow` along `rowStep`), the first `on` lit
    void cells(const mat4& base, vec3 start, vec3 step, vec3 rowStep, int perRow, vec3 size, int n, int on, float boost = 1.f) {
        for (int i = 0; i < n; ++i) {
            vec3 c = start + step * (float)(i % perRow) + rowStep * (float)(i / perRow);
            bool l = i < on;
            out.push_back({base * T(c) * glm::scale(mat4(1.f), size), l ? glow * 0.5f : BLACK * 0.6f,
                           l ? glow * (1.6f * boost) : vec3{0.f}, l ? 1 : 2});
        }
    }
    // The accent strip: brighter the fuller the gun, flaring as it fires
    vec3 accent(const GunLook& L) const {
        float fill = L.mag > 0 ? (float)L.ammo / L.mag : 0.f;
        float k = L.ammo == 0 ? 0.15f : 0.35f + 0.65f * fill;
        return glow * (k + 1.5f * L.flash);
    }
    void muzzleFlash(const mat4& base, vec3 at, float size, float flash) {
        if (flash <= 0.f) return;
        vec3 hot = glm::mix(glow, vec3{1.f}, 0.45f);
        lit(glow * flash * 1.8f);
        box(base, at, vec3{size, size, size * 1.4f} * (0.6f + 0.4f * flash), hot);
        box(base, at + vec3{0.f, 0.f, size * 0.6f}, vec3{size * 0.45f, size * 0.45f, size * 1.6f} * flash, hot);
        unlit();
    }
};

void buildRevolver(Builder& b, const GunLook& L, const GunPose& P);
void buildShotgun (Builder& b, const GunLook& L, const GunPose& P);
void buildLancer  (Builder& b, const GunLook& L, const GunPose& P);
void buildLongshot(Builder& b, const GunLook& L, const GunPose& P);

inline void buildGun(int gun, const GunLook& L, const GunPose& P, std::vector<GunPart>& out) {
    Builder b{out, glowOf(gun)};
    switch (gun) {
        case 0: buildRevolver(b, L, P); break;
        case 1: buildShotgun(b, L, P);  break;
        case 2: buildLancer(b, L, P);   break;
        default: buildLongshot(b, L, P); break;
    }
}
} // namespace gunkit
```

**The recipes** — port each existing `ViewModel::draw*` body into `inline void gunkit::build*(Builder& b, const GunLook& L, const GunPose& P)` (define them after the namespace block, `inline` so the header links once) with these mechanical rules, then the restyle rules:

- `drawBox(shader, base, c, s, col)` → `b.box(base, c, s, col)`; `glow(shader, e)` / `shader.setVec3("emissiveColor", e)` → `b.lit(e)`; `glow(shader, {})` → `b.unlit()`. `base` is `mat4(1.f)` (gun space); `reloadU` → `P.reloadU`; the FIRE-anim hammer fall `animTimer/animMax` → `P.fireU`; `heat` → `L.heat`; `cylSpin` → `L.cylSpin`; the shotgun pump slide reads `P.cycleU` (back over the first half, forward over the second) and, in a reload, the CLOSE beat; the rifles' `boltLift/boltPull` come from `P.cycleU` with the old BOLT curves (`lift = seg(u,0,.2) - seg(u,.8,1)`, `pull = seg(u,.2,.45) - seg(u,.55,.8)`) and, in a reload, OPEN (lift/pull over 0.03–0.15) and CLOSE (home over 0.86–0.97).
- The old per-gun reload timelines are remapped onto the beats: revolver swing-out 0.03–0.12, brass 0.10–0.30, speedloader in 0.25–0.45 (rounds seat at 0.45), loader away 0.55–0.7, spin shut 0.86–0.97; shotgun shells fed evenly across FEED (shell `i` of `L.refill` over `[0.25 + i·span, 0.25 + (i + 0.85)·span]`, `span = 0.6 / refill`), rack in CLOSE; Lancer bolt open in OPEN, the empty 0.10–0.24, clip in 0.25–0.38, rounds pressed 0.40–0.80, clip flicked 0.80–0.86, bolt home in CLOSE; Longshot magazine out 0.05–0.22 (a box dropping from under the receiver), new magazine up 0.25–0.45.
- **Materials** (restyle): every body colour becomes `BLACK` (frames, stocks, receivers, furniture), `STEEL` (barrels, bands, bolt, rings), `EDGE` (sights, vent rib, flats, ejector), `BONE` (grip panels, the shotgun's pump grip, the Lancer's wrist and cheek panel), `BRASS` (cartridge heads, the revolver's medallion only). The revolver's oxblood grip → `BONE`; all wood → `BLACK` with `BONE` grip panels; the Longshot's olive → `BLACK`, its poly → `STEEL`.
- **Glow**: every orange/amber emissive becomes `b.glow`-based: accent strips use `b.accent(L)`; the old fixed muzzle flash → `b.muzzleFlash(...)`; the Longshot's lenses `b.lit(b.glow * 0.4f)`.
- **Cells** replace or add the ammo readout, using `b.cells(...)` with `n = L.mag, on = L.lit, boost = 0.4f + 0.6f * P.boot`:
  - Revolver: the chamber faces on the cylinder: for `L.mag <= 8` keep the 8-slot ring but only `L.mag` cells exist (positions `k * 360/mag`); above 8, a second inner ring at radius 0.011 for cells 8+. Implement with a loop calling `b.out.push_back` the same way `cells` does (tag 1/2), so the count rule holds.
  - Shotgun: a shell window on the right side of the receiver: `cells(base, {0.034f, 0.012f, -0.09f}, {0.f, 0.f, 0.024f}, {0.f, -0.02f, 0.f}, 5, {0.006f, 0.016f, 0.018f}, L.mag, L.lit)`.
  - Lancer: a strip along the receiver's right side: `cells(base, {0.0215f, 0.026f, -0.03f}, {0.f, 0.f, 0.022f}, {0.f, -0.012f, 0.f}, 8, {0.004f, 0.008f, 0.016f}, L.mag, L.lit)`.
  - Longshot: a row on the scope's left side: `cells(base, {-0.0235f, LONG_SCOPE_Y, -0.04f}, {0.f, 0.f, 0.028f}, {0.f, -0.012f, 0.f}, 8, {0.004f, 0.01f, 0.02f}, L.mag, L.lit)`.
- **Accent strips** (new, one per gun, `b.lit(b.accent(L))`): revolver keeps its side inlays and heat stripes; shotgun a strip along the top of the barrel `{0.f, 0.053f, 0.12f}` size `{0.006f, 0.003f, 0.30f}`; Lancer a strip along the forend's right `{0.0225f, -0.004f, 0.25f}` size `{0.002f, 0.006f, 0.30f}`; Longshot the barrel flute `{0.f, 0.044f, 0.30f}` size `{0.006f, 0.003f, 0.30f}`.
- **Framing**: each recipe ends by offsetting nothing — instead pick its gun-space origin so its visual mass sits at the shared hip. Move the shotgun model up `+0.035f` and back `-0.02f` (it hid below the screen), the Longshot down `-0.01f`. The Lancer keeps `KAR_SIGHT_Y = 0.052f` and the Longshot `LONG_SCOPE_Y = 0.098f` (move them into `gunkit` as `LANCER_SIGHT_Y` / `LONG_SCOPE_Y`; `ViewModel` aims with them). If the framing test fails, adjust these per-recipe origin offsets, never the test.
- `buildFist(glow, punch, hit, out)`: port `ViewModel::drawFist`'s boxes (in fist space, without the camera placement) with glove → `BLACK`, plate → `STEEL`, knuckles → `BONE`, sleeve → `BLACK * 1.3f`, plus a seam: `b.lit(glow * (0.8f + (hit ? 2.f * punch : 0.f)))` on a thin box along the back of the hand `{0.f, 0.03f, -0.02f}` size `{0.06f, 0.004f, 0.09f}`.

- [ ] **Step 4: Run to verify it passes**

Run: `make test 2>&1 | grep -E "FAIL|ALL PASSED|FAILED"`
Expected: `ALL PASSED` — except the LANCER line, which needs Task 4's rename. Do the rename now (one line in `Weapons.h`: `"KAR98"` → `"LANCER"`) so this task ends green.

- [ ] **Step 5: Commit**

```bash
git add src/GunKit.h src/Weapons.h tests/test_game.cpp Makefile
git commit -m "GunKit: one body, a glow per gun, ammo cells on every gun, the Kar98 becomes the Lancer"
```

---

### Task 3: The view model on the kit and the grammar

**Files:**
- Modify: `src/ViewModel.h` (rewrite), `src/Gameplay_Render.h`, `src/Gameplay_Combat.h`, `src/Gameplay_Menus.h`, `src/Gameplay_Flow.h`, `src/Gameplay_Tick.h`, `src/GameplayState.h`

**Interfaces:**
- Consumes: `GunMotion` (Task 1), `gunkit` (Task 2).
- Produces: `ViewModel` keeps `update(dt, xzSpeed, onGround)`, `getBobOffset(...)`, `draw(shader, cam, activeWeapon, flash, aim, ammo, mag)`, `triggerFire()`, `triggerReload(t, shells)`, `triggerBolt(t)`, `triggerPump()`, `triggerInspect()`, `triggerGrenade()`, `triggerGrapple()`, `triggerParry(hit)`, `PARRY_TIME`, `parryTimer`; **changes** `triggerSwitch()` → `triggerSwitch(int toGun)`; **adds** `GunMotion motion;`, `void setMove(bool dashing, bool sliding)`, `void land(float fallSpeed)`, `std::vector<GunCue> takeCues()`; `KAR_SIGHT_Y`/`LONG_SCOPE_Y` keep their names (aliases of the gunkit values). `KAR_CLIP0..KAR_FLICK1` and `GameplayState::reloadSounds()` go away.

No new unit tests: the rules live in Tasks 1–2; this task is wiring, verified by the build, the suite and screenshots.

- [ ] **Step 1: Rewrite `ViewModel.h`**

Keep the header comment (rewritten to describe: framing + GunMotion + GunKit), `buildCube`, `cubeMesh`, the bob, and replace the body of `draw` with:

```cpp
    void draw(ShaderProgram& shader, const Camera& cam, int activeWeapon, float flash, float aim = 0.f,
              int ammo = 8, int mag = 8) {
        ammoSeen = ammo; magSeen = std::max(1, mag);
        if (!motion.switchingNow() && motion.shownGun() != activeWeapon) motion.gun = activeWeapon;   // a start or retry on another gun
        int g = motion.shownGun();
        GunPose P = motion.pose();
        GunLook L;
        L.ammo = ammo; L.mag = std::max(1, mag); L.lit = motion.litCells(ammo, L.mag);
        L.flash = flash; L.heat = heat; L.cylSpin = cylSpin; L.refill = reloadShells; L.aim = aim;

        glm::vec3 fwd = cam.forward(), right = cam.right();
        glm::vec3 up = glm::normalize(glm::cross(right, fwd));
        float a = aim * aim * (3.f - 2.f * aim);
        glm::vec3 hip = gunkit::hipOf(g);
        glm::vec3 aimed = g == 2 ? glm::vec3{0.f, -gunkit::LANCER_SIGHT_Y, 0.30f}
                        : g == 3 ? glm::vec3{0.f, -gunkit::LONG_SCOPE_Y, 0.05f} : hip;
        glm::vec3 off = glm::mix(hip, aimed, a) + P.offset * (1.f - 0.6f * a);
        glm::mat4 base(1.f);
        base[0] = glm::vec4(right, 0.f); base[1] = glm::vec4(up, 0.f); base[2] = glm::vec4(fwd, 0.f);
        base[3] = glm::vec4(cam.position + right * off.x + up * off.y + fwd * off.z, 1.f);
        float yawIn = (g >= 2 ? (1.f - a) * 4.f : g == 0 ? -9.f : -3.f) + P.yaw;   // barrels converge on the crosshair
        base = base * glm::rotate(glm::mat4(1.f), glm::radians(yawIn), glm::vec3{0.f, 1.f, 0.f})
                    * glm::rotate(glm::mat4(1.f), glm::radians(P.pitch * (1.f - 0.6f * a)), glm::vec3{-1.f, 0.f, 0.f})
                    * glm::rotate(glm::mat4(1.f), glm::radians(P.roll * (1.f - 0.5f * a)), glm::vec3{0.f, 0.f, 1.f});

        glm::mat4 vmProj = glm::perspective(glm::radians(65.f), cam.aspectRatio, 0.03f, 10.f);
        glClear(GL_DEPTH_BUFFER_BIT);
        shader.setMat4("projection", vmProj);
        shader.setMat4("view", cam.viewMatrix());
        glDisable(GL_CULL_FACE);
        if (!P.hidden) {
            parts.clear();
            gunkit::buildGun(g, L, P, parts);
            for (const auto& p : parts) drawPart(shader, base * p.xf, p.color, p.emissive);
        }
        if (parryTimer > 0.f) drawFist(shader, cam, gunkit::glowOf(g));
        shader.setVec3("emissiveColor", {0.f, 0.f, 0.f});
        glEnable(GL_CULL_FACE);
    }
```

with `drawPart(shader, model, color, emissive)` setting `model`, `objectColor`, `emissiveColor` and drawing `cubeMesh`; `drawFist(shader, cam, glow)` keeping the old camera placement and drawing the parts from `gunkit::buildFist(glow, punchOut, parryHit, parts)` (`punchOut` = the old 0..1 punch extension); `std::vector<GunPart> parts;` as a member (no per-frame allocation after the first).

Triggers forward to the grammar:

```cpp
    GunMotion motion;
    void triggerFire()    { motion.fire(); heat = std::min(1.f, heat + 0.22f); cylSpin = 45.f; }
    void triggerReload(float t = 0.6f, int shells = 1) { reloadShells = std::max(1, shells); motion.reload(t, ammoSeen, magSeen, reloadShells); }
    void triggerBolt(float t) { motion.cycle(std::max(t, 0.2f)); }
    void triggerPump()        { motion.cycle(0.45f); }
    void triggerSwitch(int toGun) { motion.switchTo(toGun); }
    void triggerInspect()     { motion.inspect(); }
    void setMove(bool dashing, bool sliding) { motion.setMove(dashing, sliding); }
    void land(float fallSpeed) { motion.land(fallSpeed); }
    std::vector<GunCue> takeCues() { std::vector<GunCue> c; c.swap(motion.cues); return c; }
```

(`ammoSeen`/`magSeen` are set at the top of every `draw()` from its `ammo`/`mag` arguments, so a reload started between frames starts from what was on screen.) `update(dt, …)` calls `motion.update(dt)` and keeps decaying `heat`, `cylSpin`, `parryTimer`, the bob, and the grenade/grapple poses (kept as small extra offsets added to `P.offset` while their timers run).

Delete `drawRevolver`, `drawShotgun`, `drawKar`, `drawKarReload`, `drawLongshot`, `drawBoltHandle`, `muzzleFlash`, `drawGrenade`'s callers if unused (keep `drawGrenade` only if something draws it), and the KAR timeline constants.

- [ ] **Step 2: The game side**

- `Gameplay_Menus.h` `trySwitch`: `viewModel.triggerSwitch(w);` and drop the `audio.play("reload", 80, …)` (the switch chime comes from the cues).
- `Gameplay_Combat.h`: delete `reloadSounds()` and its declaration; in `startReload` drop `reloadCueAt` and the Longshot's `audio.play("reload")`, and pass the full reload time for every gun: `viewModel.triggerReload(weapons[w].reloadTotal, mag - weapons[w].ammo);`. In `fireWeapon`, drop `audio.play("pump", 70)` (the pump cue comes from `cycle`).
- `Gameplay_Flow.h` (or wherever `boltSoundTimer` is counted down): delete `boltSoundTimer` and its `audio.play("bolt", 110)`; `Gameplay_Tick.h`'s "rifles chamber a round when the reload finishes" bolt sound goes too (the CLOSE cue plays it).
- Each frame, after `viewModel.update(...)`: `for (auto& c : viewModel.takeCues()) audio.play(c.name, c.volume, SoundGroup::PLAYER);`
- Feed movement: `viewModel.setMove(player.dashing (or the dash timer > 0), player.sliding);` and, where `justLanded` is computed in `Gameplay_Tick.h`, `viewModel.land(-velocityBeforeLanding.y)` (use the existing fall-speed value there; if none, the landing squash amount × 20).
- `Gameplay_Render.h`: unchanged call; the view model reads the kit.

- [ ] **Step 3: Build, test, look**

Run: `make 2>&1 | grep -E " error|warning:" | grep -v duplicate; make test 2>&1 | tail -1`
Expected: clean; `ALL PASSED`.

```bash
for w in 1 2 3 4; do ./shooter --arena 1 --god --clean --weapon $w --res 720 --shot 60 $SCRATCH/arm$w.bmp; done
./shooter --arena 1 --god --clean --weapon 3 --aim --res 720 --shot 60 $SCRATCH/arm3aim.bmp
```
Convert with `sips` and look: one family, each in its colour, cells readable, the shotgun in view, the Lancer's sights on the crosshair when aimed. Fix recipe origins/colours until they read; ledger anything changed.

- [ ] **Step 4: Commit**

```bash
git add src/ViewModel.h src/Gameplay_*.h src/GameplayState.h
git commit -m "The view model on the kit and the grammar: one framing, one way of moving, sounds on the beats"
```

---

### Task 4: The gun's colour everywhere; the Lancer's name everywhere; the dry click

**Files:**
- Modify: `src/Effects.h`, `src/Gameplay_Render.h`, `src/Gameplay_Combat.h`, `src/UIRenderer.h`, `src/Gameplay_HUD.h`, `src/Daily.h`, `src/Weapons.h` (comments), `README.md`, `src/GameplayState.h`

- [ ] **Step 1: Tracers and shells take the gun's colour**

`Effects.h`: `Tracer` gains `glm::vec3 color{0.97f, 0.95f, 0.72f};`; `spawnTracer(start, end, width, life, glm::vec3 color = {0.97f, 0.95f, 0.72f})` stores it; `spawnShellCasing(origin, right, glm::vec3 glow = {0.85f, 0.7f, 0.15f})` colours the casing `glm::mix({0.7f, 0.52f, 0.24f}, glow, 0.5f)`.
`Gameplay_Render.h` `renderTracers`: group by colour (at most a handful of distinct colours) — collect beams per distinct colour and call `drawBeams(beams, colour, view, proj)` once per colour.
`Gameplay_Combat.h` `fireWeapon`: `glm::vec3 gc = gunkit::glowOf(w);` → `fx.spawnTracer(..., width, life, glm::mix(glm::vec3{1.f}, gc, 0.6f))`, `fx.spawnShellCasing(origin, right, gc)`, and on each hit `fx.spawnHitSparks(at, gc * 1.2f)` in addition to the enemy-coloured sparks.

- [ ] **Step 2: HUD slots**

`UIRenderer.h`: `HudWeapon` gains `glm::vec3 glow{1.f, 0.7f, 0.2f};`; the active slot's border uses `glm::vec4(glow, 0.95f)`, its slot number text and the reload bar use the glow too. `Gameplay_HUD.h` fills `glow = gunkit::glowOf(i)` for each slot.

- [ ] **Step 3: The name**

`Daily.h`: `"THE KAR98 ONLY"` → `"THE LANCER ONLY"`. Comments mentioning the Kar98 in `Weapons.h`, `Gameplay_Combat.h`, `Gameplay_Menus.h` → the Lancer. `README.md`: weapon list, controls table, DAILY modifiers, the weapon section — "Kar98" → "Lancer" (describe it as a dark machine bolt rifle with a cyan cell strip).

- [ ] **Step 4: The dry click**

`GameplayState.h`: `float dryCd = 0.f;`. Where a fire press is ignored because the active gun has no ammo (or is reloading), and `dryCd <= 0`: `audio.play("dry", 100, SoundGroup::PLAYER); dryCd = 0.3f;` (count `dryCd` down in the tick).

- [ ] **Step 5: Build, test, commit**

Run: `make 2>&1 | grep " error"; make test 2>&1 | tail -1`
Expected: clean; `ALL PASSED`.

```bash
git add src/*.h README.md
git commit -m "Each gun's colour in its tracers, shells, sparks and HUD slot; the Lancer by name; a dry click when empty"
```

---

### Task 5: One family of sounds

**Files:**
- Create: `tools/gen_arsenal.py`, `assets/sfx/src/revolver_rec.wav`, `shotgun_rec.wav`, `kar_rec.wav`, `longshot_rec.wav` (copies of today's files)
- Modify: `assets/sfx/*.wav` (generated), `src/main.cpp` (sound list), `assets/sfx/CREDITS.md`

- [ ] **Step 1: Freeze the recordings**

```bash
mkdir -p assets/sfx/src
for n in revolver shotgun kar longshot; do cp assets/sfx/$n.wav assets/sfx/src/${n}_rec.wav; done
```

- [ ] **Step 2: Write `tools/gen_arsenal.py`**

```python
#!/usr/bin/env python3
"""
The arsenal's sounds, built from one recipe so the four guns sound like one
family (the feel track's "dark machine + glow"):

  shot = crack (the real recording's first instant) + thump (a falling sine,
         deeper and longer the heavier the gun) + ring (a few damped
         inharmonic partials in the gun's own pitch: the family sound) + tail
         (filtered noise dying away; the Longshot's a long boom)

Three variants per gun (name.wav, name_1.wav, name_2.wav) - the sound engine
rotates them. Then the mechanical foley, one recipe pitched per mechanism:
cyl_open, cyl_close, eject, shell_in, pump, bolt, reload, cell, dry and a
switch_up chime per gun in its glow's pitch.

Reads the frozen recordings in assets/sfx/src/*_rec.wav (copied once from the
old shots, so re-running never stacks layers). Stdlib only.

    python3 tools/gen_arsenal.py
"""
import math, os, random, struct, wave

SR = 44100
HERE = os.path.dirname(os.path.abspath(__file__))
SFX = os.path.join(HERE, "..", "assets", "sfx")
SRC = os.path.join(SFX, "src")

def load(path):
    w = wave.open(path)
    assert w.getnchannels() == 1 and w.getsampwidth() == 2, path
    n = w.getnframes()
    x = [v / 32768.0 for v in struct.unpack("<%dh" % n, w.readframes(n))]
    if w.getframerate() != SR:   # resample (linear) if a source isn't 44.1 kHz
        r = w.getframerate() / SR
        x = [x[min(int(i * r), n - 1)] for i in range(int(n / r))]
    return x

def save(name, x, peak=0.9):
    m = max(1e-9, max(abs(v) for v in x))
    x = [v * peak / m for v in x]
    with wave.open(os.path.join(SFX, name + ".wav"), "wb") as w:
        w.setnchannels(1); w.setsampwidth(2); w.setframerate(SR)
        w.writeframes(b"".join(struct.pack("<h", int(max(-1, min(1, v)) * 32767)) for v in x))
    print(f"wrote {name}.wav ({len(x) / SR * 1000:.0f} ms)")

def n_of(sec): return int(sec * SR)

def mix(length, *parts):
    out = [0.0] * length
    for gain, start, x in parts:
        for i, v in enumerate(x):
            j = start + i
            if 0 <= j < length: out[j] += gain * v
    return out

def onepole_lp(x, hz):
    a = 1 - math.exp(-2 * math.pi * hz / SR); y = 0.0; out = []
    for v in x: y += a * (v - y); out.append(y)
    return out

def onepole_hp(x, hz):
    lp = onepole_lp(x, hz)
    return [v - l for v, l in zip(x, lp)]

def crack(rec, keep, skip=0):
    x = rec[skip:skip + n_of(keep)]
    fade = n_of(0.03)
    for i in range(max(0, len(x) - fade), len(x)):
        x[i] *= (len(x) - i) / fade
    return onepole_hp(x, 120)

def thump(f0, f1, dur):
    out, ph = [], 0.0
    for i in range(n_of(dur)):
        t = i / SR
        f = f1 + (f0 - f1) * math.exp(-t * 18)
        ph += 2 * math.pi * f / SR
        out.append(math.sin(ph) * math.exp(-t * 6 / dur) * min(1, i / 40))
    return out

def ring(freq, dur, detune=1.0):
    partials = [(1.0, 1.0), (2.76, 0.5), (5.40, 0.25), (8.93, 0.12)]   # a struck bar: the machine's voice
    out = []
    for i in range(n_of(dur)):
        t = i / SR
        out.append(sum(a * math.sin(2 * math.pi * freq * detune * r * t) * math.exp(-t * (5 + 3 * r) / dur)
                       for r, a in partials))
    return out

def tail(dur, cutoff, seed):
    rnd = random.Random(seed)
    x = [rnd.uniform(-1, 1) * math.exp(-4 * (i / SR) / dur) for i in range(n_of(dur))]
    return onepole_lp(x, cutoff)

GUNS = {   # name: recording, crack length, thump (from, to, s), ring Hz, ring s, tail (s, Hz), gains (crack, thump, ring, tail)
    "revolver": ("revolver", 0.09, (90, 50, 0.16), 520, 0.25, (0.25, 2600), (1.0, 0.85, 0.22, 0.30)),
    "shotgun":  ("shotgun",  0.12, (70, 40, 0.26), 330, 0.30, (0.45, 1800), (1.0, 1.00, 0.20, 0.40)),
    "kar":      ("kar",      0.12, (80, 45, 0.22), 440, 0.35, (0.60, 2200), (1.0, 0.90, 0.22, 0.35)),
    "longshot": ("longshot", 0.15, (55, 30, 0.40), 220, 0.50, (1.20, 1200), (1.0, 1.10, 0.25, 0.55)),
}
GLOW_HZ = [520, 330, 440, 220]   # the switch chime of each gun, in its ring's pitch

def shots():
    energy = {}
    for name, (rec, keep, th, rf, rd, (td, tc), (gc, gt, gr, gtl)) in GUNS.items():
        src = load(os.path.join(SRC, rec + "_rec.wav"))
        for v in range(3):
            length = n_of(max(keep, th[2], rd, td) + 0.05)
            x = mix(length,
                    (gc, 0, crack(src, keep, skip=v * n_of(0.0015))),
                    (gt, 0, thump(th[0], th[1], th[2])),
                    (gr, n_of(0.004), ring(rf, rd, detune=1.0 + 0.03 * (v - 1))),
                    (gtl, n_of(0.01), tail(td, tc, seed=hash((name, v)) & 0xffff)))
            save(name if v == 0 else f"{name}_{v}", x)
            if v == 0: energy[name] = sum(s * s for s in x[:n_of(0.3)]) * (len(x) / SR)
    order = sorted(energy, key=energy.get)
    print("weight order (light -> heavy):", order)

def click(freq, dur=0.035, noise=0.6, seed=1):
    rnd = random.Random(seed)
    return [(noise * rnd.uniform(-1, 1) + (1 - noise) * math.sin(2 * math.pi * freq * i / SR)) * math.exp(-i / SR * 70)
            for i in range(n_of(dur))]

def slide(dur, cutoff, seed=2):
    rnd = random.Random(seed)
    x = [rnd.uniform(-1, 1) * math.sin(math.pi * i / n_of(dur)) for i in range(n_of(dur))]
    return onepole_lp(x, cutoff)

def foley():
    save("cyl_open",  mix(n_of(0.25), (1, 0, click(1800)), (0.7, n_of(0.06), click(1200, seed=3)), (0.3, 0, ring(1600, 0.2))), 0.7)
    save("cyl_close", mix(n_of(0.30), (1, 0, click(2200)), (0.5, 0, thump(160, 90, 0.08)), (0.35, 0, ring(1400, 0.25))), 0.8)
    save("eject",     mix(n_of(0.50), *[(0.8 - 0.15 * k, n_of(0.05 + 0.09 * k), click(3000 - 300 * k, seed=10 + k)) for k in range(4)]), 0.6)
    save("shell_in",  mix(n_of(0.14), (0.6, 0, slide(0.06, 3000)), (1, n_of(0.05), click(1400, seed=5))), 0.7)
    save("pump",      mix(n_of(0.42), (0.7, 0, slide(0.14, 2200)), (1, n_of(0.14), click(900, seed=6)),
                          (0.6, n_of(0.22), slide(0.12, 2600, seed=7)), (1, n_of(0.34), click(1100, seed=8)), (0.3, n_of(0.34), ring(330, 0.08))), 0.85)
    save("bolt",      mix(n_of(0.42), (1, 0, click(1500, seed=9)), (0.6, n_of(0.08), slide(0.1, 2500, seed=11)),
                          (1, n_of(0.2), click(1300, seed=12)), (0.9, n_of(0.32), click(1700, seed=13)), (0.25, n_of(0.32), ring(440, 0.08))), 0.8)
    save("reload",    mix(n_of(0.28), (1, 0, click(1200, seed=14)), (0.8, n_of(0.12), click(1600, seed=15))), 0.7)
    save("cell",      [math.sin(2 * math.pi * 3200 * i / SR) * math.exp(-i / SR * 120) for i in range(n_of(0.04))], 0.45)
    save("dry",       click(700, 0.05, noise=0.4, seed=16), 0.5)
    for g, hz in enumerate(GLOW_HZ):   # a short rising chime as the gun comes up
        out, ph = [], 0.0
        for i in range(n_of(0.22)):
            t = i / SR
            f = hz * (0.5 + 0.5 * min(1, t / 0.12))
            ph += 2 * math.pi * f / SR
            out.append((math.sin(ph) + 0.3 * math.sin(2.76 * ph)) * math.exp(-t * 9) * min(1, i / 200))
        save(f"switch_up{g}", mix(n_of(0.22), (1, 0, out), (0.5, 0, click(2000, 0.02, seed=20 + g))), 0.55)

if __name__ == "__main__":
    shots()
    foley()
```

- [ ] **Step 3: Generate and check**

Run: `python3 tools/gen_arsenal.py`
Expected: writes the 12 shot files and the foley; prints a weight order with the revolver first and the Longshot last (if not, raise that gun's thump/tail gains in `GUNS` and re-run; ledger it).

- [ ] **Step 4: Load the new sounds**

`src/main.cpp`: add `"cell", "dry", "switch_up0", "switch_up1", "switch_up2", "switch_up3"` to `SOUNDS` (variants `_1/_2` load automatically). `assets/sfx/CREDITS.md`: a paragraph — the four gunshots are now built by `tools/gen_arsenal.py`, layering the first instant of each CC0 recording (frozen in `assets/sfx/src/`) with synthesized thump, ring and tail; the foley is synthesized.

- [ ] **Step 5: Listen by numbers**

```bash
OVERDRIVE_SFXLIST=1 ./shooter --arena 1 --god --res 360 --shot 5 $SCRATCH/x.bmp 2>&1 | grep -E "^sfx (revolver|shotgun|kar|longshot|switch_up|cell|dry)"
for w in 1 2 3 4; do ./shooter --arena 1 --god --weapon $w --spawn 0 --spawn 0 --spawn 4 --autoaim --res 360 --audiodump $SCRATCH/gun$w.wav 6; done
```
Expected: every shot loads with 3 variants; each dump's peak ≤ 1.0 and its loudest second ranks revolver < Lancer ≤ shotgun < Longshot by RMS.

- [ ] **Step 6: Commit**

```bash
git add tools/gen_arsenal.py assets/sfx src/main.cpp
git commit -m "One family of gun sounds: crack, thump, ring and tail for every shot; matching foley, a chime per gun"
```

---

### Task 6: Verify and document

**Files:**
- Modify: `README.md`, `src/Gameplay_Dev.h` (overlay for reload/switch screenshots if needed)

- [ ] **Step 1: Screenshots of every state**

Add dev overlays to freeze the view model for screenshots (same pattern as `--overlay poseN`): `--overlay gunreload` holds the motion at `reloadU = 0.5` (call `viewModel.motion.reload(...)` once and stop updating it), `--overlay gunswitch` holds it mid-rise, `--overlay fist` holds a parry punch. Then:

```bash
for w in 1 2 3 4; do for o in none gunreload gunswitch; do
  ./shooter --arena 1 --god --clean --weapon $w --overlay $o --res 720 --shot 60 $SCRATCH/g${w}_$o.bmp; done; done
./shooter --arena 1 --god --clean --weapon 3 --aim --res 720 --shot 60 $SCRATCH/g3_aim.bmp
./shooter --arena 1 --god --clean --weapon 4 --aim --res 720 --shot 60 $SCRATCH/g4_aim.bmp
./shooter --arena 1 --god --clean --overlay fist --res 720 --shot 60 $SCRATCH/fist.bmp
```
Look at each: one family, readable cells in each colour, nothing clipping the screen edge, the Lancer's post in its notch and the Longshot's scope view when aimed.

- [ ] **Step 2: Frame time, web**

Run: `./shooter --arena 2 --wave 2 --god --bench 600 --res 720` here and on `main` (a scratch worktree); `source ~/emsdk/emsdk_env.sh && make web`.
Expected: within noise (the parts list is rebuilt per frame: ~100 boxes); web builds.

- [ ] **Step 3: README**

The weapons section: the four guns as one arsenal (blackened metal, bone grips, a glow each — amber, ember, cyan, violet — ammo cells on the gun), the motion (switch chime, the three reload beats), the sound recipe and `tools/gen_arsenal.py`; Project Structure gains `GunKit.h`, `GunMotion.h`, `tools/gen_arsenal.py`.

- [ ] **Step 4: Commit**

```bash
git add README.md src/Gameplay_Dev.h
git commit -m "README and dev overlays for the arsenal"
```

---

## After the last task

Fresh reviewer (most capable model) over the whole branch with the Review Focus list; one fix pass; then merge to `main`, push, rebuild web, copy `overdrive.js`/`overdrive.wasm`/`overdrive.data` (the data file changes: new sounds) into `Personal-Website/public/overdrive`, update the site's OVERDRIVE page (the Lancer, the arsenal look/sound), verify and push.
