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
}


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
