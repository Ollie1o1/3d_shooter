# The Descent + The Penitent Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Act II's third arena, the Descent (a cage that rides down a vast shaft, a different floor per wave), ending in the boss THE PENITENT (chained colossus → unchained → scourge).

**Architecture:** The cage is four overlapping movers on a new `Mover::Path::DRIVEN`, positioned by `LevelData::Lift` (stops, eased 8 s rides); rides start between waves once the player boards, and `WaveDirector::hold` keeps the next wave waiting. The Penitent is a new boss `EnemyType::PENITENT` whose AI lives in `src/EnemyPenitent.h` (included by `Enemy.h`) and only *emits events*; `src/PenitentHazards.h` (no GL/audio) resolves sweeps, shockwave rings, chain lashes, incense pools and ember rings against the player; `src/Gameplay_Penitent.h` wires events, chain anchors (shoot or grapple-rip), drawing and sound into `GameplayState`.

**Tech Stack:** C++17, SDL2/OpenGL 3.3 (WebGL2 via Emscripten), glm, `tests/test_game.cpp` CHECK harness, Makefile.

**Spec:** `docs/superpowers/specs/2026-10-06-act2-descent-penitent-design.md`

## Global Constraints

- Stops: Y −80 (top), −120, −160, −200, −240 (pit). Shaft centre (0, ·, −840), wall R 34; cage a 12-sided platform ~13 m across the radius (4 overlapping boxes: x±12 z±5, x±5 z±12, x±10.5 z±8, x±8 z±10.5; axis reach 12); galleries R 20–34; bridges R 12.3–20.5, 3 m wide, N/E/S/W; the pit floor is fitted round the cage's outline so it docks flush (the player can't step up ledges, so no gaps or lips).
- Ride 8 s, eased. The cage moves only between waves, and only once the player stands on it.
- Waves: stop 1 kill all; stop 2 CONDUITS "RELEASE THE CLAMPS" (4 clamps); stop 3 kill all; pit: the Penitent. `maxAlive` 12, `damageScale` 1.4, `ReverbSpace::SHAFT`.
- Falls: below the cage's top − 25 → back on the cage, 15 damage. Enemies below `voidY` die.
- Penitent: health 6000; 6 anchors × 400 HP, rip = grapple attached 0.5 s; phase 1 takes 25 %; sweeps reach 16 m (18 m risen), ±100°; low sweep hits feet below floor + 1.0; high sweep hits a player whose top is above floor + 1.3 and feet below floor + 3.5; slam ring R 14; stomp R 6; lash after 3 s beyond 22 m or 4 m above the floor, warned 0.7 s, reach 40 m; incense every 12 s, 3 pools R 3.5, 8 s, 20/s; summon 4 Hollowed Husks every 25 s (risen); phase 3 under 25 %: wound ×3, 30 % faster, ember rings; parry in the last 0.25 s → 2.5 s stagger, ×2 damage. Damage: sweeps/slam/stomp 30, lash 35, embers 15, incense 20/s; × arena `damageScale` × difficulty.
- Every Penitent attack: a light tell and a positional sound tell (ENEMY group, priority); big tells `audio.duck(6.f, 0.5f)`.
- Maps: grand and organic (curves, arches, domes, one set piece per stop), never plain repeated boxes; collision may stay boxy and hidden (`B.solid`).
- GLSL untouched; `make web` must build. New headers in Makefile `HEADERS` (and `test:` deps when tests include them).
- Commit messages: no Co-Authored-By / Claude trailer.
- `$SCRATCH` = the session scratchpad directory.

## Review Focus

1. **The player isn't on the cage when a ride should start** (on a gallery, mid-air, fell into the gap) → the ride waits ("BOARD THE CAGE"), the next wave waits with it, a fall respawns them on the cage. Test in Task 3.
2. **A retry or `--wave N` mid-Descent** → the cage resets to the top and rides to the right stop; nothing spawns before it arrives. Test in Task 3 (director hold + `rideTarget`).
3. **Enemies standing on the cage** (spawns, Husks summoned in the pit, knock-back) → they stand on it, not fall through (`groundHeightAt` must include movers). Test in Task 3 via `LevelData::groundAt`.
4. **The Penitent at a wall or the pit's edge while risen** → it can't push through the shaft wall; sweeps still resolve from where it is. Covered by `setMove` + bounds; check in Task 7 screenshot run.
5. **Anchors broken by an explosion or many pellets in one tick** → each anchor breaks once, one stagger, one sound. Test in Task 5 (`LevelData::damageAnchor` returns true only on the breaking hit).

---

## File map

- `src/Level.h` — `Mover::Path::DRIVEN` + `drive`; `LevelData::Lift`; `LevelData::ChainAnchor` + `anchors`, `damageAnchor`, `anchorAlong`, `anchorsAlive`; `LevelData::groundAt`; `Arena::waveAir`.
- `src/WaveDirector.h` — `hold`; `waveAir` in `pickSpawn`; PENITENT spawns at `bossSpawn`.
- `src/ArenaShifts.h` — `ArenaShift::DESCENT`, lift reset, `onWaveCleared` asks for the next stop.
- `src/LevelAct2.h` — `buildDescent` (corridor, shaft, cage, stops, pit, anchors, waves); the Orrery's north door; basins; finish.
- `src/Enemy.h` — `EnemyType::PENITENT`, stats, attack kinds, events, members, parry/armor/height rules; includes `src/EnemyPenitent.h` (new: `thinkPenitent`).
- `src/PenitentHazards.h` (new) — sweeps, rings, lashes, pools.
- `src/EnemyModel.h` — Penitent rig, `penitentPose`, head box, `woundBox`.
- `src/Gameplay_Penitent.h` (new) — event wiring, anchors, lift control, drawing, sounds.
- `src/GameplayState.h`, `Gameplay_Tick.h`, `Gameplay_Flow.h`, `Gameplay_Combat.h`, `Gameplay_Render.h`, `Gameplay_HUD.h` — hooks.
- `src/MusicSynth.h` — "DESCENT", `MUSIC_TRACKS` 8. `src/MenuState.h` — label. `README.md`.
- `tests/test_game.cpp`, `Makefile`.

---

### Task 1: Driven movers, the Lift, the director's hold

**Files:**
- Modify: `src/Level.h`, `src/WaveDirector.h`, `tests/test_game.cpp`

**Interfaces:**
- Produces:
  - `Mover::Path::DRIVEN`; `float Mover::drive` (0..1 along a→b; `offsetAt` ignores time for DRIVEN)
  - `struct LevelData::Lift { std::vector<int> movers; std::vector<float> stops; int at = 0, to = 0, pending = -1; float t = 0.f; static constexpr float RIDE_TIME = 8.f; bool riding() const; bool busy() const; float y() const; void request(int stop); void start(); void reset(); void update(float dt, LevelData& L); }` and `LevelData::lift`
  - `bool WaveDirector::hold` — while true, INTRO and BREAK don't count down

- [ ] **Step 1: Write the failing tests**

Add before the mouse-filter block in `tests/test_game.cpp`:

```cpp
    // ---------------------------------------------------------------- the lift (driven movers) and the director's hold
    {
        LevelData L; LevelBuilder B{L};
        int m0 = B.mover({0.f, -80.5f, 0.f}, {10.f, 0.5f, 10.f}, Mover::Path::DRIVEN, {0.f, 0.f, 0.f}, {0.f, -160.f, 0.f}, 1.f, 0.f, {1, 1, 1});
        int m1 = B.mover({12.f, -80.5f, 0.f}, {2.f, 0.5f, 5.f}, Mover::Path::DRIVEN, {0.f, 0.f, 0.f}, {0.f, -160.f, 0.f}, 1.f, 0.f, {1, 1, 1});
        L.lift.movers = {m0, m1};
        L.lift.stops = {-80.f, -120.f, -160.f, -200.f, -240.f};
        L.lift.reset(); L.lift.update(0.f, L); L.updateMovers(0.f);
        bool atTop = std::fabs(L.walls[L.movers[m0].wall].box.max.y - (-80.f)) < 1e-3f;
        L.lift.request(1);
        bool waits = L.lift.busy() && !L.lift.riding();
        L.lift.start();
        float clock = 0.f, maxStep = 0.f, prevY = L.lift.y();
        bool together = true;
        for (int i = 0; i < 60 * 9; ++i) {
            clock += DT; L.lift.update(DT, L); L.updateMovers(clock);
            maxStep = std::max(maxStep, std::fabs(L.lift.y() - prevY)); prevY = L.lift.y();
            together &= std::fabs(L.walls[L.movers[m0].wall].box.max.y - L.walls[L.movers[m1].wall].box.max.y) < 1e-4f;
        }
        bool arrived = !L.lift.busy() && L.lift.at == 1 && std::fabs(L.walls[L.movers[m0].wall].box.max.y - (-120.f)) < 1e-3f;
        CHECK(atTop && waits, "the cage starts at the top; a requested ride waits until it's started");
        CHECK(arrived && together, "a ride takes the whole cage down to the next stop together, within 9 s");
        CHECK(maxStep < 0.25f, "the ride eases (never more than 15 m/s)");
        L.lift.reset(); L.lift.update(0.f, L); L.updateMovers(clock);
        CHECK(L.lift.at == 0 && std::fabs(L.walls[L.movers[m0].wall].box.max.y - (-80.f)) < 1e-3f, "a reset puts the cage back at the top");

        LevelData A = buildLevel();
        WaveDirector d; d.level = &A; d.startArena(0);
        d.hold = true;
        std::vector<SpawnRequest> out;
        for (int i = 0; i < 60 * 10; ++i) d.update(DT, 0, A.arenas[0].playerStart, out);
        bool held = d.phase == WaveDirector::Phase::INTRO && out.empty();
        d.hold = false;
        for (int i = 0; i < 60 * 4; ++i) d.update(DT, 0, A.arenas[0].playerStart, out);
        CHECK(held && d.phase == WaveDirector::Phase::ACTIVE, "the director holds the wave while told to, then starts it");
    }
```

- [ ] **Step 2: Run to verify it fails**

Run: `make test 2>&1 | grep -E "error" | head -3`
Expected: compile errors: no `DRIVEN`, no `lift`, no `hold`.

- [ ] **Step 3: Implement**

In `src/Level.h`, `struct Mover`: change the enum and add the drive:

```cpp
    enum class Path { PINGPONG, ORBIT, DRIVEN };   // DRIVEN: placed by the game (drive), not by the clock
```
```cpp
    float     drive  = 0.f;      // DRIVEN: 0..1 along a -> b
```
and at the top of `offsetAt`:
```cpp
        if (path == Path::DRIVEN) return glm::mix(a, b, drive);
```

In `struct LevelData`, after `moverWalls`:

```cpp
    // The Descent's cage: movers driven together between stops (Y of the
    // cage's top at each). A ride is requested between waves, starts when the
    // player is aboard (GameplayState calls start()), and eases over RIDE_TIME.
    struct Lift {
        std::vector<int>   movers;
        std::vector<float> stops;
        int   at = 0, to = 0, pending = -1;
        float t = 0.f;
        static constexpr float RIDE_TIME = 8.f;
        bool  riding() const { return to != at; }
        bool  busy() const { return riding() || pending >= 0; }
        float y() const {
            if (stops.empty()) return 0.f;
            if (!riding()) return stops[at];
            float u = glm::clamp(t / RIDE_TIME, 0.f, 1.f);
            u = u * u * (3.f - 2.f * u);
            return glm::mix(stops[at], stops[to], u);
        }
        void request(int stop) { if (!stops.empty()) pending = std::clamp(stop, 0, (int)stops.size() - 1); }
        void start() { if (pending >= 0 && !riding()) { to = pending; pending = -1; t = 0.f; if (to == at) to = at; } }
        void reset() { at = to = 0; pending = -1; t = 0.f; }
        void update(float dt, LevelData& L) {
            if (movers.empty() || stops.empty()) return;
            if (riding()) {
                t += dt;
                if (t >= RIDE_TIME) { at = to; t = 0.f; }
            }
            float span = stops.back() - stops.front();
            float drive = std::fabs(span) > 1e-4f ? (y() - stops.front()) / span : 0.f;
            for (int m : movers) L.movers[m].drive = drive;
        }
    };
    Lift lift;
```

(`Lift::update` takes `LevelData&` defined later in the same struct: declare `struct Lift` as above with `update` defined after `LevelData` if the compiler needs a complete type — simplest: keep the body but move `Lift lift;` and the `Lift` struct inside `LevelData`, which is complete inside its own member function bodies.)

In `src/WaveDirector.h`, next to `fast`:

```cpp
    bool  hold = false;   // set by the caller: keep the next wave waiting (the Descent's cage is riding)
```

and in `update()`:

```cpp
        case Phase::INTRO:
            if (!hold) timer -= dt;
            if (timer <= 0.f) beginWave();
            break;
```
```cpp
        case Phase::BREAK:
            if (!hold) timer -= dt;
            if (timer <= 0.f) { ++wave; beginWave(); }
            break;
```

- [ ] **Step 4: Run to verify it passes**

Run: `make test 2>&1 | grep -E "FAIL|ALL PASSED|FAILED"`
Expected: `ALL PASSED`.

- [ ] **Step 5: Commit**

```bash
git add src/Level.h src/WaveDirector.h tests/test_game.cpp
git commit -m "Driven movers and a lift; the director can hold a wave"
```

---

### Task 2: The Descent's level

**Files:**
- Modify: `src/Level.h` (`Arena::waveAir`, `LevelData::ChainAnchor`/`anchors`/helpers, `groundAt`), `src/WaveDirector.h` (`waveAir`, PENITENT spawn — the enum value comes in Task 4, so use `isBoss(t) && t != EnemyType::WARDEN` here), `src/LevelAct2.h`, `tests/test_game.cpp`

**Interfaces:**
- Consumes: Task 1 `Lift`, `Mover::Path::DRIVEN`.
- Produces:
  - `std::vector<std::vector<glm::vec3>> Arena::waveAir` (per-wave air spawns; empty → `airSpawns`)
  - `struct LevelData::ChainAnchor { int wall; glm::vec3 pos; float hp = 400.f; bool alive = true; }`, `std::vector<ChainAnchor> anchors`
  - `int LevelData::anchorsAlive() const`
  - `bool LevelData::damageAnchor(int i, float dmg)` → true only on the hit that breaks it (parks its wall 500 m below)
  - `int LevelData::anchorAlong(glm::vec3 o, glm::vec3 d, float maxT) const` → index of the live anchor the ray reaches first within maxT, or −1
  - `float LevelData::groundAt(float x, float z, float fromY) const` → highest wall top (static or mover) at (x, z) at or below fromY + 0.5, else `baseFloor`
  - `buildAct2` builds three arenas; arena 2 is "THE DESCENT" with `shift = ArenaShift::DESCENT` (enum added in Task 3; until then build with `ArenaShift::NONE` and switch in Task 3)
  - `LevelData::lift` filled: 4 movers, stops {−80, −120, −160, −200, −240}

