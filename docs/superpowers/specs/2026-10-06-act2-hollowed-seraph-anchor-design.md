# Act II piece 2: Hollowed variants, the Seraph, the Anchor — design

Date: 2026-10-06 · Status: awaiting review · Piece 2 of 6 of Act II
(piece 1, the foundation and the Drowned Nave:
`2026-10-06-act2-foundation-drowned-nave-design.md`, merged)

## Context and decisions

Act II needs harder enemies. Piece 2 adds a modifier system that makes any
regular enemy harder in a specific way (Hollowed variants), and two new
enemies built for big arenas: the Seraph (punishes standing in the open) and
the Anchor (takes your movement tools away).

Decisions made with the user:

- Hollowed variants are **authored per wave entry**, not random.
- Three variants: **Enraged**, **Twinned**, **Haloed**. A halo breaks on a
  **headshot or a parry**.
- Seraph: a **slow-turning sweep beam**.
- Anchor: a **slow walker with a field** that turns off dash and grapple.
- They appear **in Act II only**; ENDLESS and DAILY are unchanged.

## 1. Hollowed variants

### Data

- `enum class Hollow { NONE, ENRAGED, TWINNED, HALOED };` and `Hollow hollow`
  on `Enemy`.
- `WaveEntry::hollow(Hollow)` builder, next to `.with()`; for a squad it marks
  the leader only.
- `SpawnRequest` carries the variant; `GameplayState::spawnEnemy` takes it.
- Bosses (`isBoss`), CONDUIT and CONDUCTOR never take a variant (the builder
  call is ignored for them; a test pins it).

### Rules (Enemy.h, headless-testable)

| Variant | Effect |
|---|---|
| Enraged | move speed ×1.3, telegraphs ×0.65, attack interval ×0.75, damage dealt ×1.25. Applied on top of difficulty tuning. |
| Twinned | on death sets `ev.split`; GameplayState spawns two of the same type with `hollow = NONE`, each at 35% of the parent's max health, rendered at 75% scale, pushed 1.2 m apart. Never more than one split. |
| Haloed | while the halo is up, incoming damage ×0.1. A headshot (`RayHit.head`) or a parry of one of its attacks (projectile or melee) breaks it: 0.4 s stagger, then ×2 damage taken for 2 s. Style "HALO BROKEN" +40. Multiplies with a Conductor tether. |

A Hollowed kill gives ×1.5 XP and style.

### Readability

- Enraged: red emissive eyes/core, a faint ember trail.
- Twinned: a glowing seam down the middle (copies have none).
- Haloed: a thick gold ring turning over its head; shatters into particles when broken.
- First appearance of a variant: banner "NEW: <VARIANT> <TYPE>" with a one-line
  counter (e.g. "HEADSHOT OR PARRY TO BREAK THE HALO"), through the existing
  NEW_TYPE flow extended to (type, variant).

## 2. The Seraph

`EnemyType::SERAPH`: flying, health 120, speed 5. Keeps 10–14 m over its
floor, 18–28 m from the player. Hint: "SERAPHS SWEEP A BEAM TOWARD YOU - KEEP
MOVING OR BREAK LINE OF SIGHT".

Behaviour (`thinkSeraph`):

1. **Drift**: orbits at range, slower and higher than a Raptor; never dives.
2. **Charge, 1.0 s**: stops, wings flare, a thin aim line. The beam's ground
   point starts **5 m to one side of the player**, on the side they're moving
   away from.
3. **Sweep, 3.0 s**: continuous beam from its chest to a ground point that
   tracks the player's feet at **at most 8 m/s**. The beam is cut by walls
   (ray against the level). **30 damage/s** to the player while the beam's
   segment touches the player's box.
4. **Cooldown, 3.5 s**: drifts.

Counters: a hit of ≥ 40 damage or a headshot during the charge cancels it.
A Haloed Seraph's halo breaks on a headshot (its beam can't be parried).

Events: `ev.beamOn`, `ev.beamFrom`, `ev.beamTo` each tick; GameplayState
applies damage, draws the beam with `drawBeams` (thin while charging, thick
pale gold while sweeping), sparks where it lands, steam where it meets water,
and a hiss.

