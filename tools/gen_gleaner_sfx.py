#!/usr/bin/env python3
"""
THE GLEANER's sounds (Chapter 4), synthesized like tools/gen_sfx.py: no
recordings, all oscillators, noise and filters. Writes assets/sfx/:

  gleaner_scream  the scream: a colossal train horn. A detuned brass chord
                  (F, A, C, E-flat, an octave below a locomotive's) through horn
                  formants, bending up as it starts, growling, a metal shriek
                  riding over it, sagging as it gives out, and the plain
                  throwing it back late
  gleaner_slam    a fist coming down: a sub thump that drops, crushed rock,
                  iron ringing, and a long rolling rumble
  gleaner_groan   rising out of the ground: iron dragging on iron, a creaking
                  wail that wanders, grit falling
  gleaner_blink   the moon shutting: a held breath, a low swell, a glassy
                  chord that cuts to nothing
  caller_toll     THE CALLER ringing for the sky: a house-sized cracked bell
                  (hum, prime, minor tierce, quint, nominal), every partial
                  split and beating, a buzz from the crack, the plain's echoes
  caller_clang    a round off its shell: the same bell struck small and hard
  sieve_rattle    THE SIEVE shaking its basket: slag grains rattling in a mesh,
                  chains jangling, a sizzle under it
  sieve_stake     a stilt leg driven into iron: the blow, the plate ringing,
                  cracks running out, then bursting

    python3 tools/gen_gleaner_sfx.py

It prints each sound's loudest 50 ms (RMS dBFS): src/audio/MixTable.h trims
them to their class (tests/test_game.cpp holds every file within 2 dB).
"""
import math
import os
import random
import struct
import wave

SR = 44100
OUT_DIR = os.path.join(os.path.dirname(__file__), "..", "assets", "sfx")
random.seed(24)


def write_wav(name, x):
    peak = max(1e-9, max(abs(s) for s in x))
    k = 0.95 / peak
    path = os.path.join(OUT_DIR, name + ".wav")
    frames = bytearray()
    for s in x:
        frames += struct.pack("<h", int(max(-1.0, min(1.0, s * k)) * 32767))
    with wave.open(path, "wb") as f:
        f.setnchannels(1); f.setsampwidth(2); f.setframerate(SR); f.writeframes(bytes(frames))
    y = [max(-1.0, min(1.0, s * k)) for s in x]
    print(f"wrote {path} ({len(x) / SR:.2f} s), loudest 50 ms {loudest_db(y):.1f} dBFS")


def loudest_db(x):
    win = int(SR * 0.05); hop = win // 4; best = 0.0
    for s in range(0, max(1, len(x) - win), hop):
        e = sum(v * v for v in x[s:s + win]) / win
        best = max(best, e)
    return 10 * math.log10(best) if best > 1e-12 else -120.0


class Biquad:
    """RBJ biquad: 'lp', 'hp', 'bp' (constant peak)."""
    def __init__(self, kind, f, q=0.707):
        self.kind, self.q = kind, q
        self.x1 = self.x2 = self.y1 = self.y2 = 0.0
        self.set(f)

    def set(self, f):
        f = max(10.0, min(f, SR * 0.45))
        w = 2 * math.pi * f / SR; c = math.cos(w); a = math.sin(w) / (2 * self.q)
        if self.kind == "lp": b0, b1, b2 = (1 - c) / 2, 1 - c, (1 - c) / 2
        elif self.kind == "hp": b0, b1, b2 = (1 + c) / 2, -(1 + c), (1 + c) / 2
        else: b0, b1, b2 = a, 0.0, -a
        a0, a1, a2 = 1 + a, -2 * c, 1 - a
        self.b0, self.b1, self.b2, self.a1, self.a2 = b0 / a0, b1 / a0, b2 / a0, a1 / a0, a2 / a0

    def run(self, x):
        y = self.b0 * x + self.b1 * self.x1 + self.b2 * self.x2 - self.a1 * self.y1 - self.a2 * self.y2
        self.x2, self.x1, self.y2, self.y1 = self.x1, x, self.y1, y
        return y


def smooth(t):
    t = max(0.0, min(1.0, t)); return t * t * (3 - 2 * t)


def saw(ph):
    return 2.0 * (ph - math.floor(ph + 0.5))


