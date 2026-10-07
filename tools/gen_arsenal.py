#!/usr/bin/env python3
"""
The arsenal's sounds, built from one recipe so the four guns sound like one
family (the feel track's "dark machine + glow"):

  shot = crack (the real recording's first instant) + thump (a falling sine,
         deeper and longer the heavier the gun) + ring (a few damped
         inharmonic partials in the gun's own pitch: the family sound) + tail
         (filtered noise dying away; the Longshot's a long boom)

Three variants per gun (name.wav, name_1.wav, name_2.wav) - the sound engine
rotates them. Then the mechanical foley, one recipe pitched per mechanism:
cyl_open, cyl_close, eject, shell_in, pump, bolt, reload, cell, dry and a
switch_up chime per gun in its glow's pitch.

Reads the frozen recordings in assets/sfx/src/*_rec.wav (copied once from the
old shots, so re-running never stacks layers). Stdlib only.

    python3 tools/gen_arsenal.py
"""
import math, os, random, struct, wave

SR = 44100
HERE = os.path.dirname(os.path.abspath(__file__))
SFX = os.path.join(HERE, "..", "assets", "sfx")
SRC = os.path.join(SFX, "src")

def load(path):
    w = wave.open(path)
    assert w.getnchannels() == 1 and w.getsampwidth() == 2, path
    n = w.getnframes()
    x = [v / 32768.0 for v in struct.unpack("<%dh" % n, w.readframes(n))]
    if w.getframerate() != SR:   # resample (linear) if a source isn't 44.1 kHz
        r = w.getframerate() / SR
        x = [x[min(int(i * r), n - 1)] for i in range(int(n / r))]
    return x

def save(name, x, peak=0.9):
    m = max(1e-9, max(abs(v) for v in x))
    x = [v * peak / m for v in x]
    with wave.open(os.path.join(SFX, name + ".wav"), "wb") as w:
        w.setnchannels(1); w.setsampwidth(2); w.setframerate(SR)
        w.writeframes(b"".join(struct.pack("<h", int(max(-1, min(1, v)) * 32767)) for v in x))
    print(f"wrote {name}.wav ({len(x) / SR * 1000:.0f} ms)")

def n_of(sec): return int(sec * SR)

def mix(length, *parts):
    out = [0.0] * length
    for gain, start, x in parts:
        for i, v in enumerate(x):
            j = start + i
            if 0 <= j < length: out[j] += gain * v
    return out

def onepole_lp(x, hz):
    a = 1 - math.exp(-2 * math.pi * hz / SR); y = 0.0; out = []
    for v in x: y += a * (v - y); out.append(y)
    return out

def onepole_hp(x, hz):
    lp = onepole_lp(x, hz)
    return [v - l for v, l in zip(x, lp)]

def crack(rec, keep, skip=0):
    x = rec[skip:skip + n_of(keep)]
    fade = n_of(0.03)
    for i in range(max(0, len(x) - fade), len(x)):
        x[i] *= (len(x) - i) / fade
    return onepole_hp(x, 120)

def thump(f0, f1, dur):
    out, ph = [], 0.0
    for i in range(n_of(dur)):
        t = i / SR
        f = f1 + (f0 - f1) * math.exp(-t * 18)
        ph += 2 * math.pi * f / SR
        out.append(math.sin(ph) * math.exp(-t * 6 / dur) * min(1, i / 40))
    return out

def ring(freq, dur, detune=1.0):
    partials = [(1.0, 1.0), (2.76, 0.5), (5.40, 0.25), (8.93, 0.12)]   # a struck bar: the machine's voice
    out = []
    for i in range(n_of(dur)):
        t = i / SR
        out.append(sum(a * math.sin(2 * math.pi * freq * detune * r * t) * math.exp(-t * (5 + 3 * r) / dur)
                       for r, a in partials))
    return out

def tail(dur, cutoff, seed):
    rnd = random.Random(seed)
    x = [rnd.uniform(-1, 1) * math.exp(-4 * (i / SR) / dur) for i in range(n_of(dur))]
    return onepole_lp(x, cutoff)

GUNS = {   # name: recording, crack length, thump (from, to, s), ring Hz, ring s, tail (s, Hz), gains (crack, thump, ring, tail)
    "revolver": ("revolver", 0.09, (90, 50, 0.16), 520, 0.25, (0.25, 2600), (1.0, 0.85, 0.22, 0.30)),
    "shotgun":  ("shotgun",  0.12, (70, 40, 0.26), 330, 0.30, (0.45, 1800), (1.0, 1.00, 0.20, 0.40)),
    "kar":      ("kar",      0.12, (80, 45, 0.22), 440, 0.35, (0.60, 2200), (1.0, 0.90, 0.22, 0.35)),
    "longshot": ("longshot", 0.15, (55, 30, 0.40), 220, 0.50, (1.20, 1200), (1.0, 1.10, 0.25, 0.55)),
}
GLOW_HZ = [520, 330, 440, 220]   # the switch chime of each gun, in its ring's pitch

