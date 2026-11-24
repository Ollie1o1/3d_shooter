#!/usr/bin/env python3
"""
Builds the game's core combat sounds from REAL recordings (CC0, public domain),
layered the way a sound designer would: a transient, a body, and a tail or
debris layer, each from a different recording, then compressed, limited and
matched to the game's mix class (MixTable.h).

Why this exists: until November 2026 these sounds (explosion, slam, body hits,
surface impacts, landings, getting hit) were synthesized from sine waves and
white noise in tools/gen_sfx.py / gen_combat_audio.py: 0.1 to 0.6 s mono bursts.
However good the recipe, they sound like what they are. Recordings carry the
irregular detail (air, debris, room, mechanical noise) that synthesis can't.

Sources are frozen (mono, 44.1 kHz, 16-bit, peak -1 dBFS) in
assets/sfx/src/cc0/ so the build is reproducible offline; see CREDITS.md.

    python3 tools/build_real_sfx.py              # rebuild every sound below
    python3 tools/build_real_sfx.py explosion    # just one family
    python3 tools/build_real_sfx.py --freeze DIR # (re)freeze sources from the downloaded packs in DIR

Needs ffmpeg on PATH (used for decoding, pitch, filters and the dynamics).
Prints each variant's loudest-50 ms level and the MixTable trim it needs.
"""
import math
import os
import struct
import subprocess
import sys
import tempfile
import wave

SR = 44100
ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..")
SRC = os.path.join(ROOT, "assets", "sfx", "src", "cc0")
OUT = os.path.join(ROOT, "assets", "sfx")

# Mix class targets (dBFS, loudest 50 ms RMS) from MixTable.h mixTargetDb
TARGET = {"GUN": -11.0, "FOLEY": -18.0, "PLAYER": -12.0, "ACTION": -10.0, "WORLD": -13.0, "UI": -12.0, "TELL": -14.0}

