"""
THE ABSOLUTE's sounds (imported by gen_sfx.py): his wind-ups, the cues that
say what to do (ting: parry; doom: dodge or block; pulse: get out), the booms
of his blows, his flight, and the player's side (parry clang, break, creak,
the timing tick, the wrong buzz). Each is built from a few layers so they read
apart: the swishes are noise, the gold is low and growling, the reds buzz,
the booms are sub drops with a crack and an open-sky tail.
"""
import math
import random

SR = 44100


def n(sec):
    return int(SR * sec)


def env(i, total, attack, release, curve=1.0):
    """0..1: a linear attack, a hold, a release at the end, raised to curve"""
    a = min(1.0, i / max(1, n(attack)))
    r = min(1.0, (total - i) / max(1, n(release)))
    return (min(a, r)) ** curve


def sweep(f0, f1, length, curve=1.0):
    """Phase-accumulated sine sweeping f0 -> f1 (curve > 1: late, < 1: early)"""
    out, ph = [], 0.0
    for i in range(length):
        u = (i / max(1, length - 1)) ** curve
        f = f0 + (f1 - f0) * u
        ph += 2 * math.pi * f / SR
        out.append(math.sin(ph))
    return out


def saw(f, i):
    x = (f * i / SR) % 1.0
    return 2 * x - 1


def square(f, i):
    return 1.0 if (f * i / SR) % 1.0 < 0.5 else -1.0


def noise(length):
    return [random.uniform(-1, 1) for _ in range(length)]


def onepole_lp(x, fc):
    a = 1 - math.exp(-2 * math.pi * fc / SR)
    y, p = [0.0] * len(x), 0.0
    for i, s in enumerate(x):
        p += a * (s - p)
        y[i] = p
    return y


def onepole_hp(x, fc):
    lp = onepole_lp(x, fc)
    return [s - l for s, l in zip(x, lp)]


def bandsweep(x, f0, f1, q=4.0):
    """State-variable band-pass whose centre sweeps f0 -> f1 across x"""
    low = band = 0.0
    y = []
    L = len(x)
    for i, s in enumerate(x):
        fc = f0 + (f1 - f0) * i / max(1, L - 1)
        f = 2 * math.sin(math.pi * min(fc, SR / 6) / SR)
        high = s - low - band / q
        band += f * high
        low += f * band
        y.append(band)
    return y


def drive(x, k):
    return [math.tanh(s * k) / math.tanh(k) for s in x]


def gain(x, g):
    return [s * g for s in x]


def add(*tracks):
    L = max(len(t) for t in tracks)
    out = [0.0] * L
    for t in tracks:
        for i, s in enumerate(t):
            out[i] += s
    return out


def pad(x, length):
    return x + [0.0] * max(0, length - len(x))


def tail(x, seconds, decay=0.55, wet=0.35, damp=3500.0):
    """An open-sky echo: four damped combs and two all-passes (Schroeder), x padded by `seconds`"""
    x = pad(x, len(x) + n(seconds))
    out = [0.0] * len(x)
    for d_ms in (29.7, 37.1, 41.1, 43.7):
        d = n(d_ms / 1000.0)
        buf, lp = [0.0] * len(x), 0.0
        a = 1 - math.exp(-2 * math.pi * damp / SR)
        for i in range(len(x)):
            fb = buf[i - d] if i >= d else 0.0
            lp += a * (fb - lp)
            buf[i] = x[i] + lp * decay
            out[i] += buf[i] * 0.25
    for d_ms, g in ((5.0, 0.7), (1.7, 0.7)):
        d = n(d_ms / 1000.0)
        y = [0.0] * len(out)
        for i in range(len(out)):
            xd = out[i - d] if i >= d else 0.0
            yd = y[i - d] if i >= d else 0.0
            y[i] = -g * out[i] + xd + g * yd
        out = y
    return [dry * (1 - wet) + w * wet for dry, w in zip(x, out)]


def fit(x, seconds):
    """Exactly `seconds` long, with a 10 ms fade at the end"""
    L = n(seconds)
    x = pad(x[:L], L)
    f = n(0.01)
    for i in range(f):
        x[L - 1 - i] *= i / f
    return x


# ---- wind-ups: white (swishes), gold (growl), red (whine, rumble) -------------------------------

