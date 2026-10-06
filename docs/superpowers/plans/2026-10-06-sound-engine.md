# Sound Engine Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Replace SDL_mixer channel playback of sound effects with our own mixer inside the existing audio callback: positional sound, variation, group caps with priority, ducking and per-arena reverb.

**Architecture:** `src/SfxMixer.h` (no SDL, unit-tested offline) owns the sample bank, 48 voices, spatial maths, duck envelope and a small reverb; the game thread talks to it only through a lock-free command ring. `src/AudioSystem.h` loads WAVs into it with SDL, mixes it into the buffer `MusicSynth` already fills in the `Mix_HookMusic` callback, and exposes `play` / `playAt` / `moveSource` / `setListener` / `setSpace` / `duck`. Call sites then place their sounds in the world.

**Tech Stack:** C++17, SDL2 + SDL2_mixer (native and Emscripten), glm, the repo's own `tests/test_game.cpp` CHECK harness, Makefile.

**Spec:** `docs/superpowers/specs/2026-10-06-sound-engine-design.md`

## Global Constraints

- Voice caps: PLAYER 12, ENEMY 20, WORLD 12, UI 4 (48 voices total).
- Jitter: pitch ±4 %, gain ±1.5 dB; UI pitch ±1 %, UI gain 0.
- Distance: ref 4 m, rolloff 1, silent beyond 90 m; low-pass 18 kHz near → ~2.5 kHz at 90 m; behind lowers cutoff by up to half.
- Duck: attack 20 ms, hold `seconds`, release 250 ms; overlapping ducks take the deepest. Player damage 5 dB / 0.15 s, wave start 4 dB / 0.4 s, boss telegraphs/roars 6 dB / 0.5 s. Ducks the music and the WORLD/ENEMY groups.
- Reverb sends: PLAYER 0.15, ENEMY 0.3, WORLD 0.35, UI 0. Presets OPEN / METAL / HALL / SHAFT, 1 s parameter crossfade.
- Output always within ±1 (soft limit), never NaN.
- No locks, no allocation on the audio thread. A full command ring drops the command.
- Volume API stays 0–128. Without SDL_mixer every call is a no-op.
- GLSL untouched; the web build (`make web`) must keep compiling. New headers go in the Makefile `HEADERS` list (and the `test` target's dependencies when the tests include them).
- Commit messages: no Co-Authored-By / Claude trailer (user rule).
- `$SCRATCH` in commands = the session's scratchpad directory (never write test output into the repo).

## Review Focus

1. **Device rate isn't 44.1 kHz** (browsers often run 48 kHz) → the mixer, its reverb and loaded sounds work at that rate. Test in Task 3 (mixer at 48 kHz), conversion handled in Task 4.
2. **WAV files in other formats** (16-bit, stereo, 22 kHz…) → load at the right length and pitch. Dev check in Task 4 compares loaded length against each file's duration.
3. **A source exactly at the listener** (distance 0, e.g. an explosion on the player) → centred, full level, no NaN. Test in Task 2.
4. **Master volume 0, or an unknown sound name** → nothing plays, handle 0, no voice stolen. Test in Task 1.
5. **Sounds triggered before the audio starts** (constructors, the first frame) → harmless. Commands queue in the ring; `AudioSystem::play` guards on `initialized` (Task 4).

---

## File map

- Create `src/AudioTypes.h`: `SoundGroup`, `ReverbSpace`, `SoundHandle`; tiny, so `Level.h` can name a space without pulling in the mixer.
- Create `src/SfxMixer.h`: the mixer (Tasks 1–3).
- Modify `src/AudioSystem.h`: SDL loading, callback mixing, the new API, `--audiodump` capture (Task 4).
- Modify `src/main.cpp`: `audio.start()` after loading, `--audiodump`, menu space (Task 4).
- Modify `src/Level.h`, `src/LevelAct2.h`: `Arena::space` (Task 4).
- Modify `src/GameplayState.h`, `src/Gameplay_Tick.h`, `src/Gameplay_Render.h`: listener + space each frame (Task 4).
- Modify `src/Gameplay_Combat.h`, `src/Gameplay_Flow.h`, `src/Gameplay_Shifts.h`, `src/Gameplay_Sovereign.h`, `src/Gameplay_Tick.h`, `src/Gameplay_Menus.h`: call sites + ducks (Task 5).
- Modify `tests/test_game.cpp`, `Makefile`.
- Modify `README.md`: a line about the sound engine and `--audiodump` (Task 5).

---

### Task 1: SfxMixer core: bank, voices, groups, ring

**Files:**
- Create: `src/AudioTypes.h`, `src/SfxMixer.h`
- Modify: `Makefile` (HEADERS and `test` deps), `tests/test_game.cpp`

**Interfaces:**
- Produces:
  - `enum class SoundGroup : uint8_t { PLAYER, ENEMY, WORLD, UI, COUNT }`
  - `enum class ReverbSpace : uint8_t { OPEN, METAL, HALL, SHAFT, COUNT }`
  - `using SoundHandle = uint32_t` (0 = none)
  - `SfxMixer(float sampleRate = 44100.f, uint32_t seed = 0x9E3779B9u)`, `setSampleRate(float)`
  - `void addSound(const std::string& name, std::vector<float> mono)`, `bool has(name)`, `int variants(name)`
  - `struct SfxMixer::Opts { float volume = 1; SoundGroup group = PLAYER; bool priority = false; bool positional = false; glm::vec3 pos{0}; }`
  - `SoundHandle play(const std::string& name, const Opts&)`, `void move(SoundHandle, glm::vec3)`, `void stop(SoundHandle)`, `void setListener(glm::vec3 pos, glm::vec3 right, glm::vec3 fwd)`
  - `void render(float* io, int frames)`: io is interleaved stereo holding the music; effects are added, then soft-limited
  - Introspection: `bool isPlaying(SoundHandle) const`, `int active(SoundGroup) const`, `float lastPlayPitch() const`, `int lastPlayVariant() const`

- [ ] **Step 1: Write the failing tests**

Add `#include "../src/SfxMixer.h"` with the other includes at the top of `tests/test_game.cpp`, these helpers after `overlapsBox`:

```cpp
// Sound-effects mixer helpers: deterministic test signals and measurements
static std::vector<float> sfxNoise(int n, uint32_t seed = 1) {
    std::vector<float> v(n);
    for (auto& x : v) { seed ^= seed << 13; seed ^= seed >> 17; seed ^= seed << 5; x = (seed & 0xFFFF) / 32767.5f - 1.f; }
    return v;
}
static std::vector<float> sfxSquare(int n) {
    std::vector<float> v(n);
    for (int i = 0; i < n; ++i) v[i] = (i / 50) % 2 ? 1.f : -1.f;
    return v;
}
static std::vector<float> sfxRender(SfxMixer& m, int frames, float music = 0.f) {
    std::vector<float> b(2 * frames, music);
    m.render(b.data(), frames);
    return b;
}
static float sfxRms(const std::vector<float>& b, int ch) {
    double s = 0; int n = (int)b.size() / 2;
    for (int i = 0; i < n; ++i) s += b[2 * i + ch] * b[2 * i + ch];
    return n ? (float)std::sqrt(s / n) : 0.f;
}
static float sfxHf(const std::vector<float>& b, int ch) {   // RMS of the first difference: high-frequency energy
    double s = 0; int n = (int)b.size() / 2;
    for (int i = 1; i < n; ++i) { float d = b[2 * i + ch] - b[2 * (i - 1) + ch]; s += d * d; }
    return n > 1 ? (float)std::sqrt(s / (n - 1)) : 0.f;
}
static float sfxPeak(const std::vector<float>& b) { float p = 0.f; for (float x : b) p = std::max(p, std::fabs(x)); return p; }
static bool sfxFinite(const std::vector<float>& b) { for (float x : b) if (!std::isfinite(x)) return false; return true; }
```

and this block before the mouse-filter block in `main()`:

```cpp
    // ---------------------------------------------------------------- sound effects mixer: core
    {
        using G = SoundGroup;
        auto opts = [](float vol, G g, bool prio = false) { SfxMixer::Opts o; o.volume = vol; o.group = g; o.priority = prio; return o; };
        {   // silence in, silence out
            SfxMixer m(44100.f, 3); m.addSound("noise", sfxNoise(44100));
            auto b = sfxRender(m, 44100);
            CHECK(sfxPeak(b) == 0.f, "no sounds playing: the mixer adds exactly nothing");
        }
        {   // unknown names and zero volume do nothing
            SfxMixer m(44100.f, 3); m.addSound("noise", sfxNoise(44100));
            SoundHandle a = m.play("nope", opts(1.f, G::WORLD));
            SoundHandle b = m.play("noise", opts(0.f, G::WORLD));
            sfxRender(m, 64);
            CHECK(a == 0 && b == 0 && m.active(G::WORLD) == 0, "an unknown sound or zero volume plays nothing (handle 0)");
        }
        {   // a full group steals its quietest voice
            SfxMixer m(44100.f, 3); m.addSound("noise", sfxNoise(44100));
            SoundHandle quiet = 0;
            for (int i = 0; i < 20; ++i) {
                SoundHandle h = m.play("noise", opts(i == 3 ? 0.05f : 0.5f, G::ENEMY));
                if (i == 3) quiet = h;
            }
            sfxRender(m, 64);
            bool full = m.active(G::ENEMY) == 20;
            SoundHandle fresh = m.play("noise", opts(0.5f, G::ENEMY));
            sfxRender(m, 64);
            CHECK(full && m.active(G::ENEMY) == 20 && !m.isPlaying(quiet) && m.isPlaying(fresh),
                  "a full group (ENEMY 20) gives its quietest voice to the new sound");
        }
        {   // priority voices are never stolen by a non-priority sound
            SfxMixer m(44100.f, 3); m.addSound("noise", sfxNoise(44100));
            std::vector<SoundHandle> hs;
            for (int i = 0; i < 20; ++i) hs.push_back(m.play("noise", opts(0.2f, G::ENEMY, true)));
            sfxRender(m, 64);
            SoundHandle extra = m.play("noise", opts(1.f, G::ENEMY));
            sfxRender(m, 64);
            bool all = true; for (auto h : hs) all &= m.isPlaying(h);
            CHECK(all && !m.isPlaying(extra), "a group full of priority voices (the boss) keeps them all");
        }
        {   // pitch jitter: varied, within bounds; UI tighter
            SfxMixer m(44100.f, 11); m.addSound("noise", sfxNoise(441));
            float lo = 9.f, hi = 0.f, uiLo = 9.f, uiHi = 0.f;
            for (int i = 0; i < 1000; ++i) {
                m.play("noise", opts(1.f, G::WORLD)); lo = std::min(lo, m.lastPlayPitch()); hi = std::max(hi, m.lastPlayPitch());
                m.play("noise", opts(1.f, G::UI));    uiLo = std::min(uiLo, m.lastPlayPitch()); uiHi = std::max(uiHi, m.lastPlayPitch());
                if (i % 200 == 0) sfxRender(m, 16);
            }
            CHECK(lo >= 0.96f && hi <= 1.04f && hi - lo > 0.05f, "every play gets a pitch within +-4% (and they differ)");
            CHECK(uiLo >= 0.99f && uiHi <= 1.01f, "UI sounds vary by at most +-1%");
        }
        {   // variants: never the same one twice running
            SfxMixer m(44100.f, 5);
            m.addSound("step", sfxNoise(100, 1)); m.addSound("step", sfxNoise(100, 2)); m.addSound("step", sfxNoise(100, 3));
            bool noRepeat = m.variants("step") == 3; int last = -1; bool usedAll[3] = {};
            for (int i = 0; i < 60; ++i) {
                m.play("step", opts(1.f, G::PLAYER));
                int v = m.lastPlayVariant();
                noRepeat &= v != last && v >= 0 && v < 3; last = v; if (v >= 0 && v < 3) usedAll[v] = true;
                sfxRender(m, 8);
            }
            CHECK(noRepeat && usedAll[0] && usedAll[1] && usedAll[2], "a sound with variants picks among them, never the same twice running");
        }
        {   // overload: 48 full-scale voices stay within +-1 and finite
            SfxMixer m(44100.f, 3); m.addSound("loud", sfxSquare(44100));
            for (int g = 0; g < 4; ++g) for (int i = 0; i < 20; ++i) m.play("loud", opts(1.f, (G)g));
            auto b = sfxRender(m, 8192, 0.9f);
            CHECK(sfxFinite(b) && sfxPeak(b) <= 1.f, "48 full-scale voices over loud music: never past +-1, never NaN");
        }
        {   // the command ring: a flood drops, later commands still work
            SfxMixer m(44100.f, 3); m.addSound("noise", sfxNoise(44100));
            for (int i = 0; i < 600; ++i) m.move(12345, {0.f, 0.f, 0.f});
            sfxRender(m, 64);
            SoundHandle h = m.play("noise", opts(1.f, G::WORLD));
            sfxRender(m, 64);
            CHECK(h != 0 && m.isPlaying(h), "a flooded command ring drops the overflow; the next play still lands");
        }
        {   // a sound ends when its sample does
            SfxMixer m(44100.f, 3); m.addSound("blip", sfxNoise(100));
            SoundHandle h = m.play("blip", opts(1.f, G::WORLD));
            sfxRender(m, 512);
            CHECK(!m.isPlaying(h) && m.active(G::WORLD) == 0, "a short sound frees its voice when it finishes");
        }
    }
```

In `Makefile`, add `src/AudioTypes.h src/SfxMixer.h` to `HEADERS` and to the `test:` dependency list.

- [ ] **Step 2: Run the tests to verify they fail**

Run: `make test 2>&1 | tail -5`
Expected: compile error, `src/SfxMixer.h` not found.

- [ ] **Step 3: Write `src/AudioTypes.h`**

```cpp
#pragma once
// AudioTypes.h — the names the sound engine shares with the rest of the game
// (kept tiny so level data can name a reverb space without the mixer).
#include <cstdint>

// Who a sound belongs to: each group has its own voice cap (SfxMixer::GROUP_CAP),
// reverb send, and the duck applies to ENEMY and WORLD
enum class SoundGroup : uint8_t { PLAYER, ENEMY, WORLD, UI, COUNT };

// The reverb of a place (Arena::space): OPEN yard, METAL halls, a long dark
// HALL, a tight echoing SHAFT
enum class ReverbSpace : uint8_t { OPEN, METAL, HALL, SHAFT, COUNT };

using SoundHandle = uint32_t;   // a playing sound, for moveSource/stop; 0 = none
```

- [ ] **Step 4: Write `src/SfxMixer.h`**

```cpp
#pragma once
// =============================================================================
// SfxMixer.h — the sound-effects mixer. Runs in the audio callback after
// MusicSynth has filled the buffer; no SDL, so tests drive it offline.
//
// THREADS: the game thread calls play / move / stop / setListener / setSpace /
// duck, which only push commands into a lock-free single-producer ring. The
// audio thread calls render(), which drains the ring, then mixes. addSound()
// is for loading, before the audio thread starts calling render().
//
// VOICES: 48, in four groups with caps (PLAYER 12, ENEMY 20, WORLD 12, UI 4).
// A full group gives its quietest voice to the new sound; a priority voice
// (the player's gun, a boss attack) is never taken by a non-priority sound.
// Every play gets a little pitch and gain jitter, and a sound with variants
// (name_1.wav ...) never plays the same one twice running.
// =============================================================================
#include "AudioTypes.h"
#include <glm/glm.hpp>
#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <string>
#include <unordered_map>
#include <vector>

class SfxMixer {
public:
    static constexpr int MAX_VOICES = 48;
    static constexpr int CHUNK      = 512;    // frames mixed per inner pass
    static constexpr int RING       = 512;    // commands buffered between renders
    static constexpr int GROUP_CAP[(int)SoundGroup::COUNT] = {12, 20, 12, 4};

    struct Opts {
        float      volume     = 1.f;                 // linear, before jitter
        SoundGroup group      = SoundGroup::PLAYER;
        bool       priority   = false;               // never stolen by a non-priority sound
        bool       positional = false;               // placed at pos in the world
        glm::vec3  pos{0.f};
    };

    explicit SfxMixer(float sampleRate = 44100.f, uint32_t seed = 0x9E3779B9u) : rng(seed ? seed : 1u) {
        setSampleRate(sampleRate);
    }
    SfxMixer(const SfxMixer&) = delete;
    SfxMixer& operator=(const SfxMixer&) = delete;

    void  setSampleRate(float sr) { rate = sr; }
    float sampleRate() const { return rate; }

    // --- loading (before render() runs) ---------------------------------------
    void addSound(const std::string& name, std::vector<float> mono) {
        if (mono.empty()) return;
        names[name].push_back((int)bank.size());
        bank.push_back(std::move(mono));
    }
    bool has(const std::string& name) const { return names.count(name) > 0; }
    int  variants(const std::string& name) const {
        auto it = names.find(name);
        return it == names.end() ? 0 : (int)it->second.size();
    }

    // --- game thread ----------------------------------------------------------
    SoundHandle play(const std::string& name, const Opts& o) {
        auto it = names.find(name);
        if (it == names.end() || !(o.volume > 0.f)) return 0;
        const std::vector<int>& ids = it->second;
        int pick = 0;
        if (ids.size() > 1) {
            auto lv = lastVariant.find(name);
            int last = lv == lastVariant.end() ? -1 : lv->second;
            if (last < 0) pick = (int)(next() % ids.size());
            else { pick = (int)(next() % (ids.size() - 1)); if (pick >= last) ++pick; }   // never twice running
            lastVariant[name] = pick;
        }
        const bool ui = o.group == SoundGroup::UI;
        Cmd c{};
        c.kind = Cmd::PLAY; c.serial = ++serials; if (c.serial == 0) c.serial = ++serials;
        c.sample = ids[pick];
        c.pitch = 1.f + (ui ? 0.01f : 0.04f) * uni();
        c.gain  = o.volume * (ui ? 1.f : dbToLin(1.5f * uni()));
        c.group = o.group; c.priority = o.priority; c.positional = o.positional; c.a = o.pos;
        lastPitch = c.pitch; lastPick = pick;
        return push(c) ? c.serial : 0;
    }
    void move(SoundHandle h, glm::vec3 p) { if (!h) return; Cmd c{}; c.kind = Cmd::MOVE; c.serial = h; c.a = p; push(c); }
    void stop(SoundHandle h) { if (!h) return; Cmd c{}; c.kind = Cmd::STOP; c.serial = h; push(c); }
    void setListener(glm::vec3 pos, glm::vec3 right, glm::vec3 fwd) {
        Cmd c{}; c.kind = Cmd::LISTENER; c.a = pos; c.b = right; c.c = fwd; push(c);
    }
    float lastPlayPitch() const { return lastPitch; }
    int   lastPlayVariant() const { return lastPick; }

    // --- audio thread ---------------------------------------------------------
    // io: interleaved stereo, `frames` long, already holding the music
    void render(float* io, int frames) {
        drain();
        for (int done = 0; done < frames;) {
            int n = std::min(CHUNK, frames - done);
            mixChunk(io + 2 * done, n);
            done += n;
        }
    }

    // --- introspection (audio-thread state; tests) -----------------------------
    bool isPlaying(SoundHandle h) const {
        for (const auto& v : voices) if (v.active && v.serial == h) return true;
        return false;
    }
    int active(SoundGroup g) const {
        int n = 0;
        for (const auto& v : voices) n += v.active && v.group == g;
        return n;
    }

private:
    struct Cmd {
        enum Kind : uint8_t { PLAY, MOVE, STOP, LISTENER, SPACE, DUCK } kind;
        uint32_t   serial;
        int        sample;
        float      gain, pitch, x, y;
        SoundGroup group;
        bool       priority, positional;
        glm::vec3  a, b, c;
    };
    struct Voice {
        bool       active = false, priority = false, positional = false, fresh = true;
        uint32_t   serial = 0;
        int        sample = 0;
        SoundGroup group = SoundGroup::PLAYER;
        double     pos = 0.0;            // playhead, in samples (fractional: pitch)
        float      pitch = 1.f, gain = 1.f;
        glm::vec3  at{0.f};
        float      curL = 0.f, curR = 0.f, lp = 1.f;   // smoothed gains and low-pass coefficient
        float      z = 0.f;                            // low-pass state
        float      loud = 0.f;                         // last target level (for stealing)
    };
    struct Listener { glm::vec3 pos{0.f}, right{1.f, 0.f, 0.f}, fwd{0.f, 0.f, -1.f}; };

    float rate = 44100.f;
    std::vector<std::vector<float>> bank;
    std::unordered_map<std::string, std::vector<int>> names;
    std::unordered_map<std::string, int> lastVariant;   // game thread
    uint32_t rng, serials = 0;
    float    lastPitch = 1.f;
    int      lastPick = -1;

    std::array<Cmd, RING> ring{};
    std::atomic<uint32_t> head{0}, tail{0};   // head: next write (game thread), tail: next read (audio thread)

    std::array<Voice, MAX_VOICES> voices{};
    Listener lis;

    static float dbToLin(float db) { return std::pow(10.f, db / 20.f); }
    uint32_t next() { rng ^= rng << 13; rng ^= rng >> 17; rng ^= rng << 5; return rng; }
    float uni() { return (next() & 0xFFFFFF) / 8388607.5f - 1.f; }   // -1..1

    bool push(const Cmd& c) {
        uint32_t h = head.load(std::memory_order_relaxed);
        if (h - tail.load(std::memory_order_acquire) >= (uint32_t)RING) return false;   // full: drop it
        ring[h % RING] = c;
        head.store(h + 1, std::memory_order_release);
        return true;
    }
    void drain() {
        uint32_t t = tail.load(std::memory_order_relaxed), h = head.load(std::memory_order_acquire);
        for (; t != h; ++t) apply(ring[t % RING]);
        tail.store(t, std::memory_order_release);
    }
    void apply(const Cmd& c) {
        switch (c.kind) {
            case Cmd::PLAY: startVoice(c); break;
            case Cmd::MOVE: for (auto& v : voices) if (v.active && v.serial == c.serial) v.at = c.a; break;
            case Cmd::STOP: for (auto& v : voices) if (v.active && v.serial == c.serial) v.active = false; break;
            case Cmd::LISTENER: lis.pos = c.a; lis.right = c.b; lis.fwd = c.c; break;
            default: break;
        }
    }
    void startVoice(const Cmd& c) {
        if (c.sample < 0 || c.sample >= (int)bank.size()) return;
        const int g = (int)c.group;
        int count = 0, freeSlot = -1, victim = -1;
        float quietest = 1e30f;
        for (int i = 0; i < MAX_VOICES; ++i) {
            const Voice& v = voices[i];
            if (!v.active) { if (freeSlot < 0) freeSlot = i; continue; }
            if ((int)v.group != g) continue;
            ++count;
            if (v.priority && !c.priority) continue;
            if (v.loud < quietest) { quietest = v.loud; victim = i; }
        }
        int slot = count < GROUP_CAP[g] ? freeSlot : victim;
        if (slot < 0) return;   // everything in the group outranks it
        Voice& v = voices[slot];
        v = Voice{};
        v.active = true; v.priority = c.priority; v.positional = c.positional;
        v.serial = c.serial; v.sample = c.sample; v.group = c.group;
        v.pitch = c.pitch; v.gain = c.gain; v.at = c.a; v.loud = c.gain;
    }

    // Target left/right gain and low-pass coefficient for a voice right now
    void target(const Voice& v, float& L, float& R, float& lp) const {
        L = R = v.gain;
        lp = 1.f;
    }

    void mixChunk(float* io, int n) {
        for (auto& v : voices) if (v.active) mixVoice(v, io, n);
        for (int i = 0; i < 2 * n; ++i) io[i] = softClip(io[i]);
    }
    void mixVoice(Voice& v, float* io, int n) {
        float tL, tR, tlp;
        target(v, tL, tR, tlp);
        if (v.fresh) { v.curL = tL; v.curR = tR; v.lp = tlp; v.fresh = false; }
        v.loud = std::max(tL, tR);
        const std::vector<float>& s = bank[v.sample];
        const double end = (double)s.size() - 1.0;
        if (tL + tR + v.curL + v.curR <= 0.f) {   // out of earshot: keep time, skip the work
            v.pos += (double)v.pitch * n;
            if (v.pos >= end) v.active = false;
            return;
        }
        const float dL = (tL - v.curL) / n, dR = (tR - v.curR) / n, dlp = (tlp - v.lp) / n;
        for (int i = 0; i < n; ++i) {
            if (v.pos >= end) { v.active = false; break; }
            size_t k = (size_t)v.pos;
            float f = (float)(v.pos - (double)k);
            float x = s[k] + (s[k + 1] - s[k]) * f;
            v.lp += dlp; v.curL += dL; v.curR += dR;
            v.z += v.lp * (x - v.z);
            io[2 * i]     += v.z * v.curL;
            io[2 * i + 1] += v.z * v.curR;
            v.pos += v.pitch;
        }
        v.curL = tL; v.curR = tR; v.lp = tlp;
    }
    // Leaves anything under 0.8 alone, rounds the rest off toward 1
    static float softClip(float x) {
        float a = std::fabs(x);
        if (!(a < 1e6f)) return 0.f;   // NaN / inf: silence rather than a screech
        if (a <= 0.8f) return x;
        float y = 0.8f + 0.2f * std::tanh((a - 0.8f) / 0.2f);
        return x < 0.f ? -y : y;
    }
};
```

- [ ] **Step 5: Run the tests to verify they pass**

Run: `make test 2>&1 | grep -E "FAIL|mixer|sound|ALL PASSED|FAILED"`
Expected: every new line `ok:`, ends `ALL PASSED`.

- [ ] **Step 6: Commit**

```bash
git add src/AudioTypes.h src/SfxMixer.h tests/test_game.cpp Makefile
git commit -m "SfxMixer: our own effects mixer - voices, groups with caps, priority, jitter, variants"
```

---

### Task 2: Positional sound

**Files:**
- Modify: `src/SfxMixer.h` (`target()`, constants), `tests/test_game.cpp`

**Interfaces:**
- Consumes: Task 1's `SfxMixer`, `Opts{positional, pos}`, `setListener`, `move`.
- Produces: `SfxMixer::REF_DIST = 4`, `ROLLOFF = 1`, `MAX_DIST = 90` (public constants); positional voices pan, fall off and darken.

- [ ] **Step 1: Write the failing tests**

Append inside the sound-mixer block (after the Task 1 tests, still inside its outer braces):

```cpp
        // ---- positional
        auto at = [](glm::vec3 p, float vol = 1.f, G g = G::WORLD) {
            SfxMixer::Opts o; o.volume = vol; o.group = g; o.positional = true; o.pos = p; return o;
        };
        auto lisN = [](SfxMixer& m) { m.setListener({0.f, 0.f, 0.f}, {1.f, 0.f, 0.f}, {0.f, 0.f, -1.f}); };   // facing -Z, right = +X
        {
            SfxMixer m(44100.f, 3); m.addSound("noise", sfxNoise(44100)); lisN(m);
            m.play("noise", at({10.f, 0.f, 0.f}));
            auto r = sfxRender(m, 4410);
            SfxMixer m2(44100.f, 3); m2.addSound("noise", sfxNoise(44100)); lisN(m2);
            m2.play("noise", at({-10.f, 0.f, 0.f}));
            auto l = sfxRender(m2, 4410);
            CHECK(sfxRms(r, 1) > 4.f * sfxRms(r, 0) && sfxRms(l, 0) > 4.f * sfxRms(l, 1),
                  "a sound on your right is in the right ear, on your left in the left");
        }
        {
            SfxMixer f(44100.f, 3); f.addSound("noise", sfxNoise(44100)); lisN(f);
            f.play("noise", at({0.f, 0.f, -10.f}));
            auto front = sfxRender(f, 4410);
            SfxMixer b(44100.f, 3); b.addSound("noise", sfxNoise(44100)); lisN(b);
            b.play("noise", at({0.f, 0.f, 10.f}));
            auto behind = sfxRender(b, 4410);
            CHECK(sfxHf(behind, 0) / sfxRms(behind, 0) < 0.9f * sfxHf(front, 0) / sfxRms(front, 0),
                  "the same sound behind you is darker than in front");
        }
        {
            float rms[4]; const float D[4] = {4.f, 15.f, 40.f, 95.f};
            for (int k = 0; k < 4; ++k) {
                SfxMixer m(44100.f, 3); m.addSound("noise", sfxNoise(44100)); lisN(m);
                m.play("noise", at({0.f, 0.f, -D[k]}));
                auto b = sfxRender(m, 4410);
                rms[k] = k == 3 ? sfxPeak(b) : sfxRms(b, 0);
            }
            CHECK(rms[0] > rms[1] && rms[1] > rms[2] && rms[2] > 0.f, "quieter the further away (4, 15, 40 m)");
            CHECK(rms[3] == 0.f, "beyond 90 m: silent");
        }
        {   // on top of the listener: centred, full, finite
            SfxMixer m(44100.f, 3); m.addSound("noise", sfxNoise(44100)); lisN(m);
            m.play("noise", at({0.f, 0.f, 0.f}));
            auto b = sfxRender(m, 4410);
            float l = sfxRms(b, 0), r = sfxRms(b, 1);
            CHECK(sfxFinite(b) && l > 0.3f && std::fabs(l - r) < 0.01f * l, "a sound right on you is centred and full (no NaN)");
        }
        {   // a moving source follows; a stale handle moves nothing
            SfxMixer m(44100.f, 3); m.addSound("noise", sfxNoise(44100)); m.addSound("blip", sfxNoise(100)); lisN(m);
            SoundHandle old = m.play("blip", at({0.f, 0.f, -2.f}));
            sfxRender(m, 512);
            SoundHandle h = m.play("noise", at({10.f, 0.f, 0.f}));
            m.move(old, {-10.f, 0.f, 0.f});   // finished long ago: must not touch the new voice
            auto first = sfxRender(m, 4410);
            m.move(h, {-10.f, 0.f, 0.f});
            sfxRender(m, 512);                // the swing across
            auto second = sfxRender(m, 4410);
            CHECK(sfxRms(first, 1) > 4.f * sfxRms(first, 0), "a stale handle moves nothing");
            CHECK(sfxRms(second, 0) > 4.f * sfxRms(second, 1), "a moved source is heard where it went");
        }
```

- [ ] **Step 2: Run the tests to verify they fail**

Run: `make test 2>&1 | grep -E "FAIL"`
Expected: FAIL on the right/left, darker, further-away, 90 m and moved-source lines (Task 1's `target()` ignores position).

- [ ] **Step 3: Implement**

In `SfxMixer`, add the public constants after `GROUP_CAP`:

```cpp
    static constexpr float REF_DIST = 4.f;    // full level inside this
    static constexpr float ROLLOFF  = 1.f;
    static constexpr float MAX_DIST = 90.f;   // silent beyond (fading over the last 10 m)
```

and replace `target()` with:

```cpp
    // Target left/right gain and low-pass coefficient for a voice right now:
    // distance falloff, a pan law that keeps the centre at full level, and a
    // low-pass that closes with distance and when the source is behind you
    void target(const Voice& v, float& L, float& R, float& lp) const {
        float g = v.gain;
        lp = 1.f;
        if (!v.positional) { L = R = g; return; }
        glm::vec3 d = v.at - lis.pos;
        float dist = glm::length(d);
        if (!(dist < MAX_DIST)) { L = R = 0.f; return; }
        g *= REF_DIST / (REF_DIST + ROLLOFF * std::max(0.f, dist - REF_DIST));
        g *= std::min(1.f, (MAX_DIST - dist) / 10.f);
        float pan = 0.f, behind = 0.f;
        if (dist > 0.5f) {
            glm::vec3 nd = d / dist;
            pan = glm::clamp(glm::dot(nd, lis.right), -1.f, 1.f);
            behind = std::max(0.f, -glm::dot(nd, lis.fwd));
        }
        float th = (pan + 1.f) * 0.78539816f;   // 0 (hard left) .. pi/2 (hard right)
        L = g * std::min(1.f, 1.41421356f * std::cos(th));
        R = g * std::min(1.f, 1.41421356f * std::sin(th));
        float fc = 18000.f * std::pow(2500.f / 18000.f, dist / MAX_DIST) * (1.f - 0.5f * behind);
        fc = std::min(fc, 0.45f * rate);
        lp = 1.f - std::exp(-6.2831853f * fc / rate);
    }
```

- [ ] **Step 4: Run the tests to verify they pass**

Run: `make test 2>&1 | grep -E "FAIL|ALL PASSED|FAILED"`
Expected: `ALL PASSED`.

- [ ] **Step 5: Commit**

```bash
git add src/SfxMixer.h tests/test_game.cpp
git commit -m "SfxMixer: sounds from a place - falloff, pan, darker far away and behind you"
```

---

### Task 3: Ducking and reverb

**Files:**
- Modify: `src/SfxMixer.h`, `tests/test_game.cpp`

**Interfaces:**
- Consumes: Tasks 1–2.
- Produces: `void SfxMixer::duck(float dB, float seconds)` (dB = how far down, positive), `void SfxMixer::setSpace(ReverbSpace)`, `float SfxMixer::duckLevel() const` (current duck gain, tests).

- [ ] **Step 1: Write the failing tests**

Append inside the sound-mixer block:

```cpp
        // ---- duck and reverb
        {
            SfxMixer m(44100.f, 3);
            sfxRender(m, 4410, 0.5f);
            m.duck(6.f, 0.1f);
            auto during = sfxRender(m, 2205, 0.5f);                  // 50 ms in
            float dipped = during[during.size() - 2];
            sfxRender(m, 88200, 0.5f);                                // hold 0.1 s, release 250 ms
            auto after = sfxRender(m, 441, 0.5f);
            CHECK(std::fabs(dipped - 0.5f * 0.501f) < 0.03f, "a 6 dB duck dips the music to half within 50 ms");
            CHECK(std::fabs(after.back() - 0.5f) < 0.005f, "the music comes back once the duck is over");
            SfxMixer o(44100.f, 3);
            o.duck(3.f, 0.5f); o.duck(9.f, 0.1f);
            sfxRender(o, 4410, 0.5f);
            CHECK(std::fabs(o.duckLevel() - 0.355f) < 0.03f, "overlapping ducks take the deepest");
        }
        {
            auto tail = [](ReverbSpace s) {
                SfxMixer m(44100.f, 3); m.addSound("blip", sfxNoise(2205));
                m.setSpace(s);
                sfxRender(m, 66150);                                  // let the space settle (1 s crossfade)
                m.play("blip", SfxMixer::Opts{1.f, SoundGroup::WORLD});
                sfxRender(m, 4410);                                   // the blip (50 ms) and its first echoes
                return sfxRms(sfxRender(m, 22050), 0);                // the 0.1-0.6 s tail
            };
            float open = tail(ReverbSpace::OPEN), hall = tail(ReverbSpace::HALL);
            CHECK(hall > 3.f * open && open >= 0.f, "a HALL rings on far longer than the OPEN yard");
            SfxMixer u(44100.f, 3); u.addSound("blip", sfxNoise(2205)); u.setSpace(ReverbSpace::HALL);
            sfxRender(u, 66150);
            u.play("blip", SfxMixer::Opts{1.f, SoundGroup::UI});
            sfxRender(u, 4410);
            CHECK(sfxPeak(sfxRender(u, 22050)) == 0.f, "UI sounds stay dry (no reverb)");
        }
        {   // a 48 kHz device (browsers): same behaviour, silence still silent
            SfxMixer m(48000.f, 3); m.addSound("noise", sfxNoise(48000)); m.setSpace(ReverbSpace::SHAFT);
            bool silent = sfxPeak(sfxRender(m, 48000)) == 0.f;
            m.setListener({0.f, 0.f, 0.f}, {1.f, 0.f, 0.f}, {0.f, 0.f, -1.f});
            SfxMixer::Opts o; o.group = SoundGroup::ENEMY; o.positional = true; o.pos = {10.f, 0.f, 0.f};
            m.play("noise", o);
            auto b = sfxRender(m, 4800);
            CHECK(silent && sfxFinite(b) && sfxRms(b, 1) > 3.f * sfxRms(b, 0), "at 48 kHz: silent when idle, panned when not");
        }
```

- [ ] **Step 2: Run the tests to verify they fail**

Run: `make test 2>&1 | tail -3`
Expected: compile error, no member `duck` / `setSpace` / `duckLevel`.

- [ ] **Step 3: Implement**

Add the public game-thread calls next to `setListener`:

```cpp
    // Dip the music and the ENEMY/WORLD groups by dB for `seconds`, so the
    // sound that matters (you got hit, a boss winds up) stands out
    void duck(float dB, float seconds) { Cmd c{}; c.kind = Cmd::DUCK; c.x = dB; c.y = seconds; push(c); }
    // The reverb of the place; its parameters crossfade over about a second
    void setSpace(ReverbSpace s) { Cmd c{}; c.kind = Cmd::SPACE; c.x = (float)(int)s; push(c); }
```

and to the introspection section: `float duckLevel() const { return duckGain; }`.

Replace `setSampleRate` with one that sizes the reverb (delay lengths are Freeverb's, tuned at 44.1 kHz):

```cpp
    void setSampleRate(float sr) {
        rate = sr;
        static const int COMB[4] = {1116, 1188, 1277, 1356}, AP[2] = {556, 441};
        const float k = sr / 44100.f;
        for (int i = 0; i < 4; ++i) {
            combL[i].buf.assign(std::max(1, (int)(COMB[i] * k)), 0.f);
            combR[i].buf.assign(std::max(1, (int)((COMB[i] + 23) * k)), 0.f);
            combL[i].idx = combR[i].idx = 0; combL[i].store = combR[i].store = 0.f;
        }
        for (int i = 0; i < 2; ++i) {
            apL[i].buf.assign(std::max(1, (int)(AP[i] * k)), 0.f);
            apR[i].buf.assign(std::max(1, (int)((AP[i] + 23) * k)), 0.f);
            apL[i].idx = apR[i].idx = 0;
        }
        atkK = 1.f - std::exp(-1.f / (0.020f * sr));
        relK = 1.f - std::exp(-1.f / (0.250f * sr));
    }
```

Add private state (after `Listener lis;`):

```cpp
    // Duck envelope: dips toward duckDepth while duckHold lasts, then recovers
    float duckGain = 1.f, duckDepth = 1.f, duckHold = 0.f, atkK = 0.f, relK = 0.f;

    // Reverb: Freeverb-lite, 4 damped combs + 2 allpasses a side
    struct Comb {
        std::vector<float> buf; int idx = 0; float store = 0.f;
        float process(float in, float fb, float damp) {
            float out = buf[idx];
            store = out * (1.f - damp) + store * damp;
            buf[idx] = in + store * fb;
            if (++idx >= (int)buf.size()) idx = 0;
            return out;
        }
    };
    struct Allpass {
        std::vector<float> buf; int idx = 0;
        float process(float in) {
            float b = buf[idx], out = b - in;
            buf[idx] = in + b * 0.5f;
            if (++idx >= (int)buf.size()) idx = 0;
            return out;
        }
    };
    struct Preset { float fb, damp, wet; };
    static constexpr Preset PRESETS[(int)ReverbSpace::COUNT] = {
        {0.50f, 0.50f, 0.04f},   // OPEN   nearly dry, short
        {0.80f, 0.15f, 0.16f},   // METAL  bright, medium
        {0.89f, 0.40f, 0.26f},   // HALL   long, dark
        {0.74f, 0.05f, 0.22f},   // SHAFT  tight, bright, fluttery
    };
    static constexpr float SEND[(int)SoundGroup::COUNT] = {0.15f, 0.30f, 0.35f, 0.f};
    std::array<Comb, 4> combL, combR;
    std::array<Allpass, 2> apL, apR;
    Preset revCur = PRESETS[0], revWant = PRESETS[0];
    std::array<float, CHUNK> send{};
```

In `apply()`, add the two cases before `default`:

```cpp
            case Cmd::SPACE: {
                int s = std::clamp((int)c.x, 0, (int)ReverbSpace::COUNT - 1);
                revWant = PRESETS[s];
                break;
            }
            case Cmd::DUCK: {
                float lin = dbToLin(-std::fabs(c.x));
                duckDepth = duckHold > 0.f ? std::min(duckDepth, lin) : lin;
                duckHold = std::max(duckHold, c.y);
                break;
            }
```

In `target()`, apply the duck to ENEMY and WORLD: change the first line to

```cpp
        float g = v.gain * (v.group == SoundGroup::ENEMY || v.group == SoundGroup::WORLD ? duckGain : 1.f);
```

Replace `mixChunk` with:

```cpp
    void mixChunk(float* io, int n) {
        // The duck envelope, sample by sample, on the music already in io
        for (int i = 0; i < n; ++i) {
            float want = duckHold > 0.f ? duckDepth : 1.f;
            duckGain += (want - duckGain) * (want < duckGain ? atkK : relK);
            io[2 * i] *= duckGain; io[2 * i + 1] *= duckGain;
        }
        duckHold = std::max(0.f, duckHold - n / rate);
        // Reverb parameters drift toward the place's preset (~1 s)
        float k = 1.f - std::exp(-(float)n / (0.35f * rate));
        revCur.fb   += (revWant.fb   - revCur.fb)   * k;
        revCur.damp += (revWant.damp - revCur.damp) * k;
        revCur.wet  += (revWant.wet  - revCur.wet)  * k;

        std::fill(send.begin(), send.begin() + n, 0.f);
        for (auto& v : voices) if (v.active) mixVoice(v, io, n);
        for (int i = 0; i < n; ++i) {
            float in = send[i] * 0.03f, l = 0.f, r = 0.f;
            for (int c = 0; c < 4; ++c) { l += combL[c].process(in, revCur.fb, revCur.damp); r += combR[c].process(in, revCur.fb, revCur.damp); }
            for (int a = 0; a < 2; ++a) { l = apL[a].process(l); r = apR[a].process(r); }
            io[2 * i] += l * revCur.wet; io[2 * i + 1] += r * revCur.wet;
        }
        for (int i = 0; i < 2 * n; ++i) io[i] = softClip(io[i]);
    }
```

In `mixVoice`'s sample loop, after writing `io`, feed the send:

```cpp
            send[i] += v.z * (v.curL + v.curR) * 0.5f * SEND[(int)v.group];
```

- [ ] **Step 4: Run the tests to verify they pass**

Run: `make test 2>&1 | grep -E "FAIL|ALL PASSED|FAILED"`
Expected: `ALL PASSED`. If the HALL/OPEN ratio is marginal, tune only the `PRESETS` numbers (not the test).

- [ ] **Step 5: Commit**

```bash
git add src/SfxMixer.h tests/test_game.cpp
git commit -m "SfxMixer: ducking and a reverb per place"
```

---

### Task 4: Wire it into the game

**Files:**
- Modify: `src/AudioSystem.h` (rewrite), `src/main.cpp`, `src/Level.h`, `src/LevelAct2.h`, `src/GameplayState.h`, `src/Gameplay_Tick.h`, `src/Gameplay_Render.h`

**Interfaces:**
- Consumes: `SfxMixer` (Tasks 1–3), `AudioTypes.h`.
- Produces (used by Task 5):
  - `SoundHandle AudioSystem::play(const std::string& name, int volume = 128, SoundGroup g = SoundGroup::PLAYER, bool priority = false)`
  - `SoundHandle AudioSystem::playAt(const std::string& name, glm::vec3 pos, int volume = 128, SoundGroup g = SoundGroup::WORLD, bool priority = false)`
  - `void moveSource(SoundHandle, glm::vec3)`, `void stop(SoundHandle)`, `void setListener(glm::vec3 pos, glm::vec3 right, glm::vec3 fwd)`, `void setSpace(ReverbSpace)`, `void duck(float dB, float seconds)`
  - `void start()`: hook the callback after loading
  - `void startDump(const std::string& path, float seconds)`, `bool dumpDone() const`, `void writeDump()`
  - `ReverbSpace Arena::space` (default METAL)

- [ ] **Step 1: Rewrite `src/AudioSystem.h`**

```cpp
#pragma once
#include <atomic>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>
#include "MusicSynth.h"
#include "SfxMixer.h"

// SDL_mixer may not be available — guard the include
#ifdef __has_include
#  if __has_include(<SDL2/SDL_mixer.h>)
#    include <SDL2/SDL_mixer.h>
#    define HAS_SDL_MIXER 1
#  else
#    define HAS_SDL_MIXER 0
#  endif
#else
#  include <SDL2/SDL_mixer.h>
#  define HAS_SDL_MIXER 1
#endif

// SDL_mixer opens the device and gives us one callback (its music hook): the
// soundtrack (MusicSynth) renders into it, then the effects (SfxMixer) mix on
// top, then it's converted to the device's format. Effects are placed in the
// world with playAt; the listener is the camera (setListener, every frame).
class AudioSystem {
public:
    bool  initialized  = false;
    float masterVolume = 1.f;   // 0..1, from GameSettings::audioVolume
    MusicSynth music;           // the live soundtrack (MusicSynth.h), always running
    SfxMixer   sfx;             // the effects (SfxMixer.h)

    AudioSystem() {
#if HAS_SDL_MIXER
        if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
            std::cerr << "SDL_mixer init failed: " << Mix_GetError() << "\n";
            return;
        }
        initialized = true;
        int freq = 44100, ch = 2; Uint16 fmt = AUDIO_S16SYS;
        Mix_QuerySpec(&freq, &fmt, &ch);
        outFormat = fmt; outChannels = ch; outRate = freq;
        music.setSampleRate((float)freq);
        sfx.setSampleRate((float)freq);
        musicBuf.resize(16384);          // so the audio thread never has to allocate
#endif
    }

    // After the last loadSound: from here on the audio thread mixes (the bank is fixed)
    void start() {
#if HAS_SDL_MIXER
        if (initialized && !started) { Mix_HookMusic(&AudioSystem::musicCallback, this); started = true; }
#endif
    }

    // Music volume (0..1) on top of the master volume
    void setMusicVolume(float v) { music.setVolume(v * masterVolume * 0.8f); }

    ~AudioSystem() {
#if HAS_SDL_MIXER
        if (started) Mix_HookMusic(nullptr, nullptr);
        if (initialized) Mix_CloseAudio();
#endif
    }

    // path (…/name.wav) plus any variants beside it (name_1.wav … name_8.wav)
    void loadSound(const std::string& name, const std::string& path) {
#if HAS_SDL_MIXER
        if (!initialized || started) return;
        loadInto(name, path);
        std::string stem = path.size() > 4 ? path.substr(0, path.size() - 4) : path;
        for (int k = 1; k <= 8; ++k) loadInto(name, stem + "_" + std::to_string(k) + ".wav");
#else
        (void)name; (void)path;
#endif
    }

    // Not placed: the player's own sounds, announcements, UI
    SoundHandle play(const std::string& name, int volume = 128, SoundGroup g = SoundGroup::PLAYER, bool priority = false) {
        if (!initialized) return 0;
        SfxMixer::Opts o; o.volume = volume / 128.f * masterVolume; o.group = g; o.priority = priority;
        return sfx.play(name, o);
    }
    // Placed in the world: louder near, panned to its side, darker far away / behind
    SoundHandle playAt(const std::string& name, glm::vec3 pos, int volume = 128, SoundGroup g = SoundGroup::WORLD,
                       bool priority = false) {
        if (!initialized) return 0;
        SfxMixer::Opts o; o.volume = volume / 128.f * masterVolume; o.group = g; o.priority = priority;
        o.positional = true; o.pos = pos;
        return sfx.play(name, o);
    }
    void moveSource(SoundHandle h, glm::vec3 p) { if (initialized) sfx.move(h, p); }
    void stop(SoundHandle h) { if (initialized) sfx.stop(h); }
    void setListener(glm::vec3 pos, glm::vec3 right, glm::vec3 fwd) { if (initialized) sfx.setListener(pos, right, fwd); }
    void setSpace(ReverbSpace s) { if (initialized) sfx.setSpace(s); }
    void duck(float dB, float seconds) { if (initialized) sfx.duck(dB, seconds); }

    // --audiodump FILE SECONDS: capture the final mix, written as a 16-bit WAV
    void startDump(const std::string& path, float seconds) {
        dumpPath = path;
        dumpBuf.assign(2 * (size_t)std::max(1.f, seconds * (float)outRate), 0.f);
        dumpWritten.store(0, std::memory_order_relaxed);
        dumping.store(true, std::memory_order_release);
    }
    bool dumpDone() const {
        return !dumpPath.empty() && dumpWritten.load(std::memory_order_acquire) >= (int)(dumpBuf.size() / 2);
    }
    void writeDump() {
        FILE* f = std::fopen(dumpPath.c_str(), "wb");
        if (!f) { dumpPath.clear(); return; }
        uint32_t frames = (uint32_t)(dumpBuf.size() / 2), data = frames * 4, rate = (uint32_t)outRate, br = rate * 4;
        uint32_t riff = 36 + data, fmtLen = 16; uint16_t pcm = 1, chans = 2, align = 4, bits = 16;
        std::fwrite("RIFF", 1, 4, f); std::fwrite(&riff, 4, 1, f); std::fwrite("WAVEfmt ", 1, 8, f);
        std::fwrite(&fmtLen, 4, 1, f); std::fwrite(&pcm, 2, 1, f); std::fwrite(&chans, 2, 1, f);
        std::fwrite(&rate, 4, 1, f); std::fwrite(&br, 4, 1, f); std::fwrite(&align, 2, 1, f); std::fwrite(&bits, 2, 1, f);
        std::fwrite("data", 1, 4, f); std::fwrite(&data, 4, 1, f);
        for (float v : dumpBuf) { int16_t s = (int16_t)(std::max(-1.f, std::min(1.f, v)) * 32767.f); std::fwrite(&s, 2, 1, f); }
        std::fclose(f);
        std::fprintf(stderr, "audiodump: %u frames at %u Hz -> %s\n", frames, rate, dumpPath.c_str());
        dumpPath.clear();
    }

private:
    bool started = false;
    int  outRate = 44100;
    std::string dumpPath;
    std::vector<float> dumpBuf;
    std::atomic<int>  dumpWritten{0};
    std::atomic<bool> dumping{false};
#if HAS_SDL_MIXER
    Uint16 outFormat = AUDIO_S16SYS;
    int    outChannels = 2;
    std::vector<float> musicBuf;

    // One WAV → mono float at the device rate, into the bank (missing files are skipped)
    void loadInto(const std::string& name, const std::string& path) {
        SDL_AudioSpec spec; Uint8* raw = nullptr; Uint32 len = 0;
        if (!SDL_LoadWAV(path.c_str(), &spec, &raw, &len)) return;
        SDL_AudioCVT cvt;
        if (SDL_BuildAudioCVT(&cvt, spec.format, spec.channels, spec.freq, AUDIO_F32SYS, 1, outRate) < 0) { SDL_FreeWAV(raw); return; }
        std::vector<Uint8> work((size_t)len * (size_t)std::max(1, cvt.len_mult));
        std::memcpy(work.data(), raw, len);
        SDL_FreeWAV(raw);
        int bytes = (int)len;
        if (cvt.needed) {
            cvt.len = (int)len; cvt.buf = work.data();
            if (SDL_ConvertAudio(&cvt) < 0) return;
            bytes = cvt.len_cvt;
        }
        std::vector<float> mono((size_t)bytes / sizeof(float));
        std::memcpy(mono.data(), work.data(), mono.size() * sizeof(float));
        sfx.addSound(name, std::move(mono));
    }

    // Audio thread: music, then effects on top, then convert to the device's format
    static void musicCallback(void* self, Uint8* stream, int len) {
        auto* a = static_cast<AudioSystem*>(self);
        int ch = a->outChannels > 0 ? a->outChannels : 2;
        int bytesPerSample = SDL_AUDIO_BITSIZE(a->outFormat) / 8;
        int frames = len / (bytesPerSample * ch);
        if ((int)a->musicBuf.size() < frames * 2) a->musicBuf.resize(frames * 2);
        a->music.render(a->musicBuf.data(), frames);
        a->sfx.render(a->musicBuf.data(), frames);
        const float* src = a->musicBuf.data();
        if (a->dumping.load(std::memory_order_acquire)) {
            int w = a->dumpWritten.load(std::memory_order_relaxed), cap = (int)(a->dumpBuf.size() / 2);
            int n = std::min(frames, cap - w);
            if (n > 0) std::memcpy(&a->dumpBuf[2 * (size_t)w], src, sizeof(float) * 2 * (size_t)n);
            a->dumpWritten.store(w + std::max(n, 0), std::memory_order_release);
            if (w + n >= cap) a->dumping.store(false, std::memory_order_relaxed);
        }
        if (SDL_AUDIO_ISFLOAT(a->outFormat) && bytesPerSample == 4) {
            float* out = reinterpret_cast<float*>(stream);
            for (int f = 0; f < frames; ++f) for (int c = 0; c < ch; ++c) out[f * ch + c] = src[2 * f + (c & 1)];
        } else if (bytesPerSample == 2 && SDL_AUDIO_ISSIGNED(a->outFormat)) {
            Sint16* out = reinterpret_cast<Sint16*>(stream);
            for (int f = 0; f < frames; ++f)
                for (int c = 0; c < ch; ++c) {
                    float v = src[2 * f + (c & 1)];
                    v = v < -1.f ? -1.f : v > 1.f ? 1.f : v;
                    out[f * ch + c] = (Sint16)(v * 32767.f);
                }
        } else {
            SDL_memset(stream, 0, len);   // unexpected device format: stay silent rather than screech
        }
    }
#endif
};
```

- [ ] **Step 2: main.cpp: start after loading, `--audiodump`, menu space**

After the `for (const char* name : SOUNDS)` loading loop add:

```cpp
    app->audio.start();   // the bank is complete: the audio thread may mix from here on
```

In the argument loop (next to `--shot`):

```cpp
        if (arg == "--audiodump" && i + 2 < argc) {   // dev: record the final mix to a WAV, then quit
            app->audio.startDump(argv[i + 1], (float)std::atof(argv[i + 2])); i += 2;
            g_devNoMouse = true;
        }
```

After the `shotFrames` block at the end of a frame:

```cpp
        if (audio.dumpDone()) { audio.writeDump(); running = false; }
```

(use the same `app->`/member access as the neighbouring `shotFrames` code there; it's inside `App`, so `audio`). Where the pending `NextState::Menu` is applied (the code that constructs `MenuState`), add `audio.setSpace(ReverbSpace::OPEN);`. Update the `--shot` help comment above the argument loop to mention `--audiodump FILE SECONDS`.

- [ ] **Step 3: `Arena::space`**

`src/Level.h`: add `#include "AudioTypes.h"` with the other includes; in `struct Arena` after `subtitle`:

```cpp
    ReverbSpace space = ReverbSpace::METAL;   // its reverb (AudioSystem::setSpace)
```

Set `a.space = ReverbSpace::OPEN;` after `a.name = "SUNSET YARD";` and `a.space = ReverbSpace::HALL;` after `a.name = "THE SANCTUM";`. In `src/LevelAct2.h`: `a.space = ReverbSpace::HALL;` after `a.name = "THE DROWNED NAVE";`, `o.space = ReverbSpace::HALL;` after `o.name = "THE ORRERY";`. (Foundry, Spire, Core, FAST rooms keep METAL.)

- [ ] **Step 4: listener and space each frame**

`src/GameplayState.h`, next to the other sound cooldowns (`telegraphSoundCd`): `int spaceArena = -1;   // the arena whose reverb is playing`.

`src/Gameplay_Tick.h`, at the top of the per-tick update (before movement):

```cpp
    if (director.arena != spaceArena && director.arena >= 0 && director.arena < (int)level.arenas.size()) {
        spaceArena = director.arena;   // a new place: its reverb
        audio.setSpace(level.arenas[spaceArena].space);
    }
```

`src/Gameplay_Render.h`, after `renderCam` is fully built (after `renderCam.position.y -= landSquash;`):

```cpp
    audio.setListener(renderCam.position, renderCam.right(), renderCam.forward());
```

- [ ] **Step 5: Build and run the tests**

Run: `make 2>&1 | grep -E "error|warning: unused" ; make test 2>&1 | tail -1`
Expected: no errors; `ALL PASSED`.

- [ ] **Step 6: Check loading (Review Focus 2) and the live mix**

Temporarily add, at the end of the sound loading loop in `main.cpp`, a dev print guarded by `std::getenv("OVERDRIVE_SFXLIST")`:

```cpp
    if (std::getenv("OVERDRIVE_SFXLIST")) app->audio.sfx.dumpLengths(stderr);
```

with in `SfxMixer` (public, game thread, loading only):

```cpp
    void dumpLengths(FILE* f) const {
        for (const auto& [n, ids] : names) for (int id : ids) std::fprintf(f, "sfx %s %.3f s\n", n.c_str(), bank[id].size() / rate);
    }
```

(add `#include <cstdio>` to SfxMixer.h). Run `OVERDRIVE_SFXLIST=1 ./shooter --arena 1 --god --shot 5 /tmp/x.bmp 2>&1 | grep '^sfx' | sort > $SCRATCH/loaded.txt` and compare with each file's duration via Python's `wave` module (`frames / rate`): every sound within 5 ms. Keep `dumpLengths`; it's cheap and handy.

Then record: `./shooter --arena 1 --god --spawn 2 --audiodump $SCRATCH/mix.wav 6` and check with a Python `wave` script that the file is 6 s, stereo, has non-zero RMS (music) and peak ≤ 1.0.

- [ ] **Step 7: Commit**

```bash
git add src/AudioSystem.h src/SfxMixer.h src/main.cpp src/Level.h src/LevelAct2.h src/GameplayState.h src/Gameplay_Tick.h src/Gameplay_Render.h
git commit -m "Sound effects through our own mixer: the camera listens, every arena has its own reverb, --audiodump"
```

---

### Task 5: Place every sound, add the ducks

**Files:**
- Modify: `src/Gameplay_Combat.h`, `src/Gameplay_Flow.h`, `src/Gameplay_Shifts.h`, `src/Gameplay_Sovereign.h`, `src/Gameplay_Tick.h`, `src/Gameplay_Menus.h`, `README.md`

**Interfaces:**
- Consumes: Task 4's `play` / `playAt` / `duck`.

- [ ] **Step 1: Convert the call sites**

Line numbers are as of commit 1fd6664; find by content. Calls not listed stay `audio.play(...)` with the default PLAYER group.

| Where | Now | Becomes |
|---|---|---|
| Combat `breakHalo` | `play("parry", 90)` | `playAt("parry", at, 90, SoundGroup::ENEMY)` |
| Combat punch: `clank` on an enemy (`play("clank", 50)`) | | `playAt("clank", c, 50, SoundGroup::ENEMY)` |
| Combat `spawnEnemy`'s spawn sound (the `glm::clamp(110.f - d * 2.f…)` one) | hand falloff | `playAt("spawn", pos, 110, SoundGroup::WORLD)`; delete the `d` line |
| Combat enemy telegraph (`clamp(70.f - dist * 2.f…)`) | hand falloff | `playAt("telegraph", epos, 70, SoundGroup::ENEMY, isBoss(enemies[i].type))`; when `isBoss`, also `audio.duck(6.f, 0.5f)`; keep the `dist < 30` and cooldown guards |
| Combat Seraph beam hiss `play("skim", 55)` | | `playAt("skim", ev.beamTo, 55, SoundGroup::ENEMY)` |
| Combat `ev.slam` `play("slam")` | | `playAt("slam", epos, 128, SoundGroup::ENEMY, isBoss(enemies[i].type))` |
| Combat `ev.slash` `play("dash", …)` | | `playAt("dash", epos, ev.slash == 2 ? 120 : 95, SoundGroup::ENEMY, true)` (only the Sovereign slashes) |
| Combat `ev.dashStarted` / `ev.leapStarted` | | `playAt("dash", epos, 128, SoundGroup::ENEMY, isBoss(...))` / `playAt("jump", epos, 128, SoundGroup::ENEMY, isBoss(...))` |
| Combat `ev.enraged` `play("wave")` | | `play("wave", 128, SoundGroup::UI)` + `audio.duck(6.f, 0.5f)` |
| Combat `damagePlayer` `play("player_hit")` | | unchanged + `audio.duck(5.f, 0.15f)` |
| Combat `onEnemyKilled` `play("enemy_death")` | | `playAt("enemy_death", e.position + glm::vec3{0, e.height() * 0.5f, 0}, 128, SoundGroup::ENEMY)` |
| Combat boss death `play("explosion")` | | `playAt("explosion", e.position + glm::vec3{0, 2.f, 0}, 128, SoundGroup::WORLD, true)` |
| Combat `levelup` | | `play("levelup", 128, SoundGroup::UI)` |
| Combat `processBlasts` `play("explosion")` | | `playAt("explosion", b.pos, 128, SoundGroup::WORLD)` |
| Combat deflect `play("clank", 100)` | | `playAt("clank", at, 100, SoundGroup::ENEMY, true)` |
| Combat shield `play("clank", 70)` | | `playAt("clank", at, 70, SoundGroup::ENEMY)` |
| Combat QUICKSCOPE / NOSCOPE `play("parry", 90)` | | `play("parry", 90, SoundGroup::UI)` |
| Combat weapon shot `play(SND[w])` | | `play(SND[w], 128, SoundGroup::PLAYER, true)` |
| Flow ARENA_START / WAVE_START / BOSS_START / ARENA_CLEARED / GOAL_DONE / VICTORY / FINISH_OPEN `wave`, `split`, `explosion` | | same name/volume, group `SoundGroup::UI`; WAVE_START and BOSS_START also `audio.duck(4.f, 0.4f)` |
| Flow countdown `wave`/`telegraph` | | group `SoundGroup::UI` |
| Flow doors (the `120.f - d * 3.f` one) | hand falloff | `playAt(opening ? "door" : "door_close", (c.min + c.max) * 0.5f, 120, SoundGroup::WORLD)`; delete `d` |
| Shifts flare WARN `telegraph` | | `playAt("telegraph", sa.sunPos, 110, SoundGroup::WORLD)` |
| Shifts flood start `explosion`, reactor warn `telegraph`, pulse `slam` | | same, group `SoundGroup::WORLD` (arena-wide, not placed) |
| Sovereign strikes / phantoms `telegraph` | | `playAt("telegraph", e.position, 110 / 120, SoundGroup::ENEMY, true)` + `audio.duck(6.f, 0.5f)` |
| Sovereign blink `dash` | | `playAt("dash", e.position, 128, SoundGroup::ENEMY, true)` |
| Sovereign landed blade / eruption `slam` | | `playAt("slam", s.pos, 90 / 60, SoundGroup::ENEMY, true)` |
| Sovereign last stand `wave` | | `play("wave", 128, SoundGroup::UI)` + `audio.duck(6.f, 0.5f)` |
| Tick parried heavy shell `explosion` 100 | | `playAt("explosion", pr.position, 100, SoundGroup::WORLD)` |
| Tick friendly shell `explosion` 80 | | `playAt("explosion", pr.position, 80, SoundGroup::WORLD)` |
| Tick FAST finish `wave` + `split` | | group `SoundGroup::UI` |
| Tick void fall `player_hit` | | unchanged + `audio.duck(5.f, 0.15f)` |
| Menus `upgrade`, `reload` 80, `telegraph` 50 (can't buy) | | group `SoundGroup::UI` |

After converting, `grep -n "audio.play" src/*.h` must show only player/UI/arena-wide sounds; `grep -n "d \* 2.f\|d \* 3.f\|dist \* 2.f" src/Gameplay_*.h` must find no sound volume falloff left.

- [ ] **Step 2: Build, test, check the whole mix**

Run: `make && make test 2>&1 | tail -1`
Expected: `ALL PASSED`.

Run: `./shooter --arena 3 --wave 2 --god --kite --audiodump $SCRATCH/foundry.wav 12` and `./shooter --arena 5 --god --kite --audiodump $SCRATCH/sanctum.wav 15`, then a Python `wave` script reporting per-second L/R RMS and peak: peaks ≤ 1.0, L and R RMS differ by more than 1 dB in some seconds (sounds are panned), no dropout seconds after the first.

- [ ] **Step 3: Web build**

Run: `source ~/emsdk/emsdk_env.sh && make web 2>&1 | tail -3`
Expected: builds, `web/dist/overdrive.js` updated.

- [ ] **Step 4: Frame cost**

Run: `./shooter --arena 3 --wave 2 --god --bench 600 --res 720` on `main` (stash or a second checkout of 1fd6664) and on this branch.
Expected: average frame time within noise (audio runs on its own thread natively).

- [ ] **Step 5: README**

In `README.md`'s technical or dev-flags section, add: the effects are mixed by `src/SfxMixer.h` (positional, varied, ducked, per-arena reverb), and `--audiodump FILE SECONDS` records the final mix.

- [ ] **Step 6: Commit**

```bash
git add src/Gameplay_*.h README.md
git commit -m "Every sound from its place: enemies, doors, blasts and bosses placed in the world; ducks on hits, waves and boss wind-ups"
```

---

## After the last task

Fresh reviewer (most capable model) over the whole branch against the spec, then merge to `main` per the user's call. Tell the user to copy `web/dist` to the site (Personal-Website `public/overdrive`); don't deploy.
