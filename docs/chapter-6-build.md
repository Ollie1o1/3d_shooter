# Chapter 6: THE KEPT RING — build log and hand-off

The design is `docs/chapter-6-design.md`. This file is where the build stands
and what to do next. Started 2026-11-12 (Fable 5.2); written so another model
or person can carry on without the first session's context.

## Where it stands

**2026-11-12, second session (Opus 5.5): the vertical slice exists.** The
Landing, the Drift and the Kept are built as one world (design steps 4 to 6,
less the Crook), with the ring's own sky and the chapter's four crossing
enemies. The test ground lives on behind `--ringtest`. Committed on `ship`
(`0f7a07b` and after). **Nobody has played it by hand yet**: the playtest gate
(step 3) and the slice playtest (step 6) are both still the user's.

**2026-11-12, third session (Opus 5.5): step 3's rough edges**, on branch
`ring-edges` (worktree `../3d_shooter-edges`), while the user played the slice.
Shattered loose shelters really go; the HOLDFAST bar; the swept death (13.6);
the count line (13.2); vacuum sound outside, air in the hull; the light bake.
See "Rough edges done" below. Not hand-played either.

**Try it:**

| Command | What you get |
|---|---|
| `./shooter --story6` | The chapter from the Landing: capsule field, the ridge (low gravity), the first sweep once you're over it, three Strays at the lip, the Drift, HOLDFAST on I2, the fight island, the Kept's courtyard (3 waves), the open hull, the end card. |
| `./shooter --story6 --arena 2` / `3` / `4` | Start at the Drift / the far half of the Drift / the Kept. |
| `./shooter --ringtest [--wave 2]` | The old flat test ground, sweeps every 25-40 s cycling the tiers, now with the real enemies. |
| `./shooter --story6 --god --cam X Y Z YAW PITCH --shot 100 out.bmp` | A screenshot from anywhere (the eye is held there). Good views: `--cam -14 3 -170 -90 -8` (the lanes), `--cam 0 30 -590 -90 -22` (the reef), `--cam 0 1.7 100 0 5` (the giant). |
| `./shooter --viewer stray` (herder, tally, porter) | The new models in the enemy viewer. |
| `make test-ring` | Rules, plus: every post in its shelter's shadow, every shelter solid, shelter reach on the Landing (11 s) and the Kept (12 s) under the 20 s warning, every gap across the Drift (worst 6 m; a low-gravity running jump clears ~9.9 m), the grapple gap to the shore. |

## The vertical slice (LevelRing.h)

One world, north (-Z) toward the Shepherd; sweeps travel south, so every lee
and every post is on the south side. Five arenas over three places:

| Arena | Place | z | What's there |
|---|---|---|---|
| 0 | THE LANDING | 135 .. -150 | Your open capsule and its furrow; ~300 capsules (solid within reach, sparser and visual-only out to 400 m); worn paths; 16 rock shelters; the capsule ridge at z 26-30 (2.9 m: one jump at half gravity, impossible at full); a wave of 3 STRAYS at the lip. First sweep: scripted 2 s after you cross z 20. |
| 1 | THE DRIFT | -150 .. -345 | A basin to -900 across the whole ring (nothing under you). The pier, 11 lanes of drifting chunks (PINGPONG movers along X, 3 per lane 34 m apart, 13-30 s periods, tops -0.6..1.8), every third lane's chunks carrying a boulder that is a moving shelter. Island I2 (top y 1) with the HOLDFAST capsule: walk within 4.5 m and it is yours; the Drift's first sweep comes 18 s later. |
| 2 | (the Drift) | -345 .. -560 | 5 quicker lanes, the fight island I3 (STRAYS + a HERDER, then STRAYS; two loose shelters), 5 lanes, a 7.8 m gap to the far shore (grapple the capsule stack, or jump it). |
| 3 | THE KEPT | -560 .. -760 | The reef plain (rock shelters), then the reef: capsule-course walls round a 72 x 84 m courtyard, 3 m ledges inside east and west, six capsule stacks (shelters), four loose shelters. Waves: 5 STRAYS; 4 STRAYS + 2 HERDERS + a TALLY on the east ledge; a PORTER + 4 STRAYS + a HERDER. First sweep 14 s after the card. |
| 4 | (the Kept) | -760 .. -905 | The open hull (`ring::quietHull`): no sweeps inside, emergency strips, berths, the sibling's lamp blinking every 1.12 s, two terminals. North of it, `story.end`: the completion card. |