def echoes(x, taps, cutoff):
    """The plain throwing a sound back: late, quieter copies, each darker than the last."""
    n = len(x) + int(SR * max(d for d, _ in taps))
    out = x + [0.0] * (n - len(x))
    for d, g in taps:
        lp = Biquad("lp", cutoff * (1.0 - d * 0.25))
        off = int(SR * d)
        for i, s in enumerate(x):
            out[i + off] += lp.run(s) * g
    return out


# --------------------------------------------------------------------------- the scream
def gen_scream():
    dur = 5.4
    n = int(SR * dur)
    notes = [87.31, 110.0, 130.81, 155.56]          # F2 A2 C3 Eb3: a horn's dominant chord, dropped an octave
    det = [(-0.31, 0.27), (0.22, -0.18), (-0.15, 0.33), (0.4, -0.24)]   # each note two reeds, beating
    ph = [[random.random(), random.random()] for _ in notes]
    form = [Biquad("bp", 420, 2.2), Biquad("bp", 1050, 3.0), Biquad("bp", 2300, 4.0)]
    body = Biquad("lp", 3200)
    shriekPh = 0.0
    shriekF = Biquad("bp", 1400, 6.0)
    breath = Biquad("bp", 900, 0.9)
    x = []
    for i in range(n):
        t = i / SR
        # pitch: a lurch up into it, a slow vibrato, a sag as it gives out
        bend = 0.72 + 0.28 * smooth(t / 0.4) - 0.2 * smooth((t - (dur - 1.0)) / 1.0)
        vib = 1.0 + 0.006 * math.sin(2 * math.pi * 4.6 * t) + 0.003 * math.sin(2 * math.pi * 0.7 * t)
        growl = 1.0 - 0.28 * (0.5 + 0.5 * math.sin(2 * math.pi * 31.0 * t))     # the throat rattling
        env = smooth(t / 0.22) * (1.0 - smooth((t - (dur - 0.7)) / 0.7))
        s = 0.0
        for k, f in enumerate(notes):
            for r in range(2):
                ph[k][r] += f * bend * vib * (1.0 + det[k][r] / 100.0) / SR
                s += saw(ph[k][r]) * (1.0 - 0.12 * k)
        s *= 0.18
        h = form[0].run(s) * 1.3 + form[1].run(s) * 0.9 + form[2].run(s) * 0.35 + body.run(s) * 0.5
        h = math.tanh(h * 2.6 * growl)                                           # overblown: the bell cracking
        # the shriek: metal on metal riding on top, wandering up then down
        sf = 1150 + 420 * smooth(t / 1.2) - 380 * smooth((t - 3.2) / 2.0) + 60 * math.sin(2 * math.pi * 6.3 * t)
        shriekPh += sf / SR
        sh = shriekF.run(saw(shriekPh) + random.uniform(-0.6, 0.6)) * (0.18 + 0.12 * math.sin(2 * math.pi * 0.9 * t))
        b = breath.run(random.uniform(-1, 1)) * 0.25
        x.append((h * 0.85 + sh * 0.8 + b) * env)
    x = echoes(x, [(0.42, 0.32), (1.1, 0.22), (1.9, 0.13), (2.8, 0.07)], 1800.0)
    write_wav("gleaner_scream", x)


# --------------------------------------------------------------------------- a fist landing
def gen_slam():
    dur = 4.2
    n = int(SR * dur)
    crush = Biquad("lp", 900)
    rumble = Biquad("lp", 110)
    rumble2 = Biquad("lp", 60)
    ring = [(212.0, 2.4), (377.0, 3.1), (511.0, 3.8), (733.0, 5.0)]
    ph = 0.0
    x = []
    for i in range(n):
        t = i / SR
        f = 28.0 + 30.0 * math.exp(-t * 7.0)
        ph += f / SR
        thump = math.sin(2 * math.pi * ph) * math.exp(-t * 2.2) * smooth(t / 0.006)
        c = crush.run(random.uniform(-1, 1)) * math.exp(-t * 6.0) * 1.4
        iron = sum(math.sin(2 * math.pi * fr * t) * math.exp(-t * d) for fr, d in ring) * 0.09
        r = rumble2.run(rumble.run(random.uniform(-1, 1))) * (1.0 - math.exp(-t * 8.0)) * math.exp(-t * 0.9) * 9.0
        x.append(math.tanh((thump * 1.3 + c + iron + r) * 1.4))
    write_wav("gleaner_slam", x)