# frozen name -> path inside the download folder (for --freeze). All CC0; packs in CREDITS.md
FREEZE = {
    # explosions / bangs
    "chunky_explosion": "Chunky_Explosion.mp3",
    "scifi_explosion_1": "x_sci-fi-sfx/explosion_01.ogg",
    "scifi_explosion_2": "x_sci-fi-sfx/explosion_02.ogg",
    "oga_explosion": "explosion.wav",
    "bang_01": "x_25-CC0-bang-sfx/bang_01.ogg",
    "bang_03": "x_25-CC0-bang-sfx/bang_03.ogg",
    "bang_06": "x_25-CC0-bang-sfx/bang_06.ogg",
    "bang_09": "x_25-CC0-bang-sfx/bang_09.ogg",
    "cannon_02": "x_25-CC0-bang-sfx/cannon_02.ogg",
    "cannon_04": "x_25-CC0-bang-sfx/cannon_04.ogg",
    # debris
    "rock_fall_01": "x_sfx_breaking_and_falling/bfh1_rock_falling_01.ogg",
    "rock_fall_03": "x_sfx_breaking_and_falling/bfh1_rock_falling_03.ogg",
    "rock_fall_04": "x_sfx_breaking_and_falling/bfh1_rock_falling_04.ogg",
    "rock_fall_07": "x_sfx_breaking_and_falling/bfh1_rock_falling_07.ogg",
    "rock_fall_08": "x_sfx_breaking_and_falling/bfh1_rock_falling_08.ogg",
    "rock_break_02": "x_sfx_breaking_and_falling/bfh1_rock_breaking_02.ogg",
    "rock_break_03": "x_sfx_breaking_and_falling/bfh1_rock_breaking_03.ogg",
    "rock_hit_01": "x_sfx_breaking_and_falling/bfh1_rock_hit_01.ogg",
    "metal_fall_01": "x_sfx_breaking_and_falling/bfh1_metal_falling_01.ogg",
    "metal_fall_03": "x_sfx_breaking_and_falling/bfh1_metal_falling_03.ogg",
    "falling_04": "x_sfx_breaking_and_falling/bfh1_falling_04.ogg",
    "falling_05": "x_sfx_breaking_and_falling/bfh1_falling_05.ogg",
    "metal_fall_02": "x_sfx_breaking_and_falling/bfh1_metal_falling_02.ogg",
    # body / punches / generic hits
    "punch_1": "x_punch/qubodupPunch/qubodupPunch01.flac",
    "punch_2": "x_punch/qubodupPunch/qubodupPunch02.flac",
    "punch_3": "x_punch/qubodupPunch/qubodupPunch03.flac",
    "punch_4": "x_punch/qubodupPunch/qubodupPunch04.flac",
    "punch_5": "x_punch/qubodupPunch/qubodupPunch05.flac",
    "hit_02": "x_sfx_breaking_and_falling/bfh1_hit_02.ogg",
    "hit_06": "x_sfx_breaking_and_falling/bfh1_hit_06.ogg",
    "hit_07": "x_sfx_breaking_and_falling/bfh1_hit_07.ogg",
    "hit_08": "x_sfx_breaking_and_falling/bfh1_hit_08.ogg",
    "hit_12": "x_sfx_breaking_and_falling/bfh1_hit_12.ogg",
    # metal
    "metal_hit_02": "x_sfx_breaking_and_falling/bfh1_metal_hit_02.ogg",
    "metal_hit_03": "x_sfx_breaking_and_falling/bfh1_metal_hit_03.ogg",
    "metal_hit_04": "x_sfx_breaking_and_falling/bfh1_metal_hit_04.ogg",
    "metal_hit_05": "x_sfx_breaking_and_falling/bfh1_metal_hit_05.ogg",
    "metal_hit_06": "x_sfx_breaking_and_falling/bfh1_metal_hit_06.ogg",
    "clink_1": "clink1_0.wav",
    "clink_2": "clink2.wav",
    "metal_08": "x_100-CC0-SFX_0/metal_08.ogg",
    "metal_09": "x_100-CC0-SFX_0/metal_09.ogg",
    # wood / glass
    "wood_hit_01": "x_sfx_breaking_and_falling/bfh1_wood_hit_01.ogg",
    "wood_hit_02": "x_sfx_breaking_and_falling/bfh1_wood_hit_02.ogg",
    "wood_hit_03": "x_sfx_breaking_and_falling/bfh1_wood_hit_03.ogg",
    "wood_break_01": "x_sfx_breaking_and_falling/bfh1_wood_breaking_01.ogg",
    "wood_break_03": "x_sfx_breaking_and_falling/bfh1_wood_breaking_03.ogg",
    "glass_hit_01": "x_sfx_breaking_and_falling/bfh1_glass_hit_01.ogg",
    "glass_hit_02": "x_sfx_breaking_and_falling/bfh1_glass_hit_02.ogg",
    "glass_fall_03": "x_sfx_breaking_and_falling/bfh1_glass_falling_03.ogg",
    # slams / thuds
    "slam_02": "x_100-CC0-SFX_0/slam_02.ogg",
    "slam_03": "x_100-CC0-SFX_0/slam_03.ogg",
    "slam_06": "x_100-CC0-SFX_0/slam_06.ogg",
    "door_close_03": "x_100-CC0-SFX_0/door_close_03.ogg",
    "door_close_04": "x_100-CC0-SFX_0/door_close_04.ogg",
    "thud_3": "thud3.wav",
}


def L(src, at=0.0, db=0.0, pitch=1.0, start=0.0, dur=None, hp=None, lp=None, fade=None):
    """One layer: recording `src` from `start` (s) for `dur`, at `pitch` (playback rate),
    filtered, `db` gain, placed `at` seconds in, faded out over its last `fade` s."""
    return dict(src=src, at=at, db=db, pitch=pitch, start=start, dur=dur, hp=hp, lp=lp, fade=fade)


