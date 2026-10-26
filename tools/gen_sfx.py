#!/usr/bin/env python3
"""
Procedurally synthesizes the game's sound effects into assets/sfx/*.wav.

Matches the project's philosophy of generating assets in code instead of
shipping external binary/media files (see src/TextureGen.h for the visual
equivalent). Re-run this after changing a sound's parameters below:

    python3 tools/gen_sfx.py
"""
import math
import random
import struct
import wave
import os

SR = 44100
OUT_DIR = os.path.join(os.path.dirname(__file__), "..", "assets", "sfx")


def write_wav(name, samples):
    path = os.path.join(OUT_DIR, name + ".wav")
    peak = max(1e-9, max(abs(s) for s in samples))
    scale = 0.92 / peak if peak > 0.92 else 1.0
    frames = bytearray()
    for s in samples:
        v = int(max(-1.0, min(1.0, s * scale)) * 32767)
        frames += struct.pack("<h", v)
    with wave.open(path, "wb") as f:
        f.setnchannels(1)
        f.setsampwidth(2)
        f.setframerate(SR)
        f.writeframes(bytes(frames))
    print(f"wrote {path} ({len(samples)/SR*1000:.0f} ms)")


def n_samples(seconds):
    return int(SR * seconds)


def sine(freq, i, phase=0.0):
    return math.sin(2 * math.pi * freq * i / SR + phase)


def white():
    return random.uniform(-1.0, 1.0)


def lowpass(samples, cutoff_hz):
    """Simple one-pole lowpass filter."""
    rc = 1.0 / (2 * math.pi * cutoff_hz)
    dt = 1.0 / SR
    alpha = dt / (rc + dt)
    out = [0.0] * len(samples)
    prev = 0.0
    for i, s in enumerate(samples):
        prev = prev + alpha * (s - prev)
        out[i] = prev
    return out


def highpass(samples, cutoff_hz):
    rc = 1.0 / (2 * math.pi * cutoff_hz)
    dt = 1.0 / SR
    alpha = rc / (rc + dt)
    out = [0.0] * len(samples)
    prev_in = 0.0
    prev_out = 0.0
    for i, s in enumerate(samples):
        cur = alpha * (prev_out + s - prev_in)
        out[i] = cur
        prev_in = s
        prev_out = cur
    return out


def mix(*tracks):
    n = max(len(t) for t in tracks)
    out = [0.0] * n
    for t in tracks:
        for i, s in enumerate(t):
            out[i] += s
    return out


def gen_land():
    n = n_samples(0.14)
    noise = [white() for _ in range(n)]
    noise = lowpass(noise, 900)
    out = []
    for i in range(n):
        t = i / n
        thump = sine(90 - 40 * t, i) * math.exp(-9.0 * t)
        out.append(thump * 0.8 + noise[i] * math.exp(-14.0 * t) * 0.6)
    return out


def gen_dash():
    n = n_samples(0.20)
    noise = highpass([white() for _ in range(n)], 1200)
    out = []
    for i in range(n):
        t = i / n
        sweep = sine(1800 - 1400 * t, i)
        out.append((sweep * 0.5 + noise[i] * 0.7) * math.exp(-5.5 * t))
    return out


def gen_slam():
    n = n_samples(0.32)
    noise = lowpass([white() for _ in range(n)], 500)
    out = []
    for i in range(n):
        t = i / n
        thud = sine(70 - 30 * t, i) * math.exp(-6.0 * t)
        out.append(thud * 1.1 + noise[i] * math.exp(-10.0 * t) * 0.7)
    return out


def gen_revolver():
    n = n_samples(0.16)
    noise = [white() for _ in range(n)]
    crack = highpass(noise, 800)
    body = lowpass(noise, 2200)
    out = []
    for i in range(n):
        t = i / n
        punch = sine(140, i) * math.exp(-30.0 * t)
        out.append(crack[i] * math.exp(-25.0 * t) * 0.9
                    + body[i] * math.exp(-18.0 * t) * 0.6
                    + punch * 0.7)
    return out


def gen_shotgun():
    n = n_samples(0.28)
    noise = [white() for _ in range(n)]
    body = lowpass(noise, 1400)
    out = []
    for i in range(n):
        t = i / n
        punch = sine(85, i) * math.exp(-9.0 * t)
        out.append(body[i] * math.exp(-8.0 * t) * 1.0 + punch * 0.9)
    return out


def gen_reload():
    n = n_samples(0.28)
    out = [0.0] * n
    for click_start in (0.0, 0.14):
        start = n_samples(click_start)
        cn = n_samples(0.03)
        for i in range(cn):
            if start + i >= n:
                break
            t = i / cn
            out[start + i] += (sine(2400, i) * 0.6 + white() * 0.4) * math.exp(-40.0 * t)
    return out


def gen_grapple_fire():
    n = n_samples(0.22)
    out = []
    for i in range(n):
        t = i / n
        sweep = sine(500 + 2200 * t, i)
        ring = sine(1800, i) * 0.3
        out.append((sweep * 0.7 + ring) * math.exp(-6.0 * t))
    return out


def gen_enemy_death():
    n = n_samples(0.30)
    noise = lowpass([white() for _ in range(n)], 1000)
    out = []
    for i in range(n):
        t = i / n
        tone = sine(500 - 380 * t, i) * math.exp(-5.0 * t)
        out.append(tone * 0.8 + noise[i] * math.exp(-9.0 * t) * 0.5)
    return out


def gen_player_hit():
    n = n_samples(0.22)
    noise = lowpass([white() for _ in range(n)], 700)
    out = []
    for i in range(n):
        t = i / n
        tone = sine(160 - 60 * t, i) * math.exp(-7.0 * t)
        out.append(tone * 0.9 + noise[i] * math.exp(-8.0 * t) * 0.8)
    return out


