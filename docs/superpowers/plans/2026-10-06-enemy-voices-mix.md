# Enemy voices + the threat-aware mix — Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Every enemy gets its own "machine + choir" voice (spawn, idle, steps, a tell per attack, release, hurt, death), a director decides who may speak, and the mixer ranks sounds by role (tells cut through, chatter steps back, the music dips under tells), with every sound's level in one table.

**Architecture:** `VoiceSynth.h` builds the voice bank in code at load (no files); `VoiceTable.h` names it; `EnemyVoice.h`'s `VoiceDirector` turns per-frame enemy state into `VoiceCue`s; `SfxMixer` gains roles (CHATTER cap, tell sidechain, bus compressor, per-sound source rate, pitch/drive/delay); `MixTable.h` holds mix classes, targets and per-file trims. The game feeds the director from `updateEnemies` and plays its cues. All new logic is GL- and audio-free and tested headlessly.

**Tech Stack:** C++17 header-only, glm, SDL2_mixer (device only), headless tests in `tests/test_game.cpp` (CHECK macro), Emscripten for the web build.

**Spec:** `docs/superpowers/specs/2026-10-06-enemy-voices-mix-design.md` (amended while planning: voices built in code, call-site volumes stay as context weights).

## Global Constraints

- Direction: "dark machine + glow"; voices are "machine + choir" (machine body carrying a hollow vowel choir).
- Roles: `SoundRole{CHATTER, TELL, ACTION, UI}`; TELL within 40 m, no global cooldown, same enemy ≥ 0.15 s apart, ≤ 4 per frame (nearest); hurts once per enemy per 0.4 s and ≤ 6 a second; deaths always, floor 0.5; chatter only from the nearest 4 within 25 m; bosses exempt from the chatter cap.
- Mixer: CHATTER cap 6 (inside ENEMY); a TELL may steal CHATTER, never a TELL; sidechain while any TELL plays: chatter −8 dB, music −3 dB, attack 30 ms, release 250 ms; bus compressor −10 dBFS, 2:1, attack 10 ms, release 150 ms, before the existing soft clip.
- Hollowed: Enraged pitch ×1.12 + drive; Twinned doubled, copy +15 cents and 12 ms late; Haloed a shimmer layer, `v_halo_break` when it breaks.
- Voices built at 22.05 kHz, deterministic, peak ≤ 0.95, within ±2 dB of their class level; whole bank ≤ 150 s of audio.
- Mix classes and targets (loudest 50 ms RMS, dBFS): GUN −11, FOLEY −18, PLAYER −12, ACTION −10, WORLD −13, UI −12, TELL −14, CHATTER −22.
- `telegraph` stays for UI countdowns, purchases and hazard warnings; enemy uses of `telegraph`, `enemy_death`, `spawn` go.
- After every task: `make && make test` green. Commit messages carry **no** Claude / Co-Authored-By trailer (user rule).
- Web: `source ~/emsdk/emsdk_env.sh && make web` must build; the web data does not grow.

## Review Focus

1. **A retry or arena change while a Seraph's beam voice is held** — the held sound must stop at once and the director forget everyone (Task 3 test "reset forgets everyone"; Task 4 wires `voiceLoops` stop on reset).
2. **An enemy not reported for a frame and seen again** (dev paths, ordering) must not replay its spawn cue (Task 3 test "a frame's gap is not a new enemy").
3. **An enemy killed after it was reported in the same frame** must not come back as "fresh" (spawn cue) or add a hurt after its death cry (Task 3 test "killed mid-frame").
4. **A Twinned split or a FAST section spawning a dozen at once** — three spawn cues, the nearest (Task 3 test "a dozen appearing").
5. **A beam that ends and starts again** — a STOP then a fresh START, never two held voices for one enemy (Task 3 test "beam off and on again").

---

## File Structure

- Create `src/MixTable.h` — `MixClass`, `mixTargetDb`, `shortTermDb`; (Task 5) `MixEntry`, `mixTable()`, `mixEntry()`, `mixGain()`.
- Create `src/VoiceTable.h` — `VoiceKind`, `voiceKey`, `attackKey`, `attacksOf`, `hasRelease`, `moveCadence`, `tellDur`, `voiceName`, `VoiceSpec`, `voiceBank()`, special names.
- Create `src/VoiceSynth.h` — DSP blocks, `Profile`/`profileOf`, kind recipes, `build(spec, variant)`.
- Create `src/EnemyVoice.h` — `VoiceIn`, `VoiceCue`, `VoiceDirector`.
- Modify `src/AudioTypes.h` — `SoundRole`.
- Modify `src/SfxMixer.h` — roles, caps, sidechain, compressor, source rate, pitch/drive/delay, solo, introspection.
- Modify `src/AudioSystem.h` — `addBuffer`, `playOpts`, mix gains, dev solo.
- Modify `src/GameplayState.h`, `src/Gameplay_Combat.h`, `src/Gameplay_Penitent.h`, `src/Gameplay_Flow.h`, `src/main.cpp` — wiring.
- Modify `Makefile`, `tests/test_game.cpp`, `README.md`, `assets/sfx/CREDITS.md`.

---

### Task 0: Branch

- [ ] **Step 1:** `git checkout -b voices` (from `main`, clean tree). Create the ledger per the executing skill.

---

### Task 1: The mixer learns roles

**Files:**
- Modify: `src/AudioTypes.h`, `src/SfxMixer.h`
- Test: `tests/test_game.cpp` (new block after the existing "sound effects mixer" blocks)

**Interfaces:**
- Produces: `enum class SoundRole : uint8_t { CHATTER, TELL, ACTION, UI, COUNT };`
  `SfxMixer::Opts{..., SoundRole role = ACTION; float pitch = 1, drive = 0, delay = 0;}`;
  `SfxMixer::addSound(name, std::vector<float>, float srcRate = 0.f)`;
  `CHATTER_CAP = 6`; introspection `roleActive(SoundRole)`, `sidechainLevel()`, `chatterGain()`, `musicGain()`, `compReductionDb()`; `devSolo(int roleMask)`;
  `const std::vector<float>* sample(const std::string&, int variant) const`; `float sampleRateOf(const std::string&) const`.

- [ ] **Step 1: Write the failing tests** — append this block in `tests/test_game.cpp` right after the last existing `sound effects mixer` block (before the next `// ----` section header):

```cpp
    // ---------------------------------------------------------------- sound effects mixer: roles and the bus
    {
        using G = SoundGroup; using R = SoundRole;
        auto ro = [](R role, float vol = 0.5f, bool prio = false) {
            SfxMixer::Opts o; o.volume = vol; o.group = SoundGroup::ENEMY; o.role = role; o.priority = prio; return o;
        };
        {   // chatter: at most 6 at once, a 7th takes the oldest
            SfxMixer m(44100.f, 3); m.addSound("noise", sfxNoise(44100));
            std::vector<SoundHandle> hs;
            for (int i = 0; i < 6; ++i) { hs.push_back(m.play("noise", ro(R::CHATTER))); sfxRender(m, 16); }
            SoundHandle seventh = m.play("noise", ro(R::CHATTER)); sfxRender(m, 16);
            CHECK(m.roleActive(R::CHATTER) == 6 && !m.isPlaying(hs[0]) && m.isPlaying(hs[1]) && m.isPlaying(seventh),
                  "enemy chatter: 6 voices at most, a 7th takes the oldest");
        }
        {   // chatter piling up never takes a tell
            SfxMixer m(44100.f, 3); m.addSound("noise", sfxNoise(44100));
            SoundHandle t = m.play("noise", ro(R::TELL, 0.05f, true)); sfxRender(m, 16);
            for (int i = 0; i < 8; ++i) { m.play("noise", ro(R::CHATTER)); sfxRender(m, 16); }
            CHECK(m.isPlaying(t) && m.roleActive(R::TELL) == 1 && m.roleActive(R::CHATTER) == 6,
                  "chatter piling up never silences a tell");
        }
        {   // a full ENEMY group: a tell takes chatter, never a (quieter) tell
            SfxMixer m(44100.f, 3); m.addSound("noise", sfxNoise(44100));
            for (int i = 0; i < 15; ++i) m.play("noise", ro(R::TELL, 0.01f, true));
            for (int i = 0; i < 5; ++i) m.play("noise", ro(R::CHATTER, 0.9f));
            sfxRender(m, 16);
            SoundHandle t = m.play("noise", ro(R::TELL, 0.5f, true)); sfxRender(m, 16);
            CHECK(m.active(G::ENEMY) == 20 && m.isPlaying(t) && m.roleActive(R::TELL) == 16 && m.roleActive(R::CHATTER) == 4,
                  "a tell in a full group takes a chatter voice, not a quieter tell");
        }
        {   // the sidechain: while a tell plays chatter dips 8 dB and the music 3 dB; both come back after
            SfxMixer m(44100.f, 3); m.addSound("noise", sfxNoise(44100));
            m.addSound("hush", std::vector<float>(13230, 0.f));   // a silent 0.3 s tell
            m.play("noise", ro(R::CHATTER));
            m.play("hush", ro(R::TELL, 1.f, true));
            sfxRender(m, 8820, 0.1f);   // 0.2 s
            float chatDb = 20.f * std::log10(m.chatterGain()), musDb = 20.f * std::log10(m.musicGain());
            CHECK(std::fabs(chatDb + 8.f) < 0.5f && std::fabs(musDb + 3.f) < 0.5f, "while a tell plays, chatter dips 8 dB and the music 3 dB");
            sfxRender(m, 4410); sfxRender(m, 26460);   // the tell ends at 0.3 s, then 0.6 s more
            CHECK(m.chatterGain() > 0.9f && m.musicGain() > 0.95f, "...and both come back within about half a second after it");
        }
        {   // the music really is 3 dB down under a tell
            SfxMixer m(44100.f, 3); m.addSound("hush", std::vector<float>(44100, 0.f));
            m.play("hush", ro(R::TELL, 1.f, true));
            auto b = sfxRender(m, 8820, 0.1f);
            CHECK(std::fabs(std::fabs(b[b.size() - 2]) - 0.1f * 0.70795f) < 0.003f, "the music under a tell is 3 dB down");
        }
        {   // the bus compressor: nothing on a quiet mix, at most ~5 dB on a full-scale one
            SfxMixer m(44100.f, 3);
            sfxRender(m, 4410, 0.03f);
            float quiet = m.compReductionDb();
            sfxRender(m, 22050, 1.f);
            float loud = m.compReductionDb();
            CHECK(quiet == 0.f && loud > 4.f && loud <= 5.05f, "the bus compressor leaves quiet mixes alone and takes at most ~5 dB off a full-scale one");
        }
        {   // a 22.05 kHz sound plays for its real length
            SfxMixer m(44100.f, 3); m.addSound("half", std::vector<float>(2205, 0.3f), 22050.f);   // 0.1 s
            SfxMixer::Opts o; o.group = G::UI;
            SoundHandle h = m.play("half", o);
            sfxRender(m, 4300); bool mid = m.isPlaying(h);
            sfxRender(m, 200);  bool done = !m.isPlaying(h);
            CHECK(mid && done, "a sound built at 22.05 kHz plays for its real length at 44.1 kHz");
        }
        {   // pitch: twice the rate, half the length
            SfxMixer m(44100.f, 3); m.addSound("dc", std::vector<float>(4410, 0.3f));
            SfxMixer::Opts o; o.group = G::UI; o.pitch = 2.f;
            SoundHandle h = m.play("dc", o);
            sfxRender(m, 2300);
            CHECK(!m.isPlaying(h), "a sound at pitch 2 lasts half as long");
        }
        {   // delay: silent until it's due
            SfxMixer m(44100.f, 3); m.addSound("dc", std::vector<float>(44100, 0.5f));
            SfxMixer::Opts o; o.group = G::UI; o.delay = 0.012f;
            m.play("dc", o);
            auto b = sfxRender(m, 1024);
            CHECK(b[2 * 500] == 0.f && std::fabs(b[2 * 1000]) > 0.1f, "a delayed sound (a Twinned echo) starts 12 ms late");
        }
        {   // drive: saturated (more edge), never louder
            std::vector<float> sine(44100);
            for (int i = 0; i < 44100; ++i) sine[i] = 0.6f * std::sin(6.2831853f * 220.f * i / 44100.f);
            auto run = [&](float drive) {
                SfxMixer m(44100.f, 3); m.addSound("sine", sine);
                SfxMixer::Opts o; o.group = G::UI; o.drive = drive; o.volume = 0.5f;
                m.play("sine", o); return sfxRender(m, 8192);
            };
            auto clean = run(0.f), driven = run(1.f);
            CHECK(sfxPeak(driven) <= sfxPeak(clean) + 1e-4f && sfxHf(driven, 0) / sfxRms(driven, 0) > 1.05f * sfxHf(clean, 0) / sfxRms(clean, 0),
                  "drive (an Enraged voice) adds edge without adding level");
        }
        {   // dev solo: only the chosen roles, no music
            SfxMixer m(44100.f, 3); m.addSound("dc", std::vector<float>(44100, 0.2f));
            m.devSolo(1 << (int)R::TELL);
            m.play("dc", ro(R::CHATTER, 1.f));
            auto b = sfxRender(m, 512, 0.1f);
            CHECK(sfxPeak(b) == 0.f, "soloing tells mutes chatter and the music");
        }
        {   // reading the bank back (the mix report)
            SfxMixer m(44100.f, 3); m.addSound("a", std::vector<float>(10, 0.1f), 22050.f); m.addSound("a", std::vector<float>(20, 0.1f), 22050.f);
            CHECK(m.sample("a", 1) && m.sample("a", 1)->size() == 20 && !m.sample("a", 2) && !m.sample("b", 0) && m.sampleRateOf("a") == 22050.f,
                  "the bank can be read back: each variant and its source rate");
        }
    }
```

- [ ] **Step 2: Run to verify it fails**

Run: `make test 2>&1 | tail -5`
Expected: compile error (`SoundRole` / `role` / `roleActive` not declared).

- [ ] **Step 3: Implement**

`src/AudioTypes.h` — add after `ReverbSpace`:

```cpp
// What a sound is for, so the mix can rank it (SfxMixer): a wind-up TELL cuts
// through, enemy CHATTER (idles, steps) steps back under it; ACTION is the
// rest of the game, UI the menus and announcements
enum class SoundRole : uint8_t { CHATTER, TELL, ACTION, UI, COUNT };
```

`src/SfxMixer.h`:

