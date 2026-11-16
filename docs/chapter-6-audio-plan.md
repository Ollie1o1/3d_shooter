# The Shepherd: audio plan

The fight should sound like an orbital machine trying to keep its own body in
place: measured counts, tension in metal, captive masses moving, then failure.
Attack recognition and satisfying counters come before extra ambient detail.

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

The first cue pass is implemented: 18 original cue families, three deterministic
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

Run `make shepherd-audio` to regenerate the 54 WAVs and a labelled cue reel.
Run `make shepherd-audio-preview` for two 42-second stereo reels through the real
MusicSynth, SfxMixer, DriveBed and final output saturation/headroom gain:

- `build/shepherd-audio/shepherd-cues.wav`: isolated cue families, with timecodes
  in `build/shepherd-audio/README.md`.
- `build/shepherd-audio/shepherd-mix.wav`: cue sequence over the corrected boss
  intensity at the default music slider. Seal at 30 seconds, detonation at 31.5,
  and the capsule ending's music silence at 39.
- `build/shepherd-audio/shepherd-stress.wav`: the same sequence with repeated
  revolver fire and Overdrive layers/focus from 18 to 25 seconds.

The reels use scripted source positions and cue timings, not a recorded human
playthrough. They exclude room/wind beds and other enemy voices. The preview
reports sample peaks; it does not measure true peaks or subjective readability.
Physical speaker/headphone audition is still required.

Technical verification: all 54 WAVs pass format, onset, variant, tail, headroom
and mix-level checks. The real mixer test keeps a warning audible 240 metres
behind the listener through vacuum, an 18 dB background duck and Overdrive focus.
Runtime tests cover cue cancellation and the music-intensity regression. Native
and web builds, the full regression suite, render checks, audio checks and 131
Shepherd runtime checks pass. All 54 variants are present as Ogg in the web
package. The updated adaptive mix and stress reels peak at 0.568 and 0.681 respectively
after the game's output stage. FFmpeg measures -4.4 and -2.6 dBTP respectively.
No physical-device listening was performed.

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
