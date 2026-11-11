# Boss music and loudness pass

November 11, 2026. Follows the [music score pass](music-score-pass.md) and replaces
its half-time boss breakdowns and its level trims.

## What was wrong

- The menu rendered near -8 LUFS while ZENITH and MORNINGSTAR rendered at -14.5 and
  -16.2 at the same volume: the techno trim exempted the menu and nothing else.
- Both boss forms spent four bars in a half-time breakdown (bass at 0.42, filter at
  0.35) every 45 seconds, 3 to 5 dB under their peaks.
- Every boss wind-up, hit and parry ducked the music by the full 4 to 6 dB, with the
  deepest duck held for the longest hold. In a busy boss fight the score sat several
  dB down for much of the fight.

## What changed

**Levels.** `MusicSynth::trackGain()` holds one measured gain per track, a 3:1 bus
compressor sits before the saturator, and boss intensity lifts any track by 1.5 dB.
`tools/music_levels.py` measures every track at the intensity the game plays it and
fails when one drifts more than 1 dB from its target (`--solve` prints a new table).
Targets at preview volume 0.9: fights -13.5 LUFS, bosses -12, menu -15. The 0.8
factor in `AudioSystem::setMusicVolume` is gone, which is 1.9 dB of clean gain.
Against the old scale the bosses are 4 to 6 dB louder and the menu 5 dB quieter;
a boss is now 3 dB above the menu instead of 6 to 8 below it.

**Ducking.** The music takes half of a short duck (`SfxMixer::musicShare`): a 6 dB
dip on the crowd is 3 dB on the score. Ducks deeper than 6 dB keep the music 3 dB
above the crowd, so the Gleaner's scream and the finales still clear it.

**Boss arrangements.** No half-time. A boss form changes the rhythm section at full
speed (`combatscore::Style`):

| Style | Drums and bass |
|---|---|
| TECHNO | four-on-the-floor driven kick, clap, rolling off-beat bass |
| DNB | two-step break with ghost snares, sustained Reese |
| HYBRID | the four-on-the-floor kick under the break's snares and Reese |
| HARD | hardstyle: a pitched, distorted kick whose gated tail is the bass, a screech on the theme |

ZENITH: techno 8 bars, DnB 8, a two-bar build, techno 4, hardstyle 4, hybrid 6.
MORNINGSTAR: hybrid 8, DnB 8, build 2, techno 2, hardstyle 6, DnB 6.
Builds keep the kick and roll the snare; nothing drops out.

Every other break or techno track does the same from intensity 1.1 (a boss wave, a
boss phase, OVERDRIVE): bars 9-16 of each sixteen trade to the other style, a break
track taking the four-on-the-floor kick, a techno track the two-step and Reese.

**Hardstyle** opens from intensity 1.3: boss waves, MORNINGSTAR, the Successor's
third phase, the Restorer's third phase, arena OVERDRIVE. Below that the scored
hardstyle bars play as techno. RECLAMATION (its own engine) takes the hardstyle kick
in the last two of every eight bars from intensity 1.19, which the Gleaner's fight
phases from TENDER to CORE all reach.

## Not verified

Levels, section balance and form are measured and tested. How it sounds is not:
this was written without listening. Renders for audition are in `build/boss-music/`
(`make tools/music_preview`, then see `tools/music_preview.cpp` for arguments).
True peaks of the isolated renders at preview volume reach +1 to +2.5 dBTP on the
hardest tracks; in the game the music runs at the slider's 0.6 and the final output
gain, well under that.