def gen_parry():
    n = n_samples(0.18)
    noise = highpass([white() for _ in range(n)], 2000)
    out = []
    for i in range(n):
        t = i / n
        ring = sine(2600, i) * math.exp(-10.0 * t) + sine(3900, i) * 0.5 * math.exp(-14.0 * t)
        out.append(ring * 0.6 + noise[i] * math.exp(-30.0 * t) * 0.8)
    return out


# ---- kill confirms: a short tag on a kill that says what did it (Gameplay_Combat.h) ----
def gen_kill_light():
    """Revolver: a crisp double tick with a short bright ring."""
    n = n_samples(0.16)
    noise = highpass([white() for _ in range(n)], 3000)
    out = []
    for i in range(n):
        t = i / SR
        tick = noise[i] * (math.exp(-t * 260.0) + (0.7 * math.exp(-(t - 0.035) * 260.0) if t >= 0.035 else 0.0))
        ring = (sine(1760, i) + 0.4 * sine(2640, i)) * math.exp(-t * 32.0) * (1.0 if t >= 0.035 else 0.0)
        out.append(tick * 0.8 + ring * 0.35)
    return out


def gen_kill_heavy():
    """Shotgun and blasts: a low chesty thunk with a dry crack on top."""
    n = n_samples(0.2)
    noise = lowpass([white() for _ in range(n)], 2200)
    out = []
    for i in range(n):
        t = i / SR
        body = sine(95 - 40 * min(1.0, t / 0.12), i) * math.exp(-t * 22.0)
        crack = noise[i] * math.exp(-t * 120.0)
        out.append(body * 0.9 + crack * 0.6)
    return out


def gen_kill_rifle():
    """Rifles: a high metallic ping that hangs a moment (a plate rung at range)."""
    n = n_samples(0.4)
    noise = highpass([white() for _ in range(n)], 4000)
    out = []
    for i in range(n):
        t = i / SR
        ping = (sine(2350, i) + 0.55 * sine(3790, i) * math.exp(-t * 6.0) + 0.3 * sine(5170, i) * math.exp(-t * 14.0))
        out.append(ping * math.exp(-t * 9.0) * 0.5 + noise[i] * math.exp(-t * 300.0) * 0.6)
    return out


def gen_kill_blade():
    """The sword: a steel shing that rises a hair as it rings out."""
    n = n_samples(0.32)
    noise = highpass([white() for _ in range(n)], 5000)
    out = []
    for i in range(n):
        t = i / SR
        f = 1480 + 120 * min(1.0, t / 0.08)
        ring = (math.sin(2 * math.pi * (1480 * t + 60 * min(t, 0.08) ** 2 / 0.08)) + 0.5 * sine(f * 2.01, i))
        swish = noise[i] * math.exp(-((t - 0.02) / 0.025) ** 2)
        out.append(ring * math.exp(-t * 11.0) * 0.45 + swish * 0.5)
    return out


def gen_telegraph():
    """An enemy winding up: a short dry tick with a muted low blip under it.
    (It used to be a rising 500-1200 Hz sine, the "woop" that played every
    time anything aimed at you.) Quiet and short so a room full of gunners
    reads as a texture, not a siren."""
    n = n_samples(0.11)
    noise = highpass([white() for _ in range(n)], 2500)
    out = []
    for i in range(n):
        t = i / n
        tick = noise[i] * math.exp(-60.0 * t) * 0.5
        blip = sine(260, i) * math.exp(-22.0 * t) * 0.45
        out.append(tick + blip)
    return lowpass(out, 5000)


def gen_explosion():
    n = n_samples(0.6)
    noise = [white() for _ in range(n)]
    rumble = lowpass(noise, 250)
    crackle = highpass(noise, 1500)
    out = []
    for i in range(n):
        t = i / n
        punch = sine(60, i) * math.exp(-6.0 * t)
        out.append(rumble[i] * math.exp(-3.0 * t) * 1.1
                    + crackle[i] * math.exp(-14.0 * t) * 0.5
                    + punch * 0.8)
    return out


def gen_wave():
    """Wave / arena stinger: a low two-note horn with a metallic edge."""
    n = n_samples(0.9)
    out = []
    for i in range(n):
        t = i / n
        f = 110 if t < 0.35 else 147
        env = min(1.0, t * 20) * math.exp(-2.2 * t)
        tone = sine(f, i) + 0.5 * sine(f * 2, i) + 0.25 * sine(f * 3.01, i)
        out.append(tone * env * 0.6)
    return lowpass(out, 1800)


def gen_spawn():
    """Enemy materialising: a low electric crackle that swells and cuts."""
    n = n_samples(0.38)
    noise = bandish = highpass(lowpass([white() for _ in range(n)], 4000), 600)
    out = []
    for i in range(n):
        t = i / n
        env = min(1.0, t * 6.0) * math.exp(-5.0 * t)
        crackle = noise[i] * (0.6 + 0.4 * math.copysign(1.0, sine(31, i)))
        hum = sine(110, i) * 0.5 + sine(165, i) * 0.25
        out.append((crackle * 0.45 + hum * 0.5) * env)
    return out


def gen_pickup():
    """Health orb: a quick bright two-tone chime."""
    n = n_samples(0.22)
    out = []
    for i in range(n):
        t = i / n
        f = 880 if t < 0.4 else 1320
        out.append(sine(f, i) * math.exp(-6.0 * t) * 0.7)
    return out


def gen_kar():
    """Bolt-action rifle: a sharp supersonic crack with a long rolling tail."""
    n = n_samples(0.55)
    noise = [white() for _ in range(n)]
    crack = highpass(noise, 2500)
    body = lowpass(noise, 1200)
    out = []
    for i in range(n):
        t = i / n
        punch = sine(110 - 50 * t, i) * math.exp(-18.0 * t)
        out.append(crack[i] * math.exp(-60.0 * t) * 1.1
                   + body[i] * math.exp(-7.0 * t) * 0.55
                   + punch * 0.8)
    return out


