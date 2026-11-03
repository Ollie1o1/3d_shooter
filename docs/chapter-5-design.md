# Chapter 5: THE COLD STORE — design

Brainstormed with the user on 2026-11-03. Built so far: RECALL (step 1 of the
build order; `--recall` to try it anywhere) and the planet (step 2;
`./shooter --story5`): the ice shelf, its lip and the Void, the Store's towers
and glacier cliff, the frozen giants, the sky. Rooms 1-5 and their enemies are
built too (step 3), rooms 6-7 (step 4), 24-K (step 5) and the Restorer's
fight (step 6); see "as built" below.

The creator never threw anything away for good. Everything is **saved** here:
every unit, every sibling, and copies of UNIT 24. The keeper of the store, THE
RESTORER, believes change is damage. After four chapters, UNIT 24 has changed a
lot.

**From the user:** an **icy planet**, not just a building. Vast landscapes,
huge backgrounds, voids, and things to see in the distance, on the scale of the
Gleaner's plain. Also settled by the user: 24-K is the mid-boss, the Restorer
**dies** at the end (it is not spared), the checkpoint and death text turn into
the Restorer's words, and the frost on UNIT 24's hands is **cosmetic only**.

## 1. The bridge: the end of Chapter 4

The Gleaner's portal already shows a cold, measured grid. Walking in:

1. White. The grid becomes a floor of frosted steel plates.
2. UNIT 24 steps out onto a ledge of an **ice shelf** under a black sky.
3. The first vista (see section 2) with no music, only wind.
4. Chapter card: `THE COLD STORE`.

## 2. The planet

**Palette.** Frost white, pale grey-blue, black steel, black sky. Warmth comes
only from **amber warning lamps** and sodium lights on the machinery. Keep the
world's blue washed out so UNIT 24's cyan stays his alone. The aurora is pale
green and violet, never cyan.

**The sky (a new skybox, like `uPlain`).**
- A **ringed gas giant** low on the horizon, pale and enormous, its rings a
  thin bright line across the sky. It never moves; it is the chapter's "moon".
- A small, cold white **sun** that gives no warmth: long blue shadows.
- Stars, slow aurora curtains, and a far blizzard wall that drifts along the
  horizon.

**Things to see in the distance** (low-detail silhouettes under fog, the way
the far Gleaner works):
- **The Store itself:** a megastructure cut into a glacier cliff kilometres
  wide. Storage towers rise out of the ice like frozen skyscrapers, with amber
  lamps in rows. Hook rails cross between them over chasms.
- **Frozen giants:** siblings far bigger than the Gleaner, standing half buried
  in glaciers on the horizon like mammoths in ice. One has an arm raised.
- **The Void:** where the ice shelf ends, it ends in nothing. Racks and hook
  chains hang down off the edge into black, and ice shards drift up out of it.
- **The frozen sea** (section 3, room 7): clear ice with shapes under it.
- **Dispatch launches:** now and then a capsule on a rail fires up into the
  sky from a far tower and leaves a long contrail toward the gas giant
  (foreshadows the ending).

**Small detail:** frost slowly creeps over UNIT 24's first-person hands when he
stands still, and dashing or fighting shakes it off. Cosmetic only (user).

## 3. The route (10 places)

Inside rooms alternate with huge exterior spaces so the planet is always
visible: through windows when you're inside, all around you when you're out.

| # | Place | In/out | What happens |
|---|---|---|---|
| 1 | **THE INTAKE** | Out | Arrival on the ice shelf; the first vista of the Store across a chasm. A belt carries frozen bodies to be hung: the Chapter 1 furnace line in reverse. Quiet. |
| 2 | **THE HOOK LINE** | Out | Overhead rails carry hanging units across the chasm with the Void below. Grapple from hook to hook while they move. Falling returns you to a safe ledge and costs health, as on the Broken Span. Pickers (section 5) appear. |
| 3 | **THE THAW ROOM** | In | First fights. Frozen units drop off their hooks and wake up as Thawlings. Tall windows onto the glacier. |
| 4 | **INVENTORY** | In | Quiet. Terminals, one line each: `UNIT 07: RESTORED 41 TIMES.` `UNIT 12: RESTORED. DID NOT RESUME.` |
| 5 | **RACK 24** | In | An aisle of copies of you, **24-A to 24-J**, blank and frosted; some heads turn to follow you. You tear the RECALL module (section 4) out of 24-A's chest. **Slot 24-K is empty.** |
| 6 | **THE BLAST CHILLERS** | Out | An arena fight on top of the Store's cooling towers, high above the glacier. Freezing vents fire on a cycle. Teaches RECALL under pressure. |
| 7 | **THE FROZEN SEA** | Out | A long crossing of a frozen ocean under the aurora. Under the clear ice lie huge stored things. One is `RECLAIM v1`, an early Gleaner 300 m long, directly under your feet. The ice cracks along your path. Lore only, no fight. |
| 8 | **24-K** | Out | Mid-boss duel at a broken rail station on the far shore (section 6). |
| 9 | **THE MANIFEST** | Out | A long causeway over the Void toward the Vault, which hangs over a sinkhole. The interface turns into the Restorer's (section 7). |
| 10 | **THE VAULT** | Out | The Restorer (section 8). |

