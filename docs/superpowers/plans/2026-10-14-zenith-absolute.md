# The Zenith + THE ABSOLUTE + the sword — Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** The game's encore. Straight after the Leviathan dies, the player
rises into THE ZENITH, a sixth Act II arena, to fight THE ABSOLUTE, a fast,
white-glowing humanoid duellist. A sword, OATHBREAKER, exists only for this
fight.

**Architecture:** The usual boss split. The mind (`src/EnemyAbsolute.h`) only
emits `ab*` events from a **string table**: named combos of moves, each move
carrying a tell kind (white, gold or red). What hurts you lives in
`src/AbsoluteHazards.h` (no GL, no audio). The game side is
`src/Gameplay_Absolute.h`. The sword is pure logic in `src/Sword.h`, glued
into the game by `src/Gameplay_Sword.h`, and modelled by `gunkit::buildSword`.
It is **not** a `WeaponId`: it's a separate `swordOut` state, so the saved
upgrade arrays never change.

**Tech Stack:** C++17 header-only, glm, SDL2, OpenGL 3.3 / GLES3 (web),
headless tests (`CHECK`), Emscripten web build, Python sfx generator
(`tools/gen_sfx.py`).

**Spec:** `docs/superpowers/specs/2026-10-14-zenith-absolute-design.md`

## Corrections to the spec, found while planning (tell the user)

1. **The Zenith is not directly above the Maw.** `LevelData::arenaAt` and
   `Arena::containsXZ` test XZ only, so an arena above the Maw would be
   claimed by the Maw. The Zenith's centre is **Z = (0, 400, −1400)**: high
   in the sky and 330 m past the Maw, with no XZ overlap. The player is moved
   there by `enterArena`, so it looks the same.
2. **There is no saved Act II clear today** (`Records` has only
   `act2Unlocked`). Task 5 adds `act2Cleared`, `bestAct2` and `bestZenith` to
   `Records`, and saves the first two the moment the Leviathan dies.
3. **His lines can't be spoken words.** Every enemy voice is a synthesized
   machine + choir (`VoiceSynth.h`). The lines ("KNEEL.", "TOO SLOW.",
   "AGAIN.", "BE JUDGED.", "...WORTHY.") are drawn as large subtitle text,
   each with a choir phrase from the voice bank.
4. **`ReverbSpace::OPEN` already exists** (`AudioTypes.h:12`), so the Zenith
   uses it as it is.
5. **`keepPlayerInZone` indexes `level.corridors[director.arena]`** when an
   arena is CLEARED (`Gameplay_Tick.h:389`). Once the Maw stops being the last
   arena it is CLEARED, and there is no corridor 4. Task 5 guards this.

## Global Constraints

- The sword exists **only in the Zenith** (`act2() && director.arena ==
  ZENITH_ARENA`), plus the dev flag `--sword`. It never touches `WeaponId`,
  `WEAPON_COUNT`, `Progression`'s upgrade arrays or the armory.
- Zenith: centre Z = (0, 400, −1400). Dais r 0–9 raised 1.2 m. Floor r 9–44
  with 6 m seam cells. Eight pillars at r 34 (2.5 m wide, 14 m tall). Six
  shards at r 52–62 past a 6 m gap. The Halo at r 70, 40 m up. `voidY` =
  Z.y − 40, and a fall costs 20. Reverb `OPEN`. Music track 10 "ZENITH",
  `MUSIC_TRACKS` 11. One wave `{ABSOLUTE, 1}`, `damageScale` 1.0, `maxAlive` 1.
- `ZENITH_ARENA` = 5 (the sixth Act II arena). `--act2 --arena 6` starts
  there. The dev level select lists it automatically (`MenuState.h:79`
  iterates `A2.arenas`).
- THE ABSOLUTE: `EnemyType::ABSOLUTE`, `isBoss`, 9000 hp (tunable 7000–11000
  in Task 10). Phases at 60 % and 25 %. The last judgment once at 8 %. Run
  11 m/s, dash 32 m/s.
- Colour rule: every strike is exactly one of WHITE (parryable), GOLD
  (unparryable) or RED (area / laser).
- Parry: a white strike parried → reel 0.8 s at ×1.5. The second parry in
  one string → broken 3 s at ×2. A sword parry of a phase 3 blade strike →
  the blade shatters, broken 3 s at ×2. Spent after the last judgment: 4 s
  at ×2.
- Guard: gunfire from beyond **14 m** is turned aside unless he's staggered,
  reeling, winding up or recovering. Sword hits always land.
- Tell floors for him: **0.35 s** opener, **0.22 s** follow-up within a
  string. The phase change pause is 0.6 s.
- Damage: single hits 18–30. A whole string taken is 60–70.
- Sword: strokes 70 / 70 / 120, `SWING_TIME` 0.30, combo reset 0.6 s, reach
  3.2 m, 100° arc. Lock-on step ≤ 2 m within 6 m. Thrust: hold 0.5 s, 8 m in
  0.2 s, 220. Plunge 150 within r 4. Parry window 0.15 s, spam lock 0.4 s,
  riposte ×2.5 within 1 s. Guard meter 100: white −25, gold −60, beam −15/s,
  refill 30/s after 1 s. Guard break: stunned 1 s. Block takes 25 %, moves at
  60 %. Healing: a sword hit 6 (a gun hit is 2), a parry 10.
- Text exactly as in the spec: "THE ECLIPSE BREAKS" / "SOMETHING ABOVE IT
  DOES NOT", "HE ASCENDS" / "STAY CLOSE", "THE BLADE OF LIGHT" / "MEET IT",
  "ZENITH CLEARED" / "THE LIGHT FALLS". Lines: "KNEEL.", "TOO SLOW.",
  "AGAIN.", "BE JUDGED.", "...WORTHY.".
- Commits: imperative descriptive sentences as in `git log`, **no
  Co-Authored-By / Claude trailers** (user rule). Never push without asking.
- After every task: `make && make test` green.

## Review Focus

1. **Dying or retrying mid-fight** (phase 2 or 3, pillars shattered, shards
   gone, a ray or seams live, the guard broken, the sword out mid-swing).
   `enterArena` restores every pillar and shard, clears `AbsoluteHazards`,
   resets the sword and holsters it. Test: "a retry in the Zenith restores
   the pillars and shards and clears his hazards" (Task 9).
2. **The Leviathan's death while the player is mid-air, in the flood, or
   dying in the same tick.** The Act II clear is saved on the kill. The
   ascension waits out a death (the retry restarts the Zenith, never the
   Maw). Test: "a death during the ascension retries the Zenith" (Task 5).
3. **Switching weapons mid-block, mid-swing, mid-charge or while broken.**
   Holstering resets the sword's transient state (no charged thrust on the
   gun's first click, no block kept up). Key 5 outside the Zenith does
   nothing. Test: "holstering mid-charge never fires a thrust" (Task 1) and
   "key 5 outside the Zenith keeps the gun" (Task 3).
4. **His teleports and dashes landing inside a pillar, off the edge or on a
   shard that has fallen.** Every destination goes through
   `absSafeSpot(...)`, which keeps it on the floor (r ≤ 42), out of every
   standing pillar and never on a shard. Test: "500 teleports behind a
   player standing at the edge or against a pillar all land on open floor"
   (Task 6).
5. **The F punch and the sword parry in the same tick, or F with the sword
   out.** F stays the punch/parry it always was, and both routes call one
   `parryAbsolute(e, bool sword)`, which refuses a second parry of the same
   strike. Test: "one strike can be parried once" (Task 6).

---

## File Structure

| File | Responsibility |
|---|---|
| Create `src/Sword.h` | The sword's logic: strokes, combo, charge/thrust, parry window, spam lock, block, guard meter, riposte, arc and thrust hit tests, lock-on step. |
| Create `src/EnemyAbsolute.h` | His mind: string table, phases, selection, anti-camp, teleport/dash, safe spots, parry/reel/break, blade, last judgment. Included at the end of `Enemy.h`. |
| Create `src/AbsoluteHazards.h` | Rays, the zenith ray, stomp rings, serpents, seam cells, the halo sweep, mirages, the last judgment, pillar and shard state. |
| Create `src/Gameplay_Absolute.h` | Events into hazards and damage, parry resolution, banners/lines, pillars/shards, drawing hazards and afterimages, the death. |
| Create `src/Gameplay_Sword.h` | Sword input, strokes hitting enemies, thrust, plunge, parries (projectiles, rays, strikes), block in `damagePlayerStrike`, HUD pieces. |
| Create `tests/test_zenith.cpp` | Every new test (keeps `test_game.cpp` from growing past 4.5k lines). |
| Modify `src/Enemy.h` | `EnemyType::ABSOLUTE`, attack kinds, stats row, `ab*` fields and events, `isBoss`, `parryWindow`, `deflects`, `armorMult`, `staggerTime`, tell floor, dispatch. |
| Modify `src/LevelAct2.h` | `buildZenith`: the sixth arena, pillars and shards as hidden walls registered on `LevelData`. |
| Modify `src/Level.h` | `LevelData::zenith` (centre, floor, pillar/shard wall indices and home boxes) and park/restore helpers. |
| Modify `src/Progression.h` | `Records::act2Cleared`, `bestAct2`, `bestZenith`. |
| Modify `src/Gameplay_Flow.h`, `src/Gameplay_Tick.h`, `src/Gameplay_Combat.h`, `src/Gameplay_HUD.h`, `src/Gameplay_Render.h`, `src/Gameplay_Menus.h`, `src/GameplayState.h` | The hand-off, input, damage, HUD, drawing, includes. |
| Modify `src/GunKit.h`, `src/ViewModel.h` | `buildSword`, `swordPose`, drawing the sword. |
| Modify `src/EnemyModel.h` | His rig, halo, blade, the poses of each move. |
| Modify `src/MusicSynth.h`, `src/VoiceTable.h`, `src/VoiceSynth.h`, `src/EnemyVoice.h`, `src/MixTable.h`, `tools/gen_sfx.py`, `assets/sfx/*.wav` | The ZENITH track, his voice profile and lines, sword and tell sounds. |
| Modify `src/main.cpp` | `--sword`. |
| Modify `tests/test_game.cpp`, `tests/BossSim.h`, `Makefile`, `README.md` | Count updates, his sim policy, building the new test, docs. |

---

### Task 1: The sword's logic (`Sword.h`) and the new test file

**Files:**
- Create: `src/Sword.h`
- Create: `tests/test_zenith.cpp`
- Modify: `Makefile:91-100` (the `test` target)

**Interfaces:**
- Produces (used by Tasks 2, 3, 9, 10):
  - `struct SwordInput { bool swingPressed, swingHeld, guardPressed, guardHeld; }`
  - `struct Sword` with `update(float dt, const SwordInput&) -> Sword::Out { int stroke; bool thrust; }`,
    `receive(Strike, bool fromFront, float dt = 0) -> Guarded`, `tryParry() -> bool`,
    `strokeDamage(int) -> float`, `thrustDamage() -> float`, `holster()`,
    `parryOpen()`, `blocking()`, `broken()`, `swinging()`, `moveMult()`,
    `guard01()`, statics `inArc(eye, fwd, c, r)`, `thrustHits(from, to, c, r)`,
    `lockStep(feet, fwd, target, r) -> glm::vec3`, and every constant listed
    in Global Constraints.
  - `enum class Sword::Strike { WHITE, GOLD, BEAM }`, `enum class Sword::Guarded { NONE, PARRIED, BLOCKED, BROKE }`.

- [ ] **Step 1: Create the test file with failing sword tests**

`tests/test_zenith.cpp`:

```cpp
// Headless tests for the encore: the sword (Sword.h), the Zenith, THE
// ABSOLUTE's mind and hazards, and his fight against scripted players.
// Built and run by `make test` after test_game.
#include "../src/Sword.h"
#include <cstdio>
#include <cmath>

static int failures = 0;
#define CHECK(cond, msg) do { if (!(cond)) { std::printf("FAIL: %s\n", msg); ++failures; } \
                              else { std::printf("ok:   %s\n", msg); } } while (0)
static constexpr float DT = 1.f / 60.f;

// Hold an input for t seconds
static Sword::Out run(Sword& s, SwordInput in, float t) {
    Sword::Out last; bool first = true;
    for (float u = 0.f; u < t - 1e-5f; u += DT) {
        Sword::Out o = s.update(DT, in);
        if (o.stroke >= 0) last.stroke = o.stroke;
        if (o.thrust) last.thrust = true;
        if (first) { in.swingPressed = in.guardPressed = false; first = false; }
    }
    return last;
}

int main() {
    // ---------------------------------------------------------------- the sword: strokes
    {
        Sword s;
        SwordInput click{true, true, false, false};
        Sword::Out o = s.update(DT, click);
        CHECK(o.stroke == 0 && s.swinging(), "a click starts the first stroke at once");
        CHECK(std::fabs(s.strokeDamage(0) - 70.f) < 1e-4f && std::fabs(s.strokeDamage(2) - 120.f) < 1e-4f,
              "strokes do 70, 70 and 120");
        run(s, {false, false, false, false}, Sword::SWING_TIME);
        o = s.update(DT, click);
        CHECK(o.stroke == 1, "a second click inside the combo window is the second stroke");
        run(s, {false, false, false, false}, Sword::SWING_TIME + Sword::COMBO_RESET + 0.05f);
        o = s.update(DT, click);
        CHECK(o.stroke == 0, "after 0.6 s without a swing the combo starts over");
    }
    {
        Sword s;
        s.update(DT, {true, true, false, false});
        Sword::Out o = s.update(DT, {true, true, false, false});   // clicked again mid-stroke
        CHECK(o.stroke < 0, "a click mid-stroke doesn't cut the stroke short");
        Sword::Out later = run(s, {false, false, false, false}, Sword::SWING_TIME);
        CHECK(later.stroke == 1, "...it's buffered: the next stroke follows when this one ends");
    }
    // ---------------------------------------------------------------- the sword: thrust
    {
        Sword s;
        run(s, {true, true, false, false}, Sword::CHARGE_TIME + 0.05f);
        Sword::Out o = s.update(DT, {false, false, false, false});
        CHECK(o.thrust && std::fabs(s.thrustDamage() - 220.f) < 1e-4f, "hold 0.5 s and release: a thrust for 220");
        Sword q;
        run(q, {true, true, false, false}, 0.3f);
        o = q.update(DT, {false, false, false, false});
        CHECK(!o.thrust, "released before 0.5 s: no thrust");
        Sword h;
        run(h, {true, true, false, false}, Sword::CHARGE_TIME + 0.05f);
        h.holster();
        o = h.update(DT, {false, false, false, false});
        CHECK(!o.thrust, "holstering mid-charge never fires a thrust");
    }
    // ---------------------------------------------------------------- the sword: parry, spam lock, riposte
    {
        Sword s;
        s.update(DT, {false, false, true, true});
        CHECK(s.parryOpen(), "raising the guard opens a parry window");
        CHECK(s.receive(Sword::Strike::WHITE, true) == Sword::Guarded::PARRIED, "a white strike in the window is parried");
        CHECK(std::fabs(s.strokeDamage(0) - 70.f * Sword::RIPOSTE_MULT) < 1e-3f, "the next stroke is a riposte at x2.5");
        CHECK(std::fabs(s.strokeDamage(0) - 70.f) < 1e-3f, "...and only that one");
    }
    {
        Sword s;
        s.update(DT, {false, false, true, true});
        run(s, {false, false, false, true}, 0.14f);
        CHECK(s.parryOpen(), "the window is still open at 0.14 s");
        run(s, {false, false, false, true}, 0.03f);
        CHECK(!s.parryOpen() && s.blocking(), "closed by 0.17 s: still blocking");
    }
    {
        Sword s;
        s.update(DT, {false, false, true, true});
        run(s, {false, false, false, true}, 0.2f);          // missed
        run(s, {false, false, false, false}, 0.1f);         // lowered
        s.update(DT, {false, false, true, true});           // raised again 0.1 s after the miss
        CHECK(!s.parryOpen() && s.blocking(), "a raise within 0.4 s of a missed parry blocks but can't parry");
        Sword t;
        t.update(DT, {false, false, true, true});
        run(t, {false, false, false, true}, 0.2f);
        run(t, {false, false, false, false}, 0.45f);
        t.update(DT, {false, false, true, true});
        CHECK(t.parryOpen(), "after 0.4 s the window opens again");
    }
    {
        Sword s;
        s.update(DT, {false, false, true, true});
        CHECK(s.receive(Sword::Strike::GOLD, true) == Sword::Guarded::BLOCKED, "a gold strike can't be parried: blocked");
        CHECK(s.receive(Sword::Strike::WHITE, false) == Sword::Guarded::NONE, "a strike from behind isn't guarded");
        Sword p;
        p.update(DT, {false, false, true, true});
        CHECK(p.tryParry() && !p.tryParry(), "one window parries one thing (a projectile or a ray)");
    }
    // ---------------------------------------------------------------- the sword: block and guard break
    {
        Sword s;
        s.update(DT, {false, false, true, true});
        run(s, {false, false, false, true}, 0.2f);
        s.receive(Sword::Strike::WHITE, true);
        CHECK(std::fabs(s.guard - 75.f) < 1e-3f, "a blocked white strike drains 25");
        s.receive(Sword::Strike::GOLD, true);
        CHECK(std::fabs(s.guard - 15.f) < 1e-3f, "a blocked gold strike drains 60");
        CHECK(s.receive(Sword::Strike::WHITE, true) == Sword::Guarded::BROKE && s.broken() && !s.blocking(),
              "the meter empties: guard break");
        run(s, {false, false, false, true}, 0.5f);
        CHECK(!s.blocking(), "broken: the guard can't come up for a second");
        run(s, {false, false, false, false}, Sword::BREAK_TIME);
        CHECK(!s.broken(), "...and then it can");
        float g = s.guard;
        run(s, {false, false, false, false}, 1.0f + 1.0f);
        CHECK(s.guard > g + 25.f, "the guard refills (30/s after a second down)");
        CHECK(std::fabs(Sword::BLOCK_TAKEN - 0.25f) < 1e-6f && std::fabs(s.moveMult() - 1.f) < 1e-6f,
              "a block takes 25 %; lowered, you move at full speed");
        Sword b; b.update(DT, {false, false, true, true});
        b.receive(Sword::Strike::BEAM, true, 1.f);
        CHECK(std::fabs(b.guard - 85.f) < 1e-3f && std::fabs(b.moveMult() - Sword::BLOCK_SPEED) < 1e-6f,
              "a beam drains 15 a second; blocking you move at 60 %");
        Sword n; n.update(DT, {false, false, true, true});
        Sword::Out o = n.update(DT, {true, true, false, true});
        CHECK(o.stroke < 0, "no swings while blocking");
    }
    // ---------------------------------------------------------------- the sword: reach, arc, thrust path, lock-on
    {
        glm::vec3 eye{0, 1.7f, 0}, fwd{0, 0, -1};
        CHECK(Sword::inArc(eye, fwd, {0, 1.2f, -3.f}, 0.5f), "a target 3 m ahead is in reach");
        CHECK(!Sword::inArc(eye, fwd, {0, 1.2f, -4.5f}, 0.5f), "4.5 m ahead is out of reach");
        CHECK(Sword::inArc(eye, fwd, {1.8f, 1.2f, -1.8f}, 0.5f), "45 degrees off the crosshair is in the arc");
        CHECK(!Sword::inArc(eye, fwd, {2.5f, 1.2f, 0.5f}, 0.5f), "beside and behind is not");
        CHECK(Sword::thrustHits({0, 1, 0}, {0, 1, -8}, {0.8f, 1, -5}, 0.5f), "a thrust hits what's along its path");
        CHECK(!Sword::thrustHits({0, 1, 0}, {0, 1, -8}, {3.f, 1, -5}, 0.5f), "...not what's 3 m to the side");
        glm::vec3 st = Sword::lockStep({0, 0, 0}, fwd, {0, 0, -5}, 0.8f);
        CHECK(std::fabs(glm::length(st) - Sword::STEP_DIST) < 1e-3f && st.z < 0.f, "lock-on: a stroke carries you 2 m toward him");
        st = Sword::lockStep({0, 0, 0}, fwd, {0, 0, -2.f}, 0.8f);
        CHECK(glm::length(st) < 0.25f, "...but never into him");
        st = Sword::lockStep({0, 0, 0}, fwd, {0, 0, 5}, 0.8f);
        CHECK(glm::length(st) < 1e-4f, "...and not toward something behind you");
    }

    std::printf("\n%s (%d failure%s)\n", failures ? "FAILED" : "ALL PASSED", failures, failures == 1 ? "" : "s");
    return failures ? 1 : 0;
}
```

- [ ] **Step 2: Wire the file into `make test`**

In `Makefile`, add `tests/test_zenith.cpp src/Sword.h` to the `test:` prerequisites,
then after `./tests/test_game`:

```make
	$(CXX) $(CXXFLAGS) tests/test_zenith.cpp -o tests/test_zenith $(STDLIB)
	./tests/test_zenith
```

and `tests/test_zenith` to the `clean` rule's `rm -f`.

- [ ] **Step 3: Run to see it fail**

Run: `make test`
Expected: the compile of `tests/test_zenith.cpp` fails (`src/Sword.h` not found).

- [ ] **Step 4: Write `src/Sword.h`**

