# Act II foundation + The Drowned Nave — design

Date: 2026-10-06 · Status: awaiting review · Piece 1 of 6 of Act II

## Context

Act II, "BENEATH THE ECLIPSE", is a second ARENA campaign: four bigger arenas
with harder enemies and a new final boss, THE LEVIATHAN. The Sovereign's death
cracks the Sanctum and the player falls into the machine beneath it.

Decisions already made with the user:

- Final boss: THE LEVIATHAN (piece 6).
- Act II unlocks after beating Act I.
- Four full arenas: Drowned Nave, Orrery, Descent (+ mini-boss THE PENITENT), Reliquary.
- Act II is its own run (own mode, timer, leaderboard). Later an ASCENT mode
  chains all ten arenas in one run.
- A standalone Act II run starts with a head start: level 6 (5 upgrade points).
- Nave water is wading, beaten by movement: no swimming, no damage.
- Act II is unranked until the whole act exists.

Act II is built as six pieces, each with its own spec, plan and build:

1. **Act II foundation + The Drowned Nave** (this spec)
2. Hollowed variants + Seraph + Anchor
3. The Orrery
4. The Descent + The Penitent
5. The Reliquary + Revenant + Weaver
6. The Leviathan (then: ranked leaderboard, ASCENT)

## Goals for piece 1

- A playable ACT II run from the menu, unlocked by killing the Sovereign, that
  opens with the fall and plays one arena, the Drowned Nave, end to end.
- The level code is structured so the later arenas and ASCENT slot in without
  rework.
- The water mechanic and the FLOOD arena shift, reusable by later arenas.
- No change to how Act I, FAST, ENDLESS or DAILY play, score or rank.

## 1. Structure and flow

### World placement

Act I covers Z +31..−405 with floors at Y 0. Act II lives in its own region,
**60 m below and north of it**: floors at Y −60, starting around Z −440, so
the two never overlap and ASCENT can build both into one world.

- **ASCENT (later):** when the Sovereign dies a hole opens in the Sanctum's
  centre and the player physically falls 60 m into the Nave's entry shaft.
- **Standalone ACT II:** the player spawns in mid-air at the top of that shaft.
  Either way the run opens with the fall, the eclipse light shrinking overhead.

### Code shape

- `buildLevel()` (Level.h) becomes `buildAct1(LevelBuilder&)` with no behaviour
  change. A thin `buildLevel()` stays for callers/tests and calls it.
- New `src/LevelAct2.h` with `buildAct2(LevelBuilder&)`; added to the Makefile
  HEADERS list.
- New `GameMode::ACT2`. `GameplayState` builds the level its mode needs
  (ARENA → act 1, ACT2 → act 2, FAST → gauntlet; ENDLESS/DAILY as today).
- Audit and fix code that assumes floors are at Y 0: pickup drops, spawn
  snapping (`groundHeightAt` callers), the "ground outside the arenas" plane,
  void/fall checks, camera/dev helpers. Act II gets its own outer ground plane
  well below its floors.

### Unlock

- New `act2Unlocked` field in `Records` (saved in `records.v2.cfg`; old files
  load with it false).
- Set when the Sovereign is killed in any run without god mode (practice and
  dev starts count).
