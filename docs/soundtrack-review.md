# OVERDRIVE soundtrack review

Reviewed November 9, 2026, against the current source after the combat pass.

This records the baseline before the subsequent [music pass](music-score-pass.md).
That pass implements SUNSET's ordinary-combat theme, the two longer boss
arrangements and output headroom; the measurements below describe the baseline.

## Main recommendation

Prioritize recognizable themes, distinct rhythmic identities and longer musical
development. The engine already provides a capable foundation: live layering,
Reese bass, acid synthesis, antialiased oscillators, a humanized formant choir,
organ, brass, stereo effects, phrase variation, Overdrive layers and a musical
rewind. The next substantial improvement is likely to come from what those
instruments play and when the player hears it.

This is a composition, implementation and measurement audit. I cannot directly
audition audio in this session, so it does not establish subjective listening
quality or claim that one track sounds better than another. Recommendations
about memorability, emotional impact and masking should be checked by listening.

## Material inspected

- All 18 definitions in `src/audio/MusicSynth.h`, including the separate
  RECLAMATION engine and the clinical choir/organ tracks.
- Musical routing and intensity in `Gameplay_Flow.h`, plus menu intensity in
  `main.cpp`, and the music/effects/output path in `AudioSystem.h` and `SfxMixer.h`.
- Fresh 64-second renders of every track at intensity 1.3, preview volume 0.9.
- Additional renders at ordinary-combat intensity 1.0, RECLAMATION approach
  intensity 0.78, Cold Storage exploration intensity 0.3, and menu intensity 0.55.
- A 24-second capture of the actual Combat Remix output, including music,
  weapons, enemy audio and effects, with local master 0.8 and music 0.6.

[Audition notes and files](../build/soundtrack-review/README.md) include a
[matched comparison reel](../build/soundtrack-review/comparison-reel.mp3), the
original renders, a catalog and loudness measurements. These build artifacts
are local and ignored by Git. Reference-game recordings were not copied.

## Concrete findings

### 1. Many tracks share the same rhythmic foundation

Eleven tracks have `breaks=true`. All eleven use the same base snare pattern
and the same base hat pattern. Several also share the kick and bass patterns.
There are additional programmed fills, ghost accents and rides, so this does
not mean the audio is an identical one-bar loop. It does mean that much of the
rhythmic identity is shared before a track's harmony and instrumentation enter.

Sixteen of the eighteen tracks run between 170 and 178 BPM. Cold Storage at
96 BPM and the menu at 150 BPM provide exceptions. Keeping fast tempo is
appropriate for the game; varying perceived pulse, rests, accents and drum
orchestration would produce more contrast without changing player speed.

FOUNDRY and ZENITH are an especially useful comparison: both are at 172 BPM,
root MIDI 50, with chord degrees `{0, 0, 5, 6}` and related techno voices.
Their patterns do differ, but a stronger difference in riff and arrangement
would help a boss track establish its own role.

### 2. Ordinary arena fights usually suppress the authored lead

The general engine computes `gLead` with a smooth transition between intensity
1.12 and 1.32. Ordinary fights usually request 1.0. Boss fights and arena
Overdrive can request approximately 1.35.

Consequently, many normal fights feature the drums, bass, pad and arpeggio
while the dedicated lead line is absent. The clinical choir and RECLAMATION
have additional melodic behavior, so this is not a claim that all ordinary
music lacks melody.

Test a short recognizable motif at ordinary-combat intensity, reserving a
complete melody, counterline or changed harmony for the peak. Preserve room
for attack tells rather than simply adding a loud lead throughout every fight.

### 3. Much of the generic writing uses short chord-tone templates

Most ordinary tracks use four repeating chord entries, one-bar instrumental
patterns and a two-bar lead template whose note symbols select chord tones.
The engine adds an alternating B treatment, octave changes and a sixteen-bar
filter/energy phrase. Those are useful arrangement features, but they do not
provide the same development as an authored answer phrase, new riff, bridge,
rhythmic interruption or harmonic turn.