```cpp
#pragma once
// =============================================================================
// Sword.h — OATHBREAKER, the sword (only in the Zenith, Gameplay_Sword.h):
// its strokes, the charged thrust, the parry window, the block and its guard
// meter, the riposte. No OpenGL or audio: GameplayState feeds it input and
// asks it questions, so the tests drive it directly.
//
//   LMB        a 3-stroke combo (70 / 70 / 120); hold 0.5 s and release: a
//              THRUST 8 m forward (220). Slam with it out: a PLUNGE.
//   RMB tap    the first 0.15 s of a raise PARRIES: a white strike, a
//              projectile, a ray just fired. The next stroke within 1 s is a
//              RIPOSTE (x2.5). A raise within 0.4 s of a missed parry can't.
//   RMB held   BLOCK: hits from the front do 25 %, you move at 60 %. Drains
//              the guard (white 25, gold 60, a beam 15/s; refills 30/s after
//              1 s down). Empty: GUARD BREAK, stunned 1 s.
// =============================================================================
#include <glm/glm.hpp>
#include <algorithm>
#include <cmath>

struct SwordInput { bool swingPressed = false, swingHeld = false, guardPressed = false, guardHeld = false; };

struct Sword {
    static constexpr float SWING_TIME = 0.30f, COMBO_RESET = 0.6f, REACH = 3.2f, ARC_DEG = 100.f,
                           STEP_RANGE = 6.f, STEP_DIST = 2.f,
                           CHARGE_TIME = 0.5f, THRUST_DIST = 8.f, THRUST_TIME = 0.2f, THRUST_DAMAGE = 220.f,
                           PLUNGE_DAMAGE = 150.f, PLUNGE_RADIUS = 4.f,
                           PARRY_WINDOW = 0.15f, SPAM_LOCK = 0.4f, RIPOSTE_TIME = 1.f, RIPOSTE_MULT = 2.5f,
                           GUARD_MAX = 100.f, DRAIN_WHITE = 25.f, DRAIN_GOLD = 60.f, DRAIN_BEAM = 15.f,
                           REFILL = 30.f, REFILL_DELAY = 1.f, BREAK_TIME = 1.f, BLOCK_TAKEN = 0.25f, BLOCK_SPEED = 0.6f,
                           BROKEN_SPEED = 0.3f,
                           HEAL_HIT = 6.f, HEAL_PARRY = 10.f;   // a gun hit heals 2 (Gameplay_Combat.h): the sword 3x
    static constexpr float strokeBase(int s) { return s >= 2 ? 120.f : 70.f; }

    enum class Strike { WHITE, GOLD, BEAM };
    enum class Guarded { NONE, PARRIED, BLOCKED, BROKE };
    struct Out { int stroke = -1; bool thrust = false; };

    int   combo = 0;           // the next stroke: 0 right-to-left, 1 left-to-right, 2 overhead
    int   lastStroke = 0;
    float swingT = 0.f;        // > 0: a stroke is in motion
    float sinceSwing = 1e9f;   // since the last stroke began
    bool  buffered = false;    // clicked mid-stroke: the next follows straight on
    float heldFor = 0.f;       // the swing button, held
    bool  charged = false;
    float thrustT = 0.f;       // > 0: thrusting (the game moves you)
    float guard = GUARD_MAX, sinceBlock = 1e9f, brokenT = 0.f;
    bool  raised = false;
    float raisedFor = 0.f;
    bool  windowLive = false;  // this raise's parry window: not yet used or missed
    float sinceMiss = 1e9f;
    float riposteT = 0.f;

    bool  broken() const    { return brokenT > 0.f; }
    bool  parryOpen() const { return raised && windowLive && raisedFor < PARRY_WINDOW; }
    bool  blocking() const  { return raised && !broken(); }
    bool  swinging() const  { return swingT > 0.f; }
    float moveMult() const  { return broken() ? BROKEN_SPEED : blocking() ? BLOCK_SPEED : 1.f; }
    float guard01() const   { return guard / GUARD_MAX; }

    // Put away (a gun drawn, a retry): nothing carries over but the meter
    void holster() {
        float g = guard, b = brokenT, m = sinceMiss;
        *this = Sword{};
        guard = g; brokenT = b; sinceMiss = m;
    }

    Out update(float dt, const SwordInput& in) {
        Out o;
        sinceSwing += dt; sinceMiss += dt;
        riposteT = std::max(0.f, riposteT - dt);
        thrustT  = std::max(0.f, thrustT - dt);
        brokenT  = std::max(0.f, brokenT - dt);
        // ---- the guard ----
        bool was = raised;
        raised = in.guardHeld && !broken();
        if (raised && (!was || in.guardPressed)) { raisedFor = 0.f; windowLive = sinceMiss >= SPAM_LOCK; }
        else if (raised) raisedFor += dt;
        if (windowLive && (!raised || raisedFor >= PARRY_WINDOW)) { windowLive = false; sinceMiss = 0.f; }   // missed
        if (raised) sinceBlock = 0.f; else sinceBlock += dt;
        if (!raised && !broken() && sinceBlock >= REFILL_DELAY) guard = std::min(GUARD_MAX, guard + REFILL * dt);
        // ---- strokes ----
        swingT = std::max(0.f, swingT - dt);
        if (raised || broken()) { heldFor = 0.f; charged = false; buffered = false; return o; }
        if (in.swingPressed) { if (swingT > 0.f) buffered = true; else startStroke(o); }
        else if (buffered && swingT <= 0.f) { buffered = false; startStroke(o); }
        if (in.swingHeld) { heldFor += dt; if (heldFor >= CHARGE_TIME) charged = true; }
        else {
            if (charged) { o.thrust = true; thrustT = THRUST_TIME; }
            heldFor = 0.f; charged = false;
        }
        return o;
    }

    // A strike reaching you. Only a white one in the window is parried; from
    // the front while raised, anything else is blocked.
    Guarded receive(Strike k, bool fromFront, float dt = 0.f) {
        if (!fromFront || !blocking()) return Guarded::NONE;
        if (k == Strike::WHITE && parryOpen()) { windowLive = false; riposteT = RIPOSTE_TIME; return Guarded::PARRIED; }
        guard -= k == Strike::WHITE ? DRAIN_WHITE : k == Strike::GOLD ? DRAIN_GOLD : DRAIN_BEAM * dt;
        if (guard <= 0.f) { guard = 0.f; brokenT = BREAK_TIME; raised = false; windowLive = false; return Guarded::BROKE; }
        return Guarded::BLOCKED;
    }
    // A projectile or a ray caught in the window
    bool tryParry() {
        if (!parryOpen()) return false;
        windowLive = false; riposteT = RIPOSTE_TIME;
        return true;
    }
    float strokeDamage(int stroke) { return spendRiposte(strokeBase(std::clamp(stroke, 0, 2))); }
    float thrustDamage() { return spendRiposte(THRUST_DAMAGE); }

    // ---- hit tests ----
    static bool inArc(glm::vec3 eye, glm::vec3 fwd, glm::vec3 c, float r) {
        glm::vec3 d = c - eye;
        float dist = glm::length(d);
        if (dist - r > REACH || std::fabs(d.y) > 2.5f + r) return false;
        glm::vec2 f{fwd.x, fwd.z}, dd{d.x, d.z};
        if (glm::length(f) < 1e-4f || glm::length(dd) < 0.3f) return true;
        float half = glm::radians(ARC_DEG * 0.5f) + std::atan2(r, std::max(glm::length(dd), 0.1f));
        return glm::dot(glm::normalize(f), glm::normalize(dd)) >= std::cos(std::min(half, 3.1415f));
    }
    static bool thrustHits(glm::vec3 from, glm::vec3 to, glm::vec3 c, float r) {
        glm::vec3 ab = to - from;
        float t = glm::clamp(glm::dot(c - from, ab) / std::max(glm::dot(ab, ab), 1e-6f), 0.f, 1.f);
        glm::vec3 q = from + ab * t;
        return glm::length(glm::vec2{c.x - q.x, c.z - q.z}) < r + 1.f && std::fabs(c.y - q.y) < 2.5f;
    }
    // A stroke's lock-on step: up to STEP_DIST toward a target within
    // STEP_RANGE and 60 degrees of the crosshair, stopping 1 m short of it
    static glm::vec3 lockStep(glm::vec3 feet, glm::vec3 fwd, glm::vec3 target, float r) {
        glm::vec2 d{target.x - feet.x, target.z - feet.z}, f{fwd.x, fwd.z};
        float l = glm::length(d);
        if (l < 1e-3f || l > STEP_RANGE || glm::length(f) < 1e-4f) return glm::vec3{0.f};
        if (glm::dot(d / l, glm::normalize(f)) < 0.5f) return glm::vec3{0.f};
        float step = std::clamp(l - r - 1.f, 0.f, STEP_DIST);
        return glm::vec3{d.x / l, 0.f, d.y / l} * step;
    }

private:
    void startStroke(Out& o) {
        if (sinceSwing > SWING_TIME + COMBO_RESET) combo = 0;
        o.stroke = lastStroke = combo;
        combo = (combo + 1) % 3;
        swingT = SWING_TIME; sinceSwing = 0.f;
    }
    float spendRiposte(float d) { if (riposteT > 0.f) { riposteT = 0.f; return d * RIPOSTE_MULT; } return d; }
};
```

- [ ] **Step 5: Run the tests**

Run: `make test`
Expected: test_physics and test_game unchanged and passing, then every `the sword:` check `ok`, ending `ALL PASSED (0 failures)`.

- [ ] **Step 6: Commit**

```bash
git add src/Sword.h tests/test_zenith.cpp Makefile
git commit -m "The sword's logic: a three-stroke combo, a charged thrust, a 0.15 s parry that can't be spammed, a block with a guard meter that breaks, and a riposte - all headless, with its own test file"
```

---

### Task 2: The sword's model and motion (`buildSword`, `swordPose`, drawing it)

**Files:**
- Modify: `src/GunKit.h` (append after `buildFist`, `GunKit.h:441`)
- Modify: `src/ViewModel.h` (a `drawSword` beside `drawFist`)
- Test: `tests/test_zenith.cpp`

**Interfaces:**
- Consumes: `Sword` (Task 1).
- Produces: `gunkit::SWORD_GLOW` (`{1.f, 0.25f, 0.45f}`),
  `struct SwordLook { float guard01 = 1.f; bool broken = false; float charge = 0.f; float riposte = 0.f; }`,
  `gunkit::buildSword(const SwordLook&, std::vector<GunPart>&)`,
  `struct SwordPose { glm::vec3 offset; float yaw, pitch, roll; }`,
  `gunkit::swordPose(int stroke, float u, bool raised, float charge01, float thrust01) -> SwordPose`
  (`u` 0..1 through the stroke, or −1 at rest),
  `ViewModel::drawSword(ShaderProgram&, const Camera&, const Sword&)`.

- [ ] **Step 1: Failing tests**

Add `#include "../src/GunKit.h"` to `tests/test_zenith.cpp`, and before the summary line:

```cpp
    // ---------------------------------------------------------------- the sword: its look and motion
    {
        std::vector<GunPart> parts;
        auto lit = [&](float g, bool broken) {
            parts.clear(); SwordLook L; L.guard01 = g; L.broken = broken;
            gunkit::buildSword(L, parts);
            int n = 0; for (auto& p : parts) n += p.tag == 1; return n;
        };
        CHECK(lit(1.f, false) == 8 && lit(0.5f, false) == 4 && lit(0.01f, false) == 1, "the fuller's 8 cells read the guard meter");
        CHECK(lit(1.f, true) == 0, "a broken guard: the fuller goes dark");
        parts.clear(); gunkit::buildSword(SwordLook{}, parts);
        glm::vec3 lo{1e9f}, hi{-1e9f};
        for (auto& p : parts) { glm::vec3 c{p.xf[3]}; lo = glm::min(lo, c); hi = glm::max(hi, c); }
        CHECK(hi.z - lo.z > 0.7f && hi.z - lo.z < 1.1f && hi.x - lo.x < 0.25f, "a blade about 0.9 m long and slim");
        bool glows = false;
        for (auto& p : parts) if (p.tag == 1 && glm::length(p.emissive - gunkit::SWORD_GLOW * 1.6f) < 0.05f) glows = true;
        CHECK(glows, "its cells glow rose-red");
        SwordPose a = gunkit::swordPose(0, 0.f, false, 0.f, 0.f), b = gunkit::swordPose(0, 1.f, false, 0.f, 0.f);
        CHECK(a.offset.x > 0.1f && b.offset.x < -0.1f, "stroke 0 sweeps from right to left");
        a = gunkit::swordPose(1, 0.f, false, 0.f, 0.f); b = gunkit::swordPose(1, 1.f, false, 0.f, 0.f);
        CHECK(a.offset.x < -0.1f && b.offset.x > 0.1f, "stroke 1 sweeps from left to right");
        a = gunkit::swordPose(2, 0.f, false, 0.f, 0.f); b = gunkit::swordPose(2, 1.f, false, 0.f, 0.f);
        CHECK(a.offset.y > b.offset.y + 0.2f, "stroke 2 comes down overhead");
        SwordPose g = gunkit::swordPose(-1, -1.f, true, 0.f, 0.f), r = gunkit::swordPose(-1, -1.f, false, 0.f, 0.f);
        CHECK(std::fabs(g.offset.x) < std::fabs(r.offset.x) && std::fabs(g.roll) > 60.f, "raised: the blade across the middle of the screen");
    }
```

- [ ] **Step 2: Run, see it fail**

Run: `make test`
Expected: compile error, `SwordLook` / `gunkit::buildSword` undeclared.

- [ ] **Step 3: Implement the model and pose in `GunKit.h`**

Append inside `namespace gunkit` after `buildFist`:

```cpp
// OATHBREAKER (Sword.h): the arsenal's body as a blade. Blackened gunmetal,
// worn steel edges, a bone grip wound with brass wire, a brass crossguard,
// and a fuller of 8 cells reading the guard meter (dark when it breaks).
// Sword space: +Z along the blade from the guard, +Y the blade's back.
const glm::vec3 SWORD_GLOW{1.f, 0.25f, 0.45f};
inline void buildSword(const SwordLook& L, std::vector<GunPart>& out) {
    Builder b{out, SWORD_GLOW};
    mat4 base(1.f);
    b.box(base, {0.f, 0.f, -0.09f}, {0.032f, 0.036f, 0.16f}, BONE);                 // grip
    for (int i = 0; i < 4; ++i) b.box(base, {0.f, 0.f, -0.15f + 0.04f * i}, {0.036f, 0.04f, 0.008f}, BRASS);   // the wire
    b.box(base, {0.f, 0.f, -0.18f}, {0.05f, 0.05f, 0.03f}, BRASS);                   // pommel
    b.box(base, {0.f, 0.f, 0.f}, {0.2f, 0.045f, 0.04f}, BRASS);                      // crossguard
    b.box(base, {0.f, 0.f, 0.45f}, {0.055f, 0.016f, 0.86f}, BLACK);                  // the blade
    b.box(base, {0.f, -0.012f, 0.45f}, {0.062f, 0.006f, 0.86f}, EDGE);               // its edge
    b.box(base, {0.f, 0.f, 0.905f}, {0.03f, 0.016f, 0.05f}, EDGE);                   // the point
    int on = L.broken ? 0 : (int)std::ceil(std::clamp(L.guard01, 0.f, 1.f) * 8.f - 1e-4f);
    float boost = 1.f + 0.8f * L.charge + 1.2f * L.riposte;
    b.cells(base, {0.f, 0.011f, 0.09f}, {0.f, 0.f, 0.095f}, {0.f, 0.f, 0.f}, 8,
            {0.018f, 0.006f, 0.07f}, 8, on, boost);
}

// Where the sword sits on screen (camera space, as hipOf) through a stroke:
// u 0..1 through it, or -1 at rest; raised: across the middle, edge up
inline SwordPose swordPose(int stroke, float u, bool raised, float charge01, float thrust01) {
    SwordPose p{{0.17f, -0.16f, 0.32f}, -8.f, 12.f, -18.f};   // at rest: low right, the point up and in
    if (raised) return {{0.02f, -0.12f, 0.36f}, 0.f, 4.f, -82.f};
    if (thrust01 > 0.f) return {{0.06f, -0.12f, 0.36f + 0.2f * thrust01}, 0.f, -2.f, -10.f};
    if (charge01 > 0.f && u < 0.f) { p.offset += glm::vec3{0.05f, 0.03f, -0.08f} * charge01; p.pitch += 30.f * charge01; return p; }
    if (u < 0.f) return p;
    float e = smooth(std::clamp(u, 0.f, 1.f));
    if (stroke == 0) return {{glm::mix(0.32f, -0.30f, e), -0.10f, 0.40f}, glm::mix(40.f, -50.f, e), 4.f, -95.f};
    if (stroke == 1) return {{glm::mix(-0.30f, 0.32f, e), -0.14f, 0.40f}, glm::mix(-50.f, 40.f, e), 4.f, 85.f};
    return {{0.04f, glm::mix(0.22f, -0.24f, e), 0.42f}, 0.f, glm::mix(-60.f, 50.f, e), -5.f};
}
```

and above `namespace gunkit` (next to `GunLook`):

```cpp
struct SwordLook { float guard01 = 1.f; bool broken = false; float charge = 0.f; float riposte = 0.f; };
struct SwordPose { glm::vec3 offset; float yaw, pitch, roll; };
```

- [ ] **Step 4: Draw it in `ViewModel.h`**

Add `#include "Sword.h"` and a public method, mirroring the gun's base-matrix code in `render`:

```cpp
    // The sword instead of the gun (Gameplay_Render.h calls this when it's out)
    void drawSword(ShaderProgram& shader, const Camera& cam, const Sword& s) {
        float u = s.swinging() ? 1.f - s.swingT / Sword::SWING_TIME : -1.f;
        float charge = std::min(1.f, s.heldFor / Sword::CHARGE_TIME) * (s.heldFor > 0.12f ? 1.f : 0.f);
        SwordPose P = gunkit::swordPose(s.lastStroke, u, s.blocking(), charge, s.thrustT > 0.f ? 1.f : 0.f);
        glm::vec3 fwd = cam.forward(), right = cam.right(), up = glm::normalize(glm::cross(right, fwd));
        glm::mat4 base(1.f);
        base[0] = glm::vec4(right, 0.f); base[1] = glm::vec4(up, 0.f); base[2] = glm::vec4(fwd, 0.f);
        base[3] = glm::vec4(cam.position + right * P.offset.x + up * P.offset.y + fwd * P.offset.z, 1.f);
        base = base * glm::rotate(glm::mat4(1.f), glm::radians(P.yaw), glm::vec3{0.f, 1.f, 0.f})
                    * glm::rotate(glm::mat4(1.f), glm::radians(P.pitch), glm::vec3{-1.f, 0.f, 0.f})
                    * glm::rotate(glm::mat4(1.f), glm::radians(P.roll), glm::vec3{0.f, 0.f, 1.f});
        glm::mat4 vmProj = glm::perspective(glm::radians(65.f), cam.aspectRatio, 0.03f, 10.f);
        glClear(GL_DEPTH_BUFFER_BIT);
        lastEmissive = glm::vec3{-1.f};
        shader.setMat4("projection", vmProj);
        shader.setMat4("view", cam.viewMatrix());
        glDisable(GL_CULL_FACE);
        SwordLook L; L.guard01 = s.guard01(); L.broken = s.broken(); L.charge = s.charged ? 1.f : 0.f;
        L.riposte = s.riposteT > 0.f ? 1.f : 0.f;
        parts.clear();
        gunkit::buildSword(L, parts);
        for (const auto& p : parts) drawPart(shader, base * p.xf, p.color, p.emissive);
        if (parryTimer > 0.f) drawFist(shader, cam, gunkit::SWORD_GLOW);   // F still punches
        shader.setVec3("emissiveColor", {0.f, 0.f, 0.f});
        lastEmissive = glm::vec3{-1.f};
        glEnable(GL_CULL_FACE);
    }
```

- [ ] **Step 5: Run the tests**

Run: `make && make test`
Expected: builds; every `the sword: its look and motion` check `ok`.

- [ ] **Step 6: Commit**

```bash
git add src/GunKit.h src/ViewModel.h tests/test_zenith.cpp
git commit -m "OATHBREAKER's look: gunmetal, bone and brass like the guns, a rose-red fuller of eight cells that reads the guard meter, and its strokes, guard and thrust on screen"
```

---

### Task 3: The sword in the game (`Gameplay_Sword.h`, input, damage, HUD, `--sword`)

**Files:**
- Create: `src/Gameplay_Sword.h`
- Modify: `src/GameplayState.h` (members, declarations, include at the end)
- Modify: `src/Gameplay_Menus.h:180-190` (key 5, wheel, LMB routing)
- Modify: `src/Gameplay_Tick.h:64-73` (plunge on a slam landing), `:126-139` (aim off, move speed with the sword out), the fire path
- Modify: `src/Gameplay_Combat.h` (`damagePlayerStrike`)
- Modify: `src/Gameplay_Render.h` (draw the sword instead of the gun), `src/Gameplay_HUD.h` (slot 5, guard bar)
- Modify: `src/main.cpp` (`--sword`), `src/Gameplay_Flow.h:153` (`enterArena` holsters)
- Test: `tests/test_zenith.cpp`

**Interfaces:**
- Consumes: `Sword`, `SwordInput`, `ViewModel::drawSword` (Tasks 1–2).
- Produces:
  - `inline bool g_devSword = false;` (globals next to `g_devWeapon`, `GameplayState.h:96`)
  - `static constexpr int ZENITH_ARENA = 5;` in `LevelAct2.h` (used here and in Tasks 4–5)
  - `GameplayState` members: `Sword sword; bool swordOut = false; bool lmbPrev = false, rmbPrev = false;`
  - `bool GameplayState::swordAllowed() const` → `g_devSword || (act2() && director.arena == ZENITH_ARENA)`
  - `void drawSword()`, `void holsterSword()`, `void updateSword(float dt)`
  - `bool damagePlayerStrike(float dmg, glm::vec3 from, Sword::Strike k, float dt = 0.f)` → applies the guard, then `damagePlayer`; returns true if any damage was taken. Sets `lastGuarded` (`Sword::Guarded`) for the caller.
  - `bool parryAbsolute(Enemy& e, bool bySword)` is **declared** here and defined in Task 9 (a no-op stub returning `false` until then).
  - Free function in `Sword.h` for the test: `inline bool swordKeyAllowed(bool devSword, bool act2, int arena, int zenithArena)`.

- [ ] **Step 1: Failing test for the key rule**

```cpp
    // ---------------------------------------------------------------- the sword: where it can be drawn
    {
        CHECK(swordKeyAllowed(false, true, 5, 5), "key 5 in the Zenith draws the sword");
        CHECK(!swordKeyAllowed(false, true, 4, 5) && !swordKeyAllowed(false, false, 5, 5),
              "key 5 outside the Zenith keeps the gun");
        CHECK(swordKeyAllowed(true, false, 0, 5), "--sword allows it anywhere (dev)");
    }
```

- [ ] **Step 2: Run, see it fail**

Run: `make test` → `swordKeyAllowed` undeclared.

- [ ] **Step 3: Add the rule to `Sword.h`**

```cpp
// Key 5: the sword only in the Zenith (or anywhere with --sword, for testing)
inline bool swordKeyAllowed(bool devSword, bool act2, int arena, int zenithArena) {
    return devSword || (act2 && arena == zenithArena);
}
```

- [ ] **Step 4: Write `src/Gameplay_Sword.h`**

