#!/usr/bin/env python3
"""
Builds the enemy voices from REAL recordings (CC0), replacing VoiceSynth.h's
oscillator voices kind by kind. The game loads assets/sfx/voices/<voice>.wav
(and _1.._8) when it exists, and builds the synthesized voice otherwise
(main.cpp), so a kind can move to recordings without the rest going silent.

Each enemy keeps its designed identity from VoiceSynth::profileOf: the voice's
register (f0), how many voices (a choir), its machine body and its size. A voice
is a real creature vocal pitched into that register, put through a short metal
comb (the hollow machine voice; a chorus for the massed ones) over a recorded
mechanical layer for its body. The cold machines (f0 0) have no vocal: only
machinery. Wordless by design: no speech recordings.

    make voices                             # dump the bank, then build every voice below
    python3 tools/build_real_voices.py BANK # BANK: the output of build/voice_bank_dump
    python3 tools/build_real_voices.py BANK --freeze SFXDL VOICEDL   # (re)freeze sources

Built kinds: death, hurt, idle, spawn, move, tell (as long as its wind-up, keeping
the synthesized tells' gestures) and attack (the releases). The shared specials
are still synthesized.
"""
import hashlib
import math
import os
import struct
import sys
import wave

VOICE_RATE = 22050   # like VoiceSynth::RATE: voices carry nothing above 11 kHz worth the space

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from build_real_sfx import SR, ROOT, ffmpeg_f32, write_wav  # noqa: E402

SRC = os.path.join(ROOT, "assets", "sfx", "src", "cc0", "voice")
OUT = os.path.join(ROOT, "assets", "sfx", "voices")
SFXSRC = os.path.join(ROOT, "assets", "sfx", "src")   # the gun foley recordings (mechanisms)
TARGET = {"GUN": -11.0, "FOLEY": -18.0, "PLAYER": -12.0, "ACTION": -10.0, "WORLD": -13.0, "UI": -12.0, "TELL": -14.0, "CHATTER": -22.0}
BUILT = {"death", "hurt", "idle", "spawn", "move", "tell", "attack"}

C1, C2 = "x_80-CC0-creature-SFX_0/", "x_80-CC0-creature-sfx-2/"
SN, MS, V2 = "x_monster_-_starninjas/", "x_monster_sfx_pack/monster_sfx_pack/", "x_monster-sounds-volume-2/Monster-Sounds-Volume-2/"
BE = "x_beasts/Monsters or Beasts/"
SF, OG = "x_sci-fi-sfx/", "x_100-CC0-SFX_0/"

# frozen name -> ("v" voice pack folder | "s" sfx pack folder, path, start, duration or None)
FREEZE = {}
def _f(name, root, path, start=0.0, dur=None): FREEZE[name] = (root, path, start, dur)
for n in ["roar_01", "roar_02", "roar_03", "monster_03", "monster_04", "monster_06", "monster_07", "grunt_01", "grunt_02", "grunt_03",
          "grunt_04", "grunt_05", "howl", "hurt_01", "hurt_02", "hurt_03", "hurt_04", "hurt_05", "misc_02", "misc_04", "misc_08", "misc_09",
          "weird_01", "weird_02", "weird_05", "breath", "scream_01", "scream_02", "bug_01", "bug_02", "bug_03", "bug_04",
          "alien_01", "alien_02", "alien_03", "alien_04", "alien_05"]:
    _f("c_" + n, "v", C1 + n + ".ogg")
for n in ["roar_04", "roar_05", "roar_06", "monster_08", "monster_09", "monster_10", "monster_11", "monster_12", "monster_13", "monster_15",
          "monster_16", "monster_17", "monster_18", "monster_19", "monster_20", "grunt_06", "grunt_07", "grunt_08", "grunt_09", "die_01",
          "die_02", "die_03", "die_04", "weird_06", "weird_07", "weird_08", "misc_10", "misc_11", "misc_14", "misc_16", "misc_17", "misc_18",
          "misc_19", "misc_20", "misc_21", "breath_02", "snore_02", "bug_05", "bug_06", "bug_08", "bug_10", "bug_12", "alien_08", "alien_12"]:
    _f("c_" + n, "v", C2 + n + ".ogg")
for i in range(1, 17): _f(f"sn_{i:02d}", "v", SN + f"monster.{i}.ogg")
for i in range(1, 11): _f(f"ms_{i:02d}", "v", MS + f"monster-{i}.wav")
for f, n in [("Monster-1", 1), ("Monster-2", 2), ("Monster-3", 3), ("Monster-4", 4), ("Monster-5", 5), ("Monster-6", 6), ("Monster-7", 7),
             ("Monster-8", 8), ("monster-9", 9), ("monster-10", 10), ("monster-13", 13), ("monster-14", 14), ("monster-15", 15), ("monster-16", 16)]:
    _f(f"v2_{n:02d}", "v", V2 + f + ".wav")
