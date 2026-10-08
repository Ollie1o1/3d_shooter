# Act II piece 6: The Maw + THE LEVIATHAN — Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Finish Act II: a fifth arena, the Maw, under the Reliquary's hole,
and its final boss, the Leviathan (three phases), ending the run in a real
victory.

**Architecture:** This follows the Warden and Penitent pattern. The mind
(`src/EnemyLeviathan.h`) only emits `lv*` events. What hurts you lives in
`src/LeviathanHazards.h` (no GL or audio). The game side is
`src/Gameplay_Leviathan.h`. The body is a curve from a root (the pool or a
well) to the head (`Enemy::position`). Its segment centres come from
`leviathanBody()`, which drawing, hit tests and tests all share.

**Tech Stack:** C++17 header-only, glm, headless tests (`tests/test_game.cpp`,
CHECK), Emscripten web build.

**Spec:** `docs/superpowers/specs/2026-10-08-act2-leviathan-design.md`

## Global Constraints

- Maw centre M = (0, −300, −1068): pool r 13 (open void), ring r 13–46, wells
  r 29 (radius 3.5) on the diagonals, six ribs at r 42 with ledges at +5 m
  and steps at +2.5 m, the wall at r 46–48, `voidY` −315, HALL reverb, track
  9 "LEVIATHAN", `MUSIC_TRACKS` 10, `damageScale` 1.5.
- Leviathan: 15000 hp (tuned up from 9000). Phases at 65 % and 30 %. Body ×0.2, head ×1, eye ×3
  beached or staggered, eye ×2 in phase 3, throat ×3 while inhaling.
- CRASH: tell 1.1 s, strip 5 m wide, 40 damage. Beached 3 s. Parry in the
  last 0.25 s within 6 m of the landing point → stagger 4 s.
- TORRENT: tell 0.8 s, 9 orbs (11 on Standard+) of 12; a parried orb → eye,
  180.
- TIDE: tell 0.9 s, wave out to r 46 (from a well, r 20), 1.2 m high, 25.
- SPIT: after 4 s beyond 34 m, out of sight, or 4 m+ above the ring. 1 s
  marker, radius 4, 30, then a pool burning 4 s at 15/s.
- HUNT: dive 0.8 s, hidden 1.4 s, breach tell 1.2 s at the site nearest you
  (not the last one), eruption radius 6 for 40 and a knock up, 0.8 s
  recovery, two attacks per site.
- ECLIPSE: flood to 0.6 m over 4 s, tells ×0.75. SWALLOW every third attack:
  tell 0.9 s, inhale 3 s, pull 3.5 m/s, bite at 4.5 m for 45, 450 into the
  throat in one inhale chokes it (stagger 4 s).
- Tell floors 0.35 s (opening) and 0.28 s (follow-up). Phase change pause
  0.6 s. A 3 s rise on arrival.
- Text exactly as in the spec's table. No Claude/Co-Authored-By trailers in
  commits (user rule).
- After every task: `make && make test` green.

## Review Focus

1. **Dying or retrying mid-fight (flooded, hidden, inhaling)**: `enterArena`
   puts the water back down and clears every Leviathan hazard and pull
   (Task 5 test "a retry drains the Maw and clears its hazards").
2. **The Leviathan killed while hidden or beached**: it can't be hit while
   hidden, so a hidden kill never happens. A beached death still sinks and
   ends the run (Task 2 test "untargetable while hidden").
