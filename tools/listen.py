#!/usr/bin/env python3
"""Turn audio into something Claude (or anyone) can inspect without ears.

Claude cannot hear. This renders a WAV into a "listening sheet" PNG plus a text
report describing what a listener would notice: loudness over time, noisy /
whooshing stretches, mud and masking by frequency band, transient sharpness,
silence, clipping, mono problems and repeated identical events.

    make listen-setup                                   # one-time venv
    build/.listen-venv/bin/python tools/listen.py sheet clip.wav [--marks marks.md]
    build/.listen-venv/bin/python tools/listen.py compare before.wav after.wav
    build/.listen-venv/bin/python tools/listen.py family 'assets/sfx/shep_*.wav'
    build/.listen-venv/bin/python tools/listen.py masking build/listen/ch6   # stems dir

Outputs land in build/listen/. The measurements are proxies for perception, not
a substitute for a human audition on speakers and headphones.
"""
import argparse
import glob
import json
import re
import subprocess
import sys
from pathlib import Path

import numpy as np
from scipy import signal
import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt  # noqa: E402

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "build" / "listen"
SR = 44100
BANDS = [("sub <60", 20, 60), ("low 60-250", 60, 250), ("low-mid 250-1k", 250, 1000),
         ("mid 1-4k", 1000, 4000), ("high 4k+", 4000, 20000)]
BAND_COLORS = ["#3b1f6e", "#7b3fa0", "#d0587e", "#f39c4a", "#f7e26b"]


# ---------------------------------------------------------------- loading

def load(path):
    """Mono-and-stereo float audio at SR; uses ffmpeg so mp3/ogg also work."""
    raw = subprocess.run(
        ["ffmpeg", "-v", "error", "-i", str(path), "-f", "f32le", "-ac", "2", "-ar", str(SR), "-"],
        check=True, capture_output=True).stdout
    st = np.frombuffer(raw, dtype=np.float32).reshape(-1, 2).astype(np.float64)
    return st, st.mean(axis=1)


def k_weight(x):
    """ITU-R BS.1770 K-weighting (shelf + high-pass) at 44.1 kHz."""
    b1, a1 = [1.53090959, -2.65116903, 1.16916686], [1.0, -1.66375011, 0.71265753]
    b2, a2 = [1.0, -2.0, 1.0], [1.0, -1.98992552, 0.98997601]
    return signal.lfilter(b2, a2, signal.lfilter(b1, a1, x, axis=0), axis=0)


def short_term_lufs(st, hop=0.1, win=0.4):
    kw = k_weight(st)
    p = (kw ** 2).sum(axis=1)
    n, h = int(win * SR), int(hop * SR)
    c = np.concatenate([[0], np.cumsum(p)])
    starts = np.arange(0, max(1, len(p) - n), h)
    ms = (c[np.minimum(starts + n, len(p))] - c[starts]) / n
    return starts / SR + win / 2, -0.691 + 10 * np.log10(ms + 1e-12)


def integrated_lufs(times, lufs):
    g = lufs[lufs > -70]
    if not len(g):
        return -np.inf
    rel = 10 * np.log10(np.mean(10 ** (g / 10))) - 10
    g = g[g > rel]
    return 10 * np.log10(np.mean(10 ** (g / 10)))


def true_peak(path):
    out = subprocess.run(["ffmpeg", "-hide_banner", "-nostats", "-i", str(path), "-af",
                          "ebur128=peak=true", "-f", "null", "-"], capture_output=True, text=True).stderr
    m = re.findall(r"Peak:\s+(-?[\d.]+|-inf)\s+dBFS", out)
    return float(m[-1]) if m and m[-1] != "-inf" else -np.inf


# ---------------------------------------------------------------- features