for i, f in enumerate(["Beast Growl", "Beast Growl 1", "Beast Growl 2", "Beast Growl 3", "Beast Growl 4", "Beast Growl 5"]):
    _f(f"beast_growl_{i}", "v", BE + f + ".wav")
_f("beast_dying", "v", BE + "Dieing Beast.wav")
_f("demon_growl", "v", BE + "Demon Growl.wav")
_f("big_roar", "v", "monster_roar.wav", 0.0, 3.6)
_f("alien_hiss", "v", "alien_hiss.mp3")
_f("power_off", "v", "MachinePowerOff.ogg")
for k, (a, d) in enumerate([(0.0, 2.6), (5.0, 2.6), (10.0, 2.6), (15.0, 2.6)]): _f(f"troll_idle_{k}", "v", "troll-idle-noises.ogg", a, d)
# long snarls and roars (segments between the pauses), for the sustained wind-ups
for k, (a, b) in enumerate([(0, 1.22), (1.6, 2.91), (3.28, 4.2), (5.17, 6.05), (6.72, 7.84), (8.78, 9.83)]):
    _f(f"snarl_atk_{k}", "v", "monster-snarl-attack.ogg", a, b - a)
for k, (a, b) in enumerate([(0, 3.53), (4.96, 8.76), (9.7, 12.98)]): _f(f"snarl_long_{k}", "v", "monster-snarls.ogg", a, b - a)
for k, (a, b) in enumerate([(0, 1.17), (1.86, 2.96), (3.6, 4.77), (5.35, 6.41), (7.07, 8.73)]): _f(f"troll_roar_{k}", "v", "troll-roars.ogg", a, b - a)
for k, (a, b) in enumerate([(0.33, 1.66), (2.02, 3.49), (4.01, 5.67), (6.3, 8.09)]): _f(f"snarl5_{k}", "v", "monster-snarl-5.ogg", a, b - a)
_f("m_shoot_01", "s", SF + "shoot_01.ogg"); _f("m_loop_machine_03", "s", SF + "loop_machine_03.ogg", 0.0, 2.0)
for n in ["shot_01", "shot_02", "shot_03"]: _f("b_" + n, "s", "x_25-CC0-bang-sfx/" + n + ".ogg")

# machinery
for n in ["loop_machine_01", "loop_machine_02", "loop_ambient_01", "misc_03", "misc_04", "misc_06", "misc_12", "teleport_01", "teleport_02"]:
    _f("m_" + n, "s", SF + n + ".ogg", 0.0, 2.0 if n.startswith("loop") else None)
for n in ["tools_01", "tools_02", "tools_03", "tools_05", "machine_01", "machine_03"]:
    _f("m_" + n, "s", OG + n + ".ogg")

# ---- pools: vocal material by register and kind --------------------------------------------------
POOL = {
    ("deep", "death"): ["c_roar_05", "c_roar_06", "c_monster_20", "big_roar", "demon_growl", "ms_08", "c_roar_02"],
    ("deep", "hurt"):  ["c_grunt_06", "c_grunt_07", "c_grunt_08", "c_grunt_09", "c_monster_09", "c_monster_12", "ms_02", "ms_03"],
    ("deep", "idle"):  ["troll_idle_0", "troll_idle_1", "troll_idle_2", "troll_idle_3", "c_snore_02", "c_monster_07", "c_weird_05", "c_misc_10"],
    ("deep", "spawn"): ["big_roar", "c_roar_02", "c_roar_05", "c_monster_04", "demon_growl", "ms_10"],
    ("mid", "death"):  ["c_die_03", "c_monster_06", "c_weird_06", "beast_dying", "ms_06", "beast_growl_2", "sn_08"],
    ("mid", "hurt"):   ["c_hurt_03", "c_hurt_04", "c_hurt_05", "c_grunt_03", "c_grunt_04", "c_grunt_05", "c_misc_02", "c_misc_04", "c_monster_08", "c_monster_15"],
    ("mid", "idle"):   ["c_breath", "c_weird_01", "c_weird_02", "c_weird_07", "c_weird_08", "c_misc_11", "sn_01", "sn_03", "beast_growl_0", "beast_growl_4"],
    ("mid", "spawn"):  ["c_howl", "c_roar_04", "demon_growl", "sn_07", "sn_10", "sn_13", "beast_growl_1", "beast_growl_3", "beast_growl_5", "ms_04"],
    ("high", "death"): ["c_die_01", "c_die_02", "c_die_04", "c_scream_01", "c_scream_02", "c_monster_19", "v2_02"],
    ("high", "hurt"):  ["c_hurt_01", "c_hurt_02", "v2_04", "v2_08", "v2_09", "v2_10", "c_misc_17", "c_misc_18"],
    ("high", "idle"):  ["c_misc_18", "c_misc_19", "c_misc_20", "c_misc_21", "c_monster_10", "c_monster_11", "c_misc_08", "v2_13", "v2_15"],
    ("high", "spawn"): ["c_roar_03", "c_monster_17", "c_monster_19", "c_monster_13", "c_monster_16", "v2_01", "v2_03", "v2_06", "sn_11"],
    ("tiny", "death"): ["c_die_04", "c_alien_02", "c_alien_03", "c_bug_01", "c_alien_08", "c_misc_14"],
    ("tiny", "hurt"):  ["c_bug_02", "c_bug_04", "c_bug_05", "c_bug_08", "c_alien_05", "c_bug_10"],
    ("tiny", "idle"):  ["c_bug_01", "c_bug_03", "c_bug_06", "c_bug_12", "c_alien_01", "c_alien_04", "c_alien_12", "c_misc_16"],
    ("tiny", "spawn"): ["c_alien_02", "c_alien_03", "c_alien_08", "c_misc_14", "c_misc_16"],
}
NOMINAL = {"deep": 60.0, "mid": 140.0, "high": 300.0, "tiny": 600.0}   # roughly where each pool sits

