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

## Workstream 2: boss animation and sound (branch `boss-animation`): next

## Workstream 3: polish, feel and sound across the board (branch `polish-feel`): after that