def _swish(sec, f0, f1, swell=2.2, flutter=0.0):
    L = n(sec)
    b = bandsweep(noise(L), f0, f1, q=3.0)
    return [s * (i / L) ** swell * (1 + flutter * math.sin(2 * math.pi * 9 * i / SR)) * (1.0 if i < L - n(0.012) else (L - i) / n(0.012))
            for i, s in enumerate(b)]


def gen_ab_wind_jab():
    return fit(gain(_swish(0.30, 800, 3500, 2.6), 1.4), 0.30)


def gen_ab_wind_cross():
    L = n(0.35)
    zip_ = [s * (i / L) ** 2.5 * 0.35 for i, s in enumerate(sweep(300, 1400, L, 1.6))]
    return fit(add(gain(_swish(0.35, 700, 3800, 2.8), 1.4), zip_), 0.35)


def gen_ab_wind_hook():
    return fit(gain(_swish(0.45, 400, 2000, 1.8, 0.35), 1.5), 0.45)


def gen_ab_wind_spin():
    L = n(0.50)
    base = bandsweep(noise(L), 600, 2600, q=2.5)
    out, ph = [], 0.0
    for i, s in enumerate(base):
        u = i / L
        ph += 2 * math.pi * (6 + 16 * u) / SR
        out.append(s * (0.5 + 0.5 * math.sin(ph)) ** 2 * u ** 1.4 * 1.8)
    return fit(out, 0.50)


def gen_ab_wind_meteor():
    L = n(0.60)
    chord = [0.0] * L
    for f in (1318.5, 1975.5, 2637.0):
        ph = 0.0
        for i in range(L):
            ph += 2 * math.pi * f * (1 + 0.006 * math.sin(2 * math.pi * 6 * i / SR)) * (1 + 0.15 * i / L) / SR
            chord[i] += math.sin(ph) * 0.22 * (i / L) ** 1.5
    roar = [s * (i / L) ** 2 * 0.9 for i, s in enumerate(onepole_lp(noise(L), 900))]
    return fit(add(chord, roar), 0.60)


def gen_ab_wind_blade():
    L = n(0.45)
    a, b = sweep(880, 1175, L, 0.8), sweep(887, 1183, L, 0.8)
    hiss = gain(onepole_hp(noise(L), 5000), 0.08)
    return fit([(x + y) * 0.35 * (i / L) ** 1.2 + h for i, (x, y, h) in enumerate(zip(a, b, hiss))], 0.45)


def gen_ab_thrum():
    """Gold: a deep growl swelling, a dragged gong - nothing bright"""
    L = n(0.80)
    growl = drive([saw(55 * (1 + 0.01 * math.sin(2 * math.pi * 4 * i / SR)), i) for i in range(L)], 2.5)
    growl = onepole_lp(growl, 400)
    gong = [0.0] * L
    for mult, amp in ((1.0, 0.5), (2.76, 0.28), (5.4, 0.12)):
        ph = 0.0
        for i in range(L):
            ph += 2 * math.pi * 110 * mult * (0.97 + 0.03 * i / L) / SR
            gong[i] += math.sin(ph) * amp
    return fit([(g * 0.7 + o * 0.5) * (i / L) ** 1.6 for i, (g, o) in enumerate(zip(growl, gong))], 0.80)


def gen_ab_whistle():
    L = n(0.75)
    wh = sweep(2400, 500, L, 0.6)
    air = onepole_lp(noise(L), 1500)
    return fit([(w * 0.5 + a * 0.5 * (i / L)) * (0.3 + 0.7 * (i / L)) for i, (w, a) in enumerate(zip(wh, air))], 0.75)


def gen_ab_ray_charge():
    L = n(0.65)
    wh = sweep(300, 1800, L, 1.3)
    buzz = onepole_lp([saw(70, i) for i in range(L)], 900)
    return fit([(w * 0.45 + b * 0.35) * (i / L) ** 1.3 for i, (w, b) in enumerate(zip(wh, buzz))], 0.65)


def _crackle(L, density_end=0.004, density_start=0.0005):
    out = [0.0] * L
    for i in range(L):
        dens = density_start + (density_end - density_start) * i / L
        if random.random() < dens:
            amp = random.uniform(0.4, 1.0)
            for k in range(min(n(0.004), L - i)):
                out[i + k] += random.uniform(-1, 1) * amp * (1 - k / n(0.004))
    return out