def mel_fb(n_fft, n_mels=96, fmin=20, fmax=18000):
    mel = lambda f: 2595 * np.log10(1 + f / 700)
    imel = lambda m: 700 * (10 ** (m / 2595) - 1)
    pts = imel(np.linspace(mel(fmin), mel(fmax), n_mels + 2))
    bins = np.fft.rfftfreq(n_fft, 1 / SR)
    fb = np.zeros((n_mels, len(bins)))
    for i in range(n_mels):
        lo, c, hi = pts[i:i + 3]
        fb[i] = np.clip(np.minimum((bins - lo) / (c - lo), (hi - bins) / (hi - c)), 0, None)
    return fb, pts[1:-1]


def stft(x, n_fft=2048, hop=512):
    f, t, Z = signal.stft(x, SR, nperseg=n_fft, noverlap=n_fft - hop, boundary=None, padded=False)
    return f, t, np.abs(Z)


def features(mono):
    f, t, mag = stft(mono)
    power = mag ** 2
    fb, mel_f = mel_fb(2048)
    mel_db = 10 * np.log10(fb @ power + 1e-12)
    total = power.sum(axis=0) + 1e-20
    frame_db = 10 * np.log10(total / 2048 + 1e-20)
    # Flatness 1 = white noise, 0 = pure tone; measured 150 Hz-12 kHz where hiss/whoosh lives.
    sel = (f > 150) & (f < 12000)
    ps = power[sel] + 1e-20
    flat = np.exp(np.mean(np.log(ps), axis=0)) / np.mean(ps, axis=0)
    centroid = (f[:, None] * power).sum(axis=0) / total
    bands = np.array([power[(f >= lo) & (f < hi)].sum(axis=0) for _, lo, hi in BANDS]) / total
    flux = np.r_[0, np.sqrt((np.diff(mag, axis=1).clip(0) ** 2).sum(axis=0))]
    return dict(t=t, mel_db=mel_db, mel_f=mel_f, frame_db=frame_db, flat=flat,
                centroid=centroid, bands=bands, flux=flux, power=power, f=f)


def onsets(feat, min_gap=0.06):
    fl = feat["flux"]
    if fl.max() <= 0:
        return np.array([])
    norm = fl / (np.median(fl) + 1e-9)
    loud = feat["frame_db"] > feat["frame_db"].max() - 50
    thresh = max(6.0, np.percentile(norm, 90))
    peaks, _ = signal.find_peaks(norm, height=thresh, distance=max(1, int(min_gap * SR / 512)))
    peaks = peaks[loud[peaks]]
    return feat["t"][peaks]


