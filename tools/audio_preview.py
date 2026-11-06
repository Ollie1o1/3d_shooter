#!/usr/bin/env python3
"""Render audition WAVs from the shipped combat assets and the real music synth.

make audio-preview
Optional original assets in build/audio-before/ produce a level-matched A/B reel.
"""
from pathlib import Path
import math
import struct
import subprocess
import wave

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "build" / "audio-preview"
SR = 44100
CUES = ("sword_draw", "sword_swing", "sword_hit", "sword_heavy", "sword_thrust",
        "sword_pierce", "sword_guard", "sword_block", "sword_break", "parry",
        "parry_heavy", "hit_body", "hit_armor", "impact", "impact_metal", "impact_wood", "impact_ice")


def read(path):
    with wave.open(str(path)) as f:
        assert f.getframerate() == SR and f.getsampwidth() == 2 and f.getnchannels() == 1
        return [v / 32768 for v in struct.unpack("<%dh" % f.getnframes(), f.readframes(f.getnframes()))]


def write(path, samples):
    with wave.open(str(path), "wb") as f:
        f.setparams((1, 2, SR, 0, "NONE", "not compressed"))
        f.writeframes(struct.pack("<%dh" % len(samples), *(round(max(-1, min(1, v)) * 32767) for v in samples)))


def matched(x):
    win = int(.05 * SR)
    power = 0
    for i in range(0, len(x), win // 4):
        part = x[i:i + win]
        power = max(power, sum(v * v for v in part) / len(part))
        if i + win >= len(x):
            break
    gain = min(.25 / math.sqrt(max(1e-12, power)), .97 / max(1e-12, max(abs(v) for v in x)))
    return [v * gain for v in x]


def main():
    OUT.mkdir(parents=True, exist_ok=True)
    reel = []
    notes = ["# Audio audition", "", "`combat.wav`: one cue per second, in this order:", ""]
    for i, cue in enumerate(CUES):
        x = read(ROOT / "assets" / "sfx" / (cue + ".wav"))
        reel.extend(x + [0.] * (SR - len(x)))
        notes.append(f"- {i:02d}s: {cue}")
    write(OUT / "combat.wav", reel)
    before = ROOT / "build" / "audio-before"
    comparison = []
    pairs = []
    for cue in CUES:
        old = before / (cue + ".wav")
        if not old.exists():
            continue
        pairs.append(cue)
        for path in (old, ROOT / "assets" / "sfx" / (cue + ".wav")):
            x = matched(read(path))
            comparison.extend(x + [0.] * (int(SR * 1.2) - len(x)))
    if comparison:
        write(OUT / "combat-before-after.wav", comparison)
        notes += ["", "`combat-before-after.wav`: old then new, 1.2 seconds each; matched short-term level.",
                  "Pairs: " + ", ".join(pairs) + "."]
    notes += ["", "The music WAVs render 32 seconds of the live soundtrack at combat/boss intensity.",
              "SUNSET and CORE are DnB; FOUNDRY is industrial techno."]
    for track, name, intensity in ((0, "sunset-dnb", "1.2"), (3, "core-dnb", "1.3"), (1, "foundry-techno", "1.2")):
        subprocess.run([str(ROOT / "tools" / "music_preview"), str(track), "32", str(OUT / (name + ".wav")), intensity], check=True, cwd=ROOT)
    (OUT / "README.md").write_text("\n".join(notes) + "\n")
    print(f"Audition files: {OUT}")


if __name__ == "__main__":
    main()