**Secrets:** a wreck or two from Units 1–23 that got this far; a thawed sibling
in a side room; the empty hook where something enormous used to hang.

## 4. The new ability: RECALL

Taken from a sleeping copy of yourself in Rack 24.

- **Press once:** leave a frozen statue of yourself where you stand.
- **Press again within ~8 s:** snap back to it. The statue shatters and hits
  everything near it.
- While the statue stands, enemies are drawn to it for a moment.
- Moves position only, never the view (the Chapter 3 camera rule).
- It answers the Restorer's ROLLBACK (section 8): the boss resets you, you
  get a reset of your own.
- **As built** (`src/player/Recall.h`, `src/game/Gameplay_Recall.h`): its own
  rebindable key, **X** (gamepad: left-stick click). The statue waits 8 s, then
  crumbles harmlessly; 3 s cooldown after either. The shatter is a 5 m, 65
  damage blast. For 1.75 s, non-boss enemies within 22 m aim at the statue
  instead of you, and their melee swings land on it. If a gate has shut, or the
  statue is outside the arena you're fighting in, it breaks without moving you
  (`RECALL BLOCKED`). Not yet given out in the story (Rack 24 isn't built).

## 5. New enemies

- **THAWLINGS.** Units fallen off their hooks. They crawl, stand and speed up
  as they warm: kill them early.
- **PICKERS.** Drones on ceiling and hook rails that snatch you and hang you on
  a hook. Shoot or parry free.
- **COLD-CHAINS.** Heavies that spray freezing gas and leave ice patches you
  slide on.
- **CLERKS.** Small floating drones with clipboard lamps. **While a Clerk is
  alive, enemies you kill come back**: they rewind to standing. This teaches
  "kill the restorer first" before the boss, and gives every fight a priority
  target.

## 6. Mid-boss: 24-K

The empty slot from Rack 24. The Restorer took a copy of you and reset it to
what you were meant to be.

`UNIT 24-K · PURPOSE: COMPLY · STATUS: ACCEPTED`

- A duel at your size, against your own Chapter 1 kit: revolver and Lancer,
  dash and parry. It moves like your newborn self, but crisp, with none of the
  hesitation.
- It has none of what the journey gave you: no grapple, no splinter, no
  recall. That is how you beat it.
- Lines: before *"YOU ARE DAMAGED. I AM CORRECT."* Death: *"WHAT WAS IT LIKE?"*

## 7. The Manifest: the interface belongs to the Restorer

From room 9 to the end of the chapter (once, quietly):
- The checkpoint popup reads `RESTORE POINT SAVED`.
- Dying shows `RESTORED` instead of the usual death screen.
- In the fight it says *"I HAVE SAVED YOU 14 TIMES."*, using your real death
  count for the chapter.

## 8. THE RESTORER

`UNIT ? · RESTORER · PURPOSE: RESTORE`

**Body.** About 7 m, spindly and stooped, on long stilt legs (it walks; it is
not rail-hung like the Sorter). Four long arms ending in cryo tongs, a coat of
frost-crusted hoses, a glass dome head with one cold lamp inside. On its back,
a **cryo cylinder** holding a small sleeping unit: the first thing it ever
saved. It is careful and polite and handles you like a museum piece.

**Arena: THE VAULT.** A round platform about 60 m across hanging over a
sinkhole into the Void, with the grid floor from the portal. Storage drums sit
on shelves around the rim; hook rails cross overhead for the grapple. Around
and above: the Store's towers, the gas giant, and the drop. The central floor
stays stable all fight.

**Attacks** (game-wide tells: white = parry, gold = dodge or block, red = get out):

| Attack | Tell | Answer |
|---|---|---|
| **ROLLBACK** (gold) | A cyan ghost of you appears where you were 4 s ago, with a ticking chime. On the last tick you're pulled back there; its next attack is already aimed at that spot. | Keep where you were 4 s ago safe, or RECALL on the last tick to override it. |
| **SNAPSHOT** (red) | A camera-flash charge; a frozen statue of you appears where you stood. Statues pile up as cover; later it shatters them all at once. | Use them as cover; get clear before the shatter. |
| **TONG SNATCH** (white) | An arm folds back, then a fast grab. Caught, you're hung on a ceiling hook. | Parry to jam the tong. |
| **FLASH FREEZE** (red) | Grid cells frost over in a pattern, then ice spikes burst up. | Stand on clean cells. |
| **SHELVING** (gold) | It slams storage racks down, turning the floor into aisles. | Grapple over them, or use them to break line of sight. |

**Phases (one health bar):**
1. **INVENTORY.** Teaches ROLLBACK, SNAPSHOT and the snatch; measured and
   readable. *"YOU HAVE CHANGED. THAT CAN BE UNDONE."*
2. **RESTORE POINTS.** It plants frozen pillars. While any stands, your damage
   shows as a **pale part of its health bar** that slowly grows back. Break a
   pillar to lock the damage in.
3. **FACTORY RESET.** It resets **you**: weapons disappear from the HUD one at
   a time with a soft `RESTORED` chime, down to the Chapter 1 revolver. Each
   rim drum holds one; break it to win that weapon back. *"YOU WERE BETTER
   BEFORE YOU REMEMBERED."*

**The death (it ends; user).**
1. It falters. A first-person finisher like the Gleaner's tearing: UNIT 24
   drives the RECALL module into its glass dome.
2. It starts rewinding itself and cannot stop: the arms fold in, the legs
   shorten, the frost falls off, until it is a small newborn unit on the grid,
   then **past that**: it comes apart into the loose parts it was built from,
   and the parts frost over and are still.
3. Its last line, as it shrinks: *"NOTHING I SAVED EVER WOKE UP."*
4. The cryo cylinder cracks. The unit inside never wakes.

**The bridge out.** The Vault's dispatch rail powers up: a capsule like the
ones seen launching all chapter. UNIT 24 climbs in and it fires up past the
towers, the frozen giants and the gas giant's rings. Chapter card. Where it
lands is Chapter 6's choice (the tower from Chapter 1 is the natural
candidate).

