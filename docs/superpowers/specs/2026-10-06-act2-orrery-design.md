# Act II piece 3: The Orrery — design

Date: 2026-10-06 · Status: approved in conversation (sections 1–4) · Piece 3 of 6 of Act II

## Decisions made with the user

- **Footing**: a solid outer terrace (always safe, ground enemies live there) and
  two rotating rings over a void around a captive sun (optional, rewarding, risky).
- **Arena shift**: the sun **flares** — rotating arms of light that burn anything
  in the open; the rings speed up each wave.
- **Objective wave**: **HOLD a circle that rides the inner ring**.
- Run structure (stated, not questioned): the Nave's sealed door leads on to the
  Orrery; the run spans both arenas; TO BE CONTINUED and the finish beacon move to
  after the Orrery; menu label "PREVIEW · 2/4 ARENAS".

## 1. Layout and getting there

- The Nave's passage behind the organ (floor Y −57) continues north to a shaft
  that drops to a landing apron at Y −80 outside the Orrery's south gate.
  `LevelData::corridors` gains this corridor (Nave → Orrery), so Act II's arenas
  link like Act I's: the Nave's exit door opens on clearing it, walking in starts
  the Orrery and the gate closes behind. Lighting blends teal → gold down the corridor.
- **The Orrery**: centre (0, −80, −732). Terrace floor Y −80.
  - **Sun**: a huge glowing sphere sunk in the pit (centre Y −92, radius 9),
    self-lit, the landmark; swells and brightens while a flare burns.
  - **Outer terrace**: solid annulus R 30–52 (collision as strips of boxes), an
    outer wall at R 52+, **eight great pillars** at R 41 (cover from the flare,
    grapple anchors), armillary arcs and brass gearwork as dressing.
  - **Outer ring**: 26 square segments (half-size 2.5) orbiting at R 22, tops at
    Y −80, clockwise, ~2.2 m/s.
  - **Inner ring**: 16 segments (half-size 2.2) at R 12, tops at Y −78 (+2 m),
    counter-clockwise, ~2.2 m/s.
  - **Four spokes** from the terrace (N, E, S, W) to R 25.9 at Y −80: a short
    running jump onto the outer ring (a segment's AABB reaches R + h√2 at 45°;
    spoke tips stay outside that).
  - Outer ring → inner ring: a jump plus double jump (or a grapple).
  - **Void**: a fall below Y −105 returns you to the south terrace with the
    usual 15-damage penalty (`Arena::hasRespawn`). Enemies that fall die
    (environment kill).
- Floors below: two basins — the Nave's (Z −440…−668) at −60, the Orrery's
  (Z −668…−1000) at −140. The Orrery has no water.
- Ground spawns on the terrace only; air spawns over the pit.
- Finish: a sealed arch on the north terrace (finish beacon at (0, −80, −778))
  opens on victory.

## 2. The sun's flare — `ArenaShift::SOLAR`

- One or two arms, each a **24° wedge**, turning at **15°/s** about the sun.
- Per-arm cycle: **warning 1.5 s** (faint gold on the ground, hum) → **burn 5 s**
  (white-gold, burns) → **rest 4 s** (dark, still turning). Two arms share the cycle, 180° apart.
- **25/s** to the player (0.2 s ticks of 5, no i-frames); enemies on foot take
  10 per tick (environment, style like lava).
- **Cover**: a wall (pillar, ring segment, any wall) on the segment from the sun
  to the target's chest shades it.
- Waves: 1 arm / ring speed ×1, 1 arm / ×1.3, 2 arms / ×1.6.
- Rendering: a fan of additive beams across the wedge (faint while warning,
  bright while burning); the sun swells while burning.

## 3. Waves, the riding HOLD, code

| Wave | Goal | Mix |
|---|---|---|
| 1 | Kill all | Husks, 2 Haloed Sentinels, Raptors, 1 Seraph, a Shieldbearer squad |
| 2 | HOLD the circle on the inner ring, 12 s | 2 Seraphs, Raptors, Conductors, Mites, 2 Twinned Rippers |
| 3 | Kill all | 2 Anchors, 2 Enraged Brutes, a Twinned Shieldbearer squad, 2 Seraphs, Husks |

`maxAlive` 12, `damageScale` 1.35.

- **Riding HOLD**: `WaveGoal::mover` (index of a ring segment, −1 = fixed).
  `WaveDirector::goalPos()` returns the segment's top centre when set, else
  `pos`; `inHoldZone`, the HUD marker and the circle's drawing use it.
- Ring speed per wave reuses SPEED_UP's `setSpeed`; SOLAR arenas get the same
  reset/onWave handling.
- Music: a seventh synth preset "ORRERY"; Act II picks track 5 + arena index.

## 4. Testing

Automated: map checks (spawns, ceilings, pads, doors) for the Orrery; ring
movers never pass through static walls over two full turns; each ring keeps
its segments at constant radius and spacing; a player on a segment for 10 s
is carried and stays on; a running jump from a spoke tip lands on the outer
ring at several points of its turn; a jump + double jump from the outer ring
reaches the inner ring; `goalPos()` follows its segment and a player on it fills
the hold while one on the terrace doesn't; flare timings, wedge hit, pillar
cover, two arms in wave 3, ring speeds ×1.3/×1.6; void respawn; the corridor
joins the arenas and a simulated ACT II run clears the Nave, enters the Orrery
and finishes; the Nave and Act I keep passing their existing checks.

Manual: screenshots of the drop, the overview with the sun, standing on a ring,
the flare warning and burning, the wave-2 HOLD; `--bench` with the flare burning
vs the Sanctum (within ~15%); `make web`.

## Out of scope

The Descent and the Penitent (piece 4); enemies riding rings; a new music
track beyond a preset; the ACT II leaderboard.
