# Chapter 3: THE LIVING WARD — implementation notes

The chapter described in `chapter-3-design.md` now exists in the game. Start here
in a fresh conversation; the older build prompt records the agreed scope, rather
than work that still needs starting.

## Playing it

- Story menu → **3 THE LIVING WARD**, or `./shooter --story3`.
- Web build: `?story3`. The compiled build is in `web/dist`.
- F2 dev selection includes all ten checkpoints; these skip the introduction and
  include the appropriate guns/grapple. Boss select appends **THE SUCCESSOR** at
  index 9 without changing the earlier record indices.
- A normal Chapter 2 ending continues into this chapter with upgrades carried.
- Hold the bound Jump action to skip the 23-second first-person observation scene.
  Completing/skipping it persists the seen flag; retries and Continue do not replay it.

## What was built

Ten connected rooms descend by broad, real walking ramps: Admissions, Recovery
Ward, Motor Assessment, Surgical Observation, the Conservatory, Load Testing,
Long-Term Care, Celestial Observation, Hadal Research, Final Assessment. The
Conservatory is a quiet garden with planted islands and shallow water. Load Testing has a real pit and three broad
crossings; falling returns to its ledge. Early wards are tighter than the late observation halls, with open curtain bays,
drips and a nursing station. Every combat gallery has a walking ramp.
The boss galleries have a second exit as well, flanking a calibrated floor and
large assessment gantry.

The initial observation subject floats while its wall silhouette continues to
walk and the instrument reports floor contact. A window changes to incompatible
water while a stylized ward grid remains reflected. The camera eases into the
subject and returns to the normal gameplay view without a lateral jump.

The late wards have transparent observation glass, procedural nebula/trench skies,
large exterior fragments/cliffs with parallax, floating dormant bodies, a numbered
bed repeated in incompatible rooms, an exterior maintenance chair and tools, and a
patient beyond the celestial roof. Floors remain bright and gravity stays stable.
There are optional medical records throughout, plus two numbered secret wrecks.

The Motor and Suspension controls are on walkable galleries. Celestial and Hadal
controls choose a west or east bridge. The selected slab actually rises into the
floor gap; the other stays lowered. Choices and seen records persist. Fixed side
walkways remain available. Contact markers appear when the assessment floor is clear.

Four new articulated enemy families have original GLBs, profiles, animation and AI:

- Patient: committed brace-release lunge; guns can interrupt its poise.
- Grafted: a heavy support plate blocks frontal body shots; head/flank shots work.
- Suspended: an open cage distinguishes an active enemy from dormant bodies; its
  beam commits to a lane before firing.
- Phase Subject: its exit is marked before crossing, with a separate lunge tell
  after arrival.

The Successor is a 2.65 m flesh chassis with a mechanical face. It has one health
bar (8500 base HP) and three phases at 65%/30%, original subtitles and intelligible
synthetic speech. Incision, Vector Run, Suture, Transposition, Recall, Perfusion
and late Double Exposure use independent state/events/hazards. Vector commits its
predicted lane; crossing announces its exit and cannot deal contact damage in
transit. A white fist parry cancels pending follow-ups/hazards, heals a bounded 8
before difficulty scaling, and exposes the drive. Shooting either Suture anchor
also interrupts it. No ordinary add wave or Story sword is required. Two health
pickups are placed in the arena. The final sequence stays damageable.

Boss defeat clears its live hazards/projectiles and opens a broad corridor over
empty space to a little impossible garden. The player walks through this fractured
threshold to finish the chapter; Chapter 4 is outside this implementation.

## Audio and art sources

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
phase changes and hazard timing. The lab bake is checked against current geometry.

`make test-dev` exercises real menu/checkpoint inputs, all 27 Story checkpoints,
the live lab skip/retry path, actual contact callbacks/mover collision, a real
revolver shot cutting a Suture node, a real fist parry, boss defeat and the ending.
`./shooter --verify-story` checks the Chapter 1 → 2 → 3 handoff and ending/menu path.
Native rendering regressions and `make web` pass. `make test-web-story` also runs
all three Story boss file rigs in real WebAssembly with a 32 KiB stack.

A reproducible rehearsal with actual acceleration, collision, occluded revolver
shots, ammo/reloads and 40% scripted hit accuracy takes about **155 seconds** on
Standard, sees all seven attacks and lands eight parries. The reactive policy
accumulates about 124 damage versus 468 for steady circling over a full rehearsal;
the reactive policy remains alive. These are scripted tuning instruments, not a
human playtest or a promise about every player's kill time/upgrades.

Representative native captures are under `build/lab-*.png`. The Hadal scenery
benchmark at 720p with 4x MSAA averaged about 3.12 ms/frame on this machine; that
is a short local scenery benchmark, not browser/mobile or worst-case combat data.

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
