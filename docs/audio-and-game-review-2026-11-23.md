# Audio and game review: November 23 2026

Written while Oliver was away ("i am going idle so i am trusting you fully").
Branch `audio-real` (local, not pushed). The first commit on it preserves the
uncommitted Chapter 6 audio work exactly as found on `ship`.

## Why the audio never sounded right

Five agent passes (Nov 9–17) worked on the sound. Each one measured levels,
masking, onsets and spectra, and each one still produced sound that read as fake
("a de do kind of sound", "constant whooshing", "a chime"). The root cause is
not the mix code. It is the source material:

1. **Most sounds were synthesized.** The explosion was 0.6 s of white noise over a
   sine. Body hits were a click and a sine thump (0.19 s). Surface impacts, slams,
   landings, getting hit, kill confirms, enemy voices (all of `VoiceSynth.h`) and the
   whole Chapter 6 boss (90 files from `gen_shepherd_audio.py`) were recipes of
   oscillators, noise and filters. Synthesis can match a level or a spectrum, but
   not the irregular detail of a real event (air, debris, room, mechanical noise).
   That detail is what the ear uses to judge "real". An agent that can't listen
   can't tune its way out of it. It needs recordings.
2. **The mix distorted every loud moment.** The final sum went through tanh twice
   (in `SfxMixer`, then again in `AudioSystem`) and was then scaled by 0.7. Anything
   over 0.8 was bent: an explosion over the music or a shotgun beside a boss hit
   came out crunchy. The 0.7 kept everything else quiet.
3. **Everything is short and mono.** Most effects were 0.1–0.6 s with no tail, so
   nothing had weight or space. Real blasts take about 2 s to settle.

The process lesson: **source real recordings, layer and process them; don't
synthesize foley.** Synthesis is fine for music (techno *is* synthesis) and for UI
blips. Everything physical should start from a recording.

## Done in this pass

- **`tools/build_real_sfx.py`**: builds sounds from CC0 recordings like a sound
  designer would: a transient, a body and a tail or debris layer from different
  recordings, then compression, limiting, a natural decay and per-family level
  matching to MixTable. The sources are frozen in `assets/sfx/src/cc0/` (3.4 MB,
  credited in `assets/sfx/CREDITS.md`). The build takes 4 s.
- **Replaced** (three variants each): `explosion` (2.3 s, was 0.6), `slam`,
  `hit_body` (every round that lands), `hit_armor`, `player_hit`, `impact`,
  `impact_metal`, `impact_wood`, `impact_ice`, `land`, `land_heavy`. These are
  the most-played sounds in the game: `explosion` has 49 call sites and `slam` 32.
  MixTable trims are updated.
- **`src/audio/MasterBus.h`**: a look-ahead (1.5 ms), stereo-linked peak limiter at
  -3.1 dBFS (the mix's old 0.7 output contract) replaces both tanh stages. Under the ceiling it is a pure gain (tested),
  and it never passes the ceiling (tested with 6x spikes). The preview tool and the
  playthrough audit use the same bus.
- **A/B reel**: `build/audition/real-sfx-ab.mp3` (timecodes in `real-sfx-ab.md`).
  Each family plays the old synth sound once, then the three new variants, at
  in-game levels. **Listen to this first.**

## What to do next for audio (ranked)

1. **Oliver listens to the A/B reel** and plays a fight. Every judgement in this pass
   was made from spectrograms and measurements, not ears.
2. **Enemy voices** (`VoiceSynth.h`: about 30 enemy types x idle/tell/attack/hurt/death)
   are the next biggest synthetic layer and play constantly. Replace them with
   processed recordings (CC0 creature, robot and servo packs, pitched and layered per
   enemy), keeping the code's roles and tells.
3. **Chapter 6 boss cues** (`gen_shepherd_audio.py`, 30 families) should follow the same
   route: recorded metal stress, machinery and large impacts, pitched down. The mixing
   and timing code from the Nov 16–17 passes is good and stays.
4. **Stereo for the player's own sounds**: guns, near explosions and the sword are
   folded to mono at load (`addConverted`). Stereo gunshot tails would add width.
5. Movement foley (jump, dash, slide, wade) and pickups are still synth: lower
   priority, since they are short and quiet.
6. Kill confirms and hit markers are UI and can stay designed tones (a genre
   convention).

## Whole-game review (from the docs, the hand-off and the code)

The game is huge (134k lines, 6 chapters, a dozen bosses) and its tests are
thorough. The gaps are play-feel and closure, not features:

1. **Chapter 6 needs Oliver's play notes.** The whole Shepherd fight (five phases,
   the head, the ending) is a first pass that hasn't been hand-played since the
   Nov 15–16 rebuild. Nothing else should be built on it until it has.
2. **Unfinished ending beats** (chapter-6-build.md): the death is a cut (no
   first-person spike animation, no jump clear), the capsule has no UNIT 24
   marking, the giant doesn't rise to meet you, the Shepherd's lines are banners
   only, and the bridge cutscene from the Cold Store to the ring is missing.
3. **Known rough edges**: Strays use full gravity in low-gravity levels, the Drift
   has no fight, unused `shepLamp*` members and `shep::fallLamp` are left over, the
   head's arms are plain boxes, and from 100 m it reads smaller than it should.
4. **Branch hygiene**: `ship` = `origin/main`, but local `main` is 3 weeks old,
   there are about 70 local branches, and the Nov 16–17 audio work had never
   been committed (it is now preserved on `audio-real`).
5. **Disk**: `build/` holds 5.2 GB of frame dumps and audio captures, with
   6 GB free on the disk. Oliver should clear `build/anim`, `build/polish`,
   `build/navshots` and old captures (the permission rules deny `rm` to the agent).
6. **Performance at 4K EXTREME** is pixel-bound (about 15.6 ms GPU on the reef). The
   default HIGH is fine.
