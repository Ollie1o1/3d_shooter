# Act II Foundation + The Drowned Nave — Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** A playable, unranked ACT II run (unlocked by killing the Sovereign) that opens with a fall into the Drowned Nave, a flooded cathedral whose water slows walking, is skimmed by slides, and rises each wave.

**Architecture:** Act I's builder becomes `buildAct1(LevelBuilder&)`; Act II lives in a new `LevelAct2.h` (`buildAct2`) in its own region of the world (floors at Y −60, Z < −440), so ASCENT can later build both into one `LevelData`. The hard floor at Y 0 becomes a per-region floor (`LevelData::basins`), water is data (`LevelData::water`) read by Player/Enemy each tick, and the flood is a new `ArenaShift::FLOOD`. A new `GameMode::ACT2` picks the level, the head start and the end-of-run screen.

**Tech Stack:** C++17 header-only game (one TU: `src/main.cpp`), SDL2, OpenGL 3.3 / WebGL2 (GLSL 330 rewritten to ES 300 by `ShaderProgram`), Emscripten for web, headless tests in `tests/test_game.cpp` and `tests/test_physics.cpp` (`CHECK(cond, msg)` macro).

**Spec:** `docs/superpowers/specs/2026-10-06-act2-foundation-drowned-nave-design.md`

## Global Constraints

- After every task: `make && make test` pass. Before the final commit: `source ~/emsdk/emsdk_env.sh && make web` passes.
- GLSL must stay ES-compatible: `#version 330 core` files, no features beyond ES 3.00 (`ShaderProgram` swaps the version line and adds precision).
- New headers go in the Makefile `HEADERS` list (and in the `test:` prerequisites if tests include them).
- Act I, FAST, ENDLESS and DAILY must play, score and rank exactly as before.
- Act II floors at **Y −60**, Act II region **Z < −440**. Nave tiers above its floor: nave 0, aisles +1.5, chancel +3, galleries +6, bridges +9, clerestory +12.
- Water: slow −20% at 0.4 m depth, −45% at ≥ 1.0 m, none below 0.1 m. Slides/dashes skim at full speed; a slide begun in water lasts 30% longer. Feet never more than **1.5 m** under the surface (`WADE_MAX`). Enemies on foot slowed by half the player's factor; flyers not slowed.
- Flood (surface above the nave floor): wave 1 **0.4 m**, wave 2 **2.0 m**, wave 3 **2.6 m**; each rise takes ~6 s, banner "THE WATER RISES". A spawn under more than **1.2 m** of water moves to the nearest dry spawn.
- ACT II start: level 6 with 5 unspent points; armory opens before the fall.
- ACT II is unranked: no records, no leaderboard, no name entry. Menu row: locked "DEFEAT THE SOVEREIGN"; unlocked "PREVIEW - 1/4 ARENAS".
- `act2Unlocked` is set by killing the Sovereign in any run without god mode.
- Nave tuning: `maxAlive = 11`, `damageScale = 1.3`.
- Nave water draw cost: within ~15% of the Sanctum in `--bench` at 1080 lines.
- Commits: plain messages, **no Co-Authored-By / Claude trailer** (user rule).
- `rm` is not permitted in this environment; overwrite files instead of deleting.

## Review Focus

1. **Act II code paths that still assume Y 0** (pickups, debris, decals, grapple, slam "grounded" checks, Sovereign hazards): a pickup dropped in the Nave must rest on the Nave floor, not float 60 m up. Pinned in Task 2 (`LevelData::baseFloor` feeds `groundHeightAt`, which pickups and debris already use; player/enemy floor tests). Before finishing Task 9, `grep -rn "0\.f, *0\.f\|y = 0\.f\|y < 0" src/Gameplay_*.h src/Effects.h src/GrappleHook.h src/SovereignHazards.h` and fix any floor-at-zero left.
2. **A retry mid-flood:** dying in wave 3 and retrying must put the water back to 0.4 m and the player back at the top of the shaft. Test pinned in Task 6 (`shifts.reset` restores levels).
3. **Sliding in deep water must not put the camera under the surface** (slide eye height is 0.65 m). Test pinned in Task 5 (sliding cap is surface − 0.3 m).
4. **Act I's spatial grid and collision unchanged** after the grid starts fitting its walls. Test pinned in Task 1 (Act I wall counts and an Act I pad/door test still pass; grid finds a wall at Z −600).
5. **The ACT II menu row while locked** must not start a run (keyboard, mouse or gamepad). Test pinned in Task 7 (MenuState-free check: `canStartAct2(records)` helper).

---

## File Structure

| File | Responsibility | Change |
|---|---|---|
| `src/Player.h` | `SpatialGrid` (fits walls), `Player::floorY`, `Player::wadeDepth`, `wadeFactor()` | Modify |
| `src/Level.h` | `Basin`, `WaterVolume`, `LevelData::{basins, water, baseFloor, lowestFloor, waterSurfaceAt, waterDepthAt, floorWithWater}`, `Arena::floodLevels`, `ArenaShift::FLOOD`; `buildAct1(LevelBuilder&)` + `buildLevel()` wrapper | Modify |
| `src/LevelAct2.h` | `buildAct2(LevelBuilder&)`, `buildAct2Level()`: the Drowned Nave | Create |
| `src/Enemy.h` | `Enemy::floorY`, `Enemy::wadeMul` used in `integrate` | Modify |
| `src/Projectile.h` | `ProjectileSystem::floorY` replaces the `y < 0` kill | Modify |
| `src/ArenaShifts.h` | FLOOD: `onWaveCleared`, water capture/reset/update, `floodStarted` | Modify |
| `src/WaveDirector.h` | Dry-spawn rule in `pickSpawn` | Modify |
| `src/Progression.h` | `Records::act2Unlocked`, `canStartAct2()` | Modify |
| `src/GameState.h` | `GameMode::ACT2` | Modify |
| `src/GameplayState.h` | `act2()`, `act2Falling`, `beginAct2()`, water render members | Modify |
| `src/Gameplay_Flow.h` | level choice, ranked, `beginAct2`, ACT II banners/victory/finish, music | Modify |
| `src/Gameplay_Tick.h` | floors/water per tick, director hold during fall, wade/skim sounds + spray | Modify |
| `src/Gameplay_Combat.h` | enemy floors/wade, Sovereign unlock, splash on enemy fall-in | Modify |
| `src/Gameplay_Render.h` | water pass + warm-up | Modify |
| `src/Gameplay_Shifts.h` | flood banner/rumble hook | Modify |
| `src/Gameplay_HUD.h` | ACT II victory variant, pause line | Modify |
| `src/UIRenderer.h` | `renderVictoryArena` title/sub parameters | Modify |
| `src/MenuState.h` | ACT II row, dev rows | Modify |
| `src/MusicSynth.h` | 6th track "NAVE" | Modify |
| `src/main.cpp` | `--act2`, load `wade`/`skim` sounds | Modify |
| `src/water.vert`, `src/water.frag` | water surface shader | Create |
| `tools/gen_sfx.py` | `gen_wade`, `gen_skim` | Modify |
| `assets/sfx/wade.wav`, `assets/sfx/skim.wav` | generated | Create |
| `Makefile` | `src/LevelAct2.h` in HEADERS and test prereqs | Modify |
| `tests/test_physics.cpp`, `tests/test_game.cpp` | new checks | Modify |

---

### Task 1: The spatial grid fits the level

The grid is fixed to X −240..108, Z −320..40; everything north of Z −320 (the Sanctum, all of Act II) piles into the edge row. Make it size itself to the walls it's built from.

**Files:**
- Modify: `src/Player.h:44-80` (`SpatialGrid`)
- Test: `tests/test_physics.cpp`

**Interfaces:**
- Produces: `SpatialGrid::build(walls)` sizes `X0, Z0, NX, NZ` (now non-static members) from the walls; `query` unchanged.

- [ ] **Step 1: Write the failing test** (append before the final summary in `tests/test_physics.cpp`)

```cpp
    // N. The spatial grid covers walls far from the origin (Act II sits at Z -450..-660)
    {
        std::vector<Wall> walls{ Wall{ AABB{{-2.f, -61.f, -602.f}, {2.f, -59.f, -598.f}} },
                                 Wall{ AABB{{300.f, 0.f, 0.f}, {302.f, 2.f, 2.f}} } };
        SpatialGrid grid; grid.build(walls);
        std::vector<int> out;
        grid.query(AABB{{-1.f, -61.f, -601.f}, {1.f, -59.f, -599.f}}, out);
        bool foundFar = std::find(out.begin(), out.end(), 0) != out.end();
        bool notOther = std::find(out.begin(), out.end(), 1) == out.end();
        CHECK(foundFar && notOther, "the spatial grid finds a wall at Z -600 and only nearby walls");
        Player p({0.f, -59.f + 0.5f, -600.f});
        p.floorY = -1000.f;
        for (int i = 0; i < 60; ++i) p.update(DT, keys, walls.data(), (int)walls.size(), false, &grid);
        CHECK(std::fabs(p.position.y - (-59.f)) < 0.01f && p.onGround, "the player stands on a wall top far from the origin");
    }
```

Add `#include <algorithm>` and `#include <cmath>` to the test's includes if missing. (`p.floorY` comes from Task 2; until then the test fails to compile, which counts as failing.)

- [ ] **Step 2: Run to verify it fails**

Run: `make test`
Expected: compile error (`floorY` not a member) — expected until Task 2; the grid half is verified after Step 3 plus Task 2.

- [ ] **Step 3: Implement**

Replace the `SpatialGrid` struct's constants and storage:

```cpp
struct SpatialGrid {
    static constexpr float CELL = 12.f;
    // Fitted to the walls in build(): one cell of margin all round
    float X0 = 0.f, Z0 = 0.f;
    int   NX = 1, NZ = 1;

    std::vector<std::vector<int>> cells{1};

    void build(const std::vector<Wall>& walls) {
        float x0 = 1e9f, z0 = 1e9f, x1 = -1e9f, z1 = -1e9f;
        for (auto& w : walls) {
            if (w.dynamic) continue;
            x0 = std::min(x0, w.box.min.x); z0 = std::min(z0, w.box.min.z);
            x1 = std::max(x1, w.box.max.x); z1 = std::max(z1, w.box.max.z);
        }
        if (x0 > x1) { x0 = z0 = -CELL; x1 = z1 = CELL; }
        X0 = x0 - CELL; Z0 = z0 - CELL;
        NX = (int)((x1 - X0) / CELL) + 2;
        NZ = (int)((z1 - Z0) / CELL) + 2;
        cells.assign((size_t)NX * NZ, {});
        for (int i = 0; i < (int)walls.size(); ++i)
            if (!walls[i].dynamic) insertWall(i, walls[i].box);
    }
```

Keep `query`, `cx`, `cz`, `cell`, `insertWall` as they are (they already index `cells[z * NX + x]` and clamp with `NX-1`/`NZ-1`, which now read the members).

- [ ] **Step 4: Run** `make && make test` after Task 2 Step 3 lands (the test needs `floorY`). Expected: all existing checks still `ok:` (Act I pads, doors, movers, FAST lift), new grid check `ok:`.