```cpp
#pragma once
// =============================================================================
// Gameplay_Sword.h — GameplayState: OATHBREAKER in the game (Sword.h). Key 5
// draws it (only in the Zenith); LMB strokes and the thrust, RMB the parry
// and the block, a slam with it out a plunge. Strokes hit what's in their arc,
// heal 6 a hit (a gun hit heals 2) and can't be turned aside by a guard.
// Included at the end of GameplayState.h.
// =============================================================================

inline bool GameplayState::swordAllowed() const {
    return swordKeyAllowed(g_devSword, act2(), director.arena, ZENITH_ARENA);
}
inline void GameplayState::drawSword() {
    if (!swordAllowed() || swordOut) return;
    swordOut = true; sword.holster(); aim = 0.f;
    lmbPrev = true;   // the click that drew it isn't a stroke
    audio.play("sword_draw", 110);
}
inline void GameplayState::holsterSword() {
    if (!swordOut) return;
    swordOut = false; sword.holster();
}

inline void GameplayState::updateSword(float dt) {
    if (!swordOut) return;
    Uint32 mb = SDL_GetMouseState(nullptr, nullptr);
    bool lmb = (mb & SDL_BUTTON(SDL_BUTTON_LEFT)) != 0 || gamepad::state().fire;   // (use the gamepad's fire field as Gameplay_Tick names it)
    bool rmb = (mb & SDL_BUTTON(SDL_BUTTON_RIGHT)) != 0 || gamepad::state().aim;
    if (paused || playerDead || victory) lmb = rmb = false;
    SwordInput in{lmb && !lmbPrev, lmb, rmb && !rmbPrev, rmb};
    lmbPrev = lmb; rmbPrev = rmb;
    if (in.guardPressed) audio.play("sword_guard", 80);
    Sword::Out o = sword.update(dt, in);
    glm::vec3 eye = player.camera.position, fwd = player.camera.forward();

    // A projectile met in the parry window: sent back where you look (the punch's own rules)
    if (sword.parryOpen()) {
        int pi = findParryTarget();
        if (pi >= 0 && sword.tryParry()) { reflectProjectile(pi, fwd); styleSystem.heal(Sword::HEAL_PARRY * tune().heal); }
    }
    if (o.stroke >= 0) {
        audio.play("sword_swing", 110);
        Enemy* best = nullptr; float bd = 1e9f;
        for (auto& e : enemies) {
            if (!e.targetable()) continue;
            float d = glm::length(e.position - player.position);
            if (d < bd) { bd = d; best = &e; }
        }
        if (best) {   // the lock-on step
            glm::vec3 st = Sword::lockStep(player.position, fwd, best->position, best->radius());
            player.velocity += st / Sword::SWING_TIME * 0.5f;
        }
        fx.slashes.push_back({eye + fwd * 1.2f, std::atan2(fwd.x, fwd.z), 10 + o.stroke, 0.f});   // kinds 10-12: the player's
        bool landed = false;
        for (auto& e : enemies) {
            if (!e.targetable()) continue;
            glm::vec3 c = e.position + glm::vec3{0, std::min(e.height() * 0.5f, 1.3f), 0};
            if (!Sword::inArc(eye, fwd, c, e.radius())) continue;
            float dmg = sword.strokeDamage(o.stroke);
            hurtEnemy(e, dmg, c, 12.f, Sword::HEAL_HIT, StyleSource::MELEE);
            fx.spawnHitSparks(c, gunkit::SWORD_GLOW * 1.6f);
            landed = true;
        }
        if (landed) { audio.play("sword_hit", 120); hitStopFrames = std::max(hitStopFrames, 3); shake(0.1f, 0.02f); }
    }
    if (o.thrust) {
        glm::vec3 flat = glm::normalize(glm::vec3{fwd.x, 0.f, fwd.z} + glm::vec3{1e-4f, 0.f, 0.f});
        thrustFrom = player.position;
        player.velocity = flat * (Sword::THRUST_DIST / Sword::THRUST_TIME);
        dashMomentumTimer = std::max(dashMomentumTimer, Sword::THRUST_TIME);
        thrustHit = false;
        audio.play("sword_thrust", 120);
    }
    if (sword.thrustT > 0.f && !thrustHit) {
        for (auto& e : enemies) {
            if (!e.targetable() || !Sword::thrustHits(thrustFrom, player.position, e.position + glm::vec3{0, 1.f, 0}, e.radius())) continue;
            hurtEnemy(e, sword.thrustDamage(), e.position + glm::vec3{0, 1.2f, 0}, 25.f, Sword::HEAL_HIT, StyleSource::MELEE);
            audio.play("sword_hit", 128); hitStopFrames = std::max(hitStopFrames, 5); shake(0.2f, 0.04f);
            thrustHit = true;
            break;
        }
    }
    player.horizontalSpeed = 7.f * sword.moveMult();
}

// Called from the slam landing (Gameplay_Tick.h) with the sword out
inline void GameplayState::swordPlunge(glm::vec3 at) {
    fx.spawnShockwave(at, Sword::PLUNGE_RADIUS, gunkit::SWORD_GLOW);
    audio.play("sword_hit", 128);
    for (auto& e : enemies)
        if (e.targetable() && glm::length(glm::vec2{e.position.x - at.x, e.position.z - at.z}) < Sword::PLUNGE_RADIUS + e.radius())
            hurtEnemy(e, Sword::PLUNGE_DAMAGE, e.position + glm::vec3{0, 1.f, 0}, 20.f, Sword::HEAL_HIT, StyleSource::SLAM);
}

// Damage that a raised sword can meet: parried (white, in the window),
// blocked from the front (25 %), or breaking the guard
inline bool GameplayState::damagePlayerStrike(float dmg, glm::vec3 from, Sword::Strike k, float dt) {
    lastGuarded = Sword::Guarded::NONE;
    if (swordOut) {
        glm::vec3 to = from - player.position; to.y = 0.f;
        glm::vec3 f = player.camera.forward(); f.y = 0.f;
        bool front = glm::length(to) < 0.3f || glm::dot(glm::normalize(to), glm::normalize(f + glm::vec3{1e-4f, 0, 0})) > 0.2f;
        lastGuarded = sword.receive(k, front, dt);
        if (lastGuarded == Sword::Guarded::PARRIED) {
            parryFeedback(player.camera.position + player.camera.forward() * 1.2f, true);
            styleSystem.heal(Sword::HEAL_PARRY * tune().heal);
            return false;
        }
        if (lastGuarded != Sword::Guarded::NONE) {
            fx.spawnHitSparks(player.camera.position + player.camera.forward() * 0.8f, gunkit::SWORD_GLOW * 1.4f);
            audio.play(lastGuarded == Sword::Guarded::BROKE ? "sword_break" : "sword_block", 120);
            if (lastGuarded == Sword::Guarded::BROKE) ui.toast("GUARD BROKEN", "", {1.f, 0.3f, 0.4f}, 1.f);
            dmg *= Sword::BLOCK_TAKEN;
        }
    }
    return damagePlayer(dmg, from, 0.25f, 0.05f, k == Sword::Strike::BEAM ? 0.f : 0.15f);
}
```

`reflectProjectile(int pi, glm::vec3 fwd)` is the projectile half of `punch()`
(`Gameplay_Combat.h:65-86`), pulled out unchanged into its own method so that
`punch()` and the sword share it. `punch()` then calls
`reflectProjectile(pi, fwd); return;`. Add `StyleSource::MELEE` to
`StyleSystem.h`'s enum (label "SWORD") if it doesn't exist.

- [ ] **Step 5: Hook it in**

- `GameplayState.h`: the members and declarations from Interfaces, plus
  `glm::vec3 thrustFrom{0.f}; bool thrustHit = false; Sword::Guarded lastGuarded = Sword::Guarded::NONE;`
  and the declaration `bool parryAbsolute(Enemy& e, bool bySword);`. Include
  `"Sword.h"` at the top and `#include "Gameplay_Sword.h"` beside
  `Gameplay_Leviathan.h`. Until Task 9, `Gameplay_Sword.h` ends with
  `inline bool GameplayState::parryAbsolute(Enemy&, bool) { return false; }`.
  Task 9 moves it.
- `Gameplay_Menus.h:189`: `if (key >= SDLK_1 && key <= SDLK_4) { holsterSword(); trySwitch(key - SDLK_1); }`,
  `if (key == SDLK_5) drawSword();`. Wheel and D-pad: `holsterSword()` before
  `trySwitch`. LMB: `if (... SDL_BUTTON_LEFT && !swordOut) pendingFire = true;`.
- `Gameplay_Tick.h`: in the aim block, `if (swordOut) want = false;` and
  after it `if (swordOut) updateSword(dt);`. Skip the auto-fire/held-fire path
  when `swordOut`. In the slam landing (`:64`), after the existing slam
  damage: `if (swordOut) swordPlunge(player.position);`.
- `Gameplay_Render.h`: where `viewModel.render(...)` draws the gun, use
  `if (swordOut) viewModel.drawSword(shader, player.camera, sword); else viewModel.render(...)`.
- `Gameplay_HUD.h`: when `swordAllowed()`, draw a 5th slot after the gun
  strip ("5 OATHBREAKER", lit while out). While `swordOut && (sword.blocking() || sword.riposteT > 0.f || sword.guard < Sword::GUARD_MAX)`,
  draw a 120×4 px bar 24 px under the crosshair, filled `sword.guard01()`, in
  `SWORD_GLOW` (dark red while broken).
