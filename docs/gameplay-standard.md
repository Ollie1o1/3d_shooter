# The gameplay standard

What a section of OVERDRIVE should be like to play, written down and measured, the
way the LIGHTING pass did for surfaces and the Blender pass did for scenery. Every
new section is built to it, and every old one can be held up against it.

The worked example is **Chapter 5's Thaw Room** (`src/world/LevelCold.h`, "the
room's layers"; `Gameplay_Cold.h` `updateThawHeat`). It meets every target, and its
tests (`make test-cold`) show how to prove each one.

## 1. The numbers

`make audit` measures every section of every mode (`tests/MapAudit.h`) and flags
what misses. `make audit ONLY=CH5` does one mode.

| Measure | What it is | Target (a fight of 3+ enemies) | Act I's arenas | Thaw Room before / after |
|---|---|---|---|---|
| levels | standing layers of 20 m² or more | 3 or more | 5-7 | 2 / **3** |
| cover | share of the floor within 2.5 m of something to hide behind | 25-50% | 22-36% | 22% / **29%** |
| sight | mean open distance at eye height | 12-30 m | 23-27 m | 25.9 / **25.5 m** |
| density | enemies across the waves per 100 m² | (story: 0.1-0.5; arenas 0.5-0.7) | 0.54-0.71 | 0.42 / **0.38** |
| detour | walked route to the next start / the straight line | under 1.5 | - | 1.00 / **1.00** |
| lights | self-lit parts per 100 m² (indoors) | 0.5 or more | 0.4-0.8 | 0.3 / **2.2** |
| dressing | props, models and shapes per 100 m² | 5 or more | 0.1-10 | 24.7 / **22.3** |

Bosses, duels (fewer than 3 enemies) and quiet sections are held only to the way
on, light (indoors) and dressing: a boss's open ground is its own design.

The audit is a floor, not a ceiling: a section can pass every number and still
play badly. The checklist below is the rest.

## 2. The checklist

Each rule, and where the Thaw Room does it.

1. **Read it before you're in it.** The fight's trigger starts a few metres past
   the entrance, and somewhere before it (a stair, a balcony, a doorway) shows the
   whole room. *Thaw Room: the front stairs climb to the catwalks outside the
   trigger; from the top you see every hook, tank and the lit exit.*
2. **Three layers.** The floor; a step up (one jump, or a ramp); high ground with a
   view. *The floor, the tank tops (a 1.2 m step crate puts them one jump up), and
   a catwalk 4.2 m up along each long wall.*
3. **No dead ends: everything is a loop.** Every high place has two ways down, and
   you can always get back up. *Stairs in all four corners; drop off a catwalk
   anywhere.*
4. **Cover in a rhythm.** Something to duck behind within a dash (about 6 m) of
   anywhere you fight, but long enough lanes to shoot down. *Five tanks, three
   step crates, the heater banks; 29% cover, 25 m sight.*
5. **The room has a rule, and the dressing is it.** One thing about this place that
   changes how you fight it, shown by what it's made of, not by text alone. *The
   heater banks warm a THAWLING three times as fast (it steams); the floor in front
   of each glows orange, and a sign says HEAT WAKES THEM. Fight away from the
   walls, kill the steaming ones first. The catwalks over the heat are safe ground.*
6. **One colour language.** Amber: the way on and what you can use (doors,
   stair noses, catwalk edges, chevrons). Cyan: information (logs, terminals,
   markers). Orange-red: danger (heat, tells). White on an attack: parry. *All
   four in the room.*
7. **Spawns are told.** Enemies arrive where you can see them coming, with a sound.
   *They drop from the hooks you walked under.*
8. **Waves escalate and teach.** The first wave is one kind; each one after adds
   one new problem. *5 Thawlings; then 4 and a CLERK (kill the restorer first);
   then 4, a PICKER and a Clerk.*
9. **The way on is lit and plain.** Detour under 1.5; the exit lit amber, the
   chevrons pointing at it. *1.00, a straight run to the amber door.*
10. **Something to find.** A wreck, a log, a secret, off the line but in sight.
    *Units 16 and 20 thawing against the tanks, tagged RECOVERED FROM.*

## 3. Proving it

A section is done when all of these pass:

- `make audit ONLY=<mode>`: the section meets the standard (or the miss is
  deliberate and written down in its chapter's design doc).
- `make test-route`: every chapter walkable section to section, every spawn sound.
  A new lift or grapple leg goes in `tests/test_route.cpp` AIDS.
- **A real-physics climb** of every new height: walk a `Player` up it with
  `L.baseFloor(x, z, radius)` (the game reads ramps at the body's leading edge) and
  `bodyEmbedded` each tick. The Thaw Room's is in `tests/test_cold.cpp`.
- **A bot fight:** `./shooter --story5 --arena 3 --god --cam X Y Z YAW PITCH
  --autoaim --kite --record 1 1800 /tmp/fight` plays 60 s; the frame shows whether
  the section cleared.
- **Screenshots** from the overlook, the entrance and the floor
  (`--cam x y z yaw pitch --shot 40 out.bmp`; yaw 0 looks +X, -90 looks -Z).
- `make light` after geometry changes (Chapter 5 has no bake yet).
- **Play it**, then `python3 tools/runlog.py`: deaths, time and time lost per
  section (lost: 20 s out of a fight getting no nearer the way on).

## 4. What we learned building it

- **There is no step-up.** A box you walk into is a wall. Walkable stairs are a
  `GroundRamp` under steps that are only drawn (`b.kit().box`), as the Living Ward
  does. End each ramp on a **solid landing** a body wide (0.45 m): a ramp that stops
  short of a deck leaves you a tick with no floor and you slide down its side.
- `b.wall` collides; `b.kit()`, `b.prop` and `b.neon` are only seen. Rails you can
  shoot past and drop over are kit; anything you stand on is a wall.
- A single jump rises about 1.7 m, a double about 3.4 m: steps for jumping stay
  under 1.3 m; 0.84 m steps that need a jump each are a chore, not stairs.
- A stencil's `right` vector is the viewer's right: looking -X it's (0, 0, -1),
  looking -Z it's (1, 0, 0). Long text on a small face goes on two lines.
- The audit and route graph read `L.baseFloor` and static walls only: movers,
  lifts and the grapple are invisible to them (that is what AIDS is for).

## 5. Where the maps stand (2026-11-05)

`make audit` at the time of writing: 17 of 55 sections meet the standard. The ones
to bring up first, worst and most played first:

1. **Chapter 3's middle rooms** (MOTOR ASSESSMENT to HADAL RESEARCH): about 8,000 m²
   each with 16-22% cover and 28-36 m sightlines; most are open floors with a few
   things on them. Add a second layer and cover islands in each.
2. **Chapter 5 outside the tower**: THE BLAST CHILLERS fights on 8% cover with 39 m
   sightlines; the Frozen Sea, 24-K's yard, the Manifest and the Vault are bare.
   The playtest said the same ("bare surroundings").
3. **Chapter 2's fights** (THE DEAD CONVOY, THE BURIED RELAY, THE TURBINE
   GRAVEYARD): 18-23% cover.
4. **Chapter 1's big halls** (THE ASSEMBLY HALL 14%, THE POD VAULT 21% cover).

The "bare" and "dark" flags undercount Act I, FAST and Act II, whose dressing and
lights come partly from systems the audit doesn't count yet (the arena dressing
models fitted outside the zones, the baked glow): read those two flags there as
rough.