- [ ] **Step 1: Write the failing tests**

Update the existing Act II checks:
- `"act II: the Drowned Nave, then down a corridor to the Orrery"` becomes:

```cpp
        CHECK(N.arenas.size() == 3 && std::string(N.arenas[0].name) == "THE DROWNED NAVE" && std::string(N.arenas[1].name) == "THE ORRERY" &&
              std::string(N.arenas[2].name) == "THE DESCENT" && N.corridors.size() == 2,
              "act II: the Drowned Nave, the Orrery, then the Descent, joined by corridors");
```

Add a new block after the Orrery's block:

```cpp
    // ---------------------------------------------------------------- ACT II: the Descent
    {
        LevelData N = buildAct2Level();
        const Arena& D = N.arenas[2];
        const glm::vec3 C{0.f, 0.f, -840.f};
        CHECK(D.space == ReverbSpace::SHAFT && D.waves.size() == 4 && D.maxAlive == 12 && std::fabs(D.damageScale - 1.4f) < 1e-4f,
              "the Descent: a shaft's reverb, three waves and the boss");
        CHECK(N.lift.movers.size() == 4 && N.lift.stops.size() == 5 && N.lift.stops[0] == -80.f && N.lift.stops[4] == -240.f,
              "the cage: four boxes (a 12-sided platform), five stops from -80 to -240");
        CHECK(N.anchors.size() == 6 && N.anchorsAlive() == 6, "six chain anchors in the pit");
        bool anchorsHigh = true;
        for (auto& a : N.anchors) anchorsHigh &= a.pos.y >= -232.f && a.pos.y <= -228.f + 0.01f &&
                                                glm::length(glm::vec2{a.pos.x - C.x, a.pos.z - C.z}) > 30.f;
        CHECK(anchorsHigh, "the anchors hang 8-12 m up the pit wall");
        // Spawns: each wave's ground points clear of walls and inside; air points clear
        bool ok = true;
        for (int w = 0; w < 3; ++w) {
            for (auto& sp : D.waveGround[w]) {
                if (overlapsWall(N, boxAt(sp, statsOf(EnemyType::JUGGERNAUT).radius, statsOf(EnemyType::JUGGERNAUT).height))) {
                    std::printf("      descent wave %d ground spawn (%.1f %.1f %.1f) in a wall\n", w, sp.x, sp.y, sp.z); ok = false; }
                if (!inside(D.bounds, sp)) { std::printf("      descent spawn outside bounds\n"); ok = false; }
            }
            for (auto& sp : D.waveAir[w])
                if (overlapsWall(N, boxAt(sp, statsOf(EnemyType::RAPTOR).radius, statsOf(EnemyType::RAPTOR).height))) {
                    std::printf("      descent wave %d air spawn in a wall\n", w); ok = false; }
        }
        for (auto& cp : D.goals[1].points)
            if (overlapsWall(N, boxAt(cp, statsOf(EnemyType::CONDUIT).radius, statsOf(EnemyType::CONDUIT).height))) ok = false;
        CHECK(ok, "the Descent's spawns and clamps stand clear of walls, inside the shaft");
        CHECK(D.goals[1].kind == WaveGoal::CONDUITS && D.goals[1].points.size() == 4, "stop 2: release the four clamps");
        // Every stop's ground spawns are at that stop's height (gallery or cage)
        bool levels = true;
        for (int w = 0; w < 3; ++w) for (auto& sp : D.waveGround[w]) levels &= std::fabs(sp.y - N.lift.stops[w + 1]) < 0.05f;
        CHECK(levels, "each wave spawns on its own stop's floor");
        CHECK(std::fabs(N.finishPos.y - (-240.f)) < 1e-3f && glm::length(glm::vec2{N.finishPos.x, N.finishPos.z + 840.f}) < 34.f,
              "the finish beacon stands in the Penitent's pit");
        CHECK(N.arenas[1].exitDoor >= 0, "the Orrery has a way on (its north arch)");
        // groundAt sees the cage
        N.lift.reset(); N.lift.update(0.f, N); N.updateMovers(0.f);
        CHECK(std::fabs(N.groundAt(0.f, -840.f, -79.f) - (-80.f)) < 1e-3f, "something standing on the cage stands on it (movers count as ground)");
        // Anchors: damage breaks once
        LevelData M = buildAct2Level();
        bool first = M.damageAnchor(0, 300.f), second = M.damageAnchor(0, 300.f), third = M.damageAnchor(0, 300.f);
        CHECK(!first && second && !third && M.anchorsAlive() == 5, "an anchor breaks once, on the hit that takes it past 400");
        glm::vec3 a1 = M.anchors[1].pos;
        glm::vec3 from = glm::vec3{0.f, -236.f, -840.f};
        int hit = M.anchorAlong(from, glm::normalize(a1 - from), 60.f);
        CHECK(hit == 1, "a shot at an anchor finds it");
    }
```

Update the simulated ACT II run (`"a simulated ACT II run clears the Nave, goes down to the Orrery and clears it"`): change its expectation to reach the Descent and kill the boss; replace the loop body's start with lift handling and the final CHECK:

```cpp
        for (int tick = 0; tick < 60 * 60 * 40 && d.phase != WaveDirector::Phase::VICTORY; ++tick) {
            clock += DT; N.lift.update(DT, N); N.updateMovers(clock);
            if (d.phase == WaveDirector::Phase::CLEARED) player = N.arenas[d.arena + 1].playerStart;   // down the corridor
            else if (d.goal().kind == WaveGoal::HOLD) player = d.goalPos() + glm::vec3{0, 0.05f, 0};   // onto the ring
            if (d.arena == 2) {   // the Descent: ride to each wave's stop first
                int want = std::min(d.wave + (d.phase == WaveDirector::Phase::BREAK ? 2 : 1), 4);
                if (N.lift.at != want && !N.lift.busy()) { N.lift.request(want); N.lift.start(); }
                d.hold = N.lift.busy();
                player = glm::vec3{0.f, N.lift.y(), -840.f};
            }
            std::vector<SpawnRequest> out;
            d.update(DT, (int)alive.size(), player, out);
```

and

```cpp
        CHECK(d.phase == WaveDirector::Phase::VICTORY && d.arena == 2,
              "a simulated ACT II run clears the Nave and the Orrery, rides the Descent and kills the Penitent");
```

- [ ] **Step 2: Run to verify it fails**

Run: `make test 2>&1 | grep -E "error|FAIL" | head -8`
Expected: compile errors (no `waveAir`, `anchors`, `groundAt`), then FAILs on the Descent checks.

- [ ] **Step 3: Implement the level-data helpers**

`src/Level.h`, in `struct Arena` after `waveGround`:

```cpp
    // Optional per-wave air spawns (the Descent: each wave a stop lower).
    // Empty for a wave → airSpawns.
    std::vector<std::vector<glm::vec3>> waveAir;
```

In `struct LevelData` (after `Lift lift;`):

```cpp
    // The Penitent's chains are fixed to these, high on the pit wall: shoot
    // one out (hp) or grapple onto it and hang on (GameplayState rips it)
    struct ChainAnchor { int wall = -1; glm::vec3 pos{0.f}; float hp = 400.f; bool alive = true; };
    std::vector<ChainAnchor> anchors;
    int anchorsAlive() const { int n = 0; for (auto& a : anchors) n += a.alive; return n; }
    // True only on the hit that breaks it. A broken anchor's wall is parked
    // far below, inside the grid cells it was filed under (like an open door)
    bool damageAnchor(int i, float dmg) {
        if (i < 0 || i >= (int)anchors.size() || !anchors[i].alive) return false;
        anchors[i].hp -= dmg;
        if (anchors[i].hp > 0.f) return false;
        anchors[i].alive = false;
        AABB& b = walls[anchors[i].wall].box;
        b.min.y -= 500.f; b.max.y = b.min.y + 0.01f;
        return true;
    }
    int anchorAlong(glm::vec3 o, glm::vec3 d, float maxT) const {
        int best = -1; float bt = maxT + 0.05f;
        for (int i = 0; i < (int)anchors.size(); ++i) {
            if (!anchors[i].alive) continue;
            const AABB& b = walls[anchors[i].wall].box;
            glm::vec3 inv{1.f / (d.x + 1e-9f), 1.f / (d.y + 1e-9f), 1.f / (d.z + 1e-9f)};
            glm::vec3 t0 = (b.min - o) * inv, t1 = (b.max - o) * inv;
            glm::vec3 tn = glm::min(t0, t1), tx = glm::max(t0, t1);
            float te = std::max({tn.x, tn.y, tn.z}), tl = std::min({tx.x, tx.y, tx.z});
            if (tl >= te && te > 0.f && te < bt) { bt = te; best = i; }
        }
        return best;
    }
    // The highest top (static wall or moving platform) under (x, z) at or
    // below fromY + 0.5, else the hard floor
    float groundAt(float x, float z, float fromY) const {
        float best = baseFloor(x, z);
        for (const auto& w : walls) {
            const AABB& b = w.box;
            if (x >= b.min.x && x <= b.max.x && z >= b.min.z && z <= b.max.z && b.max.y <= fromY + 0.5f)
                best = std::max(best, b.max.y);
        }
        return best;
    }
```

(`groundAt` scans every wall: it's for tests and the lift's checks. The game keeps its grid-based `groundHeightAt`, extended in Task 3 to include movers.)

`src/WaveDirector.h`, `pickSpawn`: replace the boss lines and the `pts` choice:

```cpp
        if (t == EnemyType::SOVEREIGN || (isBoss(t) && t != EnemyType::WARDEN)) return a.bossSpawn;   // waiting where it lives
```
```cpp
        const auto& pts = flying ? ((wave < (int)a.waveAir.size() && !a.waveAir[wave].empty()) ? a.waveAir[wave] : a.airSpawns)
                        : (wave < (int)a.waveGround.size() && !a.waveGround[wave].empty()) ? a.waveGround[wave]
                        : a.groundSpawns;
```

- [ ] **Step 4: Build the Descent (`src/LevelAct2.h`)**

1. Basins: change the Orrery's to `aabb(-300, 0, -668, 300, 0, -788)` and add `L.basins.push_back({aabb(-300, 0, -788, 300, 0, -1000), -300.f});   // the Descent: open below the cage`.
2. The Orrery's north arch: in the outer-wall post loop skip the north gate too:

```cpp
        if (std::fabs(p.x) < 5.5f && p.z > C.z) continue;          // the south gate
        if (std::fabs(p.x) < 5.5f && p.z < C.z) continue;          // the north arch, the way down
```

and after the south gate's posts:

```cpp
    wall(-6, O, -790, -4, O + 14, -782, stoneDark); wall(4, O, -790, 6, O + 14, -782, stoneDark);   // north arch posts
    wall(-4, O + 7, -789, 4, O + 14, -783, stoneDark);
    o.exitDoor = B.doorway(true, -4, 4, -787, -785, O, 7.f, gold, true);
```

   Move `L.finishPos` out of the Orrery (delete `L.finishPos = {0.f, O, -778.f};`).
3. Before `L.arenas.push_back(std::move(o));` nothing else changes in the Orrery. After it, call `buildDescent(B);` — a new function defined above `buildAct2` (declared `inline void buildDescent(LevelBuilder& B);` before `buildAct2`, defined after it). Update the file's header comment with the Descent.
4. `buildDescent`:

