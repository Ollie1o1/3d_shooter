# Act II piece 5: The Reliquary + the Revenant + the Weaver — design

Date: 2026-10-07 · Status: approved in conversation (approach 1, sections 1–3) · Piece 5 of 6 of Act II

## Decisions made with the user

- **The chunks drift and regroup**: broken pieces of the four Act I arenas
  float over the void; between waves they glide into a new arrangement, so
  each wave is a different layout; mid-wave only a few small debris pieces
  orbit.
- **The Revenant — catch the soul**: kill its body and its soul flees toward
  another chunk; shoot or punch it to end it, or it re-forms.
- **The Weaver — bridge-wires**: it strings glowing wires across gaps and
  walkways; touching one snares you and wakes the enemies; cut a node, duck or
  jump it, or kill the Weaver.
- **Approach 1**: chunks are groups of driven movers (the Descent cage's
  system, generalised); they only move between waves, so enemies never ride
  moving ground.
- Assumed and not objected to: reached from the Descent's pit; three waves;
  it ends at the rim of a dark hole where the Leviathan (piece 6) will rise —
  a finish beacon and TO BE CONTINUED until then; the menu reads
  "PREVIEW - 4/4 ARENAS".
- Standards: maps grand and organic, not boxes; the boss bar for every new
  enemy (light and sound tells, a skilful answer, no cheese); "dark machine +
  glow"; voices in the voice bank; on-screen text in the terse-narrator voice
  (docs/text-pass.md).

## 1. The Reliquary

### Getting there

When the Penitent dies, the Descent pit's south wall (Z ≈ −874, Y −240)
cracks open (a door, `Arena::exitDoor`) onto a broken bridge out over the void
to the Reliquary's landing (the Yard chunk in wave 1's arrangement).
`LevelData::corridors` gains the bridge. The finish beacon and TO BE
CONTINUED move from the pit to the hole's rim.

### The space

- A vast void south of the shaft, centred about (0, −245, −960), roughly
  110 m across (X −55…55, Z −905…−1015). The basin under it bottoms out far
  below (−500), so the void is open; the Descent's basin ends at Z −880.
- Lit from below by a dim red glow, from above by the eclipse through a rent
  in the ceiling. Reverb: HALL. A ninth music track, **RELIQUARY** (sparse,
  hollow, distant choirs); Act II picks track 5 + arena (Reliquary = 8),
  `MUSIC_TRACKS` 9.

### The chunks

Each a fragment of an Act I arena, edges broken and glowing, in that arena's
materials:

| Chunk | Size | Content |
|---|---|---|
| **Yard** | ~20 m | brick plaza, low walls, the dais |
| **Foundry** | ~18 m | metal slab, a cold furnace, an empty lava channel (cover) |
| **Spire** | ~16 m | stone tier with columns, a raised ledge, the tallest |
| **Core** | ~18 m | slate ring, two pillars |

Plus ~8 debris pieces (4–6 m) as stepping stones and 2–3 small orbiters that
circle the centre slowly mid-wave (no spawns on them). Every big chunk has a
grapple pillar or column; every gap between neighbours is crossable with a
jump + dash (≤ 9 m horizontal, ≤ 3 m up) or has a grapple point in reach.

### Three arrangements

1. **Scattered** — the four chunks far apart, debris between them.
2. **The ring** — the chunks circle a central gap; orbiters cross it.
3. **The stack** — the chunks at different heights, a climb from the Yard up
   to the Spire.

### The drift

- `ArenaShift::DRIFT`: when a wave is cleared (nothing alive), the chunks
  glide to the next arrangement over **6 s**, eased in and out; the next wave
  is held until they settle (`WaveDirector::hold`, as the Descent). A grinding
  rumble plays from each moving chunk (positional, WORLD).
- Projectiles and pickups left from the wave are cleared when a glide starts;
  two health orbs settle on the chunks with each glide.
