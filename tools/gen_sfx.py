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


def gen_jump():
    n = n_samples(0.16)
    out = []
    for i in range(n):
        t = i / n
        freq = 300 + 500 * t
        out.append(sine(freq, i) * math.exp(-4.0 * t))
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


def gen_hit():
    n = n_samples(0.09)
    noise = highpass([white() for _ in range(n)], 1500)
    out = []
    for i in range(n):
        t = i / n
        out.append(noise[i] * math.exp(-22.0 * t))
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


def gen_telegraph():
    n = n_samples(0.42)
    out = []
    for i in range(n):
        t = i / n
        freq = 500 + 700 * t
        env = 0.5 - 0.5 * math.cos(math.pi * min(1.0, t * 3))  # quick fade-in
        env *= math.exp(-1.5 * t)
        out.append(sine(freq, i) * env)
    return out


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
}


def main():
    random.seed(1234)  # reproducible output
    os.makedirs(OUT_DIR, exist_ok=True)
    for name, gen in GENERATORS.items():
        write_wav(name, gen())


if __name__ == "__main__":
    main()