def gen_ab_rumble():
    L = n(0.90)
    rum = gain(onepole_lp(onepole_lp(noise(L), 120), 120), 6.0)
    return fit([(r + c * 0.4) * (i / L) ** 1.1 for i, (r, c) in enumerate(zip(rum, _crackle(L)))], 0.90)


# ---- the cues ------------------------------------------------------------------------------------

def gen_ab_ting():
    """White: parry now. A struck anvil with a kick under it: a punchy sub drop and a knuckle crack, then a
    short steel ring an octave and a half lower than a bell's (it still rings, so it never reads as the DOOM)"""
    L = n(0.32)
    thump = sweep(190, 52, n(0.14), 0.4)
    crack = onepole_lp(noise(n(0.006)), 4500)
    out = []
    for i in range(L):
        t = i / SR
        a = min(1.0, i / n(0.0015))
        body = thump[i] * math.exp(-16 * t) * 1.1 if i < len(thump) else 0.0
        hit = crack[i] * (1 - i / len(crack)) * 0.9 if i < len(crack) else 0.0
        ring = (math.sin(2 * math.pi * 660 * t) * math.exp(-13 * t) + math.sin(2 * math.pi * 660 * 2.76 * t) * 0.3 * math.exp(-24 * t)) * 0.42
        out.append((body + hit + ring) * a)
    return fit(drive(out, 1.8), 0.32)


def gen_ab_doom():
    """Gold: dodge or block. A low knock - never a ting"""
    L = n(0.45)
    thump = sweep(90, 45, L, 0.5)
    out = []
    for i in range(L):
        t = i / SR
        click = math.sin(2 * math.pi * 400 * t) * math.exp(-60 * t) * 0.5
        out.append(thump[i] * math.exp(-7 * t) + click)
    return fit(drive(out, 1.6), 0.45)


def gen_ab_pulse():
    """Red: get out. Two hard blips"""
    L = n(0.30)
    out = [0.0] * L
    for start in (0.0, 0.09):
        s0 = n(start)
        for k in range(n(0.06)):
            out[s0 + k] = square(1000, k) * 0.6 * min(1.0, k / 40) * min(1.0, (n(0.06) - k) / 40)
    return fit(onepole_lp(out, 6000), 0.30)


# ---- the booms -----------------------------------------------------------------------------------

def _boom(sub0, sub1, sub_len, crack_len, crack_lp, tail_s, sub_amp=1.0, crack_amp=0.7, drive_k=1.5, decay=0.55):
    L = n(sub_len)
    sub = sweep(sub0, sub1, L, 0.45)
    sub = [s * math.exp(-4.5 * i / L) * (1 - i / L) * sub_amp for i, s in enumerate(sub)]   # (faded to nothing: no click as it ends)
    C = n(crack_len)
    crack = onepole_lp([random.uniform(-1, 1) * math.exp(-6 * k / C) for k in range(C)], crack_lp)
    crack = gain(crack, crack_amp * 2.5)
    return tail(drive(add(sub, crack), drive_k), tail_s, decay=decay)


def gen_ab_boom_jab():
    return _boom(110, 40, 0.25, 0.005, 9000, 0.6)


def gen_ab_boom_cross():
    return _boom(120, 35, 0.32, 0.008, 8000, 0.75, sub_amp=1.1, crack_amp=0.8, drive_k=1.8)


def gen_ab_boom_hook():
    """Wider: a smeared crack with the swing's air rushing through it, a long tail (on the beat: nothing leads it)"""
    L = n(0.18)
    air = [s * math.exp(-14 * i / L) * 0.45 for i, s in enumerate(bandsweep(noise(L), 2500, 600))]
    return add(_boom(105, 38, 0.3, 0.015, 6000, 0.8, crack_amp=0.6, decay=0.6), air)


def gen_ab_boom_heavy():
    b = _boom(130, 30, 0.45, 0.012, 7000, 1.0, sub_amp=1.3, crack_amp=0.9, drive_k=2.6, decay=0.62)
    L = n(0.8)
    ring = [0.0] * L
    for mult, amp in ((1.0, 0.3), (2.76, 0.18), (5.4, 0.1)):
        for i in range(L):
            ring[i] += math.sin(2 * math.pi * 220 * mult * i / SR) * amp * math.exp(-5 * i / L) * (1 - i / L)
    return add(b, ring)