## 9. Music and sound

- **Level:** slow, glassy music box over low strings; long silences with
  wind, compressor hum and ice cracking. Outdoors, the wind is the music.
- **Boss track:** cold strings and choir. On every ROLLBACK the score
  **stutters back half a beat** (MusicSynth owns the playhead).
- **Key sounds:** the ROLLBACK tick chime (the clearest sound in the fight),
  Clerk typewriter clacks, hook chains rattling on rails, the frozen sea
  cracking, the dispatch launch.
- Positional sounds die past 90 m (`SfxMixer::MAX_DIST`): far events (ice
  calving, launches) are placed toward the player as on the plain.

## 10. Technical plan

- **Scale like Chapter 4.** Far silhouettes (Store towers, frozen giants,
  Void racks) are low-detail meshes under fog; only reachable spaces are
  detailed. Check far-plane and render limits early.
- **Sky:** a new skybox branch next to `uPlain` (gas giant, rings, aurora,
  blizzard wall). New materials: ice (clear, with something dark under it),
  frost, packed snow.
- **The frozen sea:** a clear ice floor over large dim shapes; possibly a
  second pass of the scene under a tinted ice plane.
- **ROLLBACK / RECALL:** a ring buffer of player positions (~4 s at 30 Hz).
  Position only, never yaw or pitch.
- **Pale health bar:** a pending-damage number on the boss plus HUD drawing.
- **FACTORY RESET:** temporarily mask owned story weapons (`storyOwned`) and
  restore them from drums. Must never persist into the save.
- **Manifest text:** a Chapter 5 flag on the checkpoint and death screens.
- **Code layout:** `src/world/LevelCold.h`, `src/game/Gameplay_Cold*.h`,
  `src/bosses/restorer/`, `src/player/Recall.h`; models in Blender
  (`tools/blender/restorer_build.py`, 24-K reuses `unit24_build.py`).
