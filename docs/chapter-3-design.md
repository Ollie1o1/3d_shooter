# Chapter 3: THE LIVING WARD

Design consolidation, 2026-10-31. This is a build brief, not an implemented
chapter. It supersedes the transit/Surveyor direction in chapter-3-directions.md.
The companion chapter-3-build-prompt.md is the handoff for a fresh conversation.

## 1. What is settled, and what is a working decision

The user has established:

- Chapter 3 is a laboratory testing newer, flesh-like versions of the robots.
- There are exactly ten principal rooms; room ten is the boss chamber.
- The descent becomes progressively stranger, with floating bodies, dimensional
  glimpses and eventually large spaces bordering incompatible realities.
- The architecture is spacious, legible and often brightly lit. The hospital
  opening develops into surreal spaces; narrow dark corridors are not its identity.
- An entry cutscene implies immediately that this chapter will be unusual.
- Late rooms include a glass-roofed nebula view and a deep-ocean-trench environment.
- Beating the boss opens entry into more extensively fractured dimensions.
- The boss resembles a nearly human perfect test subject but retains its robot
  face. He is fast, agile, extremely strong, memorable in dialogue, and supported
  by fast, dramatic choir-led music. The Absolute and Morningstar establish the
  desired intensity, while this boss needs independent attacks and ideas.
- Small environmental implications matter as much as the large reveals.

Working decisions made to make the brief executable, open to later renaming or
human tuning: chapter title THE LIVING WARD; boss THE SUCCESSOR; three phases
within one health bar; approximately 176 BPM for the original boss score; the
room sequence, attack timings and lines below. These names and numbers are not
user-approved facts. Use them as defaults rather than blocking implementation.

## 2. Premise and progression

The lab is developing bodies that can remain coherent across unstable spatial
boundaries. Start with that practical research purpose. Keep the ultimate origin
of the fractures, the creator's identity and UNIT 24's eventual purpose unresolved.
Records can show what was measured without explaining the cosmology.

The older mechanical UNIT 24 is an unexpected visitor. THE SUCCESSOR is a viable,
self-aware subject whose apparent perfection has become his identity. He regards
UNIT 24 as an obsolete comparison specimen and believes crossing is his reward.
His composure fails because the older model is doing something the tests did not
predict. His final line can reveal that he wanted to leave too.

Escalation:

1. Rooms 1–3: credible medical/testing architecture; small physical inconsistencies.
2. Rooms 4–6: impossible views and one serene, inexplicable garden. The lab
   continues to provide recognizable routes, equipment and objectives.
3. Rooms 7–9: larger spatial contradictions and genuine dimensional crossings.
   The nebula and ocean are real exterior environments, not a simulation reveal.
4. Room 10: a stable assessment floor at the junction of incompatible worlds.
   Victory breaks the final boundary and lets the player walk beyond it.

Target an exploratory first clear around 35–50 minutes as a pacing hypothesis,
not a runtime promise. Author ten substantial areas with smaller connected spaces;
never pad them with identical repeated waves. Downward travel can use broad
stairs, ramps, open shafts, galleries and lifts. Preserve a readable route.

## 3. Ten rooms