- Main menu gets an **ACT II** row, always visible: greyed with
  "DEFEAT THE SOVEREIGN" while locked. The dev level select (`` ` ``/F2,
  `--dev`, `?dev`) ignores the lock and lists Act II arenas.
- Command line: `--act2` starts an ACT II run (like `--fast`), ignoring the lock.

### Run start

- `Progression` starts at level 6 with 5 unspent points.
- The armory opens automatically during the fall; the countdown starts on
  landing. If the player closes the armory early, the fall just continues.

### Leaderboard

- While Act II is incomplete, ACT II runs are **unranked**: no records, no
  leaderboard, no name entry. The menu row reads "ACT II — PREVIEW · 1/4 ARENAS".
- The ACT II board (local + the site's shared board, which needs an `act2`
  key in `api/overdrive-board.js` in the website repo) arrives with piece 6.

## 2. The Drowned Nave — layout

A flooded cathedral, about **100 m × 150 m** (the Sanctum is 112 × 112), floors at Y −60.

```
                N (−Z)
        ┌───────────────────┐
        │   APSE + DROWNED  │  raised chancel (+3 m)
        │  ORGAN (landmark) │
   ┌────┴───┐  CROSSING ┌───┴────┐
   │ WEST   │  ( dome   │  EAST  │  transepts: broken rose windows
   │TRANSEPT│  cracked, │TRANSEPT│  glowing red from beneath
   └────┬───┘  light    └───┬────┘
        │  ║  shaft )  ║    │
        │  ║           ║    │  aisles (+1.5 m) behind column arcades,
        │  ║   NAVE    ║    │  galleries above (+6 m),
        │  ║  (lowest) ║    │  clerestory ledges (+12 m)
        │  ║═══════════║    │  fallen flying buttresses: bridges at +9 m
        │  ║           ║    │
        └──┴─── ↓ ─────┴────┘
           entry shaft (the fall lands here)
                S (+Z)
```

### Tiers (heights above the nave floor)

| Tier | Height |
|---|---|
| Nave floor | 0 |
| Aisles | +1.5 m |
| Chancel | +3 m |
| Galleries | +6 m |
| Buttress bridges | +9 m |
| Clerestory ledges | +12 m |

Ceiling (zone.max.y) above the clerestory, under the dome's crack. Routes up:
fallen columns as ramps, rubble slopes, jump pads in side chapels, grapple
anchors on column capitals, and the organ pipes.

### Set pieces (per the "organic, not boxes" rule)

- **The crossing:** a half-collapsed dome (`dome`, `curve`) with a ring walkway
  around the break. A shaft of eclipse light falls through the crack, visible
  from everywhere. The entry shaft connects up through it.
- **The drowned organ** in the apse: tall pipes of differing heights
  (`column`/`shaft`), half under water; a climbable vertical route to the
  clerestory.
- **Pointed arch arcades** (`arch`) along both sides; several have collapsed
  into the water, leaving gaps in the galleries to dash across.
- **Light from beneath:** rose windows and the water glow red from below.
  Act II palette: cold teal, bone white, deep red from below.

Collision stays boxy (hidden boxes via `LevelBuilder::solid()`/`hide`), visuals don't.

### Waves

| Wave | Goal | Mix (Act I enemies until piece 2) |
|---|---|---|
| 1 | Kill all | Husks, Rippers, Sentinels, Shieldbearers |
| 2 | Destroy the conduits (on the galleries) | Raptors, Brutes, Conductors, Mites |
| 3 | Kill all | Brutes, Shieldbearers, Conductors, one Juggernaut |

`maxAlive = 11`, `damageScale = 1.3`. Exact counts are tuned during the build
and documented in the plan. Piece 2 adds the new enemies to these waves.

### End of run

The Nave's exit door opens on a sealed passage north. Walking into it ends the
run with a **TO BE CONTINUED** variant of the victory screen (time, style,
score shown; no name entry).

### Music

The existing synth with a new key/tempo preset for Act II; no new track.

## 3. Water and the flood

### Data

- `struct WaterVolume { AABB box; float level; }` in Level.h;
  `std::vector<WaterVolume> water` in `LevelData`.
- `LevelData::waterDepthAt(glm::vec3 p)`: depth of water above `p.y` at `p`
  (0 when dry or outside every volume).

### Movement (Player.h)

- **Walking/running:** max ground speed scaled by depth at the feet,
  linear from −20% at 0.4 m to −45% at 1.0 m and deeper; no slow below 0.1 m.
- **Sliding and dashing skim** at full speed; a slide that starts in water
  lasts 30% longer.
- **Air is unaffected:** jumps, grapple, pads behave normally.
- **Depth cap:** where the water is deeper than 1.5 m, its surface holds you
  up: the effective ground is `max(floor, surface − 1.5 m)`, an invisible
  floor 1.5 m under the surface (in `groundHeightAt` / Player ground checks).
  You wade chest-deep at the maximum slow but never sink, so the camera is
  never submerged (no swimming, no underwater rendering). Bullets, grapples
  and dropped pickups still see the real floor; pickups that would sink
  deeper float at the cap.
- **Enemies on foot** are slowed by half the player's factor. Flyers are not.

### ArenaShift::FLOOD

Modelled on `LAVA_RISE` in ArenaShifts.h; edits `LevelData::water` levels.

| Wave | Water above nave floor | Effect |
|---|---|---|
| 1 | 0.4 m | nave floor ankle-deep |
| 2 | 2.0 m | nave chest-deep (held at the 1.5 m cap, maximum slow), aisles ankle-deep |
| 3 | 4.0 m | nave and aisles chest-deep at the cap; only chancel, galleries, bridges, clerestory dry |

- The water rises between waves over ~6 s: low rumble, banner "THE WATER
  RISES", red underglow brightens.
- An enemy whose spawn point is under more than 1.2 m of water spawns at the
  nearest dry spawn instead.

### Rendering

- A new translucent water pass after opaque geometry: one quad per volume,
  new small shader (`water.vert/.frag`, GLSL ES–compatible), scrolling
  ripples, red lit from below, teal-dark on top, brighter under the light shaft.
- Particles (existing system): spray trail while skimming, splash on landing,
  splash + ripple ring when an enemy falls in.
- Shader warmed in `GameplayState::warmPipelines` to avoid a first-use hitch.

### Sound

New `wade` and `skim` via `tools/gen_sfx.py wade skim` (only these names, so
recorded sounds aren't overwritten). Wade replaces footsteps in water; skim
loops while sliding over water.

## 4. Testing and verification

### Automated (`make test`, tests/test_game.cpp)

- **Act I unchanged:** `buildAct1` produces the same counts of walls, props,
  shapes, arenas, doors, movers, pads as `buildLevel` did before the split
  (counts captured before refactoring).
- **Nave map checks**, same as existing arenas: spawns clear of walls,
  ceilings above spawns, pads land on their platforms, doors fill their gaps,
  scenery not in play space.
- **Spawns per flood level:** every wave's effective spawn stands in ≤ 1.5 m of
  water after the dry-spawn rule.
- **Depth cap:** at every flood level, a player dropped into the deepest
  water settles with feet 1.5 m below the surface and eye height above it.
- **Reachability:** a simulated player gets from the nave floor to the
  galleries and from the galleries to the bridges.
- **Water movement:** wading is slower than dry walking; a slide over water
  reaches full slide speed; walking at the deepest flood stays above a minimum.
- **The fall:** from the ACT II start the player lands in the entry shaft within
  a few seconds.
- **Unlock:** `act2Unlocked` round-trips through save/load; only a non-god
  Sovereign kill sets it.
- **Full run:** the wave director drives a simulated ACT II run to its end.

### Manual

- `make && make test`, then `source ~/emsdk/emsdk_env.sh && make web`.
- `--shot` screenshots: the fall, the crossing light shaft, the organ, each of
  the three flood levels.
- `--bench` on the Nave at 1080 lines vs the Sanctum: within ~15%.

## Out of scope for piece 1

New enemies and Hollowed variants; the Orrery, Descent, Reliquary; the
Leviathan; ASCENT; the ranked ACT II leaderboard and site `act2` key; a new
music track; swimming or underwater rendering.

## Risks

- **Y 0 assumptions** in spawn, pickup and void code. Mitigation: audit early,
  covered by the spawn and fall tests.
- **Arena size vs pacing:** a 150 m arena can feel empty. Mitigation: spawn
  points spread by tier, `maxAlive` 11, mobility routes; tune in playtest.
- **Water render cost on web:** one blended quad per volume; check with `--bench`.