def gen_longshot():
    """Heavy .50 sniper: a huge low boom, a crack on top and a long echo."""
    n = n_samples(1.1)
    noise = [white() for _ in range(n)]
    crack = highpass(noise, 1800)
    boom = lowpass(noise, 350)
    out = []
    for i in range(n):
        t = i / n
        sub = sine(52 - 18 * t, i) * math.exp(-5.0 * t)
        echo = 0.0
        if t > 0.22:
            te = t - 0.22
            echo = boom[i - n_samples(0.24)] * math.exp(-6.0 * te) * 0.35
        out.append(crack[i] * math.exp(-45.0 * t) * 0.9
                   + boom[i] * math.exp(-3.5 * t) * 1.2
                   + sub * 1.0 + echo)
    return out


def gen_bolt():
    """Rifle bolt: lift-click, a sliding rasp back and forward, lock-click."""
    n = n_samples(0.42)
    out = [0.0] * n
    def click(at, freq, amp):
        start = n_samples(at)
        cn = n_samples(0.025)
        for i in range(cn):
            if start + i < n:
                t = i / cn
                out[start + i] += (sine(freq, i) * 0.5 + white() * 0.5) * math.exp(-35.0 * t) * amp
    def rasp(at, dur, amp):
        start = n_samples(at)
        rn = n_samples(dur)
        r = highpass([white() for _ in range(rn)], 2500)
        for i in range(rn):
            if start + i < n:
                t = i / rn
                out[start + i] += r[i] * math.sin(math.pi * t) * amp
    click(0.0, 1800, 0.8)
    rasp(0.06, 0.09, 0.45)
    click(0.15, 1300, 0.7)
    rasp(0.22, 0.08, 0.45)
    click(0.33, 2200, 1.0)
    return out


def gen_scope():
    """Raising the scope: a short soft lens whoosh."""
    n = n_samples(0.18)
    noise = lowpass([white() for _ in range(n)], 1800)
    out = []
    for i in range(n):
        t = i / n
        out.append(noise[i] * math.sin(math.pi * t) * 0.6 + sine(600 + 400 * t, i) * math.exp(-8 * t) * 0.15)
    return out


def gen_levelup():
    """Level up: a bright rising major arpeggio."""
    notes = [523.25, 659.25, 783.99, 1046.5]
    step = n_samples(0.07)
    n = step * len(notes) + n_samples(0.35)
    out = [0.0] * n
    for k, f in enumerate(notes):
        start = k * step
        ln = n - start
        for i in range(ln):
            t = i / ln
            out[start + i] += (sine(f, i) + 0.4 * sine(f * 2, i)) * math.exp(-5.0 * t) * 0.35
    return out


def gen_potion():
    """Health potion: a couple of glugs and a warm chime."""
    n = n_samples(0.45)
    out = []
    for i in range(n):
        t = i / n
        glug = 0.0
        for g0 in (0.0, 0.1):
            if t * 0.45 >= g0:
                tg = t * 0.45 - g0
                glug += sine(180 + 260 * tg * 10, i) * math.exp(-40.0 * tg) * 0.6
        chime = (sine(880, i) + 0.6 * sine(1318.5, i)) * math.exp(-4.0 * t) * (0.4 if t > 0.35 else 0.0)
        out.append(glug + chime)
    return out


def gen_barrier():
    """Bumping the invisible ceiling: a soft electric fizz."""
    n = n_samples(0.25)
    noise = highpass([white() for _ in range(n)], 3000)
    out = []
    for i in range(n):
        t = i / n
        buzz = math.copysign(1.0, sine(120, i)) * 0.15
        out.append((noise[i] * 0.5 + buzz + sine(1500, i) * 0.2) * math.exp(-9.0 * t))
    return lowpass(out, 6000)


def gen_split():
    """Section split: two quick high blips."""
    n = n_samples(0.22)
    out = []
    for i in range(n):
        t = i / n
        f = 1568 if t < 0.45 else 2093
        out.append(sine(f, i) * math.exp(-6.0 * (t % 0.45)) * 0.5)
    return out


def gen_upgrade():
    """Buying an upgrade: a heavy mechanical clunk and a rising tone."""
    n = n_samples(0.35)
    noise = lowpass([white() for _ in range(n)], 900)
    out = []
    for i in range(n):
        t = i / n
        clunk = (noise[i] * 0.8 + sine(90, i) * 0.6) * math.exp(-18.0 * t)
        tone = sine(440 + 440 * t, i) * math.exp(-4.0 * t) * 0.3
        out.append(clunk + tone)
    return out


def gen_door():
    """A door parting: a pneumatic hiss over a short mechanical slide, with a
    thunk as the locks release."""
    n = n_samples(0.42)
    hiss = highpass([white() for _ in range(n)], 3000)
    rumble = lowpass([white() for _ in range(n)], 300)
    out = []
    for i in range(n):
        t = i / n
        thunk = sine(70, i) * math.exp(-30.0 * t) * 0.9
        h = hiss[i] * (math.exp(-7.0 * t) * min(1.0, t * 30.0)) * 0.5
        r = rumble[i] * math.sin(math.pi * min(1.0, t * 1.4)) * 0.8
        out.append(thunk + h + r)
    return out