def shots():
    energy = {}
    for name, (rec, keep, th, rf, rd, (td, tc), (gc, gt, gr, gtl)) in GUNS.items():
        src = load(os.path.join(SRC, rec + "_rec.wav"))
        for v in range(3):
            length = n_of(max(keep, th[2], rd, td) + 0.05)
            x = mix(length,
                    (gc, 0, crack(src, keep, skip=v * n_of(0.0015))),
                    (gt, 0, thump(th[0], th[1], th[2])),
                    (gr, n_of(0.004), ring(rf, rd, detune=1.0 + 0.03 * (v - 1))),
                    (gtl, n_of(0.01), tail(td, tc, seed=hash((name, v)) & 0xffff)))
            save(name if v == 0 else f"{name}_{v}", x, peak=0.75)   # headroom: shots stack over the music
            if v == 0: energy[name] = sum(s * s for s in x[:n_of(0.3)]) * (len(x) / SR)
    order = sorted(energy, key=energy.get)
    print("weight order (light -> heavy):", order)

def click(freq, dur=0.035, noise=0.6, seed=1):
    rnd = random.Random(seed)
    return [(noise * rnd.uniform(-1, 1) + (1 - noise) * math.sin(2 * math.pi * freq * i / SR)) * math.exp(-i / SR * 70)
            for i in range(n_of(dur))]

def slide(dur, cutoff, seed=2):
    rnd = random.Random(seed)
    x = [rnd.uniform(-1, 1) * math.sin(math.pi * i / n_of(dur)) for i in range(n_of(dur))]
    return onepole_lp(x, cutoff)

def foley():
    save("cyl_open",  mix(n_of(0.25), (1, 0, click(1800)), (0.7, n_of(0.06), click(1200, seed=3)), (0.3, 0, ring(1600, 0.2))), 0.7)
    save("cyl_close", mix(n_of(0.30), (1, 0, click(2200)), (0.5, 0, thump(160, 90, 0.08)), (0.35, 0, ring(1400, 0.25))), 0.8)
    save("eject",     mix(n_of(0.50), *[(0.8 - 0.15 * k, n_of(0.05 + 0.09 * k), click(3000 - 300 * k, seed=10 + k)) for k in range(4)]), 0.6)
    save("shell_in",  mix(n_of(0.14), (0.6, 0, slide(0.06, 3000)), (1, n_of(0.05), click(1400, seed=5))), 0.7)
    save("pump",      mix(n_of(0.42), (0.7, 0, slide(0.14, 2200)), (1, n_of(0.14), click(900, seed=6)),
                          (0.6, n_of(0.22), slide(0.12, 2600, seed=7)), (1, n_of(0.34), click(1100, seed=8)), (0.3, n_of(0.34), ring(330, 0.08))), 0.85)
    save("bolt",      mix(n_of(0.42), (1, 0, click(1500, seed=9)), (0.6, n_of(0.08), slide(0.1, 2500, seed=11)),
                          (1, n_of(0.2), click(1300, seed=12)), (0.9, n_of(0.32), click(1700, seed=13)), (0.25, n_of(0.32), ring(440, 0.08))), 0.8)
    save("reload",    mix(n_of(0.28), (1, 0, click(1200, seed=14)), (0.8, n_of(0.12), click(1600, seed=15))), 0.7)
    save("cell",      [math.sin(2 * math.pi * 3200 * i / SR) * math.exp(-i / SR * 120) for i in range(n_of(0.04))], 0.45)
    save("dry",       click(700, 0.05, noise=0.4, seed=16), 0.5)
    for g, hz in enumerate(GLOW_HZ):   # a short rising chime as the gun comes up
        out, ph = [], 0.0
        for i in range(n_of(0.22)):
            t = i / SR
            f = hz * (0.5 + 0.5 * min(1, t / 0.12))
            ph += 2 * math.pi * f / SR
            out.append((math.sin(ph) + 0.3 * math.sin(2.76 * ph)) * math.exp(-t * 9) * min(1, i / 200))
        save(f"switch_up{g}", mix(n_of(0.22), (1, 0, out), (0.5, 0, click(2000, 0.02, seed=20 + g))), 0.55)

if __name__ == "__main__":
    shots()
    foley()
