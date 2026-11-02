# Chapter 3: THE LIVING WARD — implementation notes

The chapter described in `chapter-3-design.md` now exists in the game. Start here
in a fresh conversation; the older build prompt records the agreed scope, rather
than work that still needs starting.

## Playing it

- Story menu → **3 THE LIVING WARD**, or `./shooter --story3`.
- Web build: `?story3`. The compiled build is in `web/dist`.
- F2 dev selection includes all ten checkpoints with the appropriate guns/grapple.
  Admissions plays the introduction, including in practice; later checkpoints skip it.
  Boss select appends **THE SUCCESSOR** at
  index 9 without changing the earlier record indices.
- A normal Chapter 2 ending continues into this chapter with upgrades carried.
- Hold the bound Jump action to skip the 23-second first-person observation scene.
  Completing/skipping it persists the seen flag; retries and Continue do not replay it.
  Release Jump after selecting the chapter before holding it to skip, so a held
  menu Space cannot dismiss the scene accidentally. Select a new Chapter 3 run
  to replay an introduction already marked seen in Continue.

## What was built

Ten connected spaces descend by broad, real walking ramps: Admissions, Recovery
Ward, Motor Assessment, Surgical Observation, the Conservatory, Load Testing,
Long-Term Care, Celestial Observation, Hadal Research, Final Assessment.
The November map pass replaces straight connectors with two flights and a
lateral landing. Early transfers are enclosed; later transfers use low rails
and exposed canopy frames. Each passage has its own fall limit, twelve metres
below its lowest floor. Previously, a fourteen-metre descent crossed the owning
room's twelve-metre fall limit and could reset the player on safe ground.

Room floors and perimeter walls now follow offset, stepped footprints: Recovery
turns between two wings, Motor Assessment has a U gallery, and Long-Term Care
has an upper crossing beneath suspended structures and a split roof. Celestial
and Hadal floors split around open wells; Hadal also has an eight-metre shelf
with ramps at both ends. Heights grow from 12 to 72 metres before the final arena opens completely. Late side piers and
open end arches reveal the surrounding sky without a low glass roof or giant
opaque end wall. Final Assessment is a separate 152 by 112 metre exposed
platform with clipped corners, low rails and two eight-metre galleries, each
with an approach and return ramp. Boss movement and committed vectors check
physical floor support rather than treating the arena bounds as solid ground.

The
Conservatory is a quiet garden with planted islands and shallow water. Load Testing has a real pit and three broad
crossings; falling returns to its ledge. Early wards are tighter than the late observation halls, with open curtain bays,
drips and a nursing station. Every combat gallery has a walking ramp.
Broken orbital frames rotate above the calibrated boss floor. Split pylons and
suspended arches surround it, with a lensed void and fractured accretion bands
in the nebula beyond. The exterior shifts toward crimson as the boss loses health.

The initial observation subject floats while its wall silhouette continues to
walk and the instrument reports floor contact. A window changes to incompatible
water while a stylized ward grid remains reflected. The camera eases into the
subject and returns to the normal gameplay view without a lateral jump.

The late wards have transparent observation glass, procedural nebula/trench skies,
large exterior fragments/cliffs with parallax, floating dormant bodies, a numbered
bed repeated in incompatible rooms, an exterior maintenance chair and tools, and a
patient beyond the celestial roof. Surgical Observation has a glass bay overlooking
dark water and a sealed, unsupported observation booth whose chair faces the player.
Trolley C-17 recurs in the Conservatory and Hadal Research with different damage.
Floors remain bright and gravity stays stable.
Exteriors, room tone and the boss score follow the room physically occupied;
clearing a fight cannot turn the nebula into the trench ahead of its doorway.
There are optional medical records throughout, plus two numbered secret wrecks.

The Motor and Suspension controls are on walkable galleries. Celestial and Hadal
controls choose a west or east bridge. The selected slab actually rises into the
floor gap; the other stays lowered. Choices and seen records persist. Fixed side
walkways remain available. Contact markers appear when the assessment floor is clear.

Four new articulated enemy families have original GLBs, profiles, animation and AI:

