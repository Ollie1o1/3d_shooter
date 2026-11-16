# Chapter 6: COUNTERWEIGHT (2026-11-14)

This pass supersedes the vent encounter and the floating-head finale described
in `chapter-6-build.md`. The user's direction is a monumental humanoid with a
Dr Manhattan-like face and the physical scale of the Corpse of King Minos,
with a mechanic specific to the Shepherd and the ring.

## The fight

The Shepherd holds every dispatched unit in orbit. Its own body is subject to
that same rule. When a palm lands, UNIT 24 can **pin it to the hull with
HOLDFAST**: the captive orbital mass recoils into the Shepherd's heart.

The boss is a near-black human colossus with buried blue undertones, with a sculpted bald face, eyes,
nose, mouth, ears, chest, shoulders, articulated arms and jointed hands. Three
inclined orbital cages surround its head and heart. They break apart through
three movements of the fight. The arena's tall north wall has been lowered so
the chest and arms remain visible. Raised side decks give another route over
the sweeping gaze; the fins remain useful cover.

- Face and torso take damage throughout combat. There is no compulsory
  attack / wait / vulnerability cycle.
- Grounded palms take 1.8x damage. HOLDFAST pins one for 5.5 seconds; after a
  visible 0.9-second counterweight flight, the impact deals 150 damage and
  exposes the whole body to 1.6x damage for five seconds.
  Pinning is optional; ordinary fire can finish the fight.
- A white flash in the final 0.27 seconds of a fist's wind-up permits a punch
  or sword parry. A successful parry deals 120 damage, heals and grounds the
  hand. Otherwise leave its red footprint and jump the aftershock.
- Five white moons cast from the halo can be parried back. Returned moons
  home into the heart. The boss no longer needs ordinary Strays to sustain
  its combat loop.
- The second movement introduces **memory palms**: three recorded player
  positions, each with a full 1.8-second warning before its impact. Ground
  circles and descending spectral palms show exactly where they land.
- The final movement begins with a gravity gather. HOLDFAST to the hull
  counters the pull; its ground rings can be jumped. It combines this with
  paired fists, memory palms, the gaze, reflected moons and the clocked cone.
- A lethal cone gets a full one-second flash and four-second countdown.
  Its ground markings now render at the arena's actual height.
- Damage stops at each third so burst damage cannot skip a movement.
  Retries restart that third. Defeating the body survives a finisher retry.
- The final count gives 24 seconds to reach and pin the heart housing. The
  existing internal detonation and drifting capsule ending remain; the
  humanoid sinks as it detonates.

The sword, projectile weapons, splash damage and Splinter now reach the
Shepherd's targets. Reflected moons and fast player projectiles are tested
before their normal movement step to prevent tunnelling. Palm pins use the
same nearest-hit limit as walls and nearer enemies.

## The approach

**THE ORRERY** replaces the ridge's three stationary vents. Three moons travel
an elliptical orbit through the combat space. They are always exposed; pin
one to stop it for five seconds and increase damage against it. Each broken
moon accelerates the remaining orbit. Locked ground strikes can crush the
flock as well as the player. An initial Herder / Porter pair and reinforcements
on moon destruction replace the repetitive vent-opening Stray spawns.

The six-second lethal ridge sweep is retained. A local strike must finish
before a lethal countdown can begin. Moons keep their damage on retry.
The turn before this encounter takes five seconds instead of sixteen.

In the Tuning Hall, breaking a fork transfers its resonance: the remaining
cores stay open for a fresh window and the next blast is held off long enough
to chain another break. The hall fields a Herder alongside its Stray, rather
than repeating pairs of basic enemies. HOLDFAST remains the fast way to
silence a ringing fork.

The ascent's three goals are to break a mooring with the Shepherd's strike,
stabilise a moving relay with HOLDFAST, then board the last Gather under fire.
Each has a finite, distinct guard composition. The Shepherd strikes the ledge
during its fight; its warning locks to the old player position, so these blows
can also be baited into enemies. The Gather rides remain the transport between
ledges.

An existing update-loop bug prevented the boss's ground rings from advancing.
The Shepherd path now ticks those rings while keeping crossing-only Wake
set-pieces disabled. The ring movement is covered by a runtime regression.

## Implementation and validation

- `src/bosses/shepherd/ShepherdCombat.h`: shared timing, anatomy bounds,
  face pose, phase sequencing, checkpoint health and moon trajectories.
