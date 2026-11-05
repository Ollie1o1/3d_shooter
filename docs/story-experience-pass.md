# Story spaces and the first revolver

November 5, 2026. A visual and traversal pass on Chapters 1, 2, 3 and 5.

## Chapter 1: see the way down

The furnace floor had a real collision opening, but a single rendered floor
patch covered it with concrete. That patch also occluded the baked light.
The opening is now uncovered in both representations. A wider reclamation
bay opens through the east wall and the Scrap Pit's receiving wall: UNIT 01
and the work light can be seen before stepping off the rim.

Amber collar lights, a legible SALVAGE BELOW sign, receiver ribs and guide
marks identify the drop. Old marks that floated over the expanded opening
are removed. The open receiver retains the physical transfer booster: walking
into it lands the player on the scrap beside UNIT 01, without a teleport.
The revolver marker can appear from the overlook when its sightline is clear;
the objective identifies the bay, then UNIT 01 under the work light.

Three machining stations add solid cover, standable roofs, service steps,
cooling fins, hydraulic feeds and occasional exhaust to the Assembly Hall.

## Chapter 2: freight as a combat space

The Dead Convoy has a raised loading gantry with ramps at both ends and an
open underside. Four stranded cargo chassis interrupt the longest firing
lanes and provide low roofs and climbing steps. Corrugated casings, ribs,
axles and ash connect the cover to the failed evacuation. Two generator
banks provide shelter between the Turbine Graveyard's exposed brake locks.

## Chapter 3: assessment apparatus

Motor Assessment and Long-Term Care have low fighting circuits: ramps onto
diagnostic stations, a connecting deck and a return to the floor. Curved
scanner hoops, diagnostic pads, marked runways and instrument panels give
them a purpose. Surgical Observation, Load Testing and both late observation
wards receive additional solid assessment equipment. The original galleries,
locks, deployable bridges, enemy introductions and quiet Conservatory remain.

### Recovery Ward entrance repair

The corridor from Admissions was centred at X=9 with a twelve-metre width,
but Recovery's front bay ends at X=12. Its rightmost three metres therefore
led outside the room, despite fitting inside the room's broader zone bounds.
Recovery's receiving bay now extends to X=16, enclosing the whole corridor
and providing a solid right door jamb. The repair follows the actual room
footprint, so its floor, wall, windows and dressing agree at the junction.
The original doorway and checkpoint positions still lead around the nurse
station through the intended walking route.

The regression walks both edges and the centre of that entrance. Before
the repair, the right-edge walk fell from the corridor to the void floor.
It also presses against both entrance walls to check physical containment.

## Chapter 5: service machinery and a quiet landmark

Each Blast Chiller has a frozen service module with shot-blocking casing,
a ramp, a standable roof, fin banks, amber lamps and raised feed pipes. The
central lanes and vent grates remain exposed. Two enemy starts move clear
of the new machinery. The first deck has an open flank around its module.

A small relay shelter stands off the Frozen Sea's main route, with a frosted
roof, a warm lamp, an aerial and an optional last transmission. Its open sides
frame the ice and submerged machinery without adding another combat gate.

## Verification and review

- `make test-story`: floor visibility, the overlook sightline, real descent
  physics, checkpoint stability, traversal, spawns, encounters and Sorter.
- `make test-wasteland`: both loading-gantry ramps, existing routes, spawns,
  objectives and Ballast.
- `make test-lab`: assessment circuits, Recovery doorway edges, original
  routes and galleries, spawns, bridges and Successor.
- `make test-cold`: service-deck climbs, full chapter route, spawns, vent
  warnings and existing cold mechanics.
- `make test-route`: all five chapters and every checkpoint.
- `make test-lighting` and `make test-web-lighting`: all seven shipped light
  bakes match the geometry constructed by native and WebAssembly builds.
- `make test-dev`: native game input, including the complete wake-up,
  leaving the belts, taking the salvage drop and interacting with UNIT 01.

Rebake changed chapters with `tools/light_bake story1 story2 story3` after
building that tool. Native review captures are under `build/story-experience/`.
Chapter 5 continues to use its existing unbaked lighting path.

Native review also includes sixty-second automated combat rehearsals in the
Assembly Hall, Motor Assessment and Blast Chillers. The first two encounters
cleared; the Chillers rehearsal reached its restoring-Clerk encounter. These
complement the real-controller route tests and opening integration check.

The map audit still flags low cover in several broad combat rooms and sparse
outdoor dressing. Its aggregate remains 17 of 55 sections meeting every metric;
these changes improve specific spaces, sightlines and routes within that wider
backlog. The Recovery Ward doorway regression and every new ramp are checked
independently of those aggregate measurements.

Release verification also found existing lighting differences between targets.
Builds now use precise floating-point arithmetic without contraction across
native and WebAssembly targets. Fracture counts snap division results near
whole numbers, so an exact 17 m edge produces ten pieces on every target rather
than shifting every later random draw. The fingerprint comparison remains
strict; lighting files are regenerated with the matching build settings.

The Successor rehearsal now dodges Suture lines across their nearest edge.
Its old policy ran away from a line's endpoint, sometimes staying inside the
hazard for its full length. Dashes now use the live 28 m/s speed and 0.30 s
momentum window, and preserve longer existing hit immunity. Its once-per-second
punch also reflects projectiles within the live reach and facing cone, without
crediting returned-shot damage to the boss. The boss and survival assertions
are unchanged.
