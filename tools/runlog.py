#!/usr/bin/env python3
"""
Sums up runs.jsonl (written by the game beside the binary, src/game/RunLog.h):
what kills you, what hurts you most, each gun's accuracy and kills, how long
each enemy takes to kill, and how often stuns, finishers and OVERDRIVE happen.

    python3 tools/runlog.py [runs.jsonl] [--last N]
"""
import json
import sys
from collections import Counter, defaultdict


def main():
    args = sys.argv[1:]
    last = None
    if "--last" in args:
        i = args.index("--last"); last = int(args[i + 1]); del args[i:i + 2]
    path = args[0] if args else "runs.jsonl"
    rows = [json.loads(l) for l in open(path) if l.strip()]
    if last: rows = rows[-last:]
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


if __name__ == "__main__":
    main()
