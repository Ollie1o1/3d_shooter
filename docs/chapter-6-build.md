# Chapter 6: THE KEPT RING — build log and hand-off

The design is `docs/chapter-6-design.md`. This file is where the build stands
and what to do next. Started 2026-11-12 (Fable 5.2); written so another model
or person can carry on without the first session's context.

## Where it stands

Build order steps 1 and 2 of the design (section 11) exist and run: the new
ability, and the sweep with its countdown on a test ground. Step 3, the
playtest gate, is the user's to do. Nothing after that is started.

**Try it:**

| Command | What you get |
|---|---|
| `./shooter --story6` | The test ground: shelters, posts, sweeps every 25–40 s cycling far → mid (double) → near (breakers), three waves of stand-in enemies north of z 34, HOLDFAST on **Z**, half gravity. |
| `./shooter --story6 --sweep mid` | The same, one tier only (`far`, `mid`, `near`, `gather`). |
| `./shooter --training --sweep --holdfast` | Sweeps and the spike in any other level; every tall wall counts as shelter. |
| `./shooter --story6 --overlay sweepnow --god --shot 200 out.bmp` | A screenshot mid-countdown. |
| `make test-ring` | 64 checks on the rules, no window needed. |

**Verified on 2026-11-12:** `make` builds clean; `make test-ring` passes;
`make test` passes; in game the countdown, lit posts, front and the `SWEPT`
death screen were checked by screenshot (`build/ring/count_s.png`,
`build/ring/swept_s.png`). **Not verified:** nobody has played it by hand.
HOLDFAST's rope and pin, enemies being swept, the double sweep and breakers
are covered by the logic tests only, not seen in game.

## The name

The design calls the ability ANCHOR. The code calls it **HOLDFAST**, because
`EnemyType::ANCHOR`, the Penitent's anchors and `AttackKind::BALL_ANCHOR`
already exist. The user has not confirmed the name (design section 12,
question 8). If it changes: `src/player/Holdfast.h`,
`src/game/Gameplay_Holdfast.h`, `Act::HOLDFAST`, `g_devHoldfast`,
`--holdfast`, the `holdfast*` members of `GameplayState`, and the feed text
`HELD` / `PINNED` / `HELD FAST` / `LINE LOST`.

## What was built, by file

New:

