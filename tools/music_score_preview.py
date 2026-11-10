#!/usr/bin/env python3
"""Render the real synth, measure loudness, and prepare constant-gain A/B files.

make tools/music_preview
python3 tools/music_score_preview.py --baseline-dir build/music-pass/before
Requires ffmpeg. Originals stay untouched; comparison MP3s use constant gain,
not a compressor, so the score's breakdown/peak contrast survives the match.
"""
import argparse
import json
import math
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
TRACKS = [(0, "sunset", 1.0, 24), (10, "zenith", 1.3, 46), (11, "morningstar", 1.3, 45)]


def run(args):
    return subprocess.run([str(a) for a in args], check=True, capture_output=True, text=True)


def measure(path):
    result = run(["ffmpeg", "-hide_banner", "-nostats", "-i", path, "-af",
                  "loudnorm=I=-18:TP=-2:LRA=11:print_format=json", "-f", "null", "-"])
    data, _ = json.JSONDecoder().raw_decode(result.stderr[result.stderr.rfind("{"):])
    values = {key: float(data[key]) for key in ("input_i", "input_tp", "input_lra")}
    if not all(math.isfinite(v) for v in values.values()):
        raise ValueError(f"non-finite loudness measurement: {path}")
    return values


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--out", type=Path, default=ROOT / "build/music-pass")
    parser.add_argument("--baseline-dir", type=Path)
    args = parser.parse_args()
    output = args.out.resolve()
    after = output / "after"
    after.mkdir(parents=True, exist_ok=True)
    rows = []
    links = []
    for track, name, intensity, excerpt in TRACKS:
        for suffix, seconds, flags in [("", 64, []), ("-theme-muted", 12, ["--theme-muted"]),
                                        ("-exercise", 48, ["--exercise"])]:
            path = after / f"{name}{suffix}.wav"
            result = run([ROOT / "tools/music_preview", track, seconds, path, intensity, *flags])
            print(result.stdout.strip(), flush=True)
            rows.append({"file": str(path.relative_to(output)), "kind": "after", **measure(path)})
        if args.baseline_dir:
            old = args.baseline_dir.resolve() / f"{name}.wav"
            if not old.is_file():
                raise FileNotFoundError(old)
            rows.append({"file": str(old), "kind": "before", **measure(old)})
            sources = [old, after / f"{name}.wav"]
            excerpts = []
            for index, source in enumerate(sources):
                clip = output / f".{name}-{index}.wav"
                run(["ffmpeg", "-y", "-v", "error", "-i", source, "-t", excerpt, clip])
                excerpts.append(clip)
            readings = [measure(p) for p in excerpts]
            # Both receive the same integrated loudness, capped by whichever
            # excerpt needs the most headroom. Leave 2 dB for MP3 intersample peaks.
            target = min(-18.0, *(r["input_i"] - r["input_tp"] - 2 for r in readings))
            gains = [target - r["input_i"] for r in readings]
            filters = (f"[0:a]volume={gains[0]:.3f}dB,afade=t=in:d=0.015,"
                       f"afade=t=out:st={excerpt-.025}:d=0.025[a];"
                       f"[1:a]volume={gains[1]:.3f}dB,afade=t=in:d=0.015,"
                       f"afade=t=out:st={excerpt-.025}:d=0.025[b];"
                       "anullsrc=r=44100:cl=stereo,atrim=duration=1[s];"
                       "[a][s][b]concat=n=3:v=0:a=1[out]")
            comparison = output / f"{name}-before-after.mp3"
            run(["ffmpeg", "-y", "-v", "error", "-i", excerpts[0], "-i", excerpts[1],
                 "-filter_complex", filters, "-map", "[out]", "-ar", 44100,
                 "-codec:a", "libmp3lame", "-b:a", "192k", comparison])
            for clip in excerpts:
                clip.unlink()
            links.append(f"- [{name.upper()} before / after]({comparison.name}): "
                         f"old 0–{excerpt}s; 1s gap; new {excerpt+1}–{excerpt*2+1}s. "
                         f"Both excerpts matched to {target:.1f} LUFS with constant gain.")
    (output / "measurements.json").write_text(json.dumps(rows, indent=2) + "\n")
    text = "# Soundtrack pass audition\n\n" + "\n".join(links)
    text += "\n\nThe new excerpt includes each complete arrangement and its return. "
    text += "SUNSET is rendered at ordinary combat intensity 1.0; bosses at 1.3.\n\n"
    text += "Full 64s WAVs and theme-muted diagnostic WAVs are in `after/`. "
    text += "The 48s `-exercise.wav` files use the same synth with these controls:\n\n"
    text += "| Seconds | Control |\n| --- | --- |\n| 0–8 | Combat |\n| 8–16 | Overdrive |\n"
    text += "| 16–20 | Last-enemy solo |\n| 20 | Wave stinger |\n| 24–28 | Pause filter |\n"
    text += "| 32 | Rewind |\n| 36 | Menu |\n| 40–48 | Return to track |\n\n"
    text += "`measurements.json` contains FFmpeg integrated LUFS, true peak dBTP and LRA. "
    text += "Measurements verify signal behavior; they do not replace listening to the mix.\n"
    (output / "README.md").write_text(text)
    print(f"Audition files and measurements: {output}")


if __name__ == "__main__":
    main()