1. Header comment: add a paragraph:
```cpp
// ROLES: CHATTER (enemy idles and steps, at most 6 at once; a 7th takes the
// oldest), TELL (a wind-up: never silenced by another tell, takes chatter
// first in a full group), ACTION, UI. While a TELL plays, chatter dips 8 dB
// and the music 3 dB (a sidechain). The whole bus then runs through a gentle
// compressor (2:1 above -10 dBFS) before the soft clip.
```
2. Constants after `MAX_DIST`:
```cpp
    static constexpr int   CHATTER_CAP    = 6;            // enemy idle/step voices at once
    static constexpr float SIDE_CHATTER   = 0.39811f;     // -8 dB under a TELL
    static constexpr float SIDE_MUSIC     = 0.70795f;     // -3 dB under a TELL
    static constexpr float COMP_THRESH    = 0.31623f;     // -10 dBFS
    static constexpr float COMP_THRESH_DB = -10.f, COMP_RATIO = 2.f;
```
3. `Opts` gains (after `floor`):
```cpp
        SoundRole  role       = SoundRole::ACTION;   // what it's for (see ROLES)
        float      pitch      = 1.f;                 // playback rate, on top of the jitter
        float      drive      = 0.f;                 // 0..1 saturation (an ENRAGED voice)
        float      delay      = 0.f;                 // seconds before it sounds (a TWINNED echo)
```
4. `setSampleRate` — after `relK`:
```cpp
        sideAtk = 1.f - std::exp(-1.f / (0.030f * sr));
        sideRel = 1.f - std::exp(-1.f / (0.250f * sr));
        compAtk = 1.f - std::exp(-1.f / (0.010f * sr));
        compRel = 1.f - std::exp(-1.f / (0.150f * sr));
```
5. `addSound`:
```cpp
    // srcRate: the rate it was made at (0: already the device's); the voice
    // bank is built at 22.05 kHz and played back at its real speed
    void addSound(const std::string& name, std::vector<float> mono, float srcRate = 0.f) {
        if (mono.empty()) return;
        names[name].push_back((int)bank.size());
        bank.push_back(std::move(mono));
        bankRate.push_back(srcRate);
    }
    const std::vector<float>* sample(const std::string& name, int variant) const {
        auto it = names.find(name);
        if (it == names.end() || variant < 0 || variant >= (int)it->second.size()) return nullptr;
        return &bank[it->second[variant]];
    }
    float sampleRateOf(const std::string& name) const {
        auto it = names.find(name);
        if (it == names.end()) return 0.f;
        float r = bankRate[it->second[0]];
        return r > 0.f ? r : rate;
    }
```
6. In `play`: `c.pitch = o.pitch * (1.f + (ui ? 0.01f : 0.04f) * uni());` and after setting `c.x = o.floor;` add `c.y = o.delay; c.role = o.role; c.drive = o.drive;` (`lastPitch = c.pitch;` stays).
7. Dev solo and introspection (public, after `active`):
```cpp
    int roleActive(SoundRole r) const {
        int n = 0;
        for (const auto& v : voices) n += v.active && v.role == r;
        return n;
    }
    float sidechainLevel() const { return sideEnv; }                          // 0 none .. 1 a tell is playing
    float chatterGain() const { return 1.f - sideEnv * (1.f - SIDE_CHATTER); }
    float musicGain() const { return duckGain * (1.f - sideEnv * (1.f - SIDE_MUSIC)); }
    float compReductionDb() const { return compGr; }
    // Dev (OVERDRIVE_SOLO): hear only these roles (1 << role), and no music; 0 = everything
    void devSolo(int roleMask) { soloMask.store(roleMask, std::memory_order_relaxed); }
```
8. `Cmd` gains `float drive; SoundRole role;`. `Voice` gains:
```cpp
        SoundRole  role = SoundRole::ACTION;
        float      drive = 0.f;
        int        wait = 0;                           // frames still to wait before it sounds
        uint32_t   born = 0;                           // start order (chatter steals the oldest)
```
9. Private state (after the duck envelope line):
```cpp
    // Tell sidechain envelope (0..1) and the bus compressor
    float sideEnv = 0.f, sideAtk = 0.f, sideRel = 0.f;
    float compEnv = 0.f, compGr = 0.f, compAtk = 0.f, compRel = 0.f;
    std::atomic<int> soloMask{0};
    uint32_t births = 0;
    std::vector<float> bankRate;   // per bank entry: its source rate (0 = the device's)
```
10. Replace `startVoice` with:
```cpp
    void startVoice(const Cmd& c) {
        if (c.sample < 0 || c.sample >= (int)bank.size()) return;
        const int g = (int)c.group;
        const bool tell = c.role == SoundRole::TELL, chatter = c.role == SoundRole::CHATTER;
        int count = 0, chats = 0, freeSlot = -1, victim = -1, chatVictim = -1, oldestChat = -1;
        float quietest = 1e30f, quietestChat = 1e30f;
        uint32_t oldest = 0xFFFFFFFFu;
        for (int i = 0; i < MAX_VOICES; ++i) {
            const Voice& v = voices[i];
            if (!v.active) { if (freeSlot < 0) freeSlot = i; continue; }
            if (v.role == SoundRole::CHATTER) { ++chats; if (v.born < oldest) { oldest = v.born; oldestChat = i; } }
            if ((int)v.group != g) continue;
            ++count;
            if (v.role == SoundRole::CHATTER && v.loud < quietestChat) { quietestChat = v.loud; chatVictim = i; }
            if (v.priority && !c.priority) continue;
            if (tell && v.role == SoundRole::TELL) continue;   // a tell never silences another tell
            if (v.loud < quietest) { quietest = v.loud; victim = i; }
        }
        int slot;
        if (chatter && chats >= CHATTER_CAP) slot = oldestChat;           // chatter makes room from chatter
        else if (count < GROUP_CAP[g])      slot = freeSlot;
        else                                slot = tell && chatVictim >= 0 ? chatVictim : victim;
        if (slot < 0) return;   // everything in the group outranks it
        Voice& v = voices[slot];
        v = Voice{};
        v.active = true; v.priority = c.priority; v.positional = c.positional;
        v.serial = c.serial; v.sample = c.sample; v.group = c.group;
        v.pitch = c.pitch * (bankRate[c.sample] > 0.f ? bankRate[c.sample] / rate : 1.f);
        v.gain = c.gain; v.at = c.a; v.loud = c.gain; v.floor = c.x;
        v.role = c.role; v.drive = c.drive; v.wait = (int)(std::max(0.f, c.y) * rate); v.born = ++births;
    }
```
11. In `target()`, first lines become:
```cpp
        const int solo = soloMask.load(std::memory_order_relaxed);
        if (solo && !(solo & (1 << (int)v.role))) { L = R = 0.f; lp = 1.f; return; }
        float g = v.gain * (v.group == SoundGroup::ENEMY || v.group == SoundGroup::WORLD ? duckGain : 1.f);
        if (v.role == SoundRole::CHATTER) g *= 1.f - sideEnv * (1.f - SIDE_CHATTER);   // steps back under a tell
```
12. `mixChunk`: replace the duck loop with:
```cpp
        bool tellOn = false;
        for (const auto& v : voices) if (v.active && v.role == SoundRole::TELL && v.wait <= 0) { tellOn = true; break; }
        const float side = tellOn ? 1.f : 0.f;
        const float music = soloMask.load(std::memory_order_relaxed) ? 0.f : 1.f;
        // The duck and the tell sidechain, sample by sample, on the music already in io
        for (int i = 0; i < n; ++i) {
            float want = duckHold > 0.f ? duckDepth : 1.f;
            duckGain += (want - duckGain) * (want < duckGain ? atkK : relK);
            sideEnv  += (side - sideEnv) * (side > sideEnv ? sideAtk : sideRel);
            float m = duckGain * (1.f - sideEnv * (1.f - SIDE_MUSIC)) * music;
            io[2 * i] *= m; io[2 * i + 1] *= m;
        }
```
and replace the final soft-clip loop with:
```cpp
        // The bus compressor: 2:1 above -10 dBFS, so a full wave doesn't pump
        for (int i = 0; i < n; ++i) {
            float pk = std::max(std::fabs(io[2 * i]), std::fabs(io[2 * i + 1]));
            if (!(pk < 1e6f)) pk = 0.f;
            compEnv += (pk - compEnv) * (pk > compEnv ? compAtk : compRel);
            float g = 1.f;
            if (compEnv > COMP_THRESH) { compGr = (20.f * std::log10(compEnv) - COMP_THRESH_DB) * (1.f - 1.f / COMP_RATIO); g = dbToLin(-compGr); }
            else compGr = 0.f;
            io[2 * i] *= g; io[2 * i + 1] *= g;
        }
        for (int i = 0; i < 2 * n; ++i) io[i] = softClip(io[i]);
```
13. `mixVoice`: after `if (v.fresh) {...}` and before reading `s`, handle the wait; and saturate in the sample loop:
```cpp
        int i0 = 0;
        if (v.wait > 0) { i0 = std::min(v.wait, n); v.wait -= i0; if (i0 >= n) return; }
```
change the ramp to `const int m = n - i0; const float dL = (tL - v.curL) / m, dR = (tR - v.curR) / m, dlp = (tlp - v.lp) / m;`, the out-of-earshot skip to advance by `v.pitch * m`, the loop to `for (int i = i0; i < n; ++i)`, and after computing `x`:
```cpp
            if (v.drive > 0.f) { const float k = 1.f + 4.f * v.drive; x = std::tanh(x * k) / k; }
```

- [ ] **Step 4: Run to verify it passes**

Run: `make test 2>&1 | grep -E "FAIL|ALL PASSED|FAILED"`
Expected: `ALL PASSED`. If a pre-existing mixer test now fails only because its signal crosses the compressor's threshold, lower that test's level (volume or music) so it measures what it was written to measure, and ledger it as a Ruling.

- [ ] **Step 5: Build the game**, `make 2>&1 | tail -2` → no errors.

- [ ] **Step 6: Commit**

```bash
git add src/AudioTypes.h src/SfxMixer.h tests/test_game.cpp
git commit -m "The mixer learns roles: chatter capped at six, tells cut through and dip the chatter and music, a bus compressor; per-sound source rate, pitch, drive and delay"
```

---

### Task 2: The voice bank (table + synth)

**Files:**
- Create: `src/MixTable.h`, `src/VoiceTable.h`, `src/VoiceSynth.h`
- Modify: `Makefile` (HEADERS and `test` deps: `src/MixTable.h src/VoiceTable.h src/VoiceSynth.h`), `tests/test_game.cpp` (includes + block)

**Interfaces:**
- Produces (`MixTable.h`): `enum class MixClass : uint8_t { GUN, FOLEY, PLAYER, ACTION, WORLD, UI, TELL, CHATTER, COUNT };` `float mixTargetDb(MixClass)`; `float shortTermDb(const float*, size_t, float rate)`.
- Produces (`VoiceTable.h`): `enum class VoiceKind : uint8_t { SPAWN, IDLE, MOVE, TELL, ATTACK, HURT, DEATH, SPECIAL };` `const char* voiceKey(EnemyType)`, `const char* attackKey(AttackKind)`, `std::vector<AttackKind> attacksOf(EnemyType)`, `bool hasRelease(AttackKind)`, `float moveCadence(EnemyType)`, `float tellDur(EnemyType, AttackKind)`, `std::string voiceName(EnemyType, VoiceKind, AttackKind = NONE)`, `struct VoiceSpec{name,type,kind,attack,variants,cls,special}`, `const std::vector<VoiceSpec>& voiceBank()`, `VOICE_HALO_BREAK`, `VOICE_HALO_SHIMMER`.
- Produces (`VoiceSynth.h`): `namespace VoiceSynth { constexpr float RATE = 22050.f; std::vector<float> build(const VoiceSpec&, int variant); }`

- [ ] **Step 1: Write the failing test.** Add includes at the top of `tests/test_game.cpp`: `#include "../src/VoiceSynth.h"` and `#include <set>`. Append this block after Task 1's block:

```cpp
    // ---------------------------------------------------------------- enemy voices: the bank
    {
        const auto& bank = voiceBank();
        std::map<std::string, int> seen;
        bool unique = true;
        for (auto& s : bank) unique &= seen[s.name]++ == 0;
        CHECK(unique, "every voice has its own name");
        bool table = true;
        for (int i = 0; i < (int)EnemyType::COUNT; ++i) {
            EnemyType t = (EnemyType)i;
            for (VoiceKind k : {VoiceKind::SPAWN, VoiceKind::IDLE, VoiceKind::HURT, VoiceKind::DEATH}) table &= seen.count(voiceName(t, k)) > 0;
            if (moveCadence(t) > 0.f) table &= seen.count(voiceName(t, VoiceKind::MOVE)) > 0;
            for (AttackKind a : attacksOf(t)) {
                table &= seen.count(voiceName(t, VoiceKind::TELL, a)) > 0;
                if (hasRelease(a)) table &= seen.count(voiceName(t, VoiceKind::ATTACK, a)) > 0;
            }
        }
        CHECK(table, "every enemy type has spawn, idle, hurt and death voices, steps if it moves, a tell (and release) per attack");

        // Every attack an enemy really starts has a tell: run each type against a player at a few ranges
        bool covered = true;
        for (int i = 0; i < (int)EnemyType::COUNT; ++i) {
            EnemyType t = (EnemyType)i;
            auto known = attacksOf(t);
            for (float dist : {3.f, 8.f, 14.f, 24.f}) {
                Enemy e(t, {0.f, 0.f, 0.f});
                EnemyWorld w; w.playerFeet = {dist, 0.f, 0.f}; w.playerEye = w.playerFeet + glm::vec3{0, 1.7f, 0};
                for (int f = 0; f < 60 * 40 && e.alive; ++f) {
                    if (f == 60 * 20) e.health = e.maxHealth * 0.2f;   // bosses: their late-fight moves too
                    e.update(DT, w);
                    if (e.ev.telegraphStarted && std::find(known.begin(), known.end(), e.attack) == known.end()) {
                        covered = false;
                        std::printf("      %s starts attack %d with no tell\n", statsOf(t).name, (int)e.attack);
                    }
                }
            }
        }
        CHECK(covered, "every attack an enemy starts has a tell of its own");

        float total = 0.f; bool built = true, levels = true, varied = true, same = true;
        int idx = 0;
        for (const auto& s : bank) {
            std::vector<float> first;
            for (int v = 0; v < s.variants; ++v) {
                auto b = VoiceSynth::build(s, v);
                total += b.size() / VoiceSynth::RATE;
                float pk = 0.f; bool fin = true;
                for (float x : b) { pk = std::max(pk, std::fabs(x)); fin &= std::isfinite(x); }
                float lv = shortTermDb(b.data(), b.size(), VoiceSynth::RATE);
                if (b.size() < 200 || !fin || pk > 0.951f) { built = false; std::printf("      %s/%d: %zu samples, peak %.2f\n", s.name.c_str(), v, b.size(), pk); }
                if (std::fabs(lv - mixTargetDb(s.cls)) > 2.f) { levels = false; std::printf("      %s/%d at %.1f dB (wants %.1f)\n", s.name.c_str(), v, lv, mixTargetDb(s.cls)); }
                if (v == 0) first = b; else if (b == first) varied = false;
            }
            if (idx++ % 7 == 0) same &= VoiceSynth::build(s, 0) == first;
        }
        std::printf("      voice bank: %zu names, %.1f s of audio\n", bank.size(), total);
        CHECK(built, "every voice builds: a real length, finite, peak under 0.95");
        CHECK(levels, "every voice sits within 2 dB of its mix class's level");
        CHECK(varied, "a voice's variants differ");
        CHECK(same, "the same voice builds the same every time");
        CHECK(total <= 150.f, "the whole voice bank is at most 150 s of audio");
    }
```

- [ ] **Step 2: Run to verify it fails**

Run: `make test 2>&1 | tail -5` → Expected: compile error (`VoiceSynth.h` not found).

- [ ] **Step 3: Write `src/MixTable.h`**

```cpp
#pragma once
// =============================================================================
// MixTable.h — the mix in one place. Every sound belongs to a class with a
// target level (its loudest 50 ms, RMS, dBFS); a sound file's trim brings it
// there, and the built voices (VoiceSynth.h) are made at it. Call sites keep
// their volume numbers only as context weights: 128 means "this sound at its
// class's level".
// =============================================================================
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>

enum class MixClass : uint8_t { GUN, FOLEY, PLAYER, ACTION, WORLD, UI, TELL, CHATTER, COUNT };

// Loudest first: guns and the big hits, then tells, the player, UI and the
// world, the mechanical foley, and enemy chatter well below
inline float mixTargetDb(MixClass c) {
    static const float T[(int)MixClass::COUNT] = {-11.f, -18.f, -12.f, -10.f, -13.f, -12.f, -14.f, -22.f};
    return T[(int)c];
}

// The loudest 50 ms of a sound (windows a quarter apart), as RMS in dBFS;
// -120 for silence
inline float shortTermDb(const float* x, size_t n, float rate) {
    if (!x || n == 0) return -120.f;
    const size_t win = std::max<size_t>(1, (size_t)(rate * 0.05f)), hop = std::max<size_t>(1, win / 4);
    double best = 0.0;
    for (size_t s = 0;; s += hop) {
        const size_t e = std::min(n, s + win);
        double sum = 0.0;
        for (size_t i = s; i < e; ++i) sum += (double)x[i] * x[i];
        best = std::max(best, sum / (double)(e - s));
        if (e >= n) break;
    }
    return best > 1e-12 ? 10.f * (float)std::log10(best) : -120.f;
}
```

- [ ] **Step 4: Write `src/VoiceTable.h`**

