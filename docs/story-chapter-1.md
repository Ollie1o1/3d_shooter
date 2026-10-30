# THE CRADLE — Chapter 1

OVERDRIVE's story branch begins with UNIT 24 waking on the reject line. The
chapter follows a continuous route through nine rooms, then opens onto the ash
wasteland. Lore remains optional, terse text; the later story's final purpose is
still open.

The current writing rules and encounter revision are documented in
[Story revision](story-overhaul.md). Wreck records now describe specific events;
the repeated mixed-wave fights have been replaced with distinct encounter beats.

## Route and intent

| Room | See, do, learn, find |
| --- | --- |
| Furnace Line | A first-person optics and motor boot, moving rejects, the gantry claw and furnace. Step off the belt, find the damaged REJECT TRANSFER / SALVAGE hatch beside its feeder rollers. No gun. Extra service plates, overhead trays, work lights and drain lettering frame the route. |
| Scrap Pit | Fall through the damaged reclamation hatch and its transfer tunnel onto discarded siblings. 78 discarded bodies, broken casing, overflowing collection bins and low work lights tell the room's history. UNIT 01's lamp and revolver wake one short fight. Climb the eight heaps and crates to the duct exit; lit depth markings and DUCTS lettering make the destination visible. |
| Maintenance Ducts | A locked junction fight, pipe tanks and side ledges. Slide/crawl under the low pipe run, take UNIT 02's Lancer, then board the automatic service lift to Assembly. UNIT 03 is hidden behind the junction vent. |
| Assembly Hall | Three working assembly lanes and nine unfinished siblings below the catwalks. Drop to the production floor for a catwalk crossfire followed by one heavy clean-up. Climb the broad staircase to the west exit. UNIT 04 records a failed attempt to stop a production line. |
| Archive | A quiet change of pace. Server racks, terminal screens and the wall register: UNITS 1–23 DECOMMISSIONED, UNIT 24 REJECTED. Three optional single-line records, UNIT 05, then west to the vault. |
| Pod Vault | An entry balcony, twelve glass-fronted pods and a staircase down. Cross into the far half: the fronts shatter into one coordinated containment breach. UNIT 06 holds the shotgun. Side trial stations foreshadow MEASURE and KEEP. A high service vent leads to UNIT 07's observation cell, the second secret. |
| Elevator Shaft | A freight deck rises 39 metres, pausing at three landings until their defenders are gone. Walkers hold the ledges while fliers drop in. Falling off returns the player to the platform. The top gate stays shut until the final wave clears. |
| Inspection Line | Six scanner gates oscillate vertically. Slide or time a crossing; a mark calls collectors, while a clean crossing stays quiet. Cover bays vary the corridor's width. Find the grapple beside UNIT 08 and see the Sorter waiting beyond the JUDGE gate. |
| Sorting Floor | A rail-mounted inspection machine above nine belt panels, cover supports and overhead grapple rails. Three phases reverse and accelerate the belts; the final phase warns, then releases four corner panels to the furnace. The central cross and perimeter remain. |

## The Sorter

`UNIT ? · SORTER · PURPOSE: JUDGE`

- **INSPECTION**, 100–66%: measured tells introduce its scanner, stamp, claw and body throws.
- **RECALIBRATION**, 66–30%: faster attacks, reversing belts.
- **INCINERATION**, below 30%: faster again, a 2.2-second floor-release warning,
  four panels drop, belts speed up. Rails offer grapple escape routes.
- **White claw**: punch during its last quarter-second at the marked claw point.
  It stops, clears your scanner mark, and takes double damage while staggered.
- **Red stamp**: leave its marked circle. The target stops tracking before
  impact. Its recovery exposes the core for double damage.
- **Scanner**: a visible sweeping ray, occluded by walls. It marks rather than
  directly damaging. Break sight or parry to clear the mark. Marked stamps track
  longer, but always commit before release.
- **Rejected bodies**: shoot them apart, dodge, or parry them back for a heavy hit.
- Range and elevated positions receive tracking stamps; a dash remains the answer.
- Death: `THE REGISTER SAYS YOU WERE EMPTY. / THERE IS NO COLUMN FOR THIS.`

The gate opens when the fight clears. Walk out to the red sky and the creator's
tower, then reach the chapter card. It shows time, kills, secrets, style, deaths
and rank. Enter returns to the menu. The two secrets are UNIT 03 and UNIT 07;
ordinary weapon wrecks do not count as secrets.

## Source map

- `src/world/LevelStory.h`: original furnace, pit, ducts and their collision.
- `src/world/LevelStoryChapter.h`: upper-factory geometry, encounters and dressing.
- `src/game/Gameplay_Story.h`: wake-up, inventory, wrecks, room cards and ending.
- `src/game/Gameplay_StoryFactory.h`: pod breakage, freight lift, scanners, Sorter
  environment, body throws and preview machinery.
- `src/bosses/sorter/`: independent, headless Sorter state and AI.
- `src/render/EnemyModel.h`: animated Sorter pose and scanner-eye hitbox.
- `tools/blender/sorter_build.py`: the Sorter's Blender model recipe.
- `tools/blender/unit24_build.py`: UNIT 24 and numbered wrecks.
- `tests/test_story.cpp`, `tests/BossSim.h`: real collision traversal, encounter
  progression, light-bake validity and boss policies.

Room shells continue the existing C++ level-builder approach. Character meshes
are authored in Blender. `make level-dump` exports the complete level for
`tools/blender/level_import.py`; the scene can be opened and saved in Blender.