def gen_door_close():
    """A door shutting: the slide, then a heavy clunk as it seals."""
    n = n_samples(0.45)
    rumble = lowpass([white() for _ in range(n)], 300)
    hiss = highpass([white() for _ in range(n)], 2500)
    out = []
    hit = 0.62
    for i in range(n):
        t = i / n
        r = rumble[i] * math.sin(math.pi * min(1.0, t / hit)) * 0.7 if t < hit else 0.0
        clunk = 0.0
        if t >= hit:
            tc = (t - hit) * 0.45 / 0.45
            clunk = (sine(62, i) * 1.0 + rumble[i] * 1.2) * math.exp(-16.0 * tc / (1 - hit))
        h = hiss[i] * math.exp(-12.0 * max(0.0, t - hit)) * (0.25 if t >= hit else 0.08)
        out.append(r + clunk + h)
    return out


def gen_boost():
    """Entering a boost tube: a fast rising air rush with a low kick."""
    n = n_samples(0.55)
    noise = [white() for _ in range(n)]
    out = []
    lp = 0.0
    for i in range(n):
        t = i / n
        cutoff = 400 + 5000 * t * t                      # the rush opens up
        a = (1.0 / SR) / (1.0 / (2 * math.pi * cutoff) + 1.0 / SR)
        lp += a * (noise[i] - lp)
        env = min(1.0, t * 12.0) * math.exp(-3.5 * t)
        kick = sine(55 + 40 * (1 - t), i) * math.exp(-18.0 * t)
        out.append(lp * env * 1.4 + kick * 0.8)
    return out


def gen_wade():
    """Wading: a soft low slosh with a short bright splash on top."""
    n = n_samples(0.32)
    noise = lowpass([white() for _ in range(n)], 900)
    out = []
    for i in range(n):
        t = i / n
        env = min(1.0, t * 20.0) * math.exp(-6.0 * t)
        out.append(noise[i] * env * 1.6 + sine(140 - 60 * t, i) * math.exp(-14.0 * t) * 0.25)
    return out


def gen_skim():
    """Skimming water on a slide: a hissing spray that rises and falls."""
    n = n_samples(0.6)
    noise = highpass([white() for _ in range(n)], 1800)
    out = []
    for i in range(n):
        t = i / n
        env = math.sin(math.pi * t) ** 0.6
        out.append(noise[i] * env * 0.9)
    return out


def _click(out, at, freq, amp, decay=45.0, dur=0.03):
    start = n_samples(at)
    cn = n_samples(dur)
    for i in range(cn):
        if start + i < len(out):
            t = i / cn
            out[start + i] += (sine(freq, i) * 0.5 + white() * 0.5) * math.exp(-decay * t) * amp


def gen_cyl_open():
    """Revolver cylinder swinging out: a latch click and a short ratchet."""
    out = [0.0] * n_samples(0.25)
    _click(out, 0.0, 2600, 0.9)
    for k in range(4):
        _click(out, 0.06 + k * 0.025, 3400 - k * 200, 0.35, 60.0, 0.015)
    return lowpass(out, 9000)


def gen_cyl_close():
    """Cylinder snapping home: a hard metallic snap with a ring."""
    n = n_samples(0.3)
    out = [0.0] * n
    _click(out, 0.0, 1900, 1.0, 30.0, 0.05)
    for i in range(n):
        t = i / n
        out[i] += (sine(3150, i) * 0.25 + sine(4700, i) * 0.12) * math.exp(-14.0 * t)
    return out


def gen_eject():
    """Spent brass tumbling out: a cluster of bright pings."""
    out = [0.0] * n_samples(0.5)
    random.seed(77)
    for k in range(8):
        at = 0.02 + k * 0.035 + random.uniform(0, 0.02)
        f = random.uniform(3800, 6200)
        start = n_samples(at)
        for i in range(n_samples(0.12)):
            if start + i < len(out):
                t = i / n_samples(0.12)
                out[start + i] += sine(f, i) * math.exp(-28.0 * t) * 0.3
    return out


def gen_shell_in():
    """A round or shell seating: a soft thunk and a click."""
    n = n_samples(0.14)
    body = lowpass([white() for _ in range(n)], 1200)
    out = []
    for i in range(n):
        t = i / n
        out.append(body[i] * math.exp(-25.0 * t) * 0.8 + sine(180, i) * math.exp(-30.0 * t) * 0.5)
    _click(out, 0.05, 2200, 0.6)
    return out


def gen_pump():
    """Racking the pump: slide back with a clack, slide forward with a clack."""
    out = [0.0] * n_samples(0.42)
    for at, slide_len in ((0.0, 0.09), (0.19, 0.08)):
        start = n_samples(at)
        r = highpass([white() for _ in range(n_samples(slide_len))], 1800)
        for i in range(len(r)):
            if start + i < len(out):
                out[start + i] += r[i] * math.sin(math.pi * i / len(r)) * 0.35
        _click(out, at + slide_len, 1500, 1.0, 35.0, 0.04)
    return out


def gen_sword_swing():
    """A blade through the air: a fast band-passed whoosh that rises then falls."""
    n = n_samples(0.22)
    noise = highpass(lowpass([white() for _ in range(n)], 5000), 700)
    return [noise[i] * math.sin(math.pi * i / n) ** 1.5 * 0.8 for i in range(n)]


def gen_sword_hit():
    """Steel through something hard: a bright inharmonic ring over a short crunch."""
    n = n_samples(0.35)
    crunch = lowpass([white() for _ in range(n)], 2500)
    out = []
    for i in range(n):
        t = i / n
        ring = sine(1870, i) * math.exp(-9 * t) + sine(2930, i) * 0.6 * math.exp(-12 * t) + sine(4410, i) * 0.3 * math.exp(-16 * t)
        out.append(ring * 0.55 + crunch[i] * math.exp(-40 * t) * 0.9)
    return out