At 174 BPM, four bars last about 5.5 seconds. A player can experience the
underlying harmonic cycle many times in an encounter. The solution can be
an intentionally repetitive groove with evolving sound design, or a longer
composition; each track should make that choice deliberately.

### 4. RECLAMATION offers an existing model for more authored writing

RECLAMATION has a dedicated eight-bar orchestral progression, an explicitly
pitched horn theme with varied note lengths, chromatic material, and a slower
orchestral clock against the club rhythm. Its exploration/fight orchestration
also changes with intensity.

That makes it a promising structural starting point for a flagship theme.
This assessment follows its composition data, not a listening ranking.
The clinical tracks also have a longer soprano answer, which is a strength
to develop rather than replace with generic techno.

### 5. Boss escalation often changes layers rather than musical sections

The Successor's phases alter intensity, as do the Restorer's. Overdrive adds
stabs and hats; common encounters have a last-enemy reduction and a clear
stinger. These are already meaningful interactive features.

The opportunity is a more recognizable musical payoff: a new answer to a
theme, a changed break, a harmonic arrival or a deliberately sparse interruption.
For suitable events, test introducing a new musical section at the next bar
or phrase boundary. Damage cues and attack warnings must remain immediate;
musical quantization does not mean delaying gameplay.

### 6. Track levels need another balancing pass

FFmpeg loudness measurements below exclude the first two seconds. Most renders
use intensity 1.3, with named exceptions. All isolated renders use preview
volume 0.9, so these are comparisons within this audit, not final in-game levels
or commercial soundtrack mastering targets.

| Render | Integrated LUFS | True peak dBTP | Loudness range LU |
|---|---:|---:|---:|
| AFTER HOURS, actual menu intensity 0.55 | -8.40 | -0.85 | 0.3 |
| SUNSET, ordinary combat 1.0 | -12.82 | -0.12 | 0.5 |
| FOUNDRY, ordinary combat 1.0 | -13.36 | -4.64 | 0.4 |
| ZENITH, 1.3 | -13.93 | -4.75 | 0.4 |
| MORNINGSTAR, 1.3 | -14.60 | -5.74 | 0.2 |
| RELEASE DENIED, 1.3 | -11.81 | -0.21 | 0.6 |
| RECLAMATION, 1.3 | -10.18 | -1.42 | 1.3 |
| COLD STORAGE, exploration 0.3 | -18.68 | -5.91 | 2.0 |

The menu is approximately 4.4 LU louder than ordinary SUNSET at the same
preview volume. RECLAMATION at high intensity is also approximately 4.4 LU
louder than MORNINGSTAR. Verify whether those differences serve the experience;
test per-track trims before changing the global music slider.

Most steady combat renders have little measured loudness movement. That is
not automatically a defect in dance music, and LRA does not measure harmonic
or emotional variety. It does make arrangement contrast a useful listening test.

The real combat capture measures -10.73 LUFS with +1.01 dBTP reconstructed
true peaks, while its PCM sample peak remains below zero, around -0.38 dBFS.
This indicates intersample overs rather than numerical sample overflow.
Test final output headroom and a true-peak ceiling, such as -1 dBTP, before
increasing loudness. The signal passes soft clipping in the synth and on the
effects/final buses; the latter stages act only above their knee. A bypass
comparison would establish their audible effect. These measurements alone do
not establish audible distortion or music masking an attack cue.

## Comparisons and what to borrow

### ULTRAKILL: contrast and thematic identity

