# The Shepherd: audio plan

The fight should sound like an orbital machine trying to keep its own body in
place: measured counts, tension in metal, captive masses moving, then failure.
Attack recognition and satisfying counters come before extra ambient detail.

The current November 17 revision adds a much stronger impact/footstep hierarchy,
pose-driven giant movement, damaging-hit feedback and clearly separated music
movements. The dated sections at the end record the changes and their checks.

## Audit

- Most attacks share `telegraph`, `slam`, `barrier` and `clank`. Their sounds do
  not explain which response the player needs.
- The humanoid uses THE KEPT RING at high intensity with a permanent Overdrive
  music layer. It has no arrangement tied to its three health movements.
- The mixer fades ordinary positional effects beyond 90 metres, while the
  Shepherd's chest and face are often farther away. Vacuum further reduces
  distant world sounds. Important cues need an explicit floor and role.
- Pinning and heart impact need separate audible moments. Cancellable tension
  must stop on a phase transition, death or retry.
- The established direction is full-tempo boss music. Preserve the 174 BPM
  pulse during combat and build intensity through orchestration and rhythm.
- The Shore's approach intensity was also applied inside the Shepherd whenever
  its wave director was idle, including the custom humanoid fight. This
  overwrote the boss intensity with the approach ramp.

## Sound identity and references

Use three related materials: dry counting clicks, strained inharmonic steel,
and a low mechanical body with enough midrange to read on small speakers.
Keep tails short during active attacks. Reserve long resonances for broken
clamps, victory and the quiet ending. The player's guns and parry stay present.

