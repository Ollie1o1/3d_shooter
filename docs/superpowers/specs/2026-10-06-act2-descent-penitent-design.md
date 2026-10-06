# Act II piece 4: The Descent + The Penitent — design

Date: 2026-10-06 · Status: approved in conversation (approach, sections 1–2); section 3 written into this spec for review · Piece 4 of 6 of Act II · Feel-track piece B

## Decisions made with the user

- **The Descent plays as "ride the cage"**: the fight is on a huge round
  elevator platform hanging in a vast shaft. It plunges between waves and stops
  at a different floor each wave; the bottom stop is the Penitent's pit.
- **The Penitent is a chained colossus**: a huge kneeling machine-monk chained
  to the shaft walls. Phase 1 chained (rip the anchors), phase 2 unchained and
  stalking, phase 3 scourging itself with an exposed wound.
- **Approach: a real moving cage** (driven movers, real geometry rushing past),
  not a fake scroll or a cut.
- Standards that apply: maps grand and organic, not boxes (one set-piece per
  stop, things visible beyond the walls); the **boss bar** (every attack has a
  light *and* a sound tell, phases change the fight, camping and range-cheese
  are punished, there is always a skilful answer); the "dark machine + glow"
  direction; the new sound engine (positional cues, ducks, SHAFT reverb).

## 1. Layout and the ride

### Getting there

The Orrery's north arch (today's finish, (0, −80, −778)) becomes a doorway: it
opens on clearing the Orrery and leads up a short corridor to the **top
landing** of the shaft. `LevelData::corridors` gains the corridor (Orrery →
Descent). The finish beacon and TO BE CONTINUED move to the bottom of the pit;
the menu label becomes "PREVIEW · 3/4 ARENAS".

### The shaft

- Centre (0, ·, −840). A round shaft, inner wall radius 34 m (collision as
  boxes in a ring; drawn with `ShapeKit` curves and ribs), from the landing at
  Y −80 down to the pit floor at Y −240. A basin for Z −790…−900 at −300 so the
  shaft is open below the cage.
- Dressing all the way down: ribbed arches every 10 m, hanging chains, lamps,
  the four stops' galleries, a red glow from below that grows as you descend.
  Visible above: the dark mouth of the shaft and the Orrery's light.
- Arena zone: the shaft's cylinder (XZ box inside the wall), ceiling just above
  the landing; `voidY` follows the cage (see Falls).

### The cage

- A round platform, radius 14 m, iron rim and railing, chains running up into
  the dark from four points. Collision: five boxes (a centre square and four
  side slabs) driven together so the walkable area is close to the circle.
- **Driven movers**: a new `Mover::Path::DRIVEN` whose offset comes from a
  value the game sets, not from the mover clock. `LevelData::lift` holds the
  cage's mover indices, its stops (Y −80, −120, −160, −200, −240), the current
  and target stop, and the ride clock. A ride lasts **8 s** with ease-in/out.
  `Mover::delta` is set as for any mover, so the player is carried by the
  existing carry code.
- The cage only moves **between waves**, when no enemy is alive: enemies never
  need to ride it. Projectiles and pickups left from the wave are cleared when
  a ride starts.
- On each ride: the wave banner holds until arrival, two health orbs fall in
  with the cage, the music's DESCENT preset swells, and a positional rumble +
  chain rattle plays from the cage's chains (WORLD group).

### The stops

At each stop a ring of galleries surrounds the shaft at the stop's height, and
**four bridges** (N, E, S, W) swing out from the galleries to the cage's rim
when it arrives (static walls at that height, ending just outside the cage's
radius, so the cage passes them on the way down). Enemies spawn in the
galleries and on the cage.

| Stop | Y | Floor | Set piece | Wave |
|---|---|---|---|---|
| Top | −80 | Landing | the shaft's mouth, the Orrery's gold behind you | (board; the gate shuts; first ride) |
| 1 | −120 | **Bell galleries** | arcades round the shaft, great bells hanging in them, enemies pour out of the arches | Kill all |
| 2 | −160 | **The Clamps** | four great brake clamps bite the cage's rim | **CONDUITS goal: destroy the 4 clamps** ("RELEASE THE CLAMPS") while enemies come |
| 3 | −200 | **Furnace ring** | catwalks round furnace mouths, the shaft glowing red from below | Kill all, the hardest mix |
| Bottom | −240 | **The Penitent's pit** | a round floor R 30 the cage docks flush into; six chain anchors high on the walls; the Penitent kneels at the north | Boss |