The [official INFINITE HYPERDEATH album](https://heavenpierceher.bandcamp.com/album/ultrakill-infinite-hyperdeath)
places its original music alongside classical material including Debussy's
Clair de Lune. The useful lesson for OVERDRIVE is permission to create strong
contrasts across its world, while giving important characters and places
recognizable musical ideas.

Develop the factory's rhythm, the ward's choir, the Gleaner's horn and the Cold
Store's fragile motif into a coherent musical story. A recurring UNIT 24 phrase
could appear damaged in the factory, enlarged against the Gleaner and altered
in the restored copy's confrontation. Preserve original themes and sounds.

### DOOM: a distinctive instrument palette serving authored music

Mick Gordon's [DOOM: Behind the Music presentation](https://www.gdcvault.com/play/1024068/-DOOM-Behind-the)
covers musical sound design, composition, interactive music and mixing together.
The useful application is to give a track both an identifiable timbre and a
musical idea worth recognizing.

OVERDRIVE's own machinery offers material: a pitched press, bowed metal,
furnace pulse, damaged relay or processed machine choir. Pick a signature sound
and write a compelling riff with it. Additional distortion alone will not
create the riff or its development.

### Metal: Hellsinger: make a musical arrival feel earned

Funcom's [official gameplay music video](https://www.youtube.com/watch?v=vxdsuI-gxwg)
describes layers tied to performance and vocals entering at peak performance.
For OVERDRIVE, a recognizable melodic arrival in Overdrive could make the
reward identifiable by ear. Its current additional stabs and hats already
provide a foundation. Literal vocals and rhythm-locked shooting are optional;
the musical payoff is the relevant idea.

### The two possible Blade references

The intended title was not yet clarified during this review, so these remain
separate directions:

- **Blade / acid-techno direction:** the official New Order release of
  [Confusion (Pump Panel Reconstruction Mix)](https://www.youtube.com/watch?v=-9dsrmGG4_o)
  is a useful reference for the acid approach associated with that comparison.
  ZENITH and MORNINGSTAR already have acid-line infrastructure. Audition the
  kick/bass/acid trio on its own and develop a recognizable phrase, accents,
  slides, filter movement and longer tension/release. Repetition can be the
  point when the phrase and its evolution carry it.
- **Stellar Blade:** in a [2024 developer interview](https://www.pushsquare.com/news/2024/02/interview-shift-up-on-ps5s-highly-promising-stellar-blade),
  Shift Up describes music expressing its world and the prominence of human
  voices and choruses. For OVERDRIVE, explore a more expressive foreground
  choir phrase or solo answer in the ward and Cold Store. Its synthesized
  choir is already a useful bridge to that emotional role.

## Track-specific next passes

| Track or family | Keep | First experiment |
|---|---|---|
| SUNSET / CORE / SPIRE | Fast movement-oriented foundation | One memorable ordinary-combat hook; distinct snare/accent patterns for each place |
| FOUNDRY | Industrial pulse | Give machinery percussion and bass a rhythmic signature separate from ZENITH |
| NAVE / ORRERY / RELIQUARY | Spatial bell/arp ideas and sparser writing | Make one dominant motif unmistakable; vary perceived pulse and rests |
| ZENITH / MORNINGSTAR | Acid voices and the escalation relationship | A strong acid hook, a longer build/drop, and an altered phrase for the second form |
| OBSERVATION / RELEASE DENIED | Shared clinical choir identity | Preserve the relationship, but give the boss an answer phrase and phase-specific arrangements |
| RECLAMATION | Longer horn writing and independent orchestral/club clocks | Develop the horn theme into the score's flagship and balance its loudness against other peaks |
| COLD STORAGE / THE RESTORER | Quiet contrast, choir and the rewind mechanic | Transform a fragile exploration motif into the boss's musical argument |
| AFTER HOURS | A coherent menu groove | Test a lower per-track trim and check that entering combat feels like an increase in energy |

## Suggested order

1. Match reference loudness for listening, then decide the game's intended
   per-track levels. Verify the final mix's true-peak headroom.
2. Compose one memorable SUNSET motif audible during ordinary combat.
3. Write one flagship ZENITH/MORNINGSTAR arrangement with a clear hook,
   an authored answer or breakdown, and a satisfying peak.
4. Extend the successful approach to a boss with several phases, then to
   the remaining areas. Give each family a deliberate rhythmic identity.

Composing in a DAW and importing authored note/section data would still allow
the game to synthesize the result live. The important change is an authoring
format that can express longer phrases, velocities, rests, articulations and
sections without making every track fit the same small template.

Use the matched reel for an identity test: hide track names, identify the
place or boss, and try recalling a phrase after playback. Compare full mixes
for warning recognition and weapon impact. Those listening results should
determine the next implementation pass.