The original three-room implementation was uncommitted at handoff; its robot
hands, wake-up and weapon ownership work are included with the completed chapter.
The recent main-line commits covered sword-hand poses, the ARENA/STORY menu split,
the Blender/light-bake pipeline, mouse capture and faster world texture loading.

## Developer starts and verification

```sh
make
make light
make test
./shooter --story
./shooter --story --overlay ducts --record 1 120 /tmp/ducts
./shooter --story --overlay assembly --record 1 4 /tmp/assembly
./shooter --story --overlay archive --record 1 4 /tmp/archive
./shooter --story --overlay vault --record 1 4 /tmp/vault
./shooter --story --overlay lift --record 1 4 /tmp/lift
./shooter --story --overlay inspection --record 1 4 /tmp/inspection
./shooter --story --overlay sorter --cam 72 20.5 8 -70 0 --record 1 180 /tmp/sorter
./shooter --story --overlay sorter2 --cam 72 20.5 8 -70 0 --record 1 180 /tmp/sorter2
./shooter --story --overlay sorter3 --cam 82 29 -15 -70 -20 --record 1 180 /tmp/sorter3
./shooter --story --overlay sorterend --record 1 4 /tmp/ash
./shooter --story --overlay sorterendcard --record 1 120 /tmp/end
```

`--cam` pins the camera for composition; world scripts still advance. The
headless traversal uses actual Player movement and collision, including the lift's
normal carry delta. Developer overlays grant the gear needed for that checkpoint;
a normal run always begins unarmed. `pit`, `pittake`, `chute` and `wakeNNN` also remain.

Web release follows the adjacent Personal-Website repo's documented workflow:
`make web`, copy `web/dist` with its `scripts/sync-overdrive.sh`, run `npm run verify`,
commit and push the website's main branch. Do not add story scores to arena boards.

## Verified release

- `make`, `make light`, and `make test` pass. The physics, game, Zenith and Chapter 1 suites run from the normal test target.
- Every checkpoint, the full original pit climb, duct slide, both secret vents,
  the duct-to-assembly hatch, upper stairs and doors, freight-platform ride and
  exit to the wasteland are exercised with the real Player controller.
- BossSim's scripted gun policy beats the revised Sorter in about 125 seconds; idle, corner,
  elevated and ranged camping are punished in every phase. This simulation
  measures AI and damage behavior; it is not a substitute for human balance feedback.
- Native screenshots were inspected for all nine rooms, the waking hand, both
  later boss phases, dropped floor panels, the archive register and the ending.
- Assembly Hall native benchmark at 720p on this development machine: 2.49 ms
  average, 2.99 ms 1% low. This is one local view, not a browser performance claim.
- The complete level exports and opens in Blender. The local scene is
  `build/levels/story1/cradle.blend` (generated, not versioned).
- The WebAssembly release builds. No Browser session was connected for an
  interactive web playtest; native visual checks and build verification cover
  this release, with production artifact verification after deployment.

[Archive register](story-chapter-1/archive.png) · [Sorter, floor released](story-chapter-1/sorter.png) · [Ash exit](story-chapter-1/ash.png) · [Results](story-chapter-1/results.png)

Chapter 2 continues beyond the ash exit through [the Dead Span](story-chapter-2.md),
with Ballast as its road-clearance sibling. The creator's final answer and UNIT 24's
ultimate purpose remain open. Human feedback should guide further difficulty and art tuning.

The release also fixes the older maps' cross-compiler bake fingerprints. Format
5 hashes procedural shape recipes and file-space prop geometry before floating-
point triangle culling, and handles rounding ties consistently. Dome apices close
at an exact point. All four map fingerprints agree between Clang/ARM macOS and
GCC/x86 Ubuntu; the full four-suite test target passes on both. All map bakes are
regenerated together when the format changes.

## Chapter polish: October 29, 2026

All wreck logs and terminals now require the interaction key (E by default,
R3 on a controller). Floating cyan lamps mark them. Looking at one in reach
shows a prompt; approaching alone opens nothing and awards no secret. E again
on the same record closes it; walking away also closes it. Read wreck lamps dim.
Weapons remain separate pickups. Solid walls occlude prompts and interaction.

The Lancer shaft now has an automatic service lift: a 4.5-second ascent, a flush
exit to Assembly, and an automatic return when a player waits below. The freight
lift's floor is recessed beneath its deck, and boarding detection also checks
foot contact so a flush seam cannot prevent the encounter starting. The ascent
lasts 15 seconds of movement, plus time fighting at its three stops. Both lifts
use the same controller in gameplay and the Player regression tests.

Four new Blender models supply the damaged reject hatch, scrap clusters,
salvage bins and switchgear. Rebuild with:

```sh
blender -b --factory-startup --python tools/blender/headless.py -- tools/blender/cradle_props_build.py
```

Editable prop scenes are saved to `build/cradle-props/`. Their GLBs are versioned
under `assets/props/`. Dressing extends to Assembly, Archive, the Pod Vault and
the freight landings. The pit also has oil marks, girder supports, periodic dust
from the reclamation inlet and occasional machinery sparks. The fallen crane's
collision follows its narrow angled beam rather than enclosing empty space.

Design decisions and research: [Chapter 1 polish notes](chapter-1-polish.md).

Room-by-room atmosphere and detail pass: [review and native previews](chapter-1-room-pass.md).