```cpp
#pragma once
// =============================================================================
// VoiceTable.h — the names of every enemy voice, and which an enemy has.
//   v_<type>_spawn / _idle / _move / _hurt / _death
//   v_<type>_tell_<attack>   the wind-up      v_<type>_atk_<attack>   the release
//   plus a few specials: a halo breaking, a boss enraged, the Penitent rising
// The bank itself is built in code (VoiceSynth.h); EnemyVoice.h picks cues.
// =============================================================================
#include "Enemy.h"
#include "MixTable.h"
#include <string>
#include <vector>

enum class VoiceKind : uint8_t { SPAWN, IDLE, MOVE, TELL, ATTACK, HURT, DEATH, SPECIAL };

inline const char* voiceKey(EnemyType t) {
    static const char* K[] = {"husk", "ripper", "sentinel", "raptor", "brute", "mite", "juggernaut", "warden",
                              "sovereign", "shieldbearer", "conduit", "conductor", "seraph", "anchor", "penitent"};
    static_assert(sizeof(K) / sizeof(K[0]) == (size_t)EnemyType::COUNT, "a voice key per enemy type");
    return (int)t < (int)EnemyType::COUNT ? K[(int)t] : "any";
}

inline const char* attackKey(AttackKind k) {
    switch (k) {
        case AttackKind::SHOT: return "shot";         case AttackKind::BURST: return "burst";
        case AttackKind::LUNGE: return "lunge";       case AttackKind::DIVE: return "dive";
        case AttackKind::SLAM: return "slam";         case AttackKind::LOB: return "lob";
        case AttackKind::FUSE: return "fuse";         case AttackKind::VOLLEY: return "volley";
        case AttackKind::SUMMON: return "summon";     case AttackKind::SHELL: return "shell";
        case AttackKind::SMASH: return "smash";       case AttackKind::DASH: return "dash";
        case AttackKind::SWEEP: return "sweep";       case AttackKind::CLEAVE: return "cleave";
        case AttackKind::LEAP: return "leap";         case AttackKind::CRESCENT: return "crescent";
        case AttackKind::BLINK: return "blink";       case AttackKind::JUDGMENT: return "judgment";
        case AttackKind::WHIRL: return "whirl";       case AttackKind::THRUST: return "thrust";
        case AttackKind::RUPTURE: return "rupture";   case AttackKind::PHANTOMS: return "phantoms";
        case AttackKind::BASH: return "bash";         case AttackKind::BEAM: return "beam";
        case AttackKind::CENSER_LOW: return "censer_low"; case AttackKind::CENSER_HIGH: return "censer_high";
        case AttackKind::PSLAM: return "pslam";       case AttackKind::PSTOMP: return "stomp";
        case AttackKind::PLASH: return "lash";        case AttackKind::SCOURGE: return "scourge";
        default: return "none";
    }
}

// Every attack each type winds up (the test runs each enemy to prove it)
inline std::vector<AttackKind> attacksOf(EnemyType t) {
    using A = AttackKind;
    switch (t) {
        case EnemyType::HUSK:         return {A::SHOT};
        case EnemyType::RIPPER:       return {A::LUNGE};
        case EnemyType::SENTINEL:     return {A::BURST};
        case EnemyType::RAPTOR:       return {A::SHOT, A::DIVE};
        case EnemyType::BRUTE:        return {A::SLAM, A::LOB};
        case EnemyType::MITE:         return {A::FUSE};
        case EnemyType::JUGGERNAUT:   return {A::SHELL, A::SMASH};
        case EnemyType::WARDEN:       return {A::VOLLEY, A::SLAM, A::SUMMON};
        case EnemyType::SOVEREIGN:    return {A::DASH, A::SWEEP, A::CLEAVE, A::LEAP, A::CRESCENT, A::BLINK,
                                              A::JUDGMENT, A::WHIRL, A::THRUST, A::RUPTURE, A::PHANTOMS};
        case EnemyType::SHIELDBEARER: return {A::SHOT, A::BASH};
        case EnemyType::SERAPH:       return {A::BEAM};
        case EnemyType::ANCHOR:       return {A::VOLLEY};
        case EnemyType::PENITENT:     return {A::CENSER_LOW, A::CENSER_HIGH, A::PSLAM, A::PSTOMP, A::PLASH, A::SCOURGE};
        default:                      return {};
    }
}

// Does the attack get a release sound of its own? (Slams and smashes, the
// Sovereign's strokes and the Penitent's blows already sound in the game.)
// BEAM's is held while the beam is on.
inline bool hasRelease(AttackKind k) {
    switch (k) {
        case AttackKind::SHOT: case AttackKind::BURST: case AttackKind::LUNGE: case AttackKind::DIVE:
        case AttackKind::LOB: case AttackKind::SHELL: case AttackKind::VOLLEY: case AttackKind::BASH:
        case AttackKind::BEAM: return true;
        default: return false;
    }
}

// Steps (or wingbeats) a second at full speed; 0: it doesn't move
inline float moveCadence(EnemyType t) {
    static const float C[] = {2.2f, 7.f, 1.5f, 3.f, 1.4f, 9.f, 1.2f, 1.f, 2.4f, 1.8f, 0.f, 2.f, 1.6f, 1.f, 0.7f};
    static_assert(sizeof(C) / sizeof(C[0]) == (size_t)EnemyType::COUNT, "a cadence per enemy type");
    return C[(int)t];
}

// How long a tell sounds: about the wind-up it announces
inline float tellDur(EnemyType t, AttackKind a) {
    switch (a) {
        case AttackKind::LUNGE: return 0.32f;   case AttackKind::BURST: return 0.75f;
        case AttackKind::DIVE: return 0.55f;    case AttackKind::LOB: return 0.7f;
        case AttackKind::FUSE: return 0.55f;    case AttackKind::SHELL: return 1.1f;
        case AttackKind::SMASH: return 0.95f;   case AttackKind::SUMMON: return 1.2f;
        case AttackKind::BASH: return 0.6f;     case AttackKind::BEAM: return 1.f;
        case AttackKind::CENSER_LOW: case AttackKind::CENSER_HIGH: return 0.9f;
        case AttackKind::PSLAM: return 1.f;     case AttackKind::PSTOMP: return 0.8f;
        case AttackKind::PLASH: return 0.7f;    case AttackKind::SCOURGE: return 0.8f;
        case AttackKind::JUDGMENT: return 0.65f; case AttackKind::PHANTOMS: return 0.6f;
        case AttackKind::LEAP: return 0.55f;    case AttackKind::RUPTURE: return 0.6f;
        case AttackKind::DASH: case AttackKind::SWEEP: case AttackKind::CLEAVE: case AttackKind::CRESCENT:
        case AttackKind::BLINK: case AttackKind::WHIRL: case AttackKind::THRUST: return 0.4f;
        default: return statsOf(t).telegraph > 0.f ? statsOf(t).telegraph : 0.6f;
    }
}

inline std::string voiceName(EnemyType t, VoiceKind k, AttackKind a = AttackKind::NONE) {
    const std::string n = std::string("v_") + voiceKey(t) + "_";
    switch (k) {
        case VoiceKind::SPAWN:  return n + "spawn";
        case VoiceKind::IDLE:   return n + "idle";
        case VoiceKind::MOVE:   return n + "move";
        case VoiceKind::HURT:   return n + "hurt";
        case VoiceKind::DEATH:  return n + "death";
        case VoiceKind::TELL:   return n + "tell_" + attackKey(a);
        case VoiceKind::ATTACK: return n + "atk_" + attackKey(a);
        default:                return "";
    }
}

inline const char* const VOICE_HALO_BREAK   = "v_halo_break";
inline const char* const VOICE_HALO_SHIMMER = "v_halo_shimmer";

struct VoiceSpec {
    std::string name;
    EnemyType   type;      // COUNT: shared by all
    VoiceKind   kind;
    AttackKind  attack;
    int         variants;
    MixClass    cls;
    const char* special = "";   // SPECIAL: which recipe
};

inline const std::vector<VoiceSpec>& voiceBank() {
    static const std::vector<VoiceSpec> bank = [] {
        std::vector<VoiceSpec> v;
        for (int i = 0; i < (int)EnemyType::COUNT; ++i) {
            const EnemyType t = (EnemyType)i;
            v.push_back({voiceName(t, VoiceKind::SPAWN), t, VoiceKind::SPAWN, AttackKind::NONE, 2, MixClass::WORLD});
            v.push_back({voiceName(t, VoiceKind::IDLE),  t, VoiceKind::IDLE,  AttackKind::NONE, 3, MixClass::CHATTER});
            if (moveCadence(t) > 0.f)
                v.push_back({voiceName(t, VoiceKind::MOVE), t, VoiceKind::MOVE, AttackKind::NONE, 3, MixClass::CHATTER});
            v.push_back({voiceName(t, VoiceKind::HURT),  t, VoiceKind::HURT,  AttackKind::NONE, 3, MixClass::FOLEY});
            v.push_back({voiceName(t, VoiceKind::DEATH), t, VoiceKind::DEATH, AttackKind::NONE, 2, MixClass::ACTION});
            for (AttackKind a : attacksOf(t)) {
                v.push_back({voiceName(t, VoiceKind::TELL, a), t, VoiceKind::TELL, a, 2, MixClass::TELL});
                if (hasRelease(a))
                    v.push_back({voiceName(t, VoiceKind::ATTACK, a), t, VoiceKind::ATTACK, a,
                                 a == AttackKind::BEAM ? 1 : 2, a == AttackKind::BEAM ? MixClass::TELL : MixClass::ACTION});
            }
        }
        auto sp = [&](const char* name, EnemyType t, int n, MixClass c, const char* key) {
            v.push_back({name, t, VoiceKind::SPECIAL, AttackKind::NONE, n, c, key});
        };
        sp(VOICE_HALO_BREAK,     EnemyType::COUNT,     1, MixClass::ACTION,  "halo_break");
        sp(VOICE_HALO_SHIMMER,   EnemyType::COUNT,     1, MixClass::CHATTER, "halo_shimmer");
        sp("v_warden_enrage",    EnemyType::WARDEN,    1, MixClass::TELL,    "enrage");
        sp("v_sovereign_enrage", EnemyType::SOVEREIGN, 1, MixClass::TELL,    "enrage");
        sp("v_penitent_rise",    EnemyType::PENITENT,  1, MixClass::TELL,    "rise");
        sp("v_conductor_link",   EnemyType::CONDUCTOR, 2, MixClass::FOLEY,   "link");
        return v;
    }();
    return bank;
}
```

- [ ] **Step 5: Write `src/VoiceSynth.h`**

