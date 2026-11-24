#!/usr/bin/env python3
"""Prototype: one sustained "voice" for the Shepherd's warnings (audition only).

Round 2 audition feedback: the percussive "ding/clank" warnings are fatiguing;
warnings should be consistent, audible sounds. Each tell here is one continuous
machine voice spanning the real wind-up (ShepherdCombat.h timings), shaped per
attack. No noise, no fast-decaying transients. Two timbres: motor and horn.

    build/.listen-venv/bin/python tools/proto_shepherd_voice.py
Writes build/listen/proto/<timbre>_<cue>.wav and in-fight context reels.
"""
from pathlib import Path
import sys

import numpy as np
from scipy import signal
from scipy.io import wavfile

sys.path.insert(0, str(Path(__file__).parent))
import listen  # noqa: E402

SR = 44100
ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "build" / "listen" / "proto"
SLAM_TELL, SLAM_LOCK, SWEEP_TELL, PULL_LEN = 1.65, 0.85, 1.25, 3.2


def ease(t, a, b, t0, t1):
    u = np.clip((t - t0) / (t1 - t0), 0, 1)
    return a + (b - a) * (u * u * (3 - 2 * u))


def voice(dur, f, amp, cutoff, trem_rate, trem_depth, timbre):
    n = int(dur * SR)
    t = np.arange(n) / SR
    vib = 1 + 0.004 * np.sin(2 * np.pi * 4.3 * t)
    phase = 2 * np.pi * np.cumsum(f(t) * vib) / SR
    x = np.zeros(n)
    f0 = f(t)
    for k in range(1, 60):
        if timbre == "motor":
            a = 1 / k ** 1.05
        else:  # horn: odd-led, softer upper series
            a = (1.0 if k % 2 else 0.35) / k ** 1.35
        live = (k * f0 < 6000).astype(float)
        x += a * live * (np.sin(k * phase) + 0.6 * np.sin(k * phase * 1.006 + k))   # two slightly detuned bodies
    x += 0.35 * np.sin(phase / 2)                                                     # sub-octave weight
    # Time-varying low-pass (two one-pole stages) carries the brightness contour
    c = cutoff(t)
    g = 1 - np.exp(-2 * np.pi * c / SR)
    y = np.zeros(n)
    z1 = z2 = 0.0
    for i in range(n):
        z1 += g[i] * (x[i] - z1)
        z2 += g[i] * (z1 - z2)
        y[i] = z2
    trem = 1 - trem_depth(t) * (0.5 + 0.5 * np.sin(2 * np.pi * trem_rate * t))
    env = amp(t) * trem
    env *= np.clip(t / 0.08, 0, 1) ** 2 * np.clip((dur - t) / 0.035, 0, 1)
    y = np.tanh(1.6 * y * env / (np.abs(y).max() + 1e-9)) if timbre == "motor" else y * env
    y -= signal.lfilter([1], [1, -0.999], y) * 0.001   # tiny DC guard
    return y


def level(x, target=-14.0):
    win, best = int(SR * 0.05), 0.0
    for s in range(0, len(x) - win, win // 4):
        best = max(best, float(np.mean(x[s:s + win] ** 2)))
    return x * (10 ** (target / 20) / np.sqrt(best))


def cues(timbre):
    lock = lambda t: (t >= SLAM_LOCK).astype(float)
    return {
        # Aims (trembling, rising) then locks (steady, brighter) and stops at the strike
        "fist_tell": voice(SLAM_TELL, lambda t: ease(t, 46, 74, 0, SLAM_LOCK) + ease(t, 0, 12, SLAM_LOCK, SLAM_TELL),
                           lambda t: ease(t, .55, .8, 0, SLAM_LOCK) + .2 * lock(t),
                           lambda t: ease(t, 500, 1500, 0, SLAM_LOCK) + ease(t, 0, 1700, SLAM_LOCK, SLAM_LOCK + .1),
                           6.5, lambda t: .32 * (1 - lock(t)), timbre),
        # One long swell to the release: the arm is loading
        "sweep_tell": voice(SWEEP_TELL, lambda t: ease(t, 58, 68, 0, SWEEP_TELL),
                            lambda t: ease(t, .65, 1, 0, SWEEP_TELL), lambda t: ease(t, 900, 2800, 0, SWEEP_TELL),
                            1, lambda t: 0 * t, timbre),
        # Falling and growing for the whole pull, with a slow pumping throb
        "gather": voice(PULL_LEN, lambda t: ease(t, 132, 52, 0, PULL_LEN), lambda t: ease(t, .5, 1, 0, PULL_LEN),
                        lambda t: ease(t, 2400, 1500, 0, PULL_LEN), 2.6, lambda t: .22 + 0 * t, timbre),
    }


def main():
    OUT.mkdir(parents=True, exist_ok=True)
    d = ROOT / "build" / "listen" / "ch6-tells"
    _, mix = listen.load(d / "mix.wav")
    _, focus = listen.load(d / "focus.wav")
    rest = mix - focus   # the real fight minus the three old warnings
    events = listen.read_events(d / "events.md", r"^shep_(fist_tell|sweep_tell|gather)$")
    old = {c: listen.load(ROOT / "assets/sfx" / f"shep_{c}.wav")[1] for c in ("fist_tell", "sweep_tell", "gather")}
    # In-game gain of each old cue: the focus stem's level at its onset vs the asset's own level
    gains = {}
    for at, name in events:
        c = name[5:]
        i, m = int(at * SR), len(old[c])
        gains.setdefault(c, []).append(np.sqrt(np.mean(focus[i:i + m] ** 2) / np.mean(old[c] ** 2)))
    gains = {c: float(np.median(v)) for c, v in gains.items()}
    print("in-game gain of the old cues:", {c: round(g, 2) for c, g in gains.items()})
    fx, fr = None, listen.features(rest)
    for timbre in ("motor", "horn"):
        made = {c: level(x) for c, x in cues(timbre).items()}
        place = np.zeros_like(rest)
        for c, x in made.items():
            wavfile.write(OUT / f"{timbre}_{c}.wav", SR, (np.clip(x, -1, 1) * 32767).astype(np.int16))
        for at, name in events:
            c = name[5:]
            i = int(at * SR)
            seg = made[c][: len(place) - i] * gains[c]
            place[i:i + len(seg)] += seg
        ctx = rest + place
        wavfile.write(OUT / f"fight_{timbre}.wav", SR, (np.clip(ctx[: 48 * SR], -1, 1) * 32767).astype(np.int16))
        fx = listen.features(place)
        rows = []
        for at, name in events:
            c = name[5:]
            sel = (fx["t"] >= at) & (fx["t"] < at + len(made[c]) / SR)
            cue = 10 * np.log10(np.mean(10 ** (fx["mel_db"][:, sel] / 10), axis=1))
            mk = 10 * np.log10(np.mean(10 ** (fr["mel_db"][:, sel] / 10), axis=1))
            own = cue > cue.max() - 20
            rows.append((at, c, float(np.percentile((cue - mk)[own], 75))))
        print(timbre, " ".join(f"{c}@{at:.1f}:{snr:+.1f}" for at, c, snr in rows))
    # The same reel with the old warnings, for a fair three-way comparison
    wavfile.write(OUT / "fight_current.wav", SR, (np.clip(mix[: 48 * SR], -1, 1) * 32767).astype(np.int16))


if __name__ == "__main__":
    main()