# name -> (mix class, length s, [variant layer lists], dynamics)
# dynamics: "boom" (heavy compression, glue), "hit" (fast punch), "light" (gentle)
RECIPES = {
    # ---- the big ones -------------------------------------------------------------------------
    # Crack (a close bang) + low boom (a sci-fi explosion pitched down) + the long rumble of a
    # real explosion + debris raining after it. ~2.2 s: a real blast takes time to settle.
    "explosion": ("ACTION", 2.3, [
        [L("bang_09", db=0), L("scifi_explosion_1", db=-1, pitch=0.85, lp=900), L("chunky_explosion", at=0.03, start=0.15, db=-6, lp=1400, fade=1.2),
         L("rock_fall_01", at=0.28, db=-15, hp=500, fade=0.3)],
        [L("cannon_02", db=-1), L("bang_03", db=-2, pitch=0.9, lp=700), L("chunky_explosion", at=0.03, start=1.1, db=-7, lp=1300, fade=1.2),
         L("metal_fall_01", at=0.3, db=-17, hp=700, fade=0.3)],
        [L("bang_01", db=0), L("scifi_explosion_2", db=-1, pitch=0.8, lp=800), L("chunky_explosion", at=0.03, start=2.1, db=-6, lp=1200, fade=1.0),
         L("rock_fall_03", at=0.22, db=-16, hp=600, fade=0.3)],
    ], "boom"),
    # A heavy impact on the ground (bosses, falling masses; often played pitched down to 0.33)
    "slam": ("ACTION", 1.3, [
        [L("slam_02", db=0), L("bang_03", db=-5, pitch=0.7, lp=400, fade=0.5), L("rock_break_02", at=0.05, db=-14, hp=300, fade=0.4)],
        [L("metal_08", db=-1, lp=3000), L("slam_06", db=-3), L("scifi_explosion_1", db=-8, pitch=0.7, lp=300, fade=0.6), L("rock_fall_01", at=0.08, db=-16, hp=400, fade=0.4)],
        [L("slam_03", db=0), L("cannon_04", db=-8, pitch=0.6, lp=350, fade=0.5), L("rock_fall_03", at=0.06, db=-15, hp=400, fade=0.4)],
    ], "boom"),
    # ---- every round that lands ---------------------------------------------------------------
    # A round into a body: a real punch (the weight) under a short crunch (the bite). Played
    # pitched by the target's mass at runtime (Gameplay_Tick flushHitSounds), so keep it neutral.
    "hit_body": ("ACTION", 0.32, [
        [L("punch_1", db=0, dur=0.32, fade=0.12), L("hit_08", db=-9, hp=900, dur=0.18, fade=0.08)],
        [L("punch_4", db=0, dur=0.32, fade=0.12), L("hit_06", db=-10, hp=900, dur=0.18, fade=0.08)],
        [L("punch_5", db=0, pitch=0.94, dur=0.32, fade=0.12), L("hit_12", db=-13, hp=1200, dur=0.15, fade=0.08)],
    ], "hit"),
    # A round turned by armour: a hard metal hit with a short ring, a little body under it
    "hit_armor": ("PLAYER", 0.42, [
        [L("metal_hit_02", db=0, dur=0.42, fade=0.2), L("punch_2", db=-12, lp=600, dur=0.2, fade=0.1)],
        [L("metal_hit_04", db=0, dur=0.42, fade=0.2), L("clink_1", db=-8, hp=1500, fade=0.1), L("punch_3", db=-13, lp=600, dur=0.2, fade=0.1)],
        [L("metal_hit_06", db=0, pitch=0.92, dur=0.42, fade=0.2), L("punch_2", db=-12, lp=600, dur=0.2, fade=0.1)],
    ], "hit"),
    # The player getting hit: a punch close to the mic with a dull low thud
    "player_hit": ("PLAYER", 0.4, [
        [L("punch_3", db=0, dur=0.38, fade=0.15), L("hit_02", db=-6, lp=500, dur=0.35, fade=0.15)],
        [L("punch_2", db=0, dur=0.38, fade=0.15), L("hit_07", db=-7, lp=500, dur=0.35, fade=0.15)],
        [L("punch_1", db=0, pitch=0.9, dur=0.38, fade=0.15), L("hit_02", db=-6, lp=500, pitch=1.1, dur=0.35, fade=0.15)],
    ], "hit"),
    # ---- bullets meeting the world (ImpactAudio.h: by surface) --------------------------------
    "impact": ("WORLD", 0.45, [          # stone / rock: a sharp chip, then grit trickling off
        [L("rock_hit_01", db=0), L("rock_fall_04", at=0.03, db=-8, fade=0.15), L("punch_3", db=-16, lp=400, dur=0.12, fade=0.06)],
        [L("rock_hit_01", db=0, pitch=0.88), L("rock_fall_07", at=0.03, db=-8, fade=0.15), L("punch_5", db=-16, lp=400, dur=0.12, fade=0.06)],
        [L("rock_hit_01", db=0, pitch=1.12), L("rock_fall_08", at=0.02, db=-8, fade=0.15), L("punch_1", db=-16, lp=400, dur=0.12, fade=0.06)],
    ], "hit"),
    "impact_metal": ("WORLD", 0.45, [    # hull / steel: a hard ping and a short ring
        [L("metal_hit_03", db=0, fade=0.15), L("metal_fall_03", at=0.02, db=-12, fade=0.1)],
        [L("metal_hit_06", db=0, fade=0.15), L("clink_2", at=0.0, db=-9, fade=0.1)],
        [L("metal_hit_04", db=0, pitch=1.1, fade=0.15), L("metal_fall_03", at=0.02, db=-12, pitch=0.9, fade=0.1)],
    ], "hit"),
    "impact_wood": ("WORLD", 0.4, [      # wood: a knock and splinters
        [L("wood_hit_01", db=0, fade=0.1), L("wood_break_01", at=0.01, db=-9, fade=0.1)],
        [L("wood_hit_03", db=0, fade=0.1), L("wood_break_03", at=0.01, db=-11, dur=0.3, fade=0.1)],
        [L("wood_hit_02", db=0, pitch=1.15, fade=0.1), L("wood_break_01", at=0.01, db=-9, pitch=0.9, fade=0.1)],
    ], "hit"),
    "impact_ice": ("WORLD", 0.45, [      # ice: a glassy crack and shards
        [L("glass_hit_01", db=0, pitch=0.85, fade=0.12), L("glass_fall_03", at=0.03, db=-12, dur=0.35, fade=0.15)],
        [L("glass_hit_02", db=0, pitch=0.8, fade=0.12), L("glass_fall_03", at=0.03, start=0.15, db=-12, dur=0.35, fade=0.15)],
        [L("glass_hit_01", db=0, pitch=0.72, fade=0.12), L("rock_fall_04", at=0.02, db=-12, hp=2000, fade=0.15)],
    ], "hit"),
    # ---- an enemy coming apart (Gameplay_Combat onKill, beside its death cry and debris) -----
    # A hard metal break, then its pieces clattering down. Pitched by size at runtime
    "enemy_break": ("WORLD", 1.1, [
        [L("metal_hit_05", db=0, dur=0.3, fade=0.15), L("hit_06", db=-6, lp=900, dur=0.25, fade=0.1), L("metal_fall_01", at=0.08, db=-6, fade=0.4)],
        [L("metal_hit_02", db=-1, dur=0.3, fade=0.15), L("punch_4", db=-7, lp=700, dur=0.25, fade=0.1), L("falling_04", at=0.06, db=-7, dur=1.0, fade=0.4)],
        [L("metal_hit_04", db=0, pitch=0.85, dur=0.3, fade=0.15), L("hit_06", db=-6, lp=900, dur=0.25, fade=0.1), L("metal_fall_02", at=0.07, db=-6, fade=0.3),
         L("falling_05", at=0.2, db=-12, fade=0.3)],
    ], "hit"),
    # ---- the player's body --------------------------------------------------------------------
    "land": ("PLAYER", 0.3, [            # boots onto the ground from a jump
        [L("door_close_03", db=0, lp=1800, dur=0.28, fade=0.12), L("rock_fall_04", db=-18, hp=1500, fade=0.1)],
        [L("door_close_04", db=0, lp=1800, dur=0.28, fade=0.12), L("rock_fall_08", db=-18, hp=1500, fade=0.1)],
        [L("thud_3", db=0, lp=1600, dur=0.28, fade=0.12), L("rock_fall_07", db=-18, hp=1500, fade=0.1)],
    ], "hit"),
    "land_heavy": ("PLAYER", 0.7, [      # a long fall: weight, then grit thrown up
        [L("slam_02", db=0, lp=2500, fade=0.25), L("punch_2", db=-6, lp=300, fade=0.15), L("rock_fall_01", at=0.04, db=-14, hp=800, dur=0.5, fade=0.25)],
        [L("slam_06", db=0, lp=2500, fade=0.25), L("punch_3", db=-6, lp=300, fade=0.15), L("rock_fall_03", at=0.04, db=-14, hp=800, dur=0.5, fade=0.25)],
        [L("slam_03", db=0, lp=2500, fade=0.25), L("punch_2", db=-6, lp=300, pitch=0.9, fade=0.15), L("rock_fall_08", at=0.04, db=-12, hp=800, fade=0.2)],
    ], "boom"),
}