| # | Room | Spatial identity and play | Small implication |
|---|---|---|---|
| 1 | ADMISSIONS | Wide reception and examination bays; entry cutscene; first controlled flesh-unit encounter after control returns. | Staff facilities and patient facilities are both present. A scanner calls UNIT 24 a legacy chassis. |
| 2 | RECOVERY WARD | High ward with curtains, open treatment islands and upper observation gallery; teach the new melee unit in a bounded fight. | A patient hovers just above an otherwise ordinary mattress. Its chart still records weight. |
| 3 | MOTOR ASSESSMENT | Spacious vertical testing hall; broad platforms, rehabilitation rails, routes above and below; teach the heavy's directional defense. | Earlier rigs support every limb; later stations have fewer supports. |
| 4 | SURGICAL OBSERVATION | Two-level operating theatre; crossfire between floor and galleries; first sustained impossible window view. | One observation booth has no entrance. Its chair faces the player. |
| 5 | THE CONSERVATORY | A full quiet garden with grass, shallow water, trees and apparent daylight, reached through a normal hospital door. Exploration and a movement secret; no mandatory ambush. | Watering equipment remains, but its supply pipes end disconnected. A numbered trolley will recur later. |
| 6 | LOAD TESTING | A straight broad crossing over a seemingly bottomless chamber; substantial side platforms and grapple rails offer fighting routes. A major encounter combines the early new enemies. | Hanging load indicators report a floor below the visible drop. A broken service bridge reappears much farther down. |
| 7 | LONG-TERM CARE | Large suspension atrium and isolation terraces. Floating bodies form a slowly moving spatial landmark. Introduce the crossing enemy with its tell unobstructed. | Staff spaces disappear. A body casts its shadow onto an adjacent wall. |
| 8 | CELESTIAL OBSERVATION | Bright treatment hall beneath a huge glass roof and wide windows; nebula and architectural fragments beyond. Learn one routing control safely, then use it during a bounded encounter. | A maintenance chair and tools sit on an exterior platform. A patient with an interior identification band drifts beyond the roof. |
| 9 | HADAL RESEARCH | An ordinary door reaches an illuminated hall overlooking a deep ocean trench. Broad stairs, glass galleries and a short spatial-routing encounter, followed by recovery before the boss. | Ventilation becomes a structural hum; sediment moves outside. The garden's numbered trolley returns with different damage. |
| 10 | FINAL ASSESSMENT | Large stable arena, a lower floor and broad connecting galleries. Observation boundaries show nebula, trench and garden. THE SUCCESSOR is the sole major opponent. | The final record has no correction notes. The viewing gallery contains no seats for staff. |

Two authored secrets: a motor-assessment maintenance route and a conservatory
observation route. Neither requires a blind leap. Optional wrecks, records and
physical traces give specific incidents and measurements. Reading remains explicit.

Maintain familiar material proportions, door frames, service numbering and a few
recurring props across the changing exteriors. Anomaly details should be sparse
enough to notice. Inactive suspended bodies and attacking suspended enemies have
different silhouettes, activation motion and audio; harmless bodies are not
unpredictable traps.

## 4. Entry cutscene: an ordinary procedure with an impossible result

Approximately 20–24 seconds, in the player's first-person view:

- 0–4 s: the service entrance seals; a scanner crosses UNIT 24's hands and face.
  A small monitor records LEGACY CHASSIS / MANUAL ASSESSMENT.
- 4–10 s: the camera's natural line through the observation glass reveals a
  humanoid subject performing a routine motor test: stand, take three steps, stop.
- 10–15 s: the subject rises slightly clear of the floor, while its shadow keeps
  walking on the far wall. Instruments continue to report normal contact.
- 15–20 s: the observation glass slowly reveals an impossible exterior—dark water
  with distant suspended lights—while the reflection still shows the clinical room.
  Use a sustained readable change, not a strobing flash or rapid montage.
- 20–24 s: the descent entrance opens. The display accepts the result. A simple
  announcement says, "Proceed to observation." Control returns facing a clear route.

The subject is a prototype, not a full early reveal of THE SUCCESSOR. No lore
speech explains the anomaly. There is no combat during the sequence. Skipping
lands in the identical playable state, with inventory/camera/collision correct.
A saved seen flag and post-cutscene checkpoint prevent repeats on death or Continue.
Camera movement is restrained; reduced-flashing settings still apply. This is
Chapter 3's first major entry cinematic, not a claim that the game has no prior
scripted wake-up.

## 5. Spatial interaction and presentation

Use service routing to change one understandable connection between two authored
spaces. Introduce it without enemies in room 8; use it to gain an angle or escape
route later; recall it during the boss's attacks. The player deliberately chooses
when to operate a routing control. Crossing does not randomly steal control.