- `src/game/Gameplay_ShepherdHead.h`: body combat, pins, parries, projectile
  interception, Splinter targeting and the live colossus rig.
- `src/bosses/shepherd/ShepherdModel.h`: Blender-authored anatomy on the live combat skeleton.
- `src/render/HeadMesh.h`: small VFX primitives only.
- `Gameplay_Shepherd.h`, `Gameplay_ShepherdBreak.h`, `LevelShepherd.h`: the
  orrery, resonance chains, ledge assaults and final arena sightlines.

Arena indices and Chapter 6 checkpoint numbers remain unchanged. The internal
`shepVent*` names now store the orrery's progress to avoid unnecessary changes
to the rest of the chapter flow. `assets/levels/shepherd.light` is rebaked.

`make test-shepherd` checks mesh normals and triangle winding, phase retries,
attack footprints, target trajectories, checkpoint compatibility and the
ascent encounter structure. `make test-shepherd-runtime` creates a native GL
context, tests actual combat inputs and transitions, and writes frame captures
to `build/shepherd-pass/`. The ordinary `make test` suite now includes the ring
and Shepherd logic checks.

Preview the finale with `./shooter --shepherd --arena 11`; preview the orrery
with `./shooter --shepherd --arena 4`; start the complete boss approach with
`./shooter --shepherd`. These checks do not replace a human balance playtest;
attack timing, health and counter rewards are initial tuning values.

Completed verification for this pass: `make test` (full regression suite),
`make test-shepherd`, `make test-shepherd-runtime`, native `make`, and
Emscripten `make web` passed. The native runtime checks exercise 50 combat,
transition and render cases, including the normal aftershock update and a
sword already inside the palm volume. Final frame captures were inspected.

## Blender model revision (2026-11-14)

The user's art correction supersedes the initial smooth blue body: the Shepherd
should have adult muscular anatomy and the darkness of its original spacecraft.
Its head and body now come from seven Blender-authored GLB pieces, rather than
procedural head/ellipsoid anatomy. The model has a broad angular mandible, lean
cheeks, a heavy brow with recessed almond eyes, a smaller head relative to the
shoulders, wide lats, flat pectorals, and a tapered abdominal column. Black hull
strata cover the temples, jaw rails, shoulders, ribs, forearms and hands. Three
long swept crown keels and miniature capsule docks recall the original vessel.
Blue light is concentrated in the eyes and fine seams, leaving the armor dark.

Build with `make shepherd-art` (`BLENDER` overrides the executable). Source:
`tools/blender/shepherd_build.py`. Runtime pieces: `assets/props/shepherd_*.glb`.
An assembled editable scene and two studio renders are written to
`tools/blender/scenes/shepherd.blend`; the two studio renders are
`build/shepherd-art/shepherd-bust.png` and `shepherd-face.png`.
The runtime regression also captures the model from the arena and a closer face
angle in `build/shepherd-pass/`. Pieces preload during level setup. Material
batches keep each exported part below the draw-call budget. The head collision
bounds and mouth emitter are adjusted to the new sculpt.

## Anatomical face and athletic proportions (2026-11-14)

This revision supersedes the previous face, short battle-pose arms, and blue
material. A pinned CC0 anatomical mesh and adult male/athletic morph data are
adapted in Blender through `tools/blender/shepherd_anatomy.py`; data, notices,
source links and checksums live in `tools/blender/reference/makehuman/`.
Only the mesh and target data are used. The runtime ships no model generator
and reads the resulting GLBs as before.

The face now has real eyelids, lips, nose, cheekbones and jaw topology with a
neutral expression. There is no drawn frown, nose wedge, slit-eye decal or
spherical cheek displacement. The torso and arms use anatomical surfaces;
clipping planes, closed caps and hull collars join the separate animated pieces.
The Blender studio previews show a neutral standing pose so arm lengths can be
judged without the old forward-facing foreshortening.

The silhouette is taller, with a smaller head, longer torso, and 85/82 m upper
and lower arm segments. A two-bone elbow solver preserves their ratio in idle
and punches; extreme edge-of-platform reach extends both segments equally.
Hands and elbow joints have been resized to match. Head and hand collision
volumes follow the revised proportions. The raised colossus head is now at
Y 255, Z -4135, and its chest is 93 m below it.

The surface is near-black charcoal (`.022, .024, .029`), rather than blue skin.
Neutral key and ambient light preserve that color; blue remains in subtle
undertones, seams and eyes. Material batching and the existing combat behavior
are retained. `shepherd-front.png` joins the bust and face previews in
`build/shepherd-art/`. The editable source is still
`tools/blender/scenes/shepherd.blend`.