```cpp
#pragma once
// =============================================================================
// VoiceSynth.h — every enemy voice, built in code when the game loads: the
// feel track's "machine + choir", a machine body carrying a hollow voice.
// No files: like MusicSynth and TextureGen, the sound is the recipe.
//
// Blocks: a formant CHOIR (detuned saws through a vowel's three formants),
// SERVO whine, VENT hiss, gear GRIND, METAL ring (damped inharmonic
// partials), CLICK, THUD, HUM, WINGS, SHIMMER. Each enemy type has a profile
// (its choir's pitch and vowel, its body, its size: small is high and quick,
// heavy is low and slow); each kind of voice a recipe over that profile; each
// attack its own tell. Built at RATE, normalised to its mix class's level
// (MixTable.h), deterministic (seeded by name and variant).
// =============================================================================
#include "VoiceTable.h"
#include <cmath>
#include <cstdint>
#include <string>
#include <vector>

namespace VoiceSynth {

constexpr float RATE = 22050.f;
constexpr float TAU  = 6.2831853f;
using Buf = std::vector<float>;

enum class Vowel : uint8_t { NONE, AH, OH, OO, EE, UH, EH };
enum class Body  : uint8_t { SERVO, SKITTER, HYDRAULIC, WINGS, HEAVY, TREADS, SCRAPE, CRACKLE, HUM, GRIND, GEARS, ARMOUR, CHAINS };

struct Profile {
    float f0 = 130.f;          // choir pitch, Hz (0: a cold machine, no choir)
    Vowel vowel = Vowel::OH;
    int   voices = 3;          // 1 a solo voice .. 6 a massed choir
    float formant = 1.f;       // formant shift (>1 smaller and brighter)
    Body  body = Body::SERVO;
    float bodyHz = 400.f;      // the machine layer's pitch
    float size = 0.3f;         // 0 small .. 1 colossal
};

inline Profile profileOf(EnemyType t) {
    switch (t) {
        case EnemyType::HUSK:         return {130.f, Vowel::OH,   3, 1.00f, Body::SERVO,      420.f, 0.30f};
        case EnemyType::RIPPER:       return {260.f, Vowel::AH,   2, 1.20f, Body::SKITTER,   1800.f, 0.15f};
        case EnemyType::SENTINEL:     return {  0.f, Vowel::NONE, 0, 1.00f, Body::HYDRAULIC,  300.f, 0.40f};
        case EnemyType::RAPTOR:       return {520.f, Vowel::EE,   2, 1.25f, Body::WINGS,      220.f, 0.20f};
        case EnemyType::BRUTE:        return { 70.f, Vowel::UH,   4, 0.85f, Body::HEAVY,      140.f, 0.70f};
        case EnemyType::MITE:         return {  0.f, Vowel::NONE, 0, 1.00f, Body::SKITTER,   2600.f, 0.05f};
        case EnemyType::JUGGERNAUT:   return { 55.f, Vowel::OO,   4, 0.80f, Body::TREADS,      90.f, 0.85f};
        case EnemyType::WARDEN:       return { 65.f, Vowel::OH,   6, 0.85f, Body::GEARS,      110.f, 0.90f};
        case EnemyType::SOVEREIGN:    return {110.f, Vowel::AH,   1, 0.95f, Body::ARMOUR,     900.f, 0.60f};
        case EnemyType::SHIELDBEARER: return { 98.f, Vowel::AH,   4, 0.95f, Body::SCRAPE,     600.f, 0.50f};
        case EnemyType::CONDUIT:      return {147.f, Vowel::AH,   5, 1.00f, Body::CRACKLE,   3000.f, 0.60f};
        case EnemyType::CONDUCTOR:    return {392.f, Vowel::EE,   3, 1.15f, Body::HUM,        240.f, 0.30f};
        case EnemyType::SERAPH:       return {440.f, Vowel::AH,   4, 1.20f, Body::WINGS,      180.f, 0.40f};
        case EnemyType::ANCHOR:       return { 49.f, Vowel::OO,   3, 0.80f, Body::GRIND,      120.f, 0.75f};
        case EnemyType::PENITENT:     return { 73.f, Vowel::OH,   6, 0.80f, Body::CHAINS,     160.f, 1.00f};
        default:                      return {};
    }
}

// ---- plumbing ------------------------------------------------------------------
struct Rng {
    uint32_t s;
    explicit Rng(uint32_t seed) : s(seed ? seed : 1u) {}
    float uni() { s ^= s << 13; s ^= s >> 17; s ^= s << 5; return (s & 0xFFFFFF) / 16777215.f; }
    float bi()  { return uni() * 2.f - 1.f; }
};
inline uint32_t seedOf(const std::string& name, int variant) {
    uint32_t h = 2166136261u;
    for (char c : name) { h ^= (uint8_t)c; h *= 16777619u; }
    h ^= (uint32_t)(variant + 1) * 0x9E3779B9u;
    return h ? h : 1u;
}
inline int N(float sec) { return std::max(1, (int)(sec * RATE)); }
// Mix b into a, `at` seconds in, scaled by g (a grows to fit)
inline void add(Buf& a, const Buf& b, float g = 1.f, float at = 0.f) {
    const size_t o = (size_t)std::max(0, (int)(at * RATE));
    if (a.size() < o + b.size()) a.resize(o + b.size(), 0.f);
    for (size_t i = 0; i < b.size(); ++i) a[o + i] += g * b[i];
}
// Linear attack, squared release
inline float env(int i, int n, float atk, float rel) {
    const float t = (float)i / RATE, left = (float)(n - i) / RATE;
    const float a = atk > 0.f ? std::min(1.f, t / atk) : 1.f;
    const float r = rel > 0.f ? std::min(1.f, left / rel) : 1.f;
    return a * r * r;
}
struct BandPass {
    float b0 = 0.f, b2 = 0.f, a1 = 0.f, a2 = 0.f, x1 = 0.f, x2 = 0.f, y1 = 0.f, y2 = 0.f;
    BandPass(float hz, float q) {
        const float w = TAU * std::min(hz, RATE * 0.45f) / RATE, al = std::sin(w) / (2.f * q), a0 = 1.f + al;
        b0 = al / a0; b2 = -al / a0; a1 = -2.f * std::cos(w) / a0; a2 = (1.f - al) / a0;
    }
    float operator()(float x) { float y = b0 * x + b2 * x2 - a1 * y1 - a2 * y2; x2 = x1; x1 = x; y2 = y1; y1 = y; return y; }
};
struct LowPass {
    float k, z = 0.f;
    explicit LowPass(float hz) : k(1.f - std::exp(-TAU * hz / RATE)) {}
    float operator()(float x) { z += k * (x - z); return z; }
};

// ---- blocks --------------------------------------------------------------------
inline const float* formantsOf(Vowel v) {
    static const float F[7][3] = {{0, 0, 0}, {730, 1090, 2440}, {570, 840, 2410}, {300, 870, 2240},
                                  {270, 2290, 3010}, {520, 1190, 2390}, {530, 1840, 2480}};
    return F[(int)v];
}
// `voices` detuned saws gliding f0 -> f1 through the vowel's formants
inline Buf choir(float dur, float f0, float f1, Vowel v, int voices, float shift, Rng& r,
                 float atk = 0.05f, float rel = 0.2f, float breath = 0.1f, float vib = 5.f) {
    const int n = N(dur);
    Buf out(n, 0.f);
    if (v == Vowel::NONE || voices <= 0 || f0 <= 0.f) return out;
    const float* F = formantsOf(v);
    BandPass b1(F[0] * shift, 5.f), b2(F[1] * shift, 7.f), b3(F[2] * shift, 9.f);
    std::vector<float> ph(voices), det(voices);
    for (int k = 0; k < voices; ++k) {
        ph[k] = r.uni();
        det[k] = 1.f + (voices > 1 ? 0.012f * (2.f * k / (voices - 1) - 1.f) : 0.f) + 0.003f * r.bi();
    }
    const float vibPh = r.uni() * TAU, ratio = f1 / f0, norm = 1.f / std::sqrt((float)voices);
    for (int i = 0; i < n; ++i) {
        const float f = f0 * std::pow(ratio, (float)i / n) * (1.f + 0.012f * std::sin(vibPh + TAU * vib * i / RATE));
        float src = 0.f;
        for (int k = 0; k < voices; ++k) { ph[k] += f * det[k] / RATE; ph[k] -= std::floor(ph[k]); src += 2.f * ph[k] - 1.f; }
        src = src * norm + breath * r.bi();
        out[i] = (b1(src) + 0.5f * b2(src) + 0.25f * b3(src)) * env(i, n, atk, rel);
    }
    return out;
}
inline Buf servo(float dur, float f0, float f1, Rng& r, float atk = 0.02f, float rel = 0.08f) {
    const int n = N(dur); Buf out(n); float ph = 0.f; LowPass lp(4000.f);
    for (int i = 0; i < n; ++i) {
        const float f = f0 + (f1 - f0) * (float)i / n;
        ph += f / RATE; ph -= std::floor(ph);
        out[i] = lp(0.6f * (ph < 0.5f ? 1.f : -1.f) + 0.4f * (2.f * ph - 1.f) + 0.15f * r.bi()) * env(i, n, atk, rel);
    }
    return out;
}
inline Buf vent(float dur, float hz, Rng& r, float atk = 0.01f, float rel = 0.15f) {
    const int n = N(dur); Buf out(n); BandPass bp(hz, 0.8f);
    for (int i = 0; i < n; ++i) out[i] = bp(r.bi()) * env(i, n, atk, rel);
    return out;
}
// Gear teeth: noise chopped `rate` times a second, rung at hz
inline Buf grind(float dur, float rate, float hz, Rng& r) {
    const int n = N(dur); Buf out(n); BandPass bp(hz, 2.f); float ph = 0.f;
    for (int i = 0; i < n; ++i) {
        ph += rate / RATE; ph -= std::floor(ph);
        out[i] = bp(r.bi() * (ph < 0.3f ? 1.f : 0.15f)) * 2.f * env(i, n, 0.03f, 0.1f);
    }
    return out;
}
inline Buf metal(float dur, float hz, Rng& r, float decay = 6.f) {
    static const float RT[] = {1.f, 2.76f, 5.40f, 8.93f}, GN[] = {1.f, 0.6f, 0.35f, 0.2f};
    const int n = N(dur); Buf out(n, 0.f);
    for (int p = 0; p < 4; ++p) {
        const float f = hz * RT[p] * (1.f + 0.004f * r.bi());
        if (f > RATE * 0.45f) continue;
        const float ph = r.uni() * TAU;
        for (int i = 0; i < n; ++i) { float t = (float)i / RATE; out[i] += GN[p] * std::sin(ph + TAU * f * t) * std::exp(-t * decay * (1.f + p * 0.5f)); }
    }
    for (int i = 0; i < n && i < 20; ++i) out[i] *= i / 20.f;
    return out;
}
inline Buf click(float hz, Rng& r, float dur = 0.03f) {
    const int n = N(dur); Buf out(n); BandPass bp(hz, 3.f);
    for (int i = 0; i < n; ++i) { float t = (float)i / n; out[i] = bp(r.bi()) * 3.f * (1.f - t) * (1.f - t); }
    return out;
}
inline Buf thud(float f0, float f1, float dur) {
    const int n = N(dur); Buf out(n); float ph = 0.f;
    for (int i = 0; i < n; ++i) {
        const float t = (float)i / RATE, f = f1 + (f0 - f1) * std::exp(-t * 18.f);
        ph += TAU * f / RATE;
        out[i] = std::sin(ph) * std::exp(-t * 6.f / dur) * std::min(1.f, i / 30.f);
    }
    return out;
}
inline Buf beep(float hz, float dur) {
    const int n = N(dur); Buf out(n);
    for (int i = 0; i < n; ++i) out[i] = std::sin(TAU * hz * i / RATE) * env(i, n, 0.002f, 0.01f);
    return out;
}
inline Buf hum(float dur, float hz, Rng& r) {
    const int n = N(dur); Buf out(n); const float ph = r.uni() * TAU;
    for (int i = 0; i < n; ++i) {
        const float t = (float)i / RATE;
        out[i] = (std::sin(TAU * hz * t) + 0.5f * std::sin(TAU * 2.f * hz * t + ph)) * (0.75f + 0.25f * std::sin(TAU * 3.f * t + ph)) * env(i, n, 0.05f, 0.1f);
    }
    return out;
}
inline Buf wings(float dur, float hz, Rng& r) {
    const int n = N(dur); Buf out(n); LowPass lp(hz * 4.f);
    for (int i = 0; i < n; ++i) {
        const float beat = 0.5f + 0.5f * std::sin(TAU * 6.f * i / RATE);
        out[i] = lp(r.bi()) * 3.f * beat * beat * env(i, n, 0.02f, 0.05f);
    }
    return out;
}
inline Buf shimmer(float dur, float hz, Rng& r) {
    const int n = N(dur); Buf out(n, 0.f);
    for (int k = 0; k < 4; ++k) {
        const float f = hz * (1.f + 0.13f * k + 0.01f * r.bi()), ph = r.uni() * TAU;
        for (int i = 0; i < n; ++i) out[i] += 0.25f * std::sin(ph + TAU * f * i / RATE) * (0.6f + 0.4f * std::sin(TAU * 9.f * i / RATE + k));
    }
    for (int i = 0; i < n; ++i) out[i] *= env(i, n, 0.05f, dur * 0.4f);
    return out;
}
inline Buf siren(float dur, float f0, float f1) {
    const int n = N(dur); Buf out(n); float ph = 0.f;
    for (int i = 0; i < n; ++i) { ph += (f0 * std::pow(f1 / f0, (float)i / n)) / RATE; ph -= std::floor(ph); out[i] = (2.f * std::fabs(2.f * ph - 1.f) - 1.f) * env(i, n, 0.05f, 0.05f); }
    return out;
}
inline Buf zap(float hz, Rng& r) {
    Buf out = metal(0.15f, hz, r, 25.f);
    add(out, servo(0.12f, hz * 1.5f, hz * 0.5f, r, 0.002f, 0.06f), 0.6f);
    return out;
}
inline Buf sing(float d, float f, float a, float b, Vowel v, const Profile& p, Rng& r, float atk = 0.03f) {
    return choir(d, f * a, f * b, v, p.voices, p.formant, r, atk, d * 0.3f, 0.15f);
}
inline Buf chord(float d, float f, const Profile& p, Rng& r, float r1, float r2, float r3, float atk = 0.1f) {
    const int v = std::max(2, p.voices / 2);
    Buf out = choir(d, f * r1, f * r1, p.vowel, v, p.formant, r, atk, d * 0.3f);
    add(out, choir(d, f * r2, f * r2, p.vowel, v, p.formant, r, atk, d * 0.3f));
    add(out, choir(d, f * r3, f * r3, p.vowel, v, p.formant, r, atk, d * 0.3f));
    return out;
}

// The type's machine layer, `dur` long
inline Buf body(const Profile& p, float dur, Rng& r) {
    const float h = p.bodyHz;
    Buf out(N(dur), 0.f);
    switch (p.body) {
        case Body::SERVO:     add(out, servo(dur, h, h * 1.15f, r)); break;
        case Body::SKITTER:   for (float t = 0.f; t < dur - 0.03f; t += 0.025f + 0.03f * r.uni()) add(out, click(h * (0.8f + 0.4f * r.uni()), r), 0.8f, t); break;
        case Body::HYDRAULIC: add(out, vent(dur, 900.f, r, dur * 0.3f, dur * 0.5f), 0.7f); add(out, servo(dur, h, h * 0.8f, r), 0.4f); break;
        case Body::WINGS:     add(out, wings(dur, h, r)); break;
        case Body::HEAVY:     add(out, servo(dur, h, h * 0.9f, r), 0.6f); add(out, grind(dur, 20.f, h * 3.f, r), 0.5f); break;
        case Body::TREADS:    add(out, grind(dur, 35.f, 300.f, r), 0.7f); add(out, thud(h, h * 0.6f, std::min(dur, 0.3f)), 0.5f); break;
        case Body::SCRAPE:    add(out, vent(dur, 2500.f, r, 0.02f, dur * 0.6f), 0.6f); add(out, metal(dur, h, r, 10.f), 0.3f); break;
        case Body::CRACKLE:   for (float t = 0.f; t < dur - 0.03f; t += 0.02f + 0.08f * r.uni()) add(out, click(h * (0.6f + 0.8f * r.uni()), r, 0.015f), 0.6f, t);
                              add(out, hum(dur, 60.f, r), 0.4f); break;
        case Body::HUM:       add(out, hum(dur, h, r)); break;
        case Body::GRIND:     add(out, grind(dur, 12.f, h * 3.f, r)); break;
        case Body::GEARS:     add(out, grind(dur, 18.f, 400.f, r), 0.7f); add(out, metal(dur, h * 2.f, r, 4.f), 0.4f); break;
        case Body::ARMOUR:    for (float t = 0.f; t < dur - 0.05f; t += 0.06f + 0.06f * r.uni()) add(out, metal(0.08f, h * (0.9f + 0.2f * r.uni()), r, 40.f), 0.4f, t); break;
        case Body::CHAINS:    for (float t = 0.f; t < dur - 0.08f; t += 0.05f + 0.07f * r.uni()) add(out, metal(0.12f, h * (2.f + 2.f * r.uni()), r, 30.f), 0.5f, t); break;
    }
    out.resize(N(dur));
    return out;
}

// ---- kinds ---------------------------------------------------------------------
inline Buf spawnV(const Profile& p, Rng& r) {   // a rising breath and the machine waking
    const float d = 0.45f + 0.35f * p.size;
    Buf out = choir(d, p.f0 * 0.7f, p.f0, p.vowel, p.voices, p.formant, r, d * 0.6f, 0.12f, 0.35f);
    add(out, body(p, d, r), p.f0 > 0.f ? 0.5f : 1.f);
    add(out, metal(0.3f, p.bodyHz * 2.f, r, 12.f), 0.3f, d * 0.6f);
    return out;
}
inline Buf idleV(const Profile& p, Rng& r) {    // a murmur over the hum of the body
    const float d = 0.5f + 0.5f * p.size;
    Buf out = choir(d, p.f0 * (0.98f + 0.04f * r.uni()), p.f0 * 0.95f, p.vowel, p.voices, p.formant * (0.95f + 0.1f * r.uni()), r, 0.15f, 0.3f, 0.2f, 3.f);
    add(out, body(p, d, r), p.f0 > 0.f ? 0.35f : 1.f);
    return out;
}
inline Buf moveV(const Profile& p, Rng& r) {    // one step / wingbeat
    const float d = 0.08f + 0.17f * p.size;
    Buf out(N(d), 0.f);
    switch (p.body) {
        case Body::SKITTER: for (int k = 0; k < 3; ++k) add(out, click(p.bodyHz * (0.8f + 0.4f * r.uni()), r), 0.8f, k * 0.02f); break;
        case Body::WINGS:   add(out, wings(d * 2.f, p.bodyHz, r)); break;
        case Body::HUM: case Body::CRACKLE: add(out, hum(d * 2.f, p.bodyHz, r), 0.6f); break;
        case Body::TREADS: case Body::GRIND: add(out, grind(d * 1.5f, 30.f, 250.f, r), 0.7f); add(out, thud(p.bodyHz, p.bodyHz * 0.6f, d), 0.6f); break;
        case Body::ARMOUR: case Body::CHAINS: add(out, metal(d, p.bodyHz * 1.5f, r, 25.f), 0.5f); add(out, thud(90.f + 60.f * (1.f - p.size), 50.f, d), 0.8f); break;
        default: add(out, servo(d, p.bodyHz, p.bodyHz * 1.3f, r, 0.005f, 0.04f), 0.5f); add(out, thud(110.f - 50.f * p.size, 50.f - 20.f * p.size, d), 0.9f); break;
    }
    return out;
}
inline Buf hurtV(const Profile& p, Rng& r) {    // a short choked note
    const float d = 0.16f + 0.14f * p.size;
    Buf out = choir(d, p.f0 * 1.3f, p.f0 * 0.9f, p.vowel, std::max(1, p.voices / 2), p.formant, r, 0.005f, 0.08f, 0.25f);
    add(out, click(p.bodyHz * 2.f, r), 0.6f);
    if (p.f0 <= 0.f) { add(out, metal(d, p.bodyHz * 2.f, r, 20.f), 0.8f); add(out, servo(d, p.bodyHz * 1.5f, p.bodyHz, r, 0.002f, 0.05f), 0.4f); }
    return out;
}
inline Buf deathV(const Profile& p, Rng& r) {   // the choir cut off, the machine winding down
    const float d = 0.55f + 0.65f * p.size;
    Buf out = choir(d * 0.6f, p.f0, p.f0 * 0.55f, p.vowel, p.voices, p.formant, r, 0.01f, 0.03f, 0.3f);
    add(out, servo(d, p.bodyHz, p.bodyHz * 0.15f, r, 0.01f, d * 0.5f), 0.5f);
    add(out, thud(120.f - 60.f * p.size, 40.f, 0.35f), 0.7f, d * 0.55f);
    add(out, metal(0.4f, p.bodyHz * 2.f, r, 8.f), 0.3f, d * 0.55f);
    return out;
}
inline Buf tellV(EnemyType t, AttackKind a, const Profile& p, Rng& r) {
    const float d = tellDur(t, a), f = p.f0 > 0.f ? p.f0 : 200.f;
    Buf out(N(d), 0.f);
    switch (a) {
        case AttackKind::SHOT:     // a rising hum
            add(out, choir(d, f * 0.8f, f * 1.25f, p.vowel, p.voices, p.formant, r, d * 0.7f, 0.04f, 0.1f));
            add(out, servo(d, p.bodyHz * 0.8f, p.bodyHz * 1.6f, r, d * 0.7f, 0.04f), 0.35f); break;
        case AttackKind::LUNGE:    // a sharp hiss into the lunge
            add(out, vent(d, 3200.f, r, d * 0.8f, 0.03f));
            add(out, choir(d, f, f * 1.2f, p.vowel, 2, p.formant, r, d * 0.5f, 0.03f, 0.6f), 0.5f); break;
        case AttackKind::BURST:    // a whine climbing over the paint, a click at fire
            add(out, servo(d, 500.f, 2400.f, r, d * 0.5f, 0.02f), 0.7f);
            add(out, click(3000.f, r), 1.f, d - 0.03f); break;
        case AttackKind::DIVE:     // a falling shriek
            add(out, choir(d, f * 1.6f, f * 0.8f, Vowel::EE, p.voices, p.formant, r, 0.05f, 0.1f, 0.2f));
            add(out, vent(d, 1500.f, r, d * 0.8f, 0.05f), 0.5f); break;
        case AttackKind::SLAM: case AttackKind::PSLAM:   // a huge inhale, then the gears grind
            add(out, vent(d * 0.6f, 700.f, r, d * 0.55f, 0.05f), 0.9f);
            add(out, choir(d * 0.6f, f * 0.9f, f * 1.1f, p.vowel, p.voices, p.formant * 0.9f, r, d * 0.5f, 0.05f, 0.6f), 0.5f);
            add(out, grind(d * 0.4f, 25.f, p.bodyHz * 3.f, r), 0.8f, d * 0.6f); break;
        case AttackKind::LOB:      // a short hum and a click
            add(out, choir(d, f, f * 1.15f, p.vowel, p.voices, p.formant, r, d * 0.6f, 0.05f));
            add(out, click(800.f, r), 0.8f, d - 0.04f); break;
        case AttackKind::FUSE:     // beeps, faster and faster
            for (float tt = 0.f, gap = 0.12f; tt < d - 0.02f; tt += gap, gap = std::max(0.03f, gap * 0.72f)) add(out, beep(2200.f, 0.025f), 1.f, tt);
            break;
        case AttackKind::SHELL:    // a siren rising over the choir's drone
            add(out, siren(d, 300.f, 900.f));
            add(out, choir(d, f, f, p.vowel, p.voices, p.formant, r, 0.1f, 0.1f), 0.5f); break;
        case AttackKind::SMASH:    // a ratchet winding up
            for (float tt = 0.f, gap = 0.09f; tt < d - 0.02f; tt += gap, gap = std::max(0.025f, gap * 0.85f)) add(out, click(1400.f, r), 0.7f, tt);
            add(out, grind(d, 15.f, 300.f, r), 0.5f); break;
        case AttackKind::BASH:     // a shield clang, then the war cry
            add(out, metal(0.3f, 600.f, r, 10.f), 0.8f);
            add(out, choir(d - 0.12f, f * 1.4f, f * 1.1f, Vowel::AH, p.voices, p.formant, r, 0.02f, 0.05f, 0.3f), 1.f, 0.12f); break;
        case AttackKind::BEAM:     // the chord brightens through the charge
            add(out, choir(d, f, f * 1.5f, Vowel::AH, p.voices, p.formant, r, d * 0.4f, 0.05f, 0.1f, 6.f));
            add(out, shimmer(d, 2000.f, r), 0.4f); break;
        case AttackKind::VOLLEY:
            if (t == EnemyType::ANCHOR) {   // a muffled thump, then the lob's pop
                add(out, thud(60.f, 40.f, 0.35f), 0.8f);
                add(out, choir(d, f, f, p.vowel, p.voices, p.formant, r, d * 0.5f, 0.05f), 0.6f);
                add(out, click(500.f, r), 0.6f, d - 0.04f);
            } else add(out, chord(d, f, p, r, 1.f, 1.2f, 1.5f));   // the Warden: a minor chord
            break;
        case AttackKind::SUMMON:   // a swell from below
            add(out, choir(d, f * 0.5f, f, p.vowel, 6, p.formant, r, d * 0.9f, 0.05f, 0.2f));
            add(out, grind(d, 10.f, 300.f, r), 0.4f); break;
        // THE SOVEREIGN: a sung syllable for every stroke
        case AttackKind::SWEEP:    add(out, sing(d, f, 1.0f, 1.3f, Vowel::AH, p, r)); break;
        case AttackKind::CRESCENT: add(out, sing(d, f, 1.3f, 0.9f, Vowel::AH, p, r)); break;
        case AttackKind::CLEAVE:   add(out, sing(d, f, 0.9f, 1.5f, Vowel::OH, p, r)); break;
        case AttackKind::DASH:     add(out, sing(d, f, 1.2f, 1.0f, Vowel::EH, p, r, 0.005f)); add(out, vent(d, 2000.f, r, d, 0.02f), 0.4f); break;
        case AttackKind::LEAP:     add(out, sing(d, f, 1.0f, 1.8f, Vowel::EE, p, r)); break;
        case AttackKind::BLINK:    add(out, choir(d, f, f, Vowel::EH, 1, p.formant, r, 0.02f, d * 0.4f, 1.5f)); break;   // a whisper
        case AttackKind::WHIRL:    add(out, choir(d, f, f, Vowel::OO, 1, p.formant, r, 0.03f, d * 0.3f, 0.1f, 14.f)); add(out, wings(d, 400.f, r), 0.4f); break;
        case AttackKind::THRUST:   add(out, sing(d * 0.5f, f, 1.1f, 1.0f, Vowel::EH, p, r, 0.003f));
                                   add(out, sing(d * 0.4f, f, 1.3f, 1.3f, Vowel::AH, p, r, 0.003f), 1.f, d * 0.55f); break;
        case AttackKind::RUPTURE:  add(out, sing(d, f, 0.7f, 1.0f, Vowel::UH, p, r)); add(out, grind(d, 20.f, 250.f, r), 0.6f); break;
        case AttackKind::JUDGMENT: add(out, chord(d, f, p, r, 1.f, 1.25f, 1.5f)); break;
        case AttackKind::PHANTOMS: add(out, chord(d, f, p, r, 1.f, 1.19f, 1.41f)); break;
        // THE PENITENT
        case AttackKind::CENSER_LOW:   // a low groan
            add(out, choir(d, f, f * 0.8f, Vowel::OO, p.voices, p.formant, r, d * 0.4f, 0.1f, 0.2f));
            add(out, grind(d, 14.f, 200.f, r), 0.6f); break;
        case AttackKind::CENSER_HIGH:  // a bell
            add(out, metal(d, 660.f, r, 2.f), 0.8f); add(out, metal(d, 990.f, r, 2.5f), 0.5f);
            add(out, choir(d, f * 4.f, f * 4.f, Vowel::EE, 3, 1.2f, r, d * 0.5f, 0.1f), 0.3f); break;
        case AttackKind::PSTOMP:       // two heavy beats
            add(out, thud(65.f, 35.f, 0.3f)); add(out, thud(65.f, 35.f, 0.3f), 1.f, d * 0.5f); break;
        case AttackKind::PLASH:        // the chains rattle, a whine rises
            add(out, body(p, d, r), 0.8f); add(out, servo(d, 300.f, 1500.f, r, d * 0.6f, 0.03f), 0.5f); break;
        case AttackKind::SCOURGE:      // a chant, three syllables
            for (int k = 0; k < 3; ++k)
                add(out, sing(d / 3.f, f, 1.f + 0.1f * k, 1.f + 0.1f * k, k % 2 ? Vowel::AH : Vowel::OH, p, r), 1.f, k * d / 3.f);
            break;
        default: add(out, choir(d, f, f * 1.2f, p.vowel, p.voices, p.formant, r, d * 0.6f, 0.05f)); break;
    }
    out.resize(N(d));
    return out;
}
inline Buf attackV(AttackKind a, const Profile& p, Rng& r) {
    const float f = p.f0 > 0.f ? p.f0 : 200.f;
    Buf out;
    switch (a) {
        case AttackKind::SHOT:  add(out, zap(p.bodyHz * 2.f, r)); break;
        case AttackKind::BURST: for (int k = 0; k < 3; ++k) add(out, zap(1800.f, r), 0.9f, k * 0.07f); break;
        case AttackKind::LUNGE: case AttackKind::DIVE: case AttackKind::BASH:
            add(out, vent(a == AttackKind::DIVE ? 0.4f : 0.25f, 1200.f, r, 0.04f, 0.15f));
            add(out, choir(0.2f, f * 1.2f, f, p.vowel, std::max(1, p.voices / 2), p.formant, r, 0.005f, 0.1f, 0.4f), 0.7f);
            if (a == AttackKind::BASH) add(out, metal(0.25f, 500.f, r, 14.f), 0.7f);
            break;
        case AttackKind::LOB:   add(out, thud(180.f, 90.f, 0.15f)); add(out, click(700.f, r), 0.6f); break;
        case AttackKind::SHELL: add(out, thud(70.f, 35.f, 0.5f)); add(out, vent(0.4f, 400.f, r, 0.005f, 0.3f), 0.7f); break;
        case AttackKind::VOLLEY:
            for (int k = 0; k < 3; ++k) { add(out, thud(160.f, 80.f, 0.12f), 0.8f, k * 0.06f); add(out, click(900.f, r), 0.5f, k * 0.06f); }
            break;
        case AttackKind::BEAM:  // held while the beam sweeps (3 s), stopped when it ends
            add(out, choir(3.f, f * 1.5f, f * 1.5f, Vowel::AH, p.voices, p.formant * 1.1f, r, 0.08f, 0.3f, 0.1f, 6.f));
            add(out, hum(3.f, 220.f, r), 0.5f); add(out, shimmer(3.f, 2400.f, r), 0.3f); break;
        default: add(out, click(1000.f, r)); break;
    }
    return out;
}
inline Buf specialV(const VoiceSpec& s, Rng& r) {
    const std::string k = s.special;
    if (k == "halo_break") {   // glass: many bright partials and a burst
        Buf out;
        for (int i = 0; i < 8; ++i) add(out, metal(0.5f, 2000.f + 3000.f * r.uni(), r, 8.f + 8.f * r.uni()), 0.4f, 0.01f * i);
        add(out, vent(0.3f, 5000.f, r, 0.002f, 0.25f), 0.6f);
        return out;
    }
    if (k == "halo_shimmer") return shimmer(0.4f, 3000.f, r);
    const Profile p = profileOf(s.type);
    if (k == "enrage") { Buf out = chord(1.5f, p.f0, p, r, 1.f, 1.5f, 2.f, 0.4f); add(out, grind(1.5f, 22.f, 300.f, r), 0.5f); return out; }
    if (k == "rise")   { Buf out = choir(2.f, p.f0 * 0.75f, p.f0, p.vowel, 6, p.formant, r, 1.2f, 0.4f, 0.2f); add(out, body(p, 2.f, r), 0.6f); return out; }
    if (k == "link")   return choir(0.25f, p.f0, p.f0 * 1.5f, Vowel::EE, 2, p.formant, r, 0.02f, 0.08f, 0.05f);
    return Buf(N(0.1f), 0.f);
}

// To the class's level: scale, round off anything over 0.8 (never past 0.95), repeat
inline void normalise(Buf& x, float targetDb) {
    const int fade = std::min((int)x.size(), N(0.005f));   // nothing ends on a click
    for (int i = 0; i < fade; ++i) x[x.size() - 1 - i] *= (float)i / fade;
    for (int pass = 0; pass < 3; ++pass) {
        const float lv = shortTermDb(x.data(), x.size(), RATE);
        if (lv <= -119.f) return;
        const float g = std::pow(10.f, (targetDb - lv) / 20.f);
        for (float& v : x) {
            v *= g;
            const float a = std::fabs(v);
            if (a > 0.8f) v = (v < 0.f ? -1.f : 1.f) * (0.8f + 0.15f * std::tanh((a - 0.8f) / 0.15f));
        }
    }
}

inline Buf build(const VoiceSpec& s, int variant) {
    Rng r(seedOf(s.name, variant));
    Profile p = s.type == EnemyType::COUNT ? Profile{} : profileOf(s.type);
    const float wob = 1.f + 0.03f * r.bi();   // each variant a little different
    p.f0 *= wob; p.bodyHz *= wob;
    Buf out;
    switch (s.kind) {
        case VoiceKind::SPAWN:   out = spawnV(p, r); break;
        case VoiceKind::IDLE:    out = idleV(p, r); break;
        case VoiceKind::MOVE:    out = moveV(p, r); break;
        case VoiceKind::TELL:    out = tellV(s.type, s.attack, p, r); break;
        case VoiceKind::ATTACK:  out = attackV(s.attack, p, r); break;
        case VoiceKind::HURT:    out = hurtV(p, r); break;
        case VoiceKind::DEATH:   out = deathV(p, r); break;
        case VoiceKind::SPECIAL: out = specialV(s, r); break;
    }
    normalise(out, mixTargetDb(s.cls));
    return out;
}

}   // namespace VoiceSynth
```

