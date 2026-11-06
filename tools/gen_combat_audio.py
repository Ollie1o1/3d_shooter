#!/usr/bin/env python3
"""Original layered combat sounds. Stdlib only; deterministic, safe to rerun.

Run AFTER gen_sfx.py / import_sfx.py: python3 tools/gen_combat_audio.py
The frozen CC0 metal recording is a texture, never a previously generated output.
Each cue has three independently excited variants, matched to MixTable levels.
"""
import math
from pathlib import Path
import random
import struct
import wave

SR = 44100
SFX = Path(__file__).resolve().parents[1] / "assets" / "sfx"
TAU = math.tau


def lowpass(x, hz):
    k = 1 - math.exp(-TAU * hz / SR)
    z = 0.0
    out = []
    for v in x:
        z += k * (v - z)
        out.append(z)
    return out


def band(x, lo, hi):
    lp = lowpass(x, lo)
    return lowpass([v - l for v, l in zip(x, lp)], hi)


def body(t, start=190, end=64, decay=24):
    # Integrated exponential pitch drop, so it falls without phase discontinuities.
    phase = TAU * (end * t + (start - end) * (1 - math.exp(-55 * t)) / 55)
    return math.sin(phase) * math.exp(-decay * t)


def metal(t, base, decay, phases):
    # Inharmonic modes, deliberately unequal decay: a struck plate, not a chord.
    return sum(math.sin(TAU * base * ratio * t + ph) * amp *
               math.exp(-decay * (1 + j * .42) * t)
               for j, (ratio, amp, ph) in enumerate(zip(
                   (1, 1.483, 2.117, 2.891, 4.37), (1, .55, .32, .18, .08), phases)))


def recording():
    with wave.open(str(SFX / "src" / "clank_rec.wav")) as w:
        assert (w.getnchannels(), w.getsampwidth(), w.getframerate()) == (1, 2, SR)
        return [s / 32768 for s in struct.unpack("<%dh" % w.getnframes(), w.readframes(w.getnframes()))]


def steel_contact(name, variant):
    """Real blade-on-blade recordings lead; no noise burst or synthesized plate.

    Ben Jaszczak & Brian Nelson / Still North Media, CC0. Isolated strikes
    are frozen in src/sword_steel_*.wav; provenance is in CREDITS.md.
    """
    with wave.open(str(SFX / "src" / f"sword_steel_{variant}.wav")) as w:
        rec = [s / 32768 for s in struct.unpack("<%dh" % w.getnframes(), w.readframes(w.getnframes()))]
    rate, duration = {"sword_hit": (1.0, .48), "sword_heavy": (.94, .62),
                      "sword_pierce": (1.05, .39), "sword_block": (.99, .55)}[name]
    peak = max(abs(v) for v in rec)
    n = int(duration * SR)
    x = []
    for i in range(n):
        pos = i * rate
        j = int(pos)
        f = pos - j
        v = (rec[j] * (1 - f) + rec[min(j + 1, len(rec) - 1)] * f) / peak if j < len(rec) else 0.
        x.append(v)
    # Remove handling rumble; preserve the real high-mid strike and steel tail.
    low = lowpass(x, 180)
    edge = band(x, 1800, 7500)
    x = [v - l + .16 * e for v, l, e in zip(x, low, edge)]
    # Gentle saturation controls only the recording's high-crest transient.
    # The previous low boom / grit recipe obscured the metal and sounded soft.
    x = [math.tanh(v * 2.4) for v in x]
    return [v * min(1., i / (SR * .0003)) * min(1., (n - 1 - i) / (SR * .025))
            for i, v in enumerate(x)]