# --------------------------------------------------------------------------- rising out of the ground
def gen_groan():
    dur = 6.0
    n = int(SR * dur)
    creak = Biquad("bp", 380, 5.0)
    grind = Biquad("bp", 160, 1.2)
    grit = Biquad("hp", 2500)
    low = Biquad("lp", 70)
    ph = 0.0
    x = []
    for i in range(n):
        t = i / SR
        env = smooth(t / 1.2) * (1.0 - smooth((t - 4.8) / 1.2))
        f = 46 + 14 * math.sin(2 * math.pi * 0.23 * t) + 9 * math.sin(2 * math.pi * 0.71 * t + 1.0)
        ph += f / SR
        stick = 0.6 + 0.4 * (1 if random.random() < 0.03 else 0)                # stick-slip: iron catching
        creak.set(300 + 260 * (0.5 + 0.5 * math.sin(2 * math.pi * 0.17 * t)))
        c = creak.run(saw(ph) * stick) * 1.6
        g = grind.run(random.uniform(-1, 1)) * 0.6
        gr = grit.run(random.uniform(-1, 1)) * (0.05 + 0.08 * (0.5 + 0.5 * math.sin(2 * math.pi * 3.0 * t)))
        sub = low.run(random.uniform(-1, 1)) * 5.0
        x.append(math.tanh((c + g + gr + sub) * 1.2) * env)
    x = echoes(x, [(0.6, 0.25), (1.4, 0.12)], 900.0)
    write_wav("gleaner_groan", x)


# --------------------------------------------------------------------------- the moon shutting
def gen_blink():
    dur = 2.6
    n = int(SR * dur)
    chord = [523.25, 622.25, 739.99, 1046.5]   # C, E-flat, F-sharp, C: glassy, wrong
    low = Biquad("lp", 80)
    x = []
    for i in range(n):
        t = i / SR
        swell = smooth(t / 1.8)
        cut = 1.0 - smooth((t - 1.85) / 0.06)                                   # it shuts: gone
        glass = sum(math.sin(2 * math.pi * f * t * (1 + 0.002 * math.sin(2 * math.pi * 5 * t + k))) / (k + 1)
                    for k, f in enumerate(chord)) * 0.25
        sub = math.sin(2 * math.pi * 38 * t) * 0.6 + low.run(random.uniform(-1, 1)) * 2.0
        tail = math.exp(-(t - 1.91) * 6.0) * (t > 1.91) * math.sin(2 * math.pi * 31 * t) * 0.8   # the thud of it closing
        x.append((glass * swell * swell + sub * swell) * cut + tail)
    write_wav("gleaner_blink", x)


# --------------------------------------------------------------------------- THE CALLER
BELL = [(0.5, 1.0, 0.18), (1.0, 0.8, 0.32), (1.19, 0.75, 0.42), (1.5, 0.5, 0.55), (2.0, 0.9, 0.7), (2.52, 0.35, 1.1), (3.0, 0.3, 1.4), (4.17, 0.18, 2.0)]


def bell(dur, nominal, decay, crack, strike, seed):
    """A bell's partials (ratio to its nominal, level, decay rate), each split in two and beating:
    the crack. `crack` also buzzes it (the two halves of the crack slapping together)."""
    rnd = random.Random(seed)
    n = int(SR * dur)
    parts = []
    for r, a, d in BELL:
        for k in (-1, 1):
            parts.append((nominal * r * (1.0 + k * crack * (0.004 + 0.006 * rnd.random())), a * (0.6 if k > 0 else 0.5), d * decay, rnd.random()))
    hit = Biquad("bp", nominal * 6.0, 1.2)
    buzz = Biquad("bp", nominal * 9.0, 2.0)
    x = []
    for i in range(n):
        t = i / SR
        s = 0.0
        for f, a, d, ph in parts:
            s += math.sin(2 * math.pi * (f * t + ph)) * a * math.exp(-t * d)
        b = math.tanh(s * 3.0) - math.tanh(s * 2.6)                                 # the crack buzzing
        s += buzz.run(b) * crack * 3.0 * math.exp(-t * 1.2)
        s += hit.run(rnd.uniform(-1, 1)) * strike * math.exp(-t * 40.0)            # the strike
        x.append(s * smooth(t / 0.002))
    return x