- [ ] **Step 6: Makefile** — add `src/MixTable.h src/VoiceTable.h src/VoiceSynth.h` to `HEADERS` and to the `test:` prerequisites.

- [ ] **Step 7: Run to verify it passes**

Run: `make test 2>&1 | grep -E "FAIL|voice bank|ALL PASSED|FAILED"`
Expected: `voice bank: ~125 names, N s of audio` with N ≤ 150, all voice CHECKs ok, `ALL PASSED`.
If a voice misses its level by > 2 dB, the cause is a recipe whose peaks the soft clip shaves (crest factor > ~14 dB): shorten its transient or add sustain to that recipe (not the tolerance), and ledger it. If an attack has no tell, add it to `attacksOf` and give it a `tellV` case.

- [ ] **Step 8: Listen.** Write a throwaway dumper in the scratchpad (not committed) that builds every voice and writes WAVs, then play a handful (`afplay`): Husk tell, Brute slam tell, Seraph beam, Warden volley, Sovereign sweep/cleave, Penitent censer low/high, a death, a hurt. Adjust recipes where a voice is clearly wrong for its line in the spec's table (character, register); ledger any recipe change.

```cpp
// scratchpad/dumpvoices.cpp — g++ -std=c++17 -I<repo>/src -I$(brew --prefix glm)/include dumpvoices.cpp -o dumpvoices
#include "VoiceSynth.h"
#include <cstdio>
int main() {
    for (const auto& s : voiceBank()) {
        auto b = VoiceSynth::build(s, 0);
        std::string p = s.name + ".wav"; FILE* f = std::fopen(p.c_str(), "wb");
        uint32_t n = (uint32_t)b.size(), data = n * 2, rate = 22050, br = rate * 2, riff = 36 + data, fl = 16; uint16_t pcm = 1, ch = 1, al = 2, bits = 16;
        std::fwrite("RIFF", 1, 4, f); std::fwrite(&riff, 4, 1, f); std::fwrite("WAVEfmt ", 1, 8, f); std::fwrite(&fl, 4, 1, f);
        std::fwrite(&pcm, 2, 1, f); std::fwrite(&ch, 2, 1, f); std::fwrite(&rate, 4, 1, f); std::fwrite(&br, 4, 1, f); std::fwrite(&al, 2, 1, f); std::fwrite(&bits, 2, 1, f);
        std::fwrite("data", 1, 4, f); std::fwrite(&data, 4, 1, f);
        for (float v : b) { int16_t q = (int16_t)(std::max(-1.f, std::min(1.f, v)) * 32767.f); std::fwrite(&q, 2, 1, f); }
        std::fclose(f);
    }
}
```

- [ ] **Step 9: Commit**

```bash
git add src/MixTable.h src/VoiceTable.h src/VoiceSynth.h Makefile tests/test_game.cpp
git commit -m "The enemy voice bank, built in code: machine bodies carrying a hollow choir, a voice for every kind and a tell for every attack"
```

---

### Task 3: The VoiceDirector

**Files:**
- Create: `src/EnemyVoice.h`
- Modify: `Makefile` (HEADERS, test deps: `src/EnemyVoice.h`), `tests/test_game.cpp`

**Interfaces:**
- Consumes: `voiceName`, `moveCadence`, `VOICE_HALO_*`, `voiceKey` (Task 2); `SoundRole` (Task 1).
- Produces: `struct VoiceIn`, `struct VoiceCue` (fields as below), `class VoiceDirector { reset(); begin(glm::vec3 listener, float dt); enemy(const VoiceIn&); const std::vector<VoiceCue>& end(); std::vector<VoiceCue> death(const VoiceIn&); int tracked() const; }`.

- [ ] **Step 1: Write the failing tests** — add `#include "../src/EnemyVoice.h"` at the top, append:

