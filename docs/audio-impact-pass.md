# Combat sound and techno/DnB pass

The direction is industrial steel over fast drums: a short, readable strike,
a clear recorded blade transient and a material-specific tail. Body impacts carry the low weight. Parries are a
bright reward, blocks a lower plate clang, and a broken guard a cracked,
falling sound. The music supplies forward motion while leaving room for combat.

## Research translated into the game

- [Krotos founder Orfeas Boteas on weapon design](https://www.asoundeffect.com/weaponiser-weapon-sound-effects/)
  describes combining recorded layers and sweeteners and varying pitch and
  envelopes. The non-contact cues combine noise transients, falling low oscillators,
  inharmonic metal modes and the existing CC0 metal recording. Sword contacts
  use the real blade recordings described below.
  Three separately excited variants per cue work with the mixer's no-repeat picker.
- [TC's DnB Academy](https://www.tcdnb.com/academy) discusses 174 BPM, contrasting
  bass parts, ghost snares, shorter drum tails, and phrase variation. Combat tracks
  now run at 170–178 BPM, with two-step kicks, quieter ghost notes, pickup kicks,
  occasional rides and a moving Reese bass. Techno areas keep four-to-the-floor
  patterns and acid lines. Sixteen-bar filter phrases create a turn and return.
- [Ableton's compressor/sidechain documentation](https://www.ableton.com/en/live-manual/12/live-audio-effect-reference/)
  explains making space for the kick's attack by ducking bass. The DnB bass now
  ducks under both kick and snare; its low end stays centered while upper
  harmonics widen. Sustained techno kicks receive an intensity-dependent trim
  because offline renders measured them substantially louder than the breaks.

The exact synthesis recipes, mix depths and feedback timings are this project's
design choices. These research references supplied techniques. Recorded source assets are credited below and in `assets/sfx/CREDITS.md`.

## What changed

- Rebuilt sword draw, swing, thrust, guard, cut, block and break sounds.
  The three combo swings change pitch; the third cut has a separate heavier
  steel contact and a stronger blade reaction. Ordinary cuts pause only the
  weapon for 25/45 ms while movement and the world continue. The thrust has its own
  piercing hit; the plunge uses the heavy impact.
- Rebuilt normal parries and added heavy parries. They briefly dip the background
  by 4/6 dB for 75/120 ms. The reward is a priority player sound; enemy attack
  warnings bypass the dip. Heavy sword hits have a smaller, shorter dip.
- Rebuilt body and armour impacts. Existing mass-dependent body pitch and pellet
  aggregation still place one weighted body thump at the hit.
- Wall shots choose stone, steel, wood or ice from the nearest wall's material
  during the existing raycast. Chapter-specific hull, surgical, salvage and
  froststeel surfaces map to steel. Only the first two pellets sound, as before.
- Authored 17 cue families × three deterministic WAV variants. Each is matched to
  its existing mix class with headroom, a fast onset, DC removal and faded tails.
  MixTable loads the six new sound families automatically on native and web builds.
- Reworked the live score's tempo, rhythms, bass and articulation. Chapter
  melodies, lab choir/organ, Gleaner orchestra, fight-intensity layers, OVERDRIVE,
  final-enemy reduction, stingers, pause filter and rewind remain integrated.
  Cold Storage's quiet exploration track stays at 96 BPM; the Restorer runs at 174.
- Reduced SDL's requested buffer from 2,048 to 512 frames (46.4 to 11.6 ms at
  44.1 kHz). This is requested buffering, not a measurement of total device latency.

## Audition and regeneration

```sh
make                     # updated native game
./shooter --boss 5 --sword # Absolute duel, sword available on key 5
make audio-preview       # build/audio-preview/: combat reel + three 32-second tracks
make combat-audio        # regenerate the 51 shipped combat WAVs
make test-audio           # offline asset, mixer and synth regressions
```

The combat reel plays one cue per second; its README gives the cue order.
For this session, `build/audio-before/` contains the original sword/parry/body
assets and original SUNSET/CORE renders. `make audio-preview` also writes a
level-matched before/after combat reel when those originals are present.

The generator uses only Python's standard library and the frozen
`assets/sfx/src/clank_rec.wav` and the three `sword_steel_*.wav` excerpts;
it never layers onto a generated output. Run
`make combat-audio` after `gen_sfx.py` or `import_sfx.py`, which still contain the
older recipes. Sound sources are documented in `assets/sfx/CREDITS.md`.

## Verification and listening limits

`test_audio.cpp` checks every new variant's format, onset under 5 ms, peak
headroom, mix level within 0.25 dB, distinct samples, clean tail and surface
mapping. It exercises the real mixer duck and every soundtrack's transitions,
stingers, OVERDRIVE, solo and rewind at 22.05/44.1/48 kHz. The full game suite
also checks all sound files against MixTable and the existing music behavior.

A native run loaded all 152 sound families and captured eight seconds through
the actual SDL callback using its dummy driver. Offline renders verify signal
levels and stability; headphone/speaker playback and whether the new timbres
match the player's taste still need a listening pass.

The native build and full `make test` suite passed. `make web` also completed;
all 51 combat variations were verified in its Ogg package, with frozen source
recordings excluded. The browser build was compiled, packaged and exercised
in Chrome, including F3 and all twenty training lessons. It was not auditioned
through a physical audio device in this pass.

Further creative directions to audition are a sharper electric edge on Oathbreaker,
more chopped jungle fills, and a more restrained industrial bass palette. The
separate cue recipes and break/techno track flags make those easy to tune.


## Sword contact revision after listening feedback

The first impact recipe put too much low thump and noise ahead of the steel.
The cut, heavy cut, thrust hit and block now use recorded Norse sword / katana /
sabre blade clashes from Ben Jaszczak and Brian Nelson / Still North Media's
CC0 Weapon Impacts library. Three isolated strikes are frozen in
`assets/sfx/src/sword_steel_*.wav`, with exact source offsets in CREDITS.md.
Their natural transient and ringing tail supply the contact sound. Synthetic
boom and crunch layers were removed from these four cue families. The soundtrack
source was verified unchanged. `make test-audio` passes for all 51 variants.

## Implemented feel follow-up

- Damage, steel contact, sparks and blade recoil now share the animation's contact
  marker: 38% through a light cut and 45% through the overhead. Anticipation starts
  immediately. Guarding or holstering cancels a pending contact. A riposte keeps
  its bonus through the anticipation, and each cut contacts exactly once.
- Shielded contact uses a resistance marker and a block clang, without damage or
  healing. A damaging cut no longer adds the bullet/body thump over the steel.
- Ordinary cuts pause only the sword for 25/45 ms. Player movement and enemy
  simulation continue; existing exceptional kill/stagger effects remain.
- Cut direction drives contact sparks and modest boss deflection without
  interrupting an attack tell. Hit flashes preserve more surface detail.
- Boss health drops immediately with a short gold damage trail. Kill/headshot
  markers retain priority over later pellets from the same burst.
- A 100 ms fire/dash input buffer accepts presses just before readiness, reload,
  switch or charge completion. It never bypasses weapon rates and clears across
  pause, death and incompatible equipment changes.

These values are project tuning choices. The
[impact-feedback study](https://arxiv.org/abs/2208.06155) identifies sound
coherence, hit-stop and camera control as promising factors; it does not
establish an optimal timing value for this game. The implementation aims for
agreement between input, animation and feedback while preserving movement.

See [the release notes](feel-training-reliquary.md) for the training crash,
20-lesson guide, Reliquary fixes and validation commands.