```cpp
// =============================================================================
// THE DESCENT — a cage that rides down a vast round shaft. It hangs at a
// different floor for each wave (bell galleries, the clamps, the furnace
// ring) and comes to rest in the Penitent's pit, 160 m below the Orrery.
//
//   Y  -80  the landing (the corridor from the Orrery's north arch)
//   Y -120  the bell galleries     arcades and great bells round the shaft
//   Y -160  the clamps             four brake clamps bite the cage's rim
//   Y -200  the furnace ring       catwalks round furnace mouths, red below
//   Y -240  the pit                the Penitent, chained to six anchors
// =============================================================================
inline void buildDescent(LevelBuilder& B) {
    using glm::vec3;
    LevelData& L = B.L;
    auto aabb = &LevelBuilder::aabb;
    auto wall = [&](float x0, float y0, float z0, float x1, float y1, float z1, vec3 c) { return B.wall(x0,y0,z0,x1,y1,z1,c); };
    auto neon = [&](float x0, float y0, float z0, float x1, float y1, float z1, vec3 c) { B.neon(x0,y0,z0,x1,y1,z1,c); };

    const vec3 C{0.f, 0.f, -840.f};
    const float TOP = -80.f, PIT = -240.f, RW = 34.f, RCAGE = 12.f, RGAL = 20.f;
    // The cage: four overlapping boxes, a 12-sided platform ~13 m across the radius
    struct Rect { float hx, hz; };
    const Rect CAGE[4] = {{12.f, 5.f}, {5.f, 12.f}, {10.5f, 8.f}, {8.f, 10.5f}};
    const float STOP[5] = {-80.f, -120.f, -160.f, -200.f, -240.f};
    vec3 iron{0.16f,0.15f,0.16f}, ironDark{0.09f,0.085f,0.09f}, bone{0.7f,0.66f,0.58f}, brass{0.62f,0.48f,0.26f},
         ember{1.4f,0.45f,0.12f}, amber{1.3f,0.8f,0.35f}, blood{1.2f,0.12f,0.08f}, pale{0.8f,0.85f,0.9f};
    B.mat = Mat::METAL;

    // ---- the corridor from the Orrery's north arch to the landing ----
    wall(-4, TOP - 1, -808, 4, TOP, -786, iron);
    wall(-5, TOP, -808, -4, TOP + 8, -786, ironDark); wall(4, TOP, -808, 5, TOP + 8, -786, ironDark);
    wall(-5, TOP + 8, -808, 5, TOP + 9, -786, ironDark);
    for (float z = -806.f; z < -787.f; z += 4.f) neon(-3.95f, TOP + 7.6f, z, 3.95f, TOP + 7.75f, z + 0.3f, amber * 0.5f);
    L.corridors.push_back(aabb(-6, TOP, -810, 6, TOP + 14, -784));

    Arena a;
    a.name = "THE DESCENT"; a.space = ReverbSpace::SHAFT;
    a.subtitle = "RIDE THE CAGE DOWN - SURVIVE 3 WAVES";
    a.bounds = aabb(C.x - 33, PIT, C.z - 33, C.x + 33, TOP + 14, C.z + 33);
    a.zone   = aabb(C.x - 34.5f, PIT, C.z - 34.5f, C.x + 34.5f, TOP + 16, C.z + 34.5f);
    a.playerStart = {0.f, TOP, -812.f};
    a.startYaw = -90.f;
    a.respawn = {C.x, TOP, C.z}; a.hasRespawn = true;
    a.voidY = TOP - 25.f;
    a.bossSpawn = {C.x, PIT, C.z - 24.f};

    // ---- the shaft wall: posts round R 35 (collision), a curved wall drawn
    // inside them, ribs of light every 10 m, a gap for the landing's door ----
    for (int k = 0; k < 72; ++k) {
        float ang = k * 6.2831853f / 72.f;
        vec3 p = C + vec3{std::cos(ang) * (RW + 1.2f), 0.f, std::sin(ang) * (RW + 1.2f)};
        bool door = std::fabs(p.x) < 5.5f && p.z > C.z;
        B.solid(p.x - 1.6f, PIT - 6.f, p.z - 1.6f, p.x + 1.6f, door ? TOP - 1.f : TOP + 16.f, p.z + 1.6f);
        if (door) B.solid(p.x - 1.6f, TOP + 8.f, p.z - 1.6f, p.x + 1.6f, TOP + 16.f, p.z + 1.6f);
    }
    B.kit().curve({C.x, 0.f, C.z}, RW + 0.6f, 0.f, 6.2831853f, PIT - 6.f, TOP + 16.f, 1.2f, ironDark, 72);
    for (float y = PIT + 5.f; y < TOP + 14.f; y += 10.f)
        B.kit(true).curve({C.x, 0.f, C.z}, RW - 0.05f, 0.f, 6.2831853f, y, y + 0.18f, 0.1f, amber * 0.35f, 72);
    for (int k = 0; k < 16; ++k) {                                      // ribs: arches climbing the wall
        float yaw = k * 6.2831853f / 16.f;
        vec3 p = C + vec3{std::cos(yaw) * (RW - 0.5f), 0.f, std::sin(yaw) * (RW - 0.5f)};
        B.kit().rod({p.x, PIT, p.z}, {p.x, TOP + 16.f, p.z}, 0.5f, iron, 6);
    }
    a.entryGate = B.doorway(true, -4, 4, -809, -807, TOP, 7.f, amber, false);

    // ---- an annulus floor (galleries, the pit) built as strips, like the
    // Orrery's terrace: drawn exactly where it holds you ----
    auto annulus = [&](float y, float rIn, float rOut, vec3 col, vec3 lip) {
        const float STRIP = 0.75f;
        auto band = [&](bool alongX, float side, float u0, float u1) {
            float un = (u0 < 0.f && u1 > 0.f) ? 0.f : std::min(std::fabs(u0), std::fabs(u1));
            float edge = std::sqrt(std::max(0.f, rIn * rIn - un * un));
            float from = std::max(edge, un), to = std::sqrt(std::max(0.f, rOut * rOut - un * un));
            if (from >= to) return;
            float a0 = side * from, a1 = side * to;
            if (alongX) wall(C.x + std::min(a0, a1), y - 1, C.z + u0, C.x + std::max(a0, a1), y, C.z + u1, col);
            else        wall(C.x + u0, y - 1, C.z + std::min(a0, a1), C.x + u1, y, C.z + std::max(a0, a1), col);
            if (edge > un + 1e-3f) {
                float e = side * edge;
                if (alongX) neon(C.x + e - 0.06f, y - 0.3f, C.z + u0, C.x + e + 0.06f, y + 0.03f, C.z + u1, lip);
                else        neon(C.x + u0, y - 0.3f, C.z + e - 0.06f, C.x + u1, y + 0.03f, C.z + e + 0.06f, lip);
            }
        };
        for (float u = -rOut; u < rOut - 1e-3f; u += STRIP) {
            float u1 = std::min(u + STRIP, rOut);
            for (float side : {-1.f, 1.f}) { band(true, side, u, u1); band(false, side, u, u1); }
        }
    };
    auto bridges = [&](float y, vec3 col, vec3 lip) {
        for (int k = 0; k < 4; ++k) {
            float cx = k == 0 ? 1.f : k == 1 ? -1.f : 0.f, cz = k == 2 ? 1.f : k == 3 ? -1.f : 0.f;
            float r0 = RCAGE + 0.3f, r1 = RGAL + 0.5f;   // RCAGE: the cage's reach along an axis
            if (cx != 0.f) wall(C.x + std::min(cx * r0, cx * r1), y - 0.8f, C.z - 1.5f, C.x + std::max(cx * r0, cx * r1), y, C.z + 1.5f, col);
            else           wall(C.x - 1.5f, y - 0.8f, C.z + std::min(cz * r0, cz * r1), C.x + 1.5f, y, C.z + std::max(cz * r0, cz * r1), col);
            vec3 tip = C + vec3{cx * r0, y, cz * r0};
            neon(tip.x - 1.5f, y - 0.05f, tip.z - 1.5f, tip.x + 1.5f, y + 0.04f, tip.z + 1.5f, lip);
        }
    };

    // ---- the landing: a south apron and a bridge onto the cage (no ring:
    // the cage is the only floor up here, so you're aboard when it drops) ----
    wall(-5, TOP - 1, -810, 5, TOP, C.z + RW - 0.5f, iron);
    wall(-1.5f, TOP - 0.8f, C.z + RCAGE + 0.3f, 1.5f, TOP, C.z + RW - 0.5f, iron);

    // ---- stop 1: the bell galleries ----
    annulus(STOP[1], RGAL, RW, iron, amber * 0.6f);
    bridges(STOP[1], iron, amber * 0.5f);
    for (int k = 0; k < 8; ++k) {
        float yaw = (k + 0.5f) * 0.7853982f;
        vec3 p = C + vec3{std::cos(yaw) * (RW - 1.5f), STOP[1], std::sin(yaw) * (RW - 1.5f)};
        B.kit().arch({p.x, STOP[1], p.z}, 9.f, 7.f, 0.8f, 1.2f, bone * 0.8f, -yaw + 1.5707963f, 12);   // arcades
        vec3 b = C + vec3{std::cos(yaw) * 27.f, 0.f, std::sin(yaw) * 27.f};
        B.kit().dome({b.x, STOP[1] + 5.2f, b.z}, 1.8f, brass, 4, 12);                                 // a bell
        B.kit().column({b.x, STOP[1] + 6.8f, b.z}, 0.12f, 3.f, ironDark, 6);
        B.kit(true).column({b.x, STOP[1] + 4.0f, b.z}, 0.35f, 0.5f, amber, 8);                       // its clapper glows
    }
    // ---- stop 2: the clamps ----
    annulus(STOP[2], RGAL, RW, ironDark, blood * 0.5f);
    bridges(STOP[2], ironDark, blood * 0.4f);
    std::vector<vec3> clamps;
    for (int k = 0; k < 4; ++k) {
        float yaw = k * 1.5707963f + 0.7853982f;                        // between the bridges
        vec3 p = C + vec3{std::cos(yaw) * 17.5f, STOP[2], std::sin(yaw) * 17.5f};
        wall(p.x - 2.f, STOP[2] - 1.f, p.z - 2.f, p.x + 2.f, STOP[2], p.z + 2.f, ironDark);   // the clamp's footing
        B.kit().box({p.x, STOP[2] + 4.5f, p.z}, {1.2f, 9.f, 3.2f}, brass, -yaw, 0.3f, 0.f);    // jaws over the rim
        clamps.push_back(p);
    }
    // ---- stop 3: the furnace ring ----
    annulus(STOP[3], RGAL, RW, iron, ember * 0.6f);
    bridges(STOP[3], iron, ember * 0.5f);
    for (int k = 0; k < 6; ++k) {
        float yaw = k * 1.0471976f;
        vec3 p = C + vec3{std::cos(yaw) * (RW - 0.3f), STOP[3], std::sin(yaw) * (RW - 0.3f)};
        B.kit().arch({p.x, STOP[3], p.z}, 6.f, 5.f, 0.9f, 1.4f, ironDark, -yaw + 1.5707963f, 10);   // furnace mouths
        B.kit(true).dome({p.x, STOP[3] + 1.2f, p.z}, 2.2f, ember, 3, 10);
    }
    B.kit(true).curve({C.x, 0.f, C.z}, RGAL + 0.4f, 0.f, 6.2831853f, STOP[3] + 1.0f, STOP[3] + 1.15f, 0.12f, ember, 48);   // rail light

    // ---- the pit: a round floor fitted round the cage's outline (strips along
    // X, each split where the cage is), so it docks flush; six anchors, rubble ----
    {
        const float STRIP = 0.5f;
        for (float z0 = -RW; z0 < RW - 1e-3f; z0 += STRIP) {
            float z1 = std::min(z0 + STRIP, RW);
            float zf = std::max(std::fabs(z0), std::fabs(z1));
            float xOut = std::sqrt(std::max(0.f, RW * RW - zf * zf));
            if (xOut < 0.3f) continue;
            float xIn = 0.f;
            for (const Rect& r : CAGE) if (z1 > -r.hz + 1e-3f && z0 < r.hz - 1e-3f) xIn = std::max(xIn, r.hx);
            if (xIn <= 0.f) { wall(C.x - xOut, PIT - 1, C.z + z0, C.x + xOut, PIT, C.z + z1, iron); continue; }
            xIn += 0.02f;
            if (xOut <= xIn) continue;
            wall(C.x - xOut, PIT - 1, C.z + z0, C.x - xIn, PIT, C.z + z1, iron);
            wall(C.x + xIn, PIT - 1, C.z + z0, C.x + xOut, PIT, C.z + z1, iron);
        }
    }
    for (int k = 0; k < 6; ++k) {
        static const float ANG[6] = {-170.f, -130.f, -105.f, -75.f, -50.f, -10.f};   // round the north half and the sides
        float yaw = glm::radians(ANG[k]);
        float h = 8.f + 4.f * (k % 3) / 2.f;                                          // 8, 10, 12 m up
        vec3 p = C + vec3{std::cos(yaw) * (RW - 1.0f), PIT + h, std::sin(yaw) * (RW - 1.0f)};
        int w = wall(p.x - 0.9f, p.y - 0.9f, p.z - 0.9f, p.x + 0.9f, p.y + 0.9f, p.z + 0.9f, ironDark);
        L.walls[w].hidden = true;                                                     // drawn live (glow, breaking)
        L.anchors.push_back({w, p, 400.f, true});
    }
    B.kit(true).curve({C.x, 0.f, C.z}, 15.5f, 0.f, 6.2831853f, PIT + 0.02f, PIT + 0.08f, 0.15f, blood, 48);   // a red ring round the dock
    for (int k = 0; k < 9; ++k) {                                                     // rubble round the edge
        float yaw = k * 0.698f + 0.2f;
        vec3 p = C + vec3{std::cos(yaw) * 30.f, PIT, std::sin(yaw) * 30.f};
        B.kit().rock({p.x, PIT, p.z}, 1.6f + 0.4f * (k % 3), iron, k);
    }
    L.finishPos = {C.x, PIT, C.z + 20.f};

    // ---- the cage: four overlapping driven boxes, top at the stop's height ----
    vec3 cageCol{0.2f, 0.19f, 0.2f}, cageGlow{1.2f, 0.7f, 0.3f};
    vec3 drop{0.f, PIT - TOP, 0.f};
    auto cagePart = [&](float x0, float z0, float x1, float z1) {
        int m = B.mover({C.x + (x0 + x1) * 0.5f, TOP - 0.5f, C.z + (z0 + z1) * 0.5f}, {(x1 - x0) * 0.5f, 0.5f, (z1 - z0) * 0.5f},
                        Mover::Path::DRIVEN, {0.f, 0.f, 0.f}, drop, 1.f, 0.f, cageGlow);
        L.movers[m].color = cageCol;
        L.lift.movers.push_back(m);
    };
    for (const Rect& r : CAGE) cagePart(-r.hx, -r.hz, r.hx, r.hz);
    L.lift.stops.assign(STOP, STOP + 5);

    // ---- spawns per stop: galleries round the shaft + the cage ----
    for (int w = 0; w < 3; ++w) {
        float y = STOP[w + 1];
        std::vector<vec3> g, air;
        for (int k = 0; k < 8; ++k) {
            float yaw = k * 0.7853982f + 0.3927f;                        // between bridges and clamps
            g.push_back(C + vec3{std::cos(yaw) * 27.f, y, std::sin(yaw) * 27.f});
        }
        g.push_back(C + vec3{-6.f, y, -6.f}); g.push_back(C + vec3{6.f, y, 6.f});   // on the cage
        for (int k = 0; k < 4; ++k) {
            float yaw = k * 1.5707963f;
            air.push_back(C + vec3{std::cos(yaw) * 17.f, y + 10.f, std::sin(yaw) * 17.f});
        }
        a.waveGround.push_back(g);
        a.waveAir.push_back(air);
    }
    a.waveGround.push_back({a.bossSpawn});
    a.waveAir.push_back({C + vec3{0.f, PIT + 12.f, 0.f}});
    a.groundSpawns = a.waveGround[0];
    a.airSpawns = a.waveAir[0];

    a.waves = {
        {{EnemyType::HUSK, 4}, {EnemyType::RAPTOR, 3}, {EnemyType::SERAPH, 2},
         WaveEntry(EnemyType::SHIELDBEARER, 2).with({EnemyType::HUSK}), WaveEntry(EnemyType::RIPPER, 2).hollow(Hollow::ENRAGED)},
        {{EnemyType::ANCHOR, 1}, {EnemyType::CONDUCTOR, 2}, {EnemyType::SENTINEL, 3}, {EnemyType::MITE, 6},
         WaveEntry(EnemyType::BRUTE, 2).hollow(Hollow::HALOED)},
        {{EnemyType::ANCHOR, 2}, {EnemyType::JUGGERNAUT, 1},
         WaveEntry(EnemyType::SHIELDBEARER, 2).with({EnemyType::SENTINEL}).hollow(Hollow::TWINNED),
         {EnemyType::SERAPH, 2}, WaveEntry(EnemyType::RAPTOR, 3).hollow(Hollow::ENRAGED), {EnemyType::HUSK, 4}},
        {{EnemyType::SOVEREIGN, 1}},   // replaced by the PENITENT in Task 4
    };
    a.goals = {WaveGoal{}, WaveGoal::conduits("RELEASE THE CLAMPS", clamps), WaveGoal{}, WaveGoal{}};
    a.maxAlive = 12;
    a.damageScale = 1.4f;
    a.ambient = Ambient::ASH;
    a.theme = Theme{
        {0.02f,0.015f,0.02f}, {0.22f,0.08f,0.05f}, {0.01f,0.004f,0.003f},
        glm::normalize(vec3{0.f, -1.f, -0.2f}), {1.2f,0.75f,0.4f}, 0.06f, 0.f,
        {0.05f,0.03f,0.025f}, 0.45f,
        // The light comes from the furnaces below; cold grey from the mouth above
        glm::normalize(vec3{0.1f, 0.8f, 0.2f}), {1.1f,0.5f,0.25f},
        {0.1f,0.1f,0.13f}, {0.4f,0.12f,0.06f},
        {0.12f,0.05f,0.03f}, 0.01f };
    L.arenas.push_back(std::move(a));
}
```

   Use whatever `ShapeKit` methods exist with these exact signatures (`curve`, `arch`, `dome`, `column`, `rod`, `box`, `rock`); if a call doesn't match the real signature in `src/Shapes.h`, adapt the arguments to the same intent (read the method's comment) and ledger it. The `Theme` initialiser must match the field order of `struct Theme` — copy the Orrery's layout.