```cpp
    // ---------------------------------------------------------------- enemy voices: who gets to speak
    {
        auto in = [](int uid, EnemyType t, glm::vec3 p) { VoiceIn v; v.uid = uid; v.type = t; v.pos = p; v.health = 100.f; return v; };
        auto count = [](const std::vector<VoiceCue>& cs, SoundRole r, const std::string& part = "") {
            int n = 0; for (auto& c : cs) n += c.role == r && c.name.find(part) != std::string::npos; return n;
        };
        auto windup = [](VoiceIn v, AttackKind a, float t = 0.45f) { v.attack = a; v.telegraphTimer = t; v.telegraphStarted = true; return v; };
        const glm::vec3 O{0.f};
        const float F = 1.f / 60.f;
        {   // a wind-up 35 m off, behind a wall: heard, by name, at priority
            VoiceDirector d;
            d.begin(O, F); d.enemy(in(1, EnemyType::HUSK, {35, 0, 0})); d.end();
            d.begin(O, F); d.enemy(windup(in(1, EnemyType::HUSK, {35, 0, 0}), AttackKind::SHOT));
            auto cs = d.end();
            CHECK(count(cs, SoundRole::TELL, "v_husk_tell_shot") == 1 && cs[0].priority, "a wind-up 35 m off, out of sight, is heard: its own tell, at priority");
        }
        {   // the same enemy: not twice within 0.15 s
            VoiceDirector d; int tells = 0;
            for (int f = 0; f < 7; ++f) {
                d.begin(O, F); VoiceIn v = in(1, EnemyType::RIPPER, {5, 0, 0});
                if (f == 1 || f == 6) v = windup(v, AttackKind::LUNGE, 0.3f);
                d.enemy(v); tells += count(d.end(), SoundRole::TELL);
            }
            CHECK(tells == 1, "the same enemy can't repeat its tell within 0.15 s");
        }
        {   // six at once: the nearest four
            VoiceDirector d; d.begin(O, F); for (int i = 0; i < 6; ++i) d.enemy(in(i + 1, EnemyType::HUSK, {5.f + 5.f * i, 0, 0})); d.end();
            d.begin(O, F); for (int i = 0; i < 6; ++i) d.enemy(windup(in(i + 1, EnemyType::HUSK, {5.f + 5.f * i, 0, 0}), AttackKind::SHOT));
            int n = 0; bool nearest = true; for (auto& c : d.end()) if (c.role == SoundRole::TELL) { ++n; nearest &= c.uid <= 4; }
            CHECK(n == 4 && nearest, "six wind-ups at once: the nearest four are heard");
        }
        {   // chatter: only the nearest four, and they do chatter
            VoiceDirector d(7); std::set<int> spoke;
            for (int f = 0; f < 60 * 20; ++f) {
                d.begin(O, F);
                for (int i = 0; i < 10; ++i) { VoiceIn v = in(i + 1, EnemyType::HUSK, {2.f + 2.f * i, 0, 0}); v.moveSpeed = statsOf(EnemyType::HUSK).speed; d.enemy(v); }
                for (auto& c : d.end()) if (c.role == SoundRole::CHATTER && c.name.find("spawn") == std::string::npos) spoke.insert(c.uid);
            }
            CHECK(spoke.size() == 4 && *spoke.rbegin() <= 4, "ten enemies close by: the nearest four chatter, no one else");
        }
        {   // hurts: one per enemy per 0.4 s
            VoiceDirector d; int hurts = 0; float hp = 100.f;
            for (int f = 0; f < 60; ++f) {
                d.begin(O, F); VoiceIn v = in(1, EnemyType::BRUTE, {5, 0, 0});
                if (f >= 1 && f <= 5) hp -= 8.f;
                v.health = hp; d.enemy(v); hurts += count(d.end(), SoundRole::ACTION, "hurt");
            }
            CHECK(hurts == 1, "pellets landing over a few frames: one hurt, not eight");
        }
        {   // hurts: six a second at most
            VoiceDirector d; d.begin(O, F); for (int i = 0; i < 20; ++i) d.enemy(in(i + 1, EnemyType::HUSK, {3.f + i, 0, 0})); d.end();
            d.begin(O, F); for (int i = 0; i < 20; ++i) { VoiceIn v = in(i + 1, EnemyType::HUSK, {3.f + i, 0, 0}); v.health = 50.f; d.enemy(v); }
            CHECK(count(d.end(), SoundRole::ACTION, "hurt") == 6, "twenty hit at once: six hurts a second at most");
        }
        {   // the release: when the wind-up runs out (not when it's broken off)
            auto run = [&](bool stagger) {
                VoiceDirector d; int atk = 0;
                for (int f = 0; f < 40; ++f) {
                    d.begin(O, F); VoiceIn v = in(1, EnemyType::HUSK, {6, 0, 0});
                    if (f >= 1 && f < 28) { v.attack = AttackKind::SHOT; v.telegraphTimer = 0.45f - (f - 1) * F; v.telegraphStarted = f == 1; }
                    else if (f >= 28) { v.attack = stagger ? AttackKind::NONE : AttackKind::SHOT; v.staggered = stagger; }
                    d.enemy(v); atk += count(d.end(), SoundRole::ACTION, "v_husk_atk_shot");
                }
                return atk;
            };
            CHECK(run(false) == 1 && run(true) == 0, "the release sounds when the wind-up runs out, not when a stagger breaks it off");
        }
        {   // beam held, death cries at any range and ends it, the dead are forgotten
            VoiceDirector d; d.begin(O, F); d.enemy(in(1, EnemyType::SERAPH, {80, 0, 0})); d.end();
            d.begin(O, F); VoiceIn v = in(1, EnemyType::SERAPH, {80, 0, 0}); v.beamOn = true; d.enemy(v); auto start = d.end();
            auto cs = d.death(v);
            bool started = false, cry = false, stop = false;
            for (auto& c : start) started |= c.loop == VoiceCue::START && c.name == "v_seraph_atk_beam";
            for (auto& c : cs) { cry |= c.name == "v_seraph_death" && c.floor >= 0.5f; stop |= c.loop == VoiceCue::STOP && c.uid == 1; }
            CHECK(started, "a Seraph's beam starts a held voice");
            CHECK(cry && stop, "a death always cries out (any range, with a floor) and ends its held voice");
            d.begin(O, F); d.end();
            CHECK(d.tracked() == 0, "the dead are forgotten");
        }
        {   // Review focus 5: a beam that ends and starts again
            VoiceDirector d; int starts = 0, stops = 0;
            for (int f = 0; f < 4; ++f) {
                d.begin(O, F); VoiceIn v = in(1, EnemyType::SERAPH, {10, 0, 0}); v.beamOn = f == 1 || f == 3; d.enemy(v);
                for (auto& c : d.end()) { starts += c.loop == VoiceCue::START; stops += c.loop == VoiceCue::STOP; }
            }
            CHECK(starts == 2 && stops == 1, "a beam off and on again: stopped, then a fresh held voice");
        }
        {   // gone without a death: forgotten after half a second, its held voice stopped
            VoiceDirector d; d.begin(O, F); VoiceIn v = in(1, EnemyType::SERAPH, {10, 0, 0}); v.beamOn = true; d.enemy(v); d.end();
            bool stop = false;
            for (int f = 0; f < 40; ++f) { d.begin(O, F); for (auto& c : d.end()) stop |= c.loop == VoiceCue::STOP && c.uid == 1; }
            CHECK(d.tracked() == 0 && stop, "an enemy that vanishes is forgotten and its held voice stopped");
        }
        {   // Review focus 1: a retry forgets everyone at once
            VoiceDirector d; d.begin(O, F); d.enemy(in(1, EnemyType::HUSK, {5, 0, 0})); d.end();
            d.reset();
            CHECK(d.tracked() == 0, "reset forgets everyone");
        }
        {   // Review focus 2: a frame's gap is not a new enemy
            VoiceDirector d; int spawns = 0;
            for (int f = 0; f < 3; ++f) { d.begin(O, F); if (f != 1) d.enemy(in(1, EnemyType::HUSK, {5, 0, 0})); spawns += count(d.end(), SoundRole::CHATTER, "spawn"); }
            CHECK(spawns == 1, "an enemy missing for a frame and back again doesn't spawn twice");
        }
        {   // Review focus 3: killed after it was reported this frame
            VoiceDirector d; d.begin(O, F); d.enemy(in(1, EnemyType::HUSK, {5, 0, 0})); d.end();
            d.begin(O, F); VoiceIn v = in(1, EnemyType::HUSK, {5, 0, 0}); v.health = 0.f; d.enemy(v);
            auto deathCues = d.death(v);
            auto cs = d.end();
            CHECK(count(deathCues, SoundRole::ACTION, "death") == 1 && cs.empty() && d.tracked() == 0,
                  "killed mid-frame: its death cry and nothing else (no spawn, no hurt)");
        }
        {   // Hollowed: same name, changed voice
            auto tellOf = [&](Hollow h) {
                VoiceDirector d; VoiceIn v = in(1, EnemyType::HUSK, {5, 0, 0}); v.hollow = h; v.halo = h == Hollow::HALOED;
                d.begin(O, F); d.enemy(v); d.end();
                d.begin(O, F); d.enemy(windup(v, AttackKind::SHOT)); return d.end();
            };
            auto en = tellOf(Hollow::ENRAGED), tw = tellOf(Hollow::TWINNED), ha = tellOf(Hollow::HALOED);
            int twins = 0; float delay = 0.f; bool shimmer = false;
            for (auto& c : tw) if (c.name == "v_husk_tell_shot") { ++twins; delay = std::max(delay, c.delay); }
            for (auto& c : ha) shimmer |= c.name == VOICE_HALO_SHIMMER;
            CHECK(!en.empty() && en[0].name == "v_husk_tell_shot" && en[0].pitch > 1.1f && en[0].drive > 0.f, "an Enraged voice: the same tell, higher and driven");
            CHECK(twins == 2 && std::fabs(delay - 0.012f) < 1e-4f, "a Twinned voice is doubled, the copy 12 ms late");
            CHECK(shimmer, "a Haloed one's tell shimmers");
            VoiceDirector d; VoiceIn v = in(1, EnemyType::HUSK, {5, 0, 0}); v.hollow = Hollow::HALOED; v.halo = true;
            d.begin(O, F); d.enemy(v); d.end();
            v.halo = false; d.begin(O, F); d.enemy(v);
            CHECK(count(d.end(), SoundRole::ACTION, VOICE_HALO_BREAK) == 1, "a halo breaking shatters");
        }
        {   // Review focus 4: a dozen appearing at once
            VoiceDirector d; d.begin(O, F); for (int i = 0; i < 12; ++i) d.enemy(in(i + 1, EnemyType::MITE, {3.f + i, 0, 0}));
            int n = 0; bool nearest = true; for (auto& c : d.end()) if (c.name.find("spawn") != std::string::npos) { ++n; nearest &= c.uid <= 3; }
            CHECK(n == 3 && nearest, "a dozen appearing at once: three spawn cues, the nearest");
        }
        {   // bosses: chatter whoever else is near; their wind-ups duck the mix (not the Penitent's every sweep)
            VoiceDirector d; bool bossSpoke = false;
            for (int f = 0; f < 60 * 15; ++f) {
                d.begin(O, F);
                for (int i = 0; i < 6; ++i) d.enemy(in(i + 1, EnemyType::HUSK, {2.f + i, 0, 0}));
                VoiceIn w = in(99, EnemyType::WARDEN, {40, 0, 0}); w.moveSpeed = 2.4f; d.enemy(w);
                for (auto& c : d.end()) bossSpoke |= c.uid == 99 && c.role == SoundRole::CHATTER && c.name.find("spawn") == std::string::npos;
            }
            CHECK(bossSpoke, "a boss chatters at 40 m whoever else is near");
            auto duckOf = [&](EnemyType t, AttackKind a) {
                VoiceDirector e; e.begin(O, F); e.enemy(in(5, t, {10, 0, 0})); e.end();
                e.begin(O, F); e.enemy(windup(in(5, t, {10, 0, 0}), a, 1.f));
                for (auto& c : e.end()) if (c.role == SoundRole::TELL) return c.duckDb;
                return -1.f;
            };
            CHECK(duckOf(EnemyType::WARDEN, AttackKind::SLAM) == 6.f && duckOf(EnemyType::PENITENT, AttackKind::PSLAM) == 6.f &&
                  duckOf(EnemyType::PENITENT, AttackKind::CENSER_LOW) == 0.f && duckOf(EnemyType::HUSK, AttackKind::SHOT) == 0.f,
                  "a boss's wind-up ducks the mix (not the Penitent's every sweep, not a Husk's)");
        }
        {   // every cue the director can ask for is in the bank
            std::set<std::string> names; for (auto& s : voiceBank()) names.insert(s.name);
            bool all = true;
            auto check = [&](const std::vector<VoiceCue>& cs) {
                for (auto& c : cs) if (!names.count(c.name)) { all = false; std::printf("      no voice called %s\n", c.name.c_str()); }
            };
            for (int i = 0; i < (int)EnemyType::COUNT; ++i) {
                const EnemyType t = (EnemyType)i; VoiceDirector d((uint32_t)i + 3); float hp = 100.f;
                const auto atks = attacksOf(t);
                for (int f = 0; f < 60 * 10; ++f) {
                    d.begin(O, F); VoiceIn v = in(1, t, {5, 0, 0});
                    v.moveSpeed = statsOf(t).speed; hp -= f % 50 == 0 ? 5.f : 0.f; v.health = hp;
                    if (!atks.empty()) {
                        const int k = (f / 40) % (int)atks.size(), ph = f % 40;
                        v.attack = atks[k]; v.telegraphTimer = ph < 30 ? 0.5f - ph * F : 0.f; v.telegraphStarted = ph == 0;
                        v.beamOn = atks[k] == AttackKind::BEAM && ph >= 30;
                    }
                    v.enraged = f == 300; v.rose = f == 310; v.hollow = f < 200 ? Hollow::HALOED : Hollow::NONE; v.halo = f < 200;
                    v.linkCount = f / 100;
                    d.enemy(v); check(d.end());
                }
                check(d.death(in(1, t, {5, 0, 0})));
            }
            CHECK(all, "every voice the director can ask for is in the bank");
        }
    }
```

- [ ] **Step 2: Run to verify it fails** — `make test 2>&1 | tail -3` → compile error (`EnemyVoice.h` not found).

- [ ] **Step 3: Write `src/EnemyVoice.h`**