- Patient: light flanking hunter with a committed brace-release lunge. White
  braces and active rushes can be parried; a missed or blocked rush exposes it.
  A revolver hit can break its poise and set up a finisher.
- Grafted: centre-holding heavy with a frontal support plate. Head/flank shots
  work, its gold six-metre press can be jumped, and its slow heavy orb can be
  returned for 230 damage and a poise break. It opens after either attack.
- Suspended: an open cage distinguishes an active enemy from dormant bodies.
  Its visible purple charge tracks initially, locks with a tick, then sweeps a
  fixed arc. The live ray and rendering share a wall-clipped endpoint. Shooting
  its charging/firing core interrupts the sweep and exposes it. Flyers initialize
  above the actual ward floor, avoiding an abyss-derived 250-metre hover offset.
- Phase Subject: a cyan exit silhouette appears in front of the player before
  crossing, with a separate white lunge tell after arrival. Exit selection checks
  body clearance and support, then rechecks before arriving. Stun cancels it.

The enemy pass lowers ordinary lab health and poise, rewards exposed recoveries
with extra damage and visible cyan cores, and gives the bodies distinct charge,
brace and stun poses. Encounters put Patients on galleries and a Grafted on the
Long-Term Care crossing; later fights mix light hunters with a few elites.
The shared attack scheduler counts active rushes, crossings and beams, lets
recovering enemies yield their slots, and considers readiness and time waiting.
Free Practice includes all four lab types and a **WARD SQUAD** selection. Use
brackets or controller X to cycle targets; the squad respawns after defeat.

Gun poise breaks and staggers cancel their pending lunge, beam and crossing; attacks
do not resume after a stun. Headless commitment/counter checks and native combat
tests cover all four families.

The Successor is a 3.8 m augmented chassis with an exposed segmented glowing
spine, three dorsal vanes, asymmetric shoulder housings, a split black visor,
and branching red power channels over black arm reinforcements. Its dedicated
18-channel spring rig drives a planted guard, running footwork, alternating
cuts, advancing thrust, low spinning sweep, crouched vector launch and airborne
breaker. Anticipation, hip/torso counter-rotation, follow-through and interpolated
spring state are independent of the ordinary subjects' walk cycle.

It has one health bar (9800 base HP), with phases at 70%/35%. It pursues at 22–26 metres per second, closes distance
during early melee tells, tracks predicted targets, then commits for the final
0.2 seconds. Later Vector Runs reacquire a second marked lane before dashing
again and can finish in a white thrust. A gold nine-metre sweep can be jumped;
a red aerial breaker locks its landing, travels without contact damage, and
punishes players who remain inside its 4.8-metre marker, including jumpers.
Low gallery ceilings substitute a crossing for the aerial move. Announced
crossings answer vertical separation in either direction and give a separate
white arrival tell. Perfusion gains follow-up projectile fans. Suture, Recall
and late Double Exposure retain their independent interruptible hazards.

White fist parries can meet the advancing arm before the torso enters ordinary
fist range; they cancel pending movement, volleys, nodes and follow-ups, expose
the drive for 1.1 seconds, and heal a bounded 8 before difficulty scaling.
Shooting either Suture anchor also interrupts it. Recovery footwork flows straight into the next attack without a second idle timer.
Faster pursuit and an aggressive final sequence keep pressure up without removing
committed tells. No ordinary add wave or Story sword is required. Two health
pickups are placed in the arena. The final sequence stays damageable.
Spoken lines play once per run, including checkpoint retries; a full restart resets
delivery. A retry after victory keeps the boss defeated and the threshold open.

Boss defeat clears its live hazards/projectiles and opens a broad corridor over
empty space to a little impossible garden. The player walks through this fractured
threshold to finish the chapter; Chapter 4 is outside this implementation.

## Enemy character and swarm pass

The four ward subjects were rebuilt for character and aggression.

