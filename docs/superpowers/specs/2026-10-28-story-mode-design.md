# STORY mode — design

Date: 2026-10-28 · Chapter 1 implemented 2026-10-29; design history follows.
Originally brainstormed with the user over one conversation;
decisions marked **From the user** are settled, **Proposed** ones are open to
change. Chapter 1 is the first piece to build, starting with its first room
and the opening cutscene.

## The story

A sentient robot, **UNIT 24**, wakes on its creator's furnace line: made, then
thrown away. It climbs out of the creator's factory and crosses a barren
hellscape to find the creator and learn **why it was made**.

- **From the user**
  - The question that drives everything: *why was I made?*
  - Lore comes **only through the bosses and short text**. No long cutscenes,
    no voiced exposition. Players who want lore find it; everyone else plays.
  - **The bosses are UNIT 24's siblings**: earlier creations, each made for a
    purpose.
  - **Wrecks of earlier copies** (Units 1–23, the same model) lie along the way.
  - **Linear chapters**, each one long level with a lot of movement through it.
  - **New maps**, built for story mode (not the ARENA/FAST levels reused),
    about **half the size of the whole FAST map** each: ~150 × 150 m of
    footprint, stacked in layers so the path is ~600–1000 m (~10–15 min on a
    first play).
  - **ULTRAKILL-style levels**: corridors, many fights against different
    enemies, things to find, the route going up, down, left and right, not
    one room.
  - **A tough boss at the end of every chapter**, on the level of the
    Sovereign and the Absolute. Story mode is made for these fights.
  - New bosses; old characters (Warden, Sovereign, Pacemaker, Penitent,
    Leviathan, Absolute) may come back as siblings.
  - **Details and animation matter**: build carefully, room by room, starting
    with the first room and the cutscene.

### UNIT 24 (From the user)

- **Build:** lean, human-sized (~1.8 m), slim human proportions; agile.
- **Condition:** half-finished. Bare frame and wiring show on **one side of
  the head** (a missing temple plate: wiring and the back of an eye) and **one
  side of the torso** (an open rib-like frame with a faint cyan glow inside).
- **Colours:** off-white ceramic plates on a dark grey frame. Stands out
  against the orange furnace and the red wasteland.
- **Glow:** **cyan / ice blue**, his signature colour (eyes, back spine,
  inner torso). Kept clear of the fight tells (white, gold, red).
- **Head:** a rounded box with a small brow plate over **two expressive eyes**
  that can dim, narrow and flicker under the brow (no mouth).
- **Hands** (seen all game in first person): off-white plates on the back of
  the hand and each finger, grey mechanical knuckles and joints bare between
  them, cables running into the wrist. Same joints as the current arm
  (`arm.glb`: upper, fore, fist, five fingers × 3) so the existing hand poses
  and springs drive them.
- **Signature:** a stencilled **24** on the chest plate. Every wreck carries
  its own number (01–23) in the same place.
- **Extra:** a **power spine** down his back, glowing cyan; dark on the dead
  wrecks.
- **Movement feel:** curious and a bit clumsy, newly born: fingers twitch,
  head tilts, small hesitations.

### The siblings

Every sibling knows its purpose and says it; UNIT 24's is blank. Each one's
death line is the lore. **Proposed** purposes for the existing bosses (for
later chapters):

| Sibling | Purpose |
|---|---|
| The Sorter (Ch. 1, new) | JUDGE |
| The Warden | KEEP |
| The Pacemaker | MEASURE |
| The Penitent | ATONE |
| The Leviathan | DEVOUR (eats failed creations) |
| The Sovereign | INHERIT (the favourite) |
| The Absolute / MORNINGSTAR | BE PERFECT |

### The wrecks

Units 1–23 left before UNIT 24. Their wrecks are numbered by how far each got:
Unit 1 barely out of the furnace, **Unit 23 slumped at the creator's door**.
Each wreck has one line, its last log. The player works out from the count
that they are the twenty-fourth; the game never says it. Not all 23 need a wreck the player can find: the numbers that are missing (destroyed, never found) say as much as the ones that are there.

### Text

All in the terse narrator voice (docs/text-pass.md): chapter title cards (2–3
words), sibling nameplates (`UNIT 0 · WARDEN · PURPOSE: KEEP`), boss lines
(one before, one or two during, one at death), one-line wreck logs, terminal
lines. UNIT 24's own nameplate shows its purpose blank, until the end.

### The ending (open)