```cpp
#pragma once
// =============================================================================
// EnemyVoice.h — who gets to speak. Each frame the game reports its living
// enemies (VoiceIn); the VoiceDirector answers with the voices to play
// (VoiceCue), each tagged with a role so the mixer can rank it (SfxMixer.h):
//   TELL     every wind-up within 40 m, out of sight too, at priority; the
//            same enemy not twice in 0.15 s; at most 4 a frame (the nearest)
//   ACTION   a wind-up's release; hurts (one per enemy per 0.4 s, six a
//            second); deaths, always, with a level floor (death())
//   CHATTER  idles and steps from the nearest 4 within 25 m; spawn cues
//            (the nearest 3 a frame)
// Bosses chatter whatever the cap. Hollowed voices are the same sounds,
// changed: Enraged higher and driven, Twinned doubled, Haloed shimmering.
// No GL, no audio: the game plays the cues (GameplayState::playVoice).
// =============================================================================
#include "VoiceTable.h"
#include "AudioTypes.h"
#include <algorithm>
#include <cmath>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

struct VoiceIn {
    int        uid = 0;
    EnemyType  type = EnemyType::HUSK;
    Hollow     hollow = Hollow::NONE;
    glm::vec3  pos{0.f};          // where its voice comes from (its chest)
    float      scale = 1.f;       // TWINNED copies are smaller: higher voices
    float      moveSpeed = 0.f, health = 0.f;
    AttackKind attack = AttackKind::NONE;
    float      telegraphTimer = 0.f;
    bool       telegraphStarted = false, staggered = false, beamOn = false, halo = false;
    bool       enraged = false, rose = false;   // this tick: a boss's phase two; the Penitent stood
    int        linkCount = 0;                   // a CONDUCTOR's tethers
};

struct VoiceCue {
    enum Loop : uint8_t { ONCE, START, STOP };
    std::string name;
    glm::vec3   pos{0.f};
    int         uid = 0;
    SoundRole   role = SoundRole::ACTION;
    float       volume = 1.f, pitch = 1.f, drive = 0.f, delay = 0.f, floor = 0.f;
    float       duckDb = 0.f;    // > 0: duck the rest of the mix this much (a boss winding up)
    bool        priority = false;
    Loop        loop = ONCE;     // START: a held sound that follows uid until its STOP
};

class VoiceDirector {
public:
    static constexpr float TELL_RANGE = 40.f, CHATTER_RANGE = 25.f, SPAWN_RANGE = 60.f, BOSS_RANGE = 90.f;
    static constexpr float TELL_REPEAT = 0.15f, HURT_GAP = 0.4f, IDLE_MIN = 3.f, IDLE_MAX = 7.f, FORGET_AFTER = 0.5f;
    static constexpr int   MAX_TELLS = 4, CHATTER_VOICES = 4, HURTS_PER_SEC = 6, SPAWNS_PER_FRAME = 3;

    explicit VoiceDirector(uint32_t seed = 0x51ED5EEDu) : rng(seed ? seed : 1u) {}

    void reset() { st.clear(); frame.clear(); out.clear(); hurtTimes.clear(); deadNow.clear(); now = 0.f; }
    void begin(glm::vec3 listener, float dt) { lis = listener; step = dt; now += dt; frame.clear(); out.clear(); deadNow.clear(); }
    void enemy(const VoiceIn& in) { frame.push_back(in); }
    int  tracked() const { return (int)st.size(); }

    // A kill: its death cry (any range), its held voice stopped, forgotten
    std::vector<VoiceCue> death(const VoiceIn& in) {
        std::vector<VoiceCue> cs;
        auto it = st.find(in.uid);
        if (it != st.end() && it->second.beam) { VoiceCue s = cue(in, voiceName(in.type, VoiceKind::ATTACK, AttackKind::BEAM), SoundRole::ACTION); s.loop = VoiceCue::STOP; cs.push_back(s); }
        if (it != st.end()) st.erase(it);
        deadNow.insert(in.uid);
        VoiceCue c = cue(in, voiceName(in.type, VoiceKind::DEATH), SoundRole::ACTION);
        c.floor = 0.5f;
        dress(in, c, cs);
        return cs;
    }

    const std::vector<VoiceCue>& end() {
        // Who may chatter: the nearest few (bosses always)
        std::vector<std::pair<float, int>> near;
        for (int i = 0; i < (int)frame.size(); ++i) {
            const float d = glm::length(frame[i].pos - lis);
            if (!isBoss(frame[i].type) && d < CHATTER_RANGE) near.push_back({d, i});
        }
        std::sort(near.begin(), near.end());
        std::vector<char> may(frame.size(), 0);
        for (int k = 0; k < (int)near.size() && k < CHATTER_VOICES; ++k) may[near[k].second] = 1;
        while (!hurtTimes.empty() && now - hurtTimes.front() >= 1.f) hurtTimes.erase(hurtTimes.begin());

        struct Ranked { float d; int i; VoiceCue c; };
        std::vector<Ranked> tells, spawns;
        std::vector<VoiceCue> rest;
        for (int i = 0; i < (int)frame.size(); ++i) {
            const VoiceIn& in = frame[i];
            if (deadNow.count(in.uid)) continue;   // killed this frame: its death cry says it all
            const bool boss = isBoss(in.type);
            const float d = glm::length(in.pos - lis);
            const bool fresh = st.find(in.uid) == st.end();
            State& s = st[in.uid];
            if (fresh) {
                s.lastHealth = in.health; s.halo = in.halo; s.links = in.linkCount;
                s.nextIdle = now + 0.5f + rng01() * IDLE_MAX * 0.5f;
                if (d < (boss ? BOSS_RANGE : SPAWN_RANGE)) spawns.push_back({d, i, cue(in, voiceName(in.type, VoiceKind::SPAWN), SoundRole::CHATTER)});
            }
            s.lastSeen = now;
            // The wind-up
            if (in.telegraphStarted && in.attack != AttackKind::NONE && now - s.lastTell >= TELL_REPEAT &&
                d < (boss ? BOSS_RANGE : TELL_RANGE)) {
                VoiceCue c = cue(in, voiceName(in.type, VoiceKind::TELL, in.attack), SoundRole::TELL);
                c.priority = true;
                const bool sweep = in.attack == AttackKind::CENSER_LOW || in.attack == AttackKind::CENSER_HIGH;
                if (boss && !sweep) c.duckDb = 6.f;
                tells.push_back({d, i, c});
                s.lastTell = now;
            }
            // The release: its wind-up ran out (a stagger breaks it off silently)
            if (s.telegraphing && in.telegraphTimer <= 0.f && !in.staggered && hasRelease(s.attack) && s.attack != AttackKind::BEAM)
                rest.push_back(cue(in, voiceName(in.type, VoiceKind::ATTACK, s.attack), SoundRole::ACTION));
            s.telegraphing = in.telegraphTimer > 0.f && in.attack != AttackKind::NONE;
            s.attack = in.attack;
            // A beam: held while it's on
            if (in.beamOn != s.beam) {
                VoiceCue c = cue(in, voiceName(in.type, VoiceKind::ATTACK, AttackKind::BEAM), SoundRole::ACTION);
                c.priority = true; c.loop = in.beamOn ? VoiceCue::START : VoiceCue::STOP;
                rest.push_back(c);
                s.beam = in.beamOn;
            }
            // Hurt
            if (in.health < s.lastHealth - 0.5f && now - s.lastHurt >= HURT_GAP && (int)hurtTimes.size() < HURTS_PER_SEC) {
                rest.push_back(cue(in, voiceName(in.type, VoiceKind::HURT), SoundRole::ACTION));
                s.lastHurt = now; hurtTimes.push_back(now);
            }
            s.lastHealth = in.health;
            // One-offs
            if (s.halo && !in.halo) { VoiceCue c = cue(in, VOICE_HALO_BREAK, SoundRole::ACTION); c.floor = 0.3f; rest.push_back(c); }
            s.halo = in.halo;
            if (in.type == EnemyType::CONDUCTOR && in.linkCount > s.links) rest.push_back(cue(in, "v_conductor_link", SoundRole::ACTION));
            s.links = in.linkCount;
            if (in.enraged && (in.type == EnemyType::WARDEN || in.type == EnemyType::SOVEREIGN)) {
                VoiceCue c = cue(in, std::string("v_") + voiceKey(in.type) + "_enrage", SoundRole::TELL); c.priority = true; c.duckDb = 6.f; rest.push_back(c);
            }
            if (in.rose && in.type == EnemyType::PENITENT) {
                VoiceCue c = cue(in, "v_penitent_rise", SoundRole::TELL); c.priority = true; c.duckDb = 6.f; rest.push_back(c);
            }
            // Chatter: idles and steps (the nearest few, and bosses)
            const bool speaks = may[i] || (boss && d < BOSS_RANGE);
            if (now >= s.nextIdle) {
                if (speaks) rest.push_back(cue(in, voiceName(in.type, VoiceKind::IDLE), SoundRole::CHATTER));
                s.nextIdle = now + IDLE_MIN + rng01() * (IDLE_MAX - IDLE_MIN);
            }
            const float cad = moveCadence(in.type), full = std::max(0.1f, statsOf(in.type).speed);
            const float ratio = std::min(1.5f, in.moveSpeed / full);
            if (cad > 0.f && ratio > 0.1f) {
                s.stride += step * cad * ratio;
                if (s.stride >= 1.f) {
                    s.stride -= std::floor(s.stride);
                    if (speaks) { VoiceCue c = cue(in, voiceName(in.type, VoiceKind::MOVE), SoundRole::CHATTER); c.volume = 0.5f + 0.5f * std::min(1.f, ratio); rest.push_back(c); }
                }
            }
        }
        // Forget whoever hasn't been seen for a while (their held voice stops)
        for (auto it = st.begin(); it != st.end();) {
            if (now - it->second.lastSeen > FORGET_AFTER) {
                if (it->second.beam) { VoiceCue c; c.uid = it->first; c.loop = VoiceCue::STOP; c.name = "v_seraph_atk_beam"; out.push_back(c); }
                it = st.erase(it);
            } else ++it;
        }
        auto byDist = [](const Ranked& a, const Ranked& b) { return a.d < b.d; };
        std::sort(tells.begin(), tells.end(), byDist);
        std::sort(spawns.begin(), spawns.end(), byDist);
        for (int k = 0; k < (int)tells.size() && k < MAX_TELLS; ++k) dress(frame[tells[k].i], tells[k].c, out);
        for (auto& c : rest) dress(frameOf(c.uid), c, out);
        for (int k = 0; k < (int)spawns.size() && k < SPAWNS_PER_FRAME; ++k) dress(frame[spawns[k].i], spawns[k].c, out);
        return out;
    }

private:
    struct State {
        float lastHealth = 0.f, lastTell = -1e9f, lastHurt = -1e9f, nextIdle = 0.f, stride = 0.f, lastSeen = 0.f;
        bool  telegraphing = false, beam = false, halo = false;
        AttackKind attack = AttackKind::NONE;
        int   links = 0;
    };
    std::unordered_map<int, State> st;
    std::unordered_set<int> deadNow;
    std::vector<VoiceIn> frame;
    std::vector<VoiceCue> out;
    std::vector<float> hurtTimes;
    glm::vec3 lis{0.f};
    float now = 0.f, step = 0.f;
    uint32_t rng;

    float rng01() { rng ^= rng << 13; rng ^= rng >> 17; rng ^= rng << 5; return (rng & 0xFFFFFF) / 16777215.f; }
    const VoiceIn& frameOf(int uid) const {
        for (const auto& f : frame) if (f.uid == uid) return f;
        return frame.front();   // rest only holds cues from this frame's enemies
    }
    static VoiceCue cue(const VoiceIn& in, const std::string& name, SoundRole role) {
        VoiceCue c; c.name = name; c.pos = in.pos; c.uid = in.uid; c.role = role; return c;
    }
    // The Hollowed: the same voice, changed
    static void dress(const VoiceIn& in, VoiceCue c, std::vector<VoiceCue>& to) {
        if (c.loop == VoiceCue::STOP) { to.push_back(c); return; }
        if (in.scale < 0.999f) c.pitch /= std::sqrt(std::max(0.3f, in.scale));
        if (in.hollow == Hollow::ENRAGED) { c.pitch *= 1.12f; c.drive = 0.35f; }
        to.push_back(c);
        if (in.hollow == Hollow::TWINNED && c.loop == VoiceCue::ONCE) {
            VoiceCue e = c; e.pitch *= 1.0087f; e.delay = 0.012f; e.volume *= 0.7f; e.priority = false; e.duckDb = 0.f;
            to.push_back(e);
        }
        const bool idle = c.name.size() > 5 && c.name.compare(c.name.size() - 5, 5, "_idle") == 0;
        if (in.hollow == Hollow::HALOED && in.halo && (c.role == SoundRole::TELL || idle)) {
            VoiceCue s = c; s.name = VOICE_HALO_SHIMMER; s.role = SoundRole::CHATTER; s.priority = false; s.duckDb = 0.f; s.volume *= 0.8f;
            to.push_back(s);
        }
    }
};
```

- [ ] **Step 4: Makefile** — add `src/EnemyVoice.h` to `HEADERS` and to the `test:` prerequisites.

- [ ] **Step 5: Run to verify it passes** — `make test 2>&1 | grep -E "FAIL|ALL PASSED|FAILED"` → `ALL PASSED`.

- [ ] **Step 6: Commit**

```bash
git add src/EnemyVoice.h Makefile tests/test_game.cpp
git commit -m "The voice director: every wind-up heard, the nearest few chatter, hurts and deaths rationed, the Hollowed changed"
```

---

### Task 4: The game speaks

**Files:**
- Modify: `src/AudioSystem.h`, `src/GameplayState.h`, `src/Gameplay_Combat.h`, `src/Gameplay_Penitent.h`, `src/Gameplay_Flow.h`, `src/main.cpp`

**Interfaces:**
- Consumes: `VoiceDirector`, `VoiceIn`, `VoiceCue` (Task 3); `VoiceSynth::build`, `voiceBank` (Task 2); `SfxMixer::Opts` roles, `addSound(..., srcRate)`, `devSolo` (Task 1).
- Produces: `AudioSystem::addBuffer(name, std::vector<float>, float srcRate)`, `AudioSystem::playOpts(name, SfxMixer::Opts)`; `GameplayState::voices`, `voiceLoops`, `voiceIn(const Enemy&, const EnemyEvents*)`, `playVoice(const VoiceCue&)`.

This task wires the game (GL code, not headless-testable); its proof is the build, the suite, and the manual checks in Steps 7–8.

- [ ] **Step 1: `src/AudioSystem.h`** — add after `loadSound`:

```cpp
    // A sound built in code (the enemy voices, VoiceSynth.h) at its own rate
    void addBuffer(const std::string& name, std::vector<float> mono, float srcRate) {
        if (!initialized || started) return;
        sfx.addSound(name, std::move(mono), srcRate);
    }
```
after `playAt`:
```cpp
    // Every option the mixer has (roles, pitch, drive, delay: the enemy voices)
    SoundHandle playOpts(const std::string& name, SfxMixer::Opts o) {
        if (!initialized) return 0;
        o.volume *= masterVolume;
        return sfx.play(name, o);
    }
```
and in the constructor after `initialized = true;`:
```cpp
        if (const char* solo = std::getenv("OVERDRIVE_SOLO")) {   // dev: hear one role (tell / chatter / action / ui)
            const std::string s = solo;
            int r = s == "tell" ? 1 : s == "chatter" ? 0 : s == "action" ? 2 : s == "ui" ? 3 : -1;
            if (r >= 0) sfx.devSolo(1 << r);
        }
```
(add `#include <cstdlib>`).

- [ ] **Step 2: `src/GameplayState.h`** — `#include "EnemyVoice.h"`; remove `float telegraphSoundCd` and `bool spawnSoundThisTick` (and every use: `grep -n "telegraphSoundCd\|spawnSoundThisTick" src/*.h`); add members near `sov`/`pen`:

```cpp
    VoiceDirector voices;                                // who gets to speak (EnemyVoice.h)
    std::unordered_map<int, SoundHandle> voiceLoops;     // held voices (a Seraph's beam), by enemy uid
    VoiceIn voiceIn(const Enemy& e, const EnemyEvents* ev) const;
    void playVoice(const VoiceCue& c);
```

- [ ] **Step 3: `src/Gameplay_Combat.h`** — add before `updateEnemies`:

```cpp
// What the voice director needs to know about one enemy this frame
inline VoiceIn GameplayState::voiceIn(const Enemy& e, const EnemyEvents* ev) const {
    VoiceIn v;
    v.uid = e.uid; v.type = e.type; v.hollow = e.hollow; v.scale = e.scale;
    v.pos = e.position + glm::vec3{0.f, e.height() * 0.7f, 0.f};
    v.moveSpeed = e.moveSpeed; v.health = e.health;
    v.attack = e.attack; v.telegraphTimer = e.telegraphTimer; v.staggered = e.staggered();
    v.halo = e.halo; v.linkCount = e.linkCount; v.beamOn = e.beamTimer > 0.f;
    if (ev) { v.telegraphStarted = ev->telegraphStarted; v.enraged = ev->enraged; v.rose = ev->penRose; }
    return v;
}

// One enemy voice: placed at the enemy, ranked by its role; a held one is
// kept by uid so it can follow its enemy and be stopped
inline void GameplayState::playVoice(const VoiceCue& c) {
    if (c.loop == VoiceCue::STOP) {
        auto it = voiceLoops.find(c.uid);
        if (it != voiceLoops.end()) { audio.stop(it->second); voiceLoops.erase(it); }
        return;
    }
    SfxMixer::Opts o;
    o.volume = c.volume; o.group = SoundGroup::ENEMY; o.role = c.role; o.priority = c.priority;
    o.positional = true; o.pos = c.pos; o.floor = c.floor; o.pitch = c.pitch; o.drive = c.drive; o.delay = c.delay;
    SoundHandle h = audio.playOpts(c.name, o);
    if (c.loop == VoiceCue::START) {
        auto it = voiceLoops.find(c.uid);
        if (it != voiceLoops.end()) audio.stop(it->second);
        voiceLoops[c.uid] = h;
    }
    if (c.duckDb > 0.f) audio.duck(c.duckDb, 0.5f);
}
```