- [ ] **Step 5: Commit** (together with Task 2, since the test spans both)

---

### Task 2: Floors that aren't at Y 0

**Files:**
- Modify: `src/Player.h` (`FLOOR_Y` → `floorY`), `src/Level.h` (`Basin`, `LevelData::basins/baseFloor/lowestFloor`), `src/Enemy.h:1279` (`floorY`), `src/Projectile.h:131` (`floorY`), `src/Gameplay_Tick.h` (player floor, `groundHeightAt`, projectile floor), `src/Gameplay_Combat.h` (enemy floor)
- Test: `tests/test_physics.cpp`, `tests/test_game.cpp`

**Interfaces:**
- Produces:
  - `float Player::floorY = 0.f;` — the hard floor under the player, set by the caller before `update()`.
  - `float Enemy::floorY = 0.f;` — same for an enemy, set before `Enemy::update()`.
  - `float ProjectileSystem::floorY = 0.f;` — shots below it die.
  - `struct Basin { AABB xz; float y; };` `std::vector<Basin> LevelData::basins;`
  - `float LevelData::baseFloor(float x, float z) const;` (0 outside every basin)
  - `float LevelData::lowestFloor() const;`

- [ ] **Step 1: Write the failing tests**

`tests/test_physics.cpp` (new block):

```cpp
    // N+1. The hard floor follows floorY (Act II's floors are at Y -60)
    {
        Player p({0.f, -50.f, 0.f});
        p.floorY = -60.f;
        for (int i = 0; i < 180; ++i) p.update(DT, keys, nullptr, 0);
        CHECK(std::fabs(p.position.y + 60.f) < 0.001f && p.onGround, "the player falls to floorY and stands there");
    }
```

`tests/test_game.cpp` (new block after the arena-map block):

```cpp
    // ---------------------------------------------------------------- floors below Y 0
    {
        LevelData B;
        B.basins.push_back({LevelBuilder::aabb(-10, 0, -10, 10, 0, 10), -60.f});
        CHECK(B.baseFloor(0.f, 0.f) == -60.f && B.baseFloor(50.f, 0.f) == 0.f && B.lowestFloor() == -60.f,
              "a basin lowers the floor inside it and nowhere else");
        Enemy e(EnemyType::HUSK, {0.f, -55.f, 0.f});
        e.floorY = -60.f;
        EnemyWorld w;
        for (int i = 0; i < 120; ++i) e.integrate(DT, w);
        CHECK(std::fabs(e.position.y + 60.f) < 0.001f && e.grounded, "an enemy lands on its floorY");
        ProjectileSystem ps; ps.floorY = -60.f;
        ps.fire({0.f, -50.f, 0.f}, {0.f, -1.f, 0.f}, 10.f, false, {1, 1, 1}, false, 0.f, 0.2f, false);
        auto r = ps.update(0.5f, nullptr, 0);
        bool alive = false; for (auto& p : ps.pool) alive |= p.alive;
        CHECK(alive, "a shot below Y 0 but above floorY keeps flying");
    }
```

Check `Enemy`'s constructor and `ProjectileSystem::fire/update` signatures in `src/Enemy.h` / `src/Projectile.h` before running; match the argument lists exactly (the `fire` call above mirrors `Gameplay_Combat.h:151`; `update`'s trailing parameters may have defaults — pass what the header requires, using the call at `Gameplay_Tick.h:252` as the reference).

- [ ] **Step 2: Run** `make test` — Expected: compile errors (`floorY`, `basins` missing).

- [ ] **Step 3: Implement**

`src/Player.h`: delete `static constexpr float FLOOR_Y = 0.0f;` and add next to `groundWall`:

```cpp
    // The hard floor under the player: Y 0 in Act I, lower in a basin (Act II),
    // raised by deep water (feet are held WADE_MAX under the surface). Set by
    // the caller before update().
    float floorY = 0.f;
```

Replace every `FLOOR_Y` in `resolveCollisions` with `floorY` (three uses: `position.y < floorY`, `position.y = floorY`, `position.y <= floorY + 0.001f`). Run `grep -rn FLOOR_Y src tests` — expect no hits.

`src/Level.h`, after `struct FloorPatch`:

```cpp
// Ground that isn't at Y 0: inside `xz` (its X and Z only) the hard floor is
// at `y`. Act II sits in one, 60 m under Act I.
struct Basin { AABB xz; float y; };
```

In `LevelData`, after `hazards`:

```cpp
    std::vector<Basin>      basins;
```

and with the other `LevelData` helpers:

```cpp
    // The hard floor at (x, z): a basin's, else Y 0
    float baseFloor(float x, float z) const {
        for (auto& b : basins)
            if (x >= b.xz.min.x && x <= b.xz.max.x && z >= b.xz.min.z && z <= b.xz.max.z) return b.y;
        return 0.f;
    }
    float lowestFloor() const {
        float y = 0.f;
        for (auto& b : basins) y = std::min(y, b.y);
        return y;
    }
```

`src/Enemy.h`: add to `Enemy`'s members (near `position`):

```cpp
    float floorY = 0.f;   // hard floor under it (set by GameplayState each tick)
```

and in `integrate` replace the line at 1279:

```cpp
        if (position.y < floorY) { position.y = floorY; if (velocity.y < 0.f) velocity.y = 0.f; grounded = true; }
```

`src/Projectile.h`: add `float floorY = 0.f;   // shots below this hit the ground` to `ProjectileSystem`, and change line 131 to `p.position.y < floorY - 0.5f`.

`src/Gameplay_Tick.h`, before `player.update(...)` (line ~135):

```cpp
    player.floorY = level.baseFloor(player.position.x, player.position.z);
```

`groundHeightAt` (line 425):

```cpp
inline float GameplayState::groundHeightAt(float x, float z, float fromY) const {
    static std::vector<int> cands;
    float base = level.baseFloor(x, z);
    AABB q{{x - 0.05f, base - 1.f, z - 0.05f}, {x + 0.05f, fromY + 0.5f, z + 0.05f}};
    spatialGrid.query(q, cands);
    float best = base;
    for (int i : cands) {
        const AABB& b = level.walls[i].box;
        if (x >= b.min.x && x <= b.max.x && z >= b.min.z && z <= b.max.z && b.max.y <= fromY + 0.5f)
            best = std::max(best, b.max.y);
    }
    return best;
}
```

Before `projSystem.update(...)` (line ~252): `projSystem.floorY = level.lowestFloor();`

`src/Gameplay_Combat.h`, in `updateEnemies` right before `e.update(dt, w);`:

```cpp
        e.floorY = level.baseFloor(e.position.x, e.position.z);
```

- [ ] **Step 4: Run** `make && make test` — Expected: all `ok:`, including Task 1's grid checks and the three new floor checks.

- [ ] **Step 5: Commit**

```bash
git add src/Player.h src/Level.h src/Enemy.h src/Projectile.h src/Gameplay_Tick.h src/Gameplay_Combat.h tests/test_physics.cpp tests/test_game.cpp
git commit -m "Floors below Y 0: basins, a grid that fits the level"
```

---

### Task 3: Act I's builder as `buildAct1`

**Files:**
- Modify: `src/Level.h:563-1283`
- Test: `tests/test_game.cpp`

**Interfaces:**
- Produces: `inline void buildAct1(LevelBuilder& B);` and `inline LevelData buildLevel();` (wrapper, unchanged behaviour).

- [ ] **Step 1: Capture the counts before refactoring**

Create `/private/tmp/claude-501/-Users-ollie-Desktop-3d-shooter/fe78f86e-b521-4920-97a7-267683804fa3/scratchpad/counts.cpp`:

```cpp
#include "../../../../../../../Users/ollie/Projects/overdrive/3d_shooter/src/Level.h"
#include <cstdio>
int main() {
    LevelData L = buildLevel();
    std::printf("walls %zu props %zu neon %zu shapes %zu floors %zu doors %zu pads %zu arenas %zu movers %zu\n",
                L.walls.size(), L.props.size(), L.neon.size(), L.shapes.size(), L.floors.size(),
                L.doors.size(), L.pads.size(), L.arenas.size(), L.movers.size());
}
```

Build it with the same flags as `make test` (copy the `$(CXX) $(CXXFLAGS) ... $(STDLIB)` line from `make -n test`, pointing at this file; use an absolute include path `#include "/Users/ollie/Projects/overdrive/3d_shooter/src/Level.h"` instead of the relative one). Run it and note the nine numbers.

- [ ] **Step 2: Write the test** (top of the arena-map block in `tests/test_game.cpp`), with the numbers from Step 1:

```cpp
        // Act I as built before the Act II split: nothing added, nothing lost
        CHECK(L.walls.size() == W_ && L.props.size() == P_ && L.neon.size() == N_ && L.shapes.size() == S_ &&
              L.floors.size() == FL_ && L.doors.size() == D_ && L.pads.size() == PD_ && L.movers.size() == M_,
              "buildAct1 builds exactly what buildLevel did");
```

(replace `W_`…`M_` with the literal counts). It passes now; it's the guard for Step 3.

- [ ] **Step 3: Implement the split**

In `src/Level.h` change

```cpp
inline LevelData buildLevel() {
    using glm::vec3;
    LevelData L;
    LevelBuilder B{L};
```

to

```cpp
inline void buildAct1(LevelBuilder& B) {
    using glm::vec3;
    LevelData& L = B.L;
```

replace the final `return L;` of that function with nothing, and add after it:

```cpp
// ARENA mode's level: Act I alone
inline LevelData buildLevel() {
    LevelData L;
    LevelBuilder B{L};
    buildAct1(B);
    return L;
}
```

Update the banner comment above it to `// buildAct1() — ARENA mode (Act I)`.

- [ ] **Step 4: Run** `make && make test` — Expected: all `ok:` including "buildAct1 builds exactly what buildLevel did".

- [ ] **Step 5: Commit**

```bash
git add src/Level.h tests/test_game.cpp
git commit -m "Act I's level built by buildAct1"
```

---

### Task 4: Water data and wading

**Files:**
- Modify: `src/Level.h` (`WaterVolume`, `LevelData::water` + helpers), `src/Player.h` (`wadeDepth`, `wadeFactor`, slide), `src/Enemy.h` (`wadeMul`), `src/Gameplay_Tick.h`, `src/Gameplay_Combat.h`
- Test: `tests/test_physics.cpp`, `tests/test_game.cpp`

**Interfaces:**
- Consumes: `Player::floorY`, `Enemy::floorY`, `LevelData::baseFloor` (Task 2).
- Produces:
  - `struct WaterVolume { AABB box; float level; };` (`box`: XZ extent, `box.min.y` the basin floor; `level`: surface Y)
  - `LevelData::water`, `float waterSurfaceAt(float x, float z) const` (−1e9 if dry), `float waterDepthAt(glm::vec3 p) const`, `float floorWithWater(float x, float z, bool skimming) const`, `static constexpr float WADE_MAX = 1.5f, SKIM_DEPTH = 0.3f;`
  - `float Player::wadeDepth = 0.f;` `static float Player::wadeFactor(float depth);`
  - `float Enemy::wadeMul = 1.f;`

