# Act II piece 5: The Reliquary — Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add the Reliquary — chunks of the four Act I arenas drifting over a void, regrouping between waves — with two new enemies, the Revenant (its soul flees and re-forms unless caught) and the Weaver (strings snaring wires across your routes).

**Architecture:** Chunks are groups of DRIVEN movers under a new `LevelData::Formation` (one target offset per arrangement, an eased 6 s glide), driven by a new `ArenaShift::DRIFT`. The enemies follow the Penitent's pattern: minds in `src/EnemyRelic.h` emitting events, what hurts you in `src/RelicHazards.h` (no GL/audio), the game side in `src/Gameplay_Reliquary.h`.

**Tech Stack:** C++17 header-only, glm, headless tests (`tests/test_game.cpp`, CHECK), Emscripten web build.

**Spec:** `docs/superpowers/specs/2026-10-07-act2-reliquary-design.md`

## Global Constraints

- Maps grand and organic, not boxes; "dark machine + glow"; the boss bar (light and sound tells, a skilful answer, no cheese) for every new enemy; on-screen text in the terse-narrator voice (docs/text-pass.md).
- Reliquary: centre Q = (0, −240, −960); reached through the Descent pit's south wall (Z ≈ −875) over a bridge; HALL reverb; music track 8 "RELIQUARY", `MUSIC_TRACKS` 9; three waves, `maxAlive` 12, `damageScale` 1.45; menu "PREVIEW - 4/4 ARENAS".
- Drift: glide 6 s eased, only between waves, next wave held until settled; gaps ≤ 9 m horizontal and ≤ 3 m up or a grapple point in reach; falls → last chunk stood on, 15 damage; falling enemies die as your kill.
- Revenant: 160 hp; rake (tell 0.45 s), soul bolt (tell 0.6 s, parryable); soul 40 hp, ~4 s flight to another chunk, shot → gone ("SOUL TAKEN"), punched → gone + big style, uncaught → re-forms over 1 s at half the health it last had; after two re-forms the third death is final.
- Weaver: 120 hp; a wire every ~6 s (tell 0.8 s) at chest height 1.2 m; ≤ 4 wires each (oldest replaced); touch → snared 1.2 s (×0.4 speed, no dash), 10 damage, enemies within 30 m readied; slide under / jump over; shoot a node to cut; its death drops its wires; a drift clears all wires.
- NEW hints: "ITS SOUL RUNS - CATCH IT BEFORE IT COMES BACK" (Revenant), "IT WIRES THE GAPS - CUT THE NODES OR DUCK UNDER" (Weaver).
- After every task `make && make test` green. No Claude / Co-Authored-By trailers in commits (user rule).

## Review Focus

1. **A retry or death while the chunks are mid-glide** — everything back to arrangement 0 at once, the player on solid ground, no hold left set (Task 1 test "reset mid-glide").
2. **A Revenant killed by a fall, lava-like environment or a friendly blast** — its soul still flees (or, out of the void, ends: a body lost to the void has no soul to catch) (Task 5 test "a Revenant lost to the void doesn't come back").
3. **A wire strung where the player is standing still or in mid-air** — never instantly snares the player on the tick it appears (it arms after its tell; touch tests start the next tick) (Task 4 test "a fresh wire doesn't snare where it's strung through you").
4. **A Twinned Revenant / Haloed Revenant** — the twins each carry their own soul; the halo's damage rule applies to the body, not the soul (Task 3 test "twins each flee").
5. **The player standing on a chunk the instant a glide starts, at its edge** — carried, not pushed off or embedded (Task 1 test "carried at the edge").

---

## File Structure

- Modify `src/Level.h` — `Formation`, `ArenaShift::DRIFT`, `enemyFloor` for DRIFT.
- Modify `src/ArenaShifts.h` — DRIFT reset/onWaveCleared/update.
- Modify `src/LevelAct2.h` — `buildReliquary`, the pit's south door, basins.
- Create `src/EnemyRelic.h` — `thinkRevenant`, `thinkWeaver`.
- Create `src/RelicHazards.h` — `Soul`, `Wire`, `RelicHazards`.
- Create `src/Gameplay_Reliquary.h` — drift control, falls, souls, wires, drawing.
- Modify `src/Enemy.h`, `src/EnemyModel.h`, `src/Progression.h`, `src/VoiceTable.h`, `src/VoiceSynth.h`, `src/MusicSynth.h`, `src/MenuState.h`, `src/GameplayState.h`, `src/Gameplay_Combat.h`, `src/Gameplay_Flow.h`, `src/Gameplay_Tick.h`, `src/Gameplay_Render.h`, `Makefile`, `tests/test_game.cpp`, `README.md`.

---

### Task 0: Branch

- [ ] `git checkout -b reliquary`; create the ledger.

---

### Task 1: The Formation and the DRIFT shift

**Files:** Modify `src/Level.h`, `src/ArenaShifts.h`; Test `tests/test_game.cpp`.

**Interfaces:**
- Produces: `ArenaShift::DRIFT`; `LevelData::Formation { struct Chunk { std::vector<int> movers; std::vector<glm::vec3> at; glm::vec3 home{0.f}; glm::vec2 half{0.f}; }; std::vector<Chunk> chunks; int at, to; float t; static constexpr float GLIDE_TIME = 6.f; int arrangements() const; bool gliding() const; glm::vec3 offset(int c) const; glm::vec3 top(int c) const; void glideTo(int k); void reset(LevelData&); void update(float dt, LevelData&); int chunkOfWall(const LevelData&, int wall) const; float lowestTop() const; float meanTop() const; }`; `LevelData::formation`.

- [ ] **Step 1: Write the failing tests** (append before `// ---------------------------------------------------------------- the lift (driven movers)`):

```cpp
    // ---------------------------------------------------------------- the Reliquary: a formation that drifts
    {
        LevelData F; LevelBuilder B(F);
        auto chunk = [&](glm::vec3 top, glm::vec2 half, std::vector<glm::vec3> at) {
            LevelData::Formation::Chunk c; c.home = top; c.half = half; c.at = at;
            c.movers.push_back(B.mover(top - glm::vec3{0, 0.75f, 0}, {half.x, 0.75f, half.y}, Mover::Path::DRIVEN, {0, 0, 0}, {0, 0, 0}, 1.f, 0.f, {1, 1, 1}));
            F.formation.chunks.push_back(c);
        };
        chunk({0, 0, 0},  {5, 5}, {{0, 0, 0}, {0, 0, -20}, {10, 3, 0}});
        chunk({20, 0, 0}, {4, 4}, {{0, 0, 0}, {0, 2, 0},   {-40, 0, 0}});
        LevelData::Formation& fm = F.formation;
        fm.reset(F); F.updateMovers(0.f);
        CHECK(fm.arrangements() == 3 && !fm.gliding() && F.walls[F.movers[0].wall].box.max.y == 0.f, "a formation starts in its first arrangement");
        fm.glideTo(1);
        float clock = 0.f; bool eased = true; float prevZ = 0.f, prevStep = 0.f;
        for (int i = 0; i < 60 * 3; ++i) {
            clock += DT; fm.update(DT, F); F.updateMovers(clock);
            float z = F.walls[F.movers[0].wall].box.max.z, step = std::fabs(z - prevZ);
            if (i > 5 && i < 60 && step + 1e-5f < prevStep) eased = false;   // speeding up in the first second
            prevStep = step; prevZ = z;
        }
        bool midway = fm.gliding();
        for (int i = 0; i < 60 * 4; ++i) { clock += DT; fm.update(DT, F); F.updateMovers(clock); }
        CHECK(midway && eased && !fm.gliding() && fm.at == 1, "a glide eases in, takes 6 s, and ends");
        CHECK(std::fabs(F.walls[F.movers[0].wall].box.max.z - (5.f - 20.f)) < 1e-4f && std::fabs(F.walls[F.movers[1].wall].box.max.y - 2.f) < 1e-4f,
              "each chunk ends exactly in its next arrangement");
        CHECK(fm.chunkOfWall(F, F.movers[1].wall) == 1 && fm.chunkOfWall(F, 9999) == -1 && fm.top(1) == glm::vec3{20, 2, 0},
              "a formation knows which chunk a wall belongs to, and where its top is");
        // Review focus 1: a retry mid-glide puts everything back at once
        fm.glideTo(2);
        for (int i = 0; i < 60 * 2; ++i) { clock += DT; fm.update(DT, F); F.updateMovers(clock); }
        fm.reset(F); F.updateMovers(clock);
        CHECK(!fm.gliding() && fm.at == 0 && F.walls[F.movers[0].wall].box.max.y == 0.f && F.walls[F.movers[0].wall].box.max.x == 5.f,
              "reset mid-glide puts every chunk back in its first arrangement");
        // Review focus 5: a player standing at a chunk's edge when it glides is carried and stays on
        fm.glideTo(2);
        Player p({4.6f, 0.f, 4.6f});
        p.dynWalls = F.moverWalls.data(); p.dynCount = (int)F.moverWalls.size();
        SpatialGrid pg; pg.build(F.walls);
        bool stayed = true;
        for (int i = 0; i < 60 * 7; ++i) {
            int rm = F.moverOfWall(p.groundWall);
            clock += DT; fm.update(DT, F); F.updateMovers(clock);
            if (rm >= 0) p.position += F.movers[rm].delta;
            p.floorY = -100.f;
            Uint8 k[SDL_NUM_SCANCODES]; std::memset(k, 0, sizeof(k));
            p.update(DT, k, F.walls.data(), (int)F.walls.size(), false, &pg);
            if (i > 10) stayed &= p.position.y > fm.top(0).y - 0.3f && p.position.y < fm.top(0).y + 1.2f;
        }
        CHECK(stayed && std::fabs(p.position.x - 14.6f) < 0.3f, "standing at a chunk's edge you're carried through the glide and stay on");
    }
```

- [ ] **Step 2: Run to verify it fails** — `make test 2>&1 | grep -m3 error` → `no member named 'formation'`.

- [ ] **Step 3: Implement.**