def gen_ab_boom_kick():
    return _boom(80, 38, 0.3, 0.01, 2000, 0.65, sub_amp=1.25, crack_amp=0.5, drive_k=1.9)


def gen_ab_boom_stone():
    b = _boom(100, 32, 0.35, 0.01, 6000, 0.8, sub_amp=1.2, drive_k=2.0)
    L = n(0.9)
    grains = [0.0] * L
    for _ in range(40):
        s0 = random.randint(n(0.03), n(0.6))
        g = n(random.uniform(0.008, 0.03))
        amp = random.uniform(0.15, 0.45)
        lpn = onepole_lp(noise(g), random.uniform(1500, 5000))
        for k in range(g):
            if s0 + k < L:
                grains[s0 + k] += lpn[k] * amp * (1 - k / g)
    return add(b, grains)


def gen_ab_boom_clap():
    L = n(0.08)
    blast = [s * math.exp(-30 * i / L) * 1.6 for i, s in enumerate(bandsweep(noise(L), 4000, 700, q=1.5))]
    return add(tail(blast, 0.6, decay=0.5), _boom(70, 40, 0.2, 0.002, 3000, 0.4, sub_amp=0.8, crack_amp=0.2))


# ---- the beams -----------------------------------------------------------------------------------

def _beam(f, sec):
    L = n(sec)
    buzz = drive([saw(f * (1 + 0.01 * math.sin(2 * math.pi * 7 * i / SR)), i) for i in range(L)], 2.0)
    buzz = onepole_lp(buzz, 2500)
    return fit([(b * 0.55 + c * 0.35) * env(i, L, 0.02, 0.12) for i, (b, c) in enumerate(zip(buzz, _crackle(L, 0.003, 0.003)))], sec)


def gen_ab_ray_beam():
    return _beam(110, 1.0)


def gen_ab_zray_beam():
    return _beam(73, 2.5)


# ---- flight --------------------------------------------------------------------------------------

def gen_ab_takeoff():
    L = n(1.2)
    roar = []
    lp, fc = 0.0, 300.0
    for i in range(L):
        fc = 300 + 2700 * (i / L) ** 0.7
        a = 1 - math.exp(-2 * math.pi * fc / SR)
        lp += a * (random.uniform(-1, 1) - lp)
        roar.append(lp * env(i, L, 0.15, 0.5) * 1.6)
    choir = [0.0] * L
    for f in (523.25, 659.25, 783.99):
        ph = 0.0
        for i in range(L):
            ph += 2 * math.pi * f * (1 + 0.008 * math.sin(2 * math.pi * 5.5 * i / SR + f)) / SR
            choir[i] += (math.sin(ph) + 0.3 * math.sin(2 * ph)) * 0.16 * env(i, L, 0.4, 0.5)
    return fit(add(roar, choir), 1.2)


def gen_ab_flight():
    L = n(6.0)
    base = noise(L)
    out, low, band = [], 0.0, 0.0
    for i, s in enumerate(base):
        fc = 1000 + 500 * math.sin(2 * math.pi * 0.37 * i / SR) * math.sin(2 * math.pi * 0.11 * i / SR + 1)
        f = 2 * math.sin(math.pi * fc / SR)
        high = s - low - band / 5.0
        band += f * high
        low += f * band
        out.append(band * env(i, L, 0.3, 0.3) * 0.9)
    return fit(out, 6.0)


def gen_ab_shotdown():
    L = n(1.5)
    wail = sweep(900, 150, L, 0.8)
    wail = [w * 0.45 * (1 + 0.1 * math.sin(2 * math.pi * 7 * i / SR)) * env(i, L, 0.01, 0.4) for i, w in enumerate(wail)]
    C = n(0.01)
    crack = [random.uniform(-1, 1) * math.exp(-6 * k / C) * 1.5 for k in range(C)]
    return fit(add(tail(crack, 0.5), wail), 1.5)


def gen_ab_zap():
    L = n(0.35)
    out = []
    for i in range(L):
        t = i / SR
        idx = 8 * (1 - i / L)
        out.append(math.sin(2 * math.pi * 1200 * t + idx * math.sin(2 * math.pi * 170 * t)) * math.exp(-10 * t) * 0.7)
    return fit(add(out, gain(_crackle(L, 0.02, 0.01), 0.5)), 0.35)


# ---- the player's side ---------------------------------------------------------------------------