Wave mixes (Act II enemies, Hollowed variants):

- **Stop 1**: Husks, Raptors, 2 Seraphs, a Shieldbearer squad, 2 Enraged Rippers.
- **Stop 2** (clamps): an Anchor, 2 Conductors, Sentinels, Mites, 2 Haloed Brutes.
- **Stop 3**: 2 Anchors, a Juggernaut, a Twinned Shieldbearer squad, 2 Seraphs,
  Enraged Raptors, Husks.
- **Bottom**: the Penitent (boss wave, as the Warden's).

`maxAlive` 12, `damageScale` 1.4.

### Falls

Falling into the gap between the cage and the galleries (below the current
stop's Y − 25) puts you back on the cage with the usual 15-damage penalty.
`Arena::voidY` and `respawn` are updated per stop. Enemies that fall die
(environment kill, half style).

### Sound and music

- `ReverbSpace::SHAFT` for the Descent (`Arena::space`).
- Music: an eighth synth preset **"DESCENT"** (slow, heavy, low brass and
  chains) for the Descent; the Penitent fight raises its layers like the
  Sovereign's. `MUSIC_TRACKS` 8; Act II picks track 5 + arena index.

## 2. The Penitent

### What it is

`EnemyType::PENITENT` (a boss: `isBoss`). A kneeling machine-monk about 9 m
tall (12 m standing), a rig of boxes in blackened iron, a bone mask under a
hood, two censers on chains glowing amber, six chains from its body to the
anchors. Health **6000** (before difficulty `tune().health`). Spawns at the
pit's north edge (`Arena::bossSpawn`).

### Chain anchors

- Six anchors on the pit wall, 8–12 m up, spread round the north half and the
  sides. Each is a wall box (grapple-able) with a glowing sigil, 400 HP.
- **Shoot** an anchor: hitscan and blasts that hit its box damage it.
- **Rip** an anchor: grapple onto it and stay attached 0.5 s → it breaks (+40
  style, "RIPPED"), the grapple releases.
- Each anchor broken: its chain falls (debris), the Penitent recoils (1 s
  stagger), a heavy positional clank.
- `LevelData::anchors` holds them (wall index, hp, alive); a broken anchor's
  wall is parked out of the world like an open door.

### Phase 1 — CHAINED (any anchor intact)

- Cannot move (turns in place). Takes **25 %** damage.
- **LOW SWEEP**: a censer swung round at ankle height across a 200° arc in
  front (reach 16 m). Hits a player whose feet are below 1.0 m above the floor.
  **Answer: jump.** Tell 0.9 s: censer drawn back low, amber glow, rising
  grinding whine.
- **HIGH SWEEP**: the same arc at head height. Hits a player standing or in
  the air up to 3.5 m; misses a crouching/sliding player. **Answer: slide or
  crouch.** Tell 0.9 s: censer raised high, white glow, a bell tone.
- **SLAM**: both fists, shockwave ring radius 14. **Answer: jump.** Tell 1.0 s.
- **INCENSE**: every ~12 s, three burning pools (radius 3.5) land near the
  player for 8 s (20/s).
- **CHAIN LASH** (anti-cheese): if the player stays beyond 22 m or 4 m above
  the pit floor for 3 s, a red line is marked on the ground toward them for
  0.7 s, then a chain whips along it (35, reach 40 m).

### Phase 2 — UNCHAINED (all anchors broken)

- Banner "THE PENITENT RISES" (duck, roar). It stands and stalks at 3.5 m/s.
  Full damage; **head is a weak point** (×2).
- Sweeps chain **low → high** (or high → low) with a fresh tell for each.
- **STOMP** up close (radius 6, jump it).
- **CHAIN LASH** also yanks the player toward it on a hit.
- Calls **4 Hollowed Husks** every 25 s.

### Phase 3 — SCOURGE (under 25 % health)

- It whips its own back: a **glowing wound** opens there (×3 damage, a separate
  hit box on its back).
- Every attack 30 % faster (shorter tells and cooldowns).
- Each self-lash sends a **low ring of embers** outward (15, jump it).
- The music peaks.

### The skilful answer: parry

Punch a censer in the **last 0.25 s** of a sweep or slam as it reaches you
(the player's 1 s parry cooldown makes the timing matter): it staggers for
**2.5 s** and takes **×2** damage ("HEAVY PARRY", hit-stop).

### Damage and tells

- Sweeps and slam 30, lash 35, stomp 30, embers 15, incense 20/s; all × the
  arena's `damageScale` × difficulty.
- Every attack has a light tell (censer glow colour: amber low, white high,
  red lash line) and a positional sound tell (ENEMY group, priority); big tells
  duck the mix (−6 dB, 0.5 s).

## 3. Code shape, testing

### Code

- `src/LevelAct2.h`: `buildDescent` (corridor, shaft, cage, stops, bridges,
  galleries, pit, anchors) called from `buildAct2`; the Orrery's arch becomes a
  doorway; finish beacon at the pit.
- `src/Level.h`: `Mover::Path::DRIVEN` + `Mover::drive` (0..1 along a→b);
  `LevelData::Lift` (movers, stops, current, target, clock; `startRide`,
  `update`); `LevelData::anchors`.
- `src/ArenaShifts.h`: `ArenaShift::DESCENT` — on wave cleared, start the ride
  to the next stop; reset puts the cage at the top.
- `src/Enemy.h`: `EnemyType::PENITENT`, stats, `thinkPenitent` (phases,
  attacks, chain lash rule, incense/ember/lash events in `EnemyEvents`),
  `Enemy::anchorsLeft` fed by the game; parry window on sweeps/slam.
- `src/EnemyModel.h`: `rig::penitentPose` + the rig (kneeling / standing),
  head box and back-wound box.
- `src/PenitentHazards.h` (new, no GL/audio, like `SovereignHazards.h`):
  incense pools, lash lines, ember rings, sweep arcs → hits on the player.
- `src/Gameplay_Penitent.h` (new): feeds events to the hazards, anchor
  damage/rip, drawing of hazards and chains, sounds and ducks.
- `src/MusicSynth.h`: preset "DESCENT", `MUSIC_TRACKS` 8.
- `src/MenuState.h`: "PREVIEW - 3/4 ARENAS".
- Dev: `--act2 --arena 3` starts at the Descent's top; `--spawn 14` the
  Penitent; `--overlay penitentN` freezes poses for screenshots.
- New headers in the Makefile `HEADERS` and `test` lists.

### Automated tests (`tests/test_game.cpp`)

- Map: spawns clear of walls and inside bounds at every stop; ceilings;
  the corridor joins the Orrery to the landing; doors.
- The cage: over a full descent its boxes never pass through a static wall;
  at each stop it sits flush (±2 cm) with the four bridges; a player standing
  on it for a whole ride is carried and stays on; it only rides when a wave is
  cleared and nothing is alive.
- Falls: below the stop's void line → back on the cage, 15 damage.
- The Penitent: chained, it never moves; takes 25 % while an anchor holds;
  ripping (0.5 s attached) and shooting both break an anchor and stagger it;
  all anchors broken → it walks; a low sweep hits a standing player but not
  one jumping; a high sweep hits a standing player but not one crouching; the
  lash fires only after 3 s far away or perched, never when close on the
  floor; under 25 % the wound takes ×3 and tells are shorter; a parry in the
  window staggers it 2.5 s; it never hits during a stagger.
- A simulated ACT II run clears the Nave and the Orrery, rides the Descent,
  kills the Penitent and finishes; Act I and the Nave/Orrery keep passing.

### Manual

Screenshots: the landing looking down, mid-ride, each stop, the clamps, the
pit with the Penitent kneeling, each pose (`--overlay`), phase 3's wound.
`--audiodump` of a ride and of the fight (rumble panned from the chains, ducks
on tells). `--bench` mid-ride and in the fight vs the Sanctum (within ~15 %).
`make web`.

## Out of scope

The Reliquary, Revenant, Weaver (piece 5); the Leviathan (piece 6); enemies
riding moving platforms; new enemy voices (feel-track piece D); the ACT II
leaderboard.