# machinery by body: (pool, base pitch). "../" names are the frozen sfx recordings (src/cc0 or src)
BODY = {
    "SERVO":     (["m_loop_machine_01", "m_machine_01", "m_misc_06", "#cyl_close_rec"], 1.25),
    "SKITTER":   (["m_tools_01", "m_tools_03", "m_tools_05", "@clink_1", "@clink_2"], 1.2),
    "HYDRAULIC": (["power_off", "#pump_rec", "@door_close_04", "m_misc_04"], 1.0),
    "WINGS":     (["alien_hiss", "c_breath_02", "m_misc_12"], 1.0),
    "HEAVY":     (["@metal_08", "@metal_09", "m_loop_machine_02", "m_machine_03"], 0.75),
    "TREADS":    (["m_loop_machine_02", "m_machine_03", "@metal_08"], 0.7),
    "SCRAPE":    (["@metal_fall_01", "@metal_fall_02", "@metal_hit_05"], 0.85),
    "CRACKLE":   (["m_misc_12", "m_teleport_01", "m_misc_06"], 1.0),
    "HUM":       (["m_loop_ambient_01", "m_loop_machine_01", "m_teleport_02"], 1.0),
    "GRIND":     (["m_loop_machine_02", "m_misc_03", "@metal_09"], 0.65),
    "GEARS":     (["m_tools_02", "#bolt_rec", "m_tools_05"], 0.9),
    "ARMOUR":    (["@metal_hit_02", "@metal_hit_04", "@metal_hit_06"], 0.9),
    "CHAINS":    (["@metal_fall_01", "@clink_1", "@metal_fall_02"], 0.8),
}


def src_path(name):
    if name.startswith("@"): return os.path.join(ROOT, "assets", "sfx", "src", "cc0", name[1:] + ".wav")
    if name.startswith("#"): return os.path.join(SFXSRC, name[1:] + ".wav")
    return os.path.join(SRC, name + ".wav")


def h(*parts):
    return int(hashlib.sha1("|".join(map(str, parts)).encode()).hexdigest()[:8], 16)


def register(f0):
    return "deep" if f0 < 85 else "mid" if f0 < 220 else "high" if f0 < 450 else "tiny"


def clip(name, pitch=1.0, start=0.0, dur=None, hp=None, lp=None, comb=None, chorus=0, reverse=False, fade_in=0.0):
    """One processed layer as floats. comb: (delay ms, decay) the hollow machine voice; chorus: voices"""
    chain = []
    if reverse: chain.append("areverse")
    if start > 0: chain += [f"atrim=start={start}", "asetpts=PTS-STARTPTS"]
    if abs(pitch - 1.0) > 1e-4: chain.append(f"asetrate={SR * pitch:.2f},aresample={SR}")
    if dur: chain += [f"atrim=duration={dur}", "asetpts=PTS-STARTPTS"]
    if hp: chain.append(f"highpass=f={hp}:poles=2")
    if lp: chain.append(f"lowpass=f={lp}:poles=2")
    if comb:
        d, k = comb
        chain.append(f"aecho=0.85:0.75:{d:.2f}|{d * 1.73:.2f}:{k:.2f}|{k * 0.7:.2f}")
    if chorus >= 4:
        chain.append("chorus=0.7:0.9:42|57|71:0.35|0.3|0.25:0.3|0.4|0.27:1.6|2.1|1.3")
    elif chorus >= 2:
        chain.append("chorus=0.8:0.9:45|63:0.3|0.25:0.3|0.4:1.8|2.4")
    if not chain: chain.append("anull")
    x = ffmpeg_f32(["-i", src_path(name)], ",".join(chain))
    if fade_in > 0:
        n = min(len(x), int(fade_in * SR))
        for i in range(n): x[i] *= i / n
    return x