- [ ] **Step 5: Run the tests**

Run: `make test 2>&1 | grep -E "FAIL|ALL PASSED|FAILED"`
Expected: `ALL PASSED` (the simulated run reaches VICTORY at arena 2 with the placeholder boss). The see-through checks (undersides, closed shapes) must pass for the new geometry too; fix geometry, not tests.

- [ ] **Step 6: Commit**

```bash
git add src/Level.h src/WaveDirector.h src/LevelAct2.h tests/test_game.cpp
git commit -m "The Descent: a shaft below the Orrery, a cage, three floors and the Penitent's pit"
```

---

### Task 3: Riding the cage in the game

**Files:**
- Modify: `src/ArenaShifts.h`, `src/LevelAct2.h` (shift), `src/GameplayState.h`, `src/Gameplay_Tick.h`, `src/Gameplay_Flow.h`, `src/Gameplay_Combat.h`, `src/MusicSynth.h`, `src/MenuState.h`, `tests/test_game.cpp`

**Interfaces:**
- Consumes: Task 1 `Lift`, `WaveDirector::hold`; Task 2 level.
- Produces:
  - `ArenaShift::DESCENT`; `ArenaShifts::reset` resets `L.lift`; `ArenaShifts::onWaveCleared(L, a, nextWave)` requests stop `nextWave + 1` for a DESCENT arena; `ArenaShifts::update` calls `L.lift.update(dt, L)`
  - `static int ArenaShifts::descentStop(int wave)` → `wave + 1`
  - `bool LevelData::onLift(int groundWall) const` → the wall is one of the lift's movers
  - `GameplayState::updateLift(float dt)` (in `Gameplay_Penitent.h`, created here with just the lift part)

- [ ] **Step 1: Write the failing tests**

```cpp
    // ---------------------------------------------------------------- the Descent's ride
    {
        LevelData N = buildAct2Level();
        ArenaShifts sh; sh.capture(N);
        CHECK(N.arenas[2].shift == ArenaShift::DESCENT, "the Descent's shift is the ride");
        N.lift.request(3); N.lift.start();
        for (int i = 0; i < 60 * 3; ++i) sh.update(DT, N, 2, true);
        bool moving = N.lift.riding();
        sh.reset(N);
        CHECK(moving && N.lift.at == 0 && !N.lift.busy(), "a retry stops the ride and puts the cage back at the top");
        sh.onWaveCleared(N, 2, 1);
        CHECK(N.lift.pending == 2, "clearing a wave asks for the next stop (it waits for the player to board)");
        CHECK(N.onLift(N.movers[N.lift.movers[0]].wall) && !N.onLift(0), "the game can tell when you're standing on the cage");
        // The cage never passes through a static wall on the way down, and is flush with every stop's bridges
        N.lift.reset(); N.lift.request(4); N.lift.start();
        float clock = 0.f; bool clear = true;
        SpatialGrid g; g.build(N.walls);
        for (int i = 0; i < 60 * 9; ++i) {
            clock += DT; N.lift.update(DT, N); N.updateMovers(clock);
            for (int m : N.lift.movers) {
                AABB b = N.walls[N.movers[m].wall].box;
                b.min += glm::vec3{0.02f}; b.max -= glm::vec3{0.02f};
                for (size_t w = 0; w < N.walls.size(); ++w)
                    if (!N.walls[w].dynamic && overlapsBox(b, N.walls[w].box)) { clear = false; }
            }
        }
        CHECK(clear, "the cage rides all the way down without touching a wall");
        bool flush = true;
        for (int s = 1; s < 5; ++s) {
            N.lift.reset(); N.lift.request(s); N.lift.start();
            for (int i = 0; i < 60 * 9; ++i) { clock += DT; N.lift.update(DT, N); N.updateMovers(clock); }
            float top = N.walls[N.movers[N.lift.movers[1]].wall].box.max.y;
            float bridge = N.groundAt(12.6f, -840.f, top + 0.1f);
            flush &= std::fabs(top - N.lift.stops[s]) < 0.02f && std::fabs(bridge - top) < 0.02f;
        }
        CHECK(flush, "at every stop the cage sits flush with the floor round it");
        // A player standing on the cage for a whole ride is carried and stays on
        Player p({0.f, -80.f, -840.f});
        N.lift.reset(); N.lift.update(0.f, N); N.updateMovers(clock);
        SpatialGrid pg; pg.build(N.walls);
        p.position = {3.f, -80.f, -838.f}; p.velocity = glm::vec3{0.f};
        N.lift.request(1); N.lift.start();
        const Uint8* none = SDL_GetKeyboardState(nullptr);
        bool stayed = true;
        for (int i = 0; i < 60 * 9; ++i) {
            clock += DT; N.lift.update(DT, N); N.updateMovers(clock);
            int rm = N.moverOfWall(p.groundWall);
            if (rm >= 0) p.position += N.movers[rm].delta;
            p.update(DT, none, N.walls, pg, N.moverWalls);
            stayed &= p.position.y > N.lift.y() - 1.0f && p.position.y < N.lift.y() + 1.5f;
        }
        CHECK(stayed && std::fabs(p.position.y - (-120.f)) < 0.1f, "standing on the cage, you ride it down and stay on");
    }
```

