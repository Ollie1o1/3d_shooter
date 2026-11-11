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

## Workstream 3: polish, feel and sound across the board (branch `polish-feel`, from `main`): built, waiting for a playtest

**Found already done** (so not redone): the music already reacts (the last-enemy reduction, the wave-clear
stinger, intensity following the fight, the OVERDRIVE layer: `MusicSynth.h`, `CombatScore.h`); wall shots
already answer by material and bodies by mass and armour (`ImpactAudio.h`, `Gameplay_Feel.h`); every room
has a bed of tone (`RoomTone.h`); the story chapters have their own weather and dressing. What was thin:
nothing in a room came from a place, nothing stayed after a fight, and the room ignored explosions.

**Picked and built.**
- **Places heard from where they are** (item 5). `audio/WorldEmitters.h`, `world/WorldSound.h`: lava, the
  Core's reactor, fans and pools of water each sound from their own spot, read off the level data (no
  authoring; 4 kinds, 3 variants each, built in code at load in 18 ms, nothing added to the download).
  The nearest three within 30 m, three grains a second at most, under the enemies' chatter. Quieter with
  AMBIENT DETAIL, silent at OFF.
- **What you hear, you see** (item 1, the cheap part). When a lava emitter sounds it throws a few embers
  from that spot; water drips where it laps.
- **Marks that stay** (item 2). `render/Marks.h`, `shaders/mark.*`: a round leaves a pit on the wall or
  floor it stopped on (not on doors, movers or breakables), a blast leaves soot on the floor under it,
  and spent brass lies where it landed (red plastic for the shotgun). Pits 45 s, soot 90 s, brass 30 s,
  all fading out; 192 marks and 64 casings at most at HIGH (64/16 at LOW), the oldest making room. One
  extra draw call. This replaces the old hit decal (a hard red square, 10 s): it is now a soft-edged
  stain, 14 s, and repeated hits on one enemy renew one stain instead of stacking them.
- **The room answers a blast** (item 2). `world/Reactions.h`: for 0.85 s the neon tubes within reach
  stutter in lengths, and the light they cast with them; dust comes off a ceiling over the blast (only
  where there is one); lava within 13 m throws a gout and is heard. Every explosion in the game goes
  through `playBoom`, so all of them do this. With REDUCE FLASHING on the tubes dip once and do not stutter.
- **My critique** (item 7), from 18 s recordings of bot fights in Sunset Yard, the Foundry and arena 4
  (`docs/polish-review/fight-*.jpg`). Five things, most harmful first:
  1. The kill feed filled with copies of one line (six "MITE +11 XP" in a second). **Fixed:** the same
     line again counts on the line that is up ("MITE +11 XP x5").
  2. A room showed no trace of the fight a second after it. **Fixed** by the marks above.
  3. Explosions happened in front of the room, not in it. **Fixed** by the reactions above.
  4. Not fixed, yours to decide: at level-up "UPGRADE READY - PRESS TAB" is said twice at once (a toast
     under the crosshair and the line over the health bar). The toast sits over the fight.
  5. Not fixed, yours to decide: the wide shock rings in arena 4 bloom to a solid near-white band that
     hides the enemies standing behind it. Dimming a telegraph is a readability and balance call.

**Deliberately left out (say if you want them):** arena intro fly-overs (item 6: they touch the start of
every run and the "never block play" rule; I'd want you to see a prototype first), weather for the Act I
arenas (item 3), props that break (the destructible system exists; placing more is level design), and
moving scenery such as banners and birds (item 1's larger part).

**Proof.**
- `make test-polish` (now in `make test`): 28 checks. Caps hold after a thousand rounds and the oldest
  goes first; marks fade in and out with no pop; a pit on a pit renews it; the detail setting trims at
  once and OFF leaves only stains; casings appear after their fall and shrink away; the neon stutter is
  bounded, reaches only so far, is the same at 30 and 240 Hz and does not stutter with REDUCE FLASHING;
  emitter grains are finite, at their level, click-free and deterministic; the director never exceeds
  its budget, keeps pace at 30 and 240 Hz and is silent at OFF or out of range; ACT I, ACT II and FAST
  all yield sane emitters.
- `--bench 2000 --arena 4 --wave 2 --god`, three runs each, interleaved, same settings: `main` 12.70,
  12.99, 12.70 ms; this branch 13.09, 13.14, 13.12 ms. About +0.3 ms (2 to 3%) on a GPU-bound frame.
  Later runs drifted by 2 to 3 ms as the laptop warmed, so I could not pin down which part costs it
  (`--skip marks` gave 12.84 ms once). Treat it as "small, not zero".
- Stills in `docs/polish-review/`: `marks-before.jpg` / `marks-after.jpg` (`--overlay marks`),
  `blast-neon-stutter.jpg` (`--overlay jolt`, every third frame from the blast).

**Not done / to decide.**
- I can't hear. The emitter sounds are level-checked, not listened to.
- The mark shader and the tube stutter are new GLSL: compiled and seen on desktop; the web build is
  checked for compiling only (see the report), not looked at in a browser.
- Nothing here was play-tested by a person. The bot rarely shoots walls, so pits in a real fight are
  only seen staged.
- The feed's counting has no automated test (the HUD class needs a GL context).