def fade_tail(x, sec):
    n = min(len(x), int(sec * SR))
    for i in range(n): x[len(x) - n + i] *= 0.5 + 0.5 * math.cos(math.pi * i / n)
    return x


def gain(x, db):
    g = 10 ** (db / 20); return [v * g for v in x]


def mix_into(out, x, at=0.0):
    o = int(at * SR)
    for i, v in enumerate(x):
        if o + i >= len(out): break
        out[o + i] += v


def build_voice(v, k):
    """v: one line of the bank; k: variant. Returns floats at SR, or None (not built here)"""
    kind, typ, f0, voices, body, size = v["kind"], v["type"], v["f0"], v["voices"], v["body"], v["size"]
    if kind not in BUILT or typ == "any": return None
    reg = register(f0) if f0 > 0 else None
    seed = h(typ, kind, k)
    jitter = 1.0 + 0.04 * (((seed >> 8) % 1000) / 500.0 - 1.0)
    vpitch = 1.0
    if reg:
        vpitch = max(0.62, min(1.45, (f0 / NOMINAL[reg]) ** 0.5 * (1.12 - 0.3 * size))) * jitter
    comb = (2.5 + 5.0 * ((h(typ) % 100) / 100.0), 0.22 + 0.1 * size)   # each type its own hollow
    bpool, bpitch = BODY[body]
    bpitch *= (1.15 - 0.35 * size) * jitter
    pick = lambda pool, salt=0: pool[(h(typ, kind, salt) + k) % len(pool)]

    if kind == "tell":
        return build_tell(v, k, reg, vpitch, comb, bpool, bpitch, voices, body, size)
    if kind == "attack":
        return build_release(v, k, reg, vpitch, comb, bpool, bpitch, voices, body, size)
    length = {"death": min(2.0, 0.8 + 0.9 * size), "hurt": 0.32 + 0.25 * size, "idle": 0.7 + 0.8 * size,
              "spawn": min(2.2, 1.0 + 1.0 * size), "move": 0.22 + 0.3 * size}[kind]
    out = [0.0] * int(length * SR)

    def vocal(at=0.0, db=0.0, dur=None, fade=0.2, fade_in=0.0, k2=0):
        name = pick(POOL[(reg or "mid", kind if kind != "move" else "idle")], k2)
        x = clip(name, pitch=vpitch, dur=dur or length, hp=70, comb=comb, chorus=voices if voices >= 2 else 0, fade_in=fade_in)
        mix_into(out, gain(fade_tail(x, min(fade, len(x) / SR * 0.5)), db), at)

    def mech(at=0.0, db=-8.0, dur=None, pitch=None, reverse=False, salt=1, fade=0.15, name=None):
        name = name or pick(bpool, salt)
        x = clip(name, pitch=pitch or bpitch, dur=dur or length, hp=50, reverse=reverse)
        mix_into(out, gain(fade_tail(x, min(fade, len(x) / SR * 0.5)), db), at)

    if kind == "death":
        if reg: vocal(db=0, fade=0.35)
        # the machine giving out: a power-down for the powered bodies, a collapse of metal for the rest
        if body in ("SERVO", "HYDRAULIC", "HUM", "CRACKLE") or not reg:
            mech(at=0.05, db=-6 if reg else 0, name="power_off", pitch=bpitch * 0.95, fade=0.3)
        mech(at=0.1, db=-10 if reg else -4, name=pick(["@metal_fall_01", "@metal_fall_02", "@metal_hit_05"], 3), pitch=bpitch, fade=0.3)
    elif kind == "hurt":
        if reg: vocal(db=0, fade=0.12)
        mech(db=-13 if reg else 0, name=pick(["@metal_hit_02", "@metal_hit_04", "@metal_hit_06", "@clink_1"], 4), pitch=bpitch * 1.1,
             dur=0.25, fade=0.1)
        if not reg: mech(at=0.02, db=-6, dur=length, fade=0.12)
    elif kind == "idle":
        if reg: vocal(db=0, fade=0.25, fade_in=0.03)
        mech(db=-11 if reg else 0, fade=0.25)
    elif kind == "spawn":
        # powering up (a power-down played backwards), then it announces itself
        mech(db=-5 if reg else 0, name="power_off", pitch=bpitch, reverse=True, dur=min(length, 1.2), fade=0.1)
        if reg: vocal(at=min(0.45, length * 0.3), db=0, dur=length - min(0.45, length * 0.3), fade=0.3)
        else: mech(at=0.6, db=-3, salt=5, fade=0.3)
    elif kind == "move":
        mech(db=0, dur=length, fade=0.08)
        if body == "SKITTER" and reg in ("high", "tiny"): vocal(db=-9, dur=length, fade=0.08)
    # leading silence off (a cue starts at once), then a click-free end
    pk = max(1e-9, max(abs(s) for s in out))
    first = next(i for i, s in enumerate(out) if abs(s) >= pk * 0.02)
    out = out[max(0, first - int(0.002 * SR)):]
    # A natural decay: after the hold the level falls in a straight line in dB to -42 dB at the end
    # (layers cut to length otherwise stop at full level)
    hold = {"death": 0.3, "hurt": 0.3, "idle": 0.45, "spawn": 0.55, "move": 0.35}[kind] * len(out)
    for i in range(int(hold), len(out)):
        out[i] *= 10 ** (-42 / 20 * (i - hold) / (len(out) - hold))
    out = fade_tail(out, 0.02); out[-1] = 0.0
    return out