- [ ] **Step 1: Write the failing tests**

`tests/test_physics.cpp`:

```cpp
    // N+2. Wading: slower on foot, full speed when sliding
    {
        auto runFor = [&](float depth, bool slide) {
            Player p({0.f, 0.f, 0.f});
            p.wadeDepth = depth;
            Uint8 k[SDL_NUM_SCANCODES]; std::memset(k, 0, sizeof(k));
            k[SDL_SCANCODE_W] = 1;
            for (int i = 0; i < 60; ++i) p.update(DT, k, nullptr, 0);
            float top = 0.f;
            if (slide) { k[SDL_SCANCODE_LCTRL] = 1; for (int i = 0; i < 6; ++i) { p.update(DT, k, nullptr, 0); top = std::max(top, glm::length(glm::vec2(p.velocity.x, p.velocity.z))); } }
            else top = glm::length(glm::vec2(p.velocity.x, p.velocity.z));
            return top;
        };
        float dry = runFor(0.f, false), ankle = runFor(0.4f, false), deep = runFor(1.2f, false);
        std::printf("      walk speed dry %.2f, 0.4 m %.2f, 1.2 m %.2f\n", dry, ankle, deep);
        CHECK(std::fabs(ankle / dry - 0.8f) < 0.03f && std::fabs(deep / dry - 0.55f) < 0.03f,
              "wading slows walking: -20% ankle-deep, -45% waist-deep and deeper");
        CHECK(deep > 3.f, "even the deepest water leaves you able to move");
        CHECK(runFor(1.2f, true) >= Player::SLIDE_SPEED - 0.01f, "a slide skims deep water at full slide speed");
        CHECK(Player::wadeFactor(0.05f) == 1.f, "a puddle doesn't slow you");
    }
```

`tests/test_game.cpp` (new block):

```cpp
    // ---------------------------------------------------------------- water
    {
        LevelData W;
        W.basins.push_back({LevelBuilder::aabb(-20, 0, -20, 20, 0, 20), -60.f});
        W.water.push_back({LevelBuilder::aabb(-10, -60, -10, 10, -60, 10), -57.4f});   // 2.6 m deep
        CHECK(std::fabs(W.waterDepthAt({0.f, -60.f, 0.f}) - 2.6f) < 1e-4f && W.waterDepthAt({15.f, -60.f, 0.f}) == 0.f,
              "water depth inside a volume, none outside");
        CHECK(std::fabs(W.floorWithWater(0.f, 0.f, false) - (-57.4f - LevelData::WADE_MAX)) < 1e-4f,
              "deep water holds your feet 1.5 m under the surface");
        CHECK(std::fabs(W.floorWithWater(0.f, 0.f, true) - (-57.4f - LevelData::SKIM_DEPTH)) < 1e-4f &&
              -57.4f - LevelData::SKIM_DEPTH + Player::SLIDE_EYE_H > -57.4f,
              "a slide planes on top: the camera stays above the surface");
        Player p({0.f, -50.f, 0.f});
        for (int i = 0; i < 240; ++i) { p.floorY = W.floorWithWater(p.position.x, p.position.z, p.sliding); p.update(DT, keys, nullptr, 0); }
        CHECK(p.position.y + p.eyeHeight > -57.4f && p.onGround, "dropped into deep water, you wade with your eyes above it");
    }
```

- [ ] **Step 2: Run** `make test` — Expected: compile errors (`wadeDepth`, `water`, …).

- [ ] **Step 3: Implement**

`src/Level.h`, after `Basin`:

```cpp
// Standing water: over `box` (X and Z; box.min.y is the floor it fills from)
// the surface is at `level`. ArenaShifts raises it (FLOOD).
struct WaterVolume { AABB box; float level; };
```

In `LevelData`: `std::vector<WaterVolume> water;` and the helpers:

```cpp
    static constexpr float WADE_MAX   = 1.5f;   // feet never deeper than this under the surface
    static constexpr float SKIM_DEPTH = 0.3f;   // a slide planes this far under it
    float waterSurfaceAt(float x, float z) const {
        float s = -1e9f;
        for (auto& w : water)
            if (x >= w.box.min.x && x <= w.box.max.x && z >= w.box.min.z && z <= w.box.max.z) s = std::max(s, w.level);
        return s;
    }
    float waterDepthAt(glm::vec3 p) const { return std::max(0.f, waterSurfaceAt(p.x, p.z) - p.y); }
    // The hard floor with deep water's lift on top
    float floorWithWater(float x, float z, bool skimming) const {
        return std::max(baseFloor(x, z), waterSurfaceAt(x, z) - (skimming ? SKIM_DEPTH : WADE_MAX));
    }
```

`src/Player.h`, members after `floorY`:

```cpp
    // Water above the feet (set by the caller): walking slows, sliding doesn't
    float wadeDepth = 0.f;
    // Ground speed multiplier for a wading depth: none under 0.1 m, -20% at
    // 0.4 m, -45% from 1 m down
    static float wadeFactor(float d) {
        if (d < 0.1f) return 1.f;
        if (d < 0.4f) return 1.f - 0.2f * (d - 0.1f) / 0.3f;
        return 0.8f - 0.25f * glm::clamp((d - 0.4f) / 0.6f, 0.f, 1.f);
    }
```

In `handleMovement`: the slide's trigger speed follows the water (or you could never slide out of deep water):

```cpp
            if (crouchKey && onGround && flatSpeed > horizontalSpeed * wadeFactor(wadeDepth) * 0.6f) {
```

and in the slide start branch use

```cpp
                slideTimer = SLIDE_DURATION * (wadeDepth >= 0.1f ? 1.3f : 1.f);   // water carries a slide further
```

and in the WASD block change the cap:

```cpp
                float cap = horizontalSpeed * (onGround ? wadeFactor(wadeDepth) : 1.f);
                float addSpeed = glm::clamp(cap - currentSpeed, 0.f, accel * dt);
```

Also make ground friction pull a wading walker down to the cap: after the friction block add

```cpp
        // Wading: running momentum bleeds off to the water's pace (slides keep theirs)
        if (onGround && !sliding && !grappling && wadeDepth >= 0.1f) {
            float cap = horizontalSpeed * wadeFactor(wadeDepth);
            glm::vec2 h{velocity.x, velocity.z};
            float s = glm::length(h);
            if (s > cap) { float k = std::max(cap, s - 30.f * dt) / s; velocity.x *= k; velocity.z *= k; }
        }
```

`src/Enemy.h`: member `float wadeMul = 1.f;   // wading slows its steps (set each tick)`; in `integrate` replace `position += velocity * dt;` with

```cpp
        float slow = flying ? 1.f : wadeMul;
        position += glm::vec3{velocity.x * slow, velocity.y, velocity.z * slow} * dt;
```

`src/Gameplay_Tick.h`, replace the Task 2 line before `player.update`:

```cpp
    {
        float surf = level.waterSurfaceAt(player.position.x, player.position.z);
        player.wadeDepth = std::max(0.f, surf - player.position.y);
        player.floorY = level.floorWithWater(player.position.x, player.position.z, player.sliding);
    }
```

`src/Gameplay_Combat.h`, replace the Task 2 line before `e.update`:

```cpp
        e.floorY = level.floorWithWater(e.position.x, e.position.z, false);
        e.wadeMul = e.stats().flying ? 1.f : 1.f - 0.5f * (1.f - Player::wadeFactor(level.waterDepthAt(e.position)));
```

- [ ] **Step 4: Run** `make && make test` — Expected: all `ok:`; the walk-speed line prints ~7.0 / 5.6 / 3.85.

- [ ] **Step 5: Commit**

```bash
git add src/Level.h src/Player.h src/Enemy.h src/Gameplay_Tick.h src/Gameplay_Combat.h tests/test_physics.cpp tests/test_game.cpp
git commit -m "Water: wading slows you, slides skim, deep water holds you up"
```

---

### Task 5: The flood shift and dry spawns

**Files:**
- Modify: `src/Level.h` (`ArenaShift::FLOOD`, `Arena::floodLevels`), `src/ArenaShifts.h`, `src/WaveDirector.h` (`pickSpawn`), `src/Gameplay_Flow.h` (WAVE_CLEARED hook), `src/Gameplay_Shifts.h` (banner)
- Test: `tests/test_game.cpp`

**Interfaces:**
- Consumes: `LevelData::water`, `waterDepthAt` (Task 4).
- Produces:
  - `ArenaShift::FLOOD`; `std::vector<float> Arena::floodLevels;` (surface Y for each wave)
  - `void ArenaShifts::onWaveCleared(LevelData& L, int a, int nextWave);` `bool ArenaShifts::floodStarted;` `static constexpr float FLOOD_TIME = 6.f;`
  - `static constexpr float WaveDirector::DRY_DEPTH = 1.2f;`

- [ ] **Step 1: Write the failing test**

```cpp
    // ---------------------------------------------------------------- the flood
    {
        LevelData Fd;
        LevelBuilder FB{Fd};
        Arena a; a.name = "TANK"; a.subtitle = "";
        a.bounds = LevelBuilder::aabb(-20, -60, -20, 20, -30, 20);
        a.zone = a.bounds;
        a.shift = ArenaShift::FLOOD;
        a.floodLevels = {-59.6f, -58.f, -57.4f};
        a.groundSpawns = {{-15.f, -60.f, 0.f}, {15.f, -57.f, 0.f}};   // a low one and one on a 3 m ledge
        a.waves = {{{EnemyType::HUSK, 4}}, {{EnemyType::HUSK, 4}}, {{EnemyType::HUSK, 4}}};
        Fd.arenas.push_back(a);
        Fd.water.push_back({LevelBuilder::aabb(-20, -60, -20, 20, -60, 20), -59.6f});
        ArenaShifts sh; sh.capture(Fd);
        sh.onWaveCleared(Fd, 0, 2);
        bool started = false;
        for (int i = 0; i < 60 * 7; ++i) { sh.update(DT, Fd, 0, true); started |= sh.floodStarted; }
        CHECK(started && std::fabs(Fd.water[0].level - (-57.4f)) < 1e-3f, "the water rises to the next wave's level within ~6 s");
        WaveDirector d; d.level = &Fd; d.startArena(0); d.wave = 2;
        bool allDry = true;
        for (int k = 0; k < 20; ++k) { glm::vec3 s = d.pickSpawnForTest(EnemyType::HUSK, {0.f, -57.f, 0.f}); allDry &= Fd.waterDepthAt(s) <= WaveDirector::DRY_DEPTH; }
        CHECK(allDry, "nothing spawns under more than 1.2 m of water");
        sh.reset(Fd);
        CHECK(std::fabs(Fd.water[0].level - (-59.6f)) < 1e-4f, "a retry drains the flood back to how it was built");
    }
```

- [ ] **Step 2: Run** `make test` — Expected: compile errors (`FLOOD`, `floodLevels`, `onWaveCleared`, `pickSpawnForTest`).

