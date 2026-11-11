# Fable plan and notes

Working notes for the three workstreams in `docs/fable-build-prompt.md`. Updated as each one moves.

## Workstream 1: pathfinding (branch `pathfinding`): built, waiting for a playtest

**Found.** Enemies cannot step up or jump. They walk level ground, are lifted by the hard floor (ramps,
basins, water), step down up to 1.4 m, and the ones that don't hold a perch (Rippers, Mites) drop off
ledges. Every walker moves through `Enemy::setMove`, so that is the one place routes hook in. Chapter 5
already had a hand-made route for one room (`coldcombat::chillerRoute`), which is the same need.

**Approach: a layered grid and shared flow fields** (your guess, and it held up).
- `src/world/NavGrid.h`: the arena's bounds cut into 0.8 m columns; a node per surface a body fits on;
  links only where an enemy can really go. One Dijkstra field per kind of walker (wide / tall / drops)
  from the player's node; every chaser of that kind reads it. Three priced variants split a squad over
  routes that are nearly as short. A field follows the player at most five times a second, one a tick.
- Rejected: per-enemy A* (cost grows with the wave), a navmesh (the maps are boxes; a grid reads them
  directly and rebuilds in milliseconds when cover breaks), a hierarchy (not needed at these sizes).
- "Rebuilt incrementally" turned out unnecessary: a whole field costs 2.7 ms at worst (25,000 nodes),
  so it is simply rebuilt when the player changes node.
- `src/world/NavBuild.h`: per-arena build; rebuilt when a hash of the arena's static boxes or water
  changes (doors, broken cover, the Motor Assessment load, the flood).
- `Enemy::navDir` / `setMove`: direct chase when the straight line is walkable; the route otherwise;
  feelers on top with a shorter look-ahead; a guard so a zigzag doesn't carry a walker off a ledge or
  up a ramp's side to somewhere farther from you. Gunners with no shot walk the route until one opens.

**Deliberately left out (say if you want them):**
- Bosses never use routes (their movement is scripted and their arenas are open). Flyers never do.
- No grid on the Descent and the Reliquary (the ground is moving platforms) or Chapter 4's plain (too
  wide to cut finely). Feeler steering alone there, as before.
- Enemies still can't reach you on a ledge with no ramp: that needs a jump or climb, which is a new
  enemy ability and a balance change, so I did not add it.
- An enemy lured by RECALL's statue steers by feelers (the routes lead to you).

**Proof.**
- `make test-nav` (now in `make test`): 2,243 chases across 41 arenas and sections. Routes reach the
  player in 2,233 (99.55%); feeler steering alone in 2,067 (92.2%) of the same chases. The test's floor
  is 99% and it prints each miss. No walker in a simulated wave stands wedged.
- The ten misses are spots where a 0.8 m column and the real boxes disagree (Scrap Pit, Motor
  Assessment, Hadal Research, Final Assessment, Blast Chillers, Dispatch 2). Not fixed.
- Cost: largest grid 87 ms to cut (Dispatch 2, 30,000 nodes; most are 1 to 15 ms), a field 2.7 ms worst,
  reading a route under 1 microsecond.
- `--bench 2000 --arena 4 --wave 2 --god`: enemies 0.01 ms a frame before, 0.02 ms after; frame 12.55 ms
  before, 13.14 ms after against 12.70 ms with `--nonav` in the same session (GPU-bound, within noise).
  One 4 ms frame when the grid is cut at the start of the arena. Web build not measured yet.
- Difficulty (headless, a player who stands still, nobody shooting back): melee enemy-seconds within
  6 m up 1%, gunner-seconds with a clear shot up 8%. Only 16 arenas have shared ground spawns, so this
  is a sample. Real difficulty needs your playtest.
- `--nav` draws the grid and each routed enemy's path; `--nonav` turns routes off to compare.
  Stills in `docs/nav-review/`.

## Workstream 2: boss animation and sound (branch `boss-animation`, from `main`): built, waiting for a playtest

**Found** (by drawing film strips of the real rigs on the CPU, `make boss-strips`, and measuring):
- The Absolute's planted feet slid 106 to 137% of the ground he covered: the legs were posed from the
  hips, so the feet went wherever the pelvis did. That is the gliding.