A normal door between nebula and trench is more effective than a repeated noisy
teleport effect. Staging, occlusion and authored transitions can establish that
contrast. Real seamless portals and freely rotating player gravity are not
prerequisites for the chapter. Choose the technique after a visual prototype;
preserve the impossible views rather than reducing them to flat decorative posters.

The late exteriors need scale and depth: nearby window structure, middle-distance
objects, distant silhouettes and slow parallax. Keep bright interiors against the
trench and a restrained exposure range around the nebula. Combat tells must remain
readable against both. Mandatory warnings, routes and targets survive low detail.

The full fractured-world campaign begins after this chapter. Build the threshold
and a short playable coda, not an extra eleventh principal room or Chapter 4.

## 6. New enemies

Build four distinct families, introduce one at a time, then combine them:

- PATIENT: agile, supported flesh body that briefly releases a brace for a committed
  lunge. Clear silhouette change before the lunge; a dodge or stagger exposes it.
- GRAFTED: heavy body with a directional support plate and visible tissue reservoir.
  Reposition around its committed attack or interrupt the exposed reservoir; avoid
  a generic permanent armor multiplier as its whole identity.
- SUSPENDED: active observer above the floor controlling a visible firing lane.
  It visibly unfolds and emits a distinctive cue before attacking. Shoot or grapple
  an accessible body/anchor; harmless floating bodies never share its activation tell.
- PHASE SUBJECT: crosses between marked entry/exit frames. A destination cue and
  an audible crossing precede its arrival; it stays hittable during a recovery.
  It does not appear silently behind the camera or track the player through a dodge.

Give them original models, animation, voices, hitboxes and headless behavior.
Share ordinary damage, poise and event infrastructure, not just old enemy meshes
with skin-colored materials. Preserve friendly-fire, grapple and finisher rules
where their body mass permits. Keep the initial roster small enough to finish well.

## 7. THE SUCCESSOR: body, personality and arena

Roughly 2.4–2.7 m tall, near-human proportions, stable posture, continuous synthetic
flesh, detailed hands, and a clearly mechanical face visibly related to UNIT 24.
Mechanical connections remain at the neck/joints. The face stays a robot face
through death. No copied halo, angel costume, light sword or enlarged Morningstar
transformation. Human motion, confident footwork and the contrast with the face
carry the design. Fine surface detail supplements a readable silhouette.

He is proud of being the result the lab wanted. Early delivery is warm, precise
and quietly insulting. Later delivery becomes clipped and strained. By the final
phase his test vocabulary has become personal. One human motive—wanting to leave—
is more effective than explaining the world's entire history in his death speech.

Arena: approximately 64 × 64 m of usable main floor, broad connected galleries,
clear ramps, four fixed routing frames and limited solid cover. Keep the central
combat floor stable throughout. Large views and movable outer scenery can become
strange without remapping the player's aim, rolling the camera or silently removing
landings. Bosses must threaten range and elevation through visible committed attacks.

## 8. Combat identity: a body built to cross boundaries

Maintain the intensity of the Absolute/Morningstar while using independent state,
rig, attacks and environment interactions. THE SUCCESSOR fights through incisions,
filaments, trajectory changes and marked crossings. Do not give him the existing
jab-jab-hook/meteor/corona moves under new names.

Working Standard targets: 12–14 m/s running, 32–38 m/s short committed bursts,
about 150–190 seconds for a competent gun-and-parry player. Start HP near 8,500 only
as a simulator input, then tune to that duration with the actual available loadout.
Three phases at 100–65%, 65–30%, 30–0%; one health pool, no refill or second-form tax.
Guns, movement, grapple and F parry are the required tools; no Story sword unlock.

