#!/usr/bin/env python3
"""
Builds the game's recorded sound effects from free CC0 (public domain) packs
on OpenGameArt, layered with the synthesized sounds from gen_sfx.py where a
recording alone isn't punchy enough. Sources are listed in assets/sfx/CREDITS.md.

    # 1. download and unpack the packs into one folder (see CREDITS.md), e.g.
    #    sounds.zip, swishes.zip, clangs.zip, sfx_100_v2.zip
    # 2. convert everything to mono 16-bit 44.1 kHz WAV in <folder>/conv
    #    (macOS: afconvert -f WAVE -d LEI16@44100 -c 1 in.ogg out.wav)
    # 3. python3 tools/import_sfx.py <folder>/conv

Writes: revolver, shotgun, kar, longshot, dash, grapple_fire, punch, step1-4, clank.
These replace gen_sfx.py's synthesized versions, so run gen_sfx.py first.
"""
import math
import os
import struct
import sys
import wave

SR = 44100
OUT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "assets", "sfx")


def load(path):
    w = wave.open(path)
    assert w.getnchannels() == 1 and w.getsampwidth() == 2 and w.getframerate() == SR, path
    n = w.getnframes()
    return [v / 32768.0 for v in struct.unpack("<%dh" % n, w.readframes(n))]


def trim_lead(x, rel=0.25, pre=0.004):
    """Drop the quiet lead-in so the sound starts the instant it's triggered
    (a gunshot that peaks 75 ms after the click feels laggy)."""
    m = max(abs(v) for v in x)
    k = next(i for i, v in enumerate(x) if abs(v) >= m * rel)
    k = max(0, k - int(pre * SR))
    y = x[k:]
    for i in range(min(int(0.002 * SR), len(y))):
        y[i] *= i / (0.002 * SR)
    return y


def save(name, x, peak=0.9):
    x = trim_lead(x)
    m = max(1e-9, max(abs(v) for v in x))
    x = [v * peak / m for v in x]
    with wave.open(os.path.join(OUT, name + ".wav"), "wb") as w:
        w.setnchannels(1)
        w.setsampwidth(2)
        w.setframerate(SR)
        w.writeframes(b"".join(struct.pack("<h", int(max(-1, min(1, v)) * 32767)) for v in x))
    print(f"wrote {name}.wav ({len(x) / SR * 1000:.0f} ms)")


def cut(x, start, length, fade_in=0.002, fade_out=0.08):
    a, n = int(start * SR), int(length * SR)
    y = x[a:a + n]
    fi, fo = int(fade_in * SR), int(fade_out * SR)
    for i in range(min(fi, len(y))):
        y[i] *= i / fi
    for i in range(min(fo, len(y))):
        y[-1 - i] *= i / fo
    return y


def pitch(x, factor):
    """Resample: factor > 1 raises the pitch (and shortens), < 1 lowers it."""
    n = int(len(x) / factor)
    out = []
    for i in range(n):
        p = i * factor
        k = int(p)
        f = p - k
        a = x[k] if k < len(x) else 0.0
        b = x[k + 1] if k + 1 < len(x) else 0.0
        out.append(a + (b - a) * f)
    return out


def mix(*tracks):
    """mix((samples, gain, offset_seconds), ...)"""
    n = max(len(t) + int(o * SR) for t, g, o in tracks)
    out = [0.0] * n
    for t, g, o in tracks:
        off = int(o * SR)
        for i, v in enumerate(t):
            out[off + i] += v * g
    return out


def lowpass(x, hz):
    rc, dt = 1.0 / (2 * math.pi * hz), 1.0 / SR
    a, y, out = dt / (rc + dt), 0.0, []
    for v in x:
        y += a * (v - y)
        out.append(y)
    return out


def decay(x, tau):
    return [v * math.exp(-i / SR / tau) for i, v in enumerate(x)]


def main(conv):
    c = lambda n: load(os.path.join(conv, n + ".wav"))

    # Guns: single shots cut from range recordings (a CZ-52, a shotgun, a
    # Mosin-Nagant), just before each onset
    save("revolver", cut(c("cz"), 0.22, 0.34, fade_out=0.12))
    save("shotgun", cut(c("shotty"), 0.04, 0.62, fade_out=0.2))
    mosin = cut(c("mosin"), 0.42, 1.0, fade_out=0.35)
    save("kar", mosin)
    # The Longshot: the Mosin pitched down for weight, over the synthesized boom
    sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
    import gen_sfx
    gen_sfx.random.seed(1234)
    synth_boom = gen_sfx.gen_longshot()
    save("longshot", mix((pitch(mosin, 0.78), 1.0, 0.0), (synth_boom, 0.55, 0.0)))

    # Dash: a heavy swish slowed into a whoosh, with an airy layer
    save("dash", mix((pitch(c("swish9"), 0.7), 1.0, 0.0), (lowpass(c("air_03"), 3000), 0.5, 0.02)))
    # Grapple: a quick light swish as the line flies out, then a metal clink as it bites
    save("grapple_fire", mix((pitch(c("swish4"), 1.15), 0.9, 0.0), (decay(c("metal_03"), 0.08), 0.6, 0.11)))

    # Punch (F): a short light swish
    save("punch", pitch(c("swish2"), 0.95), 0.75)

    # Footsteps: four variations, chosen at random each step
    s1, s2 = cut(c("footstep_01"), 0.09, 0.28), cut(c("footstep_02"), 0.08, 0.26)
    save("step1", s1, 0.8)
    save("step2", s2, 0.8)
    save("step3", pitch(s1, 1.08), 0.8)
    save("step4", pitch(s2, 0.93), 0.8)

    # Parry: a bright metal clang with a hard transient on top
    clang = decay(cut(c("clang2"), 0.0, 1.1, fade_out=0.3), 0.35)
    save("clank", mix((clang, 1.0, 0.0), (cut(c("metal_02"), 0.04, 0.25), 0.7, 0.0)))


if __name__ == "__main__":
    if len(sys.argv) != 2:
        sys.exit(__doc__)
    main(sys.argv[1])