- In a fight he is either standing or bursting across the floor (2,310 of 2,674 ground ticks still, the
  rest faster than a run). He almost never walks, so what you look at is his stance and his blows.
- On a cross he stood up out of his stance, and nothing shifted his weight on the wind-up.
- The Sovereign had no springs at all: his pose was computed fresh each frame from his attack state,
  so a sweep's follow-through began 2.2 rad from its wind-up in one frame. That is the screenshot look.
  At rest he was frozen apart from his walk cycle.
- Every joint of the Absolute and the Successor chased its target on the same critically damped
  spring, so nothing trailed or settled, and MORNINGSTAR was the same motion scaled up.

**Movement character** (my proposals; say if you want any changed):
- The Absolute: a precise, fast duellist. Stiff core, arms and head a little loose, light quick feet.
- MORNINGSTAR: the same body with far more mass. Every spring slower when he isn't striking and looser,
  so his trunk runs past a stop and takes a moment to settle; his guard lives at 0.58 speed; long slow
  steps that land like dropped stone (sound, dust, a small shake); a hit moves him 40% as far.
- The Successor: a surgeon. Upright, almost still in the trunk, alive in the hands and head; small
  on purpose so his stillness is the threat and a cast's locked aim stays readable.
- The Sovereign: a knight in plate over a greatsword. Heavy trunk, a sword arm slower and looser so
  the blade runs past and comes back; he breathes under it, the point drifts, now and then he shifts his grip.

**Changed.**
- `AbsoluteRig.h`: planted feet (`absFeetTick`: a foot stays where it was put in the world and steps
  when the body has moved off it; legs solved by two-bone IK, `absLegReach`), a pelvis that shifts over
  the feet, weight back on the wind-up and through the blow (`drive`), a spring of its own kind per
  joint (`absSpring`, exact at any tick length), hit recoil (`absoluteHit`). At a run the run cycle
  takes the legs back; a burst or blink carries the feet with him.
- `SuccessorRig.h`: the looser springs, a living guard, hit recoil (`successorHit`).
- `EnemyModel.h` / `SovereignState.h`: a sprung body for the Sovereign (`tickSovereignBody`), idle
  life, a break he drops into instead of cutting to, hit recoil (`sovereignHit`).
- Sound, all from existing files (no download growth): the Absolute's and MORNINGSTAR's footfalls on
  each real step, the Sovereign's armoured steps and the whoosh of each stroke (low, long), the
  Successor's steps and cutting strokes (thin, high).
- `tools/boss_strip.cpp` (`make boss-strips`): the film strips. Before and after in `docs/anim-review/`.

**Not changed.** No attack timing, telegraph length, hitbox or damage. The head hitbox follows the
drawn head as before, so it moves a few centimetres with the new weight shift.

**Proof.**
- `make test-boss-animation`: 3.4 million checks at 30/60/120/240 Hz. Planted feet slide under 0.5% of
  the ground covered (measured: millimetres over 10 to 50 m, both sizes); joints finite and bounded
  through every move; no joint snaps; the same pose at 30 and 240 Hz; a hit rocks each boss back and
  he comes home; a hit never bends a tell; MORNINGSTAR settles slower than the Absolute.
- Telegraphs: the sequence of (attack, wind-up length) each boss's real mind starts over two minutes
  is hashed and matches `main` exactly for the Absolute, the Successor and the Sovereign.
  MORNINGSTAR starts no tells in that bare setup, so he is covered by BossSim only.
- `tests/test_zenith` (BossSim): output identical to `main`, kill times included. `make test` green.

**Not done / to decide.**
- I can't hear. The new sounds are placed and level-checked (a 12 s Sovereign fight peaks at -4.7 dBFS)
  but whether the pitches suit each boss needs your ears.
- The Successor and the Sovereign did not get planted feet (their legs are still posed from the hips).
- The Sovereign's legs stay nearly straight through his blows; his crouch is small. Deeper leg work
  would mean reworking the shared humanoid walk, which I left alone.
- No in-game `--record` footage for this branch: the bot keeps the bosses too far away to judge
  motion, so the strips are the review material. BOSS mode launches stop on the Armory screen.
- Step cadence varies a little with frame rate (51 to 68 steps over the same walk). Cosmetic.

## Workstream 3: polish, feel and sound across the board (branch `polish-feel`): next
