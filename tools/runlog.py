#!/usr/bin/env python3
"""
Sums up runs.jsonl (written by the game beside the binary, src/game/RunLog.h):
what kills you, what hurts you most, each gun's accuracy and kills, how long
each enemy takes to kill, and how often stuns, finishers and OVERDRIVE happen.

    python3 tools/runlog.py [runs.jsonl] [--last N]
    python3 tools/runlog.py runs.dev.jsonl --include-dev
"""
import argparse
import json
from collections import Counter, defaultdict


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("path", nargs="?", default="runs.jsonl")
    parser.add_argument("--last", type=int)
    parser.add_argument("--include-dev", action="store_true",
                        help="include marked practice/capture rows (normally excluded)")
    args = parser.parse_args()
    rows = [json.loads(line) for line in open(args.path) if line.strip()]
    if not args.include_dev:
        rows = [r for r in rows if not r.get("practice") and not r.get("capture")]
    if args.last:
        rows = rows[-args.last:]
    if not rows:
        print("no runs logged yet"); return
    secs = sum(r["seconds"] for r in rows)
    deaths = [r for r in rows if r["how"] == "death"]
    print(f"{len(rows)} stretches, {secs / 60:.1f} min played, {len(deaths)} deaths")

    killers = Counter(r["killedBy"] for r in deaths)
    if killers:
        print("\nwhat killed you:   " + ", ".join(f"{k} {n}" for k, n in killers.most_common()))

    dmg, hits = Counter(), Counter()
    for r in rows:
        for k, (d, n) in r["damageTaken"].items(): dmg[k] += d; hits[k] += n
    if dmg:
        print("\ndamage taken (per minute played):")
        for k, d in dmg.most_common():
            print(f"  {k:<13} {d / max(secs / 60, 1e-6):7.1f}  ({hits[k]} hits, {d / hits[k]:.1f} each)")

    print("\nguns:              shots   hit %   kills")
    guns = defaultdict(lambda: [0, 0, 0])
    for r in rows:
        for g, (s, h, k) in r["guns"].items():
            guns[g][0] += s; guns[g][1] += h; guns[g][2] += k
    for g, (s, h, k) in guns.items():
        print(f"  {g:<15} {s:7d} {100 * h / s if s else 0:6.0f}% {k:7d}")

    ttk, ttkn = Counter(), Counter()
    for r in rows:
        for k, (t, n) in r["ttk"].items(): ttk[k] += t * n; ttkn[k] += n
    if ttkn:
        print("\ntime to kill (first hit to dead):")
        for k in sorted(ttkn, key=lambda k: -ttk[k] / ttkn[k]):
            print(f"  {k:<13} {ttk[k] / ttkn[k]:5.2f} s  ({ttkn[k]} kills)")

    kills = sum(sum(r["kills"].values()) for r in rows)
    stuns = sum(r["stuns"] for r in rows); fin = sum(r["finishers"] for r in rows)
    heal = sum(r["finisherHeal"] for r in rows)
    od = sum(r["overdrives"] for r in rows); ods = sum(r["overdriveSeconds"] for r in rows)
    print(f"\n{kills} kills, {stuns} stuns, {fin} finishers ({heal:.0f} HP back), "
          f"OVERDRIVE {od}x for {ods:.0f} s ({100 * ods / max(secs, 1e-6):.0f}% of the time)")

    presses = sum(r.get("parryAttempts", 0) for r in rows)
    contacts = sum(r.get("parrySuccesses", 0) for r in rows)
    retries = sum(r.get("retries", 0) for r in rows)
    print(f"\n{presses} punch/guard presses, {contacts} successful parry contacts, {retries} retries")
    story(rows)


def story(rows):
    """STORY: deaths, time and time lost, chapter by chapter and section by section."""
    rows = [r for r in rows if r.get("chapter")]
    if not rows:
        return
    print("\nSTORY (chapter.section: deaths, retries, minutes played, minutes lost):")
    deaths, retries, secs, lost, where = Counter(), Counter(), Counter(), Counter(), defaultdict(list)
    for r in rows:
        c = r["chapter"]
        retries[(c, r["section"])] += r.get("retries", 0)
        if r["how"] == "death":
            deaths[(c, r["section"])] += 1
            where[(c, r["section"])].append(("died", r["at"], r.get("killedBy", "")))
        for s, t in r.get("sectionSeconds", {}).items(): secs[(c, int(s))] += t
        for s, t in r.get("lostSeconds", {}).items(): lost[(c, int(s))] += t
        for p in r.get("lostAt", []):
            where[(c, r["section"])].append(("lost", p, ""))
    for key in sorted(set(secs) | set(deaths) | set(retries)):
        c, s = key
        flag = "  <- often lost" if lost[key] > 0.25 * max(secs[key], 1e-6) else ""
        print(f"  {c}.{s:<3} {deaths[key]:3d} deaths  {retries[key]:3d} retries  {secs[key] / 60:6.1f} min  {lost[key] / 60:5.1f} lost{flag}")
        for what, p, by in where[key][:4]:
            print(f"        {what} at ({p[0]:.0f} {p[1]:.0f} {p[2]:.0f}){' to ' + by if by else ''}")


if __name__ == "__main__":
    main()