(Adapt the `Player` construction/update call to the exact signature already used by `tests/test_physics.cpp` / the Orrery ring-riding test in `test_game.cpp` — copy that test's setup, which already carries a player on a mover.)

- [ ] **Step 2: Run to verify it fails**

Run: `make test 2>&1 | grep -E "error|FAIL" | head -5`
Expected: no `ArenaShift::DESCENT`, no `onLift`.

- [ ] **Step 3: Implement the shift**

`src/Level.h`: `enum class ArenaShift { NONE, NIGHTFALL, LAVA_RISE, SPEED_UP, OVERLOAD, FLOOD, SOLAR, DESCENT };` and in `LevelData`:

```cpp
    bool onLift(int groundWall) const {
        for (int m : lift.movers) if (movers[m].wall == groundWall) return true;
        return false;
    }
```

`src/LevelAct2.h`: `a.shift = ArenaShift::DESCENT;` in `buildDescent`.

`src/ArenaShifts.h` (header comment: add a DESCENT line — "the cage rides to the next stop once a wave is cleared and you're aboard"):
- `static int descentStop(int wave) { return wave + 1; }`
- in `reset(LevelData& L)`: `L.lift.reset(); L.lift.update(0.f, L);`
- in `onWaveCleared(LevelData& L, int a, int nextWave)`: `if (L.arenas[a].shift == ArenaShift::DESCENT) L.lift.request(descentStop(nextWave));`
- at the top of `update(...)`: `L.lift.update(dt, L);`

- [ ] **Step 4: Enemies stand on the cage**

`src/Gameplay_Tick.h`, `groundHeightAt`: after the grid loop, also test the movers:

```cpp
    for (int wi : level.moverWalls) {   // moving platforms (the Descent's cage) aren't in the grid
        const AABB& b = level.walls[wi].box;
        if (x >= b.min.x && x <= b.max.x && z >= b.min.z && z <= b.max.z && b.max.y <= fromY + 0.5f)
            best = std::max(best, b.max.y);
    }
```

- [ ] **Step 5: The ride in the game (`src/Gameplay_Penitent.h`, new)**

```cpp
#pragma once
// =============================================================================
// Gameplay_Penitent.h — the Descent's cage and the PENITENT, wired into
// GameplayState: the ride between waves, the chain anchors (shoot them or
// grapple on and rip them out), its hazards turned into damage, drawing and
// sound. Included at the end of GameplayState.h.
// =============================================================================

// The cage: a ride waits for the player to be aboard, the next wave waits for
// the ride, and the fall line follows the cage
inline void GameplayState::updateLift(float dt) {
    (void)dt;
    const Arena& ar = level.arenas[director.arena];
    if (ar.shift != ArenaShift::DESCENT) { director.hold = false; return; }
    auto& lift = level.lift;
    if (lift.pending >= 0 && !lift.riding()) {
        if (player.onGround && level.onLift(player.groundWall)) {
            lift.start();
            for (auto& p : projSystem.pool) p.alive = false;   // nothing left over from the last floor
            pickups.push_back(Pickup{glm::vec3{-4.f, lift.y() + 6.f, -840.f}, PickupKind::ORB});
            pickups.push_back(Pickup{glm::vec3{4.f, lift.y() + 6.f, -840.f}, PickupKind::ORB});
            audio.playAt("door_close", {0.f, lift.y(), -840.f}, 128, SoundGroup::WORLD);
            for (int k = 0; k < 4; ++k)
                audio.playAt("clank", glm::vec3{k < 2 ? -12.f : 12.f, lift.y() + 8.f, k % 2 ? -828.f : -852.f}, 70, SoundGroup::WORLD);
        } else if (!boardHinted) {
            boardHinted = true;
            pushBanner("BOARD THE CAGE", "IT WON'T GO DOWN WITHOUT YOU", {1.f, 0.7f, 0.3f}, 2.5f);
        }
    }
    if (lift.riding()) boardHinted = false;
    director.hold = lift.busy();
    // The fall line: 25 m under the cage; a fall puts you back aboard
    Arena& mar = level.arenas[director.arena];
    mar.voidY = lift.y() - 25.f;
    mar.respawn = {0.f, lift.y(), -840.f};
}
```

(Construct `Pickup` the way `Gameplay_Combat.h` already does for dropped orbs — copy that line; if `Pickup` has more fields, set them the same way.)

`src/GameplayState.h`: add `#include "Gameplay_Penitent.h"` after the other `Gameplay_*.h` includes at the end; declare `void updateLift(float dt);` and `bool boardHinted = false;` with the other members. Add `src/Gameplay_Penitent.h` to the Makefile `HEADERS`.

`src/Gameplay_Tick.h`, in `physicsTick` right after `updateShifts(dt);`: `updateLift(dt);`

`src/Gameplay_Flow.h`:
- `ARENA_START`: for a DESCENT arena, `level.lift.request(ArenaShifts::descentStop(director.wave));` and a banner `pushBanner("THE DESCENT", "BOARD THE CAGE", {1.f, 0.7f, 0.3f}, 3.f);` in the act2 branch when `ar.shift == ArenaShift::DESCENT` (instead of the generic act2 banner).
- `ARENA_CLEARED` act2 banner: `ev.value == 0 ? "BEHIND THE ORGAN" : "THROUGH THE NORTH ARCH"`.
- `enterArena`: `boardHinted = false;` (the shift reset already resets the lift).
- Music: `m.setTrack(... act2() ? 5 + std::min(a, 2) : ...)`.

`src/Gameplay_Combat.h`: check the enemy void line (`if (e.position.y < ar.voidY)`) uses the arena's live `voidY` — it does; nothing to change.

`src/MusicSynth.h`: move `constexpr int MUSIC_TRACKS = 8;` above `musicTrack`, index with it (`T[((i % MUSIC_TRACKS) + MUSIC_TRACKS) % MUSIC_TRACKS]`), and add:

```cpp
        // The Descent: slow, heavy, low brass and chains, falling forever
        {"DESCENT", 132.f, 43, {0, 6, 3, 5},
         "x.....x.x.......", "....x.......x..g", "x.x.X.x.x.xxX.xo",
         "x---o---x--fo---", "0..1..2..3..2..1", "3-------2-------1---0---1-------"},
```

`src/MenuState.h`: `"PREVIEW - 3/4 ARENAS - BENEATH THE ECLIPSE"`.

- [ ] **Step 6: Run the tests; build**

Run: `make test 2>&1 | grep -E "FAIL|ALL PASSED|FAILED"; make 2>&1 | grep -E " error"`
Expected: `ALL PASSED`, no build errors.

- [ ] **Step 7: Screenshots of the ride**

```bash
./shooter --act2 --arena 3 --god --res 720 --cam 0 -78 -812 -90 -35 --shot 40 $SCRATCH/descent_top.bmp
./shooter --act2 --arena 3 --god --res 720 --cam 0 -118 -830 -90 -10 --shot 40 $SCRATCH/descent_stop1.bmp
```
Convert with `sips -s format png` and look: the shaft, the cage, galleries; nothing see-through. Fix geometry that reads as plain boxes (the maps rule).

- [ ] **Step 8: Commit**

```bash
git add src/*.h tests/test_game.cpp Makefile
git commit -m "Into the Descent: board the cage, ride it down a floor per wave"
```

---

### Task 4: The Penitent's mind

**Files:**
- Modify: `src/Enemy.h`, `src/LevelAct2.h` (boss wave), `tests/test_game.cpp`
- Create: `src/EnemyPenitent.h`

**Interfaces:**
- Produces:
  - `EnemyType::PENITENT` (value 14, before `COUNT`); `isBoss` includes it; `canBeHollow` excludes it (bosses already excluded)
  - Stats: `{"PENITENT", 6000.f, 3.0f, 8.2f, 3.5f, 0.9f, 2.6f, false, {0.14f,0.13f,0.14f}, {1.3f,0.75f,0.3f}, {1.2f,0.5f,0.2f}, "BREAK ITS CHAINS - JUMP THE LOW SWEEP, SLIDE UNDER THE HIGH"}`
  - `AttackKind` += `CENSER_LOW, CENSER_HIGH, PSLAM, PSTOMP, PLASH, SCOURGE`
  - `EnemyEvents` += `int penSweep = -1; bool penSlam, penStomp, penEmbers, penRose, penLash; glm::vec3 penLashFrom, penLashDir; int penIncense; glm::vec3 penIncensePos[3]; int penSummon;`
  - `Enemy` members: `int anchorsLeft = 0; bool risen = false; float riseTimer = 0.f; bool scourging = false; float farTimer = 0.f, incenseTimer = 6.f, summonTimer = 25.f, scourgeTimer = 4.f; int comboLeft = 0, nextSweep = 0;` and constants `PEN_RISE_TIME 2.0, PEN_LASH_FAR 22, PEN_LASH_HIGH 4, PEN_LASH_AFTER 3, PEN_LASH_WARN 0.7, PEN_INCENSE_EVERY 12, PEN_SUMMON_EVERY 25, PEN_SCOURGE_EVERY 6`
  - `float Enemy::sweepReach() const` (16 chained / 18 risen); `bool Enemy::chained() const`
  - `height()` → 8.2 chained, 11.5 risen (× scale)
  - `armorMult()` → (anchorsLeft > 0 ? 0.25 : 1) × (staggered ? 2 : 1)
  - `parryWindow()` → PENITENT: attack ∈ {CENSER_LOW, CENSER_HIGH, PSLAM, PSTOMP} and telegraph < 0.25 s
  - `stagger()` also clears `comboLeft`
  - The generic 50 % "enraged" still applies (attacks 1.5× as often), on top of the phases

- [ ] **Step 1: Write the failing tests**

```cpp
    // ---------------------------------------------------------------- the PENITENT
    {
        auto world = [](glm::vec3 feet) { EnemyWorld w; w.playerFeet = feet; w.playerEye = feet + glm::vec3{0, 1.7f, 0}; return w; };
        const glm::vec3 home{0.f, -240.f, -864.f};
        {   // chained: never moves, takes a quarter
            Enemy p(EnemyType::PENITENT, home, -240.f); p.anchorsLeft = 6; p.spawnTimer = 0.f; p.state = EnemyState::ACTIVE;
            for (int i = 0; i < 60 * 12; ++i) p.update(DT, world({0.f, -240.f, -846.f}));
            CHECK(glm::length(glm::vec2{p.position.x - home.x, p.position.z - home.z}) < 0.01f, "PENITENT: chained, it never moves");
            CHECK(std::fabs(p.armorMult() - 0.25f) < 1e-4f, "PENITENT: chained, it takes a quarter of the damage");
            CHECK(std::fabs(p.height() - 8.2f) < 1e-3f, "PENITENT: kneeling, 8.2 m");
        }
        {   // it attacks with both sweeps, a slam, and drops incense
            Enemy p(EnemyType::PENITENT, home, -240.f); p.anchorsLeft = 6; p.spawnTimer = 0.f; p.state = EnemyState::ACTIVE;
            int low = 0, high = 0, slams = 0, incense = 0; bool tell = false, window = false;
            for (int i = 0; i < 60 * 40; ++i) {
                p.update(DT, world({0.f, -240.f, -852.f}));
                low += p.ev.penSweep == 0; high += p.ev.penSweep == 1; slams += p.ev.penSlam; incense += p.ev.penIncense;
                tell |= p.ev.telegraphStarted; window |= p.parryWindow();
            }
            CHECK(low > 0 && high > 0 && slams > 0, "PENITENT: low sweeps, high sweeps and slams");
            CHECK(tell && window, "PENITENT: every blow has a wind-up, with a parry window at its end");
            CHECK(incense >= 6, "PENITENT: incense every ~12 s, three pools at a time");
        }
        {   // the lash: only after 3 s far away or up high
            Enemy p(EnemyType::PENITENT, home, -240.f); p.anchorsLeft = 6; p.spawnTimer = 0.f; p.state = EnemyState::ACTIVE;
            bool lashedClose = false;
            for (int i = 0; i < 60 * 10; ++i) { p.update(DT, world({0.f, -240.f, -850.f})); lashedClose |= p.ev.penLash; }
            float firstLash = -1.f;
            for (int i = 0; i < 60 * 6 && firstLash < 0.f; ++i) { p.update(DT, world({0.f, -240.f, -830.f + 0.f * i + 4.f})); if (p.ev.penLash) firstLash = i * DT; }
            CHECK(!lashedClose, "PENITENT: no lash while you fight it up close on the floor");
            CHECK(firstLash >= 2.9f && firstLash < 4.6f, "PENITENT: stay 22 m away for 3 s and the chain lashes at you");
            Enemy q(EnemyType::PENITENT, home, -240.f); q.anchorsLeft = 6; q.spawnTimer = 0.f; q.state = EnemyState::ACTIVE;
            bool perched = false;
            for (int i = 0; i < 60 * 5; ++i) { q.update(DT, world({0.f, -233.f, -850.f})); perched |= q.ev.penLash; }
            CHECK(perched, "PENITENT: perch up high and the chain finds you there too");
        }
        {   // unchained: it rises and walks; scourge: faster, and it lashes itself
            Enemy p(EnemyType::PENITENT, home, -240.f); p.anchorsLeft = 0; p.spawnTimer = 0.f; p.state = EnemyState::ACTIVE;
            bool rose = false; glm::vec3 start = p.position;
            for (int i = 0; i < 60 * 8; ++i) { p.update(DT, world({0.f, -240.f, -830.f})); rose |= p.ev.penRose; }
            CHECK(rose && p.risen && glm::length(p.position - start) > 4.f && std::fabs(p.height() - 11.5f) < 1e-3f,
                  "PENITENT: with every chain broken it rises (11.5 m) and stalks you");
            CHECK(std::fabs(p.armorMult() - 1.f) < 1e-4f, "PENITENT: unchained, it takes full damage");
            int summons = 0;
            for (int i = 0; i < 60 * 30; ++i) { p.update(DT, world({0.f, -240.f, -830.f})); summons += p.ev.penSummon; }
            CHECK(summons >= 4, "PENITENT: risen, it calls up Hollowed Husks every 25 s");
            p.health = p.maxHealth * 0.2f;
            int embers = 0; float firstTell = -1.f;
            for (int i = 0; i < 60 * 15; ++i) {
                p.update(DT, world({0.f, -240.f, -850.f}));
                embers += p.ev.penEmbers;
                if (p.ev.telegraphStarted && firstTell < 0.f && p.attack != AttackKind::SCOURGE) firstTell = p.telegraphDuration;
            }
            CHECK(p.scourging && embers >= 1, "PENITENT: under a quarter it scourges itself, throwing rings of embers");
            CHECK(firstTell > 0.f && firstTell < 0.72f, "PENITENT: scourging, its wind-ups come 30% sooner (a sweep 0.63 s, a slam 0.7 s)");
        }
        {   // a parry staggers it: no blows during the stagger, double damage
            Enemy p(EnemyType::PENITENT, home, -240.f); p.anchorsLeft = 0; p.risen = true; p.spawnTimer = 0.f; p.state = EnemyState::ACTIVE;
            p.stagger(p.staggerTime());
            bool hit = false;
            for (int i = 0; i < 60 * 2; ++i) { p.update(DT, world({0.f, -240.f, -858.f})); hit |= p.ev.penSweep >= 0 || p.ev.penSlam || p.ev.penStomp; }
            CHECK(std::fabs(p.staggerTime() - 2.5f) < 1e-4f && !hit, "PENITENT: parried, it reels for 2.5 s and strikes nothing");
            CHECK(std::fabs(p.armorMult() - 2.f) < 1e-4f || !p.staggered(), "PENITENT: reeling, it takes double");
        }
    }
```

- [ ] **Step 2: Run to verify it fails**

Run: `make test 2>&1 | grep -E "error" | head -3`
Expected: no `EnemyType::PENITENT`.

- [ ] **Step 3: Implement**

`src/Enemy.h`:
- enum: `..., SERAPH, ANCHOR, PENITENT, COUNT };`; `isBoss`: `|| t == EnemyType::PENITENT`.
- the stats row from Interfaces, appended after ANCHOR's.
- `AttackKind`: append `CENSER_LOW, CENSER_HIGH, PSLAM, PSTOMP, PLASH, SCOURGE   // the PENITENT's`.
- `EnemyEvents`: append

```cpp
    // PENITENT: a censer sweep landed (0 low: jump it, 1 high: slide under
    // it), a slam / stomp ring, a chain lash marked toward you, incense pools,
    // a ring of embers off its own back, the moment it rises, Husks called up
    int       penSweep = -1;
    bool      penSlam = false, penStomp = false, penEmbers = false, penRose = false, penLash = false;
    glm::vec3 penLashFrom{0.f}, penLashDir{0.f};
    int       penIncense = 0;
    glm::vec3 penIncensePos[3];
    int       penSummon = 0;
```
- members (public, after the SOVEREIGN block):

```cpp
    // PENITENT
    static constexpr float PEN_RISE_TIME = 2.f, PEN_LASH_FAR = 22.f, PEN_LASH_HIGH = 4.f, PEN_LASH_AFTER = 3.f,
                           PEN_LASH_WARN = 0.7f, PEN_INCENSE_EVERY = 12.f, PEN_SUMMON_EVERY = 25.f, PEN_SCOURGE_EVERY = 6.f;
    int   anchorsLeft  = 0;      // its chains still holding (GameplayState sets it every tick)
    bool  risen        = false;  // every chain broken: it stands and walks
    float riseTimer    = 0.f;
    bool  scourging    = false;  // under a quarter: it lashes itself, the wound on its back open
    float farTimer     = 0.f;    // how long you've kept your distance (or perched)
    float incenseTimer = 6.f, summonTimer = PEN_SUMMON_EVERY, scourgeTimer = 4.f;
    int   comboLeft    = 0, nextSweep = 0;
    bool  chained() const { return type == EnemyType::PENITENT && !risen; }
    float sweepReach() const { return risen ? 18.f : 16.f; }
```
- `height()`: `if (type == EnemyType::PENITENT) return (risen ? 11.5f : 8.2f) * scale;` before the stats return.
- `armorMult()`: first line `if (type == EnemyType::PENITENT) return (anchorsLeft > 0 ? 0.25f : 1.f) * (staggered() ? 2.f : 1.f);`
- `parryWindow()`: before `return false;`:

```cpp
        if (type == EnemyType::PENITENT)
            return (attack == AttackKind::CENSER_LOW || attack == AttackKind::CENSER_HIGH ||
                    attack == AttackKind::PSLAM || attack == AttackKind::PSTOMP) && telegraphTimer < 0.25f;
```
- `stagger()`: add `comboLeft = 0;`.
- `update()` switch: `case EnemyType::PENITENT: thinkPenitent(dt, w, resolve); break;`
- declare in the private section: `void thinkPenitent(float dt, const EnemyWorld& w, bool resolve);`
- at the end of `Enemy.h` (after the class and any free functions that `EnemyPenitent.h` needs): `#include "EnemyPenitent.h"`.

`src/EnemyPenitent.h`:

```cpp
#pragma once
// =============================================================================
// EnemyPenitent.h — THE PENITENT's mind (Enemy::thinkPenitent). Included at
// the end of Enemy.h. It only decides and emits events (EnemyEvents::pen*);
// PenitentHazards.h turns them into what hurts you.
//
//   CHAINED   (anchorsLeft > 0)  kneels, turns, can't move; takes a quarter.
//             Censer sweeps low (jump) or high (slide under), a slam ring,
//             incense pools every 12 s, and the chain lash for anyone who
//             keeps away (22 m) or perches (4 m up) for 3 s.
//   UNCHAINED (all broken)       rises (2 s), stalks you; sweeps come in
//             pairs, low then high or high then low; stomps up close; calls
//             up four Hollowed Husks every 25 s; full damage.
//   SCOURGE   (under 25 %)       lashes its own back every 6 s (a ring of
//             embers), the wound opens; everything 30 % sooner.
// =============================================================================

inline void Enemy::thinkPenitent(float dt, const EnemyWorld& w, bool resolve) {
    if (!risen && anchorsLeft == 0) {   // the last chain broke
        risen = true; riseTimer = PEN_RISE_TIME; ev.penRose = true;
        attack = AttackKind::NONE; telegraphTimer = 0.f; comboLeft = 0;
    }
    if (!scourging && health < maxHealth * 0.25f) { scourging = true; scourgeTimer = 1.f; }
    const float quick = scourging ? 0.7f : 1.f;
    glm::vec3 to = flatTo(w.playerFeet);
    float d = glm::length(to);

    if (riseTimer > 0.f) { riseTimer -= dt; velocity.x = velocity.z = 0.f; animPhase += dt; return; }

    // Moving: chained it only turns; risen it walks at you, stopping to strike
    if (!risen || telegraphTimer > 0.f || d < 6.f) { velocity.x = velocity.z = 0.f; }
    else { setMove(norm2(to), stats().speed, w); animPhase += dt * 1.6f; }
    if (telegraphTimer <= 0.f) turnToward(to, dt, risen ? 1.1f : 0.8f);

    // Keeping your distance (or perching) winds up the lash
    bool away = d > PEN_LASH_FAR || w.playerFeet.y > floorY + PEN_LASH_HIGH;
    farTimer = away ? farTimer + dt : 0.f;

    if (resolve) {
        switch (attack) {
            case AttackKind::CENSER_LOW:  ev.penSweep = 0; break;
            case AttackKind::CENSER_HIGH: ev.penSweep = 1; break;
            case AttackKind::PSLAM:       ev.penSlam = true; break;
            case AttackKind::PSTOMP:      ev.penStomp = true; break;
            case AttackKind::SCOURGE:     ev.penEmbers = true; break;
            default: break;
        }
        bool swept = ev.penSweep >= 0;
        attack = AttackKind::NONE;
        if (swept && comboLeft > 0) {   // the second stroke of a pair, the other height
            --comboLeft;
            nextSweep = 1 - nextSweep;
            startAttack(nextSweep ? AttackKind::CENSER_HIGH : AttackKind::CENSER_LOW, 0.75f * quick);
            return;
        }
    }

    // Incense: three pools round where you stand
    incenseTimer -= dt / quick;
    if (incenseTimer <= 0.f) {
        incenseTimer = PEN_INCENSE_EVERY;
        ev.penIncense = 3;
        float base = frand(0.f, 6.2831853f);
        for (int k = 0; k < 3; ++k) {
            float a = base + k * 2.0943951f;
            float r = k == 0 ? 0.f : 4.5f;
            ev.penIncensePos[k] = glm::vec3{w.playerFeet.x + std::cos(a) * r, floorY, w.playerFeet.z + std::sin(a) * r};
        }
    }
    if (risen) {
        summonTimer -= dt;
        if (summonTimer <= 0.f) { summonTimer = PEN_SUMMON_EVERY; ev.penSummon = 4; }
    }
    if (telegraphTimer > 0.f) return;

    if (scourging) {
        scourgeTimer -= dt;
        if (scourgeTimer <= 0.f) { scourgeTimer = PEN_SCOURGE_EVERY * quick; startAttack(AttackKind::SCOURGE, 0.8f * quick); return; }
    }
    if (farTimer >= PEN_LASH_AFTER) {   // the lash, marked along the floor toward you
        farTimer = 0.f;
        ev.penLash = true;
        ev.penLashFrom = position + glm::vec3{0.f, 0.3f, 0.f};
        ev.penLashDir = norm2(to);
        startAttack(AttackKind::PLASH, PEN_LASH_WARN);
        return;
    }
    if (!attackReady(dt / quick)) return;
    ++attackCount;
    if (risen && d < 6.f) { startAttack(AttackKind::PSTOMP, 0.8f * quick); return; }
    if (d > sweepReach()) return;   // out of reach: chained it waits for the lash, risen it keeps walking
    if (attackCount % 4 == 0) { startAttack(AttackKind::PSLAM, 1.0f * quick); return; }
    nextSweep = rand() % 2;
    comboLeft = risen ? (scourging ? 2 : 1) : 0;
    startAttack(nextSweep ? AttackKind::CENSER_HIGH : AttackKind::CENSER_LOW, 0.9f * quick);
}
```

`startAttack` is private; `thinkPenitent` is a member, so it can call it. If `attackReady` uses `telegraphTimer` guards, the `dt / quick` division is safe (quick ≥ 0.7).

`src/LevelAct2.h`: the Descent's boss wave `{{EnemyType::PENITENT, 1}}`.

- [ ] **Step 4: Run to verify it passes**

Run: `make test 2>&1 | grep -E "PENITENT|FAIL|ALL PASSED|FAILED"`
Expected: every PENITENT line `ok:`, `ALL PASSED`. Tune only the constants named in Global Constraints if a timing check is marginal, and ledger it.

- [ ] **Step 5: Commit**

```bash
git add src/Enemy.h src/EnemyPenitent.h src/LevelAct2.h tests/test_game.cpp Makefile
git commit -m "The Penitent's mind: chained, unchained, scourging"
```

(Add `src/EnemyPenitent.h` to `HEADERS` and the `test:` deps.)

---

### Task 5: What hurts you — PenitentHazards

**Files:**
- Create: `src/PenitentHazards.h`
- Modify: `tests/test_game.cpp`, `Makefile`

**Interfaces:**
- Consumes: nothing from earlier tasks (pure geometry).
- Produces:

```cpp
class PenitentHazards {
public:
    static constexpr float HALF_ARC = 1.7453293f;          // ±100°
    static constexpr float LOW_TOP = 1.0f, HIGH_DUCK = 1.3f, HIGH_TOP = 3.5f;
    static constexpr float SLAM_RADIUS = 14.f, STOMP_RADIUS = 6.f, RING_SPEED = 16.f, RING_HEIGHT = 1.2f, EMBER_RANGE = 30.f;
    static constexpr float LASH_WARN = 0.7f, LASH_REACH = 40.f, LASH_HALF = 1.3f, LASH_LINGER = 0.3f;
    static constexpr float POOL_RADIUS = 3.5f, POOL_TIME = 8.f, POOL_WARN = 0.6f, POOL_DPS = 20.f, POOL_TICK = 0.25f;
    static constexpr float SWEEP_DAMAGE = 30.f, SLAM_DAMAGE = 30.f, LASH_DAMAGE = 35.f, EMBER_DAMAGE = 15.f;
    struct Arc  { glm::vec3 centre; float yaw, reach; int kind; float t = 0.f; };          // a drawn sweep trail
    struct Ring { glm::vec3 centre; float radius = 0.f, prev = 0.f, maxR; float damage; int kind; bool hit = false; }; // 0 slam/stomp, 1 embers
    struct Lash { glm::vec3 from, dir; float t = 0.f; bool yank; bool fired = false; };
    struct Pool { glm::vec3 pos; float t = 0.f; };
    struct Hit  { float damage; glm::vec3 from; bool yank; };
    std::vector<Arc> arcs; std::vector<Ring> rings; std::vector<Lash> lashes; std::vector<Pool> pools;
    static bool sweepHits(int kind, glm::vec3 centre, float yaw, float reach, glm::vec3 feet, float height, float floorY);
    void addSweep(glm::vec3 centre, float yaw, float reach, int kind);
    void addRing(glm::vec3 centre, float maxR, float damage, int kind);
    void addLash(glm::vec3 from, glm::vec3 dir, bool yank);
    void addPool(glm::vec3 pos);
    std::vector<Hit> update(float dt, glm::vec3 feet, float floorY);
    void clear();
};
```

- [ ] **Step 1: Write the failing tests**

```cpp
    // ---------------------------------------------------------------- the Penitent's hazards
    {
        using PH = PenitentHazards;
        const glm::vec3 c{0.f, -240.f, -864.f};
        const float yaw = 3.14159265f;   // facing +Z... (yaw 0 faces +Z): yaw pi faces -Z; the player is at +Z, so use 0
        const glm::vec3 front = c + glm::vec3{0.f, 0.f, 8.f};
        CHECK(PH::sweepHits(0, c, 0.f, 16.f, front, 1.8f, -240.f), "a low sweep hits you standing");
        CHECK(!PH::sweepHits(0, c, 0.f, 16.f, front + glm::vec3{0, 1.2f, 0}, 1.8f, -240.f), "...and passes under you mid-jump");
        CHECK(PH::sweepHits(1, c, 0.f, 16.f, front, 1.8f, -240.f), "a high sweep hits you standing");
        CHECK(PH::sweepHits(1, c, 0.f, 16.f, front + glm::vec3{0, 1.2f, 0}, 1.8f, -240.f), "...and mid-jump");
        CHECK(!PH::sweepHits(1, c, 0.f, 16.f, front, 0.9f, -240.f), "...but passes over you sliding");
        CHECK(!PH::sweepHits(0, c, 0.f, 16.f, c + glm::vec3{0, 0, -8.f}, 1.8f, -240.f), "a sweep misses behind it");
        CHECK(!PH::sweepHits(0, c, 0.f, 16.f, c + glm::vec3{0, 0, 17.f}, 1.8f, -240.f), "...and beyond its reach");
        (void)yaw;
        // A slam ring hits once, as its edge passes; not if you're in the air
        PH h; h.addRing(c, PH::SLAM_RADIUS, PH::SLAM_DAMAGE, 0);
        int hits = 0; float t = 0.f;
        for (int i = 0; i < 120; ++i) { auto hs = h.update(DT, c + glm::vec3{0, 0, 8.f}, -240.f); hits += (int)hs.size(); t += DT; }
        PH j; j.addRing(c, PH::SLAM_RADIUS, PH::SLAM_DAMAGE, 0);
        int airHits = 0;
        for (int i = 0; i < 120; ++i) airHits += (int)j.update(DT, c + glm::vec3{0, 1.5f, 8.f}, -240.f).size();
        CHECK(hits == 1 && airHits == 0 && h.rings.empty(), "a slam's ring hits you once on the floor, never in the air, then is gone");
        // The lash: warned, then it hits along its line (and only there)
        PH l; l.addLash(c, {0.f, 0.f, 1.f}, true);
        int before = 0, on = 0;
        for (int i = 0; i < 30; ++i) before += (int)l.update(DT, c + glm::vec3{0, 0, 20.f}, -240.f).size();   // 0.5 s: still the warning
        std::vector<PH::Hit> hs;
        for (int i = 0; i < 30; ++i) { auto x = l.update(DT, c + glm::vec3{0.5f, 0, 20.f}, -240.f); on += (int)x.size(); if (!x.empty()) hs = x; }
        PH l2; l2.addLash(c, {0.f, 0.f, 1.f}, false);
        int off = 0;
        for (int i = 0; i < 60; ++i) off += (int)l2.update(DT, c + glm::vec3{5.f, 0, 20.f}, -240.f).size();
        CHECK(before == 0 && on == 1 && off == 0 && !hs.empty() && hs[0].yank && std::fabs(hs[0].damage - 35.f) < 1e-4f,
              "the lash waits out its warning, hits once along its line (and can yank), misses beside it");
        // Incense: harmless while it lands, 20/s inside, nothing outside, gone after 8 s
        PH p; p.addPool(c);
        float in = 0.f, out = 0.f;
        for (int i = 0; i < 60 * 9; ++i) {
            for (auto& x : p.update(DT, c + glm::vec3{1.f, 0, 0}, -240.f)) in += x.damage;
        }
        PH q; q.addPool(c);
        for (int i = 0; i < 60 * 9; ++i) for (auto& x : q.update(DT, c + glm::vec3{5.f, 0, 0}, -240.f)) out += x.damage;
        CHECK(std::fabs(in - 20.f * (8.f - 0.6f)) < 6.f && out == 0.f && p.pools.empty(),
              "incense burns 20/s inside it after landing, nothing outside, and fades after 8 s");
    }
```

- [ ] **Step 2: Run to verify it fails**

Run: `make test 2>&1 | grep error | head -2`
Expected: `PenitentHazards` not found. (Add `#include "../src/PenitentHazards.h"` to the test includes.)

- [ ] **Step 3: Implement `src/PenitentHazards.h`**

```cpp
#pragma once
// =============================================================================
// PenitentHazards.h — what THE PENITENT throws at you besides its body: its
// censer sweeps (low: jump; high: slide under), the shockwave rings of its
// slam and stomp and the embers off its own back, the chain lash marked along
// the floor, and burning incense. No OpenGL or audio: GameplayState feeds it
// the Penitent's events, draws it, and turns its hits into damage
// (Gameplay_Penitent.h).
// =============================================================================
#include <glm/glm.hpp>
#include <vector>
#include <cmath>
#include <algorithm>

class PenitentHazards {
public:
    // (constants and structs exactly as in the Interfaces block)

    // Does a sweep of `kind` from `centre`, facing `yaw` (0 = +Z), reach the
    // player (feet, height) on a floor at floorY?
    static bool sweepHits(int kind, glm::vec3 centre, float yaw, float reach, glm::vec3 feet, float height, float floorY) {
        glm::vec2 d{feet.x - centre.x, feet.z - centre.z};
        float dist = glm::length(d);
        if (dist > reach) return false;
        if (dist > 0.5f) {
            glm::vec2 f{std::sin(yaw), std::cos(yaw)};
            if (glm::dot(d / dist, f) < std::cos(HALF_ARC)) return false;
        }
        float up = feet.y - floorY;
        if (kind == 0) return up < LOW_TOP;                          // low: clear it by jumping
        return up + height > HIGH_DUCK && up < HIGH_TOP;              // high: duck under it
    }
    void addSweep(glm::vec3 centre, float yaw, float reach, int kind) { arcs.push_back({centre, yaw, reach, kind}); }
    void addRing(glm::vec3 centre, float maxR, float damage, int kind) { Ring r; r.centre = centre; r.maxR = maxR; r.damage = damage; r.kind = kind; rings.push_back(r); }
    void addLash(glm::vec3 from, glm::vec3 dir, bool yank) { Lash l; l.from = from; l.dir = dir; l.yank = yank; lashes.push_back(l); }
    void addPool(glm::vec3 pos) { Pool p; p.pos = pos; pools.push_back(p); }
    void clear() { arcs.clear(); rings.clear(); lashes.clear(); pools.clear(); poolTick = 0.f; }

    std::vector<Hit> update(float dt, glm::vec3 feet, float floorY) {
        std::vector<Hit> hits;
        for (auto& a : arcs) a.t += dt;
        arcs.erase(std::remove_if(arcs.begin(), arcs.end(), [](const Arc& a) { return a.t > 0.4f; }), arcs.end());
        float up = feet.y - floorY;
        for (auto& r : rings) {
            r.prev = r.radius; r.radius += RING_SPEED * dt;
            float d = glm::length(glm::vec2{feet.x - r.centre.x, feet.z - r.centre.z});
            if (!r.hit && up < RING_HEIGHT && d >= r.prev && d < r.radius) { r.hit = true; hits.push_back({r.damage, r.centre, false}); }
        }
        rings.erase(std::remove_if(rings.begin(), rings.end(), [](const Ring& r) { return r.radius >= r.maxR; }), rings.end());
        for (auto& l : lashes) {
            l.t += dt;
            if (!l.fired && l.t >= LASH_WARN) {
                l.fired = true;
                glm::vec2 o{l.from.x, l.from.z}, dir{l.dir.x, l.dir.z}, p{feet.x, feet.z};
                float along = glm::dot(p - o, dir);
                float side = glm::length((p - o) - dir * along);
                if (along > 0.f && along < LASH_REACH && side < LASH_HALF && up < 3.f)
                    hits.push_back({LASH_DAMAGE, l.from, l.yank});
            }
        }
        lashes.erase(std::remove_if(lashes.begin(), lashes.end(), [](const Lash& l) { return l.t > LASH_WARN + LASH_LINGER; }), lashes.end());
        bool inPool = false;
        for (auto& p : pools) {
            p.t += dt;
            if (p.t >= POOL_WARN && up < 0.6f && glm::length(glm::vec2{feet.x - p.pos.x, feet.z - p.pos.z}) < POOL_RADIUS) inPool = true;
        }
        pools.erase(std::remove_if(pools.begin(), pools.end(), [](const Pool& p) { return p.t > POOL_TIME; }), pools.end());
        if (inPool) {
            poolTick += dt;
            while (poolTick >= POOL_TICK) { poolTick -= POOL_TICK; hits.push_back({POOL_DPS * POOL_TICK, feet, false}); }
        } else poolTick = 0.f;
        return hits;
    }
private:
    float poolTick = 0.f;
};
```

(Fill in the constant/struct declarations exactly as listed in Interfaces at the top of the class.)

- [ ] **Step 4: Run to verify it passes**

Run: `make test 2>&1 | grep -E "sweep|ring|lash|incense|FAIL|ALL PASSED|FAILED"`
Expected: all `ok:`, `ALL PASSED`.

- [ ] **Step 5: Commit**

```bash
git add src/PenitentHazards.h tests/test_game.cpp Makefile
git commit -m "The Penitent's hazards: sweeps to jump or slide, rings, the lash, incense"
```

---

### Task 6: The Penitent's body — rig, head, wound

**Files:**
- Modify: `src/EnemyModel.h`, `src/GameplayState.h` (`RayHit::wound`), `src/Gameplay_Combat.h` (hitscan + ×3), `tests/test_game.cpp`

**Interfaces:**
- Consumes: Task 4 members (`risen`, `scourging`, `attack`, `anchorsLeft`).
- Produces: `humanoidDims(PENITENT)`; `rig::penitentPose(const Enemy&) → PoseOverride`; the PENITENT case in `buildEnemy` (≥ 30 boxes: humanoid + hood, bone mask, two censers on chains, chains to anchors drawn by the game, back wound when scourging); `headBox` handles PENITENT; `bool rig::woundBox(const Enemy&, AABB&)` (true only while scourging); `RayHit{int enemy; float t; bool head; bool wound = false;}`.

- [ ] **Step 1: Write the failing tests**

```cpp
    // ---------------------------------------------------------------- the PENITENT's body
    {
        Enemy p(EnemyType::PENITENT, {0.f, -240.f, -864.f}, -240.f); p.spawnTimer = 0.f; p.state = EnemyState::ACTIVE; p.anchorsLeft = 6;
        std::vector<BoxInstance> boxes; buildEnemy(p, 0.f, boxes);
        AABB head, wound;
        bool hasHead = headBox(p, head), hasWound = woundBox(p, wound);
        CHECK(boxes.size() >= 30 && hasHead && !hasWound, "PENITENT: a big rig, a head, no wound until it scourges");
        CHECK(head.min.y > -240.f + 5.f && head.max.y < -240.f + 9.5f, "PENITENT: kneeling, its head is 5-9.5 m up");
        p.anchorsLeft = 0; p.risen = true;
        headBox(p, head);
        CHECK(head.min.y > -240.f + 8.5f, "PENITENT: risen, its head is up at 8.5 m+");
        p.scourging = true;
        bool w = woundBox(p, wound);
        glm::vec3 back{-std::sin(p.yaw), 0.f, -std::cos(p.yaw)};
        glm::vec3 wc = (wound.min + wound.max) * 0.5f;
        CHECK(w && glm::dot(glm::vec2{wc.x - p.position.x, wc.z - p.position.z}, glm::vec2{back.x, back.z}) > 0.5f,
              "PENITENT: scourging, the wound is on its back");
    }
```

- [ ] **Step 2: Run to verify it fails**

Run: `make test 2>&1 | grep error | head -2`
Expected: `woundBox` undeclared.

- [ ] **Step 3: Implement**

`src/EnemyModel.h`:
- `humanoidDims`: `case EnemyType::PENITENT: return {4.5f, 1.35f, 1.9f, 0.75f, 4.25f, 4.5f, 2.5f, 1.5f, 5.5f, 1.1f, {}, {}, {}};`
- `penitentPose`:

```cpp
// THE PENITENT's stance: kneeling while chained (the body sunk 3.3 m, legs
// folded under the floor), standing once risen; a censer drawn back low or
// raised high for each sweep, both fists up for the slam, one arm thrown back
// over its shoulder for the scourge
inline PoseOverride penitentPose(const Enemy& e) {
    PoseOverride o; o.on = true;
    o.crouch = e.risen ? 0.f : 3.3f;
    if (!e.risen && e.riseTimer > 0.f) o.crouch = 3.3f * (e.riseTimer / Enemy::PEN_RISE_TIME);
    float tp = smooth01(e.telegraphProgress() * 1.3f);
    o.rxR = o.rxL = 0.15f; o.rzR = o.rzL = 0.25f;
    o.torsoPitch = e.scourging ? 0.25f : 0.1f;
    switch (e.attack) {
        case AttackKind::CENSER_LOW:  o.rxR = 0.9f * tp; o.rzR = 1.2f * tp; o.torsoYaw = -0.5f * tp; break;   // drawn back low
        case AttackKind::CENSER_HIGH: o.rxR = -2.3f * tp; o.rzR = 0.6f * tp; o.torsoYaw = -0.4f * tp; break;  // raised high
        case AttackKind::PSLAM:       o.rxR = o.rxL = -2.8f * tp; o.torsoPitch = -0.2f * tp; break;           // both fists up
        case AttackKind::PSTOMP:      o.torsoPitch = 0.3f * tp; break;
        case AttackKind::PLASH:       o.rxL = -1.6f * tp; o.rzL = -0.4f; break;
        case AttackKind::SCOURGE:     o.rxR = -3.0f * tp; o.rzR = -0.9f * tp; o.torsoPitch = 0.4f; break;    // over its shoulder
        default: break;
    }
    return o;
}
```

- `buildEnemy` case:

```cpp
    case EnemyType::PENITENT: {
        // Blackened iron, a hood over a bone mask, two censers on chains
        vec3 iron{0.12f, 0.11f, 0.12f}, cloth{0.07f, 0.05f, 0.06f}, bone{0.72f, 0.68f, 0.6f};
        PoseOverride ov = penitentPose(e);
        HumanoidLook L = humanoidLook(e.type, iron, cloth, st.glow);
        auto f = humanoid(r, root, L, e.animPhase, e.risen ? std::max(stride, 0.3f) : 0.f, ArmPose::SWING, 0.f, false, &ov);
        r.box(f.head, {0.f, 0.8f, -0.1f}, {1.9f, 1.9f, 1.9f}, cloth);                                  // the hood
        r.box(f.head, {0.f, 0.75f, 0.78f}, {1.1f, 1.2f, 0.12f}, bone);                                 // the mask
        r.box(f.head, {0.f, 0.95f, 0.85f}, {0.8f, 0.12f, 0.05f}, bone * 0.2f, st.glow * (1.f + 2.f * tp));   // its eyes
        r.box(f.torso, {0.f, 2.1f, -1.3f}, {3.6f, 3.8f, 0.2f}, cloth * 1.2f);                         // the robe's back
        r.box(f.torso, {0.f, -0.4f, 0.f}, {4.8f, 1.6f, 2.8f}, cloth);                                  // the skirt
        for (int k = 0; k < 6; ++k)                                                                    // its chains' collar
            r.box(f.torso, {std::cos(k * 1.047f) * 2.0f, 4.0f, std::sin(k * 1.047f) * 1.2f}, {0.45f, 0.45f, 0.45f}, iron * 1.6f);
        // Censers: hang from each hand on a chain, glowing amber (white when raised high, hot when lashing)
        vec3 hot = e.attack == AttackKind::CENSER_HIGH ? vec3{1.6f, 1.6f, 1.5f} : st.glow;
        for (int s = 0; s < 2; ++s) {
            const mat4& arm = s ? f.armL : f.armR;
            float glowAmt = (s == 0 && (e.attack == AttackKind::CENSER_LOW || e.attack == AttackKind::CENSER_HIGH)) ? 1.f + 3.f * tp : 0.8f;
            for (int c = 0; c < 4; ++c) r.box(arm, {0.f, -L.armLen - 0.6f - c * 0.6f, 0.f}, {0.15f, 0.5f, 0.15f}, iron * 1.5f);
            r.box(arm, {0.f, -L.armLen - 3.4f, 0.f}, {1.1f, 1.3f, 1.1f}, brassOf(), hot * glowAmt);
            r.box(arm, {0.f, -L.armLen - 2.7f, 0.f}, {0.7f, 0.25f, 0.7f}, brassOf());
        }
        if (e.scourging) {                                                                             // the wound
            float pulse = 0.6f + 0.4f * std::sin(time * 9.f);
            r.box(f.torso, {0.f, L.torsoH * 0.55f, -L.torsoD * 0.5f - 0.05f}, {2.0f, 2.2f, 0.15f}, vec3{0.3f, 0.02f, 0.02f},
                  vec3{2.2f, 0.25f, 0.1f} * pulse);
        }
        break;
    }
```

   with a helper above `buildEnemy`: `inline vec3 brassOf() { return {0.6f, 0.45f, 0.22f}; }`.
- `headBox`: add `case EnemyType::PENITENT:` to the humanoid case list; inside, `if (e.type == EnemyType::PENITENT) ov = penitentPose(e);` and `if (e.type == EnemyType::PENITENT) { w = 1.9f; top = 1.9f; }` (the hood).
- `woundBox` after `headBox`:

```cpp
// THE PENITENT's wound: its back, open while it scourges itself (x3)
inline bool woundBox(const Enemy& e, AABB& out) {
    if (e.type != EnemyType::PENITENT || !e.scourging) return false;
    HumanoidLook L = humanoidDims(e.type);
    PoseOverride ov = penitentPose(e);
    mat4 root = T(e.position) * RY(e.yaw);
    mat4 torso = root * T({0.f, -ov.crouch + L.legLen + L.pelvisH, 0.f}) * RY(ov.torsoYaw) * RX(ov.torsoPitch);
    vec3 c = vec3(torso * glm::vec4(0.f, L.torsoH * 0.55f, -L.torsoD * 0.5f - 0.2f, 1.f));
    vec3 half{1.3f, 1.4f, 1.3f};
    out = {c - half, c + half};
    return true;
}
```
   and `using rig::woundBox;` at the bottom.

`src/GameplayState.h`: `struct RayHit { int enemy; float t; bool head; bool wound = false; };`

`src/Gameplay_Combat.h`, `hitscanAll`: before the head test, the wound:

```cpp
        AABB wound;
        float tw = woundBox(en, wound) ? rayBoxHit(origin, dir, AABB{wound.min + off, wound.max + off}) : -1.f;
        if (tw > 0.f && tw < wallT) { out.push_back({ei, tw, false, true}); continue; }
```
and in the fire loop: `float m = hits[k].wound ? 3.f : head ? d.headMult : 1.f;` plus `if (hits[k].wound) fx.spawnHitSparks(at, {1.f, 0.3f, 0.15f});`.

- [ ] **Step 4: Run to verify it passes; build**

Run: `make test 2>&1 | grep -E "PENITENT|FAIL|ALL PASSED|FAILED"; make 2>&1 | grep " error"`
Expected: `ALL PASSED`; clean build.

- [ ] **Step 5: Commit**

```bash
git add src/EnemyModel.h src/GameplayState.h src/Gameplay_Combat.h tests/test_game.cpp
git commit -m "The Penitent's body: hood and mask, censers on chains, the wound on its back"
```

---

### Task 7: The fight in the game

**Files:**
- Modify: `src/Gameplay_Penitent.h`, `src/GameplayState.h`, `src/Gameplay_Combat.h`, `src/Gameplay_Tick.h`, `src/Gameplay_Flow.h`, `src/Gameplay_Render.h`, `src/Gameplay_HUD.h`, `src/Gameplay_Dev.h` (overlay), `README.md`

**Interfaces:**
- Consumes: Tasks 2–6.
- Produces: `GameplayState::PenitentHazards pen;`, `float anchorRipTimer = 0.f; int ripAnchor = -1;`, `void onPenitentEvents(Enemy& e, const EnemyEvents& ev)`, `void updatePenitent(float dt)`, `void breakAnchor(int i, bool ripped)`, `bool hitAnchor(glm::vec3 origin, glm::vec3 dir, float wallT, float dmg)`, `void gatherPenitentBoxes(std::vector<BoxInstance>& out)`.

No new unit tests (GameplayState needs GL); verification is the build, the suite, screenshots and an audio dump. Every behaviour wired here was pinned in Tasks 4–5.

- [ ] **Step 1: Events, damage, sounds (`Gameplay_Penitent.h`)**

```cpp
inline void GameplayState::onPenitentEvents(Enemy& e, const EnemyEvents& ev) {
    const Arena& ar = level.arenas[director.arena];
    const float scale = ar.damageScale * tune().damage;
    const float floorY = e.floorY;
    glm::vec3 at = e.position + glm::vec3{0.f, 3.f, 0.f};
    // Tells: a distinct sound for each, from where it kneels; big ones duck the rest
    if (ev.telegraphStarted) {
        switch (e.attack) {
            case AttackKind::CENSER_LOW:  audio.playAt("telegraph", at, 120, SoundGroup::ENEMY, true); break;   // grinding whine
            case AttackKind::CENSER_HIGH: audio.playAt("barrier", at, 128, SoundGroup::ENEMY, true); break;     // a bell tone
            case AttackKind::PSLAM: case AttackKind::PSTOMP: case AttackKind::SCOURGE:
                audio.playAt("telegraph", at, 128, SoundGroup::ENEMY, true); audio.duck(6.f, 0.5f); break;
            default: break;
        }
    }
    if (ev.penSweep >= 0) {
        pen.addSweep(e.position, e.yaw, e.sweepReach(), ev.penSweep);
        audio.playAt("dash", at, 128, SoundGroup::ENEMY, true);
        if (PenitentHazards::sweepHits(ev.penSweep, e.position, e.yaw, e.sweepReach(), player.position, player.height, floorY))
            damagePlayer(PenitentHazards::SWEEP_DAMAGE * scale, e.position, 0.3f, 0.07f);
    }
    if (ev.penSlam)  { pen.addRing(e.position, PenitentHazards::SLAM_RADIUS, PenitentHazards::SLAM_DAMAGE, 0);  audio.playAt("slam", e.position, 128, SoundGroup::ENEMY, true); shake(0.5f, 0.07f); }
    if (ev.penStomp) { pen.addRing(e.position, PenitentHazards::STOMP_RADIUS, PenitentHazards::SLAM_DAMAGE, 0); audio.playAt("slam", e.position, 110, SoundGroup::ENEMY, true); shake(0.3f, 0.05f); }
    if (ev.penEmbers) { pen.addRing(e.position, PenitentHazards::EMBER_RANGE, PenitentHazards::EMBER_DAMAGE, 1); audio.playAt("explosion", at, 100, SoundGroup::ENEMY, true); }
    if (ev.penLash) { pen.addLash(ev.penLashFrom, ev.penLashDir, e.risen); audio.playAt("clank", at, 128, SoundGroup::ENEMY, true); audio.duck(6.f, 0.5f); }
    for (int k = 0; k < ev.penIncense; ++k) pen.addPool(ev.penIncensePos[k]);
    if (ev.penIncense > 0) audio.playAt("skim", player.position, 80, SoundGroup::ENEMY);
    if (ev.penRose) {
        pushBanner("THE PENITENT RISES", "IT WALKS - KEEP MOVING, READ EVERY SWING", {1.f, 0.4f, 0.2f}, 3.f);
        audio.play("wave", 128, SoundGroup::UI); audio.duck(6.f, 0.8f); shake(0.8f, 0.08f);
    }
    for (int k = 0; k < ev.penSummon; ++k) {
        float a = k * 1.5707963f + gameClock;
        glm::vec3 p = e.position + glm::vec3{std::cos(a) * 8.f, 0.f, std::sin(a) * 8.f};
        p.y = groundHeightAt(p.x, p.z, e.position.y + 2.f);
        spawnEnemy(EnemyType::HUSK, p);
        enemies.back().setHollow((Hollow)(1 + k % 3));
    }
}
```

(Match `spawnEnemy`'s real signature and how Hollow variants are applied to spawned enemies elsewhere — copy that code path.)

```cpp
inline void GameplayState::breakAnchor(int i, bool ripped) {
    const glm::vec3 p = level.anchors[i].pos;
    fx.spawnBurst(p, {1.2f, 0.6f, 0.25f}, 40, 9.f, 0.7f, 6.f);
    audio.playAt("clank", p, 128, SoundGroup::ENEMY, true, 0.5f);
    audio.playAt("explosion", p, 90, SoundGroup::WORLD);
    styleSystem.addStyle(ripped ? 40.f : 25.f, ripped ? StyleSource::PARRY : StyleSource::EXPLOSIVE);
    ui.feed(ripped ? "RIPPED" : "CHAIN BROKEN", {1.f, 0.7f, 0.3f});
    if (ripped) grapple.release();
    for (auto& e : enemies) if (e.alive && e.type == EnemyType::PENITENT && !e.risen) { e.stagger(1.f); break; }
}

inline bool GameplayState::hitAnchor(glm::vec3 origin, glm::vec3 dir, float wallT, float dmg) {
    int i = level.anchorAlong(origin, dir, wallT);
    if (i < 0) return false;
    fx.spawnHitSparks(origin + dir * wallT, {1.2f, 0.7f, 0.3f});
    if (level.damageAnchor(i, dmg)) breakAnchor(i, false);
    return true;
}

inline void GameplayState::updatePenitent(float dt) {
    Enemy* boss = nullptr;
    for (auto& e : enemies) if (e.alive && e.type == EnemyType::PENITENT) boss = &e;
    if (boss) boss->anchorsLeft = level.anchorsAlive();
    // Rip: hang on an anchor with the grapple for half a second
    int hooked = -1;
    if (grapple.active)
        for (int i = 0; i < (int)level.anchors.size(); ++i)
            if (level.anchors[i].alive && grapple.wall == level.anchors[i].wall) hooked = i;
    if (hooked >= 0 && hooked == ripAnchor) {
        anchorRipTimer += dt;
        if (anchorRipTimer >= 0.5f && level.damageAnchor(hooked, 1e9f)) { breakAnchor(hooked, true); ripAnchor = -1; anchorRipTimer = 0.f; }
    } else { ripAnchor = hooked; anchorRipTimer = 0.f; }
    // Its hazards
    if (!boss && pen.rings.empty() && pen.lashes.empty() && pen.pools.empty()) return;
    const float scale = level.arenas[director.arena].damageScale * tune().damage;
    float floorY = boss ? boss->floorY : level.lift.y();
    for (const auto& h : pen.update(dt, player.position, floorY)) {
        if (!damagePlayer(h.damage * scale, h.from, 0.25f, 0.05f, h.damage < 10.f ? 0.f : 0.25f)) continue;
        if (h.yank) {
            glm::vec3 in = h.from - player.position; in.y = 0.f;
            if (glm::length(in) > 0.1f) player.velocity += glm::normalize(in) * 16.f + glm::vec3{0.f, 4.f, 0.f};
        }
    }
    // Phase 3's banner, once
    if (boss && boss->scourging && !scourgeAnnounced) {
        scourgeAnnounced = true;
        pushBanner("IT SCOURGES ITSELF", "STRIKE THE WOUND ON ITS BACK", {1.f, 0.25f, 0.15f}, 3.f);
        audio.duck(6.f, 0.6f);
    }
}
```

(Use the `grapple` member that records the hooked wall — `GrappleHook` keeps `moverWall`; add `int wall = -1;` set in `attach()` if it doesn't already store the wall index, and ledger it. Use the real `damagePlayer` parameter list; the 5th argument is the i-frames used elsewhere for hazard ticks.)

- [ ] **Step 2: Hooks**

- `GameplayState.h`: `#include "PenitentHazards.h"`; members `PenitentHazards pen; float anchorRipTimer = 0.f; int ripAnchor = -1; bool scourgeAnnounced = false;`; declarations of the functions above.
- `Gameplay_Combat.h`:
  - after `if (enemies[i].type == EnemyType::SOVEREIGN) onSovereignEvents(...)`: `if (enemies[i].type == EnemyType::PENITENT) onPenitentEvents(enemies[i], ev);`
  - the generic telegraph sound: skip it for the Penitent (`ev.telegraphStarted && enemies[i].type != EnemyType::PENITENT && ...`).
  - `ev.enraged` banner: skip for the PENITENT (its phases have their own).
  - punch / parry: in the enemy loop, the PENITENT parries by being in the blow's path, not by distance:

```cpp
        bool inReach = e.type == EnemyType::PENITENT
            ? (e.attack == AttackKind::CENSER_LOW || e.attack == AttackKind::CENSER_HIGH)
                  ? PenitentHazards::sweepHits(e.attack == AttackKind::CENSER_HIGH, e.position, e.yaw, e.sweepReach(),
                                               player.position, player.height, e.floorY) || PenitentHazards::sweepHits(0, e.position, e.yaw, e.sweepReach(), player.position, 1.8f, e.floorY)
                  : glm::length(glm::vec2{player.position.x - e.position.x, player.position.z - e.position.z}) <
                        (e.attack == AttackKind::PSLAM ? PenitentHazards::SLAM_RADIUS : PenitentHazards::STOMP_RADIUS)
            : (dist < 5.5f && glm::dot(fwd, d / dist) > 0.2f);
        if (inReach) { ... the existing parry body ... }
```
    with the toast for the PENITENT: `"PARRIED"`, `"IT REELS - UNLOAD"`.
  - fire: per ray, after `hitscanAll` and before the enemy loop: `if (n == 0 || hits[0].t > wallT - 0.01f) hitAnchor(origin, dir, wallT, dmg);` (a ray that stops on a wall that's an anchor damages it).
  - `processBlasts`: anchors within a blast's radius take its damage: `for (int ai = 0; ai < (int)level.anchors.size(); ++ai) if (level.anchors[ai].alive && glm::length(level.anchors[ai].pos - b.pos) < b.radius + 1.f && level.damageAnchor(ai, b.damage)) breakAnchor(ai, false);`
- `Gameplay_Tick.h`: after `updateSovereign(dt);`: `updatePenitent(dt);`
- `Gameplay_Flow.h`:
  - `BOSS_START`: `else if (type == EnemyType::PENITENT) pushBanner("THE PENITENT", "BREAK ITS CHAINS - JUMP THE LOW SWEEP, SLIDE UNDER THE HIGH", {1.f, 0.5f, 0.2f}, 4.5f);`
  - `enterArena`: `pen.clear(); scourgeAnnounced = false; ripAnchor = -1; anchorRipTimer = 0.f;` and restore every anchor (`for (auto& a : level.anchors)` — rebuild from a copy captured at level build: keep `std::vector<LevelData::ChainAnchor> anchorsBuilt` and the anchor walls' boxes captured in `start` and restore them here).
  - music intensity: treat the PENITENT like the Sovereign for the boss layers (where the code checks `SOVEREIGN` for raged/boss music, add `|| PENITENT`).
- `Gameplay_HUD.h`: boss bar name `boss->type == EnemyType::SOVEREIGN ? "THE SOVEREIGN" : boss->type == EnemyType::PENITENT ? "THE PENITENT" : "THE WARDEN"`; for the PENITENT pass `enraged = boss->scourging`; while `anchorsLeft > 0` draw a line under the bar: `CHAINS 4/6` style text using the same UI calls as the bar's neighbours.
- `Gameplay_Render.h` (+ `Gameplay_Penitent.h`): call `gatherPenitentBoxes(out)` next to `gatherSovereignBoxes(out)`:

```cpp
inline void GameplayState::gatherPenitentBoxes(std::vector<BoxInstance>& out) {
    using namespace rig;
    float t = gameClock;
    const glm::vec3 ember{1.5f, 0.55f, 0.15f}, blood{1.6f, 0.15f, 0.08f}, amber{1.4f, 0.85f, 0.35f}, white{1.6f, 1.6f, 1.5f};
    Enemy* boss = nullptr;
    for (auto& e : enemies) if (e.alive && e.type == EnemyType::PENITENT) boss = &e;
    // Anchors: a glowing sigil on an iron block; chains from its collar to each
    for (const auto& a : level.anchors) {
        if (!a.alive) continue;
        float pulse = 0.7f + 0.3f * std::sin(t * 3.f + a.pos.x);
        push(out, T(a.pos) * S(glm::vec3{1.8f}), {0.1f, 0.09f, 0.1f}, amber * 0.2f);
        push(out, T(a.pos) * S(glm::vec3{1.0f, 1.0f, 1.9f}), {0.2f, 0.1f, 0.05f}, amber * (1.2f * pulse));
        if (!boss || boss->risen) continue;
        glm::vec3 from = boss->position + glm::vec3{0.f, boss->height() * 0.75f, 0.f};
        glm::vec3 d = a.pos - from; float len = glm::length(d);
        int links = (int)(len / 0.9f);
        float yaw = std::atan2(d.x, d.z), pitch = -std::asin(d.y / std::max(len, 1e-3f));
        for (int k = 0; k < links; ++k) {
            glm::vec3 p = from + d * ((k + 0.5f) / links) - glm::vec3{0.f, std::sin((k + 0.5f) / links * 3.1416f) * len * 0.06f, 0.f};
            push(out, T(p) * RY(yaw) * RX(pitch) * RZ(k % 2 ? 1.5708f : 0.f) * S({0.22f, 0.5f, 0.75f}), {0.16f, 0.15f, 0.16f});
        }
    }
    // Sweep trails: a fan of fading slabs along the arc at its height
    for (const auto& a : pen.arcs) {
        float fade = 1.f - a.t / 0.4f, h = a.kind == 0 ? 0.5f : 2.2f;
        for (int k = -8; k <= 8; ++k) {
            float ang = a.yaw + k * PenitentHazards::HALF_ARC / 8.f;
            glm::vec3 p = a.centre + glm::vec3{std::sin(ang) * a.reach * 0.8f, h, std::cos(ang) * a.reach * 0.8f};
            push(out, T(p) * RY(ang) * S({a.reach * 0.2f, 0.12f, 0.25f}), {0.1f, 0.05f, 0.02f},
                 (a.kind == 0 ? amber : white) * 1.6f * fade);
        }
    }
    // Rings: segments round the expanding circle (embers redder)
    for (const auto& r : pen.rings) {
        const int SEG = 40; float segLen = 6.2832f * std::max(r.radius, 0.5f) / SEG;
        for (int k = 0; k < SEG; ++k) {
            float a = (k + 0.5f) / SEG * 6.2832f;
            push(out, T(r.centre + glm::vec3{std::cos(a) * r.radius, 0.35f, std::sin(a) * r.radius}) * RY(-a) * S({0.2f, 0.7f, segLen * 0.85f}),
                 {0.1f, 0.04f, 0.02f}, (r.kind == 1 ? blood : ember) * 1.5f);
        }
    }
    // The lash: a red line on the floor while it's warned, the chain itself when it strikes
    for (const auto& l : pen.lashes) {
        bool struck = l.t >= PenitentHazards::LASH_WARN;
        float yaw = std::atan2(l.dir.x, l.dir.z);
        glm::vec3 mid = l.from + l.dir * (PenitentHazards::LASH_REACH * 0.5f);
        float pulse = 0.5f + 0.5f * std::sin(t * 30.f);
        push(out, T(glm::vec3{mid.x, l.from.y - 0.25f, mid.z}) * RY(yaw) * S({struck ? 0.6f : 0.25f, struck ? 0.5f : 0.04f, PenitentHazards::LASH_REACH}),
             {0.1f, 0.02f, 0.02f}, blood * (struck ? 2.5f : 0.6f + 1.2f * pulse));
    }
    // Incense: a burning disc (a hint while it lands, then full)
    for (const auto& p : pen.pools) {
        float on = p.t < PenitentHazards::POOL_WARN ? 0.3f : 1.f, fade = std::min(1.f, (PenitentHazards::POOL_TIME - p.t) * 2.f);
        for (int k = 0; k < 6; ++k)
            push(out, T(p.pos + glm::vec3{0.f, 0.04f, 0.f}) * RY(k * 0.5236f) * S({PenitentHazards::POOL_RADIUS * 2.f, 0.05f, 0.9f}),
                 {0.1f, 0.05f, 0.02f}, ember * on * fade * (0.8f + 0.2f * std::sin(t * 6.f + k)));
    }
    // The cage's dressing rides with it: a railing and four chains up into the dark
    if (!level.lift.movers.empty()) {
        float y = level.lift.y();
        const glm::vec3 C{0.f, y, -840.f};
        // Round the cage's outline (the corners of its four boxes): 16 corners,
        // a rail on the 12 short edges; the 4 straight edges across the axes
        // stay open, where the bridges meet the rim
        static const glm::vec2 RIM[16] = {{12.f, 5.f}, {10.5f, 8.f}, {8.f, 10.5f}, {5.f, 12.f}, {-5.f, 12.f}, {-8.f, 10.5f},
                                          {-10.5f, 8.f}, {-12.f, 5.f}, {-12.f, -5.f}, {-10.5f, -8.f}, {-8.f, -10.5f}, {-5.f, -12.f},
                                          {5.f, -12.f}, {8.f, -10.5f}, {10.5f, -8.f}, {12.f, -5.f}};
        for (int k = 0; k < 16; ++k) {
            glm::vec2 a = RIM[k], b = RIM[(k + 1) % 16], d = b - a;
            if (std::fabs(d.x) < 1e-3f || std::fabs(d.y) < 1e-3f) continue;   // an axis edge: a bridge
            glm::vec2 m = (a + b) * 0.5f;
            push(out, T(C + glm::vec3{m.x, 0.6f, m.y}) * RY(std::atan2(d.x, d.y)) * S({0.12f, 1.2f, glm::length(d)}), {0.12f, 0.11f, 0.12f});
        }
        for (int k = 0; k < 4; ++k) {
            float a = k * 1.5708f + 0.7854f;
            glm::vec3 base = C + glm::vec3{std::cos(a) * 12.f, 0.f, std::sin(a) * 12.f};
            push(out, T(base + glm::vec3{0.f, 30.f, 0.f}) * S({0.3f, 60.f, 0.3f}), {0.1f, 0.1f, 0.11f});
        }
    }
}
```

   (The rail skips the four axis edges, so you can walk off the cage onto a bridge.)
- `Gameplay_Dev.h`: `--overlay penitent0..4` freezes the boss for screenshots: 0 kneeling idle, 1 low sweep wind-up, 2 high sweep wind-up, 3 risen, 4 scourging (set `anchorsLeft`/`risen`/`scourging`/`attack`/`telegraphTimer` each tick when the overlay matches, like the Sovereign's `poseN`).
- `README.md`: the Descent (Act II list, Arenas section, the enemy table row for the Penitent with its counter), dev flags (`--act2 --arena 3`, `--spawn 14`, `--overlay penitentN`), the menu "3/4 ARENAS".

- [ ] **Step 3: Build, test**

Run: `make 2>&1 | grep -E " error|warning:" | grep -v duplicate; make test 2>&1 | tail -1`
Expected: clean; `ALL PASSED`.

- [ ] **Step 4: Screenshots**

```bash
for o in 0 1 2 3 4; do ./shooter --act2 --arena 3 --wave 4 --god --res 720 --overlay penitent$o --cam 0 -236 -836 -90 8 --shot 90 $SCRATCH/pen$o.bmp; done
./shooter --act2 --arena 3 --wave 4 --god --res 720 --cam 18 -236 -836 -140 10 --shot 120 $SCRATCH/pit.bmp
```
Convert with `sips` and look at each: the kneeling colossus and chains to six glowing anchors; the low and high wind-ups readable at a glance (censer low/amber vs raised/white); risen at 11.5 m; the glowing wound. Fix what reads poorly.

- [ ] **Step 5: Audio and frame time**

```bash
./shooter --act2 --arena 3 --god --kite --audiodump $SCRATCH/ride.wav 14
./shooter --act2 --arena 3 --wave 4 --god --kite --audiodump $SCRATCH/penitent.wav 20
./shooter --act2 --arena 3 --wave 4 --god --bench 600 --res 720
./shooter --arena 5 --god --bench 600 --res 720
```
Expected: dumps with peak ≤ 1.0 and some panned seconds; the Penitent's bench within ~15 % of the Sanctum's.

- [ ] **Step 6: Web build**

Run: `source ~/emsdk/emsdk_env.sh && make web 2>&1 | tail -2`
Expected: builds.

- [ ] **Step 7: Commit**

```bash
git add src/*.h README.md
git commit -m "The Penitent: its chains, its sweeps and lash, the rise and the scourge, in the pit at the bottom of the Descent"
```

---

## After the last task

Fresh reviewer (most capable model) over the whole branch against the spec, with the Review Focus list; fix pass; then merge to `main`, push, rebuild web, copy `overdrive.js`/`overdrive.wasm`/`overdrive.data` into `Personal-Website/public/overdrive` (the data file changes: new level), update the site's OVERDRIVE page (Act II now 3/4 arenas, the Penitent), verify and push the site — the user asked for both repos to be kept current.