def gen_sword_draw():
    """Drawing it: a scrape that brightens, ending on a ring."""
    n = n_samples(0.5)
    noise = highpass([white() for _ in range(n)], 3000)
    out = []
    for i in range(n):
        t = i / n
        ring = sine(2400, i) * 0.45 * math.exp(-8 * (t - 0.75)) if t > 0.75 else 0.0
        out.append(noise[i] * 0.35 * t * (1.0 if t < 0.8 else math.exp(-20 * (t - 0.8))) + ring)
    return out


def gen_sword_thrust():
    """The thrust: a heavy push of air with a steel edge on it."""
    n = n_samples(0.3)
    air = lowpass([white() for _ in range(n)], 1200)
    return [air[i] * math.sin(math.pi * i / n) * 0.9 + sine(1600, i) * 0.15 * math.exp(-6 * i / n) for i in range(n)]


def gen_sword_guard():
    """Raising the guard: a short metallic shink."""
    n = n_samples(0.12)
    return [sine(3200, i) * math.exp(-25 * i / n) * 0.5 + white() * math.exp(-60 * i / n) * 0.4 for i in range(n)]


def gen_sword_block():
    """A blow on the blade: a hard clang."""
    n = n_samples(0.4)
    return [(sine(620, i) + sine(1340, i) * 0.7 + sine(2210, i) * 0.4) * math.exp(-8 * i / n) * 0.5
            + white() * math.exp(-50 * i / n) * 0.5 for i in range(n)]


def gen_sword_break():
    """The guard breaks: a crack and a dying, detuned ring."""
    n = n_samples(0.6)
    return [white() * math.exp(-30 * i / n) * 0.9 + (sine(880, i) + sine(905, i)) * 0.35 * math.exp(-5 * i / n) for i in range(n)]


# ---- movement and hit feel (CameraFeel.h / Gameplay_Tick.h) ----
def _bandnoise(n, lo, hi):
    return lowpass(highpass([white() for _ in range(n)], lo), hi)


def gen_jump():
    """The push-off: a short low thump under a soft scuff (was a bright chirp)."""
    n = n_samples(0.14)
    scuff = _bandnoise(n, 500, 2500)
    out = []
    for i in range(n):
        t = i / SR
        thump = sine(115 - 300 * t, i) * math.exp(-t * 38.0)
        att = min(1.0, t / 0.004)
        out.append(thump * 0.9 + scuff[i] * att * math.exp(-t * 30.0) * 0.55)
    return out


def gen_jump_air():
    """The air jump: a rising whoosh with a faint thrust tone and a kick at its start."""
    n = n_samples(0.30)
    air = highpass([white() for _ in range(n)], 900)
    # sweep a one-pole lowpass upward across the sound
    out, prev = [], 0.0
    for i in range(n):
        u = i / n
        t = i / SR
        cut = 1200 + 5000 * u
        rc = 1.0 / (2 * math.pi * cut)
        a = (1.0 / SR) / (rc + 1.0 / SR)
        prev += a * (air[i] - prev)
        env = math.sin(math.pi * min(1.0, u * 1.6)) ** 1.5 if u < 0.625 else math.exp(-(u - 0.625) * 9.0)
        tone = sine(260 + 280 * u, i) * 0.18 * env
        kick = sine(90 - 120 * t, i) * math.exp(-t * 45.0) * 0.7
        out.append(prev * env * 0.9 + tone + kick)
    return out


def gen_land_heavy():
    """A big drop: a deep thump, a gritty crunch and a few bits of debris settling."""
    n = n_samples(0.5)
    crunch = lowpass([white() for _ in range(n)], 1800)
    out = [0.0] * n
    for i in range(n):
        t = i / SR
        thump = sine(62 - 50 * t, i) * math.exp(-t * 9.0)
        sub = sine(38, i) * math.exp(-t * 7.0) * 0.5
        out[i] = thump * 1.0 + sub + crunch[i] * math.exp(-t * 22.0) * 0.8
    for k in range(5):   # debris ticks
        at = int(SR * random.uniform(0.06, 0.32))
        f = random.uniform(1800, 3800)
        amp = random.uniform(0.08, 0.18)
        for j in range(int(SR * 0.025)):
            if at + j < n:
                tt = j / SR
                out[at + j] += (sine(f, j) * 0.6 + white() * 0.4) * amp * math.exp(-tt * 180.0)
    return out


def gen_slide():
    """Into a slide: a gritty scrape (grainy band of noise) over a low rumble, easing off."""
    n = n_samples(0.6)
    grit = _bandnoise(n, 1200, 4500)
    rumble = lowpass([white() for _ in range(n)], 220)
    out, grain = [], 1.0
    for i in range(n):
        t = i / SR
        u = i / n
        if i % 90 == 0:
            grain = random.uniform(0.45, 1.0)   # the surface's texture
        env = min(1.0, t / 0.02) * (1.0 - u) ** 1.4
        out.append(grit[i] * grain * env * 0.8 + rumble[i] * env * 1.6 + sine(70, i) * env * 0.12)
    return out


def gen_hit():
    """A hit landing: a hard click, a quick tonal thwack falling in pitch, a low knock."""
    n = n_samples(0.11)
    click = highpass([white() for _ in range(n)], 2500)
    out, ph = [], 0.0
    for i in range(n):
        t = i / SR
        f = 900 + 700 * math.exp(-t * 60.0)
        ph += 2 * math.pi * f / SR
        body = math.sin(ph) * math.exp(-t * 48.0)
        knock = sine(150 - 300 * t, i) * math.exp(-t * 55.0)
        out.append(click[i] * math.exp(-t * 900.0) * 0.9 + body * 0.55 + knock * 0.7)
    return out