def event_print(feat, t0, dur=0.35):
    """A compact timbre fingerprint of the 350 ms after an onset."""
    sel = (feat["t"] >= t0) & (feat["t"] < t0 + dur)
    if sel.sum() < 2:
        return None
    m = feat["mel_db"][:, sel]
    v = np.r_[m.mean(axis=1), (m[:, :len(m[0]) // 2].mean(axis=1) - m[:, len(m[0]) // 2:].mean(axis=1))]
    v = v - v.mean()
    return v / (np.linalg.norm(v) + 1e-9)


def runs(mask, t, min_len):
    """Contiguous True stretches at least min_len seconds long, as (start, end)."""
    out, start = [], None
    for i, m in enumerate(np.r_[mask, False]):
        if m and start is None:
            start = i
        elif not m and start is not None:
            if t[min(i, len(t) - 1)] - t[start] >= min_len:
                out.append((t[start], t[min(i, len(t) - 1)]))
            start = None
    return out


def fmt(s):
    return f"{int(s // 60)}:{s % 60:05.2f}"


# ---------------------------------------------------------------- analysis

def analyse(path):
    st, mono = load(path)
    feat = features(mono)
    lt, lufs = short_term_lufs(st)
    peak = float(np.abs(st).max())
    rms = float(np.sqrt(np.mean(mono ** 2)) + 1e-12)
    l, r = st[:, 0], st[:, 1]
    corr = float(np.corrcoef(l, r)[0, 1]) if l.std() > 0 and r.std() > 0 else 1.0
    mono_loss = 10 * np.log10((mono ** 2).mean() / (((l ** 2 + r ** 2) / 2).mean() + 1e-20) + 1e-20)
    ons = onsets(feat)
    t, active = feat["t"], feat["frame_db"] > -60
    noisy = active & (feat["flat"] > 0.30) & (feat["frame_db"] > feat["frame_db"].max() - 30)
    b = feat["bands"]
    mud = active & (b[2] > 0.45)
    boom = active & (b[0] + b[1] > 0.85)
    dull = active & (feat["centroid"] < 350)
    report = dict(
        file=str(path), seconds=round(len(mono) / SR, 2),
        integrated_lufs=round(integrated_lufs(lt, lufs), 1),
        max_short_term_lufs=round(float(lufs.max()), 1),
        loudness_range_lu=round(float(np.percentile(lufs[lufs > -70], 95) - np.percentile(lufs[lufs > -70], 10)), 1)
        if (lufs > -70).sum() > 5 else 0.0,
        sample_peak_dbfs=round(20 * np.log10(peak + 1e-12), 1),
        true_peak_dbtp=true_peak(path),
        crest_db=round(20 * np.log10(peak / rms), 1),
        clipped_samples=int((np.abs(st) >= 0.999).sum()),
        silent_pct=round(100 * float((~active).mean()), 1),
        noise_dominated_pct=round(100 * float(noisy.sum() / max(1, active.sum())), 1),
        lr_correlation=round(corr, 2), mono_energy_change_db=round(float(mono_loss), 2),
        onsets=len(ons), onsets_per_sec=round(len(ons) / max(0.1, len(mono) / SR), 2),
        band_share_pct={n: round(100 * float((b[i] * (10 ** (feat['frame_db'] / 10))).sum() /
                                                 ((10 ** (feat['frame_db'] / 10)).sum() + 1e-20)), 1)
                        for i, (n, _, _) in enumerate(BANDS)},
    )
    notes = []
    for a, z in runs(noisy, t, 0.6):
        notes.append(f"{fmt(a)}-{fmt(z)}: broadband noise dominates (flatness > 0.3) - reads as hiss/whoosh")
    for a, z in runs(mud, t, 1.0):
        notes.append(f"{fmt(a)}-{fmt(z)}: >45% of energy in 250-1k - boxy/muddy, masks voices of other cues")
    for a, z in runs(boom, t, 1.5):
        notes.append(f"{fmt(a)}-{fmt(z)}: >85% of energy below 250 Hz - boomy; little will survive laptop speakers")
    for a, z in runs(dull & ~boom, t, 1.5):
        notes.append(f"{fmt(a)}-{fmt(z)}: spectral centroid < 350 Hz - dark/dull, lacks bite")
    # Repetition: onsets whose next 350 ms are near-identical in timbre.
    prints = [(o, event_print(feat, o)) for o in ons]
    prints = [(o, p) for o, p in prints if p is not None]
    clusters = []
    for o, p in prints:
        for c in clusters:
            if float(c["p"] @ p) > 0.97:
                c["at"].append(o)
                break
        else:
            clusters.append(dict(p=p, at=[o]))
    rep = [c["at"] for c in clusters if len(c["at"]) >= 4]
    for at in sorted(rep, key=len, reverse=True)[:6]:
        notes.append(f"{len(at)} near-identical events (timbre corr > 0.97), e.g. at "
                     + ", ".join(fmt(x) for x in at[:5]) + " - machine-gun repetition risk")
    if report["clipped_samples"]:
        notes.append(f"{report['clipped_samples']} clipped samples")
    if report["true_peak_dbtp"] > -1:
        notes.append(f"true peak {report['true_peak_dbtp']} dBTP - risk of inter-sample clipping on encode")
    if report["mono_energy_change_db"] < -3:
        notes.append(f"mono downmix loses {-report['mono_energy_change_db']} dB - phasey stereo")
    gaps = runs(~active, t, 0.5)
    if active.mean() > 0.97 and len(mono) > 10 * SR:
        notes.append("almost no silence anywhere - nothing gives the ear a rest or makes cues stand out")
    report["notes"] = notes
    report["repeated_event_groups"] = len(rep)
    report["silent_gaps_over_0_5s"] = len(gaps)
    return st, mono, feat, (lt, lufs), ons, report


def read_marks(path):
    marks = []
    if not path:
        return marks
    for line in Path(path).read_text().splitlines():
        m = re.match(r"\s*[-*]?\s*(\d+(?:\.\d+)?)\s*s?\s*[:\-]\s*(.+)", line)
        if m:
            marks.append((float(m.group(1)), m.group(2).strip()[:28]))
    return marks


# ---------------------------------------------------------------- drawing

def draw_sheet(path, out_png, marks=(), title=None, t_range=None):
    st, mono, feat, (lt, lufs), ons, rep = analyse(path)
    t = feat["t"]
    dur = len(mono) / SR
    x0, x1 = t_range or (0, dur)
    fig, ax = plt.subplots(5, 1, figsize=(18, 13), sharex=True,
                           gridspec_kw=dict(height_ratios=[1, 3.2, 1.2, 1.2, 1.4]))
    fig.patch.set_facecolor("white")
    ts = np.arange(len(mono)) / SR
    step = max(1, len(mono) // 20000)
    ax[0].fill_between(ts[::step], st[::step, 0], -st[::step, 1], color="#334", lw=0)
    ax[0].set_ylim(-1, 1)
    ax[0].axhline(0.891, color="r", lw=0.5, ls=":")
    ax[0].axhline(-0.891, color="r", lw=0.5, ls=":")
    ax[0].set_ylabel("wave\nL up / R down")
    mel = feat["mel_db"]
    vmax = mel.max()
    ax[1].pcolormesh(t, np.arange(mel.shape[0]), mel, vmin=vmax - 80, vmax=vmax, cmap="magma", shading="auto")
    ticks = [50, 100, 200, 500, 1000, 2000, 4000, 8000, 16000]
    idx = [int(np.argmin(np.abs(feat["mel_f"] - f))) for f in ticks]
    ax[1].set_yticks(idx, [f"{f // 1000}k" if f >= 1000 else str(f) for f in ticks])
    ax[1].set_ylabel("mel spectrogram (Hz)\n80 dB range")
    for o in ons:
        ax[1].axvline(o, ymax=0.04, color="cyan", lw=0.8)
    ax[2].plot(lt, lufs, color="#1a5", lw=1)
    ax[2].set_ylim(max(-60, lufs.max() - 45), lufs.max() + 3)
    ax[2].axhline(rep["integrated_lufs"], color="#1a5", ls="--", lw=0.6)
    ax[2].set_ylabel("short-term\nLUFS")
    ax[3].plot(t, feat["flat"], color="#c33", lw=0.8, label="noisiness (flatness)")
    ax[3].axhline(0.3, color="#c33", ls=":", lw=0.6)
    ax[3].set_ylim(0, 1)
    ax[3].set_ylabel("noisiness\n(whoosh>0.3)")
    a3 = ax[3].twinx()
    a3.semilogy(t, feat["centroid"].clip(50, 20000), color="#36c", lw=0.8)
    a3.set_ylim(80, 12000)
    a3.set_ylabel("brightness Hz", color="#36c")
    w = 10 ** (feat["frame_db"] / 10)
    w = w / (w.max() + 1e-20)
    ax[4].stackplot(t, feat["bands"] * np.sqrt(w), colors=BAND_COLORS, labels=[b[0] for b in BANDS])
    ax[4].set_ylabel("band share\n(x level)")
    ax[4].legend(loc="upper right", fontsize=7, ncol=5)
    ax[4].set_xlabel("seconds")
    for m, label in marks:
        for a in ax:
            a.axvline(m, color="#0aa", lw=0.7, alpha=0.6)
        ax[0].text(m, 0.95, label, fontsize=7, rotation=0, va="top", color="#066",
                   bbox=dict(fc="white", ec="none", alpha=0.7, pad=1))
    ax[0].set_xlim(x0, x1)
    ax[4].xaxis.set_major_locator(plt.MaxNLocator(30))
    fig.suptitle(f"{title or Path(path).name}   |   {rep['integrated_lufs']} LUFS   "
                 f"TP {rep['true_peak_dbtp']} dBTP   noise {rep['noise_dominated_pct']}%   "
                 f"silence {rep['silent_pct']}%   onsets {rep['onsets_per_sec']}/s", fontsize=12)
    fig.tight_layout()
    fig.savefig(out_png, dpi=72)
    plt.close(fig)
    return rep


def write_report(rep, out_txt):
    lines = [f"# {Path(rep['file']).name}", ""]
    for k, v in rep.items():
        if k not in ("file", "notes"):
            lines.append(f"{k}: {v}")
    lines += ["", "## What a listener would notice"] + [f"- {n}" for n in rep["notes"] or ["nothing flagged"]]
    Path(out_txt).write_text("\n".join(lines) + "\n")
    return "\n".join(lines)


# ---------------------------------------------------------------- family

def cue_print(mono):
    feat = features(mono)
    if feat["mel_db"].shape[1] < 3:
        feat = features(np.r_[mono, np.zeros(4096)])
    m = feat["mel_db"]
    m = m - m.max()
    head = m[:, : max(2, int(0.15 * SR / 512))].mean(axis=1)
    body = m.mean(axis=1)
    env = feat["frame_db"]
    env = np.interp(np.linspace(0, len(env) - 1, 24), np.arange(len(env)), env - env.max())
    v = np.r_[body, head, env * 0.6]
    v = v - v.mean()
    return v / (np.linalg.norm(v) + 1e-9), feat


def family(pattern, out_png, variants=False):
    files = sorted(glob.glob(pattern))
    if not variants:
        files = [f for f in files if not re.search(r"_\d+\.wav$", f)]
    if not files:
        sys.exit(f"no files match {pattern}")
    names, prints, feats = [], [], []
    for f in files:
        _, mono = load(f)
        p, feat = cue_print(mono)
        names.append(Path(f).stem)
        prints.append(p)
        feats.append((feat, len(mono) / SR))
    P = np.array(prints)
    S = P @ P.T
    n = len(names)
    cols = 6
    rows = (n + cols - 1) // cols
    fig = plt.figure(figsize=(cols * 3.2, rows * 2.2 + max(6, n * 0.28)))
    gs = fig.add_gridspec(rows + 1, cols, height_ratios=[2.2] * rows + [max(6, n * 0.28)])
    for i, (name, (feat, dur)) in enumerate(zip(names, feats)):
        a = fig.add_subplot(gs[i // cols, i % cols])
        mel = feat["mel_db"]
        a.pcolormesh(feat["t"], np.arange(mel.shape[0]), mel, vmin=mel.max() - 70, vmax=mel.max(),
                     cmap="magma", shading="auto")
        a.set_title(f"{name}  {dur:.2f}s", fontsize=8)
        a.set_yticks([int(np.argmin(np.abs(feat['mel_f'] - f))) for f in (100, 1000, 8000)], ["100", "1k", "8k"],
                     fontsize=6)
        a.tick_params(axis="x", labelsize=6)
    a = fig.add_subplot(gs[rows, :])
    im = a.imshow(S, cmap="RdYlGn_r", vmin=0.3, vmax=1.0)
    a.set_xticks(range(n), names, rotation=90, fontsize=7)
    a.set_yticks(range(n), names, fontsize=7)
    for i in range(n):
        for j in range(n):
            if i != j and S[i, j] > 0.85:
                a.text(j, i, f"{S[i, j]:.2f}"[1:], ha="center", va="center", fontsize=5)
    fig.colorbar(im, ax=a, fraction=0.02, label="timbre similarity (1 = same sound)")
    fig.tight_layout()
    fig.savefig(out_png, dpi=72)
    plt.close(fig)
    pairs = sorted(((S[i, j], names[i], names[j]) for i in range(n) for j in range(i + 1, n)), reverse=True)
    return names, pairs


# ---------------------------------------------------------------- masking

def read_events(path, pattern):
    ev = []
    for line in Path(path).read_text().splitlines():
        m = re.match(r"([\d.]+) s: (\S+)", line)
        if m and re.search(pattern, m.group(2)):
            ev.append((float(m.group(1)), m.group(2)))
    return ev


def masking(stem_dir, pattern, out_png, window=0.4):
    """Per cue: how far the effects rise above music+beds in the cue's own bands.

    The effects stem holds every sound effect, so guns/steps inside the window
    count toward the cue; concurrent effects are listed so that can be judged.
    """
    d = Path(stem_dir)
    _, fx = load(d / "sfx.wav")
    _, mus = load(d / "music.wav")
    _, beds = load(d / "beds.wav")
    masker = mus + beds
    if (d / "focus.wav").exists():   # exact: the judged cues alone vs everything else
        _, foc = load(d / "focus.wav")
        masker, fx = masker + fx - foc, foc
    ffx, fmk = features(fx), features(masker)
    all_ev = read_events(d / "events.md", ".")
    ev = read_events(d / "events.md", pattern)
    t = ffx["t"]
    rows = []
    lengths = {}
    for at, name in ev:
        if name not in lengths:   # judge a short cue over its own length, not a fixed window
            src = ROOT / "assets" / "sfx" / f"{name}.wav"
            lengths[name] = float(np.clip(len(load(src)[1]) / SR, 0.1, window)) if src.exists() else window
        sel = (t >= at) & (t < at + lengths[name])
        if sel.sum() < 2:
            continue
        cue = 10 * np.log10(np.mean(10 ** (ffx["mel_db"][:, sel] / 10), axis=1))
        mk = 10 * np.log10(np.mean(10 ** (fmk["mel_db"][:, sel] / 10), axis=1))
        own = cue > cue.max() - 20            # the cue's own bands
        snr = cue - mk
        band_snr = float(np.percentile(snr[own], 75))
        e_c, e_m = (10 ** (cue / 10)).sum(), (10 ** (mk / 10)).sum()
        overall = float(10 * np.log10(e_c / (e_m + 1e-20) + 1e-20))
        peak_f = float(ffx["mel_f"][int(np.argmax(cue))])
        others = sorted({n for a, n in all_ev if abs(a - at) < 0.2 and n != name and not re.search(pattern, n)})
        verdict = "clear" if band_snr > 6 else "audible" if band_snr > 0 else "MASKED"
        rows.append(dict(t=at, name=name, band_snr=round(band_snr, 1), overall_snr=round(overall, 1),
                         peak_hz=round(peak_f), verdict=verdict, with_=others))
    # Picture: effects vs masker spectrogram with the cue marks, and a cue-SNR lane.
    fig, ax = plt.subplots(3, 1, figsize=(18, 10), sharex=True, gridspec_kw=dict(height_ratios=[2.5, 2.5, 1.3]))
    for a, f, title in ((ax[0], ffx, "judged cues"), (ax[1], fmk, "everything else (masker)")):
        a.pcolormesh(f["t"], np.arange(f["mel_db"].shape[0]), f["mel_db"], vmin=-90, vmax=-10, cmap="magma",
                     shading="auto")
        idx = [int(np.argmin(np.abs(f["mel_f"] - x))) for x in (100, 500, 2000, 8000)]
        a.set_yticks(idx, ["100", "500", "2k", "8k"])
        a.set_ylabel(title)
    colors = dict(clear="#2a2", audible="#e90", MASKED="#d22")
    for r in rows:
        ax[2].bar(r["t"], r["band_snr"], width=0.25, color=colors[r["verdict"]])
        ax[2].text(r["t"], r["band_snr"] + 1, r["name"].replace("shep_", "").replace("ring_", "r:"),
                   rotation=90, fontsize=6, va="bottom", ha="center")
        ax[0].axvline(r["t"], color=colors[r["verdict"]], lw=0.6)
    ax[2].axhline(6, color="#2a2", ls=":", lw=0.6)
    ax[2].axhline(0, color="#d22", ls=":", lw=0.6)
    ax[2].set_ylabel("cue over masker\nin cue's bands (dB)")
    ax[2].set_ylim(min(-15, min([r["band_snr"] for r in rows] or [0]) - 3), max(40, max([r["band_snr"] for r in rows] or [0]) + 12))
    ax[2].xaxis.set_major_locator(plt.MaxNLocator(30))
    fig.suptitle(f"{d.name}: cue audibility over everything else  (green > 6 dB clear, orange 0-6 audible, red masked)")
    fig.tight_layout()
    fig.savefig(out_png, dpi=72)
    plt.close(fig)
    return rows


# ---------------------------------------------------------------- cli

def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = ap.add_subparsers(dest="cmd", required=True)
    s = sub.add_parser("sheet", help="listening sheet + report for one clip")
    s.add_argument("wav")
    s.add_argument("--marks", help="markdown/text with lines like '4 s: fist'")
    s.add_argument("--range", nargs=2, type=float, metavar=("START", "END"))
    s.add_argument("--name")
    c = sub.add_parser("compare", help="stacked sheets + metric diff for before/after")
    c.add_argument("before")
    c.add_argument("after")
    c.add_argument("--marks")
    c.add_argument("--name", default="compare")
    f = sub.add_parser("family", help="contact sheet + similarity matrix for a set of cues")
    f.add_argument("pattern")
    f.add_argument("--variants", action="store_true", help="include _1/_2 variant files")
    f.add_argument("--name", default="family")
    m = sub.add_parser("masking", help="cue audibility over music+beds from a stems dir")
    m.add_argument("stems", help="dir with sfx.wav music.wav beds.wav events.md")
    m.add_argument("--cues", default=r"^(shep_|ring_)", help="regex of cue names to judge")
    m.add_argument("--name")
    a = ap.parse_args()
    OUT.mkdir(parents=True, exist_ok=True)
    if a.cmd == "sheet":
        name = a.name or Path(a.wav).stem
        rep = draw_sheet(a.wav, OUT / f"{name}.png", read_marks(a.marks), name, a.range)
        print(write_report(rep, OUT / f"{name}.txt"))
        (OUT / f"{name}.json").write_text(json.dumps(rep, indent=2))
        print(f"\nsheet: {OUT / (name + '.png')}")
    elif a.cmd == "compare":
        marks = read_marks(a.marks)
        reps = []
        for tag, wav in (("before", a.before), ("after", a.after)):
            reps.append(draw_sheet(wav, OUT / f"{a.name}-{tag}.png", marks, f"{tag.upper()}  {Path(wav).name}"))
        keys = [k for k, v in reps[0].items() if isinstance(v, (int, float))]
        print(f"{'metric':28} {'before':>10} {'after':>10}")
        for k in keys:
            print(f"{k:28} {reps[0][k]:>10} {reps[1][k]:>10}")
        for tag, r in zip(("before", "after"), reps):
            print(f"\n## {tag} notes")
            print("\n".join(f"- {n}" for n in r["notes"]) or "- none")
        print(f"\nsheets: {OUT / (a.name + '-before.png')}  {OUT / (a.name + '-after.png')}")
    elif a.cmd == "masking":
        name = a.name or Path(a.stems).name + "-masking"
        rows = masking(a.stems, a.cues, OUT / f"{name}.png")
        lines = [f"{'time':>7}  {'cue':22} {'band':>6} {'total':>6} {'peak Hz':>7}  verdict   concurrent"]
        for r in rows:
            lines.append(f"{r['t']:7.2f}  {r['name']:22} {r['band_snr']:6.1f} {r['overall_snr']:6.1f} {r['peak_hz']:7}"
                         f"  {r['verdict']:8}  {', '.join(r['with_'])}")
        txt = "\n".join(lines)
        (OUT / f"{name}.txt").write_text(txt + "\n")
        print(txt + f"\n\nsheet: {OUT / (name + '.png')}")
    else:
        names, pairs = family(a.pattern, OUT / f"{a.name}.png", a.variants)
        print(f"{len(names)} cues. Most similar pairs (risk: player can't tell them apart):")
        for s_, x, y in pairs[:15]:
            print(f"  {s_:.3f}  {x}  ~  {y}")
        print(f"\nsheet: {OUT / (a.name + '.png')}")


if __name__ == "__main__":
    main()