# ---- the wind-ups (tells) and their releases ---------------------------------------------------------
# A tell is a gesture the player learns, as long as the wind-up it announces (tellDur). The synthesized
# tells' gestures are kept, built from recordings: RISE (a growl and a mechanism gliding up to a metal
# catch at the release), HISS (breath and a snarl into a lunge), FALL (a shriek gliding down), RATCHET
# (recorded clicks, faster and faster), INHALE (a roar reversed, then grinding gears), METAL (a ring
# swelling backwards into a snap), SING (a massed voice rising), SIREN (a machine whine climbing)
GESTURE = {}
for a in ["lunge", "release", "claw", "incision", "ram", "wlunge", "leap", "dash", "charge", "gantry", "rollback"]: GESTURE[a] = "HISS"
for a in ["dive", "submerge", "teleport", "adash", "blink"]: GESTURE[a] = "FALL"
for a in ["smash", "press_row", "suture", "recall", "fuse", "grab", "snapshot", "winch"]: GESTURE[a] = "RATCHET"
for a in ["slam", "pslam", "stomp", "breaker", "cascade", "road_tear", "ballast", "crash", "tide", "breach", "swallow",
          "stamp", "press", "rupture", "grade", "reject", "detonate", "weight", "spit"]: GESTURE[a] = "INHALE"
for a in ["incision_thrust", "transpose", "sever", "bell", "bash", "hook", "cleave", "crescent", "whirl", "blade_sweep",
          "tongs", "lance", "thrust", "lattice", "reap", "crossing", "shelving"]: GESTURE[a] = "METAL"
for a in ["sweep", "judgment", "phantoms", "summon", "beam", "volley", "perfusion", "observe", "censer_low", "censer_high",
          "lash", "scourge", "torrent", "vector", "flash_freeze", "seeker", "vent", "crosswind", "purge"]: GESTURE[a] = "SING"
for a in ["scan", "shell", "burst"]: GESTURE[a] = "SIREN"
CLICKS = ["#cyl_close_rec", "@clink_1", "@clink_2", "#bolt_rec"]
LONG_VOCAL = {"deep": ["snarl_long_0", "snarl_long_1", "troll_roar_4", "big_roar", "snarl_long_2"],
              "mid": ["snarl_long_0", "snarl_long_1", "snarl_long_2", "snarl5_2", "snarl5_3", "sn_11", "sn_08"],
              "high": ["snarl5_0", "snarl5_1", "c_monster_19", "c_scream_01", "c_monster_17", "v2_02"],
              "tiny": ["c_alien_03", "c_alien_08", "c_misc_14", "c_bug_01", "c_alien_02"]}


def glide(x, r0, r1, n, loop=False):
    """x played at a rate gliding from r0 to r1 (tape-style: pitch and speed), n samples out"""
    out = [0.0] * n
    pos, m = 0.0, len(x) - 1
    if m < 2: return out
    for i in range(n):
        k = int(pos)
        if k >= m:
            if not loop: break
            pos -= m; k = int(pos)
        f = pos - k
        out[i] = x[k] + (x[k + 1] - x[k]) * f
        pos += r0 + (r1 - r0) * i / n
    return out


def env_ramp(x, lo=0.25, start=0.0):
    """crescendo: from `lo` to full across the sound (a wind-up gets nearer)"""
    n = len(x)
    for i in range(n):
        t = i / n
        x[i] *= lo + (1 - lo) * (t ** 1.3) if t >= start else lo
    return x


def ident_vocal(name, comb, voices, **kw):
    return clip(name, hp=70, comb=comb, chorus=voices if voices >= 2 else 0, **kw)