def gen_hit_head():
    """A crit on top of the hit: a short bright metallic ping (inharmonic, so it reads as armour)."""
    n = n_samples(0.26)
    out = []
    for i in range(n):
        t = i / SR
        ring = (sine(2350, i) * math.exp(-t * 16.0) + sine(3470, i) * 0.6 * math.exp(-t * 22.0)
                + sine(5230, i) * 0.35 * math.exp(-t * 30.0))
        tick = white() * math.exp(-t * 700.0) * 0.6
        out.append(ring * 0.55 + tick)
    return out


def gen_ammo_low():
    """The last rounds: a small dry mechanical tick (pitched up by the game as the mag empties)."""
    n = n_samples(0.07)
    out = []
    for i in range(n):
        t = i / SR
        out.append(white() * math.exp(-t * 900.0) * 0.6 + sine(3100, i) * math.exp(-t * 110.0) * 0.5
                   + sine(1550, i) * math.exp(-t * 140.0) * 0.25)
    return out


def gen_heartbeat():
    """Low health: lub-dub, two soft low thumps felt more than heard."""
    n = n_samples(0.42)
    out = [0.0] * n
    for at, amp, f0 in ((0.0, 1.0, 58.0), (0.17, 0.7, 52.0)):
        s0 = int(at * SR)
        for j in range(n - s0):
            t = j / SR
            env = min(1.0, t / 0.008) * math.exp(-t * 22.0)
            out[s0 + j] += (sine(f0 - 18.0 * t, j) + 0.35 * sine(2 * f0, j)) * env * amp
    return lowpass(out, 400)


# ---- the world answering a shot (Gameplay_Combat.h fireWeapon) ----
def _impact(seed_pitch):
    """A round hitting a wall: a hard crack, a short ring off the surface and grit falling away."""
    n = n_samples(0.32)
    crack = highpass([white() for _ in range(n)], 1800)
    grit = _bandnoise(n, 900, 5000)
    out = [0.0] * n
    f = 1150 * seed_pitch
    for i in range(n):
        t = i / SR
        ring = (sine(f, i) * 0.6 + sine(f * 1.53, i) * 0.4) * math.exp(-t * 45.0)
        thud = sine(140 * seed_pitch, i) * math.exp(-t * 60.0)
        out[i] = crack[i] * math.exp(-t * 400.0) * 1.0 + ring * 0.35 + thud * 0.5
    for k in range(6):   # bits of the wall pattering down
        at = int(SR * random.uniform(0.03, 0.22))
        amp = random.uniform(0.05, 0.14) * (1.0 - k / 8.0)
        for j in range(int(SR * 0.012)):
            if at + j < n:
                out[at + j] += grit[at + j] * amp * math.exp(-(j / SR) * 300.0)
    return out


def gen_impact():   return _impact(1.0)
def gen_impact_1(): return _impact(0.86)
def gen_impact_2(): return _impact(1.17)


def _casing(base):
    """Brass hitting the floor: two or three bright tinks, each softer and sooner, and a small roll."""
    n = n_samples(0.5)
    out = [0.0] * n
    at, amp, gap = 0.0, 1.0, random.uniform(0.09, 0.13)
    for b in range(3):
        s0 = int(at * SR)
        f = base * random.uniform(0.97, 1.03)
        for j in range(n - s0):
            t = j / SR
            v = (sine(f, j) + 0.5 * sine(f * 2.76, j) + 0.25 * sine(f * 5.4, j)) * math.exp(-t * 38.0)
            v += white() * math.exp(-t * 900.0) * 0.4
            out[s0 + j] += v * amp
        at += gap; gap *= 0.62; amp *= 0.45
    return out


def gen_casing():   return _casing(3300)
def gen_casing_1(): return _casing(3700)
def gen_casing_2(): return _casing(2950)


def _whiz(drop):
    """Something passing close by your head: band-passed air that swells and falls in pitch."""
    n = n_samples(0.28)
    air = [white() for _ in range(n)]
    out, lp1, lp2 = [], 0.0, 0.0
    for i in range(n):
        u = i / n
        cut = 2600 * (1.0 - drop * u) + 300
        a = 1.0 - math.exp(-2 * math.pi * cut / SR)
        lp1 += a * (air[i] - lp1)
        lp2 += a * 0.25 * (air[i] - lp2)
        env = math.exp(-((u - 0.38) ** 2) / 0.018)
        tone = sine(700 * (1.0 - 0.45 * u), i) * 0.12
        out.append(((lp1 - lp2) * 1.6 + tone) * env)
    return out


def gen_whiz():   return _whiz(0.75)
def gen_whiz_1(): return _whiz(0.6)


def gen_hit_armor():
    """A hit on armour: a dull metal clank (inharmonic, low-mid) with a short scrape."""
    n = n_samples(0.2)
    scrape = _bandnoise(n, 1500, 6000)
    out = []
    for i in range(n):
        t = i / SR
        ring = (sine(620, i) + 0.7 * sine(1013, i) + 0.45 * sine(1690, i)) * math.exp(-t * 30.0)
        out.append(ring * 0.5 + scrape[i] * math.exp(-t * 90.0) * 0.6 + white() * math.exp(-t * 800.0) * 0.5)
    return out


# ---- surfaces and moments (Gameplay_Tick.h) ----
def _read(name):
    """A sound already in assets/sfx (mono 16-bit), as floats: the recorded steps are built on."""
    w = wave.open(os.path.join(OUT_DIR, name + ".wav"))
    raw = w.readframes(w.getnframes())
    return [v / 32768.0 for v in struct.unpack("<%dh" % w.getnframes(), raw)]


def _metal_ring(n, f, amp, decay):
    return [(sine(f, i) + 0.6 * sine(f * 2.41, i) + 0.35 * sine(f * 3.93, i)) * amp * math.exp(-(i / SR) * decay) for i in range(n)]