Housemarque's [Hyperion breakdown](https://blog.playstation.com/2021/05/28/returnal-the-making-of-that-unforgettable-hyperion-fight/)
connects an approach motif, its source, and an escalating boss arrangement.
The [Returnal audio team's interview](https://www.asoundeffect.com/returnal-game-audio/)
discusses controlling dense combat and the relationship between sound and music.
Here the application is a counting motif carried from machinery into the score,
with attack signatures separated from decorative machinery. These are original
design choices, not attempts to reproduce those games' recordings or music.

## First implementation: combat cue vocabulary

| Event | Sound | Timing and mix |
|---|---|---|
| Fist wind-up | Two low steel knocks with a rising stressed edge | On wind-up, direction follows the attacking hand; protected TELL |
| Arm sweep | Wide scraped-metal rise | On wind-up; separate shorter pass at each sweep release |
| Open-hand grab | Closing ratchet with descending paired tones | On wind-up; short lock click when tracking stops |
| Gaze | Narrow electrical interval rising into a dry lock | Tracking start and actual target lock; avoid noise masking the parry |
| Memory palms | Three hollow registration taps | Exactly when each position is recorded; impacts use the heavy contact cue |
| Orbital volley | Compact glass/metal launch | Per projectile, restrained level; distinguish from countdown ticks |
| Gather | Inward pitch bend over a mechanical pressure pulse | At pull start, not repeatedly every frame |
| Palm pin | Spike bite, then steel under increasing tension | Immediate pin confirmation; tension lasts only to the pending heart impact |
| Counterweight | Dense contact and a brief bright fracture | Exactly with the delayed heart damage and recoil |
| Movement break | Cage fracture with a descending mass tail | Once per boundary, surviving as a reward above the machinery |
| Heart sealed | Short decisive latch | When the final HOLDFAST succeeds |
| Internal detonation | Contained impact, fracture, delayed low tail | At the actual internal explosion; the ending returns to quiet |

Author deterministic original cue families with three variants, using the
existing frozen CC0 metal recording as texture where useful. Put every family
in MixTable and provide a reproducible cue reel. Tells target -14 dBFS short-term
RMS; major actions -10 dBFS; contact/world textures -13 dBFS. Preserve peak
headroom and verify onset/tail, sample rate and variation. These are starting
targets; audition them in the full mix before adjusting final trims.

Important tells use the mixer's TELL role, priority, directional placement and
an audibility floor. They survive vacuum, Overdrive and ordinary crowd pressure.
Body-transmitted contacts get a floor too. Do not globally increase the WORLD
bus or duck the soundtrack for every warning. Keep cancellation handles for
long pending cues so obsolete attacks do not keep sounding after a transition.

## Implemented: dedicated Counterweight score

COUNTERWEIGHT is track 18, separate from THE KEPT RING (17). Both use F-sharp
minor at 174 BPM. The crossing keeps its existing arrangement.

| Section | Arrangement at 174 BPM | Transition |
|---|---|---|
| Orrery / interior | Existing Kept Ring score and low pulse | Separate from the final ascent score |
| Ascent | Full-tempo break, sparse count, glass-like plucks | Boarding lands on a strong musical accent |
| Movement 1: control | Precise techno kick, restrained bass, repeating three-note count | Stable, deliberate, room for attack signatures |
| Movement 2: resistance | Breakbeat variation, upward melodic answer, fractured count | Next bar changes arrangement; impact reward stays immediate |
| Movement 3: collapse | Hybrid rhythm, octave response, wider harmonic tension | Maintain tempo and comparable loudness; no permanent Overdrive layer |
| Last count | Running pulse with reduced harmonic density | Timer ticks remain independent and truthful |
| Seal / detonation | Very short held breath, then release | Silence is tied to the explosion event, never to an arbitrary music bar |
| Capsule ending | Contact sounds and one fading remnant of the count | No combat rhythm resumes |

A thread-safe movement parameter changes the arrangement on the next bar;
gameplay never waits for music and phase changes preserve the tempo clock.
Retry fades out and restarts the checkpoint's movement. A track change clears
its voices and tails. Smooth movement trims keep the combat forms comparable.

## Later ambience and voice pass

Use contact-driven hull resonance and orbital bearing ticks during quiet gaps.
Do not add a continuous rumble that occupies the entire bass range. Let the
first two ascent mechanisms share the same metal/counting vocabulary as palm
pins. Freed capsules lose their synchronized ticks as they drift away.

Prototype sparse worker-like vocal phrases only after the mix is readable.
Tells must remain recognizable with dialogue off, music off and in mono. Avoid
repeating a line on every attack. Keep attack warnings independent of optional
detail settings.

## Acceptance and audition

1. Compare cues solo, then with music at the shipped slider default, weapons,
   Overdrive and the final movement's projectiles. Export both cue and mixed reels.
2. Verify warnings are audible from every court corner, facing away, in vacuum.
3. Check that lock, parry window, impact, pin and heart damage agree with the
   simulation at 30/60/120 Hz. A pause must not add queued future cues.
4. Retry during a pin and cross a movement boundary: no orphaned tension or
   delayed reward may play in the new movement.
5. Measure headroom, callback stability and packaging on native and web builds.
6. Audition on headphones, speakers and mono at a comfortable low volume. Ask
   players to name the incoming attack from its sound. Automated signal checks
   establish technical behavior; they do not replace this listening test.

## Implemented and available for audition

The cue pass is implemented: 30 original Chapter 6 cue families, three deterministic
variants each, registered in MixTable. The game emits attack-specific wind-ups,
target locks, white parry-window cues, memory records, orbital releases, physical
contacts, pin tension, counterweight impact, cage breaks, seal and detonation.
Wind-up/tension handles are cancelled on movement changes, death and retry.
No future attack sounds are scheduled ahead of the simulation.

The Shore intensity leak is fixed. The fight now keeps its intended boss
intensity and its full-tempo rhythm changes. Overdrive's additional music layer
follows the player's actual Overdrive state. The dedicated composition and
bar-synchronized movement arrangements above are implemented. Its rounded
metallic count differs from the earlier bosses' acid leads.

Run `make shepherd-audio` to regenerate the 90 WAVs and a labelled cue reel.
Run `make shepherd-audio-preview` for two 42-second boss reels and a 22-second crossing reel through the real
MusicSynth, SfxMixer, DriveBed and final output saturation/headroom gain:

- `build/shepherd-audio/shepherd-cues.wav`: isolated cue families, with timecodes
  in `build/shepherd-audio/README.md`.
- `build/shepherd-audio/shepherd-mix.wav`: cue sequence over the corrected boss
  intensity at the default music slider. Seal at 30 seconds, detonation at 31.5,
  and music stops at the detonation event.
- `build/shepherd-audio/shepherd-stress.wav`: the same sequence with repeated
  revolver, shotgun and rifle fire and Overdrive layers/focus from 18 to 25 seconds.
- `build/shepherd-audio/ring-crossing.wav`: quiet capsule and reef contacts,
  sweep warning, countdown and debris against the crossing's score.

The reels use scripted source positions and cue timings, not a recorded human
playthrough. They exclude room/wind beds and other enemy voices. The preview
reports sample peaks; it does not measure true peaks or subjective readability.
Physical speaker/headphone audition is still required.

Technical verification: all 90 WAVs pass format, onset, variant, tail, headroom
and mix-level checks. The real mixer test keeps a warning audible 240 metres
behind the listener through vacuum, an 18 dB background duck and Overdrive focus.
Runtime tests drive the actual attack code and mixer at 30/60/120 Hz, including
paired fists, return sweeps, triple gazes and seven-moon volleys. They verify
onset counts and timing for warnings, locks, parry windows, records and impacts,
plus actual voice cancellation on phase changes, death and retry. Ending checks
verify music stays present before detonation, stops at detonation, suppresses an
active Overdrive bed, and returns on retry. Native/web and regression results
for this revision are recorded below. No physical-device listening was performed.

The five 48-second movement renders are available in
`build/shepherd-audio/score-0.wav` through `score-4.wav`. Regenerate one with:

```sh
make tools/music_preview
tools/music_preview 18 48 build/shepherd-audio/score-1.wav 1.3 --movement 1
```

At preview volume 0.9, with the first two seconds excluded, measured integrated
levels are -13.7 LUFS (ascent), -12.5 (control), -12.5 (resistance), -12.0
(collapse) and -12.4 (last count). These solo synth renders precede the final
output gain; their true peaks are not the game output's true peaks. The mixed
reels above include that output stage. Tests render every complete form at
22.05/44.1/48 kHz, compare irregular callbacks, and check phase quantization and
retry restart. The actual browser callback at the court also passes at 48 kHz.

Headless browser checks use a silent Web Audio sink, keeping the real SDL
callback and autoplay policy while avoiding dependence on a sleeping display's
audio device. The shipped shell resumes `Module.SDL2.audioContext` on a user
gesture, including when audio initializes just after the first click. Release
identities now include the loader template so shell-only fixes invalidate caches.

The Shepherd now accepts an actual charged Overdrive attack, and palm counters
can earn that charge. Its music/DriveBed layers therefore follow a usable player
power state rather than a forced soundtrack switch. The entrance, phase breaks
and final core count preserve a ready charge instead of wasting it.

## November 16 sound-design revision

The previous tests passed despite a flat mix hierarchy: every Shepherd sound
was priority, unducked and at least 65% audible at any distance. Repeated moon
launches and the rising pin tone competed with the important warnings and
rewards. Several large actions shared the same transient/body recipe. Crossing
capsules played slowed bullet impacts, and the gaze reused the arm-sweep impact.

Rebuilt the 18 existing families around short recorded-metal transients, lower
inharmonic resonances and filtered friction. Replaced sustained rising warning
oscillators with recognizable attack rhythms; memory now has three registration
taps. Fist, grip, pin, sweep, heart contact, cage fracture and seal have separate
recipes. Added distinct gaze discharge, beam warning/ignition and five crossing
families: capsule double knock, reef creak, sweep warning/pass and debris.

Warnings retain their 80% distance floor and protected TELL role. Pin/counter,
cage break, seal and detonation keep priority and a 65% floor. Ordinary contacts
use 32%; moon launches use 16%, a 0.65 trim, and can be ducked or stolen. Pin
friction uses a 0.55 trim and ordinary spatial/crowd treatment. Capsule knocks
and reef creaks are -18 dBFS foley with the CHATTER role, stepping back under a
warning. No chapter-wide music or world-bus increase was applied.

Tracked wind-ups and tension release over 12 ms on cancellation, including the
return sweep's second warning. A queued sound is cancelled before onset;
repeated stop requests cannot prolong the release. Tests cover 22.05/44.1/48 kHz.
The finale score and Overdrive bed now end at the actual internal explosion,
rather than continuing until the capsule checkpoint 7.5 seconds later.

The current mixed, stress and crossing reels peak at 0.573, 0.685 and 0.463;
FFmpeg's oversampled true peaks are -4.8, -2.2 and -6.7 dBTP. Mono downmix changes
their energy by -0.06, -0.01 and -0.50 dB respectively. These are scripted mixes,
not proof that a human can recognize every cue under fire.

For the preserved local baseline, generate an alternating BEFORE/AFTER reel:

```sh
python3 tools/gen_shepherd_audio.py --compare build/chapter6-audio-before
```

This writes `build/shepherd-audio/shepherd-comparison.wav` and `comparison.md`
with timecodes. Both versions retain the same mix-class target. The full mix
uses gameplay weights and all available variants; the comparison isolates
timbre. Headphone/speaker preference and recognition still need human audition.

Revision checks: `make test-audio` passes all 78 asset checks and device-rate
regressions. `make test-shepherd-runtime` passes 182 checks, including the new
emission/timing, contact, cancellation and ending tests. All 78 variants are in
the rebuilt web Ogg package. Both native callback captures load all 178 sound
families; the stationary court (18 s) and crossing (12 s) measure -3.5 and -6.7
dBTP at the local master 0.8/music 0.6 settings. Browser callback checks pass at
48 kHz for both starts with real firing input, no non-finite samples, and sample
peaks 0.6664 (court) and 0.6649 (crossing). The browser sink and native driver are
silent; these verify execution and signals, not physical-device playback.
The complete `make test` regression suite also passes.


## November 17: weight, movement and audible musical changes

The November 16 pass did not address the balance the player actually heard.
Footsteps used a close PLAYER-class sample at roughly half gain, while distant
boss contacts were reduced to a 32% floor. More seriously, the custom body ray
hit applied damage and sparks without emitting a dedicated impact, and the
moving torso/arms had no audio integration. Long WAV files still had very fast
natural decays. Music phases shared their opening count and lead instrument,
and boarding the giant kept the crossing track until the seventh arena.

- Chapter 6 steps are reduced 12 dB (including wading), normal landings 6 dB.
  Gun, sword and parry volumes are unchanged.
- Giant contacts and damaging hits use an 85% audibility floor, priority and
  protection from background ducking. The real mixer regression requires
  distant smacks to exceed close footsteps by 12 dB, damaging hits by 10 dB,
  even facing away in vacuum.
- Body hit detection now emits normal/heavy contact feedback, including forks,
  captive moons, grips and explosive body damage. An 85 ms aggregation window
  prevents a shotgun pellet burst from producing ten simultaneous contacts.
- Rebuilt giant hits with longer low/mid structural modes, secondary cracks and
  debris. Smacks last 1.45 s, counters 1.6 s and cage breaks 2.1 s. Tests measure
  actual energy after 300 ms, not just the nominal file length.
- Torso and arm creaks follow velocities measured from the real simulated pose.
  Sources follow their corresponding body part, with a 55% audibility floor;
  cooldowns bound voice density. A stationary pose stays silent. Movement
  handles release on phase changes, death and retry.
- Boarding the giant now selects COUNTERWEIGHT immediately. Control has a
  deliberate four-on-the-floor count; resistance starts its higher answering
  melody over breaks, with a saw/pulse lead; collapse uses an angular octave
  figure, a wider lead and hard/hybrid drums. The last count reduces to a sparse
  two-kick pulse. All stay at 174 BPM. Phase changes request the new arrangement
  immediately and apply on the next bar, preserving the clock.

There are now 30 families / 90 deterministic variants. `make test-audio` passes
format/mix/onset checks, impact-versus-step ratios, sustained impact energy and
all full-form music renders at 22.05/44.1/48 kHz. A new rendered-stem check compares
all three combat movements' first two bars: pairwise correlations are -0.082,
0.009 and 0.032, so their melodic/instrumental changes are present immediately.
`make test-shepherd-runtime` passes 192 checks, including actual body-hit emission,
pellet aggregation, pose-driven movement, movement cancellation, all live music
movement selections and the boarding track change.

For an actual gameplay mix rather than just a scripted cue sequence:

```sh
OVERDRIVE_AUDIO_AUDIT=build/shepherd-audio/live-playthrough.wav make test-shepherd-playthrough
```

The input pilot completed the real fight and ending in 72.13 s: 35 trigger pulls,
four pin attempts, one Overdrive, lowest player HP 77, no god mode. Its WAV
contains actual walking, weapons, body contacts, giant animation, counters and
music changes through all three movements. The score/effects/DriveBed/room buses
advance on the simulation's real 60 Hz clock; it excludes wind/rope postprocessing
and generated enemy voices. This is an automated input run, not human play or a
physical-device listening claim.

The new boss mix and weapon stress reels also include walking, normal/heavy
body hits and giant motion. Their sample peaks are 0.648 and 0.685, true peaks
-2.8 and -2.3 dBTP. The gameplay capture measures -2.4 dBTP. These are final-output
signals; none of the measurements substitutes for listening to the materials.

An alternating comparison against the preceding pass is available locally:
`python3 tools/gen_shepherd_audio.py --compare build/chapter6-rebalance-before`.

Native and web builds pass. All 90 variants are present in the web Ogg package.
The actual browser callbacks pass at 48 kHz for the court and crossing, with
no non-finite samples or browser errors; peaks are 0.6666 and 0.6660. A smaller
`build/shepherd-audio/live-playthrough.mp3` copy is available for convenient
audition alongside the original WAV.


## November 17 follow-up: silence and distinct attack materials

Player feedback exposed a creative failure in the preceding movement pass:
three long filtered-noise motion samples could retrigger every 1.35/2.4 s while
attack effects used similar noise swells. Their overlap made the boss read as
constant whooshing, despite correct levels and event timing.

Removed the sustained noise from idle mechanisms, attack warnings, sweeps,
grips, gaze discharge, gather, tension and ordinary impact tails. Noise remains
only in brief contact transients, environmental effects and final implosion.
The attack language now uses separate materials and gestures:

| Action | Audible gesture |
|---|---|
| Idle torso | Two quiet latch clunks; 0.32 s total |
| Idle hand recovery | Three short ratchet contacts; 0.48 s total |
| Fist wind-up | Two low piston catches, 210 ms apart |
| Arm sweep | Four accelerating chain-winch teeth, followed by torn teeth and a carriage thud |
| Grab | Three closing finger latches at 0, 90 and 300 ms |
| Gaze | A glassy FM dyad, dry lock and electrical discharge |
| Gather | Three descending pressure knocks at 0, 250 and 500 ms |
| Orbital launch | A short ceramic/glass resonance |
| Memory mark | One hollow registration tap per actual recorded position |
| Pin/counter | Spike bite, beating pitched tension, then a low contact and bright cage fracture |

Motion is FOLEY/CHATTER at -18 dBFS, with a 25% floor. One mechanism can play
at a time, followed by at least 6.5 s before the next. Generic motion is suppressed
during attacks, the entrance, movement breaks and grabs. Any already-playing
idle mechanism fades over 35 ms when an attack begins. The real pose still
controls whether a mechanism sounds; stationary poses do not emit fake activity.

The real runtime checks now require moving idle to be more than 90% silent over
30 s, with no overlapping mechanisms, at 30/60/120 Hz. They also require attack
animation to emit no generic idle layer. Asset checks measure separate 2/4/3/2/3
pulse groups for fist/sweep/grab/gaze/gather across all three variants. These are
technical checks on rhythm and density, not claims of subjective recognition.
All 90 asset checks and 199 runtime checks pass. The real input pilot still
finishes in 72.13 s with 35 trigger pulls, four pins and one Overdrive.

`make shepherd-audio-preview` now also writes
`build/shepherd-audio/attack-language.wav` and `attack-language.md`. The 34-second
preview contains no music, guns or footsteps: quiet idle at 0.5 s, fist at 4,
sweep at 9, grab at 14, gaze at 19, gather at 24 and counter at 30. It lets a
listener compare the materials and the silence between gestures directly.
The new full-gameplay capture is `live-playthrough.wav`; MP3 copies of both
are available locally. The previous gameplay mix and cue assets were preserved
in `build/chapter6-before-signatures/` for comparison.


## November 17: measuring audibility instead of guessing (tools/listen.py)

Earlier passes checked cue levels in isolation. `make listen-ch6` now records the
real input-pilot fight with separate stems (music, effects, beds, and a focus stem of
exactly the `shep_`/`ring_` cues as mixed) plus a timed cue log, then measures each
cue against everything else in its own frequency bands. It found the player's rifle
family (-17.1 LUFS, 81% of energy at 250 Hz-4 kHz) masking the boss (-22 LUFS, 4% at
1-4 kHz), not the music (-20.7 LUFS, 93% below 250 Hz).

Changes: every warning gets a short inharmonic 2-5 kHz ping layer whose pitch shape
is the attack's (fist falling pair, sweep climbing, grab closing, gather downward
glides, beam rungs, memory tick); wounds get a bright armour crack and keep their
structural ring; tells/wounds open their low-pass to 11 kHz. In the mixer, a tell or
protected boss impact/reward dips PLAYER and unprotected WORLD voices by 4 dB
(SIDE_PLAYER). Same deterministic fight: masked boss cues 20 -> 9 of 59; fist
wind-up +2.2 -> +12.9 dB over everything else, grab +0.4 -> +6.0, sweep +0.5 -> +3.9.
Gun family is 3 dB lower over the fight; overall mix -1.3 LU. Still masked: some
heavy wounds during rapid fire, one gather, one memory mark. Human audition decides.

Audition round 1 (user, same day): B clearer for grab, memory, beam and both wounds;
lock/parry/gaze no difference; sweep worse than the original; fist and gather unclear
("a de do kind of sound"). The pure-tone pings read as a chime. Round 2 takes the
brightness from the recorded clank's 1.8-7.5 kHz band instead (FIST_BITE/GATHER_BITE,
capped at 60 ms so the recording's bounce never adds a pulse), keeps both fist catches
at one pitch, and restores the original sweep. Fist +10.0 dB over everything else;
gather still masked (-2.8) and awaits a new concept from the user.