- [ ] **Step 3: Implement**

`src/Level.h`: `enum class ArenaShift { NONE, NIGHTFALL, LAVA_RISE, SPEED_UP, OVERLOAD, FLOOD };` and in `Arena` after `shift`:

```cpp
    std::vector<float> floodLevels;   // FLOOD: the water's surface (Y) for each wave
```

`src/ArenaShifts.h`: add to the header comment `//   FLOOD     (the Nave)     the water rises a step each wave (Arena::floodLevels)`. Members:

```cpp
    static constexpr float FLOOD_TIME   = 6.f;    // s for one rise
    bool  floodStarted = false;      // this update: the water began to rise (for its banner and rumble)
```

private: `std::vector<float> baseWater, waterTarget, waterRate;`

In `capture`: 

```cpp
        baseWater.clear(); for (auto& w : L.water) baseWater.push_back(w.level);
        waterTarget = baseWater; waterRate.assign(baseWater.size(), 0.f);
```

In `reset`: 

```cpp
        for (int i = 0; i < (int)L.water.size(); ++i) { L.water[i].level = waterTarget[i] = baseWater[i]; waterRate[i] = 0.f; }
```

New method:

```cpp
    // A wave was cleared in arena a: a FLOOD arena's water heads for the next
    // wave's level
    void onWaveCleared(LevelData& L, int a, int nextWave) {
        const Arena& ar = L.arenas[a];
        if (ar.shift != ArenaShift::FLOOD || nextWave >= (int)ar.floodLevels.size()) return;
        for (int i = 0; i < (int)L.water.size(); ++i)
            if (inArena(ar, L.water[i].box)) {
                waterTarget[i] = ar.floodLevels[nextWave];
                waterRate[i] = std::fabs(waterTarget[i] - L.water[i].level) / FLOOD_TIME;
            }
    }
```

In `update`, set `floodStarted = false;` alongside `pulseFired = lavaStarted = false;` and after the arena loop:

```cpp
        for (int i = 0; i < (int)L.water.size(); ++i) {
            if (L.water[i].level == waterTarget[i]) continue;
            if (std::fabs(L.water[i].level - baseWaterAtRiseStart(i)) < 1e-6f) floodStarted = true;
            L.water[i].level = approach(L.water[i].level, waterTarget[i], waterRate[i] * dt);
        }
```

Simpler and exact: track a `std::vector<bool> rising` (assigned in `capture`, cleared in `reset`): in `onWaveCleared` set `rising[i] = false`; in `update`, `if (!rising[i]) { rising[i] = true; floodStarted = true; }` before the `approach`, and `if (L.water[i].level == waterTarget[i]) rising[i] = false;` after. Use this version (drop `baseWaterAtRiseStart`).

`src/WaveDirector.h`: `static constexpr float DRY_DEPTH = 1.2f;   // spawns under more water than this move to dry ones`. In `pickSpawn`, for the ground list only (`!flying`), filter the candidates:

```cpp
        auto wet = [&](const glm::vec3& p) { return !flying && level->waterDepthAt(p) > DRY_DEPTH; };
```

and in both loops skip `wet(pts[i])` (`if (wet(pts[i])) continue;` at the top of each loop body). If every point is wet, fall back to the existing behaviour (farthest point) so a spawn still happens. Add for tests:

```cpp
    glm::vec3 pickSpawnForTest(EnemyType t, glm::vec3 player) { return pickSpawn(t, player); }
```

`src/Gameplay_Flow.h`, `case DirectorEvent::WAVE_CLEARED:` add `shifts.onWaveCleared(level, director.arena, ev.value + 1);`.

`src/Gameplay_Shifts.h`: where `shifts.lavaStarted` is turned into a banner/sound (grep `lavaStarted` in `src/Gameplay_*.h`), add the same for `floodStarted`:

```cpp
    if (shifts.floodStarted) {
        pushBanner("THE WATER RISES", "GET TO HIGH GROUND - OR SLIDE", {0.35f, 0.9f, 0.95f}, 2.6f);
        audio.play("explosion", 45);
        shake(0.5f, 0.03f);
    }
```

- [ ] **Step 4: Run** `make && make test` — Expected: all `ok:`.

- [ ] **Step 5: Commit**

```bash
git add src/Level.h src/ArenaShifts.h src/WaveDirector.h src/Gameplay_Flow.h src/Gameplay_Shifts.h tests/test_game.cpp
git commit -m "The flood: water that rises each wave, and spawns that stay dry"
```

---

### Task 6: The Drowned Nave

**Files:**
- Create: `src/LevelAct2.h`
- Modify: `Makefile` (HEADERS, test prereqs), `tests/test_game.cpp` (`padsLand` floor; Nave checks)

**Interfaces:**
- Consumes: `Basin`, `WaterVolume`, `ArenaShift::FLOOD`, `Arena::floodLevels`, `LevelBuilder`.
- Produces: `inline void buildAct2(LevelBuilder& B);` `inline LevelData buildAct2Level();` and `LevelData::finishPos` set to the end of the sealed passage `(0, F+3, -659)`.

Coordinates (F = −60): narthex + shaft X ±12, Z −450..−472; nave X ±16 with aisles to ±29, Z −473..−592; crossing X ±16 and transepts to ±50, Z −592..−622; chancel/apse X ±16, Z −622..−652; sealed passage X ±3, Z −653..−662.

- [ ] **Step 1: Write the failing tests**

In `tests/test_game.cpp`, make `padsLand` honour basins: in it, after `Player p(pad.centre);` add `p.floorY = L.baseFloor(pad.centre.x, pad.centre.z);` and inside its tick loop before `p.update(...)` add `p.floorY = L.baseFloor(p.position.x, p.position.z);`. Add `#include "../src/LevelAct2.h"` to the includes. New block:

```cpp
    // ---------------------------------------------------------------- ACT II: the Drowned Nave
    {
        LevelData N = buildAct2Level();
        SpatialGrid ng; ng.build(N.walls);
        CHECK(N.arenas.size() == 1 && std::string(N.arenas[0].name) == "THE DROWNED NAVE", "act II opens with the Drowned Nave");
        const Arena& nave = N.arenas[0];
        bool groundOk = true, airOk = true, inB = true, under = true;
        for (auto& s : allGround(nave)) {
            if (overlapsWall(N, boxAt(s, statsOf(EnemyType::JUGGERNAUT).radius, statsOf(EnemyType::JUGGERNAUT).height))) {
                std::printf("      nave ground spawn (%.1f %.1f %.1f) in a wall\n", s.x, s.y, s.z); groundOk = false; }
            if (!inside(nave.bounds, s)) inB = false;
            if (s.y + 3.f > nave.zone.max.y) under = false;
        }
        for (auto& s : nave.airSpawns)
            if (overlapsWall(N, boxAt(s, statsOf(EnemyType::RAPTOR).radius, statsOf(EnemyType::RAPTOR).height))) airOk = false;
        CHECK(groundOk && airOk && inB && under, "the Nave's spawns are clear of walls, inside it and under its ceiling");
        for (auto& g : nave.goals) for (auto& p : g.points)
            if (overlapsWall(N, boxAt(p, statsOf(EnemyType::CONDUIT).radius, statsOf(EnemyType::CONDUIT).height))) groundOk = false;
        CHECK(groundOk, "the gallery conduits stand clear of walls");
        CHECK(padsLand(N, ng, keys), "every Nave jump pad lands you on something higher");
        CHECK(nave.exitDoor >= 0 && N.doors[nave.exitDoor].locked, "the way on starts sealed");
        CHECK(nave.waves.size() == 3 && nave.goals.size() == 3 && nave.goals[1].kind == WaveGoal::CONDUITS &&
              nave.shift == ArenaShift::FLOOD && nave.floodLevels.size() == 3 && nave.maxAlive == 11 &&
              std::fabs(nave.damageScale - 1.3f) < 1e-4f, "three waves, conduits in the second, a flood, 11 at once, x1.3 damage");
        const Arena& sanctum = L.arenas.back();
        CHECK((nave.zone.max.x - nave.zone.min.x) * (nave.zone.max.z - nave.zone.min.z) >
              (sanctum.zone.max.x - sanctum.zone.min.x) * (sanctum.zone.max.z - sanctum.zone.min.z),
              "the Nave is bigger than the Sanctum");
        bool apart = true;
        for (auto& a1 : L.arenas) if (a1.zone.min.z < nave.zone.max.z) apart = false;
        CHECK(apart, "Act II never overlaps Act I (ASCENT can build both)");
        // The fall: from the top of the shaft to the narthex floor
        Player p(nave.playerStart);
        int t = 0;
        for (; t < 60 * 6 && !(p.onGround && t > 5); ++t) {
            p.floorY = N.floorWithWater(p.position.x, p.position.z, false);
            p.update(DT, keys, N.walls.data(), (int)N.walls.size(), false, &ng);
        }
        std::printf("      fell %.1f m in %.2f s\n", nave.playerStart.y - p.position.y, t * DT);
        CHECK(p.onGround && p.position.y < -58.f && p.position.z > -472.f && t < 60 * 4,
              "the fall lands you in the narthex within 4 s");
        // Every flood level: the effective spawns stay wadeable
        LevelData Nf = buildAct2Level();
        bool dryOk = true;
        for (int w = 0; w < 3; ++w) {
            Nf.water[0].level = Nf.arenas[0].floodLevels[w];
            WaveDirector d; d.level = &Nf; d.startArena(0); d.wave = w;
            for (int k = 0; k < 30; ++k) {
                glm::vec3 s = d.pickSpawnForTest(EnemyType::HUSK, Nf.arenas[0].playerStart);
                if (Nf.waterDepthAt(s) > WaveDirector::DRY_DEPTH) dryOk = false;
            }
        }
        CHECK(dryOk, "at every flood level the Nave's spawns are out of deep water");
    }
```

- [ ] **Step 2: Run** `make test` — Expected: compile error (`LevelAct2.h` missing).

- [ ] **Step 3: Implement `src/LevelAct2.h`**