| Attack | Visible action and answer | Opening / purpose |
|---|---|---|
| INCISION | Forearm tissue opens around a rigid cutting edge; two committed cuts, with a white final thrust. Read the body, evade the cuts or parry the thrust. | Parry cancels the string and exposes the extended arm. Establish intimate speed and anatomical precision. |
| VECTOR RUN | Drops into a sprinter's stance, a gold lane locks, then a powerful straight burst through it. Dash across the lane or use an upper route. | Brakes and turns visibly after a miss. He cannot steer back onto a completed dodge. |
| SUTURE | Casts two visible filaments to fixed supports, then draws them through marked strips. Shoot a filament's anchor or move through the remaining broad gap. | Cutting an anchor interrupts this pattern and briefly staggers him. Preserve at least one reachable escape. |
| TRANSPOSITION | A visible entrance and destination incision open. He crosses and attacks from the announced side. A white arrival strike can be parried. | Exit commits before damage; a parry disrupts the crossing. This expresses his purpose without a seamless-portal dependency. |
| RECALL | Leaves a visible imprint of the player's previous position, then cuts through that imprint after a delay. | Keep moving or bait the cut away from your next destination. The mark stops tracking; no retrospective hits. |
| PERFUSION | Opens the chest/arm manifold and fires a bounded fan around one slower white core projectile. | Shoot the exposed manifold during its charge, evade the fan, or parry the core. This is an active range answer with a recovery. |
| DOUBLE EXPOSURE | Late-phase follow-up: a previous incision briefly appears at a marked second location and cuts there after a new tell. | One delayed echo at a time, fresh sound and visible location. Do not replay an invisible hitbox. |

Phase 1 introduces Incision, Vector Run and Perfusion. Phase 2 adds Suture and
Transposition and pairs established moves. Phase 3 adds Recall/Double Exposure
and a small authored set of demanding combinations. Do not enable every move
simultaneously. Avoid two independent high-priority attacks resolving together.

Once below 10%, a signature FINAL ASSESSMENT sequence can combine a locked Vector
lane, a Suture gap and a white arrival strike over roughly six seconds. Each part
has its own cue and answer; the final parry ends it early. He remains damageable;
lethal damage ends the fight immediately. This is a culmination, not an invulnerable
survival intermission.

Fairness and recovery:

- Initial telegraph target 0.5–0.7 s; opener floor about 0.4 s and follow-up floor
  about 0.25 s. Validate these with human play, animation and all difficulties.
- Each strike has a clear animation, spatial warning and nonverbal sound. White
  means parry; gold means evade; red indicates a spatial hazard. Shape and sound
  redundancies remain with color-blind/reduced-flashing options.
- Typical hits 18–28 damage; a readable whole mistake should allow an escape rather
  than guarantee 100-to-zero. Damage invulnerability must prevent accidental
  multi-tick contact damage and unavoidable follow-up stacking.
- Parry interrupts pending follow-ups. A white parry restores about 8 health,
  scaled by difficulty, once per resolved attack. Strong counters expose a
  1–1.4 s damage window; use a longer break for a genuinely difficult counter.
- Place two finite health pickups where they require repositioning. No routine
  fodder adds are required to make healing possible. No infinite regeneration.
- Punish camping by selecting a suitable marked lane/crossing, not by silently
  teleporting onto the player. Brief recovery and exposed attacks support all guns.
- Reset all filaments, echoes, routing states, pending hits and phase audio on retry.

## 9. Original dialogue and delivery

Recommended script; audition and trim in play rather than filling every silence:

| Trigger | Line |
|---|---|
| Introduction | "Unit Twenty-Four. Still running." |
| Start of combat | "Come closer. I want a proper comparison." |
| First major successful player counter | "That was not in your design." |
| Phase 2 | "There. Something they missed." |
| Rare pursuit taunt | "Keep moving. I want the full result." |
| Optional introduction alternative | "The door opens for one of us." |
| Phase 3 | "No more corrections." |
| One late damage response | "You were supposed to be the easy part." |
| Defeat | "I was supposed to leave." |

