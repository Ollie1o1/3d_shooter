#!/usr/bin/env python3
"""
The arsenal's sounds, built from one recipe so the four guns sound like one
family (the feel track's "dark machine + glow"):

  shot = the real recording, whole (its crack and its own tail carry it) +
         thump (a falling sine under it, deeper the heavier the gun) + ring
         (a faint few damped inharmonic partials in the gun's own pitch: the
         family sound). Measured against the recording by the tests: it keeps
         its midrange body, its ring-out and its brightness

Three variants per gun (name.wav, name_1.wav, name_2.wav) - the sound engine
rotates them. The mechanical foley (cyl_open, cyl_close, eject, shell_in,
pump, bolt, reload) is each mechanism's own recording with a faint ring in its
gun's pitch; then a soft cell tick, a dull dry click, and a switch_up chime per
gun in its glow's pitch.

Reads the frozen recordings in assets/sfx/src/*_rec.wav (copied once from the
old sounds, so re-running never stacks layers). Stdlib only.

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

GUNS = {   # name: recording, thump (from, to, s), ring Hz, gains (thump, ring)
    # The recording carries the shot - its whole crack and its own tail; the
    # thump adds weight under it and the ring a faint metallic tint in the
    # gun's pitch (the family sound), never louder than the gun itself
    "revolver": ("revolver", (90, 50, 0.14), 520, (0.30, 0.07)),
    "shotgun":  ("shotgun",  (70, 40, 0.20), 330, (0.30, 0.06)),
    "kar":      ("kar",      (80, 45, 0.18), 440, (0.28, 0.07)),
    "longshot": ("longshot", (55, 30, 0.30), 220, (0.35, 0.08)),
}
GLOW_HZ = [520, 330, 440, 220]   # the switch chime of each gun, in its ring's pitch
PITCH = [1.0, 0.97, 1.03]        # the three variants: the same gun, a hair lower or higher

def resample(x, k):
    """Play x at k times the speed (linear interpolation)."""
    n = int(len(x) / k)
    out = []
    for i in range(n):
        p = i * k; j = int(p); f = p - j
        out.append(x[j] + (x[j + 1] - x[j]) * f if j + 1 < len(x) else x[-1])
    return out

def faded(x, sec=0.03):
    x = list(x); n = min(len(x), n_of(sec))
    for i in range(n): x[len(x) - 1 - i] *= i / n
    return x

def shots():
    energy = {}
    for name, (rec, th, rf, (gt, gr)) in GUNS.items():
        src = load(os.path.join(SRC, rec + "_rec.wav"))
        for v in range(3):
            body = faded(resample(src, PITCH[v]))
            x = mix(len(body),
                    (1.0, 0, body),
                    (gt, 0, thump(th[0] * PITCH[v], th[1] * PITCH[v], th[2])),
                    (gr, n_of(0.004), ring(rf * PITCH[v], 0.12)))
            save(name if v == 0 else f"{name}_{v}", x, peak=0.75)   # headroom: shots stack over the music
            if v == 0: energy[name] = sum(s * s for s in x[:n_of(0.3)]) * (len(x) / SR)
    order = sorted(energy, key=energy.get)
    print("weight order (light -> heavy):", order)

def click(freq, dur=0.035, noise=0.6, seed=1):
    rnd = random.Random(seed)
    return [(noise * rnd.uniform(-1, 1) + (1 - noise) * math.sin(2 * math.pi * freq * i / SR)) * math.exp(-i / SR * 70)
            for i in range(n_of(dur))]

FOLEY = {   # name: the recording it's built on, the gun pitch of its faint accent ring, peak
    "cyl_open":  ("cyl_open", 520, 0.70), "cyl_close": ("cyl_close", 520, 0.80), "eject": ("eject", 520, 0.60),
    "shell_in":  ("shell_in", 330, 0.70), "pump":      ("pump",      330, 0.85), "bolt":  ("bolt",  440, 0.80),
    "reload":    ("reload",   440, 0.70),
}

def foley():
    # The mechanism's own recording, with a faint ring in the gun's pitch on its first hit
    for name, (rec, hz, peak) in FOLEY.items():
        src = faded(load(os.path.join(SRC, rec + "_rec.wav")), 0.02)
        save(name, mix(len(src), (1.0, 0, src), (0.05, 0, ring(hz, 0.08))), peak)
    # A cell relighting: a soft, low ratchet tick (one per round, so it stays small)
    rnd = random.Random(17)
    tick = onepole_lp([rnd.uniform(-1, 1) * math.exp(-i / SR * 220) for i in range(n_of(0.03))], 1800)
    tone = [math.sin(2 * math.pi * 900 * i / SR) * math.exp(-i / SR * 160) for i in range(n_of(0.03))]
    save("cell", mix(n_of(0.03), (1.0, 0, tick), (0.5, 0, tone)), 0.30)
    # Empty: a dull clack, the cylinder-close recording's first hit, darkened
    clack = onepole_lp(load(os.path.join(SRC, "cyl_close_rec.wav"))[:n_of(0.06)], 2200)
    save("dry", faded(clack, 0.02), 0.45)
    for g, hz in enumerate(GLOW_HZ):   # a short rising chime as the gun comes up
        out, ph = [], 0.0
        for i in range(n_of(0.22)):
            t = i / SR
            f = hz * (0.5 + 0.5 * min(1, t / 0.12))
            ph += 2 * math.pi * f / SR
            out.append((math.sin(ph) + 0.3 * math.sin(2.76 * ph)) * math.exp(-t * 9) * min(1, i / 200))
        save(f"switch_up{g}", mix(n_of(0.22), (1, 0, out), (0.4, 0, click(900, 0.02, seed=20 + g))), 0.55)

if __name__ == "__main__":
    shots()
    foley()