```cpp
#pragma once
// =============================================================================
// LevelAct2.h — ACT II, BENEATH THE ECLIPSE: the arenas under the Sanctum.
//
// When the Sovereign falls the Sanctum cracks and you drop into the machine
// beneath it. Act II is built in its own region, 60 m under Act I and north
// of it (floors at Y -60, everything at Z < -440), so ASCENT can build both
// acts into one world and the fall is a real fall.
//
//                N (-Z)
//        ┌───────────────────┐  Z -622..-652  APSE: chancel (+3 m), the drowned
//        │  ORGAN    (loft)  │                organ (pipes climb to a +12 m loft)
//   ┌────┴───┐ CROSSING ┌────┴───┐  Z -592..-622  transepts (+1.5 m) to X ±50,
//   │TRANSEPT│ (light   │TRANSEPT│                rose windows; ring walkway (+9 m)
//   └────┬───┘  shaft)  └───┬────┘
//        │ ║  NAVE  (0) ║   │  Z -473..-592  nave X ±16; aisles (+1.5) to ±29,
//        │ ║═══════════ ║   │                galleries (+6) over them, buttress
//        │ ║  bridges   ║   │                bridges (+9), clerestory (+12)
//        └─┴──┐ door ┌──┴───┘
//             │NARTHEX│     Z -450..-472  the shaft you fall down
//             └───────┘
//                S (+Z)
//
// The water: one volume over the whole floor. FLOOD raises it each wave
// (0.4 m, 2.0 m, 2.6 m over the nave floor).
// =============================================================================
#include "Level.h"

inline void buildAct2(LevelBuilder& B) {
    using glm::vec3;
    LevelData& L = B.L;
    auto aabb = &LevelBuilder::aabb;
    auto wall = [&](float x0, float y0, float z0, float x1, float y1, float z1, vec3 c) { return B.wall(x0,y0,z0,x1,y1,z1,c); };
    auto prop = [&](float x0, float y0, float z0, float x1, float y1, float z1, vec3 c) { B.prop(x0,y0,z0,x1,y1,z1,c); };
    auto neon = [&](float x0, float y0, float z0, float x1, float y1, float z1, vec3 c) { B.neon(x0,y0,z0,x1,y1,z1,c); };

    const float F = -60.f;   // the Nave's floor
    vec3 bone{0.74f,0.70f,0.62f}, boneDark{0.46f,0.44f,0.40f}, slate{0.15f,0.19f,0.21f},
         teal{0.2f,0.9f,0.85f}, red{1.f,0.16f,0.08f}, brass{0.62f,0.5f,0.3f}, light{1.4f,1.25f,1.0f};

    // All of Act II stands in one basin, 60 m down
    L.basins.push_back({aabb(-300, 0, -440, 300, 0, -1000), F});

    Arena a;
    a.name = "THE DROWNED NAVE";
    a.subtitle = "THE WATER RISES - SURVIVE 3 WAVES";
    a.bounds = aabb(-49, F, -651, 49, F + 26, -451);
    a.zone   = aabb(-50.5f, F, -652.5f, 50.5f, F + 28, -449.5f);   // ceiling under the dome
    a.extraZones.push_back(aabb(-11.5f, F, -471.5f, 11.5f, -2.f, -450.5f));   // the shaft over the narthex
    a.extraZones.push_back(aabb(-2.9f, F + 3, -662.f, 2.9f, F + 9, -652.f));   // the sealed passage
    a.playerStart = {0.f, -8.f, -460.f};
    a.startYaw = -90.f;

    // ---- floors (drawn) ----
    L.floors.push_back({-300.f, -1000.f, 300.f, -440.f, F - 0.3f, {0.05f, 0.06f, 0.07f}});   // seen from wall tops
    L.floors.push_back({-12.f, -472.f, 12.f, -450.f, F, slate});                             // narthex
    L.floors.push_back({-16.f, -622.f, 16.f, -472.f, F, {0.2f, 0.22f, 0.22f}});              // nave + crossing

    // ---- the narthex and its shaft ----
    B.mat = Mat::BRICK;
    wall(-13, F, -472, -12, -2, -450, boneDark);
    wall( 12, F, -472,  13, -2, -450, boneDark);
    wall(-13, F, -450,  13, -2, -449, boneDark);
    for (float y = F + 8.f; y < -4.f; y += 7.f) {                 // rings of light down the shaft
        neon(-11.95f, y, -471.9f, -11.85f, y + 0.25f, -450.1f, teal * 0.7f);
        neon(11.85f, y, -471.9f, 11.95f, y + 0.25f, -450.1f, teal * 0.7f);
    }
    // the great doorway into the nave (open), and the wall above it up to the shaft's top
    B.wallX(-29, 29, -473, -472, F, F + 26, bone, {{-5.f, 5.f, F, F + 8.f}});
    wall(-13, F + 26, -473, 13, -2, -472, bone);
    B.kit().arch({0.f, F + 8.f, -472.5f}, 10.f, 3.f, 0.8f, 1.4f, boneDark);

    // ---- the nave: outer walls, aisles, arcades, galleries ----
    for (int s : {-1, 1}) {
        float sx = (float)s;
        wall(sx * 29.f, F, -592, sx * 30.f, F + 26, -473, bone);                       // outer wall
        wall(sx * 16.f, F, -592, sx * 29.f, F + 1.5f, -473, boneDark);                 // aisle floor (+1.5)
        // arcade columns every 12 m; galleries rest on them
        for (float z = -479.f; z > -592.f; z -= 12.f) {
            wall(sx * 16.f - 0.8f, F, z - 0.8f, sx * 16.f + 0.8f, F + 5.f, z + 0.8f, bone);
            B.kit().column({sx * 16.f, F, z}, 0.95f, 5.f, bone, 12);
        }
        // gallery deck (+6), broken where the arcade collapsed (west Z -528..-536, east -552..-560)
        float g0 = s < 0 ? -528.f : -552.f, g1 = s < 0 ? -536.f : -560.f;
        float x0 = sx * 15.2f, x1 = sx * 29.f;
        wall(std::min(x0, x1), F + 5, g0, std::max(x0, x1), F + 6, -473, boneDark);
        wall(std::min(x0, x1), F + 5, -592, std::max(x0, x1), F + 6, g1, boneDark);
        neon(std::min(x0, x1), F + 5.9f, g0 - 0.12f, std::max(x0, x1), F + 6.02f, g0, red * 0.6f);   // broken edges
        neon(std::min(x0, x1), F + 5.9f, g1, std::max(x0, x1), F + 6.02f, g1 + 0.12f, red * 0.6f);
        // pointed arches between the columns, except over the collapse
        for (float z = -485.f; z > -592.f; z -= 12.f) {
            if (z < g0 + 6.f && z > g1 - 6.f) continue;
            B.kit().arch({sx * 16.f, F + 1.5f, z}, 10.4f, 3.4f, 0.7f, 1.2f, bone, 1.5707963f);
        }
        // the collapsed span: rubble in the water under it
        for (int k = 0; k < 4; ++k)
            B.kit().rock({sx * (19.f + 2.5f * k), F + 1.5f, (g0 + g1) * 0.5f + (k % 2 ? 1.5f : -1.5f)}, 1.3f, 1.2f, boneDark, 900u + k + (s > 0 ? 10u : 0u));
        // a pad in the aisle under the gap: up through it onto the gallery beyond
        L.pads.push_back({{sx * 22.f, F + 1.5f, (g0 + g1) * 0.5f}, {1.2f, 1.2f}, {0.f, 17.f, 5.f}});
        // clerestory ledges (+12) and pads up from the gallery
        wall(std::min(sx * 26.f, sx * 29.f), F + 11, -589, std::max(sx * 26.f, sx * 29.f), F + 12, -476, boneDark);
        for (float z : {-482.f, -584.f}) L.pads.push_back({{sx * 23.5f, F + 6.f, z}, {1.1f, 1.1f}, {sx * 2.2f, 18.5f, 0.f}});
        // tall windows glowing red from below
        for (float z = -485.f; z > -592.f; z -= 24.f)
            neon(sx * 28.95f - (s > 0 ? 0.f : -0.0f), F + 13.f, z - 2.f, sx * 28.9f, F + 22.f, z + 2.f, red * 0.9f);
    }
    // the buttress bridges (+9) across the nave, with steps up from the galleries
    for (float z : {-500.f, -545.f}) {
        wall(-17, F + 8, z - 1.5f, 17, F + 9, z + 1.5f, bone);
        B.kit().arch({0.f, F + 2.f, z}, 32.f, 6.f, 0.9f, 2.6f, boneDark);    // the flying buttress under it
        for (int s : {-1, 1}) {
            float sx = (float)s;
            wall(std::min(sx * 17.f, sx * 20.f), F + 6, z - 1.5f, std::max(sx * 17.f, sx * 20.f), F + 7.5f, z + 1.5f, boneDark);
        }
        neon(-17, F + 8.95f, z - 0.15f, 17, F + 9.02f, z + 0.15f, teal * 0.5f);
    }
    // pews: low cover on the nave floor, two blocks of rows
    for (int s : {-1, 1}) for (float z = -486.f; z > -580.f; z -= 9.f) {
        if (((int)(-z) / 9) % 4 == 2) continue;   // gaps where pews were swept away
        wall(std::min(s * 3.f, s * 13.f), F, z - 0.35f, std::max(s * 3.f, s * 13.f), F + 0.9f, z + 0.35f, slate);
    }
    // a fallen column lying across the nave (collision as three blocks under it)
    B.kit().rod({-12.f, F + 0.9f, -560.f}, {8.f, F + 0.9f, -571.f}, 1.0f, bone, 12);
    B.solid(-12.5f, F, -563.5f, -5.f, F + 1.8f, -558.f);
    B.solid(-5.f, F, -567.5f, 2.5f, F + 1.8f, -562.f);
    B.solid(2.5f, F, -571.5f, 8.5f, F + 1.8f, -566.f);

    // ---- the crossing, the transepts, the light ----
    for (int s : {-1, 1}) {
        float sx = (float)s;
        wall(std::min(sx * 16.f, sx * 50.f), F, -622, std::max(sx * 16.f, sx * 50.f), F + 1.5f, -592, boneDark);   // transept floor
        wall(std::min(sx * 29.f, sx * 51.f), F, -593, std::max(sx * 29.f, sx * 51.f), F + 26, -592, bone);          // south wall
        wall(std::min(sx * 16.f, sx * 51.f), F, -623, std::max(sx * 16.f, sx * 51.f), F + 26, -622, bone);         // north wall
        wall(sx * 50.f + (s > 0 ? 0.f : -1.f), F, -622, sx * 50.f + (s > 0 ? 1.f : 0.f), F + 26, -592, bone);       // end wall
        // rose window: eight spokes of red light round a hub
        vec3 c{sx * 49.9f, F + 15.f, -607.f};
        for (int k = 0; k < 8; ++k) {
            float ang = k * 0.7853982f;
            B.kit(true).rod(c, c + vec3{0.f, std::sin(ang) * 6.f, std::cos(ang) * 6.f}, 0.18f, red * 0.9f, 6);
        }
        B.kit(true).column(c - vec3{sx * 0.2f, 0.8f, 0.f}, 0.9f, 1.6f, red, 10);
        // plinth pads up to the ring walkway
        wall(sx * 10.f - 1.3f, F, -601.3f, sx * 10.f + 1.3f, F + 1.5f, -598.7f, slate);
        L.pads.push_back({{sx * 10.f, F + 1.5f, -600.f}, {1.1f, 1.1f}, {sx * 3.5f, 22.f, 0.f}});
    }
    // ring walkway (+9) round the crossing
    wall(-16, F + 8, -594.5f, 16, F + 9, -592, boneDark);
    wall(-16, F + 8, -622, 16, F + 9, -619.5f, boneDark);
    wall(-16, F + 8, -619.5f, -13.5f, F + 9, -594.5f, boneDark);
    wall(13.5f, F + 8, -619.5f, 16, F + 9, -594.5f, boneDark);
    // the crossing piers, and the cracked dome over it (seen through the ceiling's force field)
    for (int sx : {-1, 1}) for (int sz : {-1, 1})
        wall(sx * 16.f - 1.5f, F, -607.f + sz * 15.f - 1.5f, sx * 16.f + 1.5f, F + 26, -607.f + sz * 15.f + 1.5f, bone);
    B.kit().curve({0.f, 0.f, -607.f}, 17.f, 0.f, 6.2831853f, F + 26.f, F + 30.f, 1.2f, boneDark, 24);
    B.kit().dome({0.f, F + 30.f, -607.f}, 17.f, boneDark, 6, 18);
    B.kit(true).rod({-3.f, F + 46.f, -607.f}, {5.f, F + 44.f, -603.f}, 0.4f, light * 0.6f, 6);   // the crack
    B.kit(true).column({0.f, F, -607.f}, 3.4f, 46.f, light * 0.18f, 18);                          // the shaft of eclipse light
    L.gems.push_back({{0.f, F + 0.4f, -607.f}, light, 1.2f, true});

    // ---- the chancel and the drowned organ ----
    wall(-16, F, -652, 16, F + 3, -622, slate);                    // chancel (+3)
    wall(-16, F, -622, 16, F + 1.5f, -619, slate);                 // its step
    wall(-17, F, -652, -16, F + 26, -622, bone);
    wall( 16, F, -652,  17, F + 26, -622, bone);
    // back wall with the sealed way on
    B.wallX(-17, 17, -653, -652, F, F + 26, bone, {{-3.f, 3.f, F + 3.f, F + 8.f}});
    a.exitDoor = B.doorway(true, -3, 3, -653, -652, F + 3, 5.f, red, true);
    wall(-4, F, -662, -3, F + 9, -653, slate); wall(3, F, -662, 4, F + 9, -653, slate);
    wall(-4, F, -663, 4, F + 9, -662, slate);
    wall(-3, F, -662, 3, F + 3, -653, slate);                      // passage floor (+3)
    wall(-4, F + 9, -663, 4, F + 10, -653, slate);
    L.finishPos = {0.f, F + 3.f, -659.f};
    // organ pipes: a staircase of double jumps from the chancel to the loft (+12)
    B.mat = Mat::METAL;
    const float pipeX[] = {-9.f, -6.f, -3.f, 0.f, 3.f, 6.f};
    for (int k = 0; k < 6; ++k) {
        float h = 4.5f + 1.5f * k;
        wall(pipeX[k] - 0.7f, F + 3, -646.7f, pipeX[k] + 0.7f, F + h, -645.3f, brass);
        B.kit().column({pipeX[k], F + 3.f, -646.f}, 0.75f, h - 3.f, brass, 12);
        B.kit(true).column({pipeX[k], F + h - 0.15f, -646.f}, 0.78f, 0.15f, red, 12);
    }
    for (float x : {-12.f, -10.5f, 9.f, 10.5f, 12.f}) B.kit().column({x, F + 3.f, -649.f}, 0.55f, 14.f + std::fabs(x) * 0.4f, brass * 0.8f, 10);
    B.mat = Mat::BRICK;
    wall(-14, F + 11, -652, 14, F + 12, -648.5f, boneDark);        // organ loft (+12)

    // ---- the water ----
    L.water.push_back({aabb(-50, F, -652, 50, F, -450), F + 0.4f});

    // ---- spawns, waves ----
    a.groundSpawns = {
        {-8, F, -490}, {8, F, -509}, {-8, F, -538}, {8, F, -578},                    // nave floor
        {-22, F + 1.5f, -500}, {22, F + 1.5f, -520}, {-22, F + 1.5f, -575}, {22, F + 1.5f, -585},   // aisles
        {-38, F + 1.5f, -607}, {38, F + 1.5f, -607},                                // transepts
        {-8, F + 3, -634}, {8, F + 3, -634},                                         // chancel
        {-23, F + 6, -490}, {23, F + 6, -505}, {-23, F + 6, -580}, {23, F + 6, -575},   // galleries
    };
    a.airSpawns = {{0, F + 13, -495}, {0, F + 13, -560}, {-32, F + 11, -607}, {32, F + 11, -607}};
    a.waves = {
        {{EnemyType::HUSK, 6}, {EnemyType::RIPPER, 5}, {EnemyType::SENTINEL, 3},
         WaveEntry(EnemyType::SHIELDBEARER, 2).with({EnemyType::HUSK})},
        {{EnemyType::RAPTOR, 4}, {EnemyType::BRUTE, 2}, {EnemyType::CONDUCTOR, 2}, {EnemyType::MITE, 6}},
        {{EnemyType::BRUTE, 3}, WaveEntry(EnemyType::SHIELDBEARER, 3).with({EnemyType::SENTINEL}),
         {EnemyType::CONDUCTOR, 2}, {EnemyType::JUGGERNAUT, 1}, {EnemyType::RIPPER, 4}},
    };
    a.goals = {WaveGoal{},
               WaveGoal::conduits("DESTROY THE CONDUITS ON THE GALLERIES",
                                  {{-23.f, F + 6.f, -497.f}, {23.f, F + 6.f, -522.f}, {23.f, F + 6.f, -582.f}}),
               WaveGoal{}};
    a.maxAlive = 11;
    a.damageScale = 1.3f;
    a.shift = ArenaShift::FLOOD;
    a.floodLevels = {F + 0.4f, F + 2.0f, F + 2.6f};
    a.ambient = Ambient::MOTES;
    a.theme = Theme{
        {0.01f,0.03f,0.04f}, {0.06f,0.16f,0.17f}, {0.12f,0.02f,0.02f},
        glm::normalize(vec3{0.f, 0.95f, -0.2f}), {1.6f,1.4f,1.1f}, 0.05f, 0.f,
        {0.02f,0.05f,0.06f}, 0.3f,
        // Pale light falls through the crack; the red comes up from the water
        glm::normalize(vec3{0.15f,-1.f,0.1f}), {0.85f,0.9f,0.95f},
        {0.16f,0.3f,0.32f}, {0.28f,0.06f,0.05f},
        {0.05f,0.12f,0.13f}, 0.012f };
    L.arenas.push_back(std::move(a));
}

// ACT II mode's level
inline LevelData buildAct2Level() {
    LevelData L;
    LevelBuilder B{L};
    buildAct2(B);
    return L;
}
```