Dialogue is an occasional characterization layer, never the only attack tell.
Do not repeat introduction/phase speeches on each normal retry. Offer replayable
subtitles, audio and subtitle settings, and a roughly 12-second ambient-taunt
cooldown. Attack sounds have priority over dialogue and music.

The existing v_absolute_line is a nonverbal synthesized choir phrase; it is not
intelligible spoken dialogue. A spoken version needs original recorded or
licensed/generated speech and a dedicated playback/subtitle path. Use a controlled,
recognizably human original performance, lightly processed at the robotic face.
If speech assets cannot be produced with available tools, implement the full
subtitle/event logic and distinct vocal cues, report the remaining speech work
explicitly, and continue other authorized implementation. Never label a choir
vocalization as completed spoken acting.

## 10. Original score and sound direction

The user's emotional reference is Gabriel's Apostate fight. The official track
reference is Heaven Pierce Her's "The Death of God's Will (incl. 'Horns of
Insurrection')":
https://heavenpierceher.bandcamp.com/track/the-death-of-gods-will-incl-horns-of-insurrection

The following instrumentation and tempo are our composition targets, not a claim
about that recording's measured tempo or arrangement. Compose an original score.

Working title: THE FINAL RESULT. Approximately 176 BPM; dramatic melody, an audible
wordless ensemble choir, organ-like sustained harmony, distorted rhythmic texture,
fast syncopated drums and a substantial bass foundation. Preserve contrast and
musical phrasing instead of leaving every layer at maximum throughout.

- Intro: the chapter's quiet three-note equipment motif is voiced by a low choir.
  The subject's first measured steps supply the pulse. The first committed attack
  brings in drums and the upper choir.
- Phase 1: clear original 8/16-bar theme, call-and-response between low and high
  wordless voices; active drums with space around attack cues.
- Phase 2: fuller harmony, rhythmic guitar-like or original synth ostinato, denser
  percussion, an answering choir line. The existing motif remains recognizable.
- Phase 3: choir in a stronger register, a developed counter-melody and controlled
  harmonic tension. Increase urgency through arrangement, not arbitrary global
  attack-speed changes or constant loudness.
- Defeat: rhythm and bass stop; one unfinished choral phrase resolves only when
  the boundary opens. The chamber's ordinary ventilation becomes audible again.

Use phase-aware transitions on phrase boundaries, with an immediate transition
accent when needed. Actual combat tells are never delayed to wait for music, and
players never have to parry to the soundtrack's beat. A retry resets the musical
state predictably. Keep attack cues distinct from the choir's harmonic register.

Repository implication: MusicSynth.h currently has 12 tracks; Zenith/Morningstar
use techno voicings. Adding another techno table row is insufficient for this
brief. Implement an appropriate original formant choir/instrument arrangement or
original authored stems. Evaluate it by listening, not only by checking constants.
Bound voices/CPU, do not allocate in the audio callback, and protect tell/dialogue
priority in MixTable/EnemyVoice/SfxMixer. Existing assets/credit practices apply.

Rooms also need restrained new beds: clinical pulse, garden air, suspended low
choir in celestial observation, low structural/underwater resonance in hadal
research. The boss should feel like the culmination of an audible chapter motif.

## 11. Defeat and threshold

His body becomes still and loses its effortless support. The robot face remains
recognizable. "I was supposed to leave" is brief and optional to hear/read.
Filaments release, all hostile attacks stop, and the assessment apparatus loses
its stable reading. The observation boundaries cease to separate the environments.

A door-sized exit opens onto an impossibly long, broad corridor over a void, with
a piece of the conservatory visible ahead. Return movement control, let the player
walk that first crossing, then show the chapter card. Do not replace the payoff
with an immediate black screen. Keep the ending route safe from stale hazards.
There is a checkpoint before the boss and a persisted defeated/open-exit state.

## 12. Build order and integration

1. Establish the complete ten-room route and save/checkpoint schema with a rough
   but traversable level. Add chapter selection, Chapter 2 handoff and direct dev
   starts. Preserve existing Story equipment, upgrades and discoveries.
2. Produce a representative vertical slice: bright hospital bay, entry cinematic,
   a flesh enemy, and a staged nebula-to-trench crossing. Validate scale, interior
   exposure, exterior depth, atmosphere and ordinary movement before decorating
   all ten areas. Iterate the slice, then continue the authorized full chapter.
3. Establish THE SUCCESSOR's model and locomotion early. Prototype Incision,
   Vector Run and one crossing attack in the real boss arena with a temporary
   score. Validate readable speed before expanding the complete moveset.
4. Build the remaining enemy families and authored encounter rhythm. Finish
   rooms 2–7, then the full room-8/9 routing interaction and secrets/records.
5. Complete all boss phases, counterplay, health economy, anti-camping, dialogue,
   original choir score and threshold ending. Integrate every art/audio piece.
6. Complete detail, animation and readability passes. Review each room in motion,
   including low ambient detail and reduced flashing; review every boss phase.
7. Run native and WebAssembly verification, actual input/playthrough checks,
   regression suites, frame/audio profiling and human balance checks where available.
   A scripted simulator is regression evidence, not a substitute for human feel.

Suggested code locations: LevelLab.h, Gameplay_Lab.h and a dedicated boss directory
such as bosses/successor/, plus appropriate new enemy state/model/audio modules.
Use the existing enemy event split and headless geometry/hazards architecture.
Models can use the Blender pipeline; preserve the game's existing visual vocabulary
while making the flesh bodies visibly new.

Important integration details:

- StorySave currently only accepts chapters 1–2 and fixed checkpoint limits. Add
  chapter-3 support with a versioned migration that still reads existing saves.
  Include cinematic-seen, checkpoint/equipment/upgrades, discoveries, completed
  objectives and defeated boss/open exit. Practice/capture runs cannot write it.
- Append a Successor Boss entry without changing the existing nine record indices.
  New remote leaderboard modes require deliberate API work in the separate website;
  use local boss records until that support exists. Do not accidentally submit them.
- Preserve the four-gun upgrade/save arrays and Story's equipment ownership.
  Direct chapter starts receive appropriate earlier gear; no sword requirement.
- Menu/dev/CLI/web starts must reach every room with coherent prior objective state.
- Add lab support to light baking and level export; regenerate changed bakes and
  check cross-compiler fingerprints. Rendered mechanisms and collision must agree.
- Review music-track routing: do not depend on an arena index accidentally selecting
  an old track or breaking the established track IDs.

## 13. Acceptance checks

- Exactly ten principal rooms, connected with real collision and recoverable falls.
  Broad required routes, useful landmarks and no progression through invisible walls.
- Intro plays once; normal, skipped, retry and Continue paths reach identical safe
  states. Controller input, remapped prompts and browser pointer lock remain usable.
- Hospital, garden, nebula, trench and final chamber each receive native visual
  review and an in-motion pass; their large reveals retain depth on low detail.
- Each new enemy has a distinct silhouette, attack explanation, counter, movement,
  proper hitboxes and a representative actual combat test.
- Every boss attack's tell, target commitment, interruption, recovery and damage
  agree. No pending hits after parry, phase change, death or retry. Test high,
  ranged, circling and stationary strategies plus ordinary gun play.
- Confirm actual spoken assets versus subtitle-only fallback honestly. Listen to
  the composed score and dialogue/tell overlap; verify phase transitions and retries.
- Save migration preserves old chapters and the new chapter's state; death after
  victory never resurrects the boss or closes the exit.
- Native/web builds, relevant existing suites and new meaningful lab/boss checks
  succeed. Benchmark the heaviest lab view and densest boss overlap; control optional
  particles, distant geometry, choir voices and lighting without hiding combat tells.