def build_tell(v, k, reg, vp, comb, bpool, bp, voices, body, size):
    d = max(0.2, v["dur"]); a = v["attack"]; typ = v["type"]
    g = GESTURE.get(a, "RISE")
    n = int((d + 0.12) * SR); end = int(d * SR)
    out = [0.0] * n
    pick = lambda pool, salt=0: pool[(h(typ, a, salt) + k) % len(pool)]
    lv = LONG_VOCAL[reg] if reg else None
    def catch(db=-3.0, pitch=1.0, at=None):   # the release moment, on the beat of the wind-up's end
        x = clip(pick(CLICKS, 9), pitch=pitch * bp, dur=0.12)
        mix_into(out, gain(fade_tail(x, 0.05), db), (at if at is not None else d - 0.03))
    def mech_glide(r0, r1, db, name=None, reverse=False):
        x = clip(name or pick(bpool, 3), hp=50, reverse=reverse)
        mix_into(out, gain(env_ramp(glide(x, r0 * bp, r1 * bp, end, loop=True), 0.35), db))
    def vocal_glide(r0, r1, db, pool=None, lo=0.25, reverse=False, until=None):
        x = ident_vocal(pick(pool or lv, 1), comb, voices, reverse=reverse)
        m = int((until or d) * SR)
        y = env_ramp(glide(x, r0 * vp, r1 * vp, m), lo)
        mix_into(out, gain(fade_tail(y, 0.04), db))
    if g == "RISE":
        if reg: vocal_glide(0.85, 1.22, 0)
        mech_glide(0.7, 1.5, -7 if reg else 0)
        catch()
    elif g == "HISS":
        hx = clip(pick(["alien_hiss", "c_breath", "c_breath_02"], 2), hp=300)
        mix_into(out, gain(env_ramp(glide(hx, 0.85, 1.25, end, loop=True), 0.3), -4 if reg else 0))
        if reg:
            sx = ident_vocal(pick(["snarl_atk_0", "snarl_atk_1", "snarl_atk_2", "snarl_atk_3", "snarl_atk_4", "snarl_atk_5"] if reg in ("deep", "mid")
                                  else POOL[(reg, "spawn")], 4), comb, voices, pitch=vp)
            sx = sx[:end]; mix_into(out, env_ramp(fade_tail(sx, 0.05), 0.4), max(0.0, d - len(sx) / SR))
        mech_glide(0.8, 1.3, -10)
        catch(-6)
    elif g == "FALL":
        if reg: vocal_glide(1.45, 0.8, 0, pool=LONG_VOCAL["high"] if reg != "tiny" else lv, lo=0.6)
        hx = clip(pick(["alien_hiss", "c_breath_02"], 2), hp=400)
        mix_into(out, gain(glide(hx, 1.2, 0.8, end, loop=True), -8 if reg else 0))
        catch(-6, pitch=0.8)
    elif g == "RATCHET":
        bright = a == "fuse"
        t, gap = 0.0, 0.11 if not bright else 0.13
        while t < d - 0.02:
            x = clip(pick(CLICKS, int(t * 100)), pitch=(1.6 if bright else 1.0) * bp, dur=0.06)
            mix_into(out, gain(fade_tail(x, 0.02), -2 + 6 * t / d - 6), t)
            t += gap; gap = max(0.028, gap * (0.75 if bright else 0.84))
        if not bright: mech_glide(0.6, 1.0, -10, name=pick(["m_loop_machine_02", "m_misc_03"], 5))
        if reg and not bright: vocal_glide(0.9, 1.1, -8)
        catch(0)
    elif g == "INHALE":
        if reg:   # a roar played backwards: an intake of breath that swells to the strike
            x = ident_vocal(pick(["troll_roar_0", "troll_roar_1", "troll_roar_2", "troll_roar_3", "c_roar_05", "c_roar_06"]
                                 if reg in ("deep", "mid") else POOL[(reg, "spawn")], 1), comb, voices, reverse=True, pitch=vp)
            x = x[-int(d * 0.75 * SR):] if len(x) > d * 0.75 * SR else x
            mix_into(out, env_ramp(fade_tail(x, 0.03), 0.3), max(0.0, d * 0.75 - len(x) / SR))
        gx = clip(pick(["m_loop_machine_02", "m_misc_03", "@metal_09"], 6), hp=40)
        gy = glide(gx, 0.55 * bp, 0.9 * bp, int(d * 0.45 * SR), loop=True)
        mix_into(out, gain(env_ramp(gy, 0.4), -4 if reg else 0), d * 0.55)
        thud = clip(pick(["@punch_2", "@punch_3", "@door_close_03"], 7), pitch=0.7, lp=400, dur=0.15)
        mix_into(out, gain(fade_tail(thud, 0.06), -2), d - 0.04)
    elif g == "METAL":
        rx = clip(pick(["@metal_hit_03", "@metal_hit_06", "@metal_hit_02", "@clink_1", "m_tools_02"], 2), pitch=bp * 1.1, reverse=True)
        mix_into(out, gain(env_ramp(glide(rx, 0.9, 1.0, end, loop=True), 0.15), -2))
        hx = clip(pick(["alien_hiss", "c_breath_02"], 3), hp=1500)
        mix_into(out, gain(env_ramp(glide(hx, 0.9, 1.3, end, loop=True), 0.2), -12))
        if reg: vocal_glide(0.95, 1.15, -8)
        snap = clip(pick(["@metal_hit_04", "@metal_hit_06", "@clink_2"], 8), pitch=bp * 1.2, dur=0.12)
        mix_into(out, fade_tail(snap, 0.05), d - 0.03)
    elif g == "SING":
        if reg: vocal_glide(0.95, 1.18, 0, lo=0.35)
        mech_glide(0.85, 1.25, -9 if reg else 0, name=pick(["m_loop_ambient_01", "m_loop_machine_01", "m_teleport_02"], 4))
        catch(-8)
    elif g == "SIREN":
        mech_glide(0.6, 1.5, 0, name="m_loop_machine_03")
        if reg: vocal_glide(0.85, 1.25, -6)
        catch(-4)
    out = fade_tail(out, 0.03); out[-1] = 0.0
    # starts at once: a short fade-in from the first sample (no pre-roll)
    for i in range(min(len(out), int(0.006 * SR))): out[i] *= i / (0.006 * SR)
    if max(abs(x) for x in out) < 1e-6: out[0] = 1e-4
    return out


