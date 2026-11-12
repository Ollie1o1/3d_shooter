#!/usr/bin/env python3
"""Measure every music track at the intensity the game plays it, against its loudness target.

    python3 tools/music_levels.py            # report (integrated LUFS, true peak) per track
    python3 tools/music_levels.py --solve    # also print the trackGain() table that hits the targets

Needs tools/music_preview (make tools/music_preview) and ffmpeg. Renders use preview volume 0.9;
the targets are relative to one another, not a mastering level: the game's music slider and the
final output gain sit on top.
"""
import re, subprocess, sys, tempfile, os, math

FIGHT, BOSS, MENU = -13.5, -12.0, -15.0
# (track, intensity, target LUFS)
TRACKS = [(0, 1.0, FIGHT), (1, 1.0, FIGHT), (2, 1.0, FIGHT), (3, 1.0, FIGHT), (4, 1.35, BOSS),
          (5, 1.0, FIGHT), (6, 1.0, FIGHT), (7, 1.35, BOSS), (8, 1.35, BOSS),
          (9, 1.35, BOSS), (10, 1.35, BOSS), (11, 1.0, FIGHT), (12, 1.28, BOSS), (13, 1.25, BOSS),
          (14, 1.0, None), (15, 1.23, BOSS), (16, 0.55, MENU)]
SECONDS = 96   # two passes of a boss's thirty-two bars

def measure(track, intensity, gain, wav):
    name = subprocess.run(["tools/music_preview", str(track), str(SECONDS), wav, str(intensity), "--gain", str(gain)],
                          capture_output=True, text=True, check=True).stdout.split(":")[0]
    err = subprocess.run(["ffmpeg", "-hide_banner", "-nostats", "-ss", "2", "-i", wav, "-af", "ebur128=peak=true",
                          "-f", "null", "-"], capture_output=True, text=True).stderr
    err = err[err.rindex("Summary"):]
    return name, float(re.search(r"I:\s+(-?[\d.]+)", err).group(1)), float(re.search(r"Peak:\s+(-?[\d.]+)", err).group(1))

def main():
    solve = "--solve" in sys.argv
    wav = os.path.join(tempfile.mkdtemp(), "level.wav")
    gains, worst = [], 0.0
    for track, intensity, target in TRACKS:
        gain = 1.0
        name, lufs, peak = measure(track, intensity, gain, wav)
        if target is None:   # COLD STORAGE: slow and mostly choir, set by its quiet walk rather than a fight target
            print(f"{track:2d} {name:15s} @{intensity:<4} {lufs:6.1f} LUFS (no target)       {peak:5.1f} dBTP"); gains.append(gain); continue
        if solve:
            for _ in range(6):   # the bus compressor makes level a curve, not a line: step toward it
                if abs(target - lufs) < 0.05: break
                gain *= 10 ** ((target - lufs) * 1.3 / 20)
                name, lufs, peak = measure(track, intensity, gain, wav)
        gains.append(gain); worst = max(worst, abs(lufs - target))
        print(f"{track:2d} {name:15s} @{intensity:<4} {lufs:6.1f} LUFS (target {target:5.1f})  {peak:5.1f} dBTP"
              + (f"  x{gain:.3f}" if solve else ""))
    if solve: print("multiply the trackGain() table by: " + ", ".join(f"{g:.3f}" for g in gains))
    return 0 if solve or worst <= 1.0 else 1

if __name__ == "__main__":
    sys.exit(main())