3. **A parried TORRENT orb when the Leviathan dives mid-flight**: the orb
   stops homing and flies straight on (Task 5 test "homing drops a hidden
   target").
4. **Standing in a well or on the pool's lip when a breach is picked**: the
   tell is always 1.2 s, and the eruption only hits within 6 m (Task 3
   test).
5. **The swallow's pull with the player on a ledge or in mid-air**: the pull
   only drags you horizontally and never through walls, because it's applied
   as velocity through the normal physics (Task 5).

---

## File Structure

- Create `src/EnemyLeviathan.h`: the mind, body curve and hit zones.
- Create `src/LeviathanHazards.h`: strips, tides, spits, breaches, pull and
  bite.
- Create `src/Gameplay_Leviathan.h`: events into hazards, damage, flood, hits,
  drawing, HUD, death.
- Modify `src/Enemy.h`, `src/LevelAct2.h`, `src/Level.h` (if needed),
  `src/Gameplay_Combat.h`, `src/Gameplay_Flow.h`, `src/Gameplay_Tick.h`,
  `src/Gameplay_HUD.h`, `src/Gameplay_Render.h`, `src/GameplayState.h`,
  `src/EnemyModel.h`, `src/VoiceTable.h`, `src/VoiceSynth.h`,
  `src/EnemyVoice.h`, `src/MusicSynth.h`, `src/MenuState.h`,
  `src/Progression.h`, `Makefile`, `tests/test_game.cpp`, `tests/BossSim.h`,
  `README.md`.

### Task 1: The Maw (map, hole, corridor, retry start)

- Rework the Reliquary's hole (centre −1050, r 22, no south rocks), zone and
  bounds (end at −1022), and remove the finish beacon. Add `buildMaw` with
  the pool, ring, wells, ribs, ledges, steps, pipes, wall, water volume,
  arena, zone with the hole column, `voidY`, respawn, boss spawn and track.
- Tests: 5 arenas and 4 corridors. Ring floor −300 at r 20/35/45 on several
  bearings. The pool is open at r 6. The wells are at r 29. The ledges are
  at +5 with steps at ≤ 2.6 m. A rim drop lands in the Maw zone at −300 (the
  Reliquary's `arenaAt` doesn't claim it). `playerStart` is above the ring.
  Update the old tests (3 corridors, the finish on the rim, `MUSIC_TRACKS`).

### Task 2: The Leviathan's body and mind (phases, CRASH, TORRENT, TIDE, SPIT)

- `EnemyType::LEVIATHAN`, attack kinds, fields, events; `isBoss`; stats;
  `armorMult`, `parryWindow`, `staggerTime`, `targetable`, tell floor;
  `thinkLeviathan`; `leviathanBody`, `leviathanHead/Eye/Throat` boxes;
  `leviathanZoneMult(e, zone)`.
- Tests: the rise (3 s, untargetable), phases at 65 and 30 %, multipliers, the
  CRASH tell and strip target, the beached 3 s, the parry window rule, the
  stagger at 4 s, the TORRENT event and parry value 180, the TIDE event, SPIT
  only when camping, tell floors on every difficulty and phase.

### Task 3: HUNT and ECLIPSE (dive, breach, flood signal, swallow, choke)

- In the mind: the dive, hidden and breach stages; site choice; two attacks
  per site; phase 3's return to the pool, `levFlood`, SWALLOW and inhaling;
  `onThroatDamage` → choke.
- Tests: hidden is untargetable; the breach site is nearest (not the last);
  the 1.2 s tell; SWALLOW every third attack; choke at 450; phase 3's tells
  ×0.75 with the floors.

### Task 4: LeviathanHazards

- Strips (marker, impact test with `half` 2.5), tides (ring, jump test),
  spits (marker → burst → pool), breaches (eruption radius 6), pull vector,
  bite test.
- Tests: strip hit/miss at 3 m to the side; tide hits grounded and misses
  airborne; spit burst plus pool ticks; breach at 6 vs 8 m; pull 3.5 m/s
  toward the head; bite at 4.5 m.

### Task 5: In the game (Gameplay_Leviathan.h and hooks)

- Events into hazards and damage; the flood level; the pull as velocity;
  hit zones in `hitscanAll` and blasts; parried orbs homing into the eye
  (generalise `homeOn`, drop a hidden target); the parry punch on CRASH;
  banners and toasts; the HUD line; drawing (body, head, jaw, eye, wake,
  strips, tides, spits, boiling wells); skip the generic rig, `integrate`,
  debris, floor and separation for it; on death, clear the hazards, sink, and
  the act's victory (delay, then `finishRun` and the "ACT II COMPLETE"
  screen); `enterArena` resets everything. Act II HUD shows 5 arenas; the
  menu line; Reliquary-cleared banner and waypoint to the hole; dev
  `--overlay leviathan2/3`.
- Tests: a retry drains the Maw and clears its hazards (on the
  LeviathanHazards/flood helper); homing drops a hidden target
  (`steerParried` target selection helper); a simulated Act II run reaches
  VICTORY in the Maw.

### Task 6: Voice and music

- Voice profile, a tell for every attack kind, specials (rise, phase roars,
  choke, death); track 9 LEVIATHAN.
- Tests: the existing "every attack has a tell" test passes with LEVIATHAN in
  it; `MUSIC_TRACKS == 10` and `musicTrack(9).name == "LEVIATHAN"`.

### Task 7: BossSim — cheese and kill time

- Extend BossSim with Leviathan hazards (strip, tide, spit, breach, bite,
  pull) and SOLID behaviour (shoots the head, the eye when open, the throat
  while inhaling; dashes out of strips; jumps tides).
- Tests: corner, perch, kite and ranged each take damage within 8 s. SOLID
  kills it in 200–280 s. Tune health and timings until both hold.

### Task 8: README, screenshots, bench, web, review, merge

- README section; screenshots of each phase; `--bench`; `make web`; a
  whole-branch review; fix what it finds; merge to main.