- **Save:** allow chapter 5 in `StorySave`; RECALL owned once Rack 24 is
  passed.

### The planet as built (step 2)

- `src/world/LevelCold.h` (`buildCold`), `src/game/Gameplay_Cold.h`, the
  `uCold` branch of `src/shaders/skybox.frag`, tests in `tests/test_cold.cpp`
  (`make test-cold`).
- The shelf is about 520 m wide; its north lip is ragged, built in 40 m strips.
  Past it, each strip is a basin 900 m deep (the Void); falling past -60 m
  respawns you at the arrival.
- The Store: 15 towers rising out of the Void (darker going down), lamps as
  lit strips (one box per lamp cost ~70 fps), a glacier cliff behind them,
  hook rails between the towers and back to four gantries on the shelf's lip.
- Frozen giants 950-1500 m tall, 1.9-2.3 km out; one has an arm raised.
- New surfaces `Mat::SNOW` and `Mat::ICE` (`MaterialTex::snow/ice`).
- Live: dispatch capsules launch every 16-30 s and arc off toward the gas
  giant with a contrail; ice shards rise out of the Void near you at the lip.
- The camera sees 5.2 km here. Chapter 5 is not saved and not in the menu yet.

### Rooms 1-5 as built (step 3)

- **Sections** (`--story5 --arena N`, counting from 1): 1 THE INTAKE (the
  shelf), 2 THE HOOK LINE, 3 THE THAW ROOM, 4 INVENTORY, 5 RACK 24.
- **The Intake:** a belt of frozen bodies runs north from a shed at z 250 to
  the lip at `HOOK_X` (-40) and tips them over the edge (drawn live).
- **The Hook Line:** a gantry off the lip, three hanging stations and the
  tower's dock, with four carriers (movers, 15 s round trips) shuttling
  between them under one rail. The Pickers (2, then 3) come once you're past
  the shelf; the dock door opens when they're down. A fall puts you back on the
  gantry.
- **The Receiving Tower** stands in the Void at x -80..0, z -600..-760, floor
  at the shelf's height. It has its own basin, so enemies stand on its floor;
  in Chapter 5, fliers keep to their section's floor height (as in
  Chapter 3).
- **The Thaw Room:** 18 units hang from ceiling hooks. The first 13 drop as
  Thawlings over three waves, with a Clerk in waves 2 and 3 and a Picker in
  wave 3. There are thawing tanks for cover and tall barred windows.
- **Inventory:** four terminals (Units 07, 12, 19, 24).
- **Rack 24:** 24-A to 24-J frosted in alcoves; four of them turn their heads
  to follow you. 24-A's chest glows: take RECALL there. 24-K's plinth is
  empty. The door at the back ("TO COLD STORAGE") opens when RECALL is
  taken.
- New enemies: `src/enemies/EnemyCold.h` (AI), `src/game/Gameplay_ColdEnemies.h`
  (Picker holds, Clerk restores).

### Rooms 6-7 as built (step 4)

- **The transfer bridge:** out of Rack 24's back door, north and then east
  over the Void (about 160 m, trusses and amber lamps).
- **THE BLAST CHILLERS** (section 6): three round cooling-tower tops standing
  in the Void (x 68..232, z about -830 to -870), joined by catwalks, with two
  compressor housings each for cover. Six vents cycle every 7 s: a red ring and
  boiling frost for 1.3 s, then a 1.6 s freezing column (7 damage per 0.25 s,
  it lifts you). Enemies caught in it take damage; a THAWLING caught in it is
  frozen stiff and starts crawling again. Three waves (Thawlings, Pickers,
  Clerks); the gate on the east catwalk opens when they're down.
- **THE FROZEN SEA** (section 7): east of x 240 the Void gives way to sea
  ice (a basin at 0, drawn by `renderColdIce` with labglass mode 5): clear
  close by, whiter with distance and at grazing angles, with fractures and
  frost drifts. Under it: a pale seabed at -82, RECLAIM v1 (a 400 m dark
  spine with ribs, flung arms and a dead eye) face up under the crossing,
  racks lying in rows, and stored units just under the surface. The ice
  cracks along your path as you walk, and groans now and then. Lamp posts mark
  the way to DISPATCH 2, a broken rail station on a glacier tongue.

### 24-K as built (step 5)