def build_release(v, k, reg, vp, comb, bpool, bp, voices, body, size):
    a = v["attack"]; typ = v["type"]
    pick = lambda pool, salt=0: pool[(h(typ, a, salt) + k) % len(pool)]
    def put(out, name, at=0.0, db=0.0, pitch=1.0, dur=None, lp=None, hp=None, fade=0.06):
        x = clip(name, pitch=pitch, dur=dur, lp=lp, hp=hp)
        mix_into(out, gain(fade_tail(x, min(fade, len(x) / SR * 0.5)), db), at)
    if a == "beam":   # held while the beam sweeps (3 s), stopped when it ends: steady, no decay
        out = [0.0] * int(3.0 * SR)
        x = clip("m_loop_machine_03", pitch=1.1 * bp); mix_into(out, glide(x, 1.0, 1.0, len(out), loop=True))
        y = clip("m_loop_ambient_01", pitch=1.3); mix_into(out, gain(glide(y, 1.0, 1.0, len(out), loop=True), -6))
        if reg:
            z = ident_vocal(pick(LONG_VOCAL[reg], 1), comb, max(4, voices), pitch=vp * 1.3)
            mix_into(out, gain(glide(z, 1.0, 1.0, len(out), loop=True), -5))
        out[:int(0.08 * SR)] = [s * i / (0.08 * SR) for i, s in enumerate(out[:int(0.08 * SR)])]
        return fade_tail(out, 0.1)
    out = [0.0] * int(0.6 * SR)
    shot = lambda: pick(["m_shoot_01", "b_shot_01", "b_shot_02", "b_shot_03"], 2)
    if a == "shot":
        put(out, shot(), pitch=bp * 1.05, dur=0.4, fade=0.15); put(out, pick(CLICKS, 3), db=-10, pitch=bp, dur=0.08)
    elif a == "burst":
        for j in range(3): put(out, shot(), at=j * 0.07, pitch=bp * (1.1 + 0.04 * j), dur=0.25, fade=0.1, db=-1)
    elif a in ("lunge", "dive", "bash"):
        put(out, pick(["alien_hiss", "c_breath_02"], 4), db=-4, pitch=1.2, hp=500, dur=0.3)
        if reg:
            x = ident_vocal(pick(["snarl_atk_0", "snarl_atk_2", "snarl_atk_4"] if reg in ("deep", "mid") else POOL[(reg, "hurt")], 5),
                            comb, voices, pitch=vp * 1.05, dur=0.35)
            mix_into(out, fade_tail(x, 0.12))
        if a == "bash": put(out, pick(["@metal_hit_02", "@metal_hit_04"], 6), db=-2, pitch=bp, dur=0.3, fade=0.15)
    elif a == "lob":
        put(out, pick(["@punch_2", "@punch_3"], 4), pitch=0.8, lp=600, dur=0.2); put(out, pick(CLICKS, 3), db=-6, pitch=0.8, dur=0.08)
    elif a == "shell":
        put(out, "cannon_04" if False else "b_shot_01", pitch=0.6, lp=1500, dur=0.5, fade=0.2)
        put(out, pick(["alien_hiss", "c_breath_02"], 4), at=0.03, db=-8, pitch=0.8, dur=0.4)
    elif a == "volley":
        for j in range(3):
            put(out, pick(["@punch_2", "@punch_4", "@punch_5"], j), at=j * 0.06, pitch=0.85, lp=700, dur=0.15)
            put(out, pick(CLICKS, j + 3), at=j * 0.06, db=-8, pitch=0.9, dur=0.06)
    else:   # a heavy machine letting go: a thump, a clank, a little grind
        put(out, pick(["@punch_2", "@punch_3", "@door_close_03"], 4), pitch=0.75, lp=700, dur=0.25, fade=0.1)
        put(out, pick(["@metal_hit_02", "@metal_hit_05", "@metal_08"], 5), db=-4, pitch=bp, dur=0.35, fade=0.15)
        put(out, pick(CLICKS, 3), db=-8, pitch=bp, dur=0.08)
    pk = max(1e-9, max(abs(s) for s in out))
    last = max(i for i, s in enumerate(out) if abs(s) >= pk * 0.003)
    out = out[:last + 1]
    for i in range(min(len(out), int(0.004 * SR))): out[i] *= i / (0.004 * SR)
    return fade_tail(out, 0.02)