HOLD = {"boom": 0.12, "hit": 0.3, "light": 0.3}   # share of the length kept before the decay starts

DYN = {
    # heavy glue for big events: slow-ish attack keeps the crack, the body comes up
    "boom": "acompressor=threshold=-20dB:ratio=4:attack=8:release=180:makeup=2,alimiter=limit=0.85:attack=2:release=60:level=false",
    "hit": "acompressor=threshold=-16dB:ratio=3:attack=3:release=60:makeup=1.5,alimiter=limit=0.85:attack=1:release=30:level=false",
    "light": "acompressor=threshold=-14dB:ratio=2:attack=2:release=50,alimiter=limit=0.85:attack=1:release=30:level=false",
}


def ffmpeg_f32(args_in, af):
    cmd = ["ffmpeg", "-nostdin", "-v", "error"] + args_in + ["-af", af, "-ac", "1", "-ar", str(SR), "-f", "f32le", "-"]
    raw = subprocess.run(cmd, stdin=subprocess.DEVNULL, capture_output=True, check=True).stdout
    n = len(raw) // 4
    return list(struct.unpack("<%df" % n, raw))


def write_wav(path, x):
    with wave.open(path, "wb") as w:
        w.setnchannels(1); w.setsampwidth(2); w.setframerate(SR)
        w.writeframes(b"".join(struct.pack("<h", int(round(max(-1.0, min(1.0, v)) * 32767))) for v in x))