Places get one card each (`ringLevelSeen`): the Drift's and the Kept's second
arenas are checkpoints, not new places. Each card's subtitle is the distance
(74,000 / 61,000 / 52,000 KM).

**Shelters are level data now** (`LevelData::sweepShelters`): `ring::shelters()`
(fixed: rocks, the ridge, the islands' blocks, the reef) plus, from
`buildRing()`, the drifting boulders (`wall` set: their box follows the wall
every tick, posts ride along) and the loose ones (`mover` set: DRIVEN movers a
HERDER pushes along `b`). `armSweep()` copies them and resets every DRIVEN
mover on a retry.

**Drifting chunks** are movers with `look = 1`: the generic glowing-slab draw
skips them and `gatherRing()` draws a slab with lumps (`meshlib::lumpBall`)
under and at the ends.

## The band, and collision that matches what is drawn (2026-11-12, after the user's note)

The user: the ring shouldn't be a plain to every horizon (east and west there should be space,
it should feel like standing on space rock); and rocks could be walked into, you could get stuck
inside hollow meshes or fall into the void.

- **The band** (`ring::EDGE`, `EDGE_RAG`, `edgeAt`): ground runs out to a ragged edge 116-128 m
  either side (the zones still stop you at 110: you see the drop, you don't fall off it). Past it:
  basins down to `VOID_FLOOR`, a low broken lip, cliff faces under it, and 1,600 rocks (and the odd
  capsule) hanging in space, turned any way, sparser and more scattered further out. North and
  south it still runs to the horizon. Costs about 4 fps (70 -> 66 at the reef view, `--bench 300`).
- **Collision from the mesh** (`ringSamples`, `ringSolids`, `ringMatchCollision` in `LevelRing.h`):
  a rock's or capsule's collision is its own surface, cut into up to 3x2 (rocks 4x3) boxes that meet
  without seams, so a leaning capsule is a short staircase, not one box with empty corners. Before,
  every body-height rock shape (113) and about 600 capsules could be walked into: the upright
  capsule model leans 58 degrees and is 4 m deep against a 2 m box, the lips over the Drift had
  no collision at all, and shelter rocks were drawn up to 10% wider than their boxes.
  `ringMatchCollision` runs last in `buildRing()` and gives anything still hollow within reach
  its own collision. `make test-ring` checks none is left. The shelter boxes still decide the
  sweep's shadow; the test now checks each shelter is solid through its core.
- Rebake after any change to these (`make tools/light_bake && ./tools/light_bake story6`: rebuild
  the tool first, or the bake is stale and the game draws without it).

## THE SHADOW BAND (level 4, arena 5; 2026-11-12)

North of the open hull, z -905 to -1240 (`ring::BAND_S`, `BAND_N`), a short level with one idea:
the giant's shadow crosses the ring and it is black. Rules and look: `src/game/Gameplay_Shadow.h`;
numbers: `shadowband::` and `ring::darkTheme`, `bandRocks`, `bandLamps` in `LevelRing.h`.

- **The dark** (`ringDark`): full SHADE_IN (30 m) in. The world's light lerps to the giant's
  night side (`ringShade`), the sky shows its dark face (`uRingNight`), music is off (section 9:
  true silence). The sweep's wall is drawn at 12%: you go by the count, the floor's tick and
  the posts, which stay lit.
- **Light:** your lamp (point light 3, 4.5 m ahead and low), and 13 capsules along a winding
  route with their amber lamps lit (neon: they light the ice, and the bake carries it).
  60 unlit capsules for silhouettes.
- **Lightning** (`tickShadow`, `shadowband::bolt`): every 4-11 s, three strokes over half a second
  in the giant's night side (`uRingBolt`, `uRingBoltAt`; the shader no longer picks its own),
  and for that moment the ring is lit grey. Reduced flashing: 35%. `--overlay bolt`: one every
  second or so, for screenshots.
- **Sweeps:** the mid tier (10 s, two waves) from here on; the first is scripted 22 s after the
  card. 41 rocks in nine staggered rows: the worst walk to shelter is 5.2 s (`make test-ring`).
- **One fight:** 4 STRAYS and a HERDER, triggered between z -1060 and -1110.
- **The sunrise** (past `SUNRISE_AT`, 95 m from the far side): a red arc along the giant's edge
  (`uRingDawn`), the diamond (a white flash, 1.6 s in), then sunlit ice racing toward you at a
  sweep's speed (`gatherShadow`) from 320 m north; when it reaches you the dark lifts over 1.2 s.
- The chapter's end card is now past the band (`story.end`, z -1266 to -1248).
- Not built: the sound (vacuum already; a floor hum for the front in the dark would help),
  the Shepherd's lamps turning to follow you (13.2), dust on the hands (13.5).

## THE SPOKES (level 5, arena 6; 2026-11-12)

z -1272 to -1760 (`ring::SPOKES_S`, `SPOKES_N`). Rules and look: `src/game/Gameplay_Spokes.h`; the
shapes: `ring::spokes()`, `spokeAt`, `spokeRocks` in `LevelRing.h`.

- **Five spokes** of charged dust lie across the band, slanted, each 52-66 m wide at one end and
  narrowing to an 11-13 m crossing somewhere along it (`Spoke::narrowX`, kept clear of rock and
  capsules). Drawn as a dark stain on the ice, edged by faint violet stakes and lines, with
  sparks hanging in the dust near you.
- **Inside one** (feet within it, up to 4 m above the ice): no dash, no grapple. It is the ANCHOR's
  field through the same check (`anchoredAt`) and refusal (`pinnedCue`, feed `CHARGED DUST - NO
  DASH OR GRAPPLE`); a grapple already flying goes dead as you enter. HOLDFAST still works (a
  spike, not a hook). A crackle of static while you're in it. Its tell is pale violet, not the
  ANCHOR's red: red is the countdown's alone in this chapter.
- **Sweeps:** the mid tier; the first scripted 18 s after the card. 57 rocks in 13 staggered rows,
  between and inside the spokes: the worst walk to shelter is 5.7 s.
- **Fights:** triggered between z -1440 and -1470: 5 STRAYS and 2 HERDERS, then a PORTER, 3 STRAYS
  and a TALLY.
- The chapter's end card is now past the Spokes. The Shadow Band's dark ends at its far side
  whether or not the sunrise played (a retry in the Spokes starts in daylight).
- Not built: static crawling over UNIT 24's fingers (13.5); a HUD sign that you're in a spoke
  beyond the toast and the feed.

## The sky (skybox.frag `ringSky`, `uRing`)

The giant (`RING_GIANT`, east, angular radius 0.8) sits on the horizon with its
equator in the ring plane: butter, tan and cream belts, a storm, the rings'
thin shadow stripes, a warm limb. The ring is a hair of light on the horizon,
broken where the Shepherd's gap is. The Shepherd (`uShepherd`: direction and
angular radius, `ring::shepherdSize(z)`, 0.0012 at the Landing growing to
~0.009 at the Kept's end) is a point that becomes a walnut with a ridge; its
ridge lamps light with `uRingFlash` on every sweep. `uRingNight` (the giant's
shadow, night-side lightning) is wired for the Shadow Band and is 0 for now.
Ring materials use `Mat::ROCK`: `Mat::ICE` renders as blue hex tiles, off the
chapter's palette.

## THE DISPATCHED (the chapter's enemies)

Like Chapter 3's four lab families, but the opposite look: hard shells,
bleached enamel, ring dust on every lower surface, rime in the seams,
shipping hardware worn as anatomy, one warm light each (a pale visor, an amber
capsule lamp). No cyan, no red. Models: `tools/blender/ring_foes_build.py`
(`blender -b --factory-startup --python tools/blender/headless.py -- tools/blender/ring_foes_build.py [ONLY=stray]`).
Minds: `src/enemies/EnemyRing.h`; poses: `src/render/RingFoePose.h`; their
part in the sweep: `src/game/Gameplay_RingFoes.h`.

| Enemy | Look | Fight | With the sweep |
|---|---|---|---|
| STRAY (52 hp) | Lean unit, the back half of its packing cradle still strapped on, foam blocks, a delivery tag, one pauldron torn off. Lopes in long low-gravity bounds. | The Living Ward patient's claw and lunge (`thinkLab`), white parry. | Runs for the nearest lit post; a hit during a countdown shoves you out of the lee (`SHOVED OUT`). |
| HERDER (95 hp, flies) | A squat block behind a scuffed plough plate with black chevrons, gas pods on swivel arms, a hooded lamp. | RING_RAM: backs off, gold tell, rams down a line (dodge). | Takes the loose shelter nearest you and pushes it along its rail (about 3 s). Kill it and it stops. |
| TALLY (170 hp, still) | A counting mast on three spiked legs: bead racks that slide, a counter drum that turns a notch per count, an amber lamp that blinks. | None. | While one stands the gap between sweeps halves (`SweepSchedule::tick` rate 2). |
| PORTER (320 hp) | A hauler bent under a dispatch capsule strapped to its back; loader forearms with shovel hands. | The BRUTE's slam (gold) and lob (`thinkBrute`): its lobs arc over cover. | (none yet: a target for luring into the open) |

Their voice keys, cadences and the RING_RAM tell are in `VoiceTable.h` /
`VoiceSynth.h` (the voice level test checks the tell).

## THE CROOK (the first sheepdog)

`EnemyType::CROOK` (`isSheepdog`), model `tools/blender/ring_foes_build.py`
(`ONLY=crook`; note `enemy_kit.py` hard-codes `REPO` to the main checkout, so
a build in a worktree writes `assets/enemies/crook.glb` there: move it).
Mind: `thinkCrook` / `crookLeaveTick` in `EnemyRing.h`; pose: `RingFoePose.h`;
in the world: `crookTick` and `ringFoeEvents` in `Gameplay_RingFoes.h`.

- **Where:** the Kept's first wave is the Crook, a STRAY placed at the east
  wall and two more. Its card (`THE CROOK / IT KEEPS THE FLOCK IN LINE`) and
  its bar, which empties where it breaks off, not where it would die.
- **Its opening:** 0.8 s in, it hooks the nearest STRAY and reels it in
  (`Enemy::yank`, the grapple's pull), the line drawn as a tracer.
- **One attack, RING_HOOK:** a 1.1 s throw; the aim follows you until the last
  0.35 s (`HOOK_LOCK`), then lands where you were. White for its last 0.3 s:
  parry it with the hook within 3.2 m of you (`HOOK TURNED`, a stagger).
  Caught (within 2.2 m): 9 damage and you are hauled to it for 0.7 s at
  16 m/s, out of any shelter (`HOOKED`). Spiked with HOLDFAST, or spiking
  mid-haul: `THE LINE HOLDS`. While a countdown runs it throws sooner.
- **It can't be killed here:** `takeDamage` stops at two thirds; it stops being
  targetable, crouches, bounds north over the reef (no walls, no arena
  clamp) and is gone after 3.2 s, not counted as a kill. The sweep doesn't
  throw it and HOLDFAST can't pin it.
- Not built: its own model in the code-rig check (DISPATCHED models have no
  code skeleton), the bent hook it should carry to the boss fight, a line
  cut (the design's answer to HOLDFAST, for the tines).

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

## Rough edges done (third session)

- **Shattered shelters go.** `Mover::gone`: a breaker parks a shattered loose
  shelter 5 km down (`Level::updateMovers`; its `delta` is zeroed so nothing
  riding it is dragged), `gatherRing` skips it, `armSweep` restores it. Only
  movers can be small: a fixed wall is baked into the world mesh and the
  collision grid, so any breakable shelter must be a DRIVEN mover.
- **HOLDFAST bar** (`HudState::holdfast`, above RECALL's): steady `HOLDFAST:
  HELD`, the pin's time draining as `HOLDFAST: PINNED`, else the cooldown
  (`Holdfast::cooldownFrom`, so a 0.3 s miss fills as visibly as a 5 s pin).
- **The swept death** (`updateSweptDeath`, `renderSweptDeath`): 2.2 s, run on
  the frame (the dead don't tick) as a function of time. The eye is thrown
  ~4 km down the sweep and up, wheeling back to face the ring, which ends as
  a strip; no gun; sfx ducked 60 dB and music at 0; `HOLD JUMP TO SKIP`
  (0.25 s). Then the death screen, `SWEPT. COUNT CORRECT` (the pixel font has
  no middle dot).
- **The count** (13.2): after each lethal sweep, on the countdown's plate for
  6 s: `COUNT: 40,112. ONE EXTRA.`, or `FOUR SHORT.` when DISPATCHED were
  killed or swept since the last one (`ring::countLine`, `ringGone`,
  `ringShort`, counted in `onEnemyKilled`). Replaces the feed's COUNT line.
- **Vacuum** (`SfxMixer::vacuum`, `AudioSystem::setVacuum`): outside the hull,
  ENEMY and WORLD sounds more than ~2.5 m away lose 55% gain and close to a
  520 Hz low-pass (felt, not heard). Tells, floored must-hear cues, unducked
  sounds and your own are untouched (`make test-audio` checks both). Inside
  `ring::quietHull` the vacuum lifts and the reverb goes METAL. Music is
  untouched (the "inside the helmet" pulse is still to write).
- **Light bake**: `story6` in `tools/light_bake.cpp` and
  `assets/levels/story6.light` (1.2 MB, ~1 min). The sections' zones now stop
  at 80 m, not 400 m: the bake fills each region's whole height, and at
  400 m it was 100M cells, over the loader's 64M cap ("a broken atlas").
  Rebake after changing `buildRing()`.

- **The swept, thrown** (13.6): an enemy the sweep kills leaves a tumbling
  silhouette (`sweptBodies`, drawn in `gatherSweep`) that flies down the
  sweep and up, ~250 m in 2 s, to a glint that keeps about 0.6 degrees. One
  STRAY in four (`uid % 4 == 1`) doesn't run for a post: it stops and turns to
  face the flash (`Enemy::ringFace`).
- **You hear what you touch** (13.1, `ringTouch`): standing on a capsule
  (`LevelData::knockWalls`: the Landing's reachable capsules and the reef's
  stacks), two slow knocks from inside every 3-6 s; standing pressed to a reef
  wall (`creakWalls`), `ab_creak`. Both close enough to pass the vacuum.
- **The zero**: a wall going over you in shelter ducks the mix 30 dB for
  0.4 s, then the ice patters (`skim`, unducked, delayed).

Still not done from step 3: a real air-hiss for entering the hull (it plays
`barrier`; there is no hiss sample, and new sfx need a MixTable level), and
music never ducks at the zero.

## Shortcuts taken, to be replaced

1. ~~Breakers don't remove the wall~~: done (see above), for movers.
2. ~~The sky is Chapter 5's~~: `ringSky` exists. The Shepherd is still only an
   impostor (no ridge geometry until the Shore).
3. ~~No light bake~~: baked (see above).
4. ~~Enemies are stand-ins~~: the four DISPATCHED exist. Still missing: their
   own death sounds and hurt barks beyond the synth defaults, and the Crook.
5. The test ground's gaps are 25-40 s; the chapter uses the far tier's 90-150 s,
   each place's first sweep scripted.
6. **The Gather only throws you.** The ride (design phase 4) is not built.
7. **No gamepad button** for HOLDFAST; every button is taken. R3 is finish
   / interact, L3 is RECALL.
8. **Low gravity is the player's only.** `Splinter::GRAVITY` and enemies
   are unscaled.
9. ~~The swept death is the normal death~~: built (see above). Enemies are
   still just killed, not thrown into the distance.
10. **No music of its own.** (Vacuum sound and the count line: done.)
11. ~~Not in the menu and not saved~~: done (step 4). `6  THE KEPT RING`
    is on the STORY page (practice and remix moved down to rows 7 and 8; with
    a Continue slot the nine rows use a smaller title), its five checkpoints
    are on the DEV page, `StorySave` takes chapter 6 (checkpoints 0..4), and
    the Cold Store's end card continues to chapter 6 (`nextChapter = 6`; a
    completed chapter-5 save resumes into the ring). HOLDFAST is not saved:
    a continue from checkpoint 1 means picking it up on I2 again; from 2 on,
    `beginRing` hands it over.
12. **`sweepreach` is straight-line.** Fine on open ice; a level with gaps
    or walls needs a walked check like `tests/test_cold.cpp`'s `Walk`.
13. **No runtime test.** The design asks for a check in
    `tests/test_dev.cpp`; only the logic tests exist.

## Next steps, in order

1. **Play it (the user).** `./shooter --story6` start to finish. Questions: do
   the lanes feel like hopping between asteroids or like a platformer; is the
   20 s far-tier sweep fair on moving chunks; do the Strays' shelter-grab and
   the Herders' shoves read; is the reef fight too long at three waves. Record
   the verdict here and tune `ring::lanes()`, `sweeptier`, the waves.
2. ~~The Crook's crossing encounter~~: built (see "THE CROOK" below). Needs
   the playtest: is the hook fair (1.1 s throw, aim fixed for the last
   0.35 s, catches within 2.2 m), is 320 damage to chase it off right.
3. **Rough edges:** done on `ring-edges` (see "Rough edges done"); merge it
   into `ship` after the user's playtest verdict is in.
4. ~~Menu and save~~: done. Still to do: the bridge cutscene (design
   section 2: the capsule's flight from the Store to the ring and the
   impact); for now the Cold Store's end card just loads the Landing.
5. **Levels 6-8**: the Shadow Band and the Spokes are built (above). Next, the
   Gap (THE DROVER and a raft ride over the gap), the Wake (THE BELLWETHER, the
   double sweep on a climb) and the Shore, then the boss as written.

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
- Committed on `ship`. Pyright flags the Blender scripts' exec'd names (`_og`,
  `NEEDED`, `bpy`): harmless, they run under `headless.py`.