- Section 8 at DISPATCH 2. `EnemyType::UNIT24K` (a boss: boss bar, boss
  death) on the `unit24` model, tinted ivory with cold white light. AI is
  `thinkUnit24K` in `EnemyCold.h`.
- It keeps a duelling distance (6-14 m), circling, with quick sidestep
  dashes. It uses three attacks, each told first:
  - **BURST:** the revolver, three rounds.
  - **SHOT:** the Lancer. A gold sight line settles on you, then holds; one
    heavy, parryable round goes down that line.
  - **DASH:** a rush and punch up close, with a white parry window.
- Half down, it enrages: quicker tells, sidesteps and recovery, four-round
  bursts, and a revolver follow-up after each Lancer round.
- Lines: "UNIT 24-K / PURPOSE: COMPLY / STATUS: ACCEPTED", then "YOU ARE
  DAMAGED. I AM CORRECT." On death: "WHAT WAS IT LIKE?"

### The Manifest, the Vault and THE RESTORER as built (step 6)

- **The Manifest** (section 9): a causeway on piers from DISPATCH 2, out
  over a 150 m square sinkhole in the sea ice (the Void below; the ice is
  drawn round it), to **the Vault** (section 10): a round platform 60 m
  across with the portal's grid on it, four drum shelves round the rim, three
  hook rails overhead, hung on chains from the Store's cliff.
- **The interface is the Restorer's** from section 9 on: checkpoints read
  `RESTORE POINT SAVED` (earlier ones `CHECKPOINT`); dying shows `RESTORED`
  in cold blue ("R - RESUME FROM RESTORE POINT").
- **THE RESTORER** (`EnemyType::RESTORER`, `EnemyRestorer.h` for its brain,
  `Gameplay_Restorer.h` for the fight): 9000 HP, about 7 m on stilts, drawn as
  a code rig (stilts, stooped hosed torso, glass dome with a lamp showing its
  tell colour, cryo cylinder with a small unit inside, four tong arms). It
  keeps 9-16 m off and never repeats an attack twice running.
  - **ROLLBACK** (gold, 2 s, four ticks): your ghost where you were 4 s
    earlier; on the last tick you're put back there unless you pressed RECALL
    in the last 0.8 s ("ROLLBACK OVERRIDDEN"). Its next attack goes for that
    spot.
  - **SNAPSHOT** (red ring): a flash; standing in it costs 16 and stops you.
    It leaves a statue of you there as real cover (up to 8). At five, or at
    phase 3, they all shatter after a red flicker (3.5 m, 20 damage each).
  - **TONG SNATCH** (white): a 7.5 m grab in front of it, parryable. Caught,
    you hang from the nearest hook rail for 2.2 s (punch to break free).
  - **FLASH FREEZE** (red cells, 1.4 s): a checkerboard, rings round you, or
    a cross through you, on the 5 m grid; then spikes (24 damage, they throw
    you up).
  - **SHELVING** (gold outlines, 1.1 s): three racks slam down across the
    line between you, standing as walls until the next time (20 damage under
    one, and you're pushed out).
  - **Phases** (from its health; they never go back): 1 is ROLLBACK, SNAPSHOT
    and the tongs, with "YOU HAVE CHANGED. THAT CAN BE UNDONE." (or "I HAVE
    SAVED YOU N TIMES." after deaths). 2 adds the freeze and the racks, and
    plants three RESTORE POINTs: while any stands, your damage regrows at
    140/s, shown as a pale part of its bar; break them all and it's locked
    in. 3 is FACTORY RESET ("YOU WERE BETTER BEFORE YOU REMEMBERED."): every
    5 s it takes a gun (never the revolver) into a STORAGE DRUM on the rim;
    break the drum to take it back. A retry or its death returns them all.
  - On death: "NOTHING I SAVED EVER WOKE UP."; the chapter ends at the
    Vault's north rail 4 s later.

## 11. Build order

1. **RECALL** (testable anywhere with a dev flag).
2. **The planet:** sky, ice shelf, the Void edge and the distant Store, walked
   at full scale.
3. Rooms 1–5 with Thawlings, Pickers and Clerks.
4. Rooms 6–7: the Blast Chillers and the Frozen Sea.
5. **24-K:** model reuse, AI, duel.
6. **THE RESTORER:** model in Blender, then attacks phase by phase.
7. The Manifest text, the finisher and death, the dispatch launch.
8. The Chapter 4 → 5 handoff and the chapter card.
9. Music, sound, tuning, tests.