def _step_metal(src, f):
    """A boot on a steel plate: the recorded step, a hollow knock under it and the plate ringing."""
    step = _read(src)
    n = len(step) + n_samples(0.12)
    ring = _metal_ring(n, f, 0.18, 26.0)
    knock = [sine(190, i) * math.exp(-(i / SR) * 40.0) * 0.35 for i in range(n)]
    return mix(step + [0.0] * (n - len(step)), ring, knock)


def gen_step_metal():   return _step_metal("step1", 610)
def gen_step_metal_1(): return _step_metal("step2", 680)
def gen_step_metal_2(): return _step_metal("step3", 560)


def gen_land_metal():
    """Landing on metal: the land thump with a big plate clang over it."""
    land = _read("land")
    n = len(land) + n_samples(0.35)
    ring = _metal_ring(n, 430, 0.32, 9.0)
    clank = [white() * math.exp(-(i / SR) * 300.0) * 0.4 for i in range(n)]
    return mix(land + [0.0] * (n - len(land)), ring, clank)


def gen_lastkill():
    """The last of a wave falls: a deep slowed boom and a low swell that draws in under it."""
    n = n_samples(1.3)
    rumble = lowpass([white() for _ in range(n)], 160)
    out, ph = [], 0.0
    for i in range(n):
        t = i / SR
        f = 55 * math.exp(-t * 1.2) + 30
        ph += 2 * math.pi * f / SR
        boom = math.sin(ph) * math.exp(-t * 2.6)
        swell = sine(110, i) * 0.25 * math.sin(math.pi * min(1.0, t / 1.3)) * math.exp(-t * 1.5)
        out.append(boom * 0.9 + rumble[i] * math.exp(-t * 3.0) * 1.2 + swell)
    return out


# ---- how a fight feels (Gameplay_Feel.h) ----
def gen_hit_body():
    """A round landing in a body: a short heavy thump with a dull plate knock and a crunch on the front.
    The game pitches it down for the big ones."""
    n = n_samples(0.16)
    crunch = highpass([white() for _ in range(n)], 1800)
    out, ph = [], 0.0
    for i in range(n):
        t = i / SR
        f = 70 + 160 * math.exp(-t * 45.0)
        ph += 2 * math.pi * f / SR
        thump = math.sin(ph) * math.exp(-t * 26.0)
        knock = (sine(410, i) + 0.6 * sine(673, i)) * math.exp(-t * 60.0)
        out.append(math.tanh(1.6 * thump) * 0.9 + knock * 0.3 + crunch[i] * math.exp(-t * 160.0) * 0.55)
    return out


def gen_stun():
    """Its footing breaks: a hard crack, a metal clunk ringing down, a short electric fizz as its eyes gutter."""
    n = n_samples(0.38)
    fizz = _bandnoise(n, 2500, 7000)
    out = []
    for i in range(n):
        t = i / SR
        crack = white() * math.exp(-t * 400.0)
        f = 520 * math.exp(-t * 3.0)
        clunk = (math.sin(2 * math.pi * f * t) + 0.5 * math.sin(2 * math.pi * f * 1.51 * t)) * math.exp(-t * 14.0)
        gate = 1.0 if math.sin(2 * math.pi * 38 * t) > 0.2 else 0.25
        out.append(crack * 0.8 + clunk * 0.45 + fizz[i] * math.exp(-t * 9.0) * 0.35 * gate)
    return out


def gen_finisher():
    """The finisher: a gauntlet into a body. A deep slugging punch, a crunch of plate giving way,
    a low boom under it, and a bright ring as it breaks."""
    n = n_samples(0.75)
    crunch = lowpass([white() for _ in range(n)], 3200)
    rumble = lowpass([white() for _ in range(n)], 140)
    out, ph = [], 0.0
    for i in range(n):
        t = i / SR
        f = 48 + 190 * math.exp(-t * 30.0)
        ph += 2 * math.pi * f / SR
        punch = math.tanh(2.2 * math.sin(ph) * math.exp(-t * 9.0))
        cr = crunch[i] * (math.exp(-t * 28.0) + 0.5 * math.exp(-abs(t - 0.035) * 120.0))
        ring = (sine(880, i) + 0.6 * sine(1394, i) + 0.4 * sine(2117, i)) * math.exp(-t * 7.0) * min(1.0, t / 0.01)
        out.append(punch * 0.85 + cr * 0.9 + rumble[i] * math.exp(-t * 4.0) * 1.4 + ring * 0.12)
    return out


def gen_overdrive_on():
    """OVERDRIVE: a fast rising rush into a big impact, a power chord swelling out of it and a sub drop."""
    n = n_samples(1.6)
    rush = _bandnoise(n, 800, 6000)
    out, ph, ph2 = [], 0.0, 0.0
    hit = 0.22
    for i in range(n):
        t = i / SR
        pre = rush[i] * (t / hit) ** 2 * 0.6 if t < hit else 0.0
        u = t - hit
        if u >= 0:
            f = 38 + 90 * math.exp(-u * 12.0)
            ph += 2 * math.pi * f / SR
            sub = math.sin(ph) * math.exp(-u * 2.2)
            crack = white() * math.exp(-u * 70.0)
            sw = min(1.0, u / 0.08) * math.exp(-u * 1.6)
            chord = sum(math.tanh(2.5 * sine(fr, i)) for fr in (110, 164.8, 220, 329.6)) * 0.12 * sw
            ph2 += 2 * math.pi * (220 + 30 * u) / SR
            out.append(sub * 0.9 + crack * 0.7 + chord + math.sin(ph2) * 0.05 * sw)
        else:
            out.append(pre)
    return lowpass(out, 9000)