In `spawnEnemy`: delete the three lines from `if (spawnSoundThisTick) return;` through `audio.playAt("spawn", ...)` (the director's first sight of it plays its spawn cue).

In `updateEnemies`: delete `telegraphSoundCd -= dt;`; add `voices.begin(player.camera.position, dt);` as its first line; right after `const EnemyEvents ev = e.ev;` add:
```cpp
        if (e.alive) voices.enemy(voiceIn(e, &ev));
```
delete the whole "Wind-up tick" block (the `if (ev.telegraphStarted && dist < 30.f && telegraphSoundCd ...` statement and its comment); at the end of `updateEnemies` (after the separation loop) add:
```cpp
    // The voices: what the director picked, and held ones following their enemy
    for (const VoiceCue& c : voices.end()) playVoice(c);
    for (auto& [uid, h] : voiceLoops)
        for (const auto& e : enemies)
            if (e.uid == uid) { audio.moveSource(h, e.position + glm::vec3{0.f, e.height() * 0.7f, 0.f}); break; }
```
Check that no conditional `continue` other than the dev-overlay ones sits between the loop's top and the `voices.enemy` line; if one does, move the report so every living enemy is reported every frame (ledger it).

In `onEnemyKilled`: replace the `audio.playAt("enemy_death", ...)` line with:
```cpp
    for (const VoiceCue& c : voices.death(voiceIn(e, nullptr))) playVoice(c);   // a kill confirms at any range
```

- [ ] **Step 4: `src/Gameplay_Penitent.h`** — in `onPenitentEvents`, delete the `// Tells: ...` comment and the `if (ev.telegraphStarted) { switch ... }` block (the director plays the Penitent's tells and ducks the mix for its slam, stomp, scourge and lash).

- [ ] **Step 5: `src/Gameplay_Flow.h`** — in the function that does `enemies.clear(); pendingTwins.clear();`, add after it:
```cpp
    voices.reset();
    for (auto& [uid, h] : voiceLoops) audio.stop(h);
    voiceLoops.clear();
```

- [ ] **Step 6: `src/main.cpp`** — `#include "VoiceSynth.h"`; after the `SOUNDS` loading loop and before `if (std::getenv("OVERDRIVE_SFXLIST"))`:
```cpp
    if (app->audio.initialized) {   // the enemy voices, built in code (VoiceSynth.h)
        const Uint32 t0 = SDL_GetTicks();
        float secs = 0.f;
        for (const VoiceSpec& v : voiceBank())
            for (int k = 0; k < v.variants; ++k) {
                auto b = VoiceSynth::build(v, k);
                secs += b.size() / VoiceSynth::RATE;
                app->audio.addBuffer(v.name, std::move(b), VoiceSynth::RATE);
            }
        if (std::getenv("OVERDRIVE_SFXLIST")) std::fprintf(stderr, "voices: %.1f s of audio built in %u ms\n", secs, SDL_GetTicks() - t0);
    }
```
Remove `"enemy_death"` and `"spawn"` from `SOUNDS` (no longer played; the files stay).

- [ ] **Step 7: Build, test, check**

Run: `make 2>&1 | tail -2 && make test 2>&1 | grep -E "FAIL|ALL PASSED|FAILED"`
Expected: builds; `ALL PASSED`.
Run: `grep -rn '"enemy_death"\|"spawn"' src/ ; grep -rn 'telegraph' src/Gameplay_Combat.h src/Gameplay_Penitent.h`
Expected: no enemy uses left (UI/hazard `telegraph` in Menus, Flow, Shifts and the Sovereign's strike markers stay).
Run: `OVERDRIVE_SFXLIST=1 ./shooter --play --god --audiodump /tmp/claude-voices-wave.wav 1 2>&1 | grep voices`
Expected: `voices: N s of audio built in M ms` (record M in the ledger; M on desktop well under 1000).

- [ ] **Step 8: Listen** (dumps go to the scratchpad):
`./shooter --act2 --arena 1 --god --audiodump <scratch>/wave.wav 25` (an Act II wave), `./shooter --play --arena 5 --god --audiodump <scratch>/sovereign.wav 30`, `./shooter --act2 --arena 3 --wave 4 --god --audiodump <scratch>/penitent.wav 30`. Play each (`afplay`). Check: tells distinct per type and heard over the rest, chatter present but low, deaths and hurts not spammy, Seraph beam held then stopped. Ledger anything changed.

- [ ] **Step 9: Commit**

```bash
git add src/AudioSystem.h src/GameplayState.h src/Gameplay_Combat.h src/Gameplay_Penitent.h src/Gameplay_Flow.h src/main.cpp
git commit -m "The enemies speak: the director fed every frame, its cues played at each enemy, the shared wind-up beep, spawn and death sounds gone"
```

---

### Task 5: The mix table and the loudness pass

**Files:**
- Modify: `src/MixTable.h`, `src/AudioSystem.h`, `src/main.cpp`, `tests/test_game.cpp`

**Interfaces:**
- Produces: `struct MixEntry { const char* name; MixClass cls; float trimDb; };` `const std::vector<MixEntry>& mixTable()`; `const MixEntry* mixEntry(const std::string&)`; `float mixGain(const std::string&)` (linear trim; names not in the table, e.g. voices, → 1).

- [ ] **Step 1: Write the failing test** — add near the sfx helpers in `tests/test_game.cpp`:

```cpp
// A 16-bit PCM WAV, channel 0, as floats (empty if missing)
static std::vector<float> readWav16(const std::string& path, float& rate) {
    std::vector<float> out; rate = 0.f;
    FILE* f = std::fopen(path.c_str(), "rb");
    if (!f) return out;
    char id[4], wave[4]; uint32_t len = 0; int ch = 1, bits = 16;
    if (std::fread(id, 1, 4, f) != 4 || std::fread(&len, 4, 1, f) != 1 || std::fread(wave, 1, 4, f) != 4) { std::fclose(f); return out; }
    while (std::fread(id, 1, 4, f) == 4 && std::fread(&len, 4, 1, f) == 1) {
        if (!std::memcmp(id, "fmt ", 4)) {
            std::vector<uint8_t> b(len); if (std::fread(b.data(), 1, len, f) != len) break;
            uint16_t c, bp; uint32_t sr; std::memcpy(&c, &b[2], 2); std::memcpy(&sr, &b[4], 4); std::memcpy(&bp, &b[14], 2);
            ch = c; rate = (float)sr; bits = bp;
            if (len & 1) std::fseek(f, 1, SEEK_CUR);
        } else if (!std::memcmp(id, "data", 4)) {
            std::vector<int16_t> s(len / 2); size_t got = std::fread(s.data(), 2, s.size(), f);
            if (bits == 16) for (size_t i = 0; i + ch <= got; i += ch) out.push_back(s[i] / 32768.f);
            break;
        } else std::fseek(f, len + (len & 1), SEEK_CUR);
    }
    std::fclose(f);
    return out;
}
```

and append the block:

```cpp
    // ---------------------------------------------------------------- the mix: every sound file at its class's level
    {
        bool levels = true, complete = true;
        for (const MixEntry& m : mixTable()) {
            for (int k = 0; k <= 8; ++k) {
                const std::string path = "assets/sfx/" + std::string(m.name) + (k ? "_" + std::to_string(k) : std::string()) + ".wav";
                float sr; auto x = readWav16(path, sr);
                if (x.empty()) { if (k == 0) { complete = false; std::printf("      missing %s\n", path.c_str()); } continue; }
                const float lv = shortTermDb(x.data(), x.size(), sr) + m.trimDb;
                if (std::fabs(lv - mixTargetDb(m.cls)) > 2.f) { levels = false; std::printf("      %s at %.1f dB after trim (wants %.1f)\n", path.c_str(), lv, mixTargetDb(m.cls)); }
            }
        }
        CHECK(complete, "every sound in the mix table has its file");
        CHECK(levels, "every sound file, after its trim, sits within 2 dB of its class's level");
        const char* loaded[] = {"jump", "land", "dash", "slam", "revolver", "shotgun", "reload", "grapple_fire", "hit", "player_hit",
                                "parry", "telegraph", "explosion", "wave", "pickup", "kar", "longshot", "bolt", "scope", "levelup",
                                "potion", "barrier", "split", "upgrade", "clank", "punch", "step1", "step2", "step3", "step4", "door",
                                "door_close", "boost", "cyl_open", "cyl_close", "eject", "shell_in", "pump", "wade", "skim", "cell",
                                "dry", "switch_up0", "switch_up1", "switch_up2", "switch_up3"};
        bool all = true; for (const char* n : loaded) all &= mixEntry(n) != nullptr;
        CHECK(all && mixTable().size() == sizeof(loaded) / sizeof(loaded[0]), "every sound file the game loads has exactly one place in the mix");
        CHECK(std::fabs(mixGain("jump") - std::pow(10.f, -4.1f / 20.f)) < 1e-4f && mixGain("v_husk_idle") == 1.f && mixGain("nope") == 1.f,
              "a sound's mix gain is its trim; built voices and unknown names pass at 1");
    }
```

- [ ] **Step 2: Run to verify it fails** — `make test 2>&1 | tail -3` → compile error (`mixTable` not declared).

- [ ] **Step 3: Implement** — append to `src/MixTable.h` (add `#include <string>`, `<unordered_map>`, `<vector>`):

```cpp
// Every sound file the game loads: its class and the trim (dB) that brings
// the file to the class's level. Measured with --mixreport; the test holds
// each within 2 dB. (The built voices are made at their level: no trim.)
struct MixEntry { const char* name; MixClass cls; float trimDb; };

inline const std::vector<MixEntry>& mixTable() {
    using C = MixClass;
    static const std::vector<MixEntry> T = {
        {"revolver", C::GUN, 0.2f}, {"shotgun", C::GUN, -0.2f}, {"kar", C::GUN, 0.f}, {"longshot", C::GUN, 0.f},
        {"bolt", C::FOLEY, -1.5f}, {"cell", C::FOLEY, 1.5f}, {"cyl_close", C::FOLEY, -1.6f}, {"cyl_open", C::FOLEY, 1.5f},
        {"dry", C::FOLEY, 2.4f}, {"eject", C::FOLEY, 0.7f}, {"pump", C::FOLEY, -0.6f}, {"reload", C::FOLEY, -0.8f},
        {"shell_in", C::FOLEY, -0.3f}, {"scope", C::FOLEY, 0.8f}, {"telegraph", C::FOLEY, 5.f}, {"hit", C::FOLEY, 2.5f},
        {"wade", C::FOLEY, -0.7f}, {"barrier", C::FOLEY, 0.6f},
        {"jump", C::PLAYER, -4.1f}, {"land", C::PLAYER, 1.3f}, {"dash", C::PLAYER, -2.6f}, {"grapple_fire", C::PLAYER, -0.6f},
        {"punch", C::PLAYER, 1.7f}, {"step1", C::PLAYER, 1.9f}, {"step2", C::PLAYER, 2.5f}, {"step3", C::PLAYER, 2.5f},
        {"step4", C::PLAYER, 2.5f}, {"skim", C::PLAYER, -3.2f}, {"boost", C::PLAYER, -1.9f}, {"player_hit", C::PLAYER, -2.f},
        {"parry", C::PLAYER, 3.5f}, {"switch_up0", C::PLAYER, 1.3f}, {"switch_up1", C::PLAYER, 0.9f},
        {"switch_up2", C::PLAYER, 0.6f}, {"switch_up3", C::PLAYER, 0.2f},
        {"explosion", C::ACTION, -1.2f}, {"slam", C::ACTION, -1.8f}, {"clank", C::ACTION, 2.f},
        {"door", C::WORLD, -1.5f}, {"door_close", C::WORLD, 1.6f}, {"split", C::WORLD, 0.7f}, {"pickup", C::WORLD, -2.2f},
        {"potion", C::WORLD, 0.5f},
        {"wave", C::UI, -4.2f}, {"levelup", C::UI, -0.1f}, {"upgrade", C::UI, 0.1f},
    };
    return T;
}
inline const MixEntry* mixEntry(const std::string& name) {
    static const std::unordered_map<std::string, const MixEntry*> M = [] {
        std::unordered_map<std::string, const MixEntry*> m;
        for (const auto& e : mixTable()) m[e.name] = &e;
        return m;
    }();
    auto it = M.find(name);
    return it == M.end() ? nullptr : it->second;
}
// The linear gain a sound plays at for volume 128
inline float mixGain(const std::string& name) {
    const MixEntry* e = mixEntry(name);
    return e ? std::pow(10.f, e->trimDb / 20.f) : 1.f;
}
```

`src/AudioSystem.h`: `#include "MixTable.h"`; in `play` and `playAt` multiply `o.volume` by `mixGain(name)`; in `playOpts` `o.volume *= masterVolume * mixGain(name);`.

`src/main.cpp`: replace the `SOUNDS` array and its loop with:
```cpp
    for (const MixEntry& m : mixTable())
        app->audio.loadSound(m.name, std::string("assets/sfx/") + m.name + ".wav");
```
and add `--mixreport` (prints every loaded sound's level, then quits), checked right after the voices are built:
```cpp
    for (int i = 1; i < argc; ++i) if (std::string(argv[i]) == "--mixreport") {
        auto row = [&](const std::string& n, MixClass c, float trim) {
            const std::vector<float>* s = app->audio.sfx.sample(n, 0);
            if (!s) { std::printf("%-24s missing\n", n.c_str()); return; }
            float file = shortTermDb(s->data(), s->size(), app->audio.sfx.sampleRateOf(n));
            std::printf("%-24s class %d  file %6.1f  trim %5.1f  level %6.1f  target %6.1f\n", n.c_str(), (int)c, file, trim, file + trim, mixTargetDb(c));
        };
        for (const MixEntry& m : mixTable()) row(m.name, m.cls, m.trimDb);
        for (const VoiceSpec& v : voiceBank()) row(v.name, v.cls, 0.f);
        return 0;
    }
```
(Use whatever cleanup path `main` uses on early exit if a plain `return 0;` would skip something required; ledger it.)

- [ ] **Step 4: Run to verify it passes** — `make test 2>&1 | grep -E "FAIL|ALL PASSED|FAILED"` → `ALL PASSED`. If an entry misses by > 2 dB, set its trim from the printed measurement (the trims above came from a Python measurement of the same window) and ledger it.

- [ ] **Step 5: Report** — `make && ./shooter --mixreport | head -60`. Expected: every row's `level` within 2 of `target`.

- [ ] **Step 6: Call-site pass.** Call-site volumes now mean "this context, relative to the sound's class level". Review the call sites of the sounds whose trims are largest (`jump` −4.1, `wave` −4.2, `telegraph` +5, `parry` +3.5, `skim` −3.2, `dash` −2.6, `step*` +2.5): where a constant volume was only compensating the file's level (the same sound at the same number everywhere), set it to 128 (drop the argument). Where it encodes context (a soft purchase-fail `telegraph` at 50 vs a hazard at 110), keep it. Ledger each change as a Ruling line.

- [ ] **Step 7: Listen** — re-run the three dumps from Task 4 Step 8 plus `./shooter --play --god --audiodump <scratch>/arena.wav 30`; then measure the clip share:
```bash
python3 - <<'EOF'
import wave, struct, sys, glob
for p in sorted(glob.glob(sys.argv[1] if len(sys.argv) > 1 else '<scratch>/*.wav')):
    w = wave.open(p); n = w.getnframes(); d = struct.unpack('<%dh' % (n * 2), w.readframes(n))
    hot = sum(1 for v in d if abs(v) > 0.8 * 32767) / len(d)
    print(f"{p}: {hot * 100:.3f}% of samples over 0.8")
EOF
```
Expected: each < 0.5 %. Then tells vs the rest: `OVERDRIVE_SOLO=tell ./shooter --act2 --arena 1 --god --audiodump <scratch>/tells.wav 25` and compare by ear and by the RMS of tell moments against the full `wave.wav` (tells should stand ≥ 3 dB over the bed around them). Ledger the numbers.

- [ ] **Step 8: Commit**

```bash
git add src/MixTable.h src/AudioSystem.h src/main.cpp tests/test_game.cpp src/Gameplay_*.h
git commit -m "The mix in one table: every sound in a class with a target level, trims measured and held by a test, --mixreport"
```

---

### Task 6: Docs, bench, web

**Files:**
- Modify: `README.md`, `assets/sfx/CREDITS.md`

- [ ] **Step 1: README** — in the sound section, add a short paragraph: every enemy has its own machine-and-choir voice built in code at load (`src/VoiceSynth.h`), the voice director decides who speaks (`src/EnemyVoice.h`), wind-ups always cut through and the rest steps back, every sound's level lives in `src/MixTable.h` (`--mixreport`), dev `OVERDRIVE_SOLO=tell|chatter|action|ui`.

- [ ] **Step 2: CREDITS** — add: "Enemy voices: synthesized in code at load (src/VoiceSynth.h), no recordings."

- [ ] **Step 3: Bench** — `./shooter --act2 --arena 1 --bench 2000` on this branch and on `main` (`git stash`-free: build `main` in a temporary worktree under the scratchpad). Expected: frame time within ~0.1 ms. Ledger both numbers.

- [ ] **Step 4: Web** — `source ~/emsdk/emsdk_env.sh && make web 2>&1 | tail -3`; `ls -l` the web `.data` and compare to `main`'s (1.98 MB). Expected: builds; data the same (± a few KB).

- [ ] **Step 5: Full suite** — `make && make test 2>&1 | grep -E "FAIL|ALL PASSED|FAILED"` → `ALL PASSED`.

- [ ] **Step 6: Commit**

```bash
git add README.md assets/sfx/CREDITS.md
git commit -m "README and credits for the enemy voices and the mix"
```