def short_term_db(x):
    """MixTable.h shortTermDb: the loudest 50 ms (windows a quarter apart), RMS, dBFS."""
    win = max(1, int(SR * 0.05)); hop = max(1, win // 4); best = 0.0
    sq = [v * v for v in x]
    pref = [0.0]
    for v in sq: pref.append(pref[-1] + v)
    s = 0
    while True:
        e = min(len(x), s + win)
        best = max(best, (pref[e] - pref[s]) / (e - s))
        if e >= len(x): break
        s += hop
    return 10 * math.log10(best) if best > 1e-12 else -120.0


def layer(l):
    path = os.path.join(SRC, l["src"] + ".wav")
    chain = []
    if l["start"] > 0 or l["dur"]:
        chain.append(f"atrim=start={l['start']}" + (f":duration={l['dur'] / l['pitch'] * l['pitch']}" if l["dur"] else ""))
        chain.append("asetpts=PTS-STARTPTS")
    if abs(l["pitch"] - 1.0) > 1e-6:   # tape-style: pitch and speed together, like the game's playback rate
        chain.append(f"asetrate={SR * l['pitch']:.3f},aresample={SR}")
    if l["hp"]: chain.append(f"highpass=f={l['hp']}:poles=2")
    if l["lp"]: chain.append(f"lowpass=f={l['lp']}:poles=2")
    chain.append(f"volume={l['db']}dB")
    x = ffmpeg_f32(["-i", path], ",".join(chain))
    if l["fade"]:
        n = min(len(x), int(l["fade"] * SR))
        for i in range(n):   # equal-power-ish cosine fade on the layer's own tail
            x[len(x) - n + i] *= 0.5 + 0.5 * math.cos(math.pi * i / n)
    return x


def render(cls, length, layers, dyn):
    n = int(length * SR)
    mix = [0.0] * n
    for l in layers:
        x = layer(l)
        o = int(l["at"] * SR)
        for i, v in enumerate(x):
            if o + i >= n: break
            mix[o + i] += v
    # dynamics (ffmpeg, on the sum), at a sane input level
    pk = max(1e-9, max(abs(v) for v in mix))
    mix = [v * 0.7 / pk for v in mix]
    with tempfile.NamedTemporaryFile(suffix=".f32", delete=False) as t:
        t.write(struct.pack("<%df" % n, *mix)); tmp = t.name
    try:
        y = ffmpeg_f32(["-f", "f32le", "-ar", str(SR), "-ac", "1", "-i", tmp], "highpass=f=32:poles=2," + DYN[dyn])
    finally:
        os.unlink(tmp)
    y = y[:n]
    # A natural decay: after HOLD (the attack and body), the level falls in a straight line in dB
    # to -45 dB at the end. Compression lifts tails; without this a blast held its level and was cut off
    hold = HOLD.get(dyn, 0.35) * len(y)
    for i in range(int(hold), len(y)):
        y[i] *= 10 ** (-45 / 20 * (i - hold) / (len(y) - hold))
    # start on the transient: drop any lead-in under -40 dB of the peak (a combat cue must start within 5 ms)
    pk = max(1e-9, max(abs(v) for v in y))
    k = next(i for i, v in enumerate(y) if abs(v) >= pk * 0.01)
    y = y[max(0, k - int(0.001 * SR)):]
    # click-free end: 30 ms cosine to exact zero
    m = min(len(y), int(0.03 * SR))
    for i in range(m):
        y[len(y) - m + i] *= 0.5 + 0.5 * math.cos(math.pi * i / m)
    y[-1] = 0.0
    return y


def level_family(ys, cls):
    """One level for every variant: the class target, or lower if a variant's peak would pass
    -1 dBFS there (MixTable's trim then brings the family up, and all variants share it)."""
    target = TARGET[cls]
    room = []
    for y in ys:
        st = short_term_db(y); pk = max(abs(v) for v in y)
        room.append(min(target, st + 20 * math.log10(0.89 / pk)))
    lvl = min(room)
    out = []
    for y in ys:
        g = 10 ** ((lvl - short_term_db(y)) / 20)
        out.append([v * g for v in y])
    return out


def build(names):
    trims = {}
    for name in names:
        cls, length, variants, dyn = RECIPES[name]
        trims[name] = []
        ys = level_family([render(cls, length, layers, dyn) for layers in variants], cls)
        for k, y in enumerate(ys):
            out = os.path.join(OUT, name + (f"_{k}" if k else "") + ".wav")
            write_wav(out, y)
            # measure what was actually written (16-bit)
            with wave.open(out) as w:
                q = [v / 32768.0 for v in struct.unpack("<%dh" % w.getnframes(), w.readframes(w.getnframes()))]
            st = short_term_db(q)
            trims[name].append(TARGET[cls] - st)
            print(f"  {os.path.basename(out):22s} {len(q) / SR:5.2f} s  peak {20 * math.log10(max(abs(v) for v in q)):6.1f} dBFS"
                  f"  st50 {st:6.1f}  ({cls} {TARGET[cls]:.0f}: trim {TARGET[cls] - st:+.2f})")
    print("\nMixTable trims (all variants of a family share one trim; the build keeps them within 0.25 dB):")
    for name, t in trims.items():
        spread = max(t) - min(t)
        print(f"  {name:14s} {sum(t) / len(t):+.2f} dB   (spread {spread:.2f})")


def freeze(dl):
    os.makedirs(SRC, exist_ok=True)
    for name, rel in FREEZE.items():
        x = ffmpeg_f32(["-i", os.path.join(dl, rel)], "anull")
        pk = max(1e-9, max(abs(v) for v in x))
        x = [v * 0.89 / pk for v in x]
        write_wav(os.path.join(SRC, name + ".wav"), x)
        print(f"froze {name:20s} <- {rel} ({len(x) / SR:.2f} s)")


if __name__ == "__main__":
    a = sys.argv[1:]
    if a and a[0] == "--freeze":
        freeze(a[1]); sys.exit(0)
    build(a or list(RECIPES))