The anatomical revision passed `make test-shepherd` (including export geometry,
eyeballs, material batching and arm-segment proportions),
`make test-shepherd-runtime`, native `make`, and `make web`. The neutral studio
portrait, full standing silhouette, arena view and punch capture were inspected.
The visual proportion reference includes the [Doctor Manhattan artwork shown
by DCU Guide](https://dcuguide.com/Doctor_Manhattan_%28Jon_Osterman%29_%28Watchmen%29);
this is a visual study only, not geometry or imagery shipped in the game.


## Surface, motion and celestial court revision (2026-11-15)

Carbon skin now has mineral strata, recessed veins, fine pits and micro relief.
Hull plates use scored lamination and a separate rough finish. Face detail is
quieter so the brow and jaw remain readable. The game shades these patterns in
each piece's local coordinates; they follow the limbs instead of swimming over
the model. The Blender assembly has matching procedural grain and bump materials.
Emissive eyes and palm targets retain their own material channels.

`Gameplay_ShepherdMotion.h` owns the simulated rig. Fists wind up on curved
paths, accelerate into their marked impact, remain planted on their combat
volume, then lift through a continuous 1.3-second recovery. The torso breathes,
turns and follows through; shoulder attachment, facial collision and chest
collision share that body transform. Fingers relax, close and fan by attack,
with independent wrist motion. Memory palms and the orbital/gravity gestures
blend out of the preceding pose. Rendering never advances the rig.

The final arena is now 180 by 164 metres (29,520 square metres, 2.54 times its
old area): a court of broken orbits with tarnished brass inlays, a compass
rosette, inset worn hull panels, dark radial seams, 24 crowned shelter vanes,
and flying ribs/split arches around the perimeter. Both raised side galleries
have real slopes at both ends. The central route to the heart stays clear.
Slams and memory marks use the ground height beneath their target, including
the galleries. The last count is 24 seconds to accommodate the longer route.
The crown landing and practice start remain at their established positions.

Validation adds gallery slope/landing/post checks, a complete live fist impact
and recovery check, and a render-independence check. Runtime captures include
`court.bmp` and `court-wide.bmp`. Set `SHEPHERD_CAPTURE_MOTION=1` when running
`build/test_shepherd_runtime` to export a 15 fps review sequence of a complete
wind-up, impact, grounded pause and recovery to `build/shepherd-pass/motion-*.bmp`.

## Presence and pressure revision (2026-11-15)

The previous pose changes were too small to read from the arena. The resting
hands now form an asymmetric, closing guard. A continuous weighted spine bends,
twists and breathes through the torso's actual vertex positions, including its
armor. Upper-body attachments follow the same spine, while the head turns from
the neck to watch the player. Hands orient from their forearms and blend into
the flat planted pose. The opposite arm braces during a strike. Idle movement
runs on the simulation clock and remains active between attacks.

The regular sequence now prioritizes spatial pressure instead of the old
jumpable beam/countdown loop:

- **Arm sweep:** a shoulder-led sweep crosses the player's locked depth lane.
  Its vertical footprint catches an ordinary or double jump. Move forward or
  backward out of the marked strip. The return variation crosses a separately
  marked second lane after a short transition; it has its own damage window.
- **Open-hand grab:** the target stops following after 0.65 seconds, reaches
  the mark at 1.45 seconds and closes at 1.85. Dodge sideways or parry the white
  closing window. A miss leaves a short punishable palm. If caught, shooting,
  sword contact, punching, HOLDFAST, or escaping the grip can break it. Weapons,
  movement and camera controls remain live. A held spike to the hull also
  counters the pull; scenery interrupts the carry. Failure to escape within
  2.3 seconds causes a 40-damage crush and release. Countering opens the body.
- **Locked stare:** paired eye rays visibly track, then stop following before
  the hit. Leave the marked impact, then route around paired hull scars for
  3.8 seconds. The variation fires three separately warned positions. Eyes
  also cast short, quiet shafts during idle so his attention remains visible.
- **Staggered fists:** paired slams now land 0.42 seconds apart, with individual
  white parry windows and independent target locks. Their marks overlap enough
  that standing between them does not create a free safe seam.
- Orbital volleys vary between five and seven shots, lead moving targets and
  gain speed in later movements. Persistent projectiles and gaze scars keep
  pressure through the shorter recovery beats. Hostile orbital shots clear on
  a successful grab so a caught player is not hit by an unrelated volley.

Pinned/grounded hands are never reused for invisible physical attacks. The
other hand attacks when available; with both hands planted the Shepherd uses
its eyes. Phase transitions, defeat and retries clear grips and gaze scars.
The boss stays damageable during normal attacks, so these are opportunities to
move and fight rather than mandatory waits for a vulnerability window.

`BoxInstance::deform` is zero for all existing actors. Only the Shepherd torso
uses the optional vertex deformation; its anatomical coordinates are cached
once from the existing Blender GLB. Shader normals follow the deformation's
Jacobian, and culling accounts for the bent body. Low surface-detail settings
still show all motion and mandatory attack tells.

Verification for the presence/pressure revision: native and Emscripten builds,
`make test`, `make test-render`, `make test-shepherd`, and all 108 native runtime
checks passed. The runtime checks exercise every phase's full attack sequence
and both variations, sweep damage/avoidance, grab dodge/parry/weapon/HOLDFAST
escapes, gaze locks/scar expiry, planted-arm selection, and actual torso pose
changes. Render regression checks preserve existing actor/view-model output.
The inspected 15 fps review is `build/shepherd-pass/shepherd-pressure.mp4`;
regenerate with `SHEPHERD_CAPTURE_MOTION=1 ./build/test_shepherd_runtime` and
encode the resulting `pressure-%03d.bmp` sequence at 15 fps. Final attack
balance remains subject to human play feedback.

## Court, counterweight and boarding pass (2026-11-15)

The first implementation of the finale review concentrates on readable routes,
visible consequences and momentum into the humanoid fight.

- Two inner terraces have end ramps and a central ramp from the heart lane.
  Level bridges connect them to the existing outer galleries. Orbital clamps
  initially obstruct these crossovers; the first health-third break throws the
  west clamp clear, and the second clears the east. Both galleries remain
  accessible through their original end ramps throughout the fight.
- Sixteen shelter vanes replace the repeated twenty-four-vane grid, leaving a
  wider central firing and finishing route. Shelter posts remain reachable.
  A capsule dock on the west and a brass orbital arch on the east distinguish
  the flanks. Underside ribs give the court visible depth at its edges.
- Broad, quiet slab plates replace the busy hull/circuit floor treatment.
  Sparse recessed lights on the Orrery ridge let its three moving moons remain
  the brightest targets. A neutral light floor improves the body's readability.
- Palm pins now show the cause of their damage: the hand stays grounded, the
  torso strains, a captive mass arcs toward the heart, and the arrival triggers
  damage, sound, recoil and the five-second damage opening. Each hand has an
  independent pending impact. Movement transitions, defeat and retries cancel
  unfinished impacts so they cannot damage a later movement.
- Each health-third break leaves damaged heart cages, chest seams, stress
  fractures, a displaced clamp and released capsules. Released fleets have
  separate simulation timers so advancing a movement does not reset their drift.
  The checkpoint's health third reconstructs broken clamps and opened paths;
  retries do not replay the break animation. Floors and shelters stay intact.
- Distant fire restores 1 HP at most every 0.8 seconds. Grounded-palm hits within
  42 metres, and exposed-body hits within 100 metres, restore 3 HP every 0.3
  seconds. The existing pin/parry healing remains. All amounts scale with tuning.
- The third ascent ledge is a boarding objective with finite guards. The Gather
  starts while they are alive, both ride chunks are marked immediately, and
  boarding completes the objective. Missing a ride retains the existing
  damage-and-return recovery. The first two ledges retain their composed fights.

Start the court with `./shooter --shepherd --arena 11`, the boarding ledge with
`--arena 10`, or the Orrery with `--arena 4`. `make test-shepherd` walks the actual
collision geometry through both closed and opened crossovers and checks terrace
access. `make test-shepherd-runtime` exercises delayed pin damage, transition and
retry cancellation, persistent routes, healing incentives, and boarding with
live guards. Additional captures in `build/shepherd-pass/` are
`counterweight-flight.bmp`, `court-movement-2.bmp` and `court-movement-3.bmp`.

Verification completed: native `make`, Emscripten `make web`, the full `make test`
suite, `make test-render`, and all 118 `make test-shepherd-runtime` checks passed.
The Shepherd light bake was regenerated and the final court, counterweight,
third-movement and Orrery captures were inspected. Human playtesting is still
needed to tune the new healing rates and boarding pressure.

The broader review also proposed connecting every approach objective through
visible damage, giving the first two ascent ledges new goals, optional forearm
traversal, deeper enemy/environment interactions and a musical through-line.
Those remain future passes; this change establishes the court and counterweight
behavior against which to playtest them.

## Ascent objectives and first audio implementation (2026-11-16)

All three ascent ledges now have different goals. On the first, bait a marked
Shepherd strike onto the mooring, or spike its core with HOLDFAST to call a
strike there. The delayed impact breaks the mooring; the player remains free
to dodge. On the second, HOLDFAST arrests a moving relay: hold it steady for
2.5 seconds while fighting. Releasing early resets only this short hold. On
the last, board the Gather under pressure. Each has finite guards, an explicit
objective and a marked interactable. Completing a mechanism clears its remaining
guard detail through the existing objective system. Retries restore the current
ledge's mechanism, and earlier ledges stay completed.

The HUD now uses the Shepherd's current objective during its fights. Previously,
external goals inherited Chapter 2's turbine-brake text; that text is now limited
to Chapter 2. New mechanism captures are `build/shepherd-pass/mooring.bmp` and
`relay.bmp`. Start these ledges with `--shepherd --arena 8` and `--arena 9`.

The [audio plan](chapter-6-audio-plan.md) contains the audit, cue vocabulary,
three-movement score brief, production order and audition criteria. Its first
implementation ships 54 original WAVs with protected directional warnings,
separate pin/strain/heart-impact sounds, phase breaks and a dedicated finisher.
The music's Shore intensity override is fixed and its Overdrive layer follows
the actual player state. The dedicated new composition remains planned.

Verification completed for this pass: native and web builds, the full `make test`
suite, `make test-render`, `make test-audio` and all 131 native Shepherd runtime
checks passed. The web package contains all 54 new Ogg variants. Mooring and relay
captures were inspected after fixing the HUD, and the regenerated light bake
passes the level checks. Offline mix/stress reels peak at 0.552/0.684 after the
game's output stage; subjective timbre, mix balance and encounter pressure still
need human audition and playtesting.


## Release polish (2026-11-16)

COUNTERWEIGHT now has its own 174 BPM score, with ascent, control, resistance,
collapse and last-count arrangements. Each combat movement changes rhythm and
melodic response while retaining the pulse and comparable loudness. Arrangement
changes land on the next bar; physical counters and attack cues stay immediate.
Retry fades and restarts the checkpoint's movement, and the capsule ending
remains quiet. See the audio plan for reproducible renders and measurements.

Web smoke coverage now includes each ascent objective, the court and the capsule
ending. Web Audio checks can target a supplied checkpoint, including the court;
CI exercises both SUNSET and the Shepherd through the real browser callback.
The website describes the complete sixth chapter, includes a current court
capture and lists the HOLDFAST control.

`make test-shepherd-playthrough` runs a separate input pilot through the normal
update loop with god mode off, a base Lancer, ordinary magazines and reloads,
physical movement, real parry/HOLDFAST inputs and the existing attack schedule.
The release attempt reached the core, sealed it and completed the capsule ending
in 98 simulated seconds, with 47 trigger pulls and a lowest health of 30.7.
It injects no boss damage, health refill, mid-fight teleport or completion flag.
This establishes that the full loop is playable under those inputs; it is not a
substitute for a human judging aim, challenge or sound on their own equipment.

Release verification: native and web builds, the complete `make test` suite,
render regression checks, 131 Shepherd runtime checks, the input pilot, web
Story/lighting/release checks, pointer-lock checks and Chromium smoke/audio
checks passed. Website type/content checks, 35 tests and the static build/link
verification passed before publication.


The release check also found an older platform discrepancy in both Chapter 6
levels: negative floating-point layout seeds were cast directly to unsigned
integers. ARM Clang produced zero while x86 GCC wrapped them, changing rock
recipes and rejecting the baked lighting. Seeds now truncate to a signed int32
before the standard unsigned conversion. The resulting Mac and Linux hash
input streams match byte for byte (5,467,120 and 1,285,276 words respectively).
Both levels' lighting was regenerated for the defined recipes. Seed tests and
float-cast-overflow sanitizer runs cover the conversion. Failed Linux lighting
checks retain their fingerprint stream for diagnosis without relaxing the cache
validation. Web Story preview tests now select bosses by name instead of stale
numeric enum indices.

The website offers direct practice previews of the ascent and crown fight.
`--practice` / `?practice` applies the existing unranked, no-Story-save rules to
checkpoint launches; normal Story starts retain their existing behavior.