- A player standing on a chunk is carried (the movers' `delta`, as the cage).

### Falls

Below the chunks (`voidY`, per arrangement): the player is put back on the
**last chunk they stood on** with the usual 15 damage; an enemy that falls
dies (environment kill, counted as yours).

### The end

After wave 3 the hole opens in the void at the far side (a dark ring of
debris, a glow below); the finish beacon stands at its rim.

## 2. The new enemies

### THE REVENANT (`EnemyType::REVENANT`)

A gaunt machine-knight, **160 hp**, quick; a white-blue soul glows in its
cracked chest.

- **Rake** (melee, two hits; tell 0.45 s: arms drawn back, a hiss) and a
  **soul bolt** at range (a slow, parryable bolt; tell 0.6 s: the chest
  flares, a rising choir note).
- **Its soul**: when the body dies, the soul tears free (a rising choir, a
  light trail) and flies for **~4 s** toward a spawn point on another chunk.
  - **Shot** (a small target, 40 hp, hitscan and blasts) → gone for good:
    "SOUL TAKEN", +style.
  - **Punched** as it passes (the parry, in reach) → gone, a big style bonus.
  - **Reaches its spot** → re-forms over 1 s (a light column, a choir swell)
    at **half** the health it last had; each re-form halves again; after two
    re-forms its third death is final.
- Answer: finish what you start — track the soul, or kill it where it can't
  run far.

### THE WEAVER (`EnemyType::WEAVER`)

A long-legged spider machine, **120 hp**, a violet spinner; keeps its
distance on a chunk's edge.

- Every ~6 s it strings a **wire** at chest height (1.2 m) between two points:
  across a gap you'd jump, or along a walkway near you. Tell **0.8 s**: it
  rears, the spinner brightens, a rising buzz. At most **4** wires per Weaver;
  a new one replaces its oldest.
- **Touching a wire**: snared **1.2 s** (slowed ×0.4, no dash), **10**
  damage, a twang, and every enemy within 30 m has its next attack readied.
- Answers: **slide under** or **jump over**; **shoot a node** (the glowing
  anchor at either end) to cut it; **kill the Weaver** and all its wires drop.
  Wires clear when the chunks drift.

### Their voices and hints

Both join the voice bank (Revenant: a hollow, breathy choir over armour;
Weaver: clicking legs and a buzzing spinner; a tell per attack, the soul's
flight and re-form as specials). NEW hints:

- "NEW: REVENANT · ITS SOUL RUNS - CATCH IT BEFORE IT COMES BACK"
- "NEW: WEAVER · IT WIRES THE GAPS - CUT THE NODES OR DUCK UNDER"

### The waves

`maxAlive` 12, `damageScale` 1.45.

1. Scattered: Husks, Raptors, 2 Revenants, a Shieldbearer squad, a Seraph.
2. The ring: 2 Weavers, 3 Revenants (one Enraged), an Anchor, Sentinels,
   Rippers, Mites.
3. The stack: a Juggernaut, Haloed Revenants, 2 Weavers, 2 Seraphs, a
   Conductor, Twinned Husks.

## 3. Code shape, testing

### Code

- `src/LevelAct2.h`: `buildReliquary` (bridge, chunks and debris as driven
  mover groups, pillars, arrangements, the hole and finish, basin).
- `src/Level.h`: `LevelData::Formation` — groups of DRIVEN movers with a
  target offset per wave and an eased glide (generalising `Lift`);
  "last chunk stood on" for falls.
- `src/ArenaShifts.h`: `ArenaShift::DRIFT` (glide after a cleared wave, hold
  the next).
- `src/Enemy.h` + new `src/EnemyRelic.h`: `REVENANT`, `WEAVER` (stats, minds,
  events: soul released / re-formed, wire strung).
- New `src/RelicHazards.h` (no GL/audio): souls in flight (target, shot,
  punched, arrival) and wires (touch test, snare, nodes, cut, cleared).
- New `src/Gameplay_Reliquary.h`: events → hazards, snare and wake, re-form,
  drawing (wires, souls, the hole), drift sound.
- `src/EnemyModel.h` rigs; `src/VoiceTable.h`/`src/VoiceSynth.h` voices;
  `src/MusicSynth.h` RELIQUARY (9 tracks); `src/MenuState.h` "4/4 ARENAS";
  dev `--act2 --arena 4`, `--spawn 15/16`.
- New headers in the Makefile's `HEADERS` and `test` lists.

### Automated tests

- **Map**: in every arrangement spawns stand on a chunk, inside bounds, clear
  of walls; every neighbouring gap is crossable (jump + dash) or has a grapple
  point in reach; the bridge joins the pit to the landing.
- **The drift**: chunks never pass through each other or static walls while
  gliding; each glide ends exactly in its arrangement; a player standing on a
  chunk is carried and stays on; the next wave is held until it settles.
- **Falls**: back on the last chunk stood on, 15 damage; a falling enemy dies
  as your kill.
- **Revenant**: a killed one releases a soul that flies toward another chunk;
  shot → gone; punched → gone with the bonus; uncaught → re-forms at half,
  then quarter; the third death is final.
- **Weaver**: wires appear after their 0.8 s tell, at most 4; a touch snares
  1.2 s with 10 damage and readies nearby enemies; sliding under or jumping
  over doesn't touch; a shot node cuts it; the Weaver's death drops all its
  wires; a drift clears them.
- **Cheese** (tests/BossSim-style scripted players) against both new enemies.
- **Voices**: every new attack has a tell in the bank.
- **A simulated Act II run**: Nave → Orrery → Descent → Reliquary, ending at
  the beacon; Act I and the other Act II tests keep passing.

### Manual

Screenshots of each arrangement, a glide mid-way, a fleeing soul, wires, the
hole; `--bench` during a glide and in a full wave vs the Sanctum (within
~15 %); `make web`.

## Out of scope

The Leviathan (piece 6); enemies riding moving ground; new Hollowed variants;
an ACT II leaderboard.
