# SUNSET, ZENITH and MORNINGSTAR music pass

Implemented November 9, 2026, following the [soundtrack review](soundtrack-review.md).
The boss half-time breakdowns and the level trims described here were replaced on
November 11 by the [boss music and loudness pass](boss-music-pass.md).
This pass changes composition, arrangement and output headroom. It does not copy
recordings or melodies from reference games.

## Music

SUNSET has an explicitly pitched eight-bar call and answer, with varied note
lengths, rests and velocity, inside a sixteen-bar form. It enters at ordinary
combat intensity 1.0 instead of requiring boss intensity. A rounded pulse/sine
voice distinguishes it from the acid backing. Four kick, snare and hat patterns
develop the break, with a two-bar reduction before the return.

ZENITH and MORNINGSTAR share the opening minor-third/fifth motif and develop it
differently over thirty-two bars, approximately 45 and 43 seconds respectively.
Their original tempo and keys remain. Both have a half-time breakdown, a filtered
build, peaks with an upper voice, and a turn into the return. MORNINGSTAR has a
busier falling answer, different acid figures and syncopated peak kicks. These
forms repeat on the live sequencer rather than replacing it with recorded music.

Boss acid lines have authored rests, accents and connected slides. Boss harmony
uses a raised dominant third to lead home. Section gains and filters ease between
targets; note attacks and releases are smoothed at the device sample rate. The
theme leaves deliberate gaps for combat sounds. Overdrive, the last-enemy solo,
wave stingers and pause filtering continue to respond to gameplay. Their levels
remain independent of the form: the score cannot turn a calm room into full
combat by itself.

The long form also tracks tape rewind across a four-bar boundary. Returning from
the menu resets the form and clears the outgoing track's tails as before.

## Output and measurements

Selected music received a small pre-saturation trim. The actual SUNSET gameplay
capture still measured **+1.27 dBTP** from the combined music/effects output,
despite PCM peaks below full scale. The final callback now applies a constant
0.7 gain after rounding off the complete sum, reserving approximately 3.1 dB
for reconstruction. This affects music and effects together and preserves their
relative balance. It is headroom, not a guaranteed true-peak limiter.

Fresh gameplay captures after that adjustment measure **-1.96 dBTP** for SUNSET
(24 seconds), **-2.42 dBTP** for ZENITH (48 seconds), and **-2.12 dBTP** for
MORNINGSTAR (48 seconds). The actual Web Audio callback also passed at 48 kHz:
740,352 finite channel samples, RMS 0.0985, peak 0.6727. These observations
cover the tested encounters, rather than guaranteeing every possible mix.

The following standalone renders use the real synth at preview volume 0.9,
64 seconds long. SUNSET uses intensity 1.0; bosses use 1.3. FFmpeg measurements
include the complete form and its return. Loudness range describes the rendered
audio; it does not establish subjective quality.

| Track | Before LUFS | After LUFS | Before / after LRA | After dBTP |
| --- | ---: | ---: | ---: | ---: |
| SUNSET | -12.85 | -14.78 | 0.5 / 1.6 | -1.06 |
| ZENITH | -13.93 | -14.57 | 0.4 / 3.3 | -5.36 |
| MORNINGSTAR | -14.60 | -16.15 | 0.2 / 3.4 | -6.37 |

Comparing two-bar rendered sections at 44.1 kHz, peaks stand approximately
2.6 dB above SUNSET's breakdown, 5.4 dB above ZENITH's, and 3.3 dB above
MORNINGSTAR's. Theme-muted diagnostic renders verify that the theme contributes
to each mix at its actual combat intensity.

## Audition and validation

```sh
make test-audio
make tools/music_preview
python3 tools/music_score_preview.py --baseline-dir build/music-pass/before
# Without a saved baseline, omit --baseline-dir to render the new score only.
tools/music_preview 10 48 build/zenith-controls.wav 1.3 --exercise --rate 48000
```

[Local audition files](../build/music-pass/README.md) contain one matched A/B MP3
per track, full WAVs, theme-muted diagnostics, control exercises and measured
levels. Each A/B plays the old track first, a one-second gap, then the new track;
both excerpts use constant gain to reach the same integrated loudness. There is
no extra compression in the comparison. The new excerpts include each full form.
These generated assets remain ignored by Git.

The score regression renders 782 seconds across 22.05, 44.1 and 48 kHz. It checks
complete forms and returns, finite output, PCM headroom, actual section contrast,
theme contribution, identical output under irregular callback sizes, pause
filtering, form reset and rewind across a four-bar boundary. The existing audio
suite covers all eighteen tracks, transitions, solo, stingers and Overdrive.
Gameplay WAVs exercise the final SDL callback with music, weapons, enemy sounds
and environment audio.

The full native `make test` suite, WebAssembly release and pointer-lock checks,
browser gameplay smoke tests, and the website's 35 tests and build/link
verification pass. The Web Audio signal check is reproducible with
`OVERDRIVE_PLAYWRIGHT=/path/to/playwright/index.mjs node tests/test_web_audio.mjs http://127.0.0.1:8876/ build/music-pass/web-audio.json`
against a local `web/dist` server.

The tests and measurements do not replace an ear check. No subjective listening
judgment was available in this session; the A/B and gameplay files make that
remaining check concrete.