def gen_toll():
    x = bell(6.5, 98.0, 0.55, 1.0, 1.2, 7)
    lo = Biquad("lp", 60)
    for i in range(len(x)):   # a little air moved under it
        t = i / SR
        x[i] += lo.run(random.uniform(-1, 1)) * 2.5 * math.exp(-t * 1.5) * smooth(t / 0.05)
    x = echoes(x, [(0.55, 0.3), (1.3, 0.18), (2.2, 0.1)], 1500.0)
    write_wav("caller_toll", x)


def gen_clang():
    x = bell(1.4, 330.0, 3.2, 0.6, 2.0, 11)
    write_wav("caller_clang", x)


# --------------------------------------------------------------------------- THE SIEVE
def gen_rattle():
    dur = 2.4
    n = int(SR * dur)
    x = [0.0] * n
    rnd = random.Random(5)
    # the grains: a few hundred tiny metallic ticks, thickest where the shake swings hardest
    t = 0.0
    while t < dur - 0.05:
        env = smooth(t / 0.25) * (1.0 - smooth((t - 1.9) / 0.5))
        shake = 0.5 + 0.5 * math.sin(2 * math.pi * 6.5 * t) ** 2
        rate = 40 + 520 * env * shake
        t += rnd.expovariate(rate)
        f = rnd.uniform(1400, 5200); a = rnd.uniform(0.2, 1.0) * env; dk = rnd.uniform(90, 220)
        i0 = int(t * SR)
        for j in range(int(SR * 0.03)):
            if i0 + j >= n: break
            tt = j / SR
            x[i0 + j] += math.sin(2 * math.pi * f * tt) * a * math.exp(-tt * dk) * 0.25
    chain = [(910.0, 6.0), (1370.0, 7.5), (2210.0, 9.0), (2890.0, 11.0)]
    sizz = Biquad("hp", 3500)
    low = Biquad("lp", 200)
    for i in range(n):
        tt = i / SR
        env = smooth(tt / 0.25) * (1.0 - smooth((tt - 1.9) / 0.5))
        sw = math.sin(2 * math.pi * 6.5 * tt)
        jang = sum(math.sin(2 * math.pi * f * tt + k) * math.exp(-((tt * 6.5) % 1.0) * d) for k, (f, d) in enumerate(chain)) * 0.05 * env
        x[i] += jang + sizz.run(rnd.uniform(-1, 1)) * 0.06 * env + low.run(rnd.uniform(-1, 1)) * 0.9 * env * abs(sw)
    write_wav("sieve_rattle", x)


def gen_stake():
    dur = 2.4
    n = int(SR * dur)
    rnd = random.Random(9)
    ring = [(176.0, 3.0), (409.0, 4.2), (688.0, 5.5), (1131.0, 7.0), (1702.0, 9.0)]
    crush = Biquad("lp", 1400)
    hiss = Biquad("hp", 2200)
    boom = Biquad("lp", 90)
    ph = 0.0
    x = []
    for i in range(n):
        t = i / SR
        f = 36.0 + 40.0 * math.exp(-t * 9.0)
        ph += f / SR
        thump = math.sin(2 * math.pi * ph) * math.exp(-t * 4.0) * smooth(t / 0.003)
        iron = sum(math.sin(2 * math.pi * fr * t) * math.exp(-t * d) for fr, d in ring) * 0.16
        c = crush.run(rnd.uniform(-1, 1)) * math.exp(-t * 12.0) * 1.2
        # the cracks running out: crackle thickening toward the burst at 0.75 s, then the burst
        crackle = (rnd.random() < 0.002 + 0.03 * smooth((t - 0.1) / 0.6)) * rnd.uniform(-1, 1) * (t < 0.75) * 0.9
        u = t - 0.75
        burst = (hiss.run(rnd.uniform(-1, 1)) * 0.5 + boom.run(rnd.uniform(-1, 1)) * 6.0) * (u > 0) * math.exp(-max(u, 0) * 3.0) * smooth(u / 0.01)
        x.append(math.tanh((thump * 1.4 + iron + c + crackle + burst) * 1.3))
    write_wav("sieve_stake", x)


if __name__ == "__main__":
    os.makedirs(OUT_DIR, exist_ok=True)
    gen_scream()
    gen_slam()
    gen_groan()
    gen_blink()
    gen_toll()
    gen_clang()
    gen_rattle()
    gen_stake()