Notes for the implementer:
- `LevelData::finishPos` already exists (FAST's beacon). `LevelData::gems` is `{pos, color, size, beam}`.
- If `B.wallX` gap semantics differ (`Gap{a0, a1, y0, y1}` along X), the doorway call above is `{-5, 5, F, F+8}`: a 10 m wide, 8 m tall opening.
- Tune pad launches until `padsLand` passes; keep each landing on the tier the comment names.
- Remove the no-op `(s > 0 ? 0.f : -0.0f)` in the window `neon` call when typing it in: the window sits on the inner face of the outer wall, `X = sx * 28.9..28.95`.

`Makefile`: add `src/LevelAct2.h` to `HEADERS` (next to `src/LevelGauntlet.h`) and to the `test:` prerequisites.

- [ ] **Step 4: Run** `make && make test` — Expected: all `ok:`; prints the fall distance (~52 m) and time (~2.1 s). If a spawn or pad check fails, move that spawn/pad (print lines say which) — don't loosen the check.

- [ ] **Step 5: Commit**

```bash
git add src/LevelAct2.h Makefile tests/test_game.cpp
git commit -m "The Drowned Nave: Act II's first arena"
```

---

### Task 7: Unlock and menu

**Files:**
- Modify: `src/Progression.h` (`Records::act2Unlocked`, `canStartAct2`), `src/GameState.h` (`GameMode::ACT2`), `src/MenuState.h`, `src/Gameplay_Combat.h` (Sovereign death), `src/main.cpp` (`--act2`)
- Test: `tests/test_game.cpp`

**Interfaces:**
- Produces: `GameMode::ACT2`; `bool Records::act2Unlocked = false;` saved as `act2Unlocked 0|1`; `inline bool canStartAct2(const Records& r) { return r.act2Unlocked; }`

- [ ] **Step 1: Write the failing test**

```cpp
    // ---------------------------------------------------------------- the Act II unlock
    {
        Records r; r.load();
        Records keep = r;
        r.act2Unlocked = false; r.save();
        Records a; a.load();
        CHECK(!a.act2Unlocked && !canStartAct2(a), "act II starts locked: the menu row won't start it");
        a.act2Unlocked = true; a.save();
        Records b; b.load();
        CHECK(b.act2Unlocked && canStartAct2(b), "the unlock is saved and loaded");
        keep.save();   // leave the developer's records as they were
    }
```

- [ ] **Step 2: Run** `make test` — Expected: compile error (`act2Unlocked`).

- [ ] **Step 3: Implement**

`src/Progression.h` `Records`: member `bool act2Unlocked = false;   // the Sovereign has fallen: ACT II is open`; in `load()` add `else if (key == "act2Unlocked") { int v = 0; f >> v; act2Unlocked = v != 0; }`; in `save()` add `f << "act2Unlocked " << (act2Unlocked ? 1 : 0) << "\n";`. After the struct:

```cpp
// The main menu's ACT II row only starts a run once the Sovereign has fallen
inline bool canStartAct2(const Records& r) { return r.act2Unlocked; }
```

`src/GameState.h`: `enum class GameMode { ARENA, FAST, ENDLESS, DAILY, ACT2 };` and a comment line `// ACT2:    Act II, beneath the eclipse (LevelAct2.h). Unlocked by killing the Sovereign.`

`src/Gameplay_Combat.h` at the Sovereign's death (line ~370, inside `if (e.type == EnemyType::SOVEREIGN) {`):

```cpp
            if (!g_godMode && !records.act2Unlocked) {
                records.act2Unlocked = true;
                records.save();
                ui.feed("ACT II UNLOCKED - BENEATH THE ECLIPSE", {0.35f, 0.95f, 0.9f});
            }
```

`src/MenuState.h`:
- `NUM_ITEMS = 8`; `settingsMenu.onBack = [this]() { page = MAIN; selected = 6; };`
- `activate`:

```cpp
        if (idx == 0 && onStart) onStart(GameMode::ARENA, StartOptions{});
        if (idx == 1 && onStart && canStartAct2(records)) onStart(GameMode::ACT2, StartOptions{});
        if (idx == 2 && onStart) onStart(GameMode::FAST, StartOptions{});
        if (idx == 3 && onStart) onStart(GameMode::ENDLESS, StartOptions{});
        if (idx == 4 && onStart) onStart(GameMode::DAILY, StartOptions{});
        if (idx == 5) { /* the existing LEADERBOARD body, unchanged */ }
        if (idx == 6) { page = SETTINGS; settingsMenu.selected = 1; }
        if (idx == 7 && onQuit) onQuit();
```

- `renderMain` items:

```cpp
        bool act2 = canStartAct2(records);
        Item items[NUM_ITEMS] = {
            {"ARENA", arenaSub.c_str()},
            {"ACT II", act2 ? "PREVIEW - 1/4 ARENAS - BENEATH THE ECLIPSE" : "DEFEAT THE SOVEREIGN TO UNLOCK"},
            {"FAST",  fastSub.c_str()},
            ...
```

and in the item loop, after `glm::vec4 c = ...;` add `if (i == 1 && !act2) c = {0.42f, 0.42f, 0.47f, 0.75f};`. Check the rest of `renderMain` and `handleEvent` for hard-coded item indices (`grep -n "selected ==\|idx ==\|i == [0-9]" src/MenuState.h`) and shift every one at or after 1 by one.
- Dev rows: in the constructor after the FAST rows:

```cpp
        LevelData A2 = buildAct2Level();
        for (int i = 0; i < (int)A2.arenas.size(); ++i)
            devRows.push_back({"ACT II", A2.arenas[i].name, GameMode::ACT2, i, (int)A2.arenas[i].waves.size()});
```

`devY`: for `r.mode == GameMode::ACT2` return `232.f + (A_COUNT + 1 + r.arena) * 34.f` where `A_COUNT` is the number of ARENA rows (store it in a member `int act1Rows` when building rows). `devX`: ACT2 rows use the left column (`r.mode == GameMode::FAST ? right : left`). In `devActivate` give ACT2 the ARENA wave option: `o.wave = r.mode != GameMode::FAST ? std::min(devWave, r.waves - 1) : 0;`. Column jumping: `GameMode want = r.mode == GameMode::FAST ? GameMode::ARENA : GameMode::FAST;`. Row label: `r.mode == GameMode::ARENA ? "ARENA" : r.mode == GameMode::ACT2 ? "ACT II" : "ROOM"`.
- Add `#include "LevelAct2.h"` to `MenuState.h`.

`src/main.cpp`: next to `--fast`: `if (arg == "--act2") { app->mode = GameMode::ACT2; app->pending = App::NextState::Game; }` and add `--act2 ACT II` to the flag comment.

- [ ] **Step 4: Run** `make && make test` — Expected: all `ok:`. Then `./shooter --res 720 --shot 30 /private/tmp/claude-501/-Users-ollie-Desktop-3d-shooter/fe78f86e-b521-4920-97a7-267683804fa3/scratchpad/menu.bmp` and Read the image: the ACT II row is second and greyed with "DEFEAT THE SOVEREIGN TO UNLOCK" (on this machine's records it may already be unlocked; either label is correct for the state).

- [ ] **Step 5: Commit**

```bash
git add src/Progression.h src/GameState.h src/MenuState.h src/Gameplay_Combat.h src/main.cpp tests/test_game.cpp
git commit -m "ACT II on the menu, unlocked by the Sovereign's fall"
```

---

### Task 8: The ACT II run

**Files:**
- Modify: `src/GameplayState.h`, `src/Gameplay_Flow.h`, `src/Gameplay_Tick.h`, `src/Gameplay_HUD.h`, `src/UIRenderer.h`, `src/MusicSynth.h`
- Test: `tests/test_game.cpp`

**Interfaces:**
- Consumes: `buildAct2Level()`, `GameMode::ACT2`, `LevelData::finishPos`.
- Produces: `bool GameplayState::act2() const;` `bool act2Falling;` `void beginAct2();` music track 5 "NAVE".

- [ ] **Step 1: Write the failing test** (director-level: a whole ACT II run reaches VICTORY; and the soundtrack count)

```cpp
    // ---------------------------------------------------------------- a simulated ACT II run
    {
        LevelData N = buildAct2Level();
        WaveDirector d; d.level = &N;
        d.startArena(0);
        glm::vec3 player = N.arenas[0].playerStart; player.y = -60.f;
        std::vector<float> alive;
        for (int tick = 0; tick < 60 * 60 * 20 && d.phase != WaveDirector::Phase::VICTORY; ++tick) {
            std::vector<SpawnRequest> out;
            d.update(DT, (int)alive.size(), player, out);
            for (auto& r : out) alive.push_back(3.f);
            for (auto& t : alive) t -= DT;
            for (size_t k = 0; k < alive.size(); ++k)
                if (alive[k] <= 0.f && d.conduitsLeft() > 0) d.onConduitDestroyed(d.goal().points[0]);
            alive.erase(std::remove_if(alive.begin(), alive.end(), [](float t) { return t <= 0.f; }), alive.end());
            for (auto& ev : d.events) if (ev.kind == DirectorEvent::GOAL_DONE) alive.clear();
            d.events.clear();
        }
        CHECK(d.phase == WaveDirector::Phase::VICTORY, "a simulated ACT II run clears the Nave");
        CHECK(MUSIC_TRACKS == 6 && std::string(musicTrack(5).name) == "NAVE", "the Nave has its own track");
    }
```

(Check `conduitsLeft()` and `onConduitDestroyed(glm::vec3)` exist in `WaveDirector` — they're used by the ARENA run test at line ~911; mirror that test's conduit handling exactly if this simplification doesn't clear the goal.)

- [ ] **Step 2: Run** `make test` — Expected: FAIL on the music check (and compile until `MUSIC_TRACKS` changes); the director part should already pass — if it does, it still guards the Nave's waves.

- [ ] **Step 3: Implement**

`src/MusicSynth.h`: add a sixth entry to `T[]`:

```cpp
        // The Nave: slow, cold, a bell-like arp over a drowned pulse
        {"NAVE", 138.f, 45, {0, 3, 5, 4},
         "x.....x...x.....", "....x.......x...", "x.x.X.x.x.x.X.xo",
         "x--.x--.o--.x-f.", "0.2.1.3.0.2.1.3.", "2-------3---1---0-------1---2---"},
```

and change `return T[((i % 5) + 5) % 5];` to `% 6` both places, `constexpr int MUSIC_TRACKS = 6;`. Update the header comment ("in six tracks … one for the Nave").

`src/GameplayState.h`: next to `fast()`:

```cpp
    bool act2() const { return mode == GameMode::ACT2; }
```

members near `finishOpen`: `bool act2Falling = false;   // ACT II: still dropping down the shaft (the fight waits)`; declaration `void beginAct2();`.

`src/Gameplay_Flow.h`:
- Constructor level choice: `level = fast() ? buildGauntlet() : act2() ? buildAct2Level() : buildLevel();`
- Constructor `ranked = ... && !act2();` and `newRun`: `ranked = !g_godMode && !g_practice && !act2();` (ACT II is a preview: unranked until it's complete).
- In the constructor after `enterArena(start);` and in `newRun` after its `enterArena(...)`: `if (act2()) beginAct2();`
- In `enterArena` near `finishOpen = false;`: `act2Falling = act2() && !g_devCam;`
- New function:

```cpp
// ACT II: the head start (what an Act I run has by the Sanctum), spent in the
// armory before the fall
inline void GameplayState::beginAct2() {
    prog = Progression{};
    prog.level = 6; prog.points = 5;
    pushBanner("ACT II", "BENEATH THE ECLIPSE", {0.35f, 0.95f, 0.9f}, 3.5f);
    if (!g_devCam && g_devOverlay.empty()) openArmory();
}
```

- `handleDirectorEvents`, `ARENA_START` else-branch: when `act2()`, `pushBanner(std::string("ACT II  ") + ar.name, ar.subtitle, {0.35f, 0.95f, 0.9f}, 2.6f);` instead of the "ARENA n/m" banner.
- `VICTORY`: `if (act2()) { finishOpen = true; pushBanner("THE WAY DOWN IS OPEN", "BEHIND THE ORGAN", {0.35f, 0.95f, 0.9f}, 3.f); audio.play("wave"); } else victoryDelay = 2.5f;`
- `ARENA_CLEARED`: when `act2()`, skip the "THE GATE IS OPEN - HEAD NORTH" banner (the VICTORY banner says it).
- `updateMusic`: `m.setTrack(fast() ? FAST_TRACK[a % 7] : act2() ? 5 : ARENA_TRACK[a % 5]);`
- `setupEndless` keeps `buildLevel()` (Act I).

`src/Gameplay_Tick.h`:
- Wrap the director update: `if (!act2Falling) director.update(dt, alive, player.position, spawns);` (line 242; keep `spawns` empty otherwise).
- After `player.update(...)`: `if (act2Falling && player.onGround) act2Falling = false;`
- `elapsedTime` (Gameplay_Flow.h:585): `if (!act2Falling) elapsedTime += floatDt;`
- The finish beacon block (line 287) already ends the run at `level.finishPos`; it applies to ACT II as-is.

`src/UIRenderer.h` `renderVictoryArena`: add trailing parameters `const char* title = "ALL ARENAS CLEARED", const char* sub = "THE SOVEREIGN HAS FALLEN"` and use them for the two header lines.

`src/Gameplay_HUD.h`: victory branch: `else if (act2()) ui.renderVictoryArena(totalKills, totalShots, totalHits, deaths, elapsedTime, peakStyle, prog.level, 0.f, false, false, runScore(), "TO BE CONTINUED", "THE NAVE IS BEHIND YOU - THREE MORE BELOW");` before the generic ARENA call, and skip `renderLeaderboardPanel()` when `act2()`. The pause line: `act2() ? std::string("ACT II - ") + ar.name : ...`. The top HUD's "ARENA %d/%d" strings: use `ACT II - %s` when `act2()` (grep `ARENA %d/%d` in `Gameplay_HUD.h`).

- [ ] **Step 4: Run** `make && make test` — Expected: all `ok:`. Then:

```bash
./shooter --act2 --res 720 --shot 20 /private/tmp/claude-501/-Users-ollie-Desktop-3d-shooter/fe78f86e-b521-4920-97a7-267683804fa3/scratchpad/act2_armory.bmp
```

Read the image: the armory is open with 5 points over the shaft, the "ACT II" banner behind it.

- [ ] **Step 5: Commit**

```bash
git add src/GameplayState.h src/Gameplay_Flow.h src/Gameplay_Tick.h src/Gameplay_HUD.h src/UIRenderer.h src/MusicSynth.h tests/test_game.cpp
git commit -m "The ACT II run: a head start, the fall, the Nave, to be continued"
```

---

### Task 9: Seeing and hearing the water

**Files:**
- Create: `src/water.vert`, `src/water.frag`
- Modify: `src/GameplayState.h` (shader, VAO/VBO, `renderWater`), `src/Gameplay_Flow.h` (load shader, buffers, cleanup), `src/Gameplay_Render.h` (draw + warm), `src/Gameplay_Tick.h` (wade/skim sounds, spray), `src/Gameplay_Combat.h` (enemy splash), `tools/gen_sfx.py`, `src/main.cpp` (sound list)
- Test: manual (`--shot`, `--bench`); `make test` stays green

**Interfaces:**
- Consumes: `LevelData::water`, `Player::wadeDepth`, `Player::sliding`.
- Produces: `void GameplayState::renderWater(const glm::mat4& view, const glm::mat4& proj, const Theme& th);`

- [ ] **Step 1: The shader**

`src/water.vert`:

```glsl
#version 330 core

layout(location = 0) in vec3 aPos;

uniform mat4 projection;
uniform mat4 view;

out vec3 vWorld;

void main()
{
    vWorld      = aPos;
    gl_Position = projection * view * vec4(aPos, 1.0);
}
```

`src/water.frag`:

```glsl
#version 330 core

in  vec3 vWorld;
out vec4 FragColor;

uniform float uTime;
uniform vec3  uViewPos;
uniform vec3  uDeep;        // the water's own colour, seen from above
uniform vec3  uGlow;        // the red light coming up from beneath
uniform vec3  uFogColor;
uniform float uFogDensity;
uniform vec2  uShaftXZ;     // where the eclipse light falls on it
uniform float uShaftR;

float ripple(vec2 p)
{
    return sin(p.x * 0.9 + uTime * 1.3) * 0.5
         + sin(p.y * 1.3 - uTime * 1.1) * 0.5
         + sin((p.x + p.y) * 2.1 + uTime * 2.2) * 0.25;
}

void main()
{
    float r    = ripple(vWorld.xz);
    vec3  col  = mix(uDeep, uGlow, 0.35 + 0.2 * r);
    float lit  = 1.0 - smoothstep(uShaftR * 0.5, uShaftR, length(vWorld.xz - uShaftXZ));
    col += vec3(0.9, 0.85, 0.75) * lit * (0.55 + 0.15 * r);
    vec3  v    = normalize(uViewPos - vWorld);
    float fres = pow(1.0 - abs(v.y), 3.0);
    col += uGlow * fres * 0.4;
    float d    = length(uViewPos - vWorld);
    col = mix(col, uFogColor, 1.0 - exp(-uFogDensity * d));
    FragColor = vec4(col, mix(0.7, 0.92, fres));
}
```

- [ ] **Step 2: Draw it**

`src/GameplayState.h`: members `ShaderProgram waterShader; GLuint waterVAO = 0, waterVBO = 0;` and the declaration of `renderWater`.

`src/Gameplay_Flow.h` constructor, with the other shaders: `waterShader.loadFiles("src/water.vert","src/water.frag");` and (where `tracerVAO` is created — mirror it) a VAO/VBO sized for 64 volumes × 6 vertices × 3 floats, `GL_DYNAMIC_DRAW`, attribute 0 = vec3. Destructor: delete them like `tracerVAO`.

`src/Gameplay_Render.h`:

```cpp
// Standing water: one translucent quad per volume at its current level
inline void GameplayState::renderWater(const glm::mat4& view, const glm::mat4& proj, const Theme& th) {
    if (level.water.empty()) return;
    float buf[64 * 6 * 3]; int n = 0;
    for (auto& w : level.water) {
        if (n + 18 > 64 * 18) break;
        float x0 = w.box.min.x, x1 = w.box.max.x, z0 = w.box.min.z, z1 = w.box.max.z, y = w.level;
        float q[18] = {x0,y,z0, x1,y,z0, x1,y,z1,  x0,y,z0, x1,y,z1, x0,y,z1};
        std::memcpy(buf + n, q, sizeof(q)); n += 18;
    }
    glBindBuffer(GL_ARRAY_BUFFER, waterVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, n * sizeof(float), buf);
    waterShader.use();
    waterShader.setMat4("projection", proj);
    waterShader.setMat4("view", view);
    waterShader.setFloat("uTime", gameClock);
    waterShader.setVec3("uViewPos", renderCamPos);
    waterShader.setVec3("uDeep", {0.03f, 0.1f, 0.11f});
    waterShader.setVec3("uGlow", {0.55f, 0.07f, 0.04f});
    waterShader.setVec3("uFogColor", th.fogColor);
    waterShader.setFloat("uFogDensity", th.fogDensity);
    waterShader.setVec2("uShaftXZ", {0.f, -607.f});
    waterShader.setFloat("uShaftR", 4.f);
    glDisable(GL_CULL_FACE);
    glDepthMask(GL_FALSE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBindVertexArray(waterVAO);
    glDrawArrays(GL_TRIANGLES, 0, n / 3);
    glBindVertexArray(0);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glEnable(GL_CULL_FACE);
}
```

If `ShaderProgram` has no `setVec2`, add one next to `setVec3` (`glUniform2f`). If `renderCamPos` isn't a member, pass it in like `warmPipelines` does. The shaft centre is the Nave's; when a later arena needs its own, move it into `WaterVolume`.

Call it in the main render right after the instanced boxes block and before `grapple.drawLine` (so decals/particles draw over it): `renderWater(view, proj, th);`. In `warmPipelines`, if `level.water.empty()` is false nothing extra is needed (it draws every frame from the first); if it is empty, skip.

- [ ] **Step 3: Splashes and sounds**

`tools/gen_sfx.py`, add:

```python
def gen_wade():
    """Wading: a soft low slosh with a short bright splash on top."""
    n = n_samples(0.32)
    noise = lowpass([white() for _ in range(n)], 900)
    out = []
    for i in range(n):
        t = i / n
        env = min(1.0, t * 20.0) * math.exp(-6.0 * t)
        out.append(noise[i] * env * 1.6 + sine(140 - 60 * t, i) * math.exp(-14.0 * t) * 0.25)
    return out


def gen_skim():
    """Skimming water on a slide: a hissing spray that rises and falls."""
    n = n_samples(0.6)
    noise = highpass([white() for _ in range(n)], 1800)
    out = []
    for i in range(n):
        t = i / n
        env = math.sin(math.pi * t) ** 0.6
        out.append(noise[i] * env * 0.9)
    return out
```

register `"wade": gen_wade, "skim": gen_skim,` in `GENERATORS`, then run `python3 tools/gen_sfx.py wade skim` (only these two; a full run would overwrite recorded sounds). Add `"wade", "skim"` to the `SOUNDS` list in `src/main.cpp`.

`src/Gameplay_Tick.h` footsteps: when `player.wadeDepth >= 0.1f` play `audio.play("wade", 60)` instead of a `STEPn`. While sliding with `wadeDepth >= 0.1f` (or skimming: `player.sliding && level.waterSurfaceAt(...) - player.position.y > -0.05f`): every 0.18 s `fx.spawnBurst(player.position + glm::vec3{0, 0.1f, 0}, {0.6f, 0.85f, 0.9f}, 6, 4.f, 0.35f, 9.f);` and every 0.5 s `audio.play("skim", 70)` (add `float skimTimer = 0.f;` to `GameplayState`). On `justLanded` with `wadeDepth >= 0.1f`: a bigger burst (`20, 6.f, 0.5f`) and `audio.play("wade", 90)`.

`src/Gameplay_Combat.h`: when an enemy lands (`e.grounded` after being airborne — track with the enemy's previous `grounded`, or simply on spawn) in water deeper than 0.3 m: `fx.spawnBurst(e.position, {0.6f, 0.85f, 0.9f}, 14, 5.f, 0.45f, 9.f); fx.spawnShockwave(e.position, 2.5f, {0.4f, 0.8f, 0.85f});`. Keep it to the spawn moment plus Brute/Juggernaut slams (the existing `ev.slam` branch: add the splash when `level.waterDepthAt(epos) > 0.3f`).

- [ ] **Step 4: Verify**

```bash
make && make test
S=/private/tmp/claude-501/-Users-ollie-Desktop-3d-shooter/fe78f86e-b521-4920-97a7-267683804fa3/scratchpad
./shooter --act2 --god --cam 0 -52 -480 -90 -12 --res 720 --shot 60 $S/nave_w1.bmp
./shooter --act2 --god --wave 2 --cam 22 -52 -500 -120 -10 --res 720 --shot 400 $S/nave_w2.bmp
./shooter --act2 --god --wave 3 --cam 0 -44 -560 -90 -20 --res 720 --shot 500 $S/nave_w3.bmp
./shooter --act2 --god --cam 0 -50 -590 -90 25 --res 720 --shot 60 $S/nave_crossing.bmp
./shooter --act2 --god --cam 0 -54 -630 -90 5 --res 720 --shot 60 $S/nave_organ.bmp
./shooter --act2 --god --cam 0 -10 -460 -90 -80 --res 720 --shot 5 $S/nave_fall.bmp
```

Read every image. Expected: water visible at the right height in each (wave 2/3 shots need the flood to have risen: if `--wave N` jumps past the rise, temporarily set the water from `floodLevels[N-1]` in `enterArena` when `g_startWave > 0` — that's also the right behaviour for practice starts; add it: `if (ar.shift == ArenaShift::FLOOD) for (auto& w : level.water) w.level = ar.floodLevels[std::min(g_startWave, (int)ar.floodLevels.size() - 1)];` after `shifts.reset(level)` in `enterArena`, guarded by `g_startWave > 0`). The light shaft visible in the crossing, the organ pipes stepping up, the shaft walls on the fall shot.

```bash
./shooter --act2 --god --cam 0 -52 -500 -90 -5 --bench 600 --res 1080
./shooter --arena 5 --god --cam 0 1.7 -300 -90 -5 --bench 600 --res 1080
```

Expected: the Nave's mean frame time within ~15% of the Sanctum's. If not: lower dome rings/sides, the light column's sides, or the pew count.

- [ ] **Step 5: Commit**

```bash
git add src/water.vert src/water.frag src/GameplayState.h src/Gameplay_Flow.h src/Gameplay_Render.h src/Gameplay_Tick.h src/Gameplay_Combat.h src/ShaderProgram.h tools/gen_sfx.py assets/sfx/wade.wav assets/sfx/skim.wav src/main.cpp
git commit -m "The Nave's water: a surface lit from below, splashes, wading and skimming"
```

---

### Task 10: Web build, docs, memory

**Files:**
- Modify: `README.md` (modes list), `src/Level.h` header comment (pointer to `LevelAct2.h`)
- Memory: `/Users/ollie/.claude/projects/-Users-ollie-Desktop-3d-shooter/memory/project_3d_shooter.md`, `project_act2.md`

- [ ] **Step 1: Web build**

Run: `source ~/emsdk/emsdk_env.sh && make web`
Expected: builds `web/dist/overdrive.js` with no errors (the water shader is copied by `cp src/*.vert src/*.frag`).

- [ ] **Step 2: README**

In the modes section add one line: `**ACT II (preview)** — unlocked by killing the Sovereign: fall beneath the eclipse into the Drowned Nave, a flooded cathedral whose water rises every wave. Three more arenas and a final boss are coming.` Add `--act2` to the command-line flags list if the README has one.

- [ ] **Step 3: Full check**

Run: `make && make test` — Expected: `ALL PASSED` from both test binaries.

- [ ] **Step 4: Commit**

```bash
git add README.md src/Level.h
git commit -m "README: ACT II preview"
```

- [ ] **Step 5: Memory**

Update `project_3d_shooter.md` Architecture: `LevelAct2.h` (`buildAct2`, Nave at Y −60, Z −450..−662), `LevelData::basins/water`, `Player::floorY/wadeDepth`, `ArenaShift::FLOOD`, `GameMode::ACT2`, `--act2`, `Records::act2Unlocked`. Update `project_act2.md` progress: piece 1 done (commit hash), next piece 2 (Hollowed variants + Seraph + Anchor). Tell the user to copy `web/dist` to the site repo (don't deploy).