- Models (`tools/blender/lab_ward_subjects.py`, exec'd by `lab_subjects_build.py`;
  the Successor's build is unchanged): gaunt anatomy with a ribcage, knuckled
  joints, clawed fingers and a hinged jaw full of teeth. New joints: `jaw`,
  `clothF/clothB` (gown flaps), `ribbonL/R` (straps, sleeves, IV line).
  Patient: torn gown, restraints, cracked half-mask with one lit eye, stapled
  skull. Grafted: splinted graft arm, riveted chest brace, respirator, IV bags
  over its back core. Suspended: straitjacket, bandaged head, hoist and open
  cage. Phased: split by a glowing fracture, three eyes, orbiting shards.
- Animation (`src/render/LabSubjectPose.h`): per-type gaits with arms
  opposing legs (the old shared cycle swung same-side arm and leg and bent
  knees backwards), twitches, jaw, trailing cloth and straps, and blended
  attack poses: crouched shriek and airborne pounce, raised claw rake,
  overhead press, beam convulsion, phase tearing, reeling and exposed slumps.
- Swarm AI (`EnemyLab.h`): Patients and Phase Subjects sprint in weaving
  (110% speed beyond 8 m), then each takes its own golden-angle slot three
  metres round the player and prowls there, so packs close as a ring. Up
  close they use the new `LAB_SWIPE` claw rake (0.5 s tell, white parry window
  in its last 0.2 s, 9 damage, narrow arc). The mid-range lunge is unchanged.
  The attack scheduler still limits simultaneous attackers by difficulty.
  The Grafted now trudges after the player instead of holding the centre.
- Patient health 84 -> 64; most encounters gained one or two Patients.
- `--viewer patient|grafted|suspended|phased` previews the subjects;
  F winds up an attack, N cycles attacks, W toggles the run.



Four concept boards (a fractured white-tile lab in a violet/teal void) set the
art direction. `src/world/LabDressing.h` builds it per room after each room's
geometry; `src/game/Gameplay_LabDressing.h` animates the moving parts.

- Floors are 1 m bevelled white tiles with dark grout and occasional cracks;
  walls are 1 m porcelain tiles (`MaterialTex::lino/clinic`).
- Early wards: specimen tanks (swirling two-colour currents and bubbles) and
  server racks against the walls, ring pads under the health pickups, floor
  circuit traces and red/cyan beacons at each exit, ceiling lamps with
  additive light shafts, sagging cable runs. Roots split the walls of Surgical,
  the Conservatory and Long-Term Care; debris drifts in Long-Term Care's nave.
- Late wards: lit rails, fractured slab edges with rebar and hanging cables,
  a dark service deck under every floor, floating lab islands (Celestial) and
  tumbling racks, doors, tiles and monitors outside.
- Final Assessment: four islands whose tanks arc power to emitter posts on the
  rail, two hovering pads, and a 42 m swirling portal in a cabled frame ahead
  of the arena (it reddens with the boss's damage). The sky's lensed void moved
  up and left so the two circles no longer overlap.

Placement is gameplay-safe: shapes are visual; tank/rack collision is only
placed where `clearAt` finds floor under every corner and no door, ramp,
gallery, spawn, encounter point, terminal, pickup or walking lane nearby.
Every random draw is its own statement, because argument evaluation order
differs between Clang and GCC and would change the bake hash on Linux.
After editing the dressing, rebuild the baker before baking:
`make tools/light_bake && ./tools/light_bake story3`.



`OBSERVATION` and `RELEASE DENIED` are original procedural tracks in
`src/audio/MusicSynth.h`. The latter is 176 BPM with four formant-filtered vocal
parts, moving vowels, an eight-bar soprano answer, organ harmonics, bass and fast
percussion; its intensity follows the boss phases. New room-tone recipes preserve
medical ventilation under the incompatible exteriors. Attack tells have distinct
procedural cues and remain separate from dialogue.

`tools/gen_lab_speech.py` rebuilds the original dialogue/PA recordings with eSpeak
NG and ffmpeg. They are intentionally synthetic voices. `MixTable.h` contains
measured trims for the loudest 50 ms of each recording. The wordless enemy voice
bank uses fewer cosmetic lab variants to keep startup memory bounded.

`tools/blender/lab_subjects_build.py` builds the five new enemy GLBs. Secret wrecks
18 and 21 use the existing UNIT builder with their own numbered chest stencils:

```sh
blender -b --factory-startup --python tools/blender/headless.py -- tools/blender/lab_subjects_build.py
# Rebuild only the boss:
blender -b --factory-startup --python tools/blender/headless.py -- tools/blender/lab_subjects_build.py ONLY=successor
blender -b --factory-startup --python tools/blender/headless.py -- tools/blender/unit24_build.py NUMBER=18 OUTNAME=unit18
blender -b --factory-startup --python tools/blender/headless.py -- tools/blender/unit24_build.py NUMBER=21 OUTNAME=unit21
python3 tools/gen_lab_speech.py
make tools/music_preview
./tools/music_preview 13 24 build/chapter3-release-denied.wav 1.3
```

The rendered score preview is `build/chapter3-release-denied.wav`. No third-party
song recording is embedded in the game.

## Validation and limits

`make test` includes `make test-lab`: continuous traversal with the real controller,
checkpoint clearance, both gallery directions, authored spawn capsules, finite
encounters, actual bridge collision, playable ending, cinematic clock/skip,
version-one save migration, new model/head bounds, committed attacks, cancellation,
phase changes and hazard timing. The lab bake is checked against current geometry. All four maps also pass the
closed-face/underside validator; shared vertices are welded by a 1 mm distance
tolerance so a coordinate-bin boundary cannot produce a platform-specific false gap.

`make test-dev` exercises real menu/checkpoint inputs, all 27 Story checkpoints,
the live lab skip/retry path, actual contact callbacks/mover collision, a real
revolver shot cutting a Suture node, a real fist parry, boss defeat and the ending.
`./shooter --verify-story` checks the Chapter 1 → 2 → 3 handoff and ending/menu path.
Native rendering regressions and `make web` pass. `make test-web-story` also runs
all three Story boss file rigs in real WebAssembly with a 32 KiB stack.

A reproducible rehearsal uses actual acceleration, collision, occluded revolver
shots, ammo/reloads and 40% scripted hit accuracy. It reads committed lanes,
leaves breaker markers, jumps the low sweep and approaches white thrusts to
parry. All three tested opening clocks (0%, 30%, 60% of the attack interval)
survive on Standard, taking roughly 154–158 seconds. The expanded fight exercises
nine attack kinds; steady circling takes substantially more damage than the
reactive policy. These are tuning instruments, not a human playtest or a promise
about every player's kill time/upgrades.

Native regression captures include `build/successor-front.bmp`,
`build/successor-spine.bmp`, the individual attack poses and
`build/successor-arena.bmp`. Headless tests cover repeated dash warnings,
locked airborne landings, gallery pursuit, floor-edge safety, pose springs and
hazard height. Native tests cover the extended fist counter, jumping the live
sweep, the live breaker landing hazard and the phase-responsive exterior.

No in-app browser is connected in this environment, so interactive browser
playtesting remains a manual follow-up. Audio input is unsupported here: the
actual score was rendered and checked for finite output/levels, and a listening
preview is supplied, but human musical/voice quality review is still needed.
Prioritize a human end-to-end playtest for navigation, boss feel and audio before
calling the chapter finally balanced. The implementation is complete; tuning can
continue from this version.

Save storage keeps the existing key and reads both versions 1 and 2. Chapter 3
adds the intro-seen and route-choice fields. Capture/practice/boss runs cannot
replace the Story Continue slot. The existing six remote boss leaderboard IDs
remain unchanged; appended bosses use their local records.

The release audit also widened Chapter 1's Scrap Pit receiving ledges and moved
its traversal rehearsal's launch positions farther inside each crate. The eight
1.4 m rises remain; normal jumps now have forgiving landing room on both macOS
and Linux. `story1.light` and `story3.light` were rebaked after the geometry edits.

See [the verification record](chapter-3-verification.md) for the requirement/evidence
map and the remaining human review limits.

Web releases now carry `build-info.json`, a matching loader identifier and versioned
JS/WASM/data URLs. Sync the adjacent website with `scripts/sync-overdrive.sh`,
verify it, and deploy it; `make web` alone does not update the public website.