`src/Level.h`:
- `ArenaShift` gains `DRIFT` (append after `DESCENT`).
- After `Lift lift;` add:
```cpp
    // THE RELIQUARY's drifting relics: groups of DRIVEN movers, each with an
    // offset per arrangement (one per wave, plus the path to the hole); a
    // glide eases every chunk from one arrangement to the next in GLIDE_TIME
    struct Formation {
        struct Chunk {
            std::vector<int>       movers;   // the boxes that move together
            std::vector<glm::vec3> at;       // its offset in each arrangement (at[0] = as built)
            glm::vec3 home{0.f};             // its top's centre as built
            glm::vec2 half{0.f};             // half its top's size (X, Z)
        };
        std::vector<Chunk> chunks;
        int at = 0, to = 0;
        float t = 0.f;
        static constexpr float GLIDE_TIME = 6.f;
        int  arrangements() const { return chunks.empty() ? 0 : (int)chunks[0].at.size(); }
        bool gliding() const { return to != at; }
        float eased() const { float u = glm::clamp(t / GLIDE_TIME, 0.f, 1.f); return u * u * (3.f - 2.f * u); }
        glm::vec3 offset(int c) const {
            const Chunk& k = chunks[c];
            return gliding() ? glm::mix(k.at[at], k.at[to], eased()) : k.at[at];
        }
        glm::vec3 top(int c) const { return chunks[c].home + offset(c); }
        void glideTo(int k) { if (k >= 0 && k < arrangements() && k != at && !gliding()) { to = k; t = 0.f; } }
        void reset(LevelData& L) { at = to = 0; t = 0.f; apply(L); }
        void update(float dt, LevelData& L) {
            if (gliding()) { t += dt; if (t >= GLIDE_TIME) { at = to; t = 0.f; } }
            apply(L);
        }
        void apply(LevelData& L) const {
            for (int c = 0; c < (int)chunks.size(); ++c) {
                glm::vec3 o = offset(c);
                for (int m : chunks[c].movers) { L.movers[m].a = o; L.movers[m].b = o; L.movers[m].drive = 0.f; }
            }
        }
        int chunkOfWall(const LevelData& L, int wall) const {
            for (int c = 0; c < (int)chunks.size(); ++c)
                for (int m : chunks[c].movers) if (L.movers[m].wall == wall) return c;
            return -1;
        }
        float lowestTop() const { float y = 1e9f; for (int c = 0; c < (int)chunks.size(); ++c) y = std::min(y, top(c).y); return chunks.empty() ? 0.f : y; }
        float meanTop() const { float y = 0.f; for (int c = 0; c < (int)chunks.size(); ++c) y += top(c).y; return chunks.empty() ? 0.f : y / chunks.size(); }
    };
    Formation formation;
```
- `enemyFloor`: replace the DESCENT-only early return with:
```cpp
        if (arena < 0 || arena >= (int)arenas.size()) return base;
        if (arenas[arena].shift == ArenaShift::DRIFT)   // the Reliquary: the chunks under it (fliers keep to the chunks' height)
            return flying ? std::max(base, formation.meanTop()) : std::max(base, groundUnder);
        if (arenas[arena].shift != ArenaShift::DESCENT) return base;
```

`src/ArenaShifts.h`:
- header comment: `//   DRIFT     (the Reliquary) the chunks glide to the next wave's arrangement once a wave is cleared`
- `reset`: after the lift line add `L.formation.reset(L);`
- `onWaveCleared`: first line add `if (a >= 0 && a < (int)L.arenas.size() && L.arenas[a].shift == ArenaShift::DRIFT) L.formation.glideTo(nextWave);`
- `update`: after `L.lift.update(dt, L);` add `L.formation.update(dt, L);   // the Reliquary's relics (glide only when a wave is cleared)`

- [ ] **Step 4: Run** — `make test 2>&1 | grep -E "FAIL|ALL PASSED|FAILED"` → `ALL PASSED`.

- [ ] **Step 5: Commit**
```bash
git add src/Level.h src/ArenaShifts.h tests/test_game.cpp
git commit -m "Drifting relics: a formation of driven chunks that glides to the next wave's arrangement"
```

---

### Task 2: Building the Reliquary