Model: an angular winged rig (two pairs of wings, a ring for a head) in
EnemyModel.h, with a head box for headshots.

## 3. The Anchor

`EnemyType::ANCHOR`: ground, health 260, speed 2.2, radius 1.0, height 2.8.
Hint: "ANCHORS PIN YOU DOWN - NO DASH OR GRAPPLE IN THEIR FIELD".

**Field**: a cylinder of radius **10 m** (13 m when Enraged), 6 m tall, centred on its feet.

- `bool anchoredAt(glm::vec3 feet)` (GameplayState, over live Anchors). While
  true, dash and grapple don't fire: a dull "thunk" and a red flash on the HUD
  dash/grapple indicator instead. An active grapple breaks on entering.
- Slides, jumps and double jumps still work.
- Overlapping fields don't stack beyond coverage.

Behaviour (`thinkAnchor`): walks toward the player until 8 m away, then holds.
Every 2.8 s a telegraphed (1.0 s) volley of **3 slow heavy orbs**, parryable;
a parried orb returns for 120. Turns slowly; uniform armour. Ledge-aware like
a Brute.

Death: the field collapses with a ring burst; style "FIELD BROKEN" +30.

Model: a hunched heavy rig with a ring core on its back. Field drawn as a
pulsing ring of light on the ground at its radius plus faint vertical ribs,
using the existing ring/shockwave drawing (no new shader).

## 4. Where they appear, and the Nave's waves

Act II only. Dev: `--spawn 12` (Seraph), `--spawn 13` (Anchor); the dev level
select reaches them through the Nave.

| Wave | Goal | Mix |
|---|---|---|
| 1 | Kill all | Husks, 3 Haloed Husks, Rippers, 2 Twinned Rippers, Sentinels, a Shieldbearer squad |
| 2 | Conduits on the galleries | Raptors, 2 Seraphs, Brutes (1 Enraged), Conductors, Mites |
| 3 | Kill all | 1 Anchor (first in the queue), Brutes, Twinned Shieldbearer squads, Conductors, 2 Seraphs, a Haloed Juggernaut, Rippers |

Counts are tuned during the build against the simulated run and a play check.

## 5. Testing

Automated (`make test`):

- Enraged moves faster and winds up sooner than the same type plain.
- Twinned reports exactly one split; copies don't split; copies at 35% health.
- Haloed takes ×0.1; a headshot breaks it; a parry breaks it; the ×2 window ends after 2 s.
- Bosses, CONDUIT and CONDUCTOR ignore `.hollow()`.
- Seraph charges before firing; the beam's ground point starts ≥ 4 m from the
  player and moves ≤ 8 m/s; a still player is hit; a player strafing at 7 m/s
  is grazed at most; a wall between blocks the beam; a big hit during the charge cancels it.
- Anchor: `anchoredAt` true inside the cylinder, false outside / 6 m above /
  after it dies; an Enraged Anchor's field is 13 m.
- Both new rigs build with a sane part count; head boxes line up with the model.
- The simulated ACT II run still clears the Nave; every variant and both new types spawn.
- ENDLESS's generator never produces a Seraph, an Anchor or a Hollowed enemy.

Manual: `--shot` of each variant, the Seraph mid-sweep, the Anchor's field;
`--bench` of wave 3 vs the Sanctum (within ~15%); `make web`.

## Out of scope

Revenant and Weaver (piece 5); Hollowed variants in Act I modes; new music;
the ACT II leaderboard.

## Risks

- **Seraph beam fairness at range**: 8 m/s tracking vs 7 m/s walking is tight; wading
  slows you to ~3.9 m/s. Mitigation: deep water is meant to be dangerous, high
  ground is the answer; tune the tracking speed in play if it's oppressive.
- **Anchor + Brutes + deep water** can trap the player. Mitigation: Anchor
  arrives first in wave 3 alone in the queue's head so it can be killed before
  the pile-up; its orbs are parryable.
- **EnemyType enum growth**: per-type tables (stats, rigs, humanoid dims, XP,
  style) must all gain two entries; the model test iterates COUNT, which
  catches a missed table.