- `src/world/RingSweep.h`: the rules, no OpenGL. `SweepShelter`,
  `SweepSpec`, `RingSweep` (flash → countdown → passing), `SweepSchedule`
  (when the next comes), `sweeptier::far/mid/near/aimed/gather` (the
  design's numbers), `sweepreach` (is shelter in reach).
- `src/player/Holdfast.h`: the spike's states, rope and pin timer.
- `src/world/LevelRing.h`: `buildRing()` (chapter 6, one section) and
  `ring::shelters()`, the single list of what counts as shelter.
- `src/game/Gameplay_Ring.h`: `tickSweep` (starts sweeps, kills, throws,
  sweeps enemies), `gatherSweep` (posts and front), `renderSweepCountdown`,
  `beginRing`, `updateRing`.
- `src/game/Gameplay_Holdfast.h`: aiming, latching, pinning, the rope after
  the player moves, drawing.
- `tests/test_ring.cpp`, and `test-ring` in the Makefile.

Changed (small hooks; search for `ring()`, `holdfast`, `sweep`):

- `GameplayState.h`: includes, `ring()`, members, declarations.
- `Gameplay_Flow.h`, `Gameplay_Wasteland.h`: chapter 6 builds `buildRing()`
  and asks for the `story6` bake; a retry resets the sweep and the spike.
- `Gameplay_Story.h`: begin / update / gather dispatch for chapter 6.
- `Gameplay_Tick.h`: the key, `holdfastRope()` straight after
  `player.update`, `tickHoldfast` and `tickSweep` next to `tickRecall`.
- `Gameplay_Render.h`: `gatherHoldfast`, `gatherSweep`; the sky uses
  Chapter 5's `uCold` branch for now.
- `Gameplay_HUD.h`: the countdown, and the objective line.
- `Gameplay_Menus.h`, `core/Bindings.h`: `Act::HOLDFAST`, default Z,
  rebindable (it appears in the settings page automatically).
- `DamageFeedback.h`: `DamageKind::SWEEP` → `SWEPT` on the death screen.
- `DevFlags.h`, `main.cpp`: `--story6`, `--sweep`, `--holdfast`,
  `--chapter 6`.

## How the sweep works in code (read before changing it)

- One `RingSweep` owns everything. The countdown is not a timer: it is
  `(your coordinate - the front's coordinate) / speed`, so the number and
  the visible wall cannot disagree. Running at the wall shortens it by at
  most walking speed ÷ 60 m/s.
- `clocked = true` (used by `sweeptier::aimed`) reverses that: the moment is
  fixed and the front is placed to match. Use it wherever the sweep must
  land on a beat.
- Shelter is one ray from chest height back toward the source, up to
  `SHADOW` (14 m), against `SweepShelter` boxes only. Above a shelter's top
  you are in the open.
- The design's rule "the countdown never lies" is enforced by structure:
  nothing outside `RingSweep` can change a started sweep. A TALLY should
  pass `rate = 2` to `SweepSchedule::tick`; do not touch `warning`.
- The player is killed with `damagePlayer(1e6, …, DamageKind::SWEEP)` after
  zeroing `invincFrames` (a dash must not get through). God mode and the
  training room's "never below 1 HP" still apply.
- A held player survives in the open only if `sweepShelters[i].holds` is set
  on what they are spiked to. Nothing sets it yet: it is for the Gap's raft
  and the Shepherd's hull.

## Shortcuts taken, to be replaced

1. **Breakers don't remove the wall.** A shattered small shelter stops
   sheltering and bursts, but its box is still drawn and solid. Make small
   shelters `Destructible`s (see `LevelLayout.h`, and how Ballast's
   windbreaks break and update collision).
2. **The sky is Chapter 5's** (aurora and all). There is no Shepherd in it.
3. **No light bake.** The game prints `story6.light: no bake`. See `make
   light` once the first real level exists.
4. **Enemies are stand-ins** (HUSK, RIPPER, BRUTE).
5. **The test ground's gaps are 25–40 s** so sweeps come quickly. Real
   levels use `sweeptier` gaps (90–150 / 70–120 / 50–90 s).
6. **The Gather only throws you.** The ride (design phase 4) is not built.
7. **No gamepad button** for HOLDFAST; every button is taken. R3 is finish
   / interact, L3 is RECALL.
8. **Low gravity is the player's only.** `Splinter::GRAVITY` and enemies
   are unscaled.
9. **The swept death is the normal death.** Section 13.6's tumble off the
   ring is not built; the body is not thrown once dead.
10. **No vacuum sound, no music of its own, no count line** beyond a feed
    message (`COUNT: ONE EXTRA`).
11. **Not in the menu and not saved.** `StorySave.h` rejects chapter > 5;
    `MenuState.h` builds chapters 1–5 in two places.
12. **`sweepreach` is straight-line.** Fine on open ice; a level with gaps
    or walls needs a walked check like `tests/test_cold.cpp`'s `Walk`.
13. **No runtime test.** The design asks for a check in
    `tests/test_dev.cpp`; only the logic tests exist.

## Next steps, in order

1. **Playtest gate (the user).** Ten minutes on `--story6`. The questions:
   is sheltering fun, are 20 / 10 / 6 s right, is the front readable at a
   distance, does HOLDFAST feel useful. Tune `sweeptier` and `Holdfast`
   constants before any level is built. Record the verdict here.
2. **Finish the test ground's rough edges** that affect that verdict:
   breakable small shelters (shortcut 1), a HOLDFAST bar on the HUD beside
   RECALL's (`UIRenderer.h` near the RECALL bar), the swept death.
3. **The ring's own sky** (design step 4, section 10): a `uRing` branch in
   `src/shaders/skybox.frag` beside `uCold`; the giant filling half the
   sky, ring shadow stripes, no aurora, and the Shepherd as an impostor
   whose size is a uniform set per level.
4. **STRAYS, HERDERS, TALLIES** (section 7) in `src/enemies/`, following
   how Chapter 5 added THAWLING / PICKER / CLERK (`EnemyCold.h`,
   `Gameplay_ColdEnemies.h`, `EnemyTypes.h`). Herders move only shelters
   with `fixed = false`.
5. **The vertical slice:** the Landing, the Drift and the Kept as sections
   of `buildRing()`, the Crook's crossing encounter, HOLDFAST handed over
   in the Drift (`holdfastOwned()` then checks a flag, as `coldRecall`
   does). Drifting chunks are `Mover`s; the rope already rides them.
6. Then design steps 7–12 as written.

## House rules noticed while building

- The game is one translation unit (`src/main.cpp` includes everything);
  a full build takes about 40 s. New headers need no Makefile entry.
- Rules live in a header with no OpenGL and no `GameplayState`
  (`Recall.h`, `RingSweep.h`), with a `Gameplay_*.h` beside it that touches
  the world. Tests include the rules header directly.
- On-screen text is terse capitals (`docs/text-pass.md`,
  `src/core/StoryText.h`). Tells: white = parry, gold = dodge, red = get
  out. Cyan is UNIT 24's alone.
- Mandatory tells are never hidden by detail settings.
- Music: bosses loud, full tempo, no half-time sections
  (`docs/boss-music-pass.md`, `tools/music_levels.py`).
- Nothing here is committed. The branch is `ship`.