def gen_ab_clang():
    L = n(1.2)
    out = [0.0] * L
    for f, amp, dec in ((523, 0.4, 2.5), (1347, 0.3, 3.5), (2210, 0.22, 5.0), (3050, 0.15, 7.0)):
        for i in range(L):
            t = i / SR
            out[i] += math.sin(2 * math.pi * f * t) * amp * math.exp(-dec * t)
    shimmer = [math.sin(2 * math.pi * 5274 * i / SR) * 0.08 * (1 + math.sin(2 * math.pi * 23 * i / SR)) * math.exp(-4 * i / L) for i in range(L)]
    hit = [random.uniform(-1, 1) * math.exp(-400 * k / SR) * 0.8 for k in range(n(0.01))]
    return fit(add(out, shimmer, hit), 1.2)


def gen_ab_shatter():
    L = n(1.5)
    glass = [0.0] * L
    for _ in range(60):
        s0 = random.randint(0, n(0.5))
        f = random.uniform(3000, 7000)
        g = n(random.uniform(0.03, 0.12))
        for k in range(g):
            if s0 + k < L:
                glass[s0 + k] += math.sin(2 * math.pi * f * k / SR) * 0.12 * math.exp(-6 * k / g)
    choir = [0.0] * L
    for f in (523.25, 783.99, 1046.5):
        for i in range(L):
            choir[i] += math.sin(2 * math.pi * f * i / SR) * 0.18 * env(i, L, 0.01, 1.2, 1.5)
    return fit(add(glass, choir), 1.5)


def gen_ab_creak():
    L = n(0.40)
    ph, out = 0.0, []
    for i in range(L):
        f = 90 * (1 + 0.25 * math.sin(2 * math.pi * 11 * i / SR))
        ph += f / SR
        out.append((2 * (ph % 1.0) - 1) * 0.4 * env(i, L, 0.05, 0.1))
    crack = pad([0.0] * n(0.3), n(0.3)) + [random.uniform(-1, 1) * math.exp(-500 * k / SR) for k in range(n(0.02))]
    return fit(add(onepole_lp(out, 1500), crack), 0.40)


def gen_ab_tsk():
    L = n(0.12)
    return fit([s * math.exp(-9 * i / L) * 0.9 for i, s in enumerate(onepole_hp(noise(L), 2500))], 0.12)   # (a dry tick with enough body to be heard)


def gen_ab_wrong():
    L = n(0.35)
    return fit(onepole_lp([(square(110, i) + square(116, i)) * 0.3 * env(i, L, 0.005, 0.05) for i in range(L)], 2500), 0.35)


ABSOLUTE_GENERATORS = {
    "ab_wind_jab": gen_ab_wind_jab, "ab_wind_cross": gen_ab_wind_cross, "ab_wind_hook": gen_ab_wind_hook,
    "ab_wind_spin": gen_ab_wind_spin, "ab_wind_meteor": gen_ab_wind_meteor, "ab_wind_blade": gen_ab_wind_blade,
    "ab_thrum": gen_ab_thrum, "ab_whistle": gen_ab_whistle, "ab_ray_charge": gen_ab_ray_charge, "ab_rumble": gen_ab_rumble,
    "ab_ting": gen_ab_ting, "ab_doom": gen_ab_doom, "ab_pulse": gen_ab_pulse,
    "ab_boom_jab": gen_ab_boom_jab, "ab_boom_jab_1": gen_ab_boom_jab, "ab_boom_jab_2": gen_ab_boom_jab,
    "ab_boom_cross": gen_ab_boom_cross, "ab_boom_hook": gen_ab_boom_hook, "ab_boom_heavy": gen_ab_boom_heavy,
    "ab_boom_kick": gen_ab_boom_kick, "ab_boom_stone": gen_ab_boom_stone, "ab_boom_clap": gen_ab_boom_clap,
    "ab_ray_beam": gen_ab_ray_beam, "ab_zray_beam": gen_ab_zray_beam,
    "ab_takeoff": gen_ab_takeoff, "ab_flight": gen_ab_flight, "ab_shotdown": gen_ab_shotdown, "ab_zap": gen_ab_zap,
    "ab_clang": gen_ab_clang, "ab_shatter": gen_ab_shatter, "ab_creak": gen_ab_creak, "ab_tsk": gen_ab_tsk, "ab_wrong": gen_ab_wrong,
}