- `Gameplay_Flow.h` `enterArena`: `holsterSword(); sword = Sword{};` next to `grapple.release()`.
- `main.cpp`: `if (arg == "--sword") g_devSword = true;`. Document it in the
  `--play` comment block. Web: `?sword` → the same flag, wherever the shell
  maps `?god` (`web/index.html` → main's args).
- `LevelAct2.h` (top): `static constexpr int ZENITH_ARENA = 5;   // the encore: the sixth Act II arena (Gameplay_Absolute.h)`.

- [ ] **Step 6: Run and try it**

Run: `make && make test`. Expected: green.
Run: `./shooter --play --god --sword`, press 5, swing at a Husk, hold to
thrust, raise the guard against orbs. Expected: strokes kill Husks in 1–2
hits, the thrust lunges 8 m, a well-timed RMB sends an orb back, the guard
bar drains and recovers.

- [ ] **Step 7: Commit**

```bash
git add src/Gameplay_Sword.h src/GameplayState.h src/Gameplay_Menus.h src/Gameplay_Tick.h src/Gameplay_Combat.h src/Gameplay_Render.h src/Gameplay_HUD.h src/Gameplay_Flow.h src/main.cpp src/Sword.h src/StyleSystem.h src/LevelAct2.h web/index.html tests/test_zenith.cpp
git commit -m "The sword in the game: key 5 draws it in the Zenith (or anywhere with --sword), strokes with a lock-on step that heal 6 a hit, the thrust, a plunge off a slam, RMB parries orbs back and blocks from the front, and a fifth slot with a guard bar"
```

---

### Task 4: THE ZENITH (the arena, its pillars and shards, its track)

**Files:**
- Modify: `src/LevelAct2.h` (a `buildZenith(B)` called last in `buildAct2`)
- Modify: `src/Level.h` (`LevelData::Zenith`, park/restore helpers)
- Modify: `src/MusicSynth.h:44-85` (track 10, `MUSIC_TRACKS` 11)
- Modify: `tests/test_game.cpp` (Act II counts: lines 483, 3372, 4128)
- Test: `tests/test_zenith.cpp`

**Interfaces:**
- Produces:
  - In `Level.h`, inside `LevelData`:
    ```cpp
    // THE ZENITH (LevelAct2.h): its centre on the floor, and the pieces that
    // break: eight pillars (four shatter in his ascension) and six shards
    // (they fall at his blade). Hidden walls, drawn by Gameplay_Absolute.h.
    struct Zenith {
        bool built = false;
        glm::vec3 centre{0.f};   // the floor's centre (Y = the floor)
        std::vector<int> pillarWalls, shardWalls;
        std::vector<AABB> pillarHome, shardHome;
        std::vector<bool> pillarUp, shardUp;
    } zenith;
    void parkZenithPiece(bool shard, int i);   // its wall 500 m down (like a broken anchor)
    void restoreZenith();                       // every pillar and shard back
    int  zenithPillarAt(glm::vec3 p, float pad) const;   // a standing pillar within pad of p, or -1
    bool onZenithShard(glm::vec3 feet) const;
    ```
  - In `LevelAct2.h`: `constexpr float ZEN_DAIS = 9.f, ZEN_FLOOR = 44.f, ZEN_PILLAR_R = 34.f, ZEN_SHARD_IN = 52.f, ZEN_SHARD_OUT = 62.f, ZEN_HALO_R = 70.f, ZEN_HALO_UP = 40.f, ZEN_CELL = 6.f, ZEN_DAIS_UP = 1.2f;`,
    `inline glm::vec3 zenithCentre() { return {0.f, 400.f, -1400.f}; }`,
    `inline glm::vec3 zenithPillar(int k)` (r 34 at `k * 45° + 22.5°`), `inline glm::vec3 zenithShard(int k)` (r 57 at `k * 60° + 30°`).

- [ ] **Step 1: Failing tests**

Add `#include "../src/LevelAct2.h"`, `"../src/WorldGeometry.h"` and `"../src/MusicSynth.h"` to `test_zenith.cpp`:

```cpp
    // ---------------------------------------------------------------- the Zenith
    LevelData N = buildAct2Level();
    SpatialGrid g; g.build(N.walls);
    {
        CHECK(N.arenas.size() == 6 && std::string(N.arenas[ZENITH_ARENA].name) == "THE ZENITH", "a sixth Act II arena: the Zenith");
        const Arena& Z = N.arenas[ZENITH_ARENA];
        glm::vec3 C = zenithCentre();
        CHECK(Z.waves.size() == 1 && Z.waves[0][0].type == EnemyType::ABSOLUTE && Z.maxAlive == 1 && Z.space == ReverbSpace::OPEN,
              "one wave, THE ABSOLUTE, alone, under an open sky");
        CHECK(std::fabs(Z.voidY - (C.y - 40.f)) < 1e-3f && Z.hasRespawn, "the void below, a respawn on the floor");
        CHECK(N.arenaAt(Z.playerStart) == ZENITH_ARENA && N.arenaAt(zenithShard(3)) == ZENITH_ARENA,
              "the Zenith's start and shards belong to the Zenith (arenaAt is XZ-only: it must not overlap the Maw)");
        auto floorAt = [&](glm::vec3 p) { return N.groundHeightAt(p.x, p.z, p.y + 3.f); };
        bool floorOk = true;
        for (int k = 0; k < 16; ++k) for (float r : {12.f, 25.f, 40.f}) {
            float a = k * 0.3926991f;
            glm::vec3 p = C + glm::vec3{std::cos(a) * r, 0.5f, std::sin(a) * r};
            if (N.zenithPillarAt(p, 0.5f) >= 0) continue;
            if (std::fabs(floorAt(p) - C.y) > 0.05f) floorOk = false;
        }
        CHECK(floorOk, "the floor is at the centre's height from r 9 to 44, all the way round");
        CHECK(std::fabs(floorAt(C + glm::vec3{0.f, 2.f, 3.f}) - (C.y + ZEN_DAIS_UP)) < 0.05f, "the Dais is raised 1.2 m");
        CHECK(floorAt(C + glm::vec3{48.f, 2.f, 0.f}) < C.y - 30.f, "a gap past the floor's edge (the void)");
        CHECK(N.zenith.pillarWalls.size() == 8 && N.zenith.shardWalls.size() == 6, "eight pillars, six shards");
        bool shardsOk = true;
        for (int k = 0; k < 6; ++k) { glm::vec3 s = zenithShard(k); if (std::fabs(floorAt(s + glm::vec3{0, 3, 0}) - s.y) > 0.05f || !N.onZenithShard(s)) shardsOk = false; }
        CHECK(shardsOk, "each shard is a floor you can stand on");
        N.parkZenithPiece(false, 0);
        CHECK(N.zenithPillarAt(zenithPillar(0), 0.5f) < 0 && !N.zenith.pillarUp[0], "a shattered pillar is gone: no collision");
        N.parkZenithPiece(true, 2);
        CHECK(!N.onZenithShard(zenithShard(2)), "a fallen shard is gone");
        N.restoreZenith();
        CHECK(N.zenithPillarAt(zenithPillar(0), 0.5f) == 0 && N.onZenithShard(zenithShard(2)), "a retry puts them back");
        CHECK(MUSIC_TRACKS == 11 && std::string(musicTrack(10).name) == "ZENITH" && musicTrack(10).bpm >= 165.f, "the Zenith's own fast track");
    }
```

(`groundHeightAt` is whatever `GameplayState::groundHeightAt` delegates to on
`LevelData`; if it's a GameplayState method, use the free
`WorldGeometry::groundHeight(N, x, z, fromY)` it calls. Check
`WorldGeometry.h` and use its name exactly.)

In `test_game.cpp`, update:
- `:483`: `N.arenas.size() == 6`, with `std::string(N.arenas[5].name) == "THE ZENITH"`; corridors stay 4.
- `:3372`: `N.arenas.size() == 6`.
- `:4128`: `MUSIC_TRACKS == 11`.
- `:871`: done in Task 5.

- [ ] **Step 2: Run, see it fail**

Run: `make test` → `zenithCentre`, `EnemyType::ABSOLUTE`, `LevelData::zenith` undeclared.
(Add `ABSOLUTE` to `EnemyType` before `COUNT` now, with a stats row
`{"THE ABSOLUTE", 9000.f, 0.6f, 2.6f, 11.f, 0.45f, 0.9f, false, {0.95f,0.95f,0.98f}, {1.6f,1.5f,1.2f}, {1.6f,1.55f,1.4f}, "WHITE: PARRY  GOLD: DODGE OR BLOCK  RED: GET OUT"}`,
`voiceKey` "absolute", and make `isBoss` include it, so this task compiles. The
mind arrives in Task 6. `test_game.cpp:1915`'s new-types count becomes
`COUNT - 7`.)

- [ ] **Step 3: Build the arena**

In `LevelAct2.h`, after `buildMaw`:

```cpp
// ---- THE ZENITH: the encore. A floating ruin in an open sky, far above and
// past the Maw (arenaAt is XZ-only, so it can't sit straight over it). A white
// Dais, a black floor seamed with white light (the seams carry every area
// tell), eight pillars at r 34 (four shatter), six shards past a 6 m gap (they
// fall), the Halo overhead, and the shattered eclipse behind him. ----
inline void buildZenith(LevelBuilder& B) {
    LevelData& L = B.L;
    const vec3 C = zenithCentre();
    const float F = C.y;
    const vec3 white{0.86f, 0.85f, 0.82f}, black{0.05f, 0.05f, 0.06f}, gold{1.4f, 1.05f, 0.45f}, seam{1.2f, 1.2f, 1.25f};
    B.mat = Mat::STONE;
    // the floor: a 48-sided disc of slabs (collision: a grid of boxes under the
    // disc, clipped to r 44); the Dais on top
    for (float x = -44.f; x < 44.f; x += 4.f)
        for (float z = -44.f; z < 44.f; z += 4.f) {
            float cx = x + 2.f, cz = z + 2.f;
            if (cx * cx + cz * cz > 43.f * 43.f) continue;
            B.solid(C.x + x, F - 3.f, C.z + z, C.x + x + 4.f, F, C.z + z + 4.f);
        }
    B.kit().column({C.x, F - 3.f, C.z}, ZEN_FLOOR, 3.f, black, 48);
    for (float u = -42.f; u <= 42.f; u += ZEN_CELL)   // the seams: a lit grid on the floor
        B.kit(true).box(C + vec3{u, 0.015f, 0.f}, {0.06f, 0.02f, 2.f * std::sqrt(std::max(0.f, 44.f * 44.f - u * u))}, seam * 0.25f, 0.f),
        B.kit(true).box(C + vec3{0.f, 0.015f, u}, {2.f * std::sqrt(std::max(0.f, 44.f * 44.f - u * u)), 0.02f, 0.06f}, seam * 0.25f, 0.f);
    B.kit(true).curve({C.x, 0.f, C.z}, ZEN_FLOOR, 0.f, 6.2831853f, F - 0.2f, F + 0.04f, 0.3f, seam * 0.4f, 96);   // the rim
    for (float x = -9.f; x < 9.f; x += 3.f)
        for (float z = -9.f; z < 9.f; z += 3.f)
            if ((x + 1.5f) * (x + 1.5f) + (z + 1.5f) * (z + 1.5f) < 8.5f * 8.5f)
                B.solid(C.x + x, F, C.z + z, C.x + x + 3.f, F + ZEN_DAIS_UP, C.z + z + 3.f);
    B.kit().column({C.x, F, C.z}, ZEN_DAIS, ZEN_DAIS_UP, white, 40);
    B.kit(true).curve({C.x, 0.f, C.z}, ZEN_DAIS - 1.2f, 0.f, 6.2831853f, F + ZEN_DAIS_UP, F + ZEN_DAIS_UP + 0.03f, 0.25f, gold * 0.6f, 48);
    // steps up onto the Dais, every 90 degrees
    for (int k = 0; k < 4; ++k) {
        float a = k * 1.5707963f; vec3 o{std::cos(a), 0.f, std::sin(a)};
        vec3 s = C + o * (ZEN_DAIS + 0.8f);
        B.solid(s.x - 1.5f, F, s.z - 1.5f, s.x + 1.5f, F + 0.6f, s.z + 1.5f);
        B.kit().box(s + vec3{0.f, 0.3f, 0.f}, {3.f, 0.6f, 3.f}, white * 0.9f, -a);
    }
    // eight pillars and six shards: hidden walls (they break), drawn by Gameplay_Absolute.h
    L.zenith.built = true; L.zenith.centre = C;
    for (int k = 0; k < 8; ++k) {
        vec3 p = zenithPillar(k);
        int w = (int)L.walls.size();
        B.solid(p.x - 1.25f, F, p.z - 1.25f, p.x + 1.25f, F + 14.f, p.z + 1.25f);
        L.walls.back().hidden = true;
        L.zenith.pillarWalls.push_back(w); L.zenith.pillarHome.push_back(L.walls[w].box); L.zenith.pillarUp.push_back(true);
    }
    for (int k = 0; k < 6; ++k) {
        vec3 s = zenithShard(k);
        int w = (int)L.walls.size();
        B.solid(s.x - 4.f, F - 2.5f, s.z - 4.f, s.x + 4.f, F, s.z + 4.f);
        L.walls.back().hidden = true;
        L.zenith.shardWalls.push_back(w); L.zenith.shardHome.push_back(L.walls[w].box); L.zenith.shardUp.push_back(true);
    }
    // the Halo and the shattered eclipse: backdrop, never in the way
    B.kit(true).curve({C.x, 0.f, C.z}, ZEN_HALO_R, 0.f, 6.2831853f, F + ZEN_HALO_UP, F + ZEN_HALO_UP + 1.6f, 1.2f, gold * 0.9f, 128);
    vec3 E = C + vec3{0.f, 70.f, -160.f};
    B.kit().column(E, 46.f, 2.f, black, 64);   // the disc
    for (int k = 0; k < 9; ++k) {             // light through its cracks
        float a = k * 0.698f + 0.2f;
        B.kit(true).box(E + vec3{std::cos(a) * 20.f, std::sin(a) * 20.f, 1.2f}, {38.f, 0.5f, 0.2f}, seam * 0.8f, a);
    }

    Arena a;
    a.name = "THE ZENITH"; a.space = ReverbSpace::OPEN;
    a.subtitle = "ABOVE THE ECLIPSE";
    a.bounds = LevelBuilder::aabb(C.x - 64, F - 4, C.z - 64, C.x + 64, F + 30, C.z + 64);
    a.zone   = LevelBuilder::aabb(C.x - 66, F - 60, C.z - 66, C.x + 66, F + 60, C.z + 66);
    a.playerStart = C + vec3{0.f, 0.f, 38.f};
    a.startYaw = -90.f;
    a.respawn = C + vec3{0.f, 0.f, 38.f}; a.hasRespawn = true;
    a.voidY = F - 40.f;
    a.bossSpawn = C + vec3{0.f, ZEN_DAIS_UP, 0.f};
    a.groundSpawns = {C + vec3{0, 0, 20}};
    a.waves = {{{EnemyType::ABSOLUTE, 1}}};
    a.goals = {WaveGoal{}};
    a.maxAlive = 1;
    a.damageScale = 1.f;
    a.ambient = Ambient::MOTES;
    a.theme = Theme{
        {0.92f,0.9f,0.95f}, {0.55f,0.45f,0.75f}, {0.25f,0.18f,0.35f},       // a pale sky to deep violet
        glm::normalize(vec3{0.f, 0.35f, -1.f}), {1.6f,1.5f,1.3f}, 0.09f, 0.f,   // the cracked eclipse behind him
        {0.85f,0.82f,0.9f}, 0.25f,
        glm::normalize(vec3{-0.2f,-0.9f,-0.3f}), {1.15f,1.1f,1.05f},
        {0.62f,0.6f,0.68f}, {0.3f,0.28f,0.35f},
        {0.8f,0.78f,0.86f}, 0.004f };
    L.arenas.push_back(std::move(a));
}
```

Call `buildZenith(B);` at the end of `buildAct2`. Follow `Theme`'s field order
exactly as in `buildMaw`. If a field means something else, keep the intent: a
bright sky, the sun-disc behind him, cool white key light, little fog.

In `Level.h` (inside `LevelData`):

```cpp
inline void LevelData::parkZenithPiece(bool shard, int i) {
    auto& ws = shard ? zenith.shardWalls : zenith.pillarWalls;
    auto& up = shard ? zenith.shardUp : zenith.pillarUp;
    if (i < 0 || i >= (int)ws.size() || !up[i]) return;
    up[i] = false;
    AABB& b = walls[ws[i]].box; b.min.y -= 500.f; b.max.y = b.min.y + 0.01f;
}
inline void LevelData::restoreZenith() {
    for (size_t i = 0; i < zenith.pillarWalls.size(); ++i) { walls[zenith.pillarWalls[i]].box = zenith.pillarHome[i]; zenith.pillarUp[i] = true; }
    for (size_t i = 0; i < zenith.shardWalls.size(); ++i) { walls[zenith.shardWalls[i]].box = zenith.shardHome[i]; zenith.shardUp[i] = true; }
}
inline int LevelData::zenithPillarAt(glm::vec3 p, float pad) const {
    for (size_t i = 0; i < zenith.pillarWalls.size(); ++i) {
        if (!zenith.pillarUp[i]) continue;
        const AABB& b = walls[zenith.pillarWalls[i]].box;
        if (p.x > b.min.x - pad && p.x < b.max.x + pad && p.z > b.min.z - pad && p.z < b.max.z + pad) return (int)i;
    }
    return -1;
}
inline bool LevelData::onZenithShard(glm::vec3 f) const {
    for (size_t i = 0; i < zenith.shardWalls.size(); ++i) {
        if (!zenith.shardUp[i]) continue;
        const AABB& b = walls[zenith.shardWalls[i]].box;
        if (f.x > b.min.x && f.x < b.max.x && f.z > b.min.z && f.z < b.max.z && f.y > b.max.y - 0.5f && f.y < b.max.y + 3.f) return true;
    }
    return false;
}
```

(Define them as out-of-struct inline members after `LevelData`, or in-struct, whichever the file already does for similar helpers.)

`MusicSynth.h`: `MUSIC_TRACKS = 11`, and append:

```cpp
        // The Zenith: fast and bright, a driving pulse, choir stabs, distorted bells, a lead that keeps climbing
        {"ZENITH", 170.f, 50, {0, 3, 5, 6},
         "x.x.x.x.x.x.x.x.", "....x.......x.gx", "xXxXxXxXxXxXxXxo",
         "x.oxf.oxx.oxf.ox", "0123012301230123", "0---1---2---3---4---3---2---3---"},
```

(Check what the pattern letters mean in `MusicTrack` before you settle on
these. The intent is a four-on-the-floor kick, a busier hat, and a lead that
rises step by step.)

- [ ] **Step 4: Run the tests**

Run: `make && make test`. Expected: green, including `the Zenith` checks and the updated counts.

- [ ] **Step 5: Look at it**

Run: `./shooter --act2 --arena 6 --god --cam 0 412 -1350 -90 -12 --shot 30 /private/tmp/claude-501/zenith.bmp`.
Read the BMP. Expected: a bright sky, a black seamed disc, a white Dais.
(Pillars and shards are invisible until Task 9 draws them.)

- [ ] **Step 6: Commit**

```bash
git add src/LevelAct2.h src/Level.h src/MusicSynth.h src/Enemy.h src/VoiceTable.h tests/test_game.cpp tests/test_zenith.cpp
git commit -m "The Zenith: a floating ruin under an open sky - a white Dais, a black floor seamed with light, eight pillars and six shards that can break and come back, the Halo and the shattered eclipse - and its own 170 BPM track"
```

---

### Task 5: The hand-off (the Leviathan's death → the ascension → the Zenith) and the records

**Files:**
- Modify: `src/Progression.h:100-145` (`Records`)
- Modify: `src/Gameplay_Flow.h:395-397` (VICTORY / ARENA_CLEARED), `finishRun` (`:423`), `:236` (`retryArena`)
- Modify: `src/Gameplay_Tick.h:389` (corridor guard)
- Modify: `src/Gameplay_HUD.h:56-78` (the Zenith's tag), `:254-257` (the victory card), and an ascension overlay
- Modify: `src/GameplayState.h` (`ascendT`)
- Test: `tests/test_game.cpp:871`, `tests/test_zenith.cpp`

**Interfaces:**
- Consumes: `ZENITH_ARENA`, `buildZenith` (Task 4).
- Produces:
  - `Records::act2Cleared` (bool), `bestAct2`, `bestZenith` (float seconds; 0 = none), saved as `act2Cleared`, `bestAct2`, `bestZenith`.
  - `inline void recordAct2Clear(Records& r, float t)` and `inline bool recordZenith(Records& r, float t)` (true if a new best). Pure, in `Progression.h`.
  - `GameplayState::ascendT` (−1 off, else seconds into the ascension), `ASCEND_SINK = 3.f, ASCEND_CARD = 2.f, ASCEND_WHITE = 1.f`.
  - `inline bool zenithHandoff(bool act2, int clearedArena, int arenaCount)`: true when the arena just cleared is the Maw and a Zenith follows.

- [ ] **Step 1: Failing tests**

`test_zenith.cpp` (include `"../src/Progression.h"`, `"../src/WaveDirector.h"`):

```cpp
    // ---------------------------------------------------------------- the hand-off and the records
    {
        Records r;
        recordAct2Clear(r, 1500.f);
        CHECK(r.act2Cleared && std::fabs(r.bestAct2 - 1500.f) < 1e-3f, "the Leviathan's death saves the Act II clear and its time");
        recordAct2Clear(r, 1600.f);
        CHECK(std::fabs(r.bestAct2 - 1500.f) < 1e-3f, "a slower clear doesn't replace the best");
        CHECK(recordZenith(r, 1700.f) && !recordZenith(r, 1800.f) && std::fabs(r.bestZenith - 1700.f) < 1e-3f, "ACT II + ZENITH keeps its own best");
        CHECK(zenithHandoff(true, 4, 6) && !zenithHandoff(true, 3, 6) && !zenithHandoff(false, 4, 6) && !zenithHandoff(true, 4, 5),
              "only the Maw's clear in Act II hands off to the Zenith");
        WaveDirector d; d.level = &N; d.startArena(4);
        // the Maw cleared by hand: the director must stop at CLEARED, not VICTORY
        d.phase = WaveDirector::Phase::ACTIVE; d.queue.clear();
        std::vector<SpawnRequest> out; d.update(DT, 0, N.arenas[4].playerStart, out);
        CHECK(d.phase == WaveDirector::Phase::CLEARED && d.arena == 4, "with the Zenith after it, the Maw ends CLEARED (no victory yet)");
        d.startArena(ZENITH_ARENA);
        CHECK(d.arena == ZENITH_ARENA, "the Zenith starts by hand (enterArena), not by walking in");
    }
```

(If `queue` is private on `WaveDirector`, drive the Maw to its clear the way
`test_game.cpp`'s Act II run does, then assert the phase.)

`test_game.cpp:870`: assert the simulated run ends with
`d.phase == WaveDirector::Phase::CLEARED && d.arena == 4`. Rename it
"...kills the Leviathan, and the Zenith waits above". Add a loop that calls
`d.startArena(5)`, kills the one boss the same way, and checks
`d.phase == VICTORY && d.arena == 5` ("...and the Absolute's death ends the act").

- [ ] **Step 2: Run, see it fail**

`make test` → `recordAct2Clear` undeclared; the Act II run check now fails at phase VICTORY ≠ CLEARED (after Task 4 the Maw is no longer last, so this may already read CLEARED: then only the missing symbols fail).

- [ ] **Step 3: Records**

In `Progression.h`'s `Records`: the three fields; load keys `act2Cleared`
(int), `bestAct2`, `bestZenith`; saved the same way as `act2Unlocked`. Then:

```cpp
// ACT II's clear, saved the moment the Leviathan dies (whatever happens above)
inline void recordAct2Clear(Records& r, float t) {
    r.act2Cleared = true;
    if (r.bestAct2 <= 0.f || t < r.bestAct2) r.bestAct2 = t;
}
inline bool recordZenith(Records& r, float t) {
    if (r.bestZenith > 0.f && t >= r.bestZenith) return false;
    r.bestZenith = t; return true;
}
inline bool zenithHandoff(bool act2, int clearedArena, int arenaCount) {
    return act2 && arenaCount == 6 && clearedArena == 4;
}
```

- [ ] **Step 4: The ascension in the game**

- `Gameplay_Flow.h`, in the director-event switch, `ARENA_CLEARED`:
  ```cpp
  if (zenithHandoff(act2(), ev.arena, (int)level.arenas.size())) {
      if (!g_godMode && !g_practice) { recordAct2Clear(records, elapsedTime); records.save(); }
      ascendT = 0.f;   // the Leviathan sinks (3 s), ACT II COMPLETE (2 s), white-out (1 s), then the Zenith
  }
  ```
- A new `updateAscension(float dt)`, called from the tick next to
  `updateSanctumPhase`:
  ```cpp
  inline void GameplayState::updateAscension(float dt) {
      if (ascendT < 0.f || playerDead) return;
      float was = ascendT; ascendT += dt;
      if (was < ASCEND_SINK && ascendT >= ASCEND_SINK) audio.play("wave", 128, SoundGroup::UI);
      if (was < ASCEND_SINK + ASCEND_CARD && ascendT >= ASCEND_SINK + ASCEND_CARD) {
          pushBanner("THE ECLIPSE BREAKS", "SOMETHING ABOVE IT DOES NOT", {1.f, 0.95f, 0.85f}, 3.f);
          audio.duck(8.f, 1.5f);
      }
      if (ascendT >= ASCEND_SINK + ASCEND_CARD + ASCEND_WHITE) { ascendT = -1.f; enterArena(ZENITH_ARENA); captureMouse(true); }
  }
  ```
  `damagePlayer` refuses damage while `ascendT >= 0`. `enterArena` sets
  `ascendT = -1.f` (a retry never replays it).
- `retryArena`: if `ascendT >= 0` (died in the Maw after the kill, e.g. to a
  lingering spit), skip the rest of the ascension and `enterArena(ZENITH_ARENA)`.
  The existing `cleared` branch already does this (`director.arena + 1` when
  CLEARED); make sure it lands on 5. Add the test "a death during the
  ascension retries the Zenith" by asserting
  `retryTarget(WaveDirector::Phase::CLEARED, 4) == 5`, after extracting that
  arithmetic into `inline int retryTarget(WaveDirector::Phase p, int arena)` in
  `WaveDirector.h`.
- `Gameplay_Tick.h:389`: `if (director.arena < (int)level.corridors.size()) zones.push_back(&level.corridors[director.arena]);`
- `Gameplay_HUD.h`: during `ascendT` in [ASCEND_SINK, ASCEND_SINK + ASCEND_CARD)
  draw the title card "ACT II COMPLETE" / "THE ECLIPSE BREAKS" with the run
  time, reusing the victory card's title style but no stats or buttons. In
  the last second, a full-screen white quad with alpha `(ascendT - SINK - CARD) / WHITE`;
  then over the Zenith's first second it fades back out (`whiteFade`, set by
  `enterArena` when the arena is the Zenith). The top line in the Zenith
  reads `"ENCORE  THE ZENITH"` instead of `"ACT II 6/6 ..."`, and other
  Act II arenas show `/5` (`nArenas = act2() ? 5 : ...`).
- The victory: `DirectorEvent::VICTORY` in the Zenith, so `victoryDelay` = 3 s
  (he kneels and dissolves, Task 9). In `finishRun`, if
  `act2() && director.arena == ZENITH_ARENA && !g_godMode && !g_practice`,
  call `newRecord = recordZenith(records, elapsedTime); records.save();`.
  The victory card: `director.arena == ZENITH_ARENA ? "ZENITH CLEARED" : "ACT II COMPLETE"`,
  with sub `"THE LIGHT FALLS"` / `"THE ECLIPSE BREAKS"`, and
  `records.bestZenith` shown as the best (in place of the `0.f`).
- The main menu's ACT II row: when `records.bestZenith > 0`, the subtitle
  reads `"5 ARENAS - AND WHAT WAITS ABOVE"`.

- [ ] **Step 5: Run**

`make && make test` → green.
`./shooter --act2 --arena 5 --god --overlay leviathan3`: kill the Leviathan.
Expected: it sinks, the ACT II COMPLETE card shows, the banner and white-out
follow, and you stand in the Zenith with "ENCORE THE ZENITH" on top. (The
Absolute isn't built yet, so the arena is empty.)

- [ ] **Step 6: Commit**

```bash
git add src/Progression.h src/WaveDirector.h src/Gameplay_Flow.h src/Gameplay_Tick.h src/Gameplay_HUD.h src/GameplayState.h src/MenuState.h tests/test_game.cpp tests/test_zenith.cpp
git commit -m "The Leviathan's death saves Act II's clear, then the eclipse breaks and you rise into the Zenith in the same run; a death up there retries the Zenith; the act's records gain ACT II and ACT II + ZENITH times"
```

---

### Task 6: THE ABSOLUTE, phase 1 (the string table, the colour rule, parries, teleports, anti-camp)

**Files:**
- Create: `src/EnemyAbsolute.h`
- Modify: `src/Enemy.h` (attack kinds, fields, events, `parryWindow`, `deflects`, `armorMult`, `staggerTime`, the tell floor in `startAttack`, dispatch, include)
- Modify: `src/EnemyWorld` in `Enemy.h` (Zenith info: `hasZenith`, `zenCentre`, `zenFloor`, `pillars`/`pillarUp` pointers, `onShard` flag)
- Test: `tests/test_zenith.cpp`

**Interfaces:**
- Produces:
  - `AttackKind` += `ATELEPORT, ADASH, AJAB, AHOOK, AUPPER, ADROP, AHOP, ARAY, ASTOMP, ASERPENT, AHALO, ASEAMS, AZENITH, AMIRAGE, ASWEEP, ALUNGE, ACRESCENT, AJUDGE`.
  - `enum class AbTell : int8_t { NONE = -1, WHITE = 0, GOLD = 1, RED = 2 };` and `inline AbTell abTellOf(AttackKind)`.
  - `struct AbStep { AttackKind k; float tell, damage; int minPhase; float late; };` and
    `struct AbString { const char* name; int line; int phases; AbStep steps[7]; int n; };`
    (`phases` is a bitmask of the phases it's used in; `late` is how much later than its flash a delayed strike lands).
  - `const AbString& abString(int id)`, the ids `AB_TOO_SLOW = 0, AB_KNEEL = 1, AB_JUDGED = 2, AB_SERPENT = 3, AB_SEAMS = 4, AB_ZENITH = 5, AB_SWEEPS = 6, AB_LUNGE = 7, AB_CRESCENTS = 8, AB_STRINGS = 9`.
  - Lines: `inline const char* abLine(int)`: 0 "KNEEL.", 1 "TOO SLOW.", 2 "AGAIN.", 3 "BE JUDGED.", 4 "...WORTHY.".
  - Enemy fields: as listed in Step 3.
  - Events: `ev.abTell` (int, −1 none), `ev.abPhase`, `ev.abLine`, `ev.abRay`, `ev.abStomp`, `ev.abSerpent`, `ev.abHalo`, `ev.abSeams` + `ev.abSeamFirst`, `ev.abZenith`, `ev.abMirages`, `ev.abBladeDrawn`, `ev.abBladeBroke`, `ev.abJudgeMark`, `ev.abJudge`, `ev.abLaunch`, `ev.abShards`, `ev.abShatter` (a mask of the pillars hit), `ev.abFrom`, `ev.abTo`; plus the shared `ev.blinked/blinkFrom` (afterimages), `ev.meleeHit/meleeDamage`, `ev.shots` (crescents).
  - `Enemy::abParried(bool bySword) -> int` (0 refused, 1 reeled, 2 broken, 3 blade shattered).
  - `inline glm::vec3 absSafeSpot(const EnemyWorld& w, glm::vec3 want, glm::vec3 fallback)`.

- [ ] **Step 1: Failing tests (phase 1)**

```cpp
    // ---------------------------------------------------------------- THE ABSOLUTE: phase 1
    const Arena& ZA = N.arenas[ZENITH_ARENA];
    const glm::vec3 ZC = zenithCentre();
    auto zworld = [&](glm::vec3 feet) {
        EnemyWorld w; w.playerFeet = feet; w.playerEye = feet + glm::vec3{0, 1.7f, 0};
        w.walls = N.walls.data(); w.wallCount = (int)N.walls.size(); w.grid = &g; w.bounds = ZA.bounds;
        w.hasZenith = true; w.zenCentre = ZC; w.zenFloor = ZC.y; w.level = &N;
        return w;
    };
    auto freshAbs = [&]() { Enemy e(EnemyType::ABSOLUTE, ZA.bossSpawn); e.spawnTimer = 0.f; e.state = EnemyState::ACTIVE; return e; };
    {
        // the colour rule: every strike one kind; every white one parryable, no gold or red one
        bool rule = true;
        for (int id = 0; id < AB_STRINGS; ++id) {
            const AbString& s = abString(id);
            for (int k = 0; k < s.n; ++k) {
                AbTell t = abTellOf(s.steps[k].k);
                if (s.steps[k].damage > 0.f && t == AbTell::NONE) rule = false;
            }
        }
        CHECK(rule, "every damaging move in every string has a tell colour");
        CHECK(abTellOf(AttackKind::AJAB) == AbTell::WHITE && abTellOf(AttackKind::AHOOK) == AbTell::WHITE &&
              abTellOf(AttackKind::AUPPER) == AbTell::GOLD && abTellOf(AttackKind::ADROP) == AbTell::GOLD &&
              abTellOf(AttackKind::ARAY) == AbTell::RED && abTellOf(AttackKind::ASTOMP) == AbTell::RED &&
              abTellOf(AttackKind::ASERPENT) == AbTell::WHITE && abTellOf(AttackKind::ASWEEP) == AbTell::WHITE &&
              abTellOf(AttackKind::ALUNGE) == AbTell::GOLD && abTellOf(AttackKind::ACRESCENT) == AbTell::WHITE,
              "white: jab, hook, serpent, sweep, crescent; gold: uppercut, drop-kick, lunge; red: ray, stomp");
        float full = 0.f;
        for (int id : {AB_TOO_SLOW, AB_KNEEL, AB_JUDGED}) {
            float s = 0.f; const AbString& st = abString(id);
            for (int k = 0; k < st.n; ++k) if (st.steps[k].minPhase <= 1) s += st.steps[k].k == AttackKind::ARAY ? st.steps[k].damage * 1.f : st.steps[k].damage;
            full = std::max(full, s);
        }
        CHECK(full >= 60.f && full <= 70.f, "a whole phase 1 string taken costs 60-70");
        CHECK(statsOf(EnemyType::ABSOLUTE).health == 9000.f && isBoss(EnemyType::ABSOLUTE), "9000 hp, a boss");
    }
    {
        // his arrival: 2 s untouchable on the Dais, then a string
        Enemy e = freshAbs();
        glm::vec3 me = ZC + glm::vec3{0, 0, 20};
        e.update(DT, zworld(me));
        CHECK(!e.targetable(), "arriving: untargetable");
        for (int i = 0; i < 125; ++i) e.update(DT, zworld(me));
        CHECK(e.targetable(), "after 2 s he's in the fight");
        bool started = false;
        for (int i = 0; i < 120 && !started; ++i) { e.update(DT, zworld(me)); started = e.abCur >= 0; }
        CHECK(started, "...and opens a string within 2 s");
    }
    {
        // TOO SLOW: teleport behind, jab jab hook, all white; two parries break him
        Enemy e = freshAbs(); e.abArrive = 0.f;
        glm::vec3 me = ZC + glm::vec3{0, 0, 20};
        e.abForce = AB_TOO_SLOW;
        bool blinked = false; int whites = 0; int parried = 0; float reeled = 0.f;
        for (int i = 0; i < 600; ++i) {
            e.update(DT, zworld(me));
            if (e.ev.blinked) blinked = true;
            if (e.ev.abTell == (int)AbTell::WHITE) ++whites;
            if (e.parryWindow() && parried < 2) {
                int r = e.abParried(false);
                ++parried;
                if (parried == 1) { CHECK(r == 1 && e.staggered() && e.abReeling && std::fabs(e.armorMult() - 1.5f) < 1e-4f, "the first parry: he reels (x1.5)"); reeled = e.staggerTimer; }
                if (parried == 2) CHECK(r == 2 && e.staggered() && !e.abReeling && std::fabs(e.armorMult() - 2.f) < 1e-4f && std::fabs(e.staggerTimer - 3.f) < 0.05f,
                                        "the second in one string breaks him: 3 s at x2");
            }
            if (parried >= 2) break;
        }
        CHECK(blinked, "TOO SLOW opens with a teleport (afterimage)");
        CHECK(whites >= 2 && std::fabs(reeled - Enemy::AB_REEL) < 0.05f, "its strikes flash white; a reel lasts 0.8 s");
        CHECK(e.abParried(false) == 0, "one strike can be parried once (a stagger can't be parried again)");
    }
    {
        // gold can't be parried
        Enemy e = freshAbs(); e.abArrive = 0.f; e.abForce = AB_KNEEL;
        glm::vec3 me = ZC + glm::vec3{0, 0, 12};
        bool goldWindow = false, launched = false;
        for (int i = 0; i < 400; ++i) {
            e.update(DT, zworld(me));
            if (abTellOf(e.attack) == AbTell::GOLD && e.parryWindow()) goldWindow = true;
            if (e.ev.abLaunch) launched = true;
        }
        CHECK(!goldWindow, "no parry window ever opens on a gold strike");
        CHECK(launched, "KNEEL's uppercut launches you (when it lands)");
    }
    {
        // the tell floors, every difficulty, every string
        float least = 1e9f, leastFollow = 1e9f;
        for (int d = 0; d < DIFFICULTY_COUNT; ++d)
            for (int id = 0; id < AB_STRINGS; ++id) {
                Enemy e = freshAbs(); e.abArrive = 0.f; e.abForce = id;
                EnemyWorld w = zworld(ZC + glm::vec3{0, 0, 14}); w.tune = &difficulty(d);
                for (int i = 0; i < 600; ++i) {
                    e.update(DT, w);
                    if (e.ev.telegraphStarted && abTellOf(e.attack) != AbTell::NONE)
                        (e.tellFollow ? leastFollow : least) = std::min(e.tellFollow ? leastFollow : least, e.telegraphDuration);
                }
            }
        CHECK(least >= Enemy::TELL_FLOOR - 1e-4f && leastFollow >= Enemy::AB_FOLLOW_FLOOR - 1e-4f,
              "every Absolute tell is at least 0.35 s (0.22 s within a string) on every difficulty");
    }
    {
        // anti-camp: a shard, far away, or the same pillar for 3 s brings him to you
        auto campTest = [&](glm::vec3 me, bool shard) {
            Enemy e = freshAbs(); e.abArrive = 0.f;
            EnemyWorld w = zworld(me); w.onShard = shard;
            for (int i = 0; i < 5 * 60; ++i) {
                e.update(DT, w);
                if (e.ev.blinked && glm::length(glm::vec2{e.position.x - me.x, e.position.z - me.z}) < 6.f) return i * DT;
            }
            return 99.f;
        };
        float far = campTest(ZC + glm::vec3{0, 0, 42.f}, false);
        glm::vec3 pil = zenithPillar(0); glm::vec3 out = glm::normalize(glm::vec3{pil.x - ZC.x, 0, pil.z - ZC.z});
        float behind = campTest(pil + out * 2.f, false);
        CHECK(far < 5.f && behind < 5.f, "far away or behind a pillar: within 5 s he's on you (TOO SLOW)");
    }
    {
        // teleports and dashes land on open floor
        srand(77);
        int bad = 0;
        for (int i = 0; i < 500; ++i) {
            EnemyWorld w = zworld(ZC);
            float a = frand(0.f, 6.283f), r = frand(0.f, 44.f);
            glm::vec3 want = ZC + glm::vec3{std::cos(a) * r, 0.f, std::sin(a) * r} + glm::vec3{frand(-4, 4), 0, frand(-4, 4)};
            glm::vec3 p = absSafeSpot(w, want, ZC);
            float pr = glm::length(glm::vec2{p.x - ZC.x, p.z - ZC.z});
            if (pr > 42.01f || N.zenithPillarAt(p, 0.7f) >= 0) ++bad;
        }
        CHECK(bad == 0, "500 teleports behind a player standing at the edge or against a pillar all land on open floor");
    }
    {
        // his guard: gunfire from beyond 14 m turned aside while he's idle
        Enemy e = freshAbs(); e.abArrive = 0.f; e.yaw = 0.f;
        CHECK(e.deflects({0, 0, -1}, 20.f) && !e.deflects({0, 0, -1}, 10.f), "beyond 14 m his guard turns shots aside; closer they land");
        e.stagger(1.f);
        CHECK(!e.deflects({0, 0, -1}, 20.f), "staggered, he's open from anywhere");
    }
```

- [ ] **Step 2: Run, see it fail**

`make test` → missing symbols (`abString`, `AbTell`, `abParried`, `EnemyWorld::hasZenith` ...).

- [ ] **Step 3: Enemy.h additions**

Attack kinds (after `SUBMERGE`):
```cpp
                        ATELEPORT, ADASH, AJAB, AHOOK, AUPPER, ADROP, AHOP, ARAY, ASTOMP, ASERPENT,   // THE ABSOLUTE's
                        AHALO, ASEAMS, AZENITH, AMIRAGE, ASWEEP, ALUNGE, ACRESCENT, AJUDGE };
```
`EnemyWorld`:
```cpp
    // THE ABSOLUTE's Zenith: its floor's centre, the level (its pillars), and
    // whether the player stands on a shard
    bool hasZenith = false; glm::vec3 zenCentre{0.f}; float zenFloor = 0.f;
    const LevelData* level = nullptr; bool onShard = false;
```
`EnemyWorld` is in `Enemy.h`, which only includes `Player.h`. Forward-declare
`struct LevelData;` above it, and have `EnemyAbsolute.h` include `Level.h`.
Check that `Level.h` doesn't include `Enemy.h` (it includes `WaveDirector`
types?). If it does, pass a small `const ZenithView*` instead: pillar boxes +
up flags, filled by the caller.

`EnemyEvents`:
```cpp
    // THE ABSOLUTE (EnemyAbsolute.h): the colour of the strike winding up (0
    // white, 1 gold, 2 red), a phase change, a line to show, a ray fired from
    // abFrom toward abTo, a stomp ring, a serpent, the halo sweep, the seams
    // (which parity burns first), a zenith ray, mirages, the blade drawn or
    // shattered, the last judgment marked / landing, an uppercut's launch,
    // the shards falling, pillars to shatter (a mask)
    int  abTell = -1, abPhase = 0, abLine = -1, abSeamFirst = 0, abMirages = 0, abShatter = 0;
    bool abRay = false, abStomp = false, abSerpent = false, abHalo = false, abSeams = false, abZenith = false,
         abBladeDrawn = false, abBladeBroke = false, abJudgeMark = false, abJudge = false, abLaunch = false, abShards = false;
    glm::vec3 abFrom{0.f}, abTo{0.f};
```
`Enemy` fields:
```cpp
    // THE ABSOLUTE (EnemyAbsolute.h)
    static constexpr float AB_PHASE2 = 0.60f, AB_PHASE3 = 0.25f, AB_JUDGE_AT = 0.08f, AB_ARRIVE = 2.f, AB_REEL = 0.8f,
                           AB_BREAK = 3.f, AB_SPENT = 4.f, AB_DEFLECT = 14.f, AB_CAMP = 3.f, AB_FAR = 30.f, AB_RUN = 11.f,
                           AB_DASH = 32.f, AB_REACH = 2.8f, AB_FOLLOW_FLOOR = 0.22f, AB_QUICK = 0.75f, AB_RANGE = 20.f,
                           AB_BLADE_BACK = 3.f;
    int   abPhase = 1;
    int   abCur = -1, abStep = 0;      // the string running (an AB_* id), the move in it
    int   abForce = -1;                // tests / dev: the next string
    int   abParries = 0;               // white strikes parried in this string
    bool  abReeling = false;           // this stagger is a reel (x1.5), not a break (x2)
    bool  abParriedThis = false;       // the current strike has been parried
    float abArrive = AB_ARRIVE;
    float abCamp = 0.f; int abCampPillar = -1;
    int   abRot = 0;                   // the rotation
    bool  abBlade = false; float abBladeGone = 0.f;
    bool  abJudged = false;
    float abLate = 0.f;                // a delayed strike: how long after its flash it lands
    glm::vec3 abAim{0.f};
    glm::vec3 abCentreCache{0.f};      // the Zenith's centre (set each tick from the world)
    AttackKind lastAbAttack = AttackKind::NONE;   // the strike that just resolved (the game reads its colour)
```
- `parryWindow()`: `if (type == EnemyType::ABSOLUTE) return abTellOf(attack) == AbTell::WHITE && attack != AttackKind::ASERPENT && attack != AttackKind::ACRESCENT && !abParriedThis && telegraphTimer < 0.25f;`.
  (A delayed strike's window is its last 0.25 s too, because its `telegraphTimer` includes `late`.)
- `deflects()`: accept `ABSOLUTE` with `AB_DEFLECT`. Add the condition
  `type == EnemyType::ABSOLUTE ? dist < AB_DEFLECT : dist < DEFLECT_RANGE`. It's
  open while telegraphing/recovering as the Sovereign's guard is. No facing
  check for him (he guards all round).
- `armorMult()`: `if (type == EnemyType::ABSOLUTE) return staggered() ? (abReeling ? 1.5f : 2.f) : 1.f;`.
- `staggerTime()`: `ABSOLUTE ? AB_BREAK`.
- `startAttack`: add `ABSOLUTE` to the floor clamp, with
  `followUp ? (type == EnemyType::ABSOLUTE ? AB_FOLLOW_FLOOR : TELL_FLOOR_FOLLOW) : TELL_FLOOR`.
- `targetable()`: `&& !(type == EnemyType::ABSOLUTE && abArrive > 0.f)`.
- `stagger()` resets `abCur = -1; abStep = 0;` (not `abParries`).
- `update()`: in the stagger branch, `if (type == EnemyType::ABSOLUTE) { if (staggerTimer <= 0.f) abReeling = false; return; }` (he doesn't slide).
  Dispatch: `case EnemyType::ABSOLUTE: thinkAbsolute(dt, w, resolve); return;`.
  Declare `void thinkAbsolute(float dt, const EnemyWorld& w, bool resolve); int abParried(bool bySword);` next to `thinkLeviathan`.
- `#include "EnemyAbsolute.h"` at the end, after `EnemyRelic.h`.
- `isBoss`: add `ABSOLUTE`. `takeDamage`'s enrage-at-50 % rule: exclude ABSOLUTE (it has its own phases).

- [ ] **Step 4: Write `src/EnemyAbsolute.h` (phase 1; phases 2–3 in Task 8)**

```cpp
#pragma once
// =============================================================================
// EnemyAbsolute.h — THE ABSOLUTE's mind (Enemy::thinkAbsolute), included at
// the end of Enemy.h. He fights in STRINGS: named combos of moves, each move
// with one tell colour (white: parry it; gold: dodge or block it; red: get out
// of where it lands). It only decides and emits events (EnemyEvents::ab*);
// AbsoluteHazards.h and Gameplay_Absolute.h turn them into what hurts you.
//
//   JUDGMENT  (100-60 %)  TOO SLOW, KNEEL, BE JUDGED, SERPENT at range.
//   ASCENSION (60-25 %)   a 360 sweep at chest height (slide), four pillars
//                         shatter; every string a hit longer and 25 % quicker;
//                         MIRAGE, SEAMS, the ZENITH ray for shard campers;
//                         delayed strikes (the window is on the hit).
//   ABSOLUTE  (< 25 %)    the blade: SWEEPS, LUNGE, CRESCENTS; a sword parry
//                         shatters it; at 8 % the LAST JUDGMENT (stand by him).
// Parry a white strike: he reels 0.8 s (x1.5); twice in one string: broken
// 3 s (x2). Camp (a shard, 30 m off, one pillar) 3 s: he comes for you.
// =============================================================================
#include "Level.h"

enum class AbTell : int8_t { NONE = -1, WHITE = 0, GOLD = 1, RED = 2 };
inline AbTell abTellOf(AttackKind k) {
    switch (k) {
        case AttackKind::AJAB: case AttackKind::AHOOK: case AttackKind::ASERPENT:
        case AttackKind::ASWEEP: case AttackKind::ACRESCENT: return AbTell::WHITE;
        case AttackKind::AUPPER: case AttackKind::ADROP: case AttackKind::ALUNGE: return AbTell::GOLD;
        case AttackKind::ARAY: case AttackKind::ASTOMP: case AttackKind::AHALO: case AttackKind::ASEAMS:
        case AttackKind::AZENITH: case AttackKind::AJUDGE: return AbTell::RED;
        default: return AbTell::NONE;
    }
}

struct AbStep { AttackKind k; float tell, damage; int minPhase; float late; };
struct AbString { const char* name; int line; int phases; AbStep steps[7]; int n; };
enum { AB_TOO_SLOW, AB_KNEEL, AB_JUDGED, AB_SERPENT, AB_SEAMS, AB_ZENITH, AB_SWEEPS, AB_LUNGE, AB_CRESCENTS, AB_STRINGS };
constexpr int AB_P1 = 1, AB_P2 = 2, AB_P3 = 4;

inline const AbString& abString(int id) {
    using A = AttackKind;
    static const AbString S[AB_STRINGS] = {
        {"TOO SLOW", 1, AB_P1 | AB_P2 | AB_P3, {{A::ATELEPORT, 0.25f, 0.f, 1, 0.f}, {A::AJAB, 0.45f, 15.f, 1, 0.f},
                                              {A::AJAB, 0.30f, 15.f, 1, 0.f}, {A::AJAB, 0.30f, 15.f, 2, 0.2f},
                                              {A::AHOOK, 0.40f, 22.f, 1, 0.f}}, 5},
        {"KNEEL", 0, AB_P1 | AB_P2 | AB_P3, {{A::ADASH, 0.35f, 0.f, 1, 0.f}, {A::AUPPER, 0.50f, 20.f, 1, 0.f},
                                            {A::ADROP, 0.45f, 28.f, 1, 0.f}, {A::ADROP, 0.40f, 28.f, 2, 0.f}}, 4},
        {"BE JUDGED", 3, AB_P1 | AB_P2 | AB_P3, {{A::AHOP, 0.20f, 0.f, 1, 0.f}, {A::ARAY, 0.60f, 25.f, 1, 0.f},
                                                {A::ARAY, 0.50f, 25.f, 2, 0.f}, {A::ASTOMP, 0.50f, 20.f, 1, 0.f}}, 4},
        {"SERPENT", -1, AB_P1 | AB_P2, {{A::ASERPENT, 0.50f, 20.f, 1, 0.f}}, 1},
        {"SEAMS", 3, AB_P2 | AB_P3, {{A::ASEAMS, 0.90f, 25.f, 2, 0.f}}, 1},
        {"ZENITH", -1, AB_P2, {{A::AZENITH, 0.60f, 20.f, 2, 0.f}}, 1},
        {"SWEEPS", 2, AB_P3, {{A::ADASH, 0.35f, 0.f, 3, 0.f}, {A::ASWEEP, 0.40f, 22.f, 3, 0.f},
                              {A::ASWEEP, 0.30f, 22.f, 3, 0.2f}, {A::ASWEEP, 0.30f, 22.f, 3, 0.f}}, 4},
        {"LUNGE", -1, AB_P3, {{A::ALUNGE, 0.50f, 30.f, 3, 0.f}}, 1},
        {"CRESCENTS", -1, AB_P3, {{A::ACRESCENT, 0.45f, 18.f, 3, 0.f}, {A::ACRESCENT, 0.35f, 18.f, 3, 0.f}}, 2},
    };
    return S[std::clamp(id, 0, AB_STRINGS - 1)];
}
inline const char* abLine(int i) {
    static const char* L[] = {"KNEEL.", "TOO SLOW.", "AGAIN.", "BE JUDGED.", "...WORTHY."};
    return i >= 0 && i < 5 ? L[i] : "";
}

// A teleport or dash's landing: on the floor (r <= 42 from the centre), clear
// of every standing pillar (pushed out radially), never on a shard
inline glm::vec3 absSafeSpot(const EnemyWorld& w, glm::vec3 want, glm::vec3 fallback) {
    if (!w.hasZenith) return want;
    glm::vec3 c = w.zenCentre;
    glm::vec2 d{want.x - c.x, want.z - c.z};
    float r = glm::length(d);
    if (r > 42.f) d *= 42.f / r;
    glm::vec3 p{c.x + d.x, w.zenFloor, c.z + d.y};
    if (w.level) {
        for (int tries = 0; tries < 8; ++tries) {
            int k = w.level->zenithPillarAt(p, 0.8f);
            if (k < 0) break;
            const AABB& b = w.level->walls[w.level->zenith.pillarWalls[k]].box;
            glm::vec2 bc{(b.min.x + b.max.x) * 0.5f, (b.min.z + b.max.z) * 0.5f};
            glm::vec2 out{p.x - bc.x, p.z - bc.y};
            if (glm::length(out) < 1e-3f) out = {bc.x - c.x, bc.y - c.z};
            out = glm::normalize(out);
            p.x = bc.x + out.x * 2.4f; p.z = bc.y + out.y * 2.4f;   // to the pillar's open side
            glm::vec2 q{p.x - c.x, p.z - c.z}; float qr = glm::length(q);
            if (qr > 42.f) { q *= 42.f / qr; p.x = c.x + q.x; p.z = c.z + q.y; }
        }
        if (w.level->zenithPillarAt(p, 0.7f) >= 0) p = glm::vec3{fallback.x, w.zenFloor, fallback.z};
    }
    if (glm::length(glm::vec2{p.x - c.x, p.z - c.z}) < ZEN_DAIS + 0.5f) p.y = w.zenFloor + ZEN_DAIS_UP;
    return p;
}

// Parried: a white strike in its window (the punch, or the sword's parry)
inline int Enemy::abParried(bool bySword) {
    if (type != EnemyType::ABSOLUTE || staggered() || abParriedThis || abTellOf(attack) != AbTell::WHITE) return 0;
    abParriedThis = true;
    if (bySword && abBlade && attack == AttackKind::ASWEEP) {   // blade meets blade: it shatters
        ev.abBladeBroke = true; abBlade = false; abBladeGone = AB_BLADE_BACK + AB_BREAK;
        stagger(AB_BREAK); abReeling = false; abParries = 0;
        return 3;
    }
    if (++abParries >= 2) { stagger(AB_BREAK); abReeling = false; abParries = 0; return 2; }
    stagger(AB_REEL); abReeling = true;   // (stagger() ends the string: the parry interrupts it)
    return 1;
}

inline void Enemy::thinkAbsolute(float dt, const EnemyWorld& w, bool resolve) {
    const glm::vec3 C = w.hasZenith ? w.zenCentre : position;
    abCentreCache = C;
    glm::vec3 feet = w.playerFeet;
    glm::vec3 to = flatTo(feet);
    float d = glm::length(to);
    const float hp = health / maxHealth;
    const float quick = abPhase >= 2 ? AB_QUICK : 1.f;

    if (abArrive > 0.f) {   // descending onto the Dais
        abArrive -= dt;
        turnToward(to, dt, 6.f);
        return;
    }
    if (abBladeGone > 0.f) { abBladeGone -= dt; if (abBladeGone <= 0.f && abPhase == 3) { abBlade = true; ev.abBladeDrawn = true; } }
    abPhaseCheck(hp);                // Task 8 (phases 2-3, the last judgment); a no-op stub in this task
    if (phaseHold > 0.f) { phaseHold -= dt; return; }

    // ---- anti-camp clock ----
    int pillar = w.level ? w.level->zenithPillarAt(feet, 3.f) : -1;
    bool behind = pillar >= 0 && pillar == abCampPillar;
    bool camping = w.onShard || glm::length(glm::vec2{feet.x - C.x, feet.z - C.z}) > AB_FAR || behind;
    abCampPillar = pillar;
    abCamp = camping ? abCamp + dt : 0.f;

    // ---- a strike landing ----
    if (resolve) abResolve(w, feet, to, d);
    if (telegraphTimer > 0.f) {   // winding up: face you; a jab or hook closes in
        turnToward(to, dt, 10.f);
        if ((attack == AttackKind::AJAB || attack == AttackKind::AHOOK || attack == AttackKind::ASWEEP) && d > AB_REACH * 0.8f) {
            glm::vec3 dir = norm2(to);
            position = absSafeSpot(w, position + dir * std::min(d - AB_REACH * 0.7f, AB_RUN * 1.4f * dt), position);
        }
        ev.abTell = telegraphTimer == telegraphDuration - 0.f || ev.telegraphStarted ? (int)abTellOf(attack) : -1;   // on the tick it starts
        return;
    }
    if (recoverTimer > 0.f) { turnToward(to, dt, 4.f); return; }

    // ---- the next move: the rest of this string, or a new one ----
    if (abCur >= 0) {
        const AbString& s = abString_();
        while (abStep < s.n && s.steps[abStep].minPhase > abPhase) ++abStep;
        if (abStep < s.n) { abBegin(s.steps[abStep], abStep > 0, quick, w, feet); return; }
        abCur = -1; abParries = 0;
        recover(0.8f * quick);
        return;
    }
    int id = abChoose(camping, d, w);
    abCur = id; abStep = 0; abParries = 0;
    if (abString_().line >= 0) ev.abLine = abString_().line;
    abBegin(abString_().steps[0], false, quick, w, feet);
}
```

Then the private helpers. Declare them in `Enemy` next to `thinkAbsolute`:
`const AbString& abString_() const { return abString(abCur); }`,
`void abPhaseCheck(float hp)`, `int abChoose(bool camping, float d, const EnemyWorld& w)`,
`void abBegin(const AbStep& s, bool follow, float quick, const EnemyWorld& w, glm::vec3 feet)`,
`void abResolve(const EnemyWorld& w, glm::vec3 feet, glm::vec3 to, float d)`.
(The running string's field is `abCur`; `abString(id)` is the free
function that looks a string up.)

```cpp
inline void Enemy::abPhaseCheck(float) {}   // phases 2-3: Task 8

inline int Enemy::abChoose(bool camping, float d, const EnemyWorld& w) {
    if (abForce >= 0) { int f = abForce; abForce = -1; return f; }
    if (abCamp >= AB_CAMP) {
        abCamp = 0.f;
        return (abPhase == 2 && w.onShard) ? AB_ZENITH : AB_TOO_SLOW;
    }
    if (d > AB_RANGE) return abPhase == 3 ? ((abRot++ & 1) ? AB_CRESCENTS : AB_LUNGE) : ((abRot++ & 1) ? AB_SERPENT : AB_TOO_SLOW);
    static const int R1[] = {AB_TOO_SLOW, AB_KNEEL, AB_JUDGED, AB_KNEEL, AB_TOO_SLOW, AB_JUDGED};
    static const int R2[] = {AB_TOO_SLOW, AB_SEAMS, AB_KNEEL, AB_JUDGED, AB_TOO_SLOW, AB_KNEEL, AB_SEAMS, AB_JUDGED};
    static const int R3[] = {AB_SWEEPS, AB_KNEEL, AB_LUNGE, AB_TOO_SLOW, AB_CRESCENTS, AB_SWEEPS, AB_JUDGED, AB_SEAMS};
    (void)camping;
    if (abPhase == 1) return R1[abRot++ % 6];
    if (abPhase == 2) return R2[abRot++ % 8];
    return R3[abRot++ % 8];
}

inline void Enemy::abBegin(const AbStep& s, bool follow, float quick, const EnemyWorld& w, glm::vec3 feet) {
    abParriedThis = false;
    abLate = s.late;
    float tell = (follow ? s.tell * quick : s.tell) + s.late;
    startAttack(s.k, tell, follow);
    glm::vec3 to = flatTo(feet), dir = norm2(to);
    switch (s.k) {
        case AttackKind::ATELEPORT: {   // behind you, from where he stood
            glm::vec3 want = feet + (glm::length(to) > 0.1f ? dir : glm::vec3{0, 0, 1}) * 2.5f;
            ev.blinked = true; ev.blinkFrom = position;
            position = absSafeSpot(w, want, feet - dir * 2.5f);
            break;
        }
        case AttackKind::ADASH: {
            glm::vec3 want = feet - dir * 2.2f;
            ev.blinked = true; ev.blinkFrom = position;   // the dash's afterimage
            position = absSafeSpot(w, want, position);
            break;
        }
        case AttackKind::AHOP: position = absSafeSpot(w, position - dir * 6.f, position); break;
        case AttackKind::ARAY: case AttackKind::ALUNGE: abAim = feet; ev.abFrom = position + glm::vec3{0, 1.6f, 0}; ev.abTo = feet + glm::vec3{0, 1.f, 0}; break;
        case AttackKind::ADROP: abAim = feet + leadVel_ * tell; break;
        default: break;
    }
    ev.abTell = (int)abTellOf(s.k);
}

inline void Enemy::abResolve(const EnemyWorld& w, glm::vec3 feet, glm::vec3 to, float d) {
    const AbStep& s = abString_().steps[std::min(abStep, abString_().n - 1)];
    float vert = std::fabs(feet.y - position.y);
    auto melee = [&](float reach) {
        if (d < reach && vert < 2.2f) { ev.meleeHit = true; ev.meleeDamage = s.damage * tune_->damage; }
    };
    switch (attack) {
        case AttackKind::AJAB: case AttackKind::AHOOK: melee(AB_REACH); break;
        case AttackKind::AUPPER: melee(AB_REACH); if (ev.meleeHit) ev.abLaunch = true; break;
        case AttackKind::ADROP: {   // a leap onto where you were going
            ev.blinked = true; ev.blinkFrom = position;
            position = absSafeSpot(w, abAim, position);
            float dd = glm::length(glm::vec2{feet.x - position.x, feet.z - position.z});
            if (dd < 2.6f) { ev.meleeHit = true; ev.meleeDamage = s.damage * tune_->damage; }
            break;
        }
        case AttackKind::ARAY:    ev.abRay = true; ev.abFrom = position + glm::vec3{0, 1.6f, 0}; ev.abTo = feet + glm::vec3{0, 1.f, 0}; break;
        case AttackKind::ASTOMP:  ev.abStomp = true; ev.abFrom = position; break;
        case AttackKind::ASERPENT: ev.abSerpent = true; ev.abFrom = position + glm::vec3{0, 1.8f, 0}; break;
        default: abResolveLate(w, feet, d, s); break;   // phases 2-3 (Task 8); a no-op until then
    }
    (void)to;
    lastAbAttack = attack;
    attack = AttackKind::NONE;
    ++abStep;
}
inline void Enemy::abResolveLate(const EnemyWorld&, glm::vec3, float, const AbStep&) {}
```

(Fix the `ev.abTell` line in `thinkAbsolute`: `abBegin` sets `ev.abTell`, so
drop the conditional assignment there. It's shown only to make clear that the
event fires once, on the tick a tell starts.)

- [ ] **Step 5: Run the tests**

`make test` → every `THE ABSOLUTE: phase 1` check `ok`. Tune the string table
(never below the floors) if "a whole string costs 60–70" or the camp timings
miss. Re-run until green.

- [ ] **Step 6: Commit**

```bash
git add src/EnemyAbsolute.h src/Enemy.h tests/test_zenith.cpp
git commit -m "THE ABSOLUTE's mind, phase 1: strings of moves in a table, each strike white (parry), gold (dodge or block) or red (get out); parry one and he reels, two in a string and he breaks; teleports and dashes that always land on open floor; camp three seconds and he comes for you"
```

---

### Task 7: AbsoluteHazards (rays, stomp rings, serpents, seams, the zenith ray, the halo sweep, mirages, the last judgment)

**Files:**
- Create: `src/AbsoluteHazards.h`
- Test: `tests/test_zenith.cpp`
- Modify: `Makefile` (prerequisite)

**Interfaces:**
- Produces `struct AbsoluteHazards` with:
  - constants `RAY_TIME = 1.f, RAY_CHASE = 5.5f, RAY_DPS = 25.f, RAY_HALF = 0.6f, RAY_PARRY = 0.15f, RAY_REFLECT = 250.f; STOMP_SPEED = 18.f, STOMP_H = 1.f, STOMP_DAMAGE = 20.f; SERPENT_SPEED = 6.5f, SERPENT_TURN = 1.6f, SERPENT_LIFE = 6.f, SERPENT_DAMAGE = 20.f, SERPENT_PARRY = 300.f; SEAM_WARN = 0.9f, SEAM_GAP = 0.7f, SEAM_DAMAGE = 25.f; ZRAY_TIME = 2.5f, ZRAY_CHASE = 5.f, ZRAY_R = 1.6f, ZRAY_DPS = 20.f; HALO_TIME = 2.f, HALO_LO = 1.0f, HALO_HI = 1.6f, HALO_DAMAGE = 30.f; MIRAGE_TELL = 0.5f, MIRAGE_DAMAGE = 8.f; JUDGE_WARN = 2.f, JUDGE_SAFE = 6.f, JUDGE_DAMAGE = 60.f;`
  - `struct Hit { float damage; glm::vec3 from; Sword::Strike kind; bool tick; };` (`tick`: per-second damage already scaled by dt)
  - `void addRay(glm::vec3 from, glm::vec3 at)`, `addStomp(glm::vec3 at)`, `addSerpent(glm::vec3 at, glm::vec3 dir)`, `addSeams(glm::vec3 centre, int firstParity)`, `addZenith(glm::vec3 haloPoint, glm::vec3 at)`, `addHalo(glm::vec3 centre, float floorY)`, `addMirage(glm::vec3 at)`, `addJudge(glm::vec3 centre, glm::vec3 boss)`
  - `std::vector<Hit> update(float dt, glm::vec3 feet, float height, bool grounded, const LevelData* lv, glm::vec3 bossPos)`
  - `bool rayParry(int i)` (true if ray i is in its first 0.15 s; it ends), `int serpentNear(glm::vec3 p, float r) const`, `void reflectSerpent(int i)`, `int reflectedHits(glm::vec3 boss, float r)` (counts and removes reflected serpents reaching him), `int mirageAlong(glm::vec3 o, glm::vec3 dir, float maxT) const`, `void killMirage(int i)`, `void clear()`
  - `static int seamParity(glm::vec3 centre, glm::vec3 p)` → `((floor((p.x-c.x)/6) + floor((p.z-c.z)/6)) & 1)`
  - `int haloShatterMask(const LevelData& lv)` (pillars the halo beam passed this tick, used by the game to park four of them)
  - public vectors `rays, stomps, serpents, seams, zrays, halos, mirages, judges` (for drawing).

- [ ] **Step 1: Failing tests**

```cpp
    // ---------------------------------------------------------------- the Absolute's hazards
    {
        using AH = AbsoluteHazards;
        glm::vec3 c = ZC;
        auto total = [](const std::vector<AH::Hit>& hs) { float s = 0; for (auto& h : hs) s += h.damage; return s; };
        { AH h; glm::vec3 me = c + glm::vec3{0, 0, 12};
          h.addRay(c + glm::vec3{0, 1.6f, 0}, me + glm::vec3{0, 1, 0});
          float got = 0; for (float t = 0; t < 1.2f; t += DT) got += total(h.update(DT, me, 1.8f, true, &N, c));
          CHECK(got > 20.f && got < 27.f, "standing in a ray: about 25 over its second");
          AH r; glm::vec3 p = me; float got2 = 0;
          r.addRay(c + glm::vec3{0, 1.6f, 0}, p + glm::vec3{0, 1, 0});
          for (float t = 0; t < 1.2f; t += DT) { p.x += 7.f * DT; got2 += total(r.update(DT, p, 1.8f, true, &N, c)); }
          CHECK(got2 < 8.f, "strafing at a run outpaces it (it chases at 5.5 m/s)");
          AH q; glm::vec3 pil = zenithPillar(0), out = glm::normalize(glm::vec3{pil.x - c.x, 0, pil.z - c.z});
          glm::vec3 hide = pil + out * 2.f;
          q.addRay(c + glm::vec3{0, 1.6f, 0}, hide + glm::vec3{0, 1, 0});
          float got3 = 0; for (float t = 0; t < 1.2f; t += DT) got3 += total(q.update(DT, hide, 1.8f, true, &N, c));
          CHECK(got3 < 1.f, "a pillar between you stops it");
          AH pr; pr.addRay(c, me); pr.update(0.1f, me, 1.8f, true, &N, c);
          CHECK(pr.rayParry(0) && pr.rays.empty(), "a ray caught in its first 0.15 s is parried and ends");
          AH late; late.addRay(c, me); late.update(0.2f, me, 1.8f, true, &N, c);
          CHECK(!late.rayParry(0), "...not after"); }
        { AH h; glm::vec3 me = c + glm::vec3{0, 0, 9};
          h.addStomp(c);
          float grounded = 0, jumped = 0;
          for (float t = 0; t < 1.f; t += DT) grounded += total(h.update(DT, me, 1.8f, true, &N, c));
          AH j; j.addStomp(c);
          for (float t = 0; t < 1.f; t += DT) jumped += total(j.update(DT, me + glm::vec3{0, 1.4f, 0}, 1.8f, false, &N, c));
          CHECK(std::fabs(grounded - AH::STOMP_DAMAGE) < 1e-3f && jumped == 0.f, "the stomp ring hits once on the ground; jump it"); }
        { AH h; glm::vec3 me = c + glm::vec3{0, 0, 15}, p = me;
          h.addSerpent(c + glm::vec3{0, 1.8f, 0}, {0, 0, 1});
          float got = 0;
          for (float t = 0; t < AH::SERPENT_LIFE + 0.5f; t += DT) { p.x += 7.f * DT * (std::fmod(t, 4.f) < 2.f ? 1.f : -1.f); p.z += 2.f * DT; got += total(h.update(DT, p, 1.8f, true, &N, c)); }
          CHECK(got == 0.f, "a running player outruns a serpent");
          AH s; s.addSerpent(c + glm::vec3{0, 1.8f, 0}, {0, 0, 1});
          float hit = 0; for (float t = 0; t < 4.f; t += DT) hit += total(s.update(DT, me, 1.8f, true, &N, c));
          CHECK(std::fabs(hit - AH::SERPENT_DAMAGE) < 1e-3f, "standing still, it reaches you");
          AH r; r.addSerpent(me + glm::vec3{0, 1.5f, -1.f}, {0, 0, 1});
          int k = r.serpentNear(me + glm::vec3{0, 1.5f, 0}, 2.f);
          CHECK(k == 0, "a serpent within reach can be found to parry");
          r.reflectSerpent(k);
          int back = 0; for (float t = 0; t < 4.f && !back; t += DT) { r.update(DT, me, 1.8f, true, &N, c); back = r.reflectedHits(c + glm::vec3{0, 1.3f, 0}, 1.2f); }
          CHECK(back == 1, "parried, it flies back into him"); }
        { AH h; h.addSeams(c, 0);
          glm::vec3 a = c + glm::vec3{3, 0, 3}, b = c + glm::vec3{9, 0, 3};
          CHECK(AH::seamParity(c, a) != AH::seamParity(c, b), "neighbouring cells alternate");
          glm::vec3 in0 = AH::seamParity(c, a) == 0 ? a : b, in1 = AH::seamParity(c, a) == 0 ? b : a;
          AH h0; h0.addSeams(c, 0); float d0 = 0; for (float t = 0; t < AH::SEAM_WARN + 0.05f; t += DT) d0 += total(h0.update(DT, in0, 1.8f, true, &N, c));
          AH h1; h1.addSeams(c, 0); float d1 = 0; for (float t = 0; t < AH::SEAM_WARN + 0.05f; t += DT) d1 += total(h1.update(DT, in1, 1.8f, true, &N, c));
          CHECK(d0 == AH::SEAM_DAMAGE && d1 == 0.f, "the first wave burns one parity; the dark cells are safe");
          AH h2; h2.addSeams(c, 0); float d2 = 0; for (float t = 0; t < AH::SEAM_WARN + AH::SEAM_GAP + 0.05f; t += DT) d2 += total(h2.update(DT, in1, 1.8f, true, &N, c));
          CHECK(d2 == AH::SEAM_DAMAGE, "...then the other, 0.7 s later: step across"); }
        { AH h; glm::vec3 me = c + glm::vec3{0, 0, 20}, p = me;
          h.addZenith(c + glm::vec3{0, 40, 0}, me);
          float got = 0; for (float t = 0; t < AH::ZRAY_TIME; t += DT) { p.x += 7.f * DT; got += total(h.update(DT, p, 1.8f, true, &N, c)); }
          CHECK(got < 4.f, "the zenith ray turns slower than you run"); }
        { AH h; glm::vec3 me = c + glm::vec3{0, 0, 20};
          h.addHalo(c, c.y);
          float stand = 0; for (float t = 0; t < AH::HALO_TIME + 0.1f; t += DT) stand += total(h.update(DT, me, 1.8f, true, &N, c));
          AH s; s.addHalo(c, c.y);
          float slide = 0; for (float t = 0; t < AH::HALO_TIME + 0.1f; t += DT) slide += total(s.update(DT, me, 0.9f, true, &N, c));
          CHECK(stand == AH::HALO_DAMAGE && slide == 0.f, "the 360 sweep hits you standing; slide under it");
          AH m; m.addHalo(c, c.y); int mask = 0;
          for (float t = 0; t < AH::HALO_TIME + 0.1f; t += DT) { m.update(DT, me, 1.8f, true, &N, c); mask |= m.haloShatterMask(N); }
          CHECK(mask == 0x55, "it shatters four pillars (every other one)"); }
        { AH h; h.addJudge(c, c);
          float near = 0, far = 0; AH f; f.addJudge(c, c);
          for (float t = 0; t < AH::JUDGE_WARN + 0.1f; t += DT) { near += total(h.update(DT, c + glm::vec3{0, 0, 4}, 1.8f, true, &N, c)); far += total(f.update(DT, c + glm::vec3{0, 6, 20}, 1.8f, false, &N, c)); }
          CHECK(near == 0.f && far == AH::JUDGE_DAMAGE, "the last judgment: beside him you're safe; anywhere else (even in the air) it's 60"); }
        { AH h; glm::vec3 me = c + glm::vec3{0, 0, 10};
          h.addMirage(me + glm::vec3{2, 0, 0});
          CHECK(h.mirageAlong(me + glm::vec3{0, 1.5f, 0}, glm::vec3{1, 0, 0}, 10.f) == 0, "a mirage can be shot");
          h.killMirage(0);
          float got = 0; for (float t = 0; t < 1.f; t += DT) got += total(h.update(DT, me, 1.8f, true, &N, c));
          CHECK(h.mirages.empty() && got == 0.f, "one hit and it's gone");
          AH k; k.addMirage(me + glm::vec3{2, 0, 0}); float hit = 0;
          for (float t = 0; t < 1.f; t += DT) hit += total(k.update(DT, me, 1.8f, true, &N, c));
          CHECK(hit == AH::MIRAGE_DAMAGE, "left alone its jab lands for half (8)"); }
        { AH h; h.addRay(c, c); h.addStomp(c); h.addSerpent(c, {0, 0, 1}); h.addSeams(c, 0); h.addZenith(c, c); h.addHalo(c, c.y); h.addMirage(c); h.addJudge(c, c);
          h.clear();
          CHECK(h.rays.empty() && h.stomps.empty() && h.serpents.empty() && h.seams.empty() && h.zrays.empty() && h.halos.empty() && h.mirages.empty() && h.judges.empty(),
                "clear() empties every hazard (a retry)"); }
    }
```

- [ ] **Step 2: Run, see it fail**

`make test` → `AbsoluteHazards` undeclared.

- [ ] **Step 3: Write `src/AbsoluteHazards.h`**

```cpp
#pragma once
// =============================================================================
// AbsoluteHazards.h — what THE ABSOLUTE leaves in the Zenith besides his
// fists: rays (red; they chase slower than you strafe, pillars stop them, a
// sword parry in the first 0.15 s sends one back), stomp rings (jump them),
// serpents (white; outrun them or parry them back for 300), the seams (two
// waves of burning cells: stand in the dark ones), the zenith ray from the
// Halo, the 360 sweep at chest height (slide under; it shatters four
// pillars), mirages (one hit kills them), and the last judgment (be beside
// him). No OpenGL or audio: Gameplay_Absolute.h draws and sounds them.
// =============================================================================
#include "Level.h"
#include "Sword.h"
#include <vector>

struct AbsoluteHazards {
    static constexpr float RAY_TIME = 1.f, RAY_CHASE = 5.5f, RAY_DPS = 25.f, RAY_HALF = 0.6f, RAY_PARRY = 0.15f, RAY_REFLECT = 250.f,
                           STOMP_SPEED = 18.f, STOMP_H = 1.f, STOMP_DAMAGE = 20.f, STOMP_MAX = 46.f,
                           SERPENT_SPEED = 6.5f, SERPENT_TURN = 1.6f, SERPENT_LIFE = 6.f, SERPENT_DAMAGE = 20.f, SERPENT_PARRY = 300.f,
                           SEAM_WARN = 0.9f, SEAM_GAP = 0.7f, SEAM_DAMAGE = 25.f, SEAM_CELL = 6.f,
                           ZRAY_TIME = 2.5f, ZRAY_CHASE = 5.f, ZRAY_R = 1.6f, ZRAY_DPS = 20.f,
                           HALO_TIME = 2.f, HALO_LO = 1.0f, HALO_HI = 1.6f, HALO_DAMAGE = 30.f, HALO_REACH = 46.f,
                           MIRAGE_TELL = 0.5f, MIRAGE_DAMAGE = 8.f, MIRAGE_R = 0.6f,
                           JUDGE_WARN = 2.f, JUDGE_SAFE = 6.f, JUDGE_DAMAGE = 60.f;
    struct Hit { float damage; glm::vec3 from; Sword::Strike kind; bool tick; };
    struct Ray { glm::vec3 from, at; float t = 0.f; };
    struct Stomp { glm::vec3 at; float r = 0.f; bool hit = false; };
    struct Serpent { glm::vec3 pos, dir; float t = 0.f; bool back = false; };
    struct Seams { glm::vec3 centre; int first; float t = 0.f; bool done0 = false, done1 = false; };
    struct ZRay { glm::vec3 halo, at; float t = 0.f; };
    struct Halo { glm::vec3 centre; float floorY, t = 0.f, prevA = 0.f; bool hit = false; };
    struct Mirage { glm::vec3 at; float t = 0.f; bool struck = false; };
    struct Judge { glm::vec3 centre, boss; float t = 0.f; bool done = false; };
    std::vector<Ray> rays; std::vector<Stomp> stomps; std::vector<Serpent> serpents; std::vector<Seams> seams;
    std::vector<ZRay> zrays; std::vector<Halo> halos; std::vector<Mirage> mirages; std::vector<Judge> judges;
    int shatterThisTick = 0;

    void clear() { rays.clear(); stomps.clear(); serpents.clear(); seams.clear(); zrays.clear(); halos.clear(); mirages.clear(); judges.clear(); shatterThisTick = 0; }
    void addRay(glm::vec3 from, glm::vec3 at)        { rays.push_back({from, at}); }
    void addStomp(glm::vec3 at)                      { stomps.push_back({at}); }
    void addSerpent(glm::vec3 at, glm::vec3 dir)     { serpents.push_back({at, glm::normalize(dir)}); }
    void addSeams(glm::vec3 centre, int first)       { seams.push_back({centre, first & 1}); }
    void addZenith(glm::vec3 halo, glm::vec3 at)     { zrays.push_back({halo, at}); }
    void addHalo(glm::vec3 centre, float floorY)     { halos.push_back({centre, floorY}); }
    void addMirage(glm::vec3 at)                     { mirages.push_back({at}); }
    void addJudge(glm::vec3 centre, glm::vec3 boss)  { judges.push_back({centre, boss}); }

    static int seamParity(glm::vec3 c, glm::vec3 p) {
        return ((int)std::floor((p.x - c.x) / SEAM_CELL) + (int)std::floor((p.z - c.z) / SEAM_CELL)) & 1;
    }
    static bool blocked(const LevelData* lv, glm::vec3 a, glm::vec3 b) {
        if (!lv) return false;
        for (size_t i = 0; i < lv->zenith.pillarWalls.size(); ++i)
            if (lv->zenith.pillarUp[i] && segmentHitsBox(a, b, lv->walls[lv->zenith.pillarWalls[i]].box)) return true;
        return false;
    }
    static glm::vec3 chase(glm::vec3 at, glm::vec3 target, float speed, float dt) {
        glm::vec3 d = target - at; d.y = 0.f;
        float l = glm::length(d);
        return l < 1e-4f ? at : at + d / l * std::min(l, speed * dt);
    }

    std::vector<Hit> update(float dt, glm::vec3 feet, float height, bool grounded, const LevelData* lv, glm::vec3 boss) {
        std::vector<Hit> out;
        glm::vec3 chest = feet + glm::vec3{0.f, height * 0.6f, 0.f};
        shatterThisTick = 0;
        for (auto& r : rays) {   // the end chases you; you're hit within RAY_HALF of the line, unless a pillar is between
            r.t += dt;
            r.at = chase(r.at, chest, RAY_CHASE, dt); r.at.y = chest.y;
            glm::vec3 ab = r.at - r.from;
            glm::vec3 far = r.from + ab * (60.f / std::max(glm::length(ab), 0.1f));
            float t = glm::clamp(glm::dot(chest - r.from, far - r.from) / glm::dot(far - r.from, far - r.from), 0.f, 1.f);
            glm::vec3 q = r.from + (far - r.from) * t;
            if (glm::length(chest - q) < RAY_HALF + 0.4f && !blocked(lv, r.from, chest))
                out.push_back({RAY_DPS * dt, r.from, Sword::Strike::BEAM, true});
        }
        rays.erase(std::remove_if(rays.begin(), rays.end(), [](const Ray& r) { return r.t >= RAY_TIME; }), rays.end());
        for (auto& s : stomps) {
            float was = s.r; s.r += STOMP_SPEED * dt;
            float d = glm::length(glm::vec2{feet.x - s.at.x, feet.z - s.at.z});
            if (!s.hit && d >= was - 0.5f && d < s.r + 0.5f && grounded && feet.y < s.at.y + STOMP_H) { s.hit = true; out.push_back({STOMP_DAMAGE, s.at, Sword::Strike::GOLD, false}); }
        }
        stomps.erase(std::remove_if(stomps.begin(), stomps.end(), [](const Stomp& s) { return s.r > STOMP_MAX; }), stomps.end());
        for (auto& s : serpents) {
            s.t += dt;
            glm::vec3 target = s.back ? boss + glm::vec3{0.f, 1.3f, 0.f} : chest;
            glm::vec3 want = target - s.pos; float wl = glm::length(want);
            if (wl > 1e-3f) {
                want /= wl;
                float ang = std::acos(glm::clamp(glm::dot(s.dir, want), -1.f, 1.f)), turn = (s.back ? 6.f : SERPENT_TURN) * dt;
                s.dir = ang <= turn ? want : glm::normalize(s.dir + (want - s.dir) * (turn / std::max(ang, 1e-3f)));
            }
            s.pos += s.dir * (s.back ? 30.f : SERPENT_SPEED) * dt;
            if (!s.back && glm::length(s.pos - chest) < 0.9f) { out.push_back({SERPENT_DAMAGE, s.pos, Sword::Strike::WHITE, false}); s.t = 1e9f; }
        }
        serpents.erase(std::remove_if(serpents.begin(), serpents.end(), [](const Serpent& s) { return s.t >= SERPENT_LIFE && !s.back; }), serpents.end());
        for (auto& s : seams) {
            s.t += dt;
            auto burn = [&](int parity, bool& done, float at) {
                if (done || s.t < at) return;
                done = true;
                if (grounded && feet.y < s.centre.y + 0.6f && seamParity(s.centre, feet) == parity &&
                    glm::length(glm::vec2{feet.x - s.centre.x, feet.z - s.centre.z}) < 44.f)
                    out.push_back({SEAM_DAMAGE, feet, Sword::Strike::GOLD, false});
            };
            burn(s.first, s.done0, SEAM_WARN);
            burn(s.first ^ 1, s.done1, SEAM_WARN + SEAM_GAP);
        }
        seams.erase(std::remove_if(seams.begin(), seams.end(), [](const Seams& s) { return s.done1; }), seams.end());
        for (auto& z : zrays) {
            z.t += dt;
            z.at = chase(z.at, feet, ZRAY_CHASE, dt);
            if (glm::length(glm::vec2{feet.x - z.at.x, feet.z - z.at.z}) < ZRAY_R) out.push_back({ZRAY_DPS * dt, z.halo, Sword::Strike::BEAM, true});
        }
        zrays.erase(std::remove_if(zrays.begin(), zrays.end(), [](const ZRay& z) { return z.t >= ZRAY_TIME; }), zrays.end());
        for (auto& h : halos) {   // a beam from the centre turning a full circle in HALO_TIME, at chest height
            h.t += dt;
            float a = 6.2831853f * std::min(1.f, h.t / HALO_TIME), was = h.prevA; h.prevA = a;
            float pa = std::atan2(feet.z - h.centre.z, feet.x - h.centre.x); if (pa < 0.f) pa += 6.2831853f;
            bool swept = pa >= was && pa < a;
            float top = feet.y + height;
            if (!h.hit && swept && glm::length(glm::vec2{feet.x - h.centre.x, feet.z - h.centre.z}) < HALO_REACH &&
                top > h.floorY + HALO_LO && feet.y < h.floorY + HALO_HI) { h.hit = true; out.push_back({HALO_DAMAGE, h.centre, Sword::Strike::BEAM, false}); }
            if (lv) for (size_t i = 0; i < lv->zenith.pillarWalls.size(); i += 2) {   // every other pillar shatters
                const AABB& b = lv->zenith.pillarHome[i];
                float ba = std::atan2((b.min.z + b.max.z) * 0.5f - h.centre.z, (b.min.x + b.max.x) * 0.5f - h.centre.x);
                if (ba < 0.f) ba += 6.2831853f;
                if (ba >= was && ba < a) shatterThisTick |= 1 << i;
            }
        }
        halos.erase(std::remove_if(halos.begin(), halos.end(), [](const Halo& h) { return h.t >= HALO_TIME; }), halos.end());
        for (auto& m : mirages) {   // each one jabs once, MIRAGE_TELL after it appears
            m.t += dt;
            if (!m.struck && m.t >= MIRAGE_TELL) {
                m.struck = true;
                if (glm::length(glm::vec2{feet.x - m.at.x, feet.z - m.at.z}) < 3.2f) out.push_back({MIRAGE_DAMAGE, m.at, Sword::Strike::WHITE, false});
            }
        }
        mirages.erase(std::remove_if(mirages.begin(), mirages.end(), [](const Mirage& m) { return m.t > MIRAGE_TELL + 0.3f; }), mirages.end());
        for (auto& j : judges) {
            j.t += dt;
            if (!j.done && j.t >= JUDGE_WARN) {
                j.done = true;
                if (glm::length(glm::vec2{feet.x - j.boss.x, feet.z - j.boss.z}) > JUDGE_SAFE) out.push_back({JUDGE_DAMAGE, j.boss, Sword::Strike::GOLD, false});
            }
        }
        judges.erase(std::remove_if(judges.begin(), judges.end(), [](const Judge& j) { return j.done; }), judges.end());
        return out;
    }

    int  haloShatterMask(const LevelData&) const { return shatterThisTick; }
    bool rayParry(int i) {
        if (i < 0 || i >= (int)rays.size() || rays[i].t > RAY_PARRY) return false;
        rays.erase(rays.begin() + i); return true;
    }
    int serpentNear(glm::vec3 p, float r) const {
        for (int i = 0; i < (int)serpents.size(); ++i) if (!serpents[i].back && glm::length(serpents[i].pos - p) < r) return i;
        return -1;
    }
    void reflectSerpent(int i) { if (i >= 0 && i < (int)serpents.size()) { serpents[i].back = true; serpents[i].t = 0.f; } }
    int reflectedHits(glm::vec3 boss, float r) {
        int n = 0;
        for (auto it = serpents.begin(); it != serpents.end();)
            if (it->back && glm::length(it->pos - boss) < r) { ++n; it = serpents.erase(it); } else ++it;
        return n;
    }
    int mirageAlong(glm::vec3 o, glm::vec3 d, float maxT) const {
        for (int i = 0; i < (int)mirages.size(); ++i) {
            AABB b{mirages[i].at + glm::vec3{-MIRAGE_R, 0.f, -MIRAGE_R}, mirages[i].at + glm::vec3{MIRAGE_R, 2.6f, MIRAGE_R}};
            float t = rayBoxHit(o, d, b);
            if (t >= 0.f && t <= maxT) return i;
        }
        return -1;
    }
    void killMirage(int i) { if (i >= 0 && i < (int)mirages.size()) mirages.erase(mirages.begin() + i); }
};
```

(`segmentHitsBox` and `rayBoxHit` come from `Enemy.h`, so include
`"Enemy.h"` rather than redefining them. The halo test expects mask `0x55`:
pillars 0, 2, 4 and 6.)

Add `src/AbsoluteHazards.h` and `src/EnemyAbsolute.h` to the Makefile's `test:` prerequisites.

- [ ] **Step 4: Run** `make test` → the hazards block all `ok`. Adjust only
  the hazard internals (never the constants in Global Constraints) until green.

- [ ] **Step 5: Commit**

```bash
git add src/AbsoluteHazards.h Makefile tests/test_zenith.cpp
git commit -m "What the Absolute leaves in the Zenith: rays that chase slower than you strafe and stop at pillars, stomp rings to jump, serpents to outrun or send back, two waves of burning seams, the zenith ray, a 360 sweep to slide under that shatters four pillars, mirages, and the last judgment"
```

---

### Task 8: Phases 2 and 3 in the mind (ascension, mirages, seams, zenith ray, the blade, crescents, lunge, the last judgment)

**Files:**
- Modify: `src/EnemyAbsolute.h` (`abPhaseCheck`, `abResolveLate`, the blade in `abChoose`/`abBegin`)
- Test: `tests/test_zenith.cpp`

**Interfaces:**
- Consumes: Task 6's fields and events, Task 7's constants (`AbsoluteHazards::JUDGE_WARN`).
- Produces: phase transitions emitting `ev.abPhase` (2, 3), `ev.abHalo` (phase 2 start), `ev.abBladeDrawn` and `ev.abShards` (phase 3 start), `ev.abJudgeMark` (8 %) then `ev.abJudge` 2 s later and a 4 s stagger (`AB_SPENT`, ×2); `ev.abSeams`/`abSeamFirst`; `ev.abZenith` with `abFrom` (the halo point over you) / `abTo` (you); `ev.abMirages = 2` with mirage spots in `ev.phantomPos[0..1]` (reusing the Sovereign's array); crescents as `ev.shots` (2, speed 22, damage 18, `shotParry` 200); the lunge as a dash through `abAim` (`ev.meleeHit` if you're within 1.6 m of its path).

- [ ] **Step 1: Failing tests**

```cpp
    // ---------------------------------------------------------------- THE ABSOLUTE: phases 2 and 3
    {
        Enemy e = freshAbs(); e.abArrive = 0.f;
        glm::vec3 me = ZC + glm::vec3{0, 0, 14};
        e.health = e.maxHealth * 0.59f;
        e.update(DT, zworld(me));
        CHECK(e.abPhase == 2 && e.ev.abPhase == 2 && e.ev.abHalo && e.phaseHold > 0.f, "at 60 %: ASCENSION, the halo sweep, a pause");
        e.phaseHold = 0.f;
        // strings run a hit longer and quicker
        Enemy a = freshAbs(); a.abArrive = 0.f; a.abForce = AB_TOO_SLOW;
        Enemy b = freshAbs(); b.abArrive = 0.f; b.abForce = AB_TOO_SLOW; b.abPhase = 2;
        int ja = 0, jb = 0; float ta = 0, tb = 0;
        for (int i = 0; i < 400; ++i) { a.update(DT, zworld(me)); b.update(DT, zworld(me));
            if (a.ev.telegraphStarted && a.attack == AttackKind::AJAB) { ++ja; if (a.tellFollow) ta = a.telegraphDuration; }
            if (b.ev.telegraphStarted && b.attack == AttackKind::AJAB) { ++jb; if (b.tellFollow && b.abLate == 0.f) tb = b.telegraphDuration; } }
        CHECK(jb == ja + 1 && tb < ta, "phase 2: one more jab, follow-ups 25 % quicker");
        // a delayed strike: its window is on the hit
        Enemy dl = freshAbs(); dl.abArrive = 0.f; dl.abForce = AB_TOO_SLOW; dl.abPhase = 2;
        bool lateWindowOk = false;
        for (int i = 0; i < 400; ++i) { dl.update(DT, zworld(me));
            if (dl.abLate > 0.f && dl.attack == AttackKind::AJAB && dl.telegraphTimer > 0.f && dl.telegraphTimer < 0.25f) lateWindowOk = dl.parryWindow(); }
        CHECK(lateWindowOk, "a delayed strike's parry window is its last 0.25 s, as any other");
        // seams and mirages show up in phase 2's rotation; the zenith ray answers a shard
        Enemy r = freshAbs(); r.abArrive = 0.f; r.abPhase = 2;
        bool seams = false, mirages = false, zray = false;
        for (int i = 0; i < 60 * 40; ++i) { r.update(DT, zworld(me)); seams |= r.ev.abSeams; mirages |= r.ev.abMirages > 0; }
        Enemy s = freshAbs(); s.abArrive = 0.f; s.abPhase = 2;
        EnemyWorld sw = zworld(zenithShard(0)); sw.onShard = true;
        for (int i = 0; i < 60 * 6 && !zray; ++i) { s.update(DT, sw); zray |= s.ev.abZenith; }
        CHECK(seams && mirages, "phase 2 brings the seams and mirages");
        CHECK(zray, "camping on a shard in phase 2 brings the zenith ray");
    }
    {
        Enemy e = freshAbs(); e.abArrive = 0.f; e.abPhase = 2;
        glm::vec3 me = ZC + glm::vec3{0, 0, 10};
        e.health = e.maxHealth * 0.24f;
        e.update(DT, zworld(me));
        CHECK(e.abPhase == 3 && e.ev.abPhase == 3 && e.ev.abBladeDrawn && e.ev.abShards && e.abBlade, "at 25 %: the blade, the shards fall");
        e.phaseHold = 0.f; e.abForce = AB_SWEEPS;
        int r = 0;
        for (int i = 0; i < 400 && !r; ++i) { e.update(DT, zworld(me)); if (e.parryWindow()) r = e.abParried(true); }
        CHECK(r == 3 && !e.abBlade && e.staggered() && std::fabs(e.armorMult() - 2.f) < 1e-4f && e.ev.abBladeBroke,
              "a sword parry of a sweep shatters the blade: broken 3 s at x2");
        for (int i = 0; i < (int)((Enemy::AB_BREAK + Enemy::AB_BLADE_BACK + 0.2f) * 60); ++i) e.update(DT, zworld(me));
        CHECK(e.abBlade, "...and he draws it again");
        Enemy f = freshAbs(); f.abArrive = 0.f; f.abPhase = 3; f.abBlade = true; f.abForce = AB_SWEEPS;
        int rf = 0;
        for (int i = 0; i < 400 && !rf; ++i) { f.update(DT, zworld(me)); if (f.parryWindow()) rf = f.abParried(false); }
        CHECK(rf == 1 && f.abBlade, "an F parry only interrupts it");
        Enemy c = freshAbs(); c.abArrive = 0.f; c.abPhase = 3; c.abBlade = true; c.abForce = AB_CRESCENTS;
        int shots = 0; float parryVal = 0;
        for (int i = 0; i < 300; ++i) { c.update(DT, zworld(ZC + glm::vec3{0, 0, 25})); shots += c.ev.shots; if (c.ev.shots) parryVal = c.ev.shotParry; }
        CHECK(shots >= 2 && std::fabs(parryVal - 200.f) < 1e-3f, "crescents: thrown arcs that parry back for 200");
    }
    {
        Enemy e = freshAbs(); e.abArrive = 0.f; e.abPhase = 3; e.abBlade = true;
        glm::vec3 me = ZC + glm::vec3{0, 0, 10};
        e.health = e.maxHealth * 0.079f;
        bool marked = false, landed = false; float spentAt = 0;
        for (int i = 0; i < 60 * 4; ++i) {
            e.update(DT, zworld(me));
            marked |= e.ev.abJudgeMark;
            if (e.ev.abJudge) { landed = true; spentAt = e.staggerTimer; }
        }
        CHECK(marked && landed && e.abJudged && std::fabs(spentAt - Enemy::AB_SPENT) < 0.05f, "at 8 %: the last judgment, then he's spent 4 s (x2)");
        e.health = e.maxHealth * 0.05f;
        bool again = false; for (int i = 0; i < 60 * 10; ++i) { e.update(DT, zworld(me)); again |= e.ev.abJudgeMark; }
        CHECK(!again, "only once");
    }
```

- [ ] **Step 2: Run, see it fail** (`abPhaseCheck` is a stub).

- [ ] **Step 3: Implement**

```cpp
inline void Enemy::abPhaseCheck(float hp) {
    if (abPhase == 1 && hp <= AB_PHASE2) {
        abPhase = 2; ev.abPhase = 2; ev.abHalo = true; ev.abLine = 2; phaseHold = PHASE_PAUSE + AbsoluteHazards::HALO_TIME;
        attack = AttackKind::NONE; telegraphTimer = 0.f; abCur = -1; abRot = 0;
        position = glm::vec3{abCentreCache.x, abCentreCache.y + ZEN_DAIS_UP, abCentreCache.z};   // he rises over the Dais
    }
    if (abPhase == 2 && hp <= AB_PHASE3) {
        abPhase = 3; ev.abPhase = 3; ev.abBladeDrawn = true; ev.abShards = true; ev.abLine = 0; abBlade = true;
        phaseHold = PHASE_PAUSE; attack = AttackKind::NONE; telegraphTimer = 0.f; abCur = -1; abRot = 0;
    }
    if (abPhase == 3 && !abJudged && hp <= AB_JUDGE_AT) {
        abJudged = true; ev.abJudgeMark = true; ev.abLine = 3;
        attack = AttackKind::AJUDGE; telegraphDuration = telegraphTimer = AbsoluteHazards::JUDGE_WARN;
        abCur = -1; position = glm::vec3{abCentreCache.x, abCentreCache.y + ZEN_DAIS_UP, abCentreCache.z};
    }
}
```

`abCentreCache` is a new field, set from `w.zenCentre` at the top of
`thinkAbsolute`. Including `AbsoluteHazards.h` from `EnemyAbsolute.h` would
make a cycle (`AbsoluteHazards.h` needs `Enemy.h`'s ray helpers). Instead,
hard-code the two times as `Enemy` constants `AB_HALO_TIME = 2.f` and
`AB_JUDGE_WARN = 2.f`, and have `AbsoluteHazards` use
`HALO_TIME = Enemy::AB_HALO_TIME`, `JUDGE_WARN = Enemy::AB_JUDGE_WARN`.

`abResolveLate`:

```cpp
inline void Enemy::abResolveLate(const EnemyWorld& w, glm::vec3 feet, float d, const AbStep& s) {
    switch (attack) {
        case AttackKind::ASEAMS: ev.abSeams = true; ev.abSeamFirst = (int)(age * 7.f) & 1; break;
        case AttackKind::AZENITH: ev.abZenith = true; ev.abFrom = feet + glm::vec3{0.f, ZEN_HALO_UP, 0.f}; ev.abTo = feet; break;
        case AttackKind::ASWEEP:
            if (d < AB_REACH + 0.8f && std::fabs(feet.y - position.y) < 2.2f) { ev.meleeHit = true; ev.meleeDamage = s.damage * tune_->damage; }
            ev.slash = abStep & 1;   // the blade's arc (Effects, as the Sovereign's)
            break;
        case AttackKind::ALUNGE: {   // through where you stood, 20 m at most
            glm::vec3 from = position, dir = norm2(abAim - position);
            glm::vec3 end = absSafeSpot(w, position + dir * std::min(20.f, glm::length(flatTo(abAim)) + 3.f), position);
            ev.blinked = true; ev.blinkFrom = from;
            glm::vec3 ab = end - from; float t = glm::clamp(glm::dot(feet - from, ab) / std::max(glm::dot(ab, ab), 1e-4f), 0.f, 1.f);
            if (glm::length(glm::vec2{feet.x - (from + ab * t).x, feet.z - (from + ab * t).z}) < 1.6f) { ev.meleeHit = true; ev.meleeDamage = s.damage * tune_->damage; }
            position = end;
            break;
        }
        case AttackKind::ACRESCENT: {
            glm::vec3 eye = w.playerEye + leadVel_ * 0.4f;
            ev.shotOrigin = position + glm::vec3{0.f, 1.6f, 0.f};
            glm::vec3 aim = glm::normalize(eye - ev.shotOrigin);
            ev.shots = 2; ev.shotDir[0] = rotY(aim, 0.06f); ev.shotDir[1] = rotY(aim, -0.06f);
            ev.shotSpeed = 22.f; ev.shotDamage = s.damage * tune_->damage; ev.shotSize = 1.6f; ev.shotParry = 200.f;
            break;
        }
        case AttackKind::AJUDGE: ev.abJudge = true; stagger(AB_SPENT); abReeling = false; break;
        default: break;
    }
}
```

In `abBegin`: `AMIRAGE` isn't a string step. Instead TOO SLOW in phase 2
starts with `ev.abMirages = 2`, placing `ev.phantomPos[0..1]` with
`absSafeSpot` at ±70° around you, 3 m out. Do this when
`abCur == AB_TOO_SLOW && abStep == 0 && abPhase >= 2 && (abRot % 2 == 0)`.

The last judgment's resolve: `thinkAbsolute` calls `abResolve` on `resolve`,
but `AJUDGE` isn't part of a string. Branch at the top of `abResolve`:
`if (attack == AttackKind::AJUDGE) { abResolveLate(w, feet, d, AbStep{AttackKind::AJUDGE, 0, 0, 3, 0}); attack = AttackKind::NONE; return; }`.

- [ ] **Step 4: Run** `make test` → green.

- [ ] **Step 5: Commit**

```bash
git add src/EnemyAbsolute.h src/Enemy.h src/AbsoluteHazards.h tests/test_zenith.cpp
git commit -m "The Absolute's ascension and his blade: at 60 % he rises and sweeps the arena, strings grow a hit and quicken, seams, mirages and the zenith ray join in; at 25 % the blade of light (a sword parry shatters it), the lunge and crescents; at 8 % the last judgment, once"
```

---

### Task 9: The fight in the game (`Gameplay_Absolute.h`, his rig, drawing, banners, lines)

**Files:**
- Create: `src/Gameplay_Absolute.h`
- Modify: `src/GameplayState.h` (members `AbsoluteHazards abh; Enemy* absolute(); void onAbsoluteEvents(Enemy&, const EnemyEvents&); void updateAbsolute(float dt); void renderAbsolute(...); float abLineT = 0.f; int abLineShown = -1; float abDeathT = 1e9f; Enemy abCorpse;` and the include)
- Modify: `src/Gameplay_Combat.h` (`punch()` parries the Absolute through `parryAbsolute`; hitscan: deflect at 14 m via `deflects`, mirages hit by `mirageAlong`, reflected serpents/crescents/rays damage him; on his death `abDeathT = 0`)
- Modify: `src/Gameplay_Tick.h` (fill `EnemyWorld` Zenith fields and `onShard`; call `updateAbsolute`; route `ev.ab*`)
- Modify: `src/Gameplay_Flow.h` (`enterArena`: `abh.clear(); level.restoreZenith(); abLineShown = -1; abDeathT = 1e9f;`)
- Modify: `src/EnemyModel.h` (the ABSOLUTE rig and poses), `src/Gameplay_Render.h` (pillars, shards, hazards, afterimages, white-out), `src/Gameplay_HUD.h` (his lines as subtitles)
- Modify: `src/Gameplay_Dev.h` + `src/Gameplay_Flow.h:222` (`--overlay absolute2|absolute3|absolute8` set his health to 0.59/0.24/0.079)
- Test: `tests/test_zenith.cpp`

**Interfaces:**
- Consumes: everything above.
- Produces: `bool GameplayState::parryAbsolute(Enemy& e, bool bySword)` (replacing Task 3's stub). It calls `e.abParried(bySword)` and on success gives feedback, heals `Sword::HEAL_PARRY` when `bySword`, and shows toasts ("REELING" / "BROKEN - THREE SECONDS" / "HIS BLADE SHATTERS"). Also a pure helper for the retry test in `AbsoluteHazards.h`:
  `inline void resetZenithFight(LevelData& lv, AbsoluteHazards& h) { h.clear(); lv.restoreZenith(); }`.

- [ ] **Step 1: Failing test**

```cpp
    // ---------------------------------------------------------------- the fight: a retry puts the Zenith back
    {
        LevelData Z = buildAct2Level();
        AbsoluteHazards h;
        for (int k = 0; k < 8; k += 2) Z.parkZenithPiece(false, k);
        for (int k = 0; k < 6; ++k) Z.parkZenithPiece(true, k);
        h.addRay(ZC, ZC); h.addSeams(ZC, 0); h.addJudge(ZC, ZC);
        resetZenithFight(Z, h);
        bool all = true;
        for (int k = 0; k < 8; ++k) all = all && Z.zenith.pillarUp[k];
        for (int k = 0; k < 6; ++k) all = all && Z.zenith.shardUp[k];
        CHECK(all && h.rays.empty() && h.seams.empty() && h.judges.empty(), "a retry in the Zenith restores the pillars and shards and clears his hazards");
    }
```

- [ ] **Step 2: Run, see it fail** → `resetZenithFight` undeclared. Add it (Interfaces), run, green.

- [ ] **Step 3: The game side.** `src/Gameplay_Absolute.h`, following
`Gameplay_Leviathan.h`'s layout:

```cpp
#pragma once
// =============================================================================
// Gameplay_Absolute.h — GameplayState: THE ABSOLUTE's fight in the Zenith.
// His events turned into hazards and damage (AbsoluteHazards.h), parries (the
// punch or the sword), his guard against long shots, the pillars that
// shatter and the shards that fall, his lines on screen, and his death.
// Included at the end of GameplayState.h.
// =============================================================================

inline Enemy* GameplayState::absolute() {
    for (auto& e : enemies) if (e.alive && e.type == EnemyType::ABSOLUTE) return &e;
    return nullptr;
}

inline bool GameplayState::parryAbsolute(Enemy& e, bool bySword) {
    int r = e.abParried(bySword);
    if (!r) return false;
    parryFeedback(e.position + glm::vec3{0.f, 1.6f, 0.f}, r >= 2);
    styleSystem.addStyle(r >= 2 ? 80.f : 40.f, StyleSource::PARRY);
    if (bySword) styleSystem.heal(Sword::HEAL_PARRY * tune().heal);
    gainXp(r >= 2 ? 40 : 15);
    audio.play("ab_ting", 128);
    if (r == 1) ui.feed("REELING", {1.f, 1.f, 1.f});
    if (r == 2) { ui.toast("BROKEN", "THREE SECONDS - DOUBLE DAMAGE", {1.f, 0.95f, 0.7f}, 1.6f); shake(0.4f, 0.06f); }
    if (r == 3) { ui.toast("HIS BLADE SHATTERS", "THREE SECONDS - DOUBLE DAMAGE", {1.f, 0.4f, 0.55f}, 1.8f); shake(0.6f, 0.08f);
                  fx.spawnBurst(e.position + glm::vec3{0.f, 1.8f, 0.f}, {1.6f, 1.6f, 1.7f}, 60, 14.f, 0.7f, 6.f); }
    return true;
}

inline void GameplayState::onAbsoluteEvents(Enemy& e, const EnemyEvents& ev) {
    const float scale = level.arenas[director.arena].damageScale * tune().damage;
    const glm::vec3 white{1.6f, 1.6f, 1.7f}, gold{1.6f, 1.15f, 0.45f}, red{1.5f, 0.2f, 0.15f};
    if (ev.abTell == (int)AbTell::WHITE) audio.playAt("ab_ting", e.position, 120, SoundGroup::ENEMY, true);
    if (ev.abTell == (int)AbTell::GOLD)  audio.playAt("ab_thrum", e.position, 128, SoundGroup::ENEMY, true);
    if (ev.abTell == (int)AbTell::RED)   audio.playAt("telegraph", e.position, 128, SoundGroup::ENEMY, true);
    if (ev.abLine >= 0) { abLineShown = ev.abLine; abLineT = 1.6f; }
    if (ev.blinked) fx.addAfterimage(ev.blinkFrom, e.yaw, e.attack);   // a fading white copy (Effects.h)
    if (ev.abPhase == 2) { pushBanner("HE ASCENDS", "STAY CLOSE", {1.f, 0.95f, 0.8f}, 3.f); shake(0.5f, 0.06f); audio.duck(6.f, 0.6f); }
    if (ev.abPhase == 3) pushBanner("THE BLADE OF LIGHT", "MEET IT", {1.f, 0.5f, 0.6f}, 3.f);
    if (ev.abHalo) abh.addHalo(level.zenith.centre, level.zenith.centre.y);
    if (ev.abShards) for (int k = 0; k < (int)level.zenith.shardWalls.size(); ++k) { level.parkZenithPiece(true, k); shardFall[k] = 0.f; }
    if (ev.abRay) abh.addRay(ev.abFrom, ev.abTo);
    if (ev.abStomp) { abh.addStomp(ev.abFrom); fx.spawnShockwave(ev.abFrom, 3.f, red); audio.playAt("slam", ev.abFrom, 120, SoundGroup::ENEMY); }
    if (ev.abSerpent) abh.addSerpent(ev.abFrom, glm::normalize(player.position - e.position + glm::vec3{0, 0.01f, 0}));
    if (ev.abSeams) abh.addSeams(level.zenith.centre, ev.abSeamFirst);
    if (ev.abZenith) abh.addZenith(ev.abFrom, ev.abTo);
    for (int k = 0; k < ev.abMirages; ++k) abh.addMirage(ev.phantomPos[k]);
    if (ev.abJudgeMark) abh.addJudge(level.zenith.centre, e.position);
    if (ev.abLaunch) { player.velocity.y = 11.f; player.onGround = false; }
    if (ev.meleeHit) {
        AbTell t = abTellOf(e.attack == AttackKind::NONE ? e.lastAbAttack : e.attack);
        damagePlayerStrike(ev.meleeDamage * scale, e.position + glm::vec3{0, 1.5f, 0},
                           t == AbTell::GOLD ? Sword::Strike::GOLD : Sword::Strike::WHITE);
        if (lastGuarded == Sword::Guarded::PARRIED) parryAbsolute(e, true);
    }
}

inline void GameplayState::updateAbsolute(float dt) {
    Enemy* e = absolute();
    glm::vec3 bp = e ? e->position : level.zenith.centre;
    const float scale = director.arena < (int)level.arenas.size() ? level.arenas[director.arena].damageScale * tune().damage : 1.f;
    for (auto& h : abh.update(dt, player.position, player.height, player.onGround, &level, bp)) {
        // a ray in its first 0.15 s, met with the sword's parry, goes back to him
        if (h.kind == Sword::Strike::BEAM && swordOut && !abh.rays.empty() && abh.rays.front().t <= AbsoluteHazards::RAY_PARRY && sword.tryParry()) {
            abh.rayParry(0);
            if (e) hurtEnemy(*e, AbsoluteHazards::RAY_REFLECT, bp + glm::vec3{0, 1.6f, 0}, 50.f, Sword::HEAL_PARRY, StyleSource::PARRY);
            ui.toast("RAY RETURNED", "", {1.f, 0.9f, 0.9f}, 1.2f);
            continue;
        }
        damagePlayerStrike(h.damage * scale, h.from, h.kind, h.tick ? dt : 0.f);
    }
    if (int m = abh.haloShatterMask(level)) for (int k = 0; k < 8; ++k) if (m & (1 << k)) {
        glm::vec3 p = zenithPillar(k);
        level.parkZenithPiece(false, k);
        fx.spawnBurst(p + glm::vec3{0, 7, 0}, {0.9f, 0.88f, 0.85f}, 80, 12.f, 1.2f, 14.f);
        audio.playAt("explosion", p, 128, SoundGroup::WORLD);
    }
    // a serpent within reach of a parry (the punch's window, or the sword's)
    int s = abh.serpentNear(player.camera.position, 2.2f);
    if (s >= 0 && ((swordOut && sword.tryParry()) || punchedThisTick)) { abh.reflectSerpent(s); parryFeedback(abh.serpents[s].pos, false); }
    if (e) for (int n = abh.reflectedHits(bp + glm::vec3{0, 1.3f, 0}, 1.4f); n > 0; --n)
        hurtEnemy(*e, AbsoluteHazards::SERPENT_PARRY, bp + glm::vec3{0, 1.3f, 0}, 40.f, 4.f, StyleSource::PARRY);
    if (abLineT > 0.f) abLineT -= dt;
    for (auto& [k, t] : shardFall) t += dt;   // falling shards: drawn sinking (render)
    updateAbsoluteDeath(dt);
}
```

`lastAbAttack` is a new Enemy field that `abResolve` sets before clearing
`attack`, so the game knows which strike landed. `punchedThisTick` is set in
`punch()`, and `punch()` gains, before its generic enemy loop:
`for (auto& e : enemies) if (e.type == EnemyType::ABSOLUTE && e.parryWindow() && withinReach(e)) { if (parryAbsolute(e, false)) return; }`.
`shardFall` is a `std::map<int, float>` member. The sword's parry of a melee
hit already works through `damagePlayerStrike` → `PARRIED` → `parryAbsolute(e, true)`.
His strike must not land when parried: in `onAbsoluteEvents` the damage call
returns early on `PARRIED`, so nothing else is needed.

His guard: in hitscan (`Gameplay_Combat.h:752`), the existing
`if (e.deflects(dir, hits[k].t))` branch now also covers the Absolute (Task 6).
Make its feedback say `"TURNED ASIDE - GET CLOSER"` for him. Mirages: before
the enemy loop in hitscan, `int m = abh.mirageAlong(eye, dir, range); if (m >= 0) { abh.killMirage(m); fx.spawnBurst(...); }`.
A sword stroke kills any mirage within the arc too.

Death: in `onEnemyKilled`, for the Absolute,
`abCorpse = e; abCorpse.alive = true; abDeathT = 0.f; abLineShown = 4; abLineT = 3.f; abh.clear();`.
`updateAbsoluteDeath` kneels him (0–1 s), cracks the halo (1–1.8 s), then
dissolves him upward in white bursts (1.8–3 s). The director's VICTORY
arrives first; Task 5 already set `victoryDelay` 3 s for Act II, which this
fits.

`EnemyModel.h`: an ABSOLUTE rig built as the existing humanoid rigs
(`husk`/`sovereign` builders): 2.6 m tall, all parts with an emissive of
`stats().glow * 0.9`, the head with a gold slit, the broken halo (eight boxes
on a ring of radius 0.55 behind the head, two missing), and wrist wraps (thin
boxes trailing back from the forearms by `moveSpeed`). Poses by `attack`:
jab (the right arm straight out), hook (the left arm swung across), uppercut
(crouched, the right fist low), drop-kick (in the air, one leg out), ray (the
right palm forward, glowing red during the tell), stomp (the right knee
high), serpent (both hands together, forward), sweep/lunge (the blade in the
right hand, held per the Sovereign's poses), judgment (arms wide, floating).
The tell flash: during any tell, the striking limb's emissive is
`white`/`gold`/`red` per `abTellOf(attack)` × (1 + 2 × telegraphProgress()).
The blade, when `abBlade`: a 1.4 m box of emissive white-pink.

`Gameplay_Render.h`: draw the standing pillars from `level.zenith.pillarHome`
with the BoxRenderer (white stone, glowing cracks), and the standing shards.
Fallen shards sink `shardFall[k] * 9 m/s` and fade over 3 s. Hazards: rays
as long thin emissive boxes from `from` through `at` (a red line during the
tell, which is drawn from the telegraph state, then white-hot); stomp rings as
`fx` shockwaves; serpents as a chain of 6 glowing boxes trailing their
direction; seams as lit floor cells (red at alpha `t / SEAM_WARN` before
each wave); the zenith ray as a vertical emissive column at `at`; the halo
sweep as a 46 m emissive bar at chest height turning about the centre;
mirages as his rig at 40 % emissive with a flicker; the last judgment as
every seam red except a 6 m disc around him. Afterimages: `Effects` gains
`struct Afterimage { glm::vec3 pos; float yaw, t; AttackKind pose; }` with
life 0.35 s, drawn as his rig at alpha `1 - t / 0.35`, emissive only.

`Gameplay_HUD.h`: while `abLineT > 0`, draw `abLine(abLineShown)` centred at
62 % of the screen height, size 5, white, alpha `min(1, abLineT / 0.4)`, with
a dark drop shadow.

`Gameplay_Tick.h`: where the `EnemyWorld` is built for the enemies, add
`w.hasZenith = level.zenith.built && act2() && director.arena == ZENITH_ARENA; w.zenCentre = level.zenith.centre; w.zenFloor = level.zenith.centre.y; w.level = &level; w.onShard = level.onZenithShard(player.position);`.
After `e.update`, `if (e.type == EnemyType::ABSOLUTE) onAbsoluteEvents(e, e.ev);`.
Skip the generic `integrate`, debris and separation for him, as the Leviathan
does. Call `updateAbsolute(dt)` when `w.hasZenith`.

- [ ] **Step 4: Run and look**

`make && make test` → green.
`./shooter --act2 --arena 6 --god --sword`: fight him for two minutes.
Expected: white/gold/red flashes before every strike, the sword parry
breaks him, pillars shatter at 60 %, the shards fall at 25 %, and his lines
show on screen.
Screenshots: `--overlay absolute2` / `absolute3` / `absolute8` with
`--cam ... --shot 90 ...bmp` for each phase. Read each BMP. If his rig
reads as a white silhouette with no form, lower its emissive to 0.6.

- [ ] **Step 5: Commit**

```bash
git add src/Gameplay_Absolute.h src/GameplayState.h src/Gameplay_Combat.h src/Gameplay_Tick.h src/Gameplay_Flow.h src/Gameplay_Render.h src/Gameplay_HUD.h src/Gameplay_Dev.h src/EnemyModel.h src/Effects.h src/AbsoluteHazards.h src/Enemy.h tests/test_zenith.cpp
git commit -m "The Absolute in the Zenith: a figure of white light whose every strike flashes its colour, parried with a punch or the sword, his guard turning long shots aside, pillars shattering and shards falling, rays, seams, serpents and mirages drawn, his lines on screen, and his death on his knees"
```

---

### Task 10: Against scripted players (BossSim), and tuning

**Files:**
- Modify: `tests/BossSim.h` (an Absolute branch: its world, events, movement and parries)
- Test: `tests/test_zenith.cpp` (include `../src/SovereignHazards.h`, `../src/WardenHazards.h`, `../src/LeviathanHazards.h`, `../src/Projectile.h`, `../src/ArenaShifts.h`, then `"BossSim.h"`, exactly as `test_game.cpp:25-34`)

**Interfaces:**
- Consumes: Tasks 6–8, `AbsoluteHazards`.
- Produces: `BossSim::Policy` gains `STILL` (never moves, never parries) and
  `DODGER` (SOLID's movement and dodges, guns only, no parries). SOLID
  against the Absolute uses the sword: DPS 55 within 3 m, 30 beyond 14 m
  (the guard turns most shots aside). It parries every other white window and
  blocks gold strikes it can't dodge.

- [ ] **Step 1: The tests**

```cpp
    // ---------------------------------------------------------------- THE ABSOLUTE against scripted players
    {
        auto run = [&](BossSim::Policy p, float limit) {
            BossSim s(N, g, ZA, EnemyType::ABSOLUTE, p);
            while (s.t < limit && s.bossAlive() && s.damageTaken < 1e5f) s.step(DT);
            return s;
        };
        BossSim solid = run(BossSim::SOLID, 400.f);
        std::printf("      absolute: solid kills in %.0f s, taking %.0f\n", solid.t, solid.damageTaken);
        CHECK(!solid.bossAlive() && solid.t >= 150.f && solid.t <= 180.f, "a solid player (sword, half the parries) kills the Absolute in 150-180 s");
        BossSim dodger = run(BossSim::DODGER, 900.f);
        std::printf("      absolute: guns-only dodger kills in %.0f s\n", dodger.t);
        CHECK(!dodger.bossAlive() && dodger.t > solid.t, "a dodge-only, guns-only player can still win, more slowly");
        BossSim still = run(BossSim::STILL, 30.f);
        CHECK(still.damageTaken >= 100.f, "a player who stands still is dead inside 30 s");
        bool punished = true;
        for (auto pol : {BossSim::CORNER, BossSim::PERCH, BossSim::KITE, BossSim::RANGED})
            for (float hp : {1.f, 0.55f, 0.2f}) {
                BossSim c(N, g, ZA, EnemyType::ABSOLUTE, pol);
                c.boss.health = c.boss.maxHealth * hp; c.boss.abArrive = 0.f;
                if (hp < 0.6f) c.boss.abPhase = 2;
                if (hp < 0.25f) { c.boss.abPhase = 3; c.boss.abBlade = true; }
                float first = -1.f;
                while (c.t < 8.f && first < 0.f) { c.step(DT); if (c.damageTaken > 0.f) first = c.t; }
                if (first < 0.f || first > 5.f) { punished = false; std::printf("      not punished: policy %d at %.0f%%\n", (int)pol, hp * 100); }
            }
        CHECK(punished, "every cheese (corner, perch, edge-kiting, ranged-only) is punished within 5 s, in every phase");
    }
```

- [ ] **Step 2: Run, see it fail** (BossSim knows nothing of him: he never
gets `hasZenith`, the hazards never hurt, SOLID shoots from 10 m).

- [ ] **Step 3: BossSim's Absolute branch**

- `world()`: `w.hasZenith = A.name && std::string(A.name) == "THE ZENITH"; w.zenCentre = zenithCentre(); w.zenFloor = w.zenCentre.y; w.level = &L; w.onShard = L.onZenithShard(feet);`
- `groundY()`: the Zenith's floor for him.
- Hazards: an `AbsoluteHazards abs;` member. In `step`, after `boss.update`:
  feed `ev.ab*` as `onAbsoluteEvents` does (rays, stomps, serpents, seams,
  zenith, halo, mirages, judge, launch), and `hurt()` every hit from
  `abs.update(dt, feet, height, grounded, &L, boss.position)`. `height` is
  0.9 while SOLID/DODGER "slides" (during a halo sweep), else 1.8. `grounded`
  is false while jumping a stomp ring. Melee: `ev.meleeHit` → `hurt(ev.meleeDamage)`,
  unless SOLID is blocking a gold strike (×0.25).
- Movement for SOLID/DODGER against him: hold 2.5 m (SOLID) or 16 m (DODGER).
  From `REACT` after a tell: on red, step out of the marked thing (strafe 7
  m/s perpendicular to a ray, jump stomp rings as they reach you, stand in a
  cell of the safe parity, run 8 m from the zenith ray's point, slide under
  the halo, go to him during the judgment). On gold, dash 5 m sideways (SOLID
  blocks instead when a dash is on cooldown, 1 s). On white, SOLID parries
  every other window (`boss.abParried(true)`) and otherwise takes it.
  Serpents: run perpendicular.
- `shoot()` for him: SOLID does 55 DPS × `armorMult()` when within 3.2 m
  (sword), else 0. DODGER does 40 DPS × `armorMult()` within 14 m, and 15 % of
  that beyond (the guard).
- `STILL` stays at `playerStart`.
- Cheese policies already exist. PERCH picks the highest walkable top: in the
  Zenith that's the Dais (or a shard, if its top is higher; make sure PERCH
  picks a shard by preferring tops beyond r 50 when the arena is the Zenith).

- [ ] **Step 4: Tune until green**

Run `make test` and read the printed kill times. Adjust **only** these
levers, in this order:
1. `statsOf(ABSOLUTE).health` within 7000–11000. Also update the "9000 hp"
   check and the spec's number if it moves.
2. The recovery between strings (`recover(0.8f * quick)`, never below
   `RECOVER_MIN`).
3. String damages, keeping each whole string at 60–70.
Record the final numbers in the commit message.

- [ ] **Step 5: Commit**

```bash
git add tests/BossSim.h tests/test_zenith.cpp src/Enemy.h src/EnemyAbsolute.h
git commit -m "The Absolute against scripted players: a solid player with the sword kills him in <N> s, a guns-only dodger still wins in <M> s, standing still dies inside 30 s, and every cheese is punished within 5 s in every phase"
```

---

### Task 11: Sound, voice, music; README; final verification

**Files:**
- Modify: `tools/gen_sfx.py` (`sword_draw`, `sword_swing`, `sword_hit`, `sword_thrust`, `sword_guard`, `sword_block`, `sword_break`, `ab_ting`, `ab_thrum`) → `assets/sfx/*.wav`
- Modify: `src/MixTable.h` (their classes and levels)
- Modify: `src/VoiceTable.h`, `src/VoiceSynth.h`, `src/EnemyVoice.h` (his profile: a bright high choir "EE", body `HUM`, small/quick; specials `v_absolute_line` ×5 phrases and `v_absolute_death`)
- Modify: `README.md` (the encore, the sword, controls, `--sword`, `--overlay absolute2/3/8`)
- Test: `tests/test_zenith.cpp`, `tests/test_game.cpp:2719` (the voice bank's total-length bound)

**Interfaces:**
- Consumes: every sound name used in Tasks 3 and 9.

- [ ] **Step 1: Failing test**

```cpp
    // ---------------------------------------------------------------- the encore's sounds
    {
        for (const char* n : {"sword_draw", "sword_swing", "sword_hit", "sword_thrust", "sword_guard", "sword_block", "sword_break", "ab_ting", "ab_thrum"})
            CHECK(mixEntry(n) != nullptr, n);
        int lines = 0; for (auto& v : voiceBank()) lines += std::string(v.name).rfind("v_absolute_line", 0) == 0;
        CHECK(lines == 1, "his lines have a choir phrase in the voice bank");
    }
```

(Include `"../src/MixTable.h"` and `"../src/VoiceTable.h"`. Use the actual
bank accessor name from `VoiceTable.h` (the function that returns `bank`)
and its entry's name field.)

- [ ] **Step 2: Run, see it fail.**

- [ ] **Step 3: The sounds**

`gen_sfx.py`, following `gen_parry`'s style:

```python
def gen_sword_swing():
    """A blade through the air: a fast band-passed whoosh that rises then falls."""
    n = n_samples(0.22)
    noise = highpass(lowpass([white() for _ in range(n)], 5000), 700)
    return [noise[i] * math.sin(math.pi * i / n) ** 1.5 * 0.8 for i in range(n)]


def gen_sword_hit():
    """Steel through something hard: a bright inharmonic ring over a short crunch."""
    n = n_samples(0.35)
    crunch = lowpass([white() for _ in range(n)], 2500)
    out = []
    for i in range(n):
        t = i / n
        ring = sine(1870, i) * math.exp(-9 * t) + sine(2930, i) * 0.6 * math.exp(-12 * t) + sine(4410, i) * 0.3 * math.exp(-16 * t)
        out.append(ring * 0.55 + crunch[i] * math.exp(-40 * t) * 0.9)
    return out


def gen_sword_draw():
    """Drawing it: a long scrape that brightens, ending on a ring."""
    n = n_samples(0.5)
    noise = highpass([white() for _ in range(n)], 3000)
    return [noise[i] * 0.35 * (i / n) * math.exp(-2 * (1 - i / n)) + sine(2400, i) * 0.4 * max(0.0, (i / n - 0.8) * 5) * math.exp(-8 * max(0.0, i / n - 0.8)) for i in range(n)]


def gen_sword_thrust():
    """The thrust: a heavy push of air with a steel edge on it."""
    n = n_samples(0.3)
    air = lowpass([white() for _ in range(n)], 1200)
    return [air[i] * math.sin(math.pi * i / n) * 0.9 + sine(1600, i) * 0.15 * math.exp(-6 * i / n) for i in range(n)]


def gen_sword_guard():
    """Raising the guard: a short metallic shink."""
    n = n_samples(0.12)
    return [sine(3200, i) * math.exp(-25 * i / n) * 0.5 + white() * math.exp(-60 * i / n) * 0.4 for i in range(n)]


def gen_sword_block():
    """A blow on the blade: a hard clang."""
    n = n_samples(0.4)
    return [(sine(620, i) + sine(1340, i) * 0.7 + sine(2210, i) * 0.4) * math.exp(-8 * i / n) * 0.5 + white() * math.exp(-50 * i / n) * 0.5 for i in range(n)]


def gen_sword_break():
    """The guard breaks: a crack and a dying, detuned ring."""
    n = n_samples(0.6)
    return [white() * math.exp(-30 * i / n) * 0.9 + (sine(880, i) + sine(905, i)) * 0.35 * math.exp(-5 * i / n) for i in range(n)]


def gen_ab_ting():
    """His white tell: a pure high bell - parry this."""
    n = n_samples(0.45)
    return [(sine(2093, i) + sine(4186, i) * 0.35) * math.exp(-7 * i / n) * 0.6 for i in range(n)]


def gen_ab_thrum():
    """His gold tell: a low swelling thrum - don't parry this."""
    n = n_samples(0.5)
    return [(sine(73, i) + sine(146, i) * 0.6 + sine(219, i) * 0.3) * math.sin(math.pi * i / n) * 0.8 for i in range(n)]
```

Register them in `GENERATORS`, then run `python3 tools/gen_sfx.py sword_draw sword_swing sword_hit sword_thrust sword_guard sword_block sword_break ab_ting ab_thrum`.
`MixTable.h`: `sword_*` in `C::PLAYER` near `parry` (0–3 dB), and
`ab_ting`/`ab_thrum` in `C::ACTION` at +2 dB (tells must cut through).
Check the loader picks new names up from the mix table / directory the same
way it does `parry`. If there's a list of files, add them there too.

Voice: a profile for `ABSOLUTE` in `VoiceSynth.h` (high bright choir, little
body). Its attack tells for `AJAB/AHOOK/...` map to short choir hits
(`VoiceTable.h:74`'s per-type list:
`{A::AJAB, A::AUPPER, A::ARAY, A::ASWEEP, A::ALUNGE, A::AJUDGE}`). A special
`v_absolute_line` (one variant: a falling three-note choir phrase, ~0.8 s)
plays with each on-screen line, and `v_absolute_death` plays with
"...WORTHY.". `EnemyVoice.h`: on `ev.abLine >= 0` cue `v_absolute_line`. Raise
`test_game.cpp:2719`'s bank bound only if the new voices push it over 200 s,
and say so in the commit.

`README.md`: an "Encore" section (how you get there, THE ABSOLUTE in a
paragraph, the colour rule, the sword's controls), the dev flags, and the
updated line count (`wc -l src/*.h src/*.cpp`).

- [ ] **Step 4: Final verification**

Run each and read the output:
- `make && make test`: all three suites `ALL PASSED`.
- `source ~/emsdk/emsdk_env.sh && make web`: builds with no new warnings.
- Native screenshots at each phase (`--overlay absolute2/3/8`), read back.
- Headless web check (the puppeteer setup in memory: cached puppeteer-core +
  system Chrome `--use-angle=metal`): `?act2&arena=6&god&sword` loads with no
  console errors and holds ~60 fps on the M2.

- [ ] **Step 5: Commit**

```bash
git add tools/gen_sfx.py assets/sfx src/MixTable.h src/VoiceTable.h src/VoiceSynth.h src/EnemyVoice.h README.md tests/test_game.cpp tests/test_zenith.cpp
git commit -m "The encore's sound: the sword's draw, swing, hit, thrust, guard, block and break, his bright white tell and low gold one, his choir and a phrase under each line; README: the Zenith, the Absolute and the sword"
```

Then **stop and ask the user** before merging `zenith` into main, pushing, or
syncing the site build. Their own playtest is the real tuning pass.