def make(name, variant, rec):
    if name in ("sword_hit", "sword_heavy", "sword_pierce", "sword_block"):
        return steel_contact(name, variant)
    rng = random.Random(7409 + variant * 131 + sum((i + 1) * ord(c) for i, c in enumerate(name)))
    duration = {"sword_draw": .46, "sword_swing": .19, "sword_thrust": .24,
                "sword_guard": .13, "sword_hit": .29, "sword_heavy": .43,
                "sword_pierce": .32, "sword_block": .38, "sword_break": .58,
                "parry": .48, "parry_heavy": .65, "hit_body": .19,
                "hit_armor": .25, "impact": .28, "impact_metal": .34,
                "impact_wood": .22, "impact_ice": .38}[name]
    n = int(duration * SR)
    noise = [rng.uniform(-1, 1) for _ in range(n)]
    grit = band(noise, 550, 5300)
    air = band(noise, 350, 8500)
    low = lowpass(noise, 1500)
    phases = [rng.uniform(-.25, .25) for _ in range(5)]
    pitch = 1 + (variant - 1) * .035
    out = []
    for i in range(n):
        t = i / SR
        tt = t * pitch
        crack = grit[i] * math.exp(-t * 320)
        thud = body(tt)
        if name in ("sword_swing", "sword_thrust"):
            thrust = name == "sword_thrust"
            # Air arrives immediately and crests early, rather than a late 110 ms swell.
            env = (1 - math.exp(-t * 200)) * math.exp(-t * (25 if thrust else 32))
            scrape = math.sin(TAU * (1900 * t - 2100 * t * t)) * math.exp(-t * 45)
            v = air[i] * env * 2.2 + low[i] * env * .6 + scrape * .045
        elif name == "sword_draw":
            env = (1 - math.exp(-t * 130)) * math.exp(-t * 13)
            v = air[i] * env * .8 + metal(tt, 1530, 13, phases) * .09 + thud * .12
        elif name == "sword_guard":
            v = metal(tt, 1150, 54, phases) * .23 + crack * .65 + thud * .2
        elif name in ("parry", "parry_heavy", "sword_break"):
            parry = name.startswith("parry")
            heavy = name in ("parry_heavy", "sword_break")
            base = 1420 if parry else 390 if heavy else 580
            v = metal(tt, base, 7 if parry else 12, phases) * (.3 if parry else .26)
            v += body(tt, 260, 47 if heavy else 72, 14 if heavy else 23) * (.95 if heavy else .65)
            v += crack * 1.5
            j = int(i * (1.1 if parry else .72) * pitch)
            if j < len(rec):
                v += rec[j] * (.48 if parry else .28) * math.exp(-t * 10)
            if name == "sword_break":
                v += grit[i] * math.exp(-abs(t - .06) * 55) * .8
                v += metal(tt, 220 * (1 - .35 * t), 9, phases) * .15
        elif name == "hit_body":
            v = thud * .95 + crack * 1.2 + metal(tt, 360, 62, phases) * .12
            v += low[i] * math.exp(-abs(t - .012) * 150) * .4
        elif name in ("hit_armor", "impact_metal"):
            v = crack * 1.3 + thud * .35 + metal(tt, 930 if name == "hit_armor" else 670, 19, phases) * .4
        elif name == "impact_wood":
            v = body(tt, 420, 150, 49) * .65 + low[i] * math.exp(-t * 50) * 1.1 + crack * .5
        elif name == "impact_ice":
            v = crack + metal(tt, 2240, 24, phases) * .15 + thud * .16
            for at in (.027, .061, .109):
                if t > at:
                    v += air[i] * math.exp(-(t - at) * 140) * .28
        else:  # stone: dense crack, low body, scattered chips, no metallic ring
            v = crack * 1.4 + low[i] * math.exp(-t * 65) * 1.5 + thud * .5
            for at in (.023, .052, .097):
                if t > at:
                    v += grit[i] * math.exp(-(t - at) * 190) * .17
        # Sub-millisecond onset and 12 ms tail fades; remove DC below afterwards.
        v *= min(1, t / .0004) * min(1, (duration - t) / .012)
        out.append(v)
    # Softly control the raw transient before loudness matching, preserving headroom.
    out = [math.tanh(v * 2.0) for v in out]
    dc = lowpass(out, 28)
    return [(v - d) * min(1, (n - 1 - i) / (SR * .005))
            for i, (v, d) in enumerate(zip(out, dc))]


def save(name, x, target):
    # Same 50 ms / quarter-window measurement as MixTable::shortTermDb.
    win = int(SR * .05)
    best = 0
    for start in range(0, len(x), win // 4):
        part = x[start:start + win]
        best = max(best, sum(v * v for v in part) / len(part))
        if start + win >= len(x):
            break
    gain = 10 ** (target / 20) / math.sqrt(best)
    peak = max(abs(v * gain) for v in x)
    if peak > .97:
        raise ValueError(f"{name}: peak {peak:.3f}; adjust the recipe's crest factor")
    with wave.open(str(SFX / (name + ".wav")), "wb") as w:
        w.setparams((1, 2, SR, 0, "NONE", "not compressed"))
        w.writeframes(struct.pack("<%dh" % len(x), *(round(v * gain * 32767) for v in x)))
    print(f"{name:20} {len(x) / SR:.2f}s  {target:.1f} dBFS  peak {peak:.2f}")


def main():
    rec = recording()
    cues = {"sword_draw": -12, "sword_swing": -12, "sword_thrust": -12,
            "sword_guard": -12, "sword_hit": -12, "sword_heavy": -12,
            "sword_pierce": -12, "sword_block": -12, "sword_break": -12,
            "parry": -12, "parry_heavy": -12, "hit_body": -10,
            "hit_armor": -12, "impact": -13, "impact_metal": -13,
            "impact_wood": -13, "impact_ice": -13}
    for name, target in cues.items():
        for variant in range(3):
            save(name + (f"_{variant}" if variant else ""), make(name, variant, rec), target)


if __name__ == "__main__":
    main()