**Files:** Modify `src/LevelAct2.h`, `src/MusicSynth.h`, `src/Gameplay_Flow.h`, `src/MenuState.h`, `src/Gameplay_Combat.h` (the Penitent's death banner), `tests/test_game.cpp`.

**Interfaces:**
- Consumes: `Formation` (Task 1).
- Produces: arena 3 "THE RELIQUARY" with `shift = DRIFT`, `waveGround[0..2]`, `waveAir[0..2]`, `formation` with 4 arrangements (3 waves + the path to the hole), `finishPos` at the hole's rim; `static constexpr glm::vec3 RELIQUARY_Q{0.f, -240.f, -960.f}` in `LevelAct2.h`; the Descent's `exitDoor` in the pit's south wall; `MUSIC_TRACKS` 9.

- [ ] **Step 1: Write the failing tests:**

```cpp
    // ---------------------------------------------------------------- the Reliquary: the map
    {
        LevelData N = buildAct2Level();
        CHECK(N.arenas.size() == 4 && std::string(N.arenas[3].name) == "THE RELIQUARY" && N.arenas[3].shift == ArenaShift::DRIFT &&
              N.arenas[3].waves.size() == 3 && N.arenas[3].maxAlive == 12 && std::fabs(N.arenas[3].damageScale - 1.45f) < 1e-4f,
              "the Reliquary: the fourth Act II arena, three waves, 12 at once, x1.45");
        const Arena& R = N.arenas[3];
        auto& fm = N.formation;
        CHECK(fm.chunks.size() >= 12 && fm.arrangements() == 4, "four relics and their debris, in four arrangements (three waves and the way to the hole)");
        SpatialGrid g; g.build(N.walls);
        float clock = 0.f;
        // Spawns stand on a chunk in their wave's arrangement, inside bounds, clear of walls
        bool spawnsOk = true;
        for (int w = 0; w < 3; ++w) {
            fm.at = fm.to = w; fm.t = 0.f; fm.apply(N); N.updateMovers(clock);
            for (auto& s : R.waveGround[w]) {
                float top = N.groundAt(s.x, s.z, s.y + 0.5f);
                bool clear = !overlapsWall(N, boxAt(s + glm::vec3{0, 0.05f, 0}, 0.6f, 1.8f));
                if (std::fabs(top - s.y) > 0.05f || !inside(R.bounds, s) || !clear) { spawnsOk = false; std::printf("      wave %d spawn (%.1f %.1f %.1f) top %.2f clear %d\n", w, s.x, s.y, s.z, top, clear); }
            }
        }
        CHECK(spawnsOk, "every wave's spawns stand on a chunk, inside the Reliquary, clear of walls");
        // Every chunk can be reached from the landing in every arrangement: jump+dash gaps or a grapple point in reach
        const float GRAPPLE = GrappleHook{}.maxLength - 2.f;
        bool reach = true;
        for (int k = 0; k < 4; ++k) {
            fm.at = fm.to = k; fm.t = 0.f; fm.apply(N); N.updateMovers(clock);
            int n = (int)fm.chunks.size();
            auto rect = [&](int c) { glm::vec3 t = fm.top(c); return glm::vec4{t.x - fm.chunks[c].half.x, t.z - fm.chunks[c].half.y, t.x + fm.chunks[c].half.x, t.z + fm.chunks[c].half.y}; };
            auto gap = [&](int a, int b) { glm::vec4 A = rect(a), Bq = rect(b);
                float dx = std::max(0.f, std::max(A.x - Bq.z, Bq.x - A.z)), dz = std::max(0.f, std::max(A.y - Bq.w, Bq.y - A.w)); return std::sqrt(dx * dx + dz * dz); };
            std::vector<char> seen(n, 0); std::vector<int> q{0}; seen[0] = 1;   // chunk 0: the Yard, where the bridge lands
            while (!q.empty()) {
                int a = q.back(); q.pop_back();
                for (int b = 0; b < n; ++b) {
                    if (seen[b]) continue;
                    float up = fm.top(b).y - fm.top(a).y;
                    bool hop = gap(a, b) <= 9.f && up <= 3.f;
                    bool hook = fm.chunks[b].movers.size() > 1 && gap(a, b) <= GRAPPLE;   // big chunks carry a pillar
                    if (hop || hook) { seen[b] = 1; q.push_back(b); }
                }
            }
            for (int c = 0; c < n; ++c) if (!seen[c]) { reach = false; std::printf("      arrangement %d: chunk %d unreachable\n", k, c); }
        }
        CHECK(reach, "in every arrangement every chunk can be reached from the Yard (a jump and a dash, or a grapple)");
        // Gliding, no chunk passes through another or through the static world
        bool clean = true;
        for (int k = 0; k < 3; ++k) {
            fm.at = fm.to = k; fm.t = 0.f; fm.apply(N); N.updateMovers(clock); fm.glideTo(k + 1);
            for (int i = 0; i < 60 * 7; ++i) {
                clock += DT; fm.update(DT, N); N.updateMovers(clock);
                if (i % 6) continue;
                for (int a = 0; a < (int)fm.chunks.size(); ++a)
                    for (int ma : fm.chunks[a].movers) {
                        AABB A = N.walls[N.movers[ma].wall].box;
                        for (int b = a + 1; b < (int)fm.chunks.size(); ++b)
                            for (int mb : fm.chunks[b].movers) if (overlapsBox(A, N.walls[N.movers[mb].wall].box, 0.02f)) { clean = false; }
                        for (auto& wl : N.walls) if (!wl.dynamic && overlapsBox(A, wl.box, 0.02f)) { clean = false; }
                    }
            }
        }
        CHECK(clean, "gliding, no relic passes through another or through the static world");
        // The way in: a bridge from the pit's south wall to the Yard; the way out: the hole's rim
        fm.reset(N); N.updateMovers(0.f);
        float bridge = N.groundAt(0.f, -890.f, -239.f), landing = N.groundAt(fm.top(0).x, fm.top(0).z, fm.top(0).y + 0.5f);
        CHECK(std::fabs(bridge + 240.f) < 0.05f && std::fabs(landing - fm.top(0).y) < 0.05f && N.arenas[2].exitDoor >= 0,
              "a bridge leads from a door in the pit's south wall out to the Yard");
        fm.at = fm.to = 3; fm.apply(N); N.updateMovers(0.f);
        CHECK(std::fabs(N.groundAt(N.finishPos.x, N.finishPos.z, N.finishPos.y + 0.5f) - N.finishPos.y) < 0.05f && N.finishPos.z < -1000.f,
              "the finish stands on the hole's rim, past the last arrangement");
        CHECK(MUSIC_TRACKS == 9 && std::string(musicTrack(8).name) == "RELIQUARY", "the Reliquary has its own track");
    }
```
Update the tests that pinned the old shape: the act-2 arena count (`N.arenas.size() == 3` → 4 wherever asserted), `MUSIC_TRACKS == 8` → 9, the Descent's basin assertion (now ends at Z −880), and the simulated ACT II run's victory arena (`d.arena == 2` → `3`, with the Reliquary ridden as below):
```cpp
            if (d.arena == 3) {   // the Reliquary: the relics drift into the next wave's arrangement first
                int want = std::min(d.wave + (d.phase == WaveDirector::Phase::BREAK ? 1 : 0), 3);
                if (N.formation.at != want && !N.formation.gliding()) N.formation.glideTo(want);
                N.formation.update(DT, N);
                d.hold = N.formation.gliding();
                player = N.formation.top(0) + glm::vec3{0, 0.05f, 0};
            }
```
(and its CHECK message: "... kills the Penitent and clears the Reliquary").

- [ ] **Step 2: Run to verify it fails** — `make test 2>&1 | grep -E "FAIL|error" | head` → the Reliquary tests fail.

- [ ] **Step 3: Implement.**

`src/MusicSynth.h`: `MUSIC_TRACKS = 9`; add after DESCENT:
```cpp
        // The Reliquary: sparse and hollow, a slow bell over a far-off choir pad
        {"RELIQUARY", 120.f, 41, {0, 3, 6, 4},
         "x.......x.......", "........x.......", "x...x.X.x...x.Xo",
         "x---.---o---.--f", "0...2...1...3...", "3-------------2-1---------------"},
```
`src/Gameplay_Flow.h` (track pick): `act2() ? 5 + std::min(a, 3)`.
`src/MenuState.h`: `"PREVIEW - 3/4 ARENAS - BENEATH THE ECLIPSE"` → `"PREVIEW - 4/4 ARENAS - BENEATH THE ECLIPSE"`.
`src/Gameplay_Combat.h` (the Penitent's death): the subtitle `"REACH THE BEACON IN THE PIT"` → `"THE SOUTH WALL HAS OPENED"`.

`src/LevelAct2.h`:
1. Basins: the Descent's `aabb(-300, 0, -788, 300, 0, -1000)` → `aabb(-300, 0, -788, 300, 0, -880)`; add `L.basins.push_back({aabb(-300, 0, -880, 300, 0, -1200), -500.f});   // the Reliquary: the void, far below`.
2. The shaft wall loop in `buildDescent`: a south gap at pit level —
```cpp
        bool exit = std::fabs(p.x) < 5.5f && p.z < C.z;   // the way on: the pit's south wall
        if (exit) {
            B.solid(p.x - 1.6f, PIT - 6.f, p.z - 1.6f, p.x + 1.6f, PIT, p.z + 1.6f);
            B.solid(p.x - 1.6f, PIT + 8.f, p.z - 1.6f, p.x + 1.6f, TOP + 16.f, p.z + 1.6f);
            continue;
        }
```
(before the existing `B.solid`), the drawn curve split to leave the gap (two `curve` calls: from `-π/2 + 0.17` round to `3π/2 - 0.17` — the gap centred on the south), and after the entry gate: `a.exitDoor = B.doorway(true, -4, 4, C.z - RW - 2.4f, C.z - RW - 0.2f, PIT, 7.f, blood, true);`.
3. `L.finishPos` moves out of `buildDescent` (delete that line; `buildReliquary` sets it).
4. `buildReliquary(L, B)` called from `buildAct2` after `buildDescent`:

```cpp
// =============================================================================
// THE RELIQUARY — pieces of the four Act I arenas, torn loose, drifting over
// the void south of the Descent. They regroup between waves (LevelData::
// Formation, ArenaShift::DRIFT): scattered, a ring, a stack, then a path to
// the hole where something waits (piece 6).
// =============================================================================
static constexpr glm::vec3 RELIQUARY_Q{0.f, -240.f, -960.f};

inline void buildReliquary(LevelData& L, LevelBuilder& B) {
    using vec3 = glm::vec3;
    const vec3 Q = RELIQUARY_Q;
    const vec3 brick{0.45f, 0.25f, 0.2f}, rust{0.36f, 0.22f, 0.15f}, stone{0.55f, 0.53f, 0.5f}, slate{0.24f, 0.28f, 0.34f},
               debrisCol{0.2f, 0.19f, 0.2f}, amber{1.2f, 0.7f, 0.3f}, ember{1.3f, 0.45f, 0.15f}, bone{0.9f, 0.85f, 0.75f},
               cyan{0.2f, 1.f, 1.1f}, blood{1.f, 0.15f, 0.08f};
    auto wall = [&](float x0, float y0, float z0, float x1, float y1, float z1, vec3 c) { return B.wall(x0, y0, z0, x1, y1, z1, c); };

    // ---- the bridge out of the pit's south wall ----
    wall(-3, Q.y - 1, -906, 3, Q.y, -873, debrisCol);
    L.corridors.push_back(B.aabb(-5, Q.y, -906, 5, Q.y + 12, -872));

    // ---- a relic: boxes (local to its top centre, top at y 0) moving as one ----
    struct Box { vec3 lo, hi; vec3 col; };
    auto relic = [&](vec3 top, vec2 half, vec3 col, vec3 glow, const std::vector<Box>& extra, std::vector<vec3> at) {
        LevelData::Formation::Chunk c; c.home = top; c.half = half; c.at = std::move(at);
        auto add = [&](vec3 lo, vec3 hi, vec3 bc) {
            int m = B.mover(top + (lo + hi) * 0.5f, (hi - lo) * 0.5f, Mover::Path::DRIVEN, {0, 0, 0}, {0, 0, 0}, 1.f, 0.f, glow);
            L.movers[m].color = bc;
            c.movers.push_back(m);
        };
        add({-half.x, -1.5f, -half.y}, {half.x, 0.f, half.y}, col);   // the slab
        for (const Box& b : extra) add(b.lo, b.hi, b.col);
        L.formation.chunks.push_back(c);
    };
    // Arrangements (offsets from where each is built): 0 scattered, 1 the ring, 2 the stack, 3 the path to the hole
    // (positions are the tops' centres relative to Q; at[k] = pos[k] - pos[0])
    auto offsets = [&](std::vector<vec3> pos) { std::vector<vec3> o; for (auto& p : pos) o.push_back(p - pos[0]); return o; };
    // The Yard: brick, a corner of its low wall, the dais, a lamp post to grapple
    std::vector<vec3> yardPos{{0, 0, 45}, {0, 0, 32}, {0, 0, 30}, {0, 0, 28}};
    relic(Q + yardPos[0], {10, 10}, brick, amber,
          {{{-10, 0, -10}, {10, 1.2f, -9.4f}, brick * 0.8f}, {{-10, 0, -10}, {-9.4f, 1.2f, 10}, brick * 0.8f},
           {{-3, 0, -3}, {3, 0.8f, 3}, brick * 1.1f}, {{7.4f, 0, 7.4f}, {8.6f, 9, 8.6f}, debrisCol}}, offsets(yardPos));
    // The Foundry: rust, a cold furnace, the empty channel's walls, a chimney to grapple
    std::vector<vec3> foundryPos{{-38, 0, 0}, {-32, 0, 0}, {-24, 3, 8}, {0, 0, 9}};
    relic(Q + foundryPos[0], {9, 8}, rust, ember,
          {{{-9, 0, -8}, {-5, 5, -4}, rust * 0.7f}, {{-3, 0, -6}, {-2.4f, 1.f, 6}, rust * 0.9f}, {{2.4f, 0, -6}, {3, 1.f, 6}, rust * 0.9f},
           {{6, 0, -7.2f}, {7.2f, 10, -6}, debrisCol}}, offsets(foundryPos));
    // The Spire: pale stone, a raised ledge, columns
    std::vector<vec3> spirePos{{38, 1, -8}, {32, 0, 0}, {22, 9, -6}, {0, 0, -27}};
    relic(Q + spirePos[0], {7, 7}, stone, bone,
          {{{-7, 0, -7}, {-1, 2.5f, 7}, stone * 0.9f}, {{3.5f, 0, -4.5f}, {4.5f, 8, -3.5f}, stone}, {{3.5f, 0, 3.5f}, {4.5f, 8, 4.5f}, stone}}, offsets(spirePos));
    // The Core: slate, two of its pillars
    std::vector<vec3> corePos{{0, 0, -40}, {0, 0, -32}, {-4, 6, -18}, {0, 0, -9}};
    relic(Q + corePos[0], {9, 9}, slate, cyan,
          {{{-6, 0, -1}, {-4, 7, 1}, slate * 0.8f}, {{4, 0, -1}, {6, 7, 1}, slate * 0.8f}}, offsets(corePos));
    // Debris: stepping stones (5 x 5)
    const vec3 DEBRIS[8][4] = {
        {{-16, 0, 30}, {21.2f, 0, 21.2f},  {-12, 1.5f, 20}, {-16, 0, 20}},
        {{-27, 0, 17}, {-21.2f, 0, 21.2f}, {-18, 4.5f, -6}, {16, 0, 20}},
        {{16, 0, 30},  {-21.2f, 0, -21.2f},{9, 7.5f, -18},  {-16, 0, -8}},
        {{27, 0, 17},  {21.2f, 0, -21.2f}, {12, 1.5f, 22},  {16, 0, -8}},
        {{-27, 0, -20},{27.7f, 0, 11.5f},  {24, 4.5f, 12},  {0, 0, -40}},
        {{-14, 0, -32},{-27.7f, 0, 11.5f}, {-30, 0, 24},    {0, 0, -47}},
        {{27, 0, -24}, {-27.7f, 0, -11.5f},{30, 0, -24},    {-16, 0, -30}},
        {{14, 0, -32}, {27.7f, 0, -11.5f}, {-30, 0, -24},   {16, 0, -30}},
    };
    for (auto& d : DEBRIS) relic(Q + d[0], {2.5f, 2.5f}, debrisCol, blood * 0.6f, {}, offsets({d[0], d[1], d[2], d[3]}));
    // Orbiters: small stones circling 6 m under the relics (a catch if you fall); no spawns on them
    for (int k = 0; k < 3; ++k)
        B.mover(Q + vec3{0, -6.5f, 0}, {1.8f, 0.5f, 1.8f}, Mover::Path::ORBIT, {16, 0, 0}, {0, 0, 16}, 40.f, k / 3.f, blood * 0.5f);
    L.formation.reset(L);

    // ---- the rim of the hole, and what waits below it (piece 6) ----
    wall(-8, Q.y - 1, Q.z - 62, 8, Q.y, Q.z - 50, debrisCol);
    B.kit(true).curve({Q.x, 0.f, Q.z - 80.f}, 18.f, 0.f, 6.2831853f, Q.y - 40.f, Q.y - 39.8f, 0.4f, blood, 48);   // a glow far down
    for (int k = 0; k < 10; ++k) {
        float yaw = k * 0.628f;
        B.kit().rock({Q.x + std::cos(yaw) * 20.f, Q.y - 3.f, Q.z - 80.f + std::sin(yaw) * 20.f}, 2.f, 3.f, debrisCol, (uint32_t)(40 + k));
    }
    L.finishPos = {Q.x, Q.y, Q.z - 56.f};

    // ---- the arena ----
    Arena a;
    a.name = "THE RELIQUARY"; a.space = ReverbSpace::HALL;
    a.subtitle = "THE OLD WORLD, IN PIECES - SURVIVE 3 WAVES";
    a.bounds = B.aabb(-56, Q.y - 20, Q.z - 66, 56, Q.y + 30, Q.z + 60);
    a.zone   = B.aabb(-58, Q.y - 30, Q.z - 68, 58, Q.y + 40, Q.z + 92);
    a.playerStart = {0.f, Q.y, -880.f};
    a.startYaw = -90.f;
    a.respawn = Q + yardPos[0]; a.hasRespawn = true;
    a.voidY = Q.y - 22.f;
    // Spawns: three on each relic in each wave's arrangement (clear of its props), fliers above the middle
    const vec3 LOCAL[4][3] = {{{5, 0, 6}, {-5, 0, 6}, {-6, 0, -5}}, {{6, 0, 5}, {-6, 0, 4}, {5, 0, -2}},
                              {{-4, 2.5f, 0}, {2, 0, 0}, {5, 0, -6}}, {{0, 0, 5}, {0, 0, -5}, {-6, 0, 6}}};
    for (int w = 0; w < 3; ++w) {
        std::vector<vec3> g, air;
        for (int c = 0; c < 4; ++c)
            for (const vec3& l : LOCAL[c]) g.push_back(L.formation.chunks[c].home + L.formation.chunks[c].at[w] + l);
        for (int k = 0; k < 4; ++k) {
            float yaw = k * 1.5707963f + 0.7853982f;
            air.push_back(Q + vec3{std::cos(yaw) * 16.f, 12.f + 3.f * w, std::sin(yaw) * 16.f});
        }
        a.waveGround.push_back(g); a.waveAir.push_back(air);
    }
    a.groundSpawns = a.waveGround[0]; a.airSpawns = a.waveAir[0];
    a.waves = {
        {{EnemyType::HUSK, 4}, {EnemyType::RAPTOR, 3}, {EnemyType::REVENANT, 2},
         WaveEntry(EnemyType::SHIELDBEARER, 2).with({EnemyType::HUSK}), {EnemyType::SERAPH, 1}},
        {{EnemyType::WEAVER, 2}, {EnemyType::REVENANT, 2}, WaveEntry(EnemyType::REVENANT, 1).hollow(Hollow::ENRAGED),
         {EnemyType::ANCHOR, 1}, {EnemyType::SENTINEL, 2}, {EnemyType::RIPPER, 3}, {EnemyType::MITE, 4}},
        {{EnemyType::JUGGERNAUT, 1}, WaveEntry(EnemyType::REVENANT, 2).hollow(Hollow::HALOED), {EnemyType::WEAVER, 2},
         {EnemyType::SERAPH, 2}, {EnemyType::CONDUCTOR, 1}, WaveEntry(EnemyType::HUSK, 3).hollow(Hollow::TWINNED)},
    };
    a.goals = {WaveGoal{}, WaveGoal{}, WaveGoal{}};
    a.maxAlive = 12;
    a.damageScale = 1.45f;
    a.shift = ArenaShift::DRIFT;
    a.ambient = Ambient::MOTES;
    a.theme = Theme{
        {0.012f,0.008f,0.014f}, {0.16f,0.06f,0.08f}, {0.006f,0.003f,0.004f},
        glm::normalize(vec3{0.3f, 0.85f, -0.4f}), {0.55f,0.5f,0.6f}, 0.06f, 0.f,
        {0.1f,0.03f,0.04f}, 1.f,
        glm::normalize(vec3{-0.3f,-0.7f,0.6f}), {0.5f,0.2f,0.18f},
        {0.12f,0.08f,0.1f}, {0.03f,0.02f,0.025f},
        {0.08f,0.03f,0.04f}, 0.01f };
    L.arenas.push_back(std::move(a));
}
```
(Match `Theme`'s field order to the Descent's initializer in the same file; `LevelBuilder::aabb`, `B.kit().rock`, `B.kit(true).curve`, `B.mover`, `B.wall` are the builder's existing helpers. If `EnemyType::REVENANT`/`WEAVER` don't exist yet, temporarily use `HUSK`/`SENTINEL` in the wave lists and switch them in Task 3/4 — ledger it.)

- [ ] **Step 4: Run and tune.** `make test 2>&1 | grep -E "wave . spawn|unreachable|FAIL|ALL PASSED|FAILED"`. If a spawn sits on a prop or a debris piece is unreachable or a glide overlaps, move the debris positions (not the four relics) until the tests pass; ledger the final table.

- [ ] **Step 5: Look at it.** `./shooter --act2 --arena 4 --god --cam 0 -215 -900 -90 -25 --shot 120 <scratch>/r0.bmp` (wave 1 from above the bridge); convert and read. Adjust what reads badly; ledger it.

- [ ] **Step 6: Commit**
```bash
git add src/LevelAct2.h src/MusicSynth.h src/Gameplay_Flow.h src/MenuState.h src/Gameplay_Combat.h tests/test_game.cpp
git commit -m "The Reliquary: the four Act I arenas in pieces over the void, three arrangements and a path to the hole; out through the pit's south wall"
```

---

### Task 3: The Revenant

**Files:** Modify `src/Enemy.h`, `src/Progression.h`, `src/VoiceTable.h`, `src/VoiceSynth.h`, `Makefile`; Create `src/EnemyRelic.h`, `src/RelicHazards.h`; Test `tests/test_game.cpp`.

**Interfaces:**
- Produces: `EnemyType::REVENANT` (15), `EnemyType::WEAVER` (16, stats + stub mind here; its mind in Task 4); `AttackKind::{RAKE, SOULBOLT, STRING}`; `Enemy::reforms` (int); `EnemyEvents::comboHit`; `struct Soul { int fromUid; EnemyType type; Hollow hollow; glm::vec3 pos, from, to; float t, hp, bodyHealth, scale; int reforms; bool alive; }`; `class RelicHazards { std::vector<Soul> souls; std::vector<Wire> wires; static constexpr float SOUL_TIME = 4.f, SOUL_HP = 40.f, SOUL_RADIUS = 0.6f; Soul* releaseSoul(const Enemy& e, glm::vec3 to); int raySoul(glm::vec3 o, glm::vec3 d, float maxT, float& t) const; bool hurtSoul(int i, float dmg); int soulNear(glm::vec3 p, float r) const; std::vector<Soul> arrived(float dt); void clear(); }` and `glm::vec3 soulDestination(const std::vector<glm::vec3>& spawns, glm::vec3 from, float minDist)`.

- [ ] **Step 1: Write the failing tests:**

```cpp
    // ---------------------------------------------------------------- the Revenant
    {
        LevelData N = buildAct2Level(); SpatialGrid g; g.build(N.walls);
        const Arena& R = N.arenas[3];
        N.formation.reset(N); N.updateMovers(0.f);
        auto world = [&](glm::vec3 feet) { EnemyWorld w; w.playerFeet = feet; w.playerEye = feet + glm::vec3{0, 1.7f, 0};
                                           w.walls = N.walls.data(); w.wallCount = (int)N.walls.size(); w.grid = &g; w.bounds = R.bounds; return w; };
        CHECK(statsOf(EnemyType::REVENANT).health == 160.f && statsOf(EnemyType::WEAVER).health == 120.f, "a Revenant has 160 health, a Weaver 120");
        // It rakes up close and bolts at range, each with its tell
        glm::vec3 yard = N.formation.top(0);
        auto run = [&](float dist) {
            Enemy e(EnemyType::REVENANT, yard + glm::vec3{0, 0, -dist}); e.spawnTimer = 0.f; e.state = EnemyState::ACTIVE;
            EnemyWorld w = world(yard + glm::vec3{0, 0, 3});
            int rakes = 0, bolts = 0, hits = 0; float rakeTell = 0.f, boltTell = 0.f;
            for (int f = 0; f < 60 * 10; ++f) {
                e.update(DT, w);
                if (e.ev.telegraphStarted && e.attack == AttackKind::RAKE) { ++rakes; rakeTell = e.telegraphDuration; }
                if (e.ev.telegraphStarted && e.attack == AttackKind::SOULBOLT) { ++bolts; boltTell = e.telegraphDuration; }
                hits += e.ev.meleeHit;
            }
            return std::tuple<int, int, int, float, float>{rakes, bolts, hits, rakeTell, boltTell};
        };
        auto [r1, b1, h1, rt, bt0] = run(1.f);
        auto [r2, b2, h2, rt2, bt] = run(9.f);
        CHECK(r1 >= 2 && h1 >= 2 * r1 - 1 && std::fabs(rt - 0.45f) < 0.05f, "up close it rakes, two hits a time (0.45 s tell)");
        CHECK(b2 >= 2 && std::fabs(bt - 0.6f) < 0.05f, "at range it throws soul bolts (0.6 s tell)");
        // Its soul: flees toward another chunk, can be shot, punched, or re-forms at half
        RelicHazards hz;
        Enemy dead(EnemyType::REVENANT, yard); dead.maxHealth = dead.health = 160.f;
        glm::vec3 to = soulDestination(R.waveGround[0], yard, 15.f);
        Soul* s = hz.releaseSoul(dead, to);
        CHECK(s && glm::length(to - yard) >= 15.f && s->bodyHealth == 80.f && s->reforms == 1, "a dying Revenant releases a soul toward a spot on another chunk, carrying half its health");
        float t = 0.f; int hit = hz.raySoul(s->pos + glm::vec3{0, 0, 10}, {0, 0, -1}, 50.f, t);
        bool shot = hit == 0 && hz.hurtSoul(0, 40.f) && hz.souls.empty();
        CHECK(shot, "a soul shot for 40 is gone for good");
        hz.releaseSoul(dead, to);
        bool punched = hz.soulNear(hz.souls[0].pos + glm::vec3{1, 0, 0}, 2.5f) == 0;
        CHECK(punched, "a soul passing within reach can be punched");
        hz.clear(); hz.releaseSoul(dead, to);
        std::vector<Soul> back; float flown = 0.f;
        for (int f = 0; f < 60 * 6 && back.empty(); ++f) { auto a = hz.arrived(DT); flown += DT; back.insert(back.end(), a.begin(), a.end()); }
        CHECK(back.size() == 1 && std::fabs(flown - RelicHazards::SOUL_TIME) < 0.1f && glm::length(back[0].pos - to) < 0.1f,
              "an uncaught soul arrives after about 4 s at its spot");
        Enemy twice(EnemyType::REVENANT, yard); twice.reforms = 2;
        CHECK(hz.releaseSoul(twice, to) == nullptr, "after two re-forms, the third death is final");
        // Review focus 4: twins each flee
        Enemy twin(EnemyType::REVENANT, yard); twin.setHollow(Hollow::TWINNED); twin.scale = 0.7f;
        hz.clear();
        CHECK(hz.releaseSoul(twin, to) && hz.releaseSoul(twin, to) && hz.souls.size() == 2 && hz.souls[0].hollow == Hollow::TWINNED,
              "each Twinned Revenant carries its own soul");
    }
```

- [ ] **Step 2: Run to verify it fails** — compile errors (`REVENANT`, `RelicHazards`).

- [ ] **Step 3: Implement.**

`src/Enemy.h`:
- `EnemyType`: `..., ANCHOR, PENITENT, REVENANT, WEAVER, COUNT`.
- `AttackKind`: append `, RAKE, SOULBOLT, STRING   // the REVENANT's, the WEAVER's`.
- Stats rows after PENITENT:
```cpp
        {"REVENANT", 160.f, 0.5f, 2.3f, 5.4f, 0.45f, 1.6f, false,
         {0.2f,0.21f,0.26f}, {0.75f,0.9f,1.3f}, {0.6f,0.85f,1.2f},
         "ITS SOUL RUNS - CATCH IT BEFORE IT COMES BACK"},
        {"WEAVER", 120.f, 0.9f, 1.3f, 4.2f, 0.8f, 6.f, false,
         {0.16f,0.13f,0.2f}, {0.75f,0.35f,1.1f}, {0.75f,0.35f,1.1f},
         "IT WIRES THE GAPS - CUT THE NODES OR DUCK UNDER"},
```
- Enemy fields (after the WARDEN block): `int reforms = 0;   // REVENANT: how many times its soul has come back` and `int comboLeft2 = 0;` is not needed — reuse `comboLeft` (exists).
- `canBeHollow` unchanged (both may be Hollowed).
- `update` dispatch: `case EnemyType::REVENANT: thinkRevenant(dt, w, resolve); break;` and `case EnemyType::WEAVER: thinkWeaver(dt, w, resolve); break;`; declarations `void thinkRevenant(float dt, const EnemyWorld& w, bool resolve);   // EnemyRelic.h` and `void thinkWeaver(...)`; at the end of the file `#include "EnemyRelic.h"`.
- ROSTER comment lines for both.
- EnemyEvents: `bool wire = false; glm::vec3 wireA{0.f}, wireB{0.f};   // WEAVER: a wire strung between A and B`.

`src/EnemyRelic.h`:
```cpp
#pragma once
// =============================================================================
// EnemyRelic.h — the Reliquary's two: THE REVENANT (rakes up close, soul bolts
// at range; when it dies its soul flees - RelicHazards.h) and THE WEAVER
// (keeps its distance, strings wires across your way). Minds only; events out.
// Included at the end of Enemy.h.
// =============================================================================

inline void Enemy::thinkRevenant(float dt, const EnemyWorld& w, bool resolve) {
    glm::vec3 to = flatTo(w.playerFeet);
    float d = glm::length(to);
    if (telegraphTimer > 0.f) { velocity.x = velocity.z = 0.f; }
    else if (d > 2.4f) { setMove(norm2(to), stats().speed, w); animPhase += dt * 3.f; }
    else velocity.x = velocity.z = 0.f;
    turnToward(to, dt, 6.f);
    if (resolve) {
        AttackKind k = attack;
        attack = AttackKind::NONE;
        if (k == AttackKind::RAKE) {
            if (d < 3.2f) { ev.meleeHit = true; ev.meleeDamage = 14.f; }
            if (comboLeft > 0) { --comboLeft; startAttack(AttackKind::RAKE, 0.25f); return; }   // the second hand
        }
        if (k == AttackKind::SOULBOLT) { fireAt(w.playerEye, 1, 0.f, 14.f, 14.f, 1.2f); ev.shotParry = 60.f; }
    }
    if (!attackReady(dt)) return;
    if (d < 3.2f) { comboLeft = 1; startAttack(AttackKind::RAKE, stats().telegraph); }
    else if (d > 8.f && lineOfSight(eyePos(), w)) startAttack(AttackKind::SOULBOLT, 0.6f);
}

inline void Enemy::thinkWeaver(float dt, const EnemyWorld& w, bool resolve) {
    (void)resolve; velocity.x = velocity.z = 0.f;   // Task 4 gives it its wires
    (void)dt; (void)w;
}
```
Note: the rake's second hit is a follow-up (`startAttack(..., 0.25f)`), so its `telegraphDuration` for the first is `stats().telegraph` (0.45 × difficulty windup); the test uses the default difficulty.

`src/RelicHazards.h`:
```cpp
#pragma once
// =============================================================================
// RelicHazards.h — the Reliquary's: REVENANT souls in flight (shoot them, punch
// them, or they re-form the body at a spot on another chunk) and WEAVER wires
// (Task 4). No OpenGL or audio: Gameplay_Reliquary.h feeds and draws them.
// =============================================================================
#include "Enemy.h"
#include <vector>
#include <cmath>
#include <algorithm>

struct Soul {
    int fromUid = 0; EnemyType type = EnemyType::REVENANT; Hollow hollow = Hollow::NONE;
    glm::vec3 pos{0.f}, from{0.f}, to{0.f};
    float t = 0.f, hp = 40.f, bodyHealth = 80.f, scale = 1.f;
    int reforms = 1;
};

// Where a soul goes: the spawn spot furthest from where it died, at least minDist away
inline glm::vec3 soulDestination(const std::vector<glm::vec3>& spawns, glm::vec3 from, float minDist) {
    glm::vec3 best = from; float bd = -1.f;
    for (auto& s : spawns) { float d = glm::length(s - from); if (d >= minDist && (bd < 0.f || d < bd)) { bd = d; best = s; } }
    if (bd < 0.f) for (auto& s : spawns) { float d = glm::length(s - from); if (d > bd) { bd = d; best = s; } }
    return best;
}

class RelicHazards {
public:
    static constexpr float SOUL_TIME = 4.f, SOUL_HP = 40.f, SOUL_RADIUS = 0.6f, SOUL_ARC = 6.f;
    std::vector<Soul> souls;

    // A Revenant's body died: its soul flees to `to` (nullptr: that was its last life)
    Soul* releaseSoul(const Enemy& e, glm::vec3 to) {
        if (e.reforms >= 2) return nullptr;
        Soul s; s.fromUid = e.uid; s.type = e.type; s.hollow = e.hollow; s.scale = e.scale;
        s.from = s.pos = e.position + glm::vec3{0.f, e.height() * 0.6f, 0.f};
        s.to = to; s.hp = SOUL_HP; s.bodyHealth = e.maxHealth * 0.5f; s.reforms = e.reforms + 1;
        souls.push_back(s);
        return &souls.back();
    }
    // Ray (o along d, up to maxT) against the souls: the nearest hit, or -1
    int raySoul(glm::vec3 o, glm::vec3 d, float maxT, float& t) const {
        int best = -1; float bt = maxT;
        for (int i = 0; i < (int)souls.size(); ++i) {
            glm::vec3 oc = souls[i].pos - o;
            float along = glm::dot(oc, d);
            if (along < 0.f || along > bt) continue;
            if (glm::length(oc - d * along) < SOUL_RADIUS) { bt = along; best = i; }
        }
        t = bt; return best;
    }
    // True when that killed it (it's removed)
    bool hurtSoul(int i, float dmg) {
        if (i < 0 || i >= (int)souls.size()) return false;
        souls[i].hp -= dmg;
        if (souls[i].hp > 0.f) return false;
        souls.erase(souls.begin() + i);
        return true;
    }
    int soulNear(glm::vec3 p, float r) const {
        for (int i = 0; i < (int)souls.size(); ++i) if (glm::length(souls[i].pos - p) < r) return i;
        return -1;
    }
    void removeSoul(int i) { if (i >= 0 && i < (int)souls.size()) souls.erase(souls.begin() + i); }
    // Fly; returns the souls that reached their spot this tick (removed here: the game re-forms them)
    std::vector<Soul> arrived(float dt) {
        std::vector<Soul> out;
        for (auto& s : souls) {
            s.t += dt;
            float u = std::min(1.f, s.t / SOUL_TIME);
            float e = u * u * (3.f - 2.f * u);
            s.pos = glm::mix(s.from, s.to, e) + glm::vec3{0.f, std::sin(u * 3.1415927f) * SOUL_ARC, 0.f};
            if (u >= 1.f) out.push_back(s);
        }
        souls.erase(std::remove_if(souls.begin(), souls.end(), [](const Soul& s) { return s.t >= SOUL_TIME; }), souls.end());
        return out;
    }
    void clear() { souls.clear(); }
};
```
(`Soul::to` is the spawn spot's feet; the soul's `pos` ends exactly there — `glm::length(back[0].pos - to) < 0.1f`.)

`src/Progression.h` `xpForKill`: `case EnemyType::REVENANT: return 45; case EnemyType::WEAVER: return 50;`.

`src/VoiceTable.h`: `voiceKey` K gains `"revenant", "weaver"`; `moveCadence` C gains `2.6f, 5.f`; `attacksOf`: `REVENANT {A::RAKE, A::SOULBOLT}`, `WEAVER {A::STRING}`; `attackKey`: `RAKE "rake"`, `SOULBOLT "soulbolt"`, `STRING "string"`; `hasRelease`: `RAKE`, `SOULBOLT` true; `tellDur`: `RAKE 0.45f`, `SOULBOLT 0.6f`, `STRING 0.8f`; specials: `sp("v_revenant_soul", EnemyType::REVENANT, 1, MixClass::TELL, "soul"); sp("v_revenant_reform", EnemyType::REVENANT, 1, MixClass::TELL, "reform"); sp("v_weaver_twang", EnemyType::WEAVER, 2, MixClass::ACTION, "twang");`.

`src/VoiceSynth.h`: `profileOf`: `case EnemyType::REVENANT: return {165.f, Vowel::OO, 3, 1.05f, Body::ARMOUR, 700.f, 0.35f};` `case EnemyType::WEAVER: return {0.f, Vowel::NONE, 0, 1.f, Body::SKITTER, 1600.f, 0.3f};`. `tellV` cases:
```cpp
        // THE REVENANT / THE WEAVER
        case AttackKind::RAKE:     add(out, vent(d, 2600.f, r, d * 0.7f, 0.03f)); add(out, metal(0.12f, 900.f, r, 30.f), 0.6f, d - 0.12f); break;
        case AttackKind::SOULBOLT: add(out, choir(d, f * 0.9f, f * 1.6f, Vowel::OO, p.voices, p.formant, r, d * 0.8f, 0.05f, 0.4f)); break;
        case AttackKind::STRING:   add(out, servo(d, 300.f, 1400.f, r, d * 0.8f, 0.03f), 0.8f);
                                   for (float tt = 0.f; tt < d - 0.05f; tt += 0.06f) add(out, click(2200.f, r, 0.02f), 0.4f, tt); break;
```
`specialV`: `if (k == "soul") return choir(0.9f, 300.f, 700.f, Vowel::OO, 3, 1.1f, r, 0.05f, 0.4f, 0.4f, 7.f);`
`if (k == "reform") { Buf out = choir(1.f, 140.f, 220.f, Vowel::OH, 4, 1.f, r, 0.8f, 0.15f, 0.2f); add(out, shimmer(1.f, 2400.f, r), 0.3f); return out; }`
`if (k == "twang") { Buf out = metal(0.3f, 1300.f, r, 9.f); add(out, servo(0.25f, 900.f, 300.f, r, 0.002f, 0.2f), 0.5f); return out; }`

Makefile: `src/EnemyRelic.h src/RelicHazards.h` in HEADERS and the test list; `#include "../src/RelicHazards.h"` in the tests.

- [ ] **Step 4: Run** — `make test 2>&1 | grep -E "FAIL|ALL PASSED|FAILED|wants|no tell"` → `ALL PASSED`. (The voice tests cover the new tells; switch the Reliquary waves from placeholder types if Task 2 ruled them in.)

- [ ] **Step 5: Commit**
```bash
git add src/Enemy.h src/EnemyRelic.h src/RelicHazards.h src/Progression.h src/VoiceTable.h src/VoiceSynth.h src/LevelAct2.h Makefile tests/test_game.cpp
git commit -m "The Revenant: rakes and soul bolts; when it dies its soul flees for another chunk"
```

---

### Task 4: The Weaver and its wires

**Files:** Modify `src/EnemyRelic.h`, `src/RelicHazards.h`; Test `tests/test_game.cpp`.

**Interfaces:**
- Produces: `struct Wire { int owner; glm::vec3 a, b; float age; }`; `RelicHazards::wires`, `static constexpr float WIRE_HEIGHT = 1.2f, WIRE_LEN = 10.f, NODE_RADIUS = 0.35f, SNARE_TIME = 1.2f, SNARE_DAMAGE = 10.f, WIRE_ARM = 0.1f; static constexpr int MAX_WIRES = 4;`, `void addWire(int owner, glm::vec3 a, glm::vec3 b)`, `static bool wireTouches(const Wire&, glm::vec3 feet, float height, float radius)`, `int touchedWire(glm::vec3 feet, float height, float radius) const` (armed wires only), `int rayNode(glm::vec3 o, glm::vec3 d, float maxT, float& t) const`, `void cutWire(int i)`, `void dropOwner(int owner)`, `void clearWires()`, `void age(float dt)`; `Enemy::thinkWeaver` emitting `ev.wire`, `ev.wireA`, `ev.wireB`.

- [ ] **Step 1: Write the failing tests:**

```cpp
    // ---------------------------------------------------------------- the Weaver
    {
        LevelData N = buildAct2Level(); SpatialGrid g; g.build(N.walls);
        const Arena& R = N.arenas[3];
        N.formation.reset(N); N.updateMovers(0.f);
        glm::vec3 core = N.formation.top(3);
        Enemy wv(EnemyType::WEAVER, core + glm::vec3{0, 0, -5}); wv.spawnTimer = 0.f; wv.state = EnemyState::ACTIVE;
        EnemyWorld w; w.playerFeet = core + glm::vec3{0, 0, 6}; w.playerEye = w.playerFeet + glm::vec3{0, 1.7f, 0};
        w.walls = N.walls.data(); w.wallCount = (int)N.walls.size(); w.grid = &g; w.bounds = R.bounds; w.playerVel = {4, 0, 0};
        int strung = 0; float tell = 0.f; glm::vec3 A{0.f}, Bw{0.f};
        for (int f = 0; f < 60 * 14; ++f) {
            wv.update(DT, w);
            if (wv.ev.telegraphStarted && wv.attack == AttackKind::STRING) tell = wv.telegraphDuration;
            if (wv.ev.wire) { ++strung; A = wv.ev.wireA; Bw = wv.ev.wireB; }
        }
        CHECK(strung >= 2 && std::fabs(tell - 0.8f) < 0.05f, "a Weaver strings a wire every ~6 s after a 0.8 s tell");
        CHECK(std::fabs(A.y - (w.playerFeet.y + RelicHazards::WIRE_HEIGHT)) < 0.05f && std::fabs(glm::length(Bw - A) - RelicHazards::WIRE_LEN) < 0.1f,
              "its wire runs 10 m across your way at chest height");
        RelicHazards hz;
        glm::vec3 F{0, 0, 0};
        hz.addWire(1, {-5, 1.2f, 0}, {5, 1.2f, 0});
        // Review focus 3: not armed on the tick it's strung
        int fresh = hz.touchedWire(F, 1.8f, 0.4f);
        hz.age(DT * 7);
        int standing = hz.touchedWire(F, 1.8f, 0.4f), sliding = hz.touchedWire(F, 0.9f, 0.4f), jumped = hz.touchedWire(F + glm::vec3{0, 1.4f, 0}, 1.8f, 0.4f),
            beside = hz.touchedWire(F + glm::vec3{0, 0, 2}, 1.8f, 0.4f);
        CHECK(fresh < 0, "a fresh wire doesn't snare where it's strung through you");
        CHECK(standing == 0 && sliding < 0 && jumped < 0 && beside < 0, "standing in a wire snares you; sliding under or jumping over doesn't");
        for (int k = 0; k < 4; ++k) hz.addWire(1, {-5, 1.2f, 3.f + k}, {5, 1.2f, 3.f + k});
        CHECK(hz.wires.size() == 4 && hz.wires[0].a.z == 3.f, "at most 4 wires per Weaver: a fifth replaces its oldest");
        float t = 0.f; int node = hz.rayNode({5, 1.2f, 10}, {0, 0, -1}, 50.f, t);
        if (node >= 0) hz.cutWire(node);
        CHECK(node >= 0 && hz.wires.size() == 3, "shooting a wire's node cuts it");
        hz.addWire(2, {0, 1.2f, -5}, {0, 1.2f, 5});
        hz.dropOwner(1);
        CHECK(hz.wires.size() == 1 && hz.wires[0].owner == 2, "a Weaver's death drops all its wires (and only its)");
        hz.clearWires();
        CHECK(hz.wires.empty(), "a drift clears every wire");
    }
```

- [ ] **Step 2: Run to verify it fails** — compile errors.

- [ ] **Step 3: Implement.**

`src/RelicHazards.h` (add before `class RelicHazards`):
```cpp
struct Wire { int owner = 0; glm::vec3 a{0.f}, b{0.f}; float age = 0.f; };
```
inside the class:
```cpp
    static constexpr float WIRE_HEIGHT = 1.2f, WIRE_LEN = 10.f, NODE_RADIUS = 0.35f, SNARE_TIME = 1.2f, SNARE_DAMAGE = 10.f, WIRE_ARM = 0.1f;
    static constexpr int   MAX_WIRES = 4;
    std::vector<Wire> wires;

    void addWire(int owner, glm::vec3 a, glm::vec3 b) {
        int n = 0, oldest = -1;
        for (int i = 0; i < (int)wires.size(); ++i) if (wires[i].owner == owner) { ++n; if (oldest < 0) oldest = i; }
        if (n >= MAX_WIRES && oldest >= 0) wires.erase(wires.begin() + oldest);
        wires.push_back({owner, a, b, 0.f});
    }
    // Does the player's body (feet up to feet+height, this thick) cross the wire?
    static bool wireTouches(const Wire& w, glm::vec3 feet, float height, float radius) {
        if (w.a.y < feet.y + 0.05f || w.a.y > feet.y + height) return false;   // under it (sliding) or over it (jumping)
        glm::vec2 a{w.a.x, w.a.z}, b{w.b.x, w.b.z}, p{feet.x, feet.z};
        glm::vec2 ab = b - a;
        float u = glm::clamp(glm::dot(p - a, ab) / std::max(glm::dot(ab, ab), 1e-6f), 0.f, 1.f);
        return glm::length(p - (a + ab * u)) < radius + 0.08f;
    }
    int touchedWire(glm::vec3 feet, float height, float radius) const {
        for (int i = 0; i < (int)wires.size(); ++i) if (wires[i].age >= WIRE_ARM && wireTouches(wires[i], feet, height, radius)) return i;
        return -1;
    }
    int rayNode(glm::vec3 o, glm::vec3 d, float maxT, float& t) const {
        int best = -1; float bt = maxT;
        for (int i = 0; i < (int)wires.size(); ++i)
            for (glm::vec3 n : {wires[i].a, wires[i].b}) {
                glm::vec3 oc = n - o; float along = glm::dot(oc, d);
                if (along < 0.f || along > bt) continue;
                if (glm::length(oc - d * along) < NODE_RADIUS) { bt = along; best = i; }
            }
        t = bt; return best;
    }
    void cutWire(int i) { if (i >= 0 && i < (int)wires.size()) wires.erase(wires.begin() + i); }
    void dropOwner(int owner) { wires.erase(std::remove_if(wires.begin(), wires.end(), [owner](const Wire& w) { return w.owner == owner; }), wires.end()); }
    void clearWires() { wires.clear(); }
    void age(float dt) { for (auto& w : wires) w.age += dt; }
```
and `clear()` also clears wires.

`src/EnemyRelic.h` — replace the stub:
```cpp
// It hangs back (14-20 m), and every ~6 s strings a wire across where you're
// heading: 10 m long, at chest height, square to your movement
inline void Enemy::thinkWeaver(float dt, const EnemyWorld& w, bool resolve) {
    glm::vec3 to = flatTo(w.playerFeet);
    float d = glm::length(to);
    glm::vec3 dir = norm2(to), side{-dir.z, 0.f, dir.x};
    if (telegraphTimer > 0.f) { velocity.x = velocity.z = 0.f; }
    else {
        strafeTimer -= dt;
        if (strafeTimer <= 0.f) { strafeTimer = frand(2.f, 3.5f); strafeDir = -strafeDir; }
        glm::vec3 mv = d < 14.f ? -dir + side * strafeDir * 0.5f : d > 20.f ? dir : side * strafeDir;
        setMove(mv, stats().speed, w);
        animPhase += dt * 5.f;
    }
    turnToward(to, dt, 4.f);
    if (resolve && attack == AttackKind::STRING) {
        attack = AttackKind::NONE;
        glm::vec3 v{w.playerVel.x, 0.f, w.playerVel.z};
        float sp = glm::length(v);
        glm::vec3 fwd = sp > 0.5f ? v / sp : -dir;                 // across your path (or across the line to it)
        glm::vec3 across{-fwd.z, 0.f, fwd.x};
        glm::vec3 c = w.playerFeet + (sp > 0.5f ? fwd * 4.f : glm::vec3{0.f});
        c.y = w.playerFeet.y + 1.2f;
        ev.wire = true;
        ev.wireA = c - across * 5.f;
        ev.wireB = c + across * 5.f;
    }
    if (telegraphTimer > 0.f) return;
    if (!attackReady(dt)) return;
    if (lineOfSight(eyePos(), w)) startAttack(AttackKind::STRING, stats().telegraph);
}
```

- [ ] **Step 4: Run** — `make test` → `ALL PASSED`.

- [ ] **Step 5: Commit**
```bash
git add src/EnemyRelic.h src/RelicHazards.h tests/test_game.cpp
git commit -m "The Weaver: wires across your way at chest height; duck, jump, cut a node, or kill it"
```

---

### Task 5: The Reliquary in the game

**Files:** Create `src/Gameplay_Reliquary.h`; Modify `src/GameplayState.h`, `src/Gameplay_Combat.h`, `src/Gameplay_Flow.h`, `src/Gameplay_Tick.h`, `src/Gameplay_Render.h`, `src/EnemyModel.h`, `Makefile`, `tests/test_game.cpp`.

**Interfaces:**
- Consumes: Tasks 1–4.
- Produces: `GameplayState::{relic, lastChunk, snareTimer, driftRumble, updateReliquary, onReliquaryEvents, releaseRevenantSoul, catchSoul, gatherReliquaryBoxes}`; `bool revenantSoulEscapes(StyleSource src, bool inVoid)` in `RelicHazards.h`.

- [ ] **Step 1: Write the failing test** (the headless part — the rule for when a soul flees):

```cpp
    {   // Review focus 2: a Revenant lost to the void doesn't come back; killed any other way, its soul flees
        CHECK(!revenantSoulEscapes(StyleSource::ENVIRONMENT, true) && revenantSoulEscapes(StyleSource::ENVIRONMENT, false) &&
              revenantSoulEscapes(StyleSource::FRIENDLY, false) && revenantSoulEscapes(StyleSource::BULLET, false),
              "a Revenant lost to the void doesn't come back; killed any other way, its soul flees");
    }
```
(Use whatever the `StyleSource` enum calls a gunshot if not `BULLET`.)

- [ ] **Step 2: Run to verify it fails** — compile error.

- [ ] **Step 3: Implement.**

`src/RelicHazards.h`: `inline bool revenantSoulEscapes(StyleSource src, bool inVoid) { (void)src; return !inVoid; }` (include `StyleSystem.h` for `StyleSource`).

`src/GameplayState.h`: `#include "RelicHazards.h"`; members:
```cpp
    // THE RELIQUARY (Gameplay_Reliquary.h): souls and wires, the last relic you
    // stood on, the snare, the drift's rumble
    RelicHazards relic;
    int   lastChunk = 0;
    float snareTimer = 0.f;
    bool  driftWasGliding = false;
    void updateReliquary(float dt);
    void onReliquaryEvents(Enemy& e, const EnemyEvents& ev);
    void releaseRevenantSoul(const Enemy& e, bool inVoid);
    bool catchSoulWithPunch();
    void gatherReliquaryBoxes(std::vector<BoxInstance>& out);
```
and `#include "Gameplay_Reliquary.h"` with the other `Gameplay_*` includes.

`src/Gameplay_Reliquary.h`:
```cpp
#pragma once
// =============================================================================
// Gameplay_Reliquary.h — GameplayState: THE RELIQUARY. The relics drift to the
// next wave's arrangement once a wave is cleared (the fight waits for them to
// settle); a fall puts you back on the last relic you stood on; Revenant souls
// flee and re-form; Weaver wires snare and wake the wave; drawing all of it.
// Included at the end of GameplayState.h.
// =============================================================================

inline void GameplayState::updateReliquary(float dt) {
    const Arena& ar = level.arenas[director.arena];
    if (ar.shift != ArenaShift::DRIFT) { relic.clear(); return; }
    auto& fm = level.formation;
    // The drift: started by ArenaShifts on a cleared wave; the fight waits for it
    if (fm.gliding() && !driftWasGliding) {
        relic.clearWires();
        for (auto& p : projSystem.pool) p.alive = false;
        pickups.clear();
        for (int c = 0; c < 4; ++c) audio.playAt("door", fm.top(c), 110, SoundGroup::WORLD);   // stone grinding as each relic lets go
        shake(0.25f, 0.03f);
    }
    if (!fm.gliding() && driftWasGliding) {   // settled: a jolt, and health on two of the relics
        audio.playAt("slam", fm.top(0), 100, SoundGroup::WORLD);
        for (int c : {1, 3}) {
            glm::vec3 p = fm.top(c) + glm::vec3{0.f, 0.6f, 0.f};
            pickups.push_back({p, glm::vec3{0.f}, 1e9f, PickupKind::ORB, p.y - 0.6f, p});
        }
    }
    driftWasGliding = fm.gliding();
    director.hold = fm.gliding();
    // The last relic you stood on: where a fall puts you back
    if (player.onGround) { int c = fm.chunkOfWall(level, player.groundWall); if (c >= 0) lastChunk = c; }
    Arena& mar = level.arenas[director.arena];
    mar.respawn = fm.top(lastChunk) + glm::vec3{0.f, 0.05f, 0.f};
    mar.voidY = fm.lowestTop() - 22.f;
    // Souls: fly; those that get there re-form their Revenant
    for (const Soul& s : relic.arrived(dt)) {
        glm::vec3 at = s.to;
        spawnEnemy(s.type, at, s.hollow);
        Enemy& e = enemies.back();
        e.maxHealth = e.health = s.bodyHealth; e.reforms = s.reforms; e.scale = s.scale;
        audio.playAt("v_revenant_reform", at + glm::vec3{0.f, 1.5f, 0.f}, 128, SoundGroup::ENEMY, true, 0.4f);
        fx.spawnBurst(at + glm::vec3{0.f, 1.f, 0.f}, {0.75f, 0.9f, 1.3f}, 40, 5.f, 0.9f, 6.f);
        ui.feed("IT CAME BACK", {0.75f, 0.9f, 1.3f});
    }
    // Wires: age, snare on touch (once per wire contact)
    relic.age(dt);
    if (snareTimer > 0.f) snareTimer -= dt;
    int wi = relic.touchedWire(player.position, player.height, player.radius);
    if (wi >= 0 && snareTimer <= 0.f) {
        snareTimer = RelicHazards::SNARE_TIME;
        damagePlayer(RelicHazards::SNARE_DAMAGE * level.arenas[director.arena].damageScale * tune().damage, player.position, 0.15f, 0.04f, 0.f);
        audio.playAt("v_weaver_twang", player.position + glm::vec3{0.f, 1.2f, 0.f}, 128, SoundGroup::ENEMY, true, 0.5f);
        ui.feed("SNARED", {0.75f, 0.35f, 1.1f});
        for (auto& e : enemies)   // the whole wave feels it
            if (e.targetable() && glm::length(e.position - player.position) < 30.f) e.attackTimer = e.stats().attackEvery;
        relic.cutWire(wi);   // it snaps
    }
    if (snareTimer > 0.f) {   // snared: slowed to a crawl, no dash
        glm::vec2 h{player.velocity.x, player.velocity.z};
        float sp = glm::length(h), cap = 3.f;
        if (sp > cap) { player.velocity.x *= cap / sp; player.velocity.z *= cap / sp; }
    }
}

inline void GameplayState::onReliquaryEvents(Enemy& e, const EnemyEvents& ev) {
    if (ev.wire) {
        relic.addWire(e.uid, ev.wireA, ev.wireB);
        audio.playAt("v_weaver_twang", (ev.wireA + ev.wireB) * 0.5f, 90, SoundGroup::ENEMY, false, 0.3f);
    }
}

inline void GameplayState::releaseRevenantSoul(const Enemy& e, bool inVoid) {
    if (!revenantSoulEscapes(StyleSource::ENVIRONMENT, inVoid)) return;
    const Arena& ar = level.arenas[director.arena];
    const std::vector<glm::vec3>& spots = director.wave >= 0 && director.wave < (int)ar.waveGround.size() ? ar.waveGround[director.wave] : ar.groundSpawns;
    if (!relic.releaseSoul(e, soulDestination(spots, e.position, 15.f))) return;
    audio.playAt("v_revenant_soul", e.position + glm::vec3{0.f, e.height() * 0.6f, 0.f}, 128, SoundGroup::ENEMY, true, 0.4f);
    ui.feed("ITS SOUL RUNS", {0.75f, 0.9f, 1.3f});
}

// The parry key, aimed at a soul passing within reach: caught for good
inline bool GameplayState::catchSoulWithPunch() {
    glm::vec3 eye = player.camera.position, fwd = player.camera.forward();
    int i = relic.soulNear(eye + fwd * 1.5f, 2.5f);
    if (i < 0) return false;
    glm::vec3 at = relic.souls[i].pos;
    relic.removeSoul(i);
    parryFeedback(at, true);
    styleSystem.addStyle(80.f, StyleSource::PARRY);
    ui.toast("SOUL TAKEN", "BARE-HANDED", {0.75f, 0.9f, 1.3f}, 1.4f);
    return true;
}

inline void GameplayState::gatherReliquaryBoxes(std::vector<BoxInstance>& out) {
    using namespace rig;
    const float t = gameClock;
    const glm::vec3 violet{0.75f, 0.35f, 1.1f}, soul{0.75f, 0.9f, 1.3f};
    for (const auto& w : relic.wires) {
        glm::vec3 d = w.b - w.a; float len = glm::length(d);
        float yaw = std::atan2(d.x, d.z), hum = 0.7f + 0.3f * std::sin(t * 30.f + w.a.x);
        push(out, T((w.a + w.b) * 0.5f) * RY(yaw) * S({0.04f, 0.04f, len}), violet * 0.3f, violet * (1.4f * hum));
        for (glm::vec3 n : {w.a, w.b}) push(out, T(n) * S(glm::vec3{0.35f}), violet * 0.3f, violet * 2.f);
    }
    for (const auto& s : relic.souls) {
        float pulse = 0.8f + 0.2f * std::sin(t * 12.f);
        push(out, T(s.pos) * RY(t * 3.f) * S(glm::vec3{0.6f * pulse}), soul * 0.5f, soul * 2.5f);
    }
}
```

Wiring:
- `src/Gameplay_Tick.h`: after `updateWarden(dt);` → `updateReliquary(dt);`. In the dash input, `&& snareTimer <= 0.f` added to the dash condition (no dash while snared).
- `src/Gameplay_Render.h`: after `gatherWardenBoxes(out);` → `gatherReliquaryBoxes(out);`.
- `src/Gameplay_Combat.h`:
  - after the Warden hook: `if (enemies[i].type == EnemyType::WEAVER) onReliquaryEvents(enemies[i], ev);`
  - `onEnemyKilled`: `if (e.type == EnemyType::REVENANT) releaseRevenantSoul(e, e.position.y < level.arenas[director.arena].voidY);` and `if (e.type == EnemyType::WEAVER) relic.dropOwner(e.uid);`
  - the void-fall kill (`// Fell into the void`) passes through `onEnemyKilled` already, so its `position.y < voidY` makes the soul not escape.
  - `fireWeapon` hitscan: before resolving enemy hits, test souls and nodes along the ray up to the first wall/enemy hit: `float st; int si = relic.raySoul(origin, dir, firstT, st); if (si >= 0 && relic.hurtSoul(si, dmg)) { styleSystem.addStyle(40.f, StyleSource::PARRY); ui.feed("SOUL TAKEN", {0.75f, 0.9f, 1.3f}); }` and `int ni = relic.rayNode(origin, dir, firstT, st); if (ni >= 0) { relic.cutWire(ni); audio.playAt("v_weaver_twang", origin + dir * st, 100, SoundGroup::ENEMY); }` (`firstT` = the distance of the nearest wall/enemy hit the existing code computed).
  - Blasts: souls within the radius take the blast's damage; nodes within it cut their wire.
  - `punch()`: first line after the cooldown: `if (catchSoulWithPunch()) return;`.
- `src/Gameplay_Flow.h` (the reset beside `ward.clear()`): `relic.clear(); lastChunk = 0; snareTimer = 0.f; driftWasGliding = false;`.
- `src/EnemyModel.h`: `humanoidDims` case `REVENANT` (gaunt: thinner torso and limbs than a Husk — copy the HUSK row and multiply widths by 0.8); a build case for `REVENANT` (humanoid, its chest a white-blue soul box glowing brighter during SOULBOLT, claws on the forearms raised during RAKE), add `REVENANT` to the humanoid head-box case list; a build case for `WEAVER` (a low body box, six long legs in two rows swinging with `animPhase`, a violet spinner box on its back glowing `0.4 + 3·tp` during STRING).
- Makefile: `src/Gameplay_Reliquary.h` in HEADERS.

- [ ] **Step 4: Run** — `make 2>&1 | grep -E "error|warning" | grep -v duplicate; make test 2>&1 | tail -1` → clean, `ALL PASSED`.

- [ ] **Step 5: Look at it.** Screenshots (scratchpad): `./shooter --act2 --arena 4 --god --shot 300 r1.bmp` (the landing, wave 1), `--act2 --arena 4 --wave 2 --god --cam 0 -200 -930 -90 -35 --shot 600 r2.bmp` (the ring from above), `--wave 3 ... r3.bmp` (the stack), `--spawn 15 --spawn 16 --shot 900 r4.bmp` (a Revenant and a Weaver; wait for a wire). Read them; fix what reads badly; ledger it.

- [ ] **Step 6: Commit**
```bash
git add src/Gameplay_Reliquary.h src/GameplayState.h src/Gameplay_Combat.h src/Gameplay_Flow.h src/Gameplay_Tick.h src/Gameplay_Render.h src/EnemyModel.h src/RelicHazards.h Makefile tests/test_game.cpp
git commit -m "The Reliquary in the game: relics that drift between waves, a fall back to the last one you stood on, souls to catch, wires that snare and wake the wave"
```

---

### Task 6: Cheese, docs, bench, web

**Files:** Modify `tests/test_game.cpp`, `README.md`.

- [ ] **Step 1: Cheese test** — append (uses `tests/BossSim.h`'s idea, inline for regular enemies): a Revenant and a Weaver each against a player camping a relic's far corner and one keeping 30 m away; each takes damage or a wire within 10 s:
```cpp
    {
        LevelData N = buildAct2Level(); SpatialGrid g; g.build(N.walls);
        const Arena& R = N.arenas[3]; N.formation.reset(N); N.updateMovers(0.f);
        auto pressure = [&](EnemyType t, glm::vec3 feet) {
            Enemy e(t, N.formation.top(3)); e.spawnTimer = 0.f; e.state = EnemyState::ACTIVE;
            EnemyWorld w; w.playerFeet = feet; w.playerEye = feet + glm::vec3{0, 1.7f, 0};
            w.walls = N.walls.data(); w.wallCount = (int)N.walls.size(); w.grid = &g; w.bounds = R.bounds;
            ProjectileSystem ps; ps.floorY = -600.f; std::vector<Enemy> none;
            for (int f = 0; f < 60 * 10; ++f) {
                e.update(DT, w);
                if (e.ev.meleeHit || e.ev.wire) return true;
                for (int k = 0; k < e.ev.shots; ++k) ps.fire(e.ev.shotOrigin, e.ev.shotDir[k] * e.ev.shotSpeed, e.ev.shotDamage, false);
                if (ps.update(DT, N.walls.data(), (int)N.walls.size(), none, w.playerEye, &g).hitPlayer) return true;
            }
            return false;
        };
        glm::vec3 corner = N.formation.top(0) + glm::vec3{8.5f, 0, 8.5f}, far = N.formation.top(1) + glm::vec3{-6, 0, 0};
        CHECK(pressure(EnemyType::REVENANT, corner) && pressure(EnemyType::REVENANT, far) &&
              pressure(EnemyType::WEAVER, corner) && pressure(EnemyType::WEAVER, far),
              "neither camping a corner nor keeping away escapes a Revenant or a Weaver for 10 s");
    }
```
Run; if one fails, fix the enemy (e.g. the Revenant's bolt range, the Weaver's line-of-sight rule), not the test; ledger it.

- [ ] **Step 2: README** — Act II section: the Reliquary (drifting relics, arrangements, the path to the hole), the Revenant and the Weaver rows in the enemy table, the menu's 4/4.
- [ ] **Step 3: Bench** — `./shooter --act2 --arena 4 --wave 2 --bench 2000` vs the Sanctum (`--arena 5`) on the same build; within ~15 %. Ledger both.
- [ ] **Step 4: Web** — `source ~/emsdk/emsdk_env.sh && make web` builds.
- [ ] **Step 5: Full suite and commit**
```bash
make && make test
git add tests/test_game.cpp README.md
git commit -m "Reliquary: no cheese against its two, README"
```