Three candidates, not chosen yet: **to choose its own purpose** (the hopeful
one: six siblings were given purposes and were miserable; UNIT 24 was the
experiment without one), **to clean up** (it was made to destroy its
siblings; the wrecks were the units that refused) or **to replace him** (the
dying creator's test for a successor; a final choice).

## Chapter 1: THE CRADLE

The bottom of the creator's factory. The route climbs **up and out**,
twisting left, right and down on the way. Factory brutalism suits the engine
(walls are axis-aligned boxes; slopes are stairs).

### The opening cutscene (From the user: the wake-up on the furnace belt)

First person, no camera cuts, short:

1. Black. A boot sequence types out: `UNIT 24 ... ONLINE` / `MOTOR ... OK` /
   `PURPOSE: ——————` / `STATUS: REJECTED`.
2. Vision comes on in pieces (CRT-style flicker and tearing). On your back on
   a belt, the ceiling sliding past, an orange glow growing.
3. Your hands rise into view and the fingers flex for the first time (the
   five-finger hand rig; story mode wants **robot hands**, not the gloves).
4. Control comes back: get off the belt before the furnace. The bodies
   around you keep riding into the fire.

### The route

| # | Zone | What happens |
|---|---|---|
| 1 | **The Furnace Line** | The wake-up. Jump off the belt; learn to look, move, jump. **Unarmed.** |
| 2 | **The Scrap Pit** | A deep pit of thrown-away robots; climb the piles. Scrap wakes up: the first enemies. **Unit 1's wreck holds the revolver** (the first gun, from a dead copy of you). Dash, shooting, climbing. |
| 3 | **Maintenance Ducts** | Tight corridors, crawlspaces to slide through, forks; a hidden side room. **The Lancer on another wreck** (From the user) in this zone or the next. |
| 4 | **The Assembly Hall** | A huge hall; from catwalks you see the line still building siblings. Doors lock: the first real arena fight. |
| 5 | **The Archive** | Quiet, no enemies. Terminals, one line each. On a wall: `UNITS 1–23: DECOMMISSIONED. UNIT 24: REJECTED.` |
| 6 | **The Pod Vault** | Rows of glass pods with siblings asleep; partway through they crack open (ambush). Test chambers to the side foreshadow later bosses (a stopwatch on a running track, a guard post). |
| 7 | **The Elevator Shaft** | A platform rides up a shaft; enemies drop in at each floor. |
| 8 | **The Inspection Line** (From the user: one more room before the boss) | **Proposed:** a long gauntlet of scanner gates. Being caught in a beam stamps you and calls in enemies, so speed and movement keep you clean. It ends at a window onto the Sorting Floor: you see the Sorter at work, and it turns its head toward you. |
| 9 | **The Sorting Floor**: boss, **THE SORTER** | See below. |

**Ending:** the Sorter falls, the gate grinds open, and UNIT 24 walks out under
a red sky onto an ash wasteland, with a tower glowing on the horizon: where
the creator is.

**Proposed** level features (ULTRAKILL's): locked-door fights; checkpoints
every zone or two; secrets (hidden wrecks, a weapon variant) with
`SECRETS n/m` on the end screen; doors that open only from the far side as
shortcuts; rhythm of fight, travel, quiet; a rank at the end (time, style,
secrets) built on the existing scoring.

**Open:** whether the shotgun appears in Chapter 1 or is saved for Chapter 2.

### THE SORTER

`UNIT ? · SORTER · PURPOSE: JUDGE`. The machine that inspects every creation
and sends the rejects to the furnace: the thing that threw UNIT 24 away. It
knows you: `REJECT. RETURN TO LINE.`

**From the user:** really strong, on the Sovereign's and the Absolute's level;
its weapons fit what it is.

**Proposed** body and arsenal (everything an inspection machine would carry):

- **Body:** a gantry machine hung from rails over the sorting floor, a
  scanner-eye head, two long claw arms, a hydraulic stamp, inspection lamps.
- **The Claws:** grab and throw you onto the belts toward the furnace mouth;
  quick snatch combos. Parry the grab to jam the claw.
- **The Stamp:** a `REJECT` hydraulic press slammed down where you stand,
  leaving a branded plate; in combos it walks the stamps toward you. A
  parried or dodged stamp that bites into the floor sticks there: its core
  is open for a moment.
- **The Scanner:** a beam sweeping the floor. Caught in it you are **marked**,
  and its next attacks home in. A parry, or breaking line of sight behind
  cover, clears the mark.
- **The Line itself:** it runs the floor. It reverses belts, speeds them up,
  opens sorting chutes (trapdoors to the fire).
- **Throws siblings:** rejected bodies from the line, hurled as heavy shots
  (shoot them apart, or parry them back).
- **Phases (proposed):**
  1. **INSPECTION**: measured and readable; teaches every tell.
  2. **RECALIBRATION**: faster, combos chained, belts reversing under you.
  3. **INCINERATION**: the floor drops away to the furnace; you fight on a
     few platforms and on the Sorter's own rails (the grapple), it throws
     everything.
- The game-wide tells hold: every attack has a light **and** a sound cue;
  white: parry, gold: dodge or block, red: get out. Camping and range cheese
  are punished; there is always a skilful answer. Held to a kill time by the
  BossSim like the others.
- **Death line (proposed):** `I JUDGED THEM ALL. I NEVER KNEW WHAT FOR.`

## What has to be built

There is no conveyor and no cutscene system in the engine yet.

- **Story mode shell:** the STORY menu entry (now "coming soon"), a chapter
  run, checkpoints and respawning at them, a chapter end screen.
- **A level made entirely in Blender.** Today a Blender layout only sits on top
  of a level built in C++ (`world/LevelLayout.h`). New `od_kind`s: trigger
  volumes, checkpoints, locked doors, fight encounters (waves tied to a
  trigger), pickups (weapons, wreck logs), conveyors (moving floor),
  cutscene camera points.
- **Scripted sequences:** a timeline that can take the controls away, move the
  camera, pose the hands, type text, play sound and drive props; the wake-up
  is the first.
- **UNIT 24 in Blender:** `tools/blender/unit24_arm_build.py` (the
  first-person robot arm, `assets/guns/unit24_arm.glb`, same joints as
  `arm.glb`) and `tools/blender/unit24_build.py` (the full body,
  `assets/enemies/unit24.glb`: head, eyes, torso, limbs on joints, for the
  wrecks, the bodies on the belt and later shots of him; the wrecks swap the
  chest number and darken the glow).
- **Conveyors:** belts that carry the player and props.
- **Streaming/culling check:** a level of this size has far more boxes than
  any arena; measure before it gets big.

## Approach

Room by room, each one finished before the next:

1. **Design note** for the room: what you see, do, learn and find.
2. **Greybox**: plain boxes in Blender, played in game to check size and flow.
3. **Art pass**: props built in Blender (belts, furnace, bodies, arms).
4. **Animation**: keyed in Blender where it can be, sprung in code where it
   should feel alive.
5. **Light and sound**: baked light, the furnace's glow, generated sound.
6. **Review** with screenshots (`--overlay`) and play; then the next room.

Chapter 1 starts with **the opening cutscene and the Furnace Line**.

## Original handoff (2026-10-29, before Chapter 1 completion)

- **The Furnace Line** and the wake-up (`src/world/LevelStory.h`, `src/game/Gameplay_Story.h`).
- **The Scrap Pit**: the chute drops into a boost tunnel that throws you out of a hatch onto the landing heap.
  UNIT 01 sits under the only lamp, the revolver glinting in its lap (a waypoint after 3 s, a `[E] TAKE THE
  REVOLVER` prompt, its last log typed out as you near it). Taking it wakes the scrap: two waves (husks, two of
  them rising on your own heap; then husks and mites). Then the climb out up eight steps to the duct door.
- **The Maintenance Ducts**: a corridor, the junction fight (husks, rippers, mites; the gate locks behind you),
  a crawlspace under a pipe run (slide; you crawl on if the slide runs out under it), UNIT 02 with the Lancer,
  a secret vent room with UNIT 03, a shaft of ledges up to the end card: THE ASSEMBLY HALL, next.
- Guns are found, not given: unfound slots are empty, the wheel and number keys skip them, what you've taken
  stays taken through a retry. No grenades, grapple or sword in the story yet.
- Dev starts: `--story --overlay pit | pittake | ducts | chute | wakeNNN`.

## Chapter 1 implementation (2026-10-29)

All nine zones now connect to the Sorter and the ash-wasteland ending. The original
three-room handoff above is retained as history. Current implementation, room
intent, developer starts, verification and next-session notes are in
[`docs/story-chapter-1.md`](../../story-chapter-1.md).

- Assembly Hall: three working lines seen from catwalks, a production-floor fight,
  three enemy waves, entry and exit gates, a staircase back up to the Archive.
- Archive: no enemies, three optional terminals, the decommission register on its wall.
- Pod Vault: twelve sleeping units, breakable glass, three ambush waves, UNIT 06's
  shotgun and UNIT 07 in a hidden observation cell.
- Elevator Shaft: board the freight platform; it climbs 39 metres and waits at
  three floors for their defenders. A fall returns you to the platform.
- Inspection Line: six moving scanner beams; being marked calls collectors.
  UNIT 08's log and the grapple precede the boss. A clean crossing stays quiet.
- Sorting Floor: a new Blender gantry model and dedicated Sorter AI; scanner
  marking, parryable claws, committed stamps with exposed-core recoveries,
  rejected bodies that can be shot or parried, reversing belts, and three phases.
  Four corner tiles warn before dropping; the cross and perimeter stay traversable.
- Ending: the exit unlocks, the factory opens onto ash and a red sky with the
  creator's tower ahead, then a chapter-complete card with time, kills, style,
  deaths, rank and two secrets. Enter returns to the menu.
- Weapons, grapple and discovered logs survive a retry in the current run.
  Starting a new run clears them. Story runs never post to the arena leaderboard.

The open shotgun question is implemented as a Pod Vault pickup. The overarching
story's final purpose/ending remains undecided; completing Chapter 1 does not pick it.