def gen_overdrive_off():
    """OVERDRIVE burning out: the chord sagging down and away, a breath of air, a soft thud."""
    n = n_samples(0.9)
    air = _bandnoise(n, 400, 3000)
    out = []
    for i in range(n):
        t = i / SR
        bend = math.exp(-t * 1.2)
        env = math.exp(-t * 3.5)
        chord = sum(math.tanh(2.0 * math.sin(2 * math.pi * fr * (0.6 + 0.4 * bend) * t)) for fr in (110, 164.8, 220)) * 0.14 * env
        thud = math.sin(2 * math.pi * (60 * bend) * t) * math.exp(-t * 9.0)
        out.append(chord + air[i] * 0.25 * math.sin(math.pi * min(1.0, t / 0.9)) + thud * 0.6)
    return out


def gen_finish_ready():
    """One in reach to finish: a soft low metallic 'tung' and a quick rise above it, short so it never crowds the fight."""
    n = n_samples(0.32)
    out = []
    for i in range(n):
        t = i / SR
        body = (sine(196, i) + 0.5 * sine(311, i)) * math.exp(-t * 11.0) * min(1.0, t / 0.004)
        rise = sine(784 + 400 * min(1.0, t / 0.08), i) * 0.25 * math.exp(-t * 18.0)
        out.append(body * 0.7 + rise + white() * math.exp(-t * 500.0) * 0.15)
    return out


def gen_hook_yank():
    """The hook biting into an enemy: a chain snapping taut (a bright metallic crack and rattle) over a short heavy whoosh."""
    n = n_samples(0.42)
    whoosh = _bandnoise(n, 300, 2500)
    out = []
    for i in range(n):
        t = i / SR
        crack = white() * math.exp(-t * 300.0)
        ring = (sine(1180, i) + 0.6 * sine(1730, i) + 0.4 * sine(2610, i)) * math.exp(-t * 22.0)
        rattle = white() * (0.5 + 0.5 * math.sin(2 * math.pi * 55 * t)) * math.exp(-t * 14.0) * 0.3
        sw = math.sin(math.pi * min(1.0, t / 0.42)) ** 2
        thump = math.sin(2 * math.pi * (90 * math.exp(-t * 8.0) + 40) * t) * math.exp(-t * 16.0)
        out.append(crack * 0.7 + ring * 0.3 + rattle + whoosh[i] * sw * 0.5 + thump * 0.6)
    return out


GENERATORS = {
    "jump": gen_jump,
    "land": gen_land,
    "dash": gen_dash,
    "slam": gen_slam,
    "revolver": gen_revolver,
    "shotgun": gen_shotgun,
    "reload": gen_reload,
    "grapple_fire": gen_grapple_fire,
    "hit": gen_hit,
    "enemy_death": gen_enemy_death,
    "player_hit": gen_player_hit,
    "parry": gen_parry,
    "telegraph": gen_telegraph,
    "explosion": gen_explosion,
    "wave": gen_wave,
    "spawn": gen_spawn,
    "pickup": gen_pickup,
    "kar": gen_kar,
    "longshot": gen_longshot,
    "bolt": gen_bolt,
    "scope": gen_scope,
    "levelup": gen_levelup,
    "potion": gen_potion,
    "barrier": gen_barrier,
    "split": gen_split,
    "upgrade": gen_upgrade,
    "door": gen_door,
    "door_close": gen_door_close,
    "boost": gen_boost,
    "cyl_open": gen_cyl_open,
    "cyl_close": gen_cyl_close,
    "eject": gen_eject,
    "shell_in": gen_shell_in,
    "pump": gen_pump,
    "wade": gen_wade,
    "skim": gen_skim,
    "sword_draw": gen_sword_draw,
    "sword_swing": gen_sword_swing,
    "sword_hit": gen_sword_hit,
    "sword_thrust": gen_sword_thrust,
    "sword_guard": gen_sword_guard,
    "sword_block": gen_sword_block,
    "sword_break": gen_sword_break,
    "kill_light": gen_kill_light,
    "kill_heavy": gen_kill_heavy,
    "kill_rifle": gen_kill_rifle,
    "kill_blade": gen_kill_blade,
    "jump_air": gen_jump_air,
    "land_heavy": gen_land_heavy,
    "slide": gen_slide,
    "hit_head": gen_hit_head,
    "ammo_low": gen_ammo_low,
    "heartbeat": gen_heartbeat,
    "impact": gen_impact, "impact_1": gen_impact_1, "impact_2": gen_impact_2,
    "casing": gen_casing, "casing_1": gen_casing_1, "casing_2": gen_casing_2,
    "whiz": gen_whiz, "whiz_1": gen_whiz_1,
    "hit_armor": gen_hit_armor,
    "step_metal": gen_step_metal, "step_metal_1": gen_step_metal_1, "step_metal_2": gen_step_metal_2,
    "land_metal": gen_land_metal,
    "lastkill": gen_lastkill,
    "hit_body": gen_hit_body, "stun": gen_stun, "finisher": gen_finisher,
    "overdrive_on": gen_overdrive_on, "overdrive_off": gen_overdrive_off, "finish_ready": gen_finish_ready, "hook_yank": gen_hook_yank,
}

from sfx_absolute import ABSOLUTE_GENERATORS  # THE ABSOLUTE's (tools/sfx_absolute.py)
GENERATORS.update(ABSOLUTE_GENERATORS)


def main():
    """No arguments: regenerate every synthesized sound (then re-run
    import_sfx.py for the recorded ones it replaces). With names: only those,
    e.g. `python3 tools/gen_sfx.py door boost`."""
    import sys
    os.makedirs(OUT_DIR, exist_ok=True)
    names = sys.argv[1:]
    if not names:
        random.seed(1234)  # reproducible output
        for name, gen in GENERATORS.items():
            write_wav(name, gen())
        return
    for name in names:
        random.seed(name)
        write_wav(name, GENERATORS[name]())


if __name__ == "__main__":
    main()