def level(x, cls):
    """The voice at its mix class's level (loudest 50 ms, measured at VOICE_RATE as the game will),
    peaks kept under -1 dBFS by a gentle limiter"""
    target = TARGET[cls]
    for _ in range(5):
        st = st_db(x, VOICE_RATE)
        if st <= -119: return x
        g = 10 ** ((target - st) / 20)
        y = []
        for v in x:
            v *= g; a = abs(v)
            if a > 0.7: v = (1 if v > 0 else -1) * (0.7 + 0.19 * math.tanh((a - 0.7) / 0.19))
            y.append(v)
        x = y
    return x


def resample(x, rate):
    """band-limited resample (ffmpeg) of floats at SR to `rate`"""
    import subprocess
    raw = subprocess.run(["ffmpeg", "-nostdin", "-v", "error", "-f", "f32le", "-ar", str(SR), "-ac", "1", "-i", "-",
                          "-af", "highpass=f=55:poles=2", "-ar", str(rate), "-f", "f32le", "-"], input=struct.pack("<%df" % len(x), *x), capture_output=True, check=True).stdout
    return list(struct.unpack("<%df" % (len(raw) // 4), raw))


def st_db(x, rate):
    """MixTable.h shortTermDb at `rate`"""
    win = max(1, int(rate * 0.05)); hop = max(1, win // 4); best = 0.0
    pref = [0.0]
    for v in x: pref.append(pref[-1] + v * v)
    s = 0
    while True:
        e = min(len(x), s + win); best = max(best, (pref[e] - pref[s]) / (e - s))
        if e >= len(x): break
        s += hop
    return 10 * math.log10(best) if best > 1e-12 else -120.0


def write_voice(path, x):
    """16-bit mono at VOICE_RATE"""
    with wave.open(path, "wb") as w:
        w.setnchannels(1); w.setsampwidth(2); w.setframerate(VOICE_RATE)
        w.writeframes(b"".join(struct.pack("<h", int(round(max(-1.0, min(1.0, v)) * 32767))) for v in x))


def read_bank(path):
    bank = []
    for line in open(path):
        p = line.split()
        if len(p) < 12: continue
        bank.append(dict(name=p[0], kind=p[1], type=p[2], attack=p[3], variants=int(p[4]), cls=p[5], f0=float(p[6]),
                         voices=int(p[7]), body=p[8], size=float(p[9]), boss=int(p[10]), dur=float(p[11])))
    return bank


def freeze(sfx_dl, voice_dl):
    os.makedirs(SRC, exist_ok=True)
    for name, (root, rel, start, dur) in FREEZE.items():
        base = voice_dl if root == "v" else sfx_dl
        af = []
        if start or dur: af += [f"atrim=start={start}" + (f":duration={dur}" if dur else ""), "asetpts=PTS-STARTPTS"]
        x = ffmpeg_f32(["-i", os.path.join(base, rel)], ",".join(af) if af else "anull")
        pk = max(1e-9, max(abs(v) for v in x))
        write_wav(os.path.join(SRC, name + ".wav"), [v * 0.89 / pk for v in x])
    print(f"froze {len(FREEZE)} recordings into {SRC}")


def main():
    a = sys.argv[1:]
    bank = read_bank(a[0])
    if len(a) >= 4 and a[1] == "--freeze":
        freeze(a[2], a[3]); return
    only = set(a[1:])
    os.makedirs(OUT, exist_ok=True)
    n = secs = 0
    worst = 0.0
    for v in bank:
        if only and v["kind"] not in only and v["type"] not in only: continue
        for k in range(v["variants"]):
            x = build_voice(v, k)
            if x is None: continue
            x = level(resample(x, VOICE_RATE), v["cls"])
            write_voice(os.path.join(OUT, v["name"] + (f"_{k}" if k else "") + ".wav"), x)
            off = abs(st_db(x, VOICE_RATE) - TARGET[v["cls"]]); worst = max(worst, off)
            n += 1; secs += len(x) / VOICE_RATE
    print(f"built {n} voice files, {secs:.0f} s of audio; worst level error {worst:.2f} dB")


if __name__ == "__main__":
    main()
